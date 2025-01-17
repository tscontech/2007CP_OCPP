#include <string.h>

#include "bt_target.h"
#include "gki_common.h"
#include "userial.h"
#include "hcidefs.h"
#include <stdbool.h>
#include <stdint.h>

#include "ite/ith.h"
#include "ite/itp.h"
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"
#include "openrtos/semphr.h"
#include "openrtos/timers.h"
#include "bta_platform.h"


extern taskinfo_t TaskInfoID[GKI_MAX_TASKS];

#define READ_THREAD_PRI             TaskInfoID[USERIAL_TASK].taskPriority
#define READ_THREAD_STACK_SIZE      TaskInfoID[USERIAL_TASK].taskStackSize
#define READ_THREAD_NAME            ((const char *) "hci_uart_task")

#define HCI_TYPE_COMMAND            1
#define HCI_TYPE_ACL_DATA           2
#define HCI_TYPE_SCO_DATA           3
#define HCI_TYPE_EVENT              4

#define OS_WAIT_FOREVER             ((unsigned) 0xffffffff)

#define HCI_LOOPBACK_PREAM_LEN      2
#define HCI_EVENT_PREAM_LEN         3
#define HCI_ACL_PREAM_LEN           5

#define USERIAL_TRUE                1
#define USERIAL_FALSE               0

#define USERIAL_OUT_OF_HEAP_SPACE   2
#define USERIAL_ERROR               1
#define USERIAL_SUCCESS             0

//#define UART_TXRX_VERBOSE           1

#define TEST_DEVICE     itpDeviceUart0
#define TEST_PORT       ITP_DEVICE_UART0
bt_bus_t bt_bus = {
	.info = {
	.port = ITH_UART0,
	.parity = 0,
	.txPin = CFG_GPIO_UART0_TX,
	.rxPin = CFG_GPIO_UART0_RX,
	.baud = CFG_UART0_BAUDRATE,
	.timeout = 0,
#ifdef DMA
	.mode = UART_DMA_MODE, //UART_INTR_MODE , UART_DMA_MODE, UART_FIFO_MODE
#else
	.mode = UART_INTR_MODE, //UART_INTR_MODE , UART_DMA_MODE, UART_FIFO_MODE
#endif
	.forDbg = false,
	},
	.reg_on = CFG_GPIO_BT_REG_ON,
};



typedef bool                        userial_bool_t;
typedef int                         userial_result_t;

userial_result_t bt_bus_init( void );
static userial_result_t bt_bus_deinit( void );
static userial_result_t bt_bus_transmit( const uint8_t* data_out, uint32_t size );
static userial_result_t bt_bus_receive( uint8_t* data_in, uint32_t size, uint32_t timeout_ms );
userial_result_t bt_bus_uart_reconifig_baud(uint32_t baud);


static const UINT32 userial_baud_tbl[] = {
    300, /* USERIAL_BAUD_300          0 */
    600, /* USERIAL_BAUD_600          1 */
    1200, /* USERIAL_BAUD_1200         2 */
    2400, /* USERIAL_BAUD_2400         3 */
    9600, /* USERIAL_BAUD_9600         4 */
    19200, /* USERIAL_BAUD_19200        5 */
    57600, /* USERIAL_BAUD_57600        6 */
    115200, /* USERIAL_BAUD_115200       7 */
    230400, /* USERIAL_BAUD_230400       8 */
    460800, /* USERIAL_BAUD_460800       9 */
    921600, /* USERIAL_BAUD_921600       10 */
    1000000, /* USERIAL_BAUD_1M           11 */
    1500000, /* USERIAL_BAUD_1_5M         12 */
    2000000, /* USERIAL_BAUD_2M           13 */
    3000000, /* USERIAL_BAUD_3M           14 */
    4000000 /* USERIAL_BAUD_4M           15 */
};

TaskHandle_t read_thread_id = NULL ;

#define USERIAL_ASSERT( error_string, assertion )         do { if ( !(assertion) ) printf( ( error_string ) ); } while(0) //do { (void)(assertion); } while(0)

#define VERIFY_RETVAL( x ) \
do \
    { \
        userial_result_t verify_result = (x); \
        if ( verify_result != USERIAL_SUCCESS ) \
{ \
    USERIAL_ASSERT( "bus error", 0!=0 ); \
        return verify_result; \
    } \
} while( 0 )

    /* Macro for checking of bus is initialised */
#define IS_BUS_INITIALISED( ) \
    do \
        { \
            if ( bus_initialised == USERIAL_FALSE ) \
    { \
        USERIAL_ASSERT( "bus uninitialised", 0!=0 ); \
            return USERIAL_ERROR; \
        } \
    }while ( 0 )

        /* Macro for checking if bus is ready */
#define BT_BUS_IS_READY( ) \
        do \
            { \
                if ( bt_bus_is_ready( ) == USERIAL_FALSE ) \
        { \
            USERIAL_ASSERT( "bus not ready", 0!=0 ) \
                return USERIAL_ERROR; \
            } \
        }while ( 0 )

            /* Macro for waiting until bus is ready */
#define BT_BUS_WAIT_UNTIL_READY( ) \
            do \
                { \
                    while ( bt_bus_is_ready( ) == USERIAL_FALSE ) \
            { \
                vTaskDelay( 10 ); \
                } \
            } while ( 0 )


                BUFFER_Q Userial_in_q;
static BT_HDR *pbuf_USERIAL_Read = NULL;

void userial_read_thread( void *p );
UINT8 g_readThreadAlive = 0;

#define UART_RX_EVENT   0x01
/* USERIAL control block */
typedef struct
{
    int fd;
    //bool needPause;
    //bool uartIsBlock;   // uart need to set this flag before select.
    tUSERIAL_CBACK *p_cback;
} tUSERIAL_CB;
tUSERIAL_CB userial_cb = {
    .fd = -1,
    //.needPause = false,
    //.uartIsBlock = false,
.p_cback = 0};

void dumphex(const char * tag,const uint8_t * data, const int len)
{
    printf("%s", tag);
    for(int i = 0; i < len; i++)
    {
        printf("%02x ", data[i] & 0x0ff);

        if((i != 0 )&& ((i % 16) == 15))
        {
            printf("\n");
        }
    }
    printf("\r\n");
}
/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum
{
    HCI_UNINITIALIZED = 0x00, // Uninitialized
    HCI_COMMAND_PACKET = 0x01, // HCI Command packet from Host to Controller
    HCI_ACL_DATA_PACKET = 0x02, // Bidirectional Asynchronous Connection-Less Link (ACL) data packet
    HCI_SCO_DATA_PACKET = 0x03, // Bidirectional Synchronous Connection-Oriented (SCO) link data packet
    HCI_EVENT_PACKET = 0x04, // Events
    HCI_LOOPBACK_MODE = 0xFF,
// Loopback mode
// HCI Event packet from Controller to Host
} hci_packet_type_t;

typedef struct
{
    hci_packet_type_t packet_type; /* This is transport layer packet type. Not transmitted if transport bus is USB */
    uint8_t content_length;
} hci_loopback_packet_header_t;


typedef struct
{
    hci_packet_type_t packet_type; /* This is transport layer packet type. Not transmitted if transport bus is USB */
    uint8_t event_code;
    uint8_t content_length;
} hci_event_header_t;

typedef struct
{
    hci_packet_type_t packet_type; /* This is transport layer packet type. Not transmitted if transport bus is USB */
    uint16_t hci_handle;
    uint16_t content_length;
} hci_acl_packet_header_t;

typedef struct
{
    hci_packet_type_t packet_type; /* This is transport layer packet type. Not transmitted if transport bus is USB */
    uint16_t hci_handle;
    uint8_t  content_length;
} hci_sco_packet_header_t;



static volatile BOOLEAN bus_initialised = USERIAL_FALSE;
static volatile BOOLEAN device_powered  = USERIAL_FALSE;

extern void init_bt_gpio();
extern void bt_rst();


static sem_t UartSemIntr;

static void UartCallback(void* arg1, uint32_t arg2)
{
    sem_post(&UartSemIntr);
}

userial_result_t bt_bus_init( void )
{
    userial_result_t init_result = USERIAL_SUCCESS;
    
    if( bus_initialised == USERIAL_FALSE )
    {
        /* Initial BT Power Pin(output) */
        ithGpioSetOut(bt_bus.reg_on);
        ithGpioSetMode(bt_bus.reg_on, ITH_GPIO_MODE0);
        ithGpioClear(bt_bus.reg_on);     // chipset powerOff
        vTaskDelay(150);
        
        /* PowerOn and waiting chipset standby */
        ithGpioSet(bt_bus.reg_on);
        vTaskDelay(250);
    }
    return USERIAL_SUCCESS;
}

extern uint32_t  bt_power_down(void);
userial_result_t bt_bus_deinit( void )
{
    userial_result_t result = USERIAL_SUCCESS;

    if ( bus_initialised == USERIAL_FALSE )
    {
        return USERIAL_SUCCESS;
    }
    //bt_power_down();
    bus_initialised = USERIAL_FALSE;

    return result;
}

SemaphoreHandle_t uart_tx_sem = NULL;
#define RTOS_WAIT_FOREVER                          (portMAX_DELAY)

userial_result_t bt_bus_transmit( const uint8_t* data_out, uint32_t size )
{
    if (!uart_tx_sem)
        uart_tx_sem = xSemaphoreCreateMutex( );
    xSemaphoreTake(uart_tx_sem, RTOS_WAIT_FOREVER);
    int ret = write(TEST_PORT, data_out, size);
#ifdef UART_TXRX_VERBOSE
    int x = size > 16? 16 : size;
    dumphex("tx-> ", data_out, x);
#endif
    //vTaskDelay(30);
    xSemaphoreGive(uart_tx_sem);
    //return ret == 0 ?  USERIAL_SUCCESS: USERIAL_ERROR;
    return USERIAL_SUCCESS;
}

static userial_result_t bt_bus_receive( uint8_t* data_in, uint32_t size, uint32_t timeout_ms )
{
    int recv_len = 0;
    uint8_t i;
    int tryCount = 60;
    int alreadyRead = 0;
    int remains = size; 
    int read_len;
    IS_BUS_INITIALISED();
    do{
#ifndef DMA
        sem_wait(&UartSemIntr);
        recv_len+=read(TEST_PORT, &data_in[recv_len],size-recv_len);
#else
        read_len = read(TEST_PORT, &data_in[recv_len], size - recv_len);
        if (!read_len)
            vTaskDelay(5);
      
        recv_len += read_len;
#endif
    }while(recv_len<size);
    
#ifdef UART_TXRX_VERBOSE
    int x = size > 16? 16 : size;
    dumphex("rx <- ", data_in, x);
#endif

    return recv_len==size? USERIAL_SUCCESS : USERIAL_ERROR;
}


static uint8_t uart_init(uint32_t baudrate)
{
    
    bt_bus.info.baud = baudrate ; 
    
    /* Open BT Uart */
    itpRegisterDevice(TEST_PORT, &TEST_DEVICE);
    ioctl( TEST_PORT, ITP_IOCTL_INIT, (void*)&bt_bus.info);
#ifndef DMA
    ioctl(TEST_PORT, ITP_IOCTL_REG_UART_CB, (void*)UartCallback);
#endif

#ifndef DMA
    /* Initial Rx semaphore */
    sem_init(&UartSemIntr, 0, 0);
#endif
    bus_initialised = USERIAL_TRUE;
    return 0;
}

static uint8_t uart_deinit(void)
{
    if ( bus_initialised == USERIAL_TRUE )
    {
        //TODO
        return USERIAL_SUCCESS;
    }
    return USERIAL_SUCCESS;
}

userial_result_t bt_bus_uart_reconifig_baud(uint32_t baud)
{
    DRV_TRACE_DEBUG1("uart recofigure baudreat%d",baud);
    GKI_delay(50);

    bt_bus.info.baud = baud ; 
    ioctl( TEST_PORT, ITP_IOCTL_RESET, (void*)&bt_bus.info );
#ifndef DMA
    ioctl( TEST_PORT, ITP_IOCTL_REG_UART_CB, (void*)UartCallback );
#endif

    return  USERIAL_SUCCESS;
}


/*******************************************************************************
 **
 ** Function           USERIAL_GetLineSpeed
 **
 ** Description        This function convert USERIAL baud to line speed.
 **
 ** Output Parameter   None
 **
 ** Returns            line speed
 **
 *******************************************************************************/
UINT32 USERIAL_GetLineSpeed( UINT8 baud )
{
    if ( baud <= USERIAL_BAUD_4M )
        return ( userial_baud_tbl[baud - USERIAL_BAUD_300] );
    else
        return 0;
}

/*******************************************************************************
 **
 ** Function           USERIAL_GetBaud
 **
 ** Description        This function convert line speed to USERIAL baud.
 **
 ** Output Parameter   None
 **
 ** Returns            line speed
 **
 *******************************************************************************/
UDRV_API
extern UINT8 USERIAL_GetBaud( UINT32 line_speed )
{
    UINT8 i;
    for ( i = USERIAL_BAUD_300; i <= USERIAL_BAUD_4M; i++ )
    {
        if ( userial_baud_tbl[i - USERIAL_BAUD_300] == line_speed )
            return i;
    }

    return USERIAL_BAUD_AUTO;
}

/*******************************************************************************
 **
 ** Function           USERIAL_Init
 **
 ** Description        This function initializes the  serial driver.
 **
 ** Output Parameter   None
 **
 ** Returns            Nothing
 **
 *******************************************************************************/

UDRV_API
void USERIAL_Init( void *p_cfg )
{
    DRV_TRACE_DEBUG0("USERIAL_Init");

    //memset(&userial_cb, 0, sizeof(userial_cb));
    GKI_init_q( &Userial_in_q );

    return;
}

void USERIAL_Open_ReadThread( void )
{
    DRV_TRACE_DEBUG0("USERIAL_Open_ReadThread++");

    g_readThreadAlive = 1;
    
    xTaskCreate(&userial_read_thread, READ_THREAD_NAME, READ_THREAD_STACK_SIZE, NULL, READ_THREAD_PRI, &read_thread_id);
    
    DRV_TRACE_DEBUG1("USERIAL_Open_ReadThread id:%x\r\n",read_thread_id);
    
    if(read_thread_id == NULL)
    {
        DRV_TRACE_DEBUG0("USERIAL_Open failed to create userial_read_thread");
        USERIAL_Close( 0 );
        g_readThreadAlive = 0;
        return; 
    }
    
}

void USERIAL_Close_ReadThread( void )
{
    // Close our read thread
    g_readThreadAlive = 0;

    vTaskDelay(500);
    DRV_TRACE_DEBUG0("USERIAL_Close_ReadThread \n");

    // Flush the queue
    do
    {
        pbuf_USERIAL_Read = (BT_HDR *) GKI_dequeue( &Userial_in_q );
        if ( pbuf_USERIAL_Read )
        {
            GKI_freebuf( pbuf_USERIAL_Read );
        }

    } while ( pbuf_USERIAL_Read );
}

/*******************************************************************************
 **
 ** Function           USERIAL_Open
 **
 ** Description        Open the indicated serial port with the given configuration
 **
 ** Output Parameter   None
 **
 ** Returns            Nothing
 **
 *******************************************************************************/

UDRV_API
void USERIAL_Open( tUSERIAL_PORT port, tUSERIAL_OPEN_CFG *p_cfg, tUSERIAL_CBACK *p_cback )
{

    userial_result_t result;

    DRV_TRACE_DEBUG0("USERIAL_Open");

    result = bt_bus_init( );
    if ( result != USERIAL_SUCCESS )
    {
        DRV_TRACE_DEBUG0("USERIAL_Open failed");
        return;
    }

    uart_init(115200);
    USERIAL_Open_ReadThread( );

    userial_cb.p_cback = p_cback;
}

/*******************************************************************************
 **
 ** Function           USERIAL_Read
 **
 ** Description        Read data from a serial port using byte buffers.
 **
 ** Output Parameter   None
 **
 ** Returns            Number of bytes actually read from the serial port and
 **                    copied into p_data.  This may be less than len.
 **
 *******************************************************************************/

UDRV_API
UINT16 USERIAL_Read( tUSERIAL_PORT port, UINT8 *p_data, UINT16 len )
{
    UINT16 total_len = 0;
    UINT16 copy_len = 0;
    UINT8 * current_packet = NULL;

    do
    {
        if ( pbuf_USERIAL_Read != NULL )
        {
            current_packet = ( (UINT8 *) ( pbuf_USERIAL_Read + 1 ) ) + ( pbuf_USERIAL_Read->offset );

            if ( ( pbuf_USERIAL_Read->len ) <= ( len - total_len ) )
                copy_len = pbuf_USERIAL_Read->len;
            else
                copy_len = ( len - total_len );

            memcpy( ( p_data + total_len ), current_packet, copy_len );

            total_len += copy_len;

            pbuf_USERIAL_Read->offset += copy_len;
            pbuf_USERIAL_Read->len -= copy_len;

            if ( pbuf_USERIAL_Read->len == 0 )
            {
                GKI_freebuf( pbuf_USERIAL_Read );
                pbuf_USERIAL_Read = NULL;
            }
        }

        if ( pbuf_USERIAL_Read == NULL )
        {
            pbuf_USERIAL_Read = (BT_HDR *) GKI_dequeue( &Userial_in_q );
//            DRV_TRACE_DEBUG1("USERIAL_Read dequeue %08x", pbuf_USERIAL_Read);
        }
    } while ( ( pbuf_USERIAL_Read != NULL ) && ( total_len < len ) );

//    DRV_TRACE_DEBUG1("USERIAL_Read %i bytes", total_len);

    return total_len;
}

/*******************************************************************************
 **
 ** Function           USERIAL_Readbuf
 **
 ** Description        Read data from a serial port using GKI buffers.
 **
 ** Output Parameter   Pointer to a GKI buffer which contains the data.
 **
 ** Returns            Nothing
 **
 ** Comments           The caller of this function is responsible for freeing the
 **                    GKI buffer when it is finished with the data.  If there is
 **                    no data to be read, the value of the returned pointer is
 **                    NULL.
 **
 *******************************************************************************/
UDRV_API
void USERIAL_ReadBuf( tUSERIAL_PORT port, BT_HDR **p_buf )
{
    if(p_buf == NULL)
        return;

    *p_buf = (BT_HDR *) GKI_dequeue( &Userial_in_q );
    //DRV_TRACE_DEBUG1("USERIAL_ReadBuf dequeue %08x", *p_buf);
}


/*******************************************************************************
 **
 ** Function           USERIAL_WriteBuf
 **
 ** Description        Write data to a serial port using a GKI buffer.
 **
 ** Output Parameter   None
 **
 ** Returns            TRUE  if buffer accepted for write.
 **                    FALSE if there is already a buffer being processed.
 **
 ** Comments           The buffer will be freed by the serial driver.  Therefore,
 **                    the application calling this function must not free the
 **                    buffer.
 **
 *******************************************************************************/
UDRV_API
BOOLEAN USERIAL_WriteBuf( tUSERIAL_PORT port, BT_HDR *p_buf )
{
    return FALSE;
}

/*******************************************************************************
 **
 ** Function           USERIAL_Write
 **
 ** Description        Write data to a serial port using a byte buffer.
 **
 ** Output Parameter   None
 **
 ** Returns            Number of bytes actually written to the serial port.  This
 **                    may be less than len.
 **
 *******************************************************************************/
UDRV_API
UINT16 USERIAL_Write( tUSERIAL_PORT port, UINT8 *p_data, UINT16 len )
{
    userial_result_t result;

//    DRV_TRACE_DEBUG1("USERIAL_Write len=%d", len);
    result = bt_bus_transmit( p_data, len );
    if ( result != USERIAL_SUCCESS )
    {
        DRV_TRACE_DEBUG0("USERIAL_Write failed");
        return 0;
    }
//    DRV_TRACE_DEBUG0("USERIAL_Write success");
    return len;
}

UDRV_API void USERIAL_ChangeBaud(tUSERIAL_PORT port, UINT32 v)
{
    bt_bus_uart_reconifig_baud(v);
}

/*******************************************************************************
 **
 ** Function           USERIAL_Ioctl
 **
 ** Description        Perform an operation on a serial port.
 **
 ** Output Parameter   The p_data parameter is either an input or output depending
 **                    on the operation.
 **
 ** Returns            Nothing
 **
 *******************************************************************************/

UDRV_API
void USERIAL_Ioctl( tUSERIAL_PORT port, tUSERIAL_OP op, tUSERIAL_IOCTL_DATA *p_data )
{

    switch ( op )
    {
        case USERIAL_OP_FLUSH:
            break;
        case USERIAL_OP_FLUSH_RX:
            break;
        case USERIAL_OP_FLUSH_TX:
            break;
        case USERIAL_OP_BAUD_RD:
            break;
        case USERIAL_OP_BAUD_WR:
            //USERIAL_Close_ReadThread( ); // Close read thread before de-initing UART
            /*    printf("change Baud rate event CB (%d)\n", p_data->baud);
                userial_cb.needPause = true;
                while(userial_cb.uartIsBlock == true)
                {
                    rt_thread_mdelay(1);
                }*/
            bt_bus_uart_reconifig_baud(USERIAL_GetLineSpeed(p_data->baud));
            //userial_cb.needPause = false;
            //USERIAL_Open_ReadThread( ); // Start the read thread again
            break;
        default:
            break;
    }

    return;
}

/*******************************************************************************
 **
 ** Function           USERIAL_Close
 **
 ** Description        Close a serial port
 **
 ** Output Parameter   None
 **
 ** Returns            Nothing
 **
 *******************************************************************************/
UDRV_API
void USERIAL_Close( tUSERIAL_PORT port )
{
    userial_result_t result;
    DRV_TRACE_DEBUG0("USERIAL_Close\n");

    USERIAL_Close_ReadThread( );

    uart_deinit();

    vTaskDelay(500);

    result = bt_bus_deinit( );
    if ( result != USERIAL_SUCCESS )
    {
        DRV_TRACE_DEBUG0("USERIAL_Close failed\n");
    }

    DRV_TRACE_DEBUG0("USERIAL_Close--\n");
}

/*******************************************************************************
 **
 ** Function           USERIAL_Feature
 **
 ** Description        Check whether a feature of the serial API is supported.
 **
 ** Output Parameter   None
 **
 ** Returns            TRUE  if the feature is supported
 **                    FALSE if the feature is not supported
 **
 *******************************************************************************/
UDRV_API BOOLEAN USERIAL_Feature( tUSERIAL_FEATURE feature )
{
    return FALSE;
}

#define USERIAL_DROP_BUF_SIZE   20
static void bt_hci_transport_driver_bus_drop_data (uint32_t size)
{
    UINT8       tmp[USERIAL_DROP_BUF_SIZE];
    uint32_t    read_size = size;
    uint32_t    left_size = size;
    while (left_size)
    {
        if (read_size > USERIAL_DROP_BUF_SIZE)
            read_size = USERIAL_DROP_BUF_SIZE;
        bt_bus_receive( (uint8_t*) tmp, read_size, OS_WAIT_FOREVER );
        if (left_size >= read_size)
        {
            left_size -= read_size;
            read_size  = left_size;
        }
    }
}

userial_result_t bt_hci_transport_driver_bus_read_handler( BT_HDR** pp_packet )
{
    hci_packet_type_t packet_type = HCI_UNINITIALIZED;
    UINT8 *current_packet;
    BT_HDR* packet = NULL;
    userial_result_t result = USERIAL_SUCCESS;

    if ( pp_packet == NULL || *pp_packet != NULL)
    {
        DRV_TRACE_ERROR2("%s %d\r\n",__func__,__LINE__);
        GKI_delay(100);    
        return USERIAL_ERROR;
    }

    // Read 1 byte:
    //    packet_type
    // Use a timeout here so we can shutdown the thread
    if ( bt_bus_receive( (uint8_t*) &packet_type, 1, OS_WAIT_FOREVER ) != USERIAL_SUCCESS)
    {
        //DRV_TRACE_ERROR0("bt_bus error reading pkt_type");
        DRV_TRACE_ERROR0("bt_hci_transport_driver_bus_read_handler return USERIAL_ERROR\n");
        return USERIAL_ERROR;
    }

    //DRV_TRACE_DEBUG1("bt_hci_transport_driver_bus_read_handler packet_type=0x%x", packet_type);
    //printf("HCI EVT 0x%04x", packet_type & 0x0ffff);

    switch ( packet_type )
    {
        case HCI_LOOPBACK_MODE:

            ; /* make sure that below declaration is not the first statement after above case */
            hci_loopback_packet_header_t header;

            // Read 1 byte:
            //    content_length
            if ( bt_bus_receive( (uint8_t*) &header.content_length, 1, OS_WAIT_FOREVER ) != USERIAL_SUCCESS )
            {
                DRV_TRACE_ERROR0("bt_bus error loopback header");
                result = USERIAL_ERROR;
                break;
            }
            packet = (BT_HDR *) GKI_getpoolbuf( HCI_ACL_POOL_ID);

            if(!packet)
            {
                bt_hci_transport_driver_bus_drop_data ((uint32_t) header.content_length);
                DRV_TRACE_ERROR0("unable to allocate from GKI ACL Pool (for Loopback)!!");
                result = USERIAL_OUT_OF_HEAP_SPACE;
                break;
            }

            packet->offset = 0;
            packet->layer_specific = 0;
            current_packet = (UINT8 *) ( packet + 1 );
            *current_packet++ = packet_type;
            *current_packet++ = header.content_length;
            packet->len       = header.content_length;

            //DRV_TRACE_DEBUG1("HCI_LOOPBACK_MODE packet->len=0x%x", packet->len);

            // Read payload
            if ( bt_bus_receive( current_packet, packet->len, OS_WAIT_FOREVER ) != USERIAL_SUCCESS )
            {
                DRV_TRACE_DEBUG0("bt_bus_receive FAIL in bt_hci_transport_driver_bus_read_handler");
                result = USERIAL_ERROR;
                break;
            }

            packet->len = packet->len + 2; // +2 for packet type + read length
            break;
            
        case HCI_EVENT_PACKET:
        {
            hci_event_header_t header;

            // Read 2 bytes:
            //    event_code
            //    content_length
            if ( bt_bus_receive( (uint8_t*) &header.event_code, 2, OS_WAIT_FOREVER ) != USERIAL_SUCCESS )
            {
                result = USERIAL_ERROR;
                break;
            }

            packet = (BT_HDR *) GKI_getpoolbuf(HCI_CMD_POOL_ID);
            if(!packet)
            {
                bt_hci_transport_driver_bus_drop_data ((uint32_t) header.content_length);
                DRV_TRACE_ERROR0("unable to allocate from GKI CMD Pool!!");
                result = USERIAL_OUT_OF_HEAP_SPACE;
                break;
            }

            packet->offset = 0;
            packet->layer_specific = 0;
            current_packet = (UINT8 *) ( packet + 1 );
            *current_packet++ = packet_type;
            *current_packet++ = header.event_code;
            *current_packet++ = header.content_length;
            packet->len = header.content_length + 3; // +3 to include sizeof: packet_type=1 event_code=1 content_length=1

            if(header.content_length > 280)
            {
                DRV_TRACE_ERROR1("length error !!!! %d\n", header.content_length & 0x0FFFF);
                result = USERIAL_ERROR;
                break;
            }
            // Read payload
            if ( bt_bus_receive( (uint8_t*) current_packet, (uint32_t) header.content_length, OS_WAIT_FOREVER ) != USERIAL_SUCCESS )
            {
                result = USERIAL_ERROR;
                break;
            }
            //DRV_TRACE_ERROR0("EVENT PACKETl!!");

            break;
        }

        case HCI_ACL_DATA_PACKET:
        {
            hci_acl_packet_header_t header;

            // Read 4 bytes:
            //    hci_handle (2 bytes)
            //    content_length (2 bytes)
            if ( bt_bus_receive( (uint8_t*) &header.hci_handle, HCI_DATA_PREAMBLE_SIZE, OS_WAIT_FOREVER ) != USERIAL_SUCCESS )
            {
                DRV_TRACE_ERROR0("bt_bus error acl header");
                result = USERIAL_ERROR;
                break;
            }

            packet = (BT_HDR *) GKI_getpoolbuf(HCI_ACL_POOL_ID);
            if(!packet)
            {
                bt_hci_transport_driver_bus_drop_data ((uint32_t) header.content_length);
                DRV_TRACE_ERROR0("unable to allocate from GKI ACL Pool!!");
                result = USERIAL_OUT_OF_HEAP_SPACE;
                break;
            }

            packet->offset = 0;
            packet->layer_specific = 0;
            current_packet = (UINT8 *) ( packet + 1 );
            UINT8_TO_STREAM(current_packet, packet_type);
            UINT16_TO_STREAM(current_packet, header.hci_handle);
            UINT16_TO_STREAM(current_packet, header.content_length);

            packet->len = header.content_length + 5; // +5 to include sizeof: packet_type=1 hci_handle=2 content_length=2

            if (header.content_length > (HCI_ACL_POOL_BUF_SIZE - BT_HDR_SIZE))
            {
                printf("bt_bus error invalid acl len %i, handle %04x", header.content_length, header.hci_handle);
                result = USERIAL_ERROR;
                // dump all data in driver then sleep forever
                //void dump_uart_buffer(int fd);
                //dump_uart_buffer(userial_cb.fd);
                //while(1) vTaskDelay(100);

                break;
            }
            // Read payload
            if ( bt_bus_receive( (uint8_t*) current_packet, (uint32_t) header.content_length, OS_WAIT_FOREVER ) != USERIAL_SUCCESS )
            {
                DRV_TRACE_ERROR1("bt_bus read failed, handle %d\n", header.content_length);
                result = USERIAL_ERROR;
                break;
            }
            //DRV_TRACE_ERROR0("ACL Success!!");
            break;
        }
#if 1
        case HCI_SCO_DATA_PACKET:
        {
            hci_sco_packet_header_t header;

            // Read 4 bytes:
            //    hci_handle (2 bytes)
            //    content_length (1 bytes)
            if ( bt_bus_receive( (uint8_t*) &header.hci_handle, HCI_SCO_PREAMBLE_SIZE, OS_WAIT_FOREVER ) != USERIAL_SUCCESS )
            {
                DRV_TRACE_ERROR0("bt_bus error sco header");
                result = USERIAL_ERROR;
                break;
            }

            packet = (BT_HDR *) GKI_getpoolbuf(HCI_SCO_POOL_ID);
            if(!packet)
            {
                bt_hci_transport_driver_bus_drop_data ((uint32_t) header.content_length);
                DRV_TRACE_ERROR0("unable to allocate from GKI SCO Pool!!");
                result = USERIAL_OUT_OF_HEAP_SPACE;
                break;
            }

            packet->offset = 0;
            packet->layer_specific = 0;
            current_packet = (UINT8 *) ( packet + 1 );
            UINT8_TO_STREAM(current_packet, packet_type);
            UINT16_TO_STREAM(current_packet, header.hci_handle);
            UINT8_TO_STREAM(current_packet, header.content_length);

            packet->len = header.content_length + 4; // +4 to include sizeof: packet_type=1 hci_handle=2 content_length=1

            // Read payload
            if ( bt_bus_receive( (uint8_t*) current_packet, (uint32_t) header.content_length, OS_WAIT_FOREVER ) != USERIAL_SUCCESS )
            {
                result = USERIAL_ERROR;
                break;
            }

            break;
        }
#endif
        case HCI_COMMAND_PACKET: /* Fall-through */
        default:
            DRV_TRACE_ERROR1("unknow packet 0x%x!",packet_type);
            result = USERIAL_ERROR;
            break;
    }

    if(result != USERIAL_SUCCESS)
    {
        if(packet != NULL)
            GKI_freebuf(packet);
        packet = NULL;
    }

    *pp_packet = packet;

    return result;
}


/*******************************************************************************
 **
 ** Function           userial_read_thread
 **
 ** Description        userial_read_thread
 **
 ** Output Parameter   None
 **
 ** Returns            None
 **
 *******************************************************************************/
void userial_read_thread( void *p )
{
    userial_result_t ret;

    DRV_TRACE_DEBUG0("userial_read_thread running.\n");
    GKI_delay(10);
    
    while ( g_readThreadAlive )
    {
        BT_HDR *p_buf = NULL;        
        ret = bt_hci_transport_driver_bus_read_handler( &p_buf );
        if(ret == USERIAL_SUCCESS)
        {
            GKI_enqueue( &Userial_in_q, p_buf );

            if (userial_cb.p_cback)
            {
                (userial_cb.p_cback)(0, USERIAL_RX_READY_EVT, NULL);
            }
        }
        else if (ret == USERIAL_OUT_OF_HEAP_SPACE)
        {
            GKI_delay(500);
        }
    }
    DRV_TRACE_DEBUG0("userial_read_thread exit \n");
}

