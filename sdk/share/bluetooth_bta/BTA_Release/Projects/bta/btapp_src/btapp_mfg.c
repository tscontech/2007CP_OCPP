/****************************************************************************
**
**  Name:          btapp_mfg.c
**
**  Description:   Contains btapp mfg for RF test
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"

#define DEFAULT_READ_TIMEOUT      1000
#define DEFAULT_MAX_READ_TIMEOUT  3000
#define STABILIZATION_DELAY       250

extern const char brcm_patch_version[];
extern const uint8_t brcm_patchram_buf[];
extern const int brcm_patch_ram_length_bt;
extern const uint8_t brcm_patchrabrcm_patchram_format_btm_format;

/* HCI Transport Layer Packet Type */
typedef enum
{
    HCI_COMMAND_PACKET  = 0x01, // HCI Command packet from Host to Controller
    HCI_ACL_DATA_PACKET = 0x02, // Bidirectional Asynchronous Connection-Less Link (ACL) data packet
    HCI_SCO_DATA_PACKET = 0x03, // Bidirectional Synchronous Connection-Oriented (SCO) link data packet
    HCI_EVENT_PACKET    = 0x04, // HCI Event packet from Controller to Host
} hci_packet_type_t;

typedef enum
{
    HCI_CMD_RESET,
    HCI_CMD_DOWNLOAD_MINIDRIVER,
    HCI_CMD_WRITE_RAM,
    HCI_CMD_LAUNCH_RAM,
    HCI_CMD_READ_BD_ADDR,
    HCI_CMD_WRITE_BD_ADDR,
} hci_command_type_t;

typedef enum
{
    HCI_CMD_OPCODE_RESET               = 0x0C03,
    HCI_CMD_OPCODE_DOWNLOAD_MINIDRIVER = 0xFC2E,
    HCI_CMD_OPCODE_WRITE_RAM           = 0xFC4C,
    HCI_CMD_OPCODE_LAUNCH_RAM          = 0xFC4E,
    HCI_CMD_OPCODE_READ_BD_ADDR        = 0x1009,
    HCI_CMD_OPCODE_WRITE_BD_ADDR       = 0xFC01,
} hci_command_opcode_t;

#pragma pack(1)
typedef struct
{
    hci_packet_type_t packet_type; /* This is transport layer packet type. Not transmitted if transport bus is USB */
    uint16_t          op_code;
    uint8_t           content_length;
} hci_command_header_t;

typedef struct
{
    hci_packet_type_t packet_type; /* This is transport layer packet type. Not transmitted if transport bus is USB */
    uint8_t           event_code;
    uint8_t           content_length;
} hci_event_header_t;

typedef struct
{
    hci_event_header_t header;
    uint8_t            total_packets;
    uint16_t           op_code;
    uint8_t            status;
} hci_event_extended_header_t;
#pragma pack()

#ifndef BT_HCI_COMMAND_PACKET_COUNT
#define BT_HCI_COMMAND_PACKET_COUNT (4)
#endif /* BT_HCI_COMMAND_PACKET_COUNT */

#ifndef BT_HCI_EVENT_PACKET_COUNT
#define BT_HCI_EVENT_PACKET_COUNT   (4)
#endif /* HCI_EVENT_PACKET_COUNT */

#ifndef BT_HCI_ACL_PACKET_COUNT
#define BT_HCI_ACL_PACKET_COUNT     (8)
#endif /* BT_HCI_ACL_PACKET_COUNT */

#define BT_HCI_COMMAND_DATA_SIZE    (256)    /* Maximum HCI command parameters total length */
#define BT_HCI_EVENT_DATA_SIZE      (256)    /* Maximum HCI event parameters total length   */

#ifndef BT_HCI_ACL_DATA_SIZE
#define BT_HCI_ACL_DATA_SIZE        (23 + 4) /* ATT MTU size + L2CAP header size            */
#endif /* BT_HCI_ACL_DATA_SIZE */

#define BT_HCI_COMMAND_HEADER_SIZE  ( sizeof( hci_command_header_t ) )
#define BT_HCI_EVENT_HEADER_SIZE    ( sizeof( hci_event_header_t) )
#define BT_HCI_ACL_HEADER_SIZE      ( sizeof( hci_acl_packet_header_t ) )

static const hci_command_header_t hci_commands[] =
{
    [HCI_CMD_RESET]               = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_RESET,               .content_length = 0x0 },
    [HCI_CMD_DOWNLOAD_MINIDRIVER] = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_DOWNLOAD_MINIDRIVER, .content_length = 0x0 },
    [HCI_CMD_WRITE_RAM]           = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_WRITE_RAM,           .content_length = 0x0 },
    [HCI_CMD_LAUNCH_RAM]          = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_LAUNCH_RAM,          .content_length = 0x0 },
};

static const hci_event_extended_header_t expected_hci_events[] =
{
    [HCI_CMD_RESET]               = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_RESET,               .status = 0x0 },
    [HCI_CMD_DOWNLOAD_MINIDRIVER] = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_DOWNLOAD_MINIDRIVER, .status = 0x0 },
    [HCI_CMD_WRITE_RAM]           = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_WRITE_RAM,           .status = 0x0 },
    [HCI_CMD_LAUNCH_RAM]          = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_LAUNCH_RAM,          .status = 0x0 },
};

static uint8_t m_bt_mfg_init = 0;

#define VERIFY_RETVAL( x ) \
do \
{ \
    uint8_t verify_result = (x); \
    if ( verify_result <= 0 ) \
    { \
        BRCM_PLATFORM_TRACE( "BT bus error\r\n"); \
        return verify_result; \
    } \
} while( 0 )

#define VERIFY_RESPONSE( a, b, size ) \
{ \
    if ( memcmp( (a), (b), (size) ) != 0 ) \
    { \
        BRCM_PLATFORM_TRACE( "HCI unexpected response\r\n"); \
        return -1; \
    } \
}

#define VERIFY_BT_MFG_INITED()    \
do \
{ \
    if(m_bt_mfg_init == 0) \
    {\
        BRCM_PLATFORM_TRACE( "ERROR bt_mfg not init yet\r\n"); \
        return -1; \
    } \
} while(0)

static int8_t bt_hci_reset_ex(void);
static int8_t execute_le_transmitter_test(uint8_t chan_number, uint8_t length, uint8_t pattern);
static int8_t execute_le_enhanced_transmitter_test(uint8_t chan_number, uint8_t length, uint8_t pattern, uint8_t phy);
static int8_t execute_radio_tx_test(const char *bdaddr, uint8_t frequency, uint8_t modulation_type, uint8_t logical_channel, uint8_t bb_packet_type, uint32_t packet_length, uint8_t tx_power);

static void print_usage_le_transmitter_test(void);
static void print_usage_radio_tx_test(void);
static void print_usage_le_enhanced_transmitter_test(void);

int8_t bt_firmware_download( const uint8_t* firmware_image, uint32_t size, const char* version )
{
    uint8_t* data = (uint8_t*) firmware_image;
    uint32_t remaining_length = size;
    hci_event_extended_header_t hci_event;
    int8_t status;

    if(pf_trans_get_status() != PF_TRANS_STATUS_READY)
    {
        return -1;
    }

    status = bt_hci_reset_ex();
    if(status != BTA_SUCCESS)
    {
        return -1;
    }

    BRCM_PLATFORM_TRACE( "Downloading Bluetooth firmware:%s\r\n", version);
    /* Send hci_download_minidriver command */
    VERIFY_RETVAL( pf_trans_send( (const uint8_t* ) &hci_commands[ HCI_CMD_DOWNLOAD_MINIDRIVER ], sizeof(hci_command_header_t) ) );
    VERIFY_RETVAL( pf_trans_receive( (uint8_t*) &hci_event, sizeof( hci_event ), DEFAULT_READ_TIMEOUT ) );
    VERIFY_RESPONSE( &hci_event, &expected_hci_events[ HCI_CMD_DOWNLOAD_MINIDRIVER ], sizeof( hci_event ) );


    /* The firmware image (.hcd format) contains a collection of hci_write_ram command + a block of the image,
     * followed by a hci_write_ram image at the end. Parse and send each individual command and wait for the response.
     * This is to ensure the integrity of the firmware image sent to the bluetooth chip.
     */
    while ( remaining_length )
    {
        uint32_t data_length = data[ 2 ] + 3; /* content of data length + 2 bytes of opcode and 1 byte of data length */
        uint8_t residual_data = 0;
        hci_command_opcode_t command_opcode = *(hci_command_opcode_t*) data;
        uint8_t temp_data[ 256 ];

        memset( &hci_event, 0, sizeof( hci_event ) );
        memset( temp_data, 0, sizeof( temp_data ) );

        /* 43438 requires the packet type before each write RAM command */
        temp_data[ 0 ] = HCI_COMMAND_PACKET;
        memcpy( &temp_data[ 1 ], data, data_length );

        /* Send hci_write_ram command. The length of the data immediately follows the command opcode */
        VERIFY_RETVAL( pf_trans_send( (const uint8_t* ) temp_data, data_length + 1 ) );
        VERIFY_RETVAL( pf_trans_receive( (uint8_t*) &hci_event, sizeof(hci_event), DEFAULT_READ_TIMEOUT ) );

        switch ( command_opcode )
        {
            case HCI_CMD_OPCODE_WRITE_RAM:
                VERIFY_RESPONSE( &hci_event, &expected_hci_events[ HCI_CMD_WRITE_RAM ], sizeof( hci_event ) );

                /* Update remaining length and data pointer */
                data += data_length;
                remaining_length -= data_length;
                break;

            case HCI_CMD_OPCODE_LAUNCH_RAM:
                VERIFY_RESPONSE( &hci_event, &expected_hci_events[ HCI_CMD_LAUNCH_RAM ], sizeof( hci_event ) );

                BRCM_PLATFORM_TRACE( "Firmware download complete\r\n");
                remaining_length = 0;

                break;

            default:
                BRCM_PLATFORM_TRACE("Bluetooth firmware parsing error\r\n");
                return -1;
        }
    }

    /* Wait for bluetooth chip to pull its RTS (host's CTS) low. From observation using CRO, it takes the bluetooth chip > 170ms to pull its RTS low after CTS low */
    rt_thread_mdelay( STABILIZATION_DELAY );

    status = bt_hci_reset_ex();
    if(status != BTA_SUCCESS)
    {
        return -1;
    }
    return 0;
}

int8_t bt_mfg_init( int32_t argc, const char *argv[] )
{
    int8_t result;
    int8_t status;

    UNUSED(argc);
    UNUSED(argv);

    if(m_bt_mfg_init == 0)
    {
        UPIO_Init(NULL);

        /* reset bt controller*/
        UPIO_Set(UPIO_GENERAL, BT_REG_ON_GPIO, UPIO_OFF);
        rt_thread_mdelay(50);
        UPIO_Set(UPIO_GENERAL, BT_REG_ON_GPIO, UPIO_ON);
        rt_thread_mdelay(50);

        /* Initialize the platform UART interface to Bluetooth radio */
        result = pf_trans_init(115200);
        if(result != 0)
        {
            BRCM_PLATFORM_TRACE("Bluetooth bus initialization failed\r\n");
            return -1;
        }

        result = bt_firmware_download( brcm_patchram_buf, brcm_patch_ram_length, brcm_patch_version );
        if ( result != 0 )
        {
            BRCM_PLATFORM_TRACE("Error downloading Bluetooth firmware\r\n");
            return -1;
        }

        m_bt_mfg_init = 1;
    }
    else
    {
        BRCM_PLATFORM_TRACE("bt_mfg_init has done!\r\n");
    }

    return 0;
}

int8_t bt_mfg_deinit( int32_t argc, const char *argv[] )
{
    UNUSED(argc);
    UNUSED(argv);

    if(m_bt_mfg_init == 1)
    {
        pf_trans_deinit();

        UPIO_Set(UPIO_GENERAL, BT_REG_ON_GPIO, UPIO_OFF);
        UPIO_Set(UPIO_GENERAL, HCILP_BT_WAKE_GPIO, UPIO_OFF);
        UPIO_DeInit();
        m_bt_mfg_init = 0;

        BRCM_PLATFORM_TRACE("SUCCESS\n");
    }
    else
    {
        BRCM_PLATFORM_TRACE("bt_mfg_deinit has done\r\n");
    }

    return 0;
}

static void bt_hci_display(uint8_t *data, uint8_t len)
{
    uint8_t index = 0;
    uint8_t value;

    BRCM_PLATFORM_TRACE( " \n----------------------------------\n");
    for (index = 0 ; index < len ; index++)
    {
        value = *(data+index);
        BRCM_PLATFORM_TRACE("0x%02x ", value);
    }
    BRCM_PLATFORM_TRACE( " \n----------------------------------\n");
}

int8_t bt_mfg_send_hci( uint8_t *cmd, uint8_t len, \
                                            uint8_t *res, uint8_t res_len )
{
    hci_event_extended_header_t hci_event;
    char hci_command[20];
    char hci_response[256];
    uint8_t index = 0;
    int8_t status = 0;

    for (index = 0 ; index < len ; index++)
    {
        hci_command[index] = *(cmd+index);
    }

    /* Flush the UART RX by a dummy read */
    bt_bus_receive( (uint8_t* ) &hci_response, sizeof( hci_response ), 1 );

    pf_trans_send( (const uint8_t* ) hci_command, len );

    pf_trans_receive( (uint8_t* ) &hci_event, sizeof( hci_event ), DEFAULT_READ_TIMEOUT );

    if (memcmp(&hci_event, res, 7) == 0)
    {
        status = 0;
    }
    else
    {
        status = -1;
    }

    return status;
}

int8_t bt_mfg_hci_send_any(int32_t argc, const char *argv[])
{
    uint8_t hci_command[256] = {0};
    uint8_t hci_response[256] = {0};
    hci_event_header_t hci_evt;
    uint8_t hci_rcv = 0, hci_has_rcved = 0;
    uint8_t* p_parm = (uint8_t*)argv[0];
    uint16_t pos = 0, idx = 0;

    VERIFY_BT_MFG_INITED();

    if(argc != 1 || argv[0] == NULL)
    {
        BRCM_PLATFORM_TRACE( "ERROR\n");
        return -1;
    }
    else
    {
        while(pos < strlen(p_parm) && idx < sizeof(hci_command))
        {
            unsigned int val = 0;
            if (sscanf(p_parm + pos, "%02x", &val) != 1)
            {
                BRCM_PLATFORM_TRACE("Invalid format\n");
                return -1;
            }
            hci_command[idx++] = val;
            pos += 3;
        }

        if(idx >= 4)
        {
            BRCM_PLATFORM_TRACE( "\nsend any hci:");
            bt_hci_display(hci_command, idx);

            /* Flush the UART RX by a dummy read */
            bt_bus_receive( (uint8_t* ) &hci_response, sizeof( hci_response ), 1 );

            pf_trans_send( (const uint8_t* ) hci_command, idx );

            /* Read HCI EVENT header first */
            hci_rcv = pf_trans_receive( (uint8_t* ) &hci_evt, sizeof( hci_evt ), DEFAULT_READ_TIMEOUT );
            if(hci_rcv != -1 && hci_rcv == sizeof(hci_evt))
            {
                memcpy(&hci_response[0], &hci_evt, sizeof(hci_evt));
                hci_has_rcved = hci_rcv;

                /* Try to read the next content */
                hci_rcv = pf_trans_receive( (uint8_t* ) &hci_response[hci_has_rcved], hci_evt.content_length, DEFAULT_READ_TIMEOUT );
                if(hci_rcv != -1 && hci_rcv == hci_evt.content_length)
                {
                    hci_has_rcved += hci_rcv;
                    BRCM_PLATFORM_TRACE( "\nhci_rcv: counts:%d, content:", hci_has_rcved);
                    bt_hci_display(hci_response, hci_has_rcved);
                }
                else
                {
                    BRCM_PLATFORM_TRACE( "ERROR rcv hci data fail\n");
                    return -1;
                }
            }
            else
            {
                BRCM_PLATFORM_TRACE( "ERROR rcv hci data fail\n");
                return -1;
            }
        }
        else
        {
            BRCM_PLATFORM_TRACE( "ERROR Invalid length\n");
            return -1;
        }
    }

    BRCM_PLATFORM_TRACE("SUCCESS\n");

    return 0;
}

int8_t bt_hci_reset( int32_t argc, const char *argv[] )
{
    VERIFY_BT_MFG_INITED();

    return bt_hci_reset_ex();
}

static int8_t bt_hci_reset_ex(void)
{
    uint8_t hci_reset[] = {0x01, 0x03, 0x0c, 0x00};
    uint8_t hci_reset_cmd_complete_event[] = {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00};
    uint8_t result;

    BRCM_PLATFORM_TRACE( "send HCI_RESET!\n");
    bt_hci_display(hci_reset, sizeof(hci_reset));

    result = bt_mfg_send_hci(hci_reset, sizeof(hci_reset),
                hci_reset_cmd_complete_event, sizeof(hci_reset_cmd_complete_event));
    if(result != 0)
    {
        result = bt_mfg_send_hci(hci_reset, sizeof(hci_reset),
                hci_reset_cmd_complete_event, sizeof(hci_reset_cmd_complete_event));
        if(result != 0)
        {
            BRCM_PLATFORM_TRACE( "ERROR\n");
            return -1;
        }
    }
    BRCM_PLATFORM_TRACE( "SUCCESS\n");
    return 0;
}

int8_t bt_le_tx_test( int32_t argc, const char* argv[] )
{
    uint8_t chan_num = 0;
    uint8_t pattern = 0;
    uint8_t length = 0;

    VERIFY_BT_MFG_INITED();

    if (argc != 3)
    {
        print_usage_le_transmitter_test();
        return -1;
    }

    chan_num = atoi(argv[0]);
    if (chan_num <= 39)
    {
        length = atoi(argv[1]);
        pattern = atoi(argv[2]);
        if (pattern < 7)
        {
            return (execute_le_transmitter_test(chan_num, length, pattern));
        }
    }
    print_usage_le_transmitter_test();

    return 0;
}

int8_t bt_le_test_end( int32_t argc, const char* argv[] )
{
    uint8_t result;

    uint8_t hci_le_test_end[] = {0x01, 0x1f, 0x20, 0x00};
    uint8_t hci_le_test_end_cmd_complete_event[] = {0x04, 0x0e, 0x06, 0x01, \
                                                    0x1f, 0x20, 0x00};
	uint8_t hci_le_test_end_cmd_complete_data[2];

    VERIFY_BT_MFG_INITED();

    BRCM_PLATFORM_TRACE ("Send LE test end HCI Command:\n");
    bt_hci_display(hci_le_test_end, sizeof(hci_le_test_end));
    /* write HCI */
    result = bt_mfg_send_hci(hci_le_test_end, sizeof(hci_le_test_end), \
                                hci_le_test_end_cmd_complete_event, \
                                sizeof(hci_le_test_end_cmd_complete_event));
    if(result != 0)
    {
        BRCM_PLATFORM_TRACE( "ERROR \n");
        return -1;
    }

	/* Receive 2 more pending bytes from the command complete event */
	bt_bus_receive(&hci_le_test_end_cmd_complete_data[0], sizeof(hci_le_test_end_cmd_complete_data), DEFAULT_READ_TIMEOUT );
	BRCM_PLATFORM_TRACE("SUCCESS num_packets_received %d\n", hci_le_test_end_cmd_complete_data[0] + ((uint16_t)hci_le_test_end_cmd_complete_data[1] << 8));

    return 0;
}

int8_t bt_radio_tx_test( int32_t argc, const char* argv[] )
{
    uint32_t frequency;
    uint8_t modulation_type;
    uint8_t logical_channel;
    uint8_t bb_packet_type;
    uint32_t packet_length;
    uint8_t tx_power;
    uint8_t modulation_type_mapping[] = { 0x1, //  0x00 8-bit Pattern
                                          0x2, // 0xFF 8-bit Pattern
                                          0x3, // 0xAA 8-bit Pattern
                                          0x9, // 0xF0 8-bit Pattern
                                          0x4  // PRBS9 Pattern
                                         };

    VERIFY_BT_MFG_INITED();

    if (argc != 7)
    {
        print_usage_radio_tx_test();
        return -1;
    }

    if(strlen(argv[0])==12)
    {
        frequency = atoi(argv[1]);
        if ((frequency == 0) || ((frequency >= 2402) && (frequency <= 2480)))
        {
            modulation_type = atoi(argv[2]);
            if (modulation_type <= 4)
            {
                logical_channel = atoi(argv[3]);
                if (logical_channel <= 1)
                {
                     bb_packet_type = atoi(argv[4]);
                     if ((bb_packet_type >= 3) && (bb_packet_type <= 15))
                     {
                         packet_length = atoi(argv[5]);
                         if (packet_length <= 0xffff)
                         {
                             tx_power = atoi(argv[6]);
                             if (tx_power <= 7)
                             {
                                 return execute_radio_tx_test(argv[0], frequency, modulation_type_mapping[modulation_type], logical_channel, bb_packet_type, packet_length, tx_power);
                             }
                         }
                     }
                }
            }
        }
    }
    print_usage_radio_tx_test();

    return -1;
}

static void print_usage_le_transmitter_test(void)
{
    BRCM_PLATFORM_TRACE ("Usage: ble_tx <tx_channel> <data_length> <packet_payload>\n");
    BRCM_PLATFORM_TRACE ("                tx_channel = Range: 0 - 39\n");
    BRCM_PLATFORM_TRACE ("                data_length: (0 - 255)\n");
    BRCM_PLATFORM_TRACE ("                data_pattern: (0 - 6)\n");
    BRCM_PLATFORM_TRACE ("                    0 Pseudo-Random bit sequence 9\n");
    BRCM_PLATFORM_TRACE ("                    1 Pattern of alternating bits '11110000'\n");
    BRCM_PLATFORM_TRACE ("                    2 Pattern of alternating bits '10101010'\n");
    BRCM_PLATFORM_TRACE ("                    3 Pseudo-Random bit sequence 15\n");
    BRCM_PLATFORM_TRACE ("                    4 Pattern of All '1' bits\n");
    BRCM_PLATFORM_TRACE ("                    5 Pattern of All '0' bits\n");
    BRCM_PLATFORM_TRACE ("                    6 Pattern of alternating bits '00001111'\n");
}

static int8_t execute_le_transmitter_test(uint8_t chan_number, uint8_t length, uint8_t pattern)
{
    uint8_t result;
    uint8_t hci_le_transmitter_test[] = {0x01, 0x01E, 0x20, 0x03, 0x00, 0x00, 0x00};
    uint8_t hci_le_transmitter_test_cmd_complete_event[] = {0x04, 0x0e, 0x04, 0x01, \
                                                            0x01E, 0x20, 0x00};
    hci_le_transmitter_test[4] = chan_number;
    hci_le_transmitter_test[5] = length;
    hci_le_transmitter_test[6] = pattern;

    BRCM_PLATFORM_TRACE( "Send LE TX test command\n");

    bt_hci_display(hci_le_transmitter_test, sizeof(hci_le_transmitter_test));
    result = bt_mfg_send_hci(hci_le_transmitter_test, sizeof(hci_le_transmitter_test),
                             hci_le_transmitter_test_cmd_complete_event, \
                             sizeof(hci_le_transmitter_test_cmd_complete_event));
    if(result != 0)
    {
        BRCM_PLATFORM_TRACE( "ERROR\n");
        return -1;
    }
    BRCM_PLATFORM_TRACE( "SUCCESS\n");
    return 0;
}

static int8_t execute_radio_tx_test(const char *bdaddr, uint8_t frequency, uint8_t modulation_type, uint8_t logical_channel, uint8_t bb_packet_type, uint32_t packet_length, uint8_t tx_power)
{
    int params[6];
    uint8_t i;
    uint8_t result;

    sscanf(bdaddr, "%02x%02x%02x%02x%02x%02x", &params[0], &params[1], &params[2], \
                                               &params[3], &params[4], &params[5]);
#if 0
    uint8_t hci_write_bd_addr_cmd_complete_event[] = { 0x04, 0xe, 0x04, 0x01, \
                                                       0x01, 0xFC, 0x00 };
    uint8_t hci_write_bd_addr[] = { 0x01, 0x01, 0xFC, 0x06, 0, 0, 0, 0, 0, 0 };
    for(i = 0; i < 6; i++ )
    {
        hci_write_bd_addr[i+4] = params[5-i];
    }

    bt_hci_display(hci_write_bd_addr, sizeof(hci_write_bd_addr));
    result = bt_mfg_send_hci(hci_write_bd_addr, sizeof(hci_write_bd_addr),\
                                hci_write_bd_addr_cmd_complete_event, \
                                sizeof(hci_write_bd_addr_cmd_complete_event));
    if(result != 0)
    {
        BRCM_PLATFORM_TRACE( "ERROR\n");
        return ACE_STATUS_IO_ERROR;
    }
    BRCM_PLATFORM_TRACE( "SUCCESS\n");
#endif

    uint8_t hci_radio_tx_test[20] = {0x01, 0x051, 0xfc, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t hci_radio_tx_test_cmd_complete_event[] = {0x04, 0x0e, 0x04, 0x01, 0x051, 0xfc, 0x00};

    for(i = 0; i < 6; i++ )
    {
        hci_radio_tx_test[i+4] = params[5-i];    //bd address
    }
    hci_radio_tx_test[10] = (frequency==0) ? 0 : 1;        //0: hopping, 1: single frequency
    hci_radio_tx_test[11] = (frequency==0) ? 0 : (frequency - 2402);  //0: hopping 0-79:channel number (0: 2402 MHz)
    hci_radio_tx_test[12] = modulation_type;               //data pattern (3: 0xAA  8-bit Pattern)
    hci_radio_tx_test[13] = logical_channel;               //logical_Channel (0:ACL EDR, 1:ACL Basic)
    hci_radio_tx_test[14] = bb_packet_type;                //modulation type (BB_Packet_Type. 3:DM1, 4: DH1 / 2-DH1)
    hci_radio_tx_test[15] = packet_length & 0xff;          //low byte of packet_length
    hci_radio_tx_test[16] = (packet_length>>8) & 0xff;     //high byte of packet_length
    hci_radio_tx_test[17] = 9;                             // 8 for power in dBm, 9 for power table index
    hci_radio_tx_test[18] = 0;                              //dBm value
    hci_radio_tx_test[19] = tx_power;                       //power table index

    bt_hci_display(hci_radio_tx_test, sizeof(hci_radio_tx_test));
    result = bt_mfg_send_hci(hci_radio_tx_test, sizeof(hci_radio_tx_test),\
                                hci_radio_tx_test_cmd_complete_event, \
                                sizeof(hci_radio_tx_test_cmd_complete_event));
    if(result != 0)
    {
        BRCM_PLATFORM_TRACE( "ERROR\n");
        return -1;
    }
    BRCM_PLATFORM_TRACE( "SUCCESS\n");
    return 0;
}

static void print_usage_radio_tx_test(void)
{
    BRCM_PLATFORM_TRACE ("Usage: bt_tx <bd_addr> <frequency> <modulation_type> <logical_channel> <bb_packet_type> <packet_length> <power_table_index>\n");
    BRCM_PLATFORM_TRACE ("                bd_addr: BD_ADDR of Tx device (6 bytes, no space between bytes)\n");
    BRCM_PLATFORM_TRACE ("                frequency: 0 for hopping or (2402 - 2480) transmit frequency in MHz\n");
    BRCM_PLATFORM_TRACE ("                    0: normal Bluetooth hopping sequence (79 channels)\n");
    BRCM_PLATFORM_TRACE ("                    2402 - 2480: single frequency without hopping\n");
    BRCM_PLATFORM_TRACE ("                modulation_type: sets the data pattern\n");
    BRCM_PLATFORM_TRACE ("                    0: 0x00 8-bit Pattern\n");
    BRCM_PLATFORM_TRACE ("                    1: 0xFF 8-bit Pattern\n");
    BRCM_PLATFORM_TRACE ("                    2: 0xAA 8-bit Pattern\n");
    BRCM_PLATFORM_TRACE ("                    3: 0xF0 8-bit Pattern\n");
    BRCM_PLATFORM_TRACE ("                    4: PRBS9 Pattern\n");
    BRCM_PLATFORM_TRACE ("                logical_channel: sets the logical channel to Basic Rate (BR) or Enhanced Data Rate (EDR) for ACL packets\n");
    BRCM_PLATFORM_TRACE ("                    0: EDR\n");
    BRCM_PLATFORM_TRACE ("                    1: BR\n");
    BRCM_PLATFORM_TRACE ("                bb_packet_type: baseband packet type to use\n");
    BRCM_PLATFORM_TRACE ("                    3: DM1\n");
    BRCM_PLATFORM_TRACE ("                    4: DH1 / 2-DH1\n");
    BRCM_PLATFORM_TRACE ("                    8: 3-DH1\n");
    BRCM_PLATFORM_TRACE ("                    10: DM3 / 2-DH3\n");
    BRCM_PLATFORM_TRACE ("                    11: DH3 / 3-DH3\n");
    BRCM_PLATFORM_TRACE ("                    12: EV4 / 2-EV5\n");
    BRCM_PLATFORM_TRACE ("                    13: EV5 / 3-EV5\n");
    BRCM_PLATFORM_TRACE ("                    14: DM5 / 2-DH5\n");
    BRCM_PLATFORM_TRACE ("                    15: DH5 / 3-DH5\n");
    BRCM_PLATFORM_TRACE ("                packet_length: 0 - 65535. Device will limit the length to the max for the baseband packet type\n");
    BRCM_PLATFORM_TRACE ("                power_table_index = (0 - 7) as indexes into chip power table\n");
}

static int8_t execute_le_enhanced_transmitter_test(uint8_t chan_number, uint8_t length, uint8_t pattern, uint8_t phy)
{
    uint8_t result;
    uint8_t hci_le_enhanced_transmitter_test[] = {0x01, 0x34, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00};
    uint8_t hci_le_enhanced_transmitter_test_cmd_complete_event[] = {0x04, 0x0e, 0x04, 0x01, 0x34, 0x20, 0x00};

    hci_le_enhanced_transmitter_test[4] = chan_number;
    hci_le_enhanced_transmitter_test[5] = length;
    hci_le_enhanced_transmitter_test[6] = pattern;
    hci_le_enhanced_transmitter_test[7] = phy;

    BRCM_PLATFORM_TRACE( "Send LE Enhanced TX test command\n");

    bt_hci_display(hci_le_enhanced_transmitter_test, sizeof(hci_le_enhanced_transmitter_test));
    result = bt_mfg_send_hci(hci_le_enhanced_transmitter_test, sizeof(hci_le_enhanced_transmitter_test),
                             hci_le_enhanced_transmitter_test_cmd_complete_event, \
                             sizeof(hci_le_enhanced_transmitter_test_cmd_complete_event));
    if(result != 0)
    {
        BRCM_PLATFORM_TRACE( "ERROR\n");
        return -1;
    }
    BRCM_PLATFORM_TRACE( "SUCCESS\n");
    return 0;
}

static void print_usage_le_enhanced_transmitter_test(void)
{
    BRCM_PLATFORM_TRACE ("Usage: ble_enhanced_tx <tx_channel> <data_length> <data_pattern> <PHY>\n");
    BRCM_PLATFORM_TRACE ("                tx_channel = (F - 2402) / 2\n");
    BRCM_PLATFORM_TRACE ("                    Range: 0 - 39. Frequency Range : 2402 MHz to 2480 MHz\n");
    BRCM_PLATFORM_TRACE ("                data_length: (0 - 255)\n");
    BRCM_PLATFORM_TRACE ("                data_pattern: (0 - 6)\n");
    BRCM_PLATFORM_TRACE ("                    0 Pseudo-Random bit sequence 9\n");
    BRCM_PLATFORM_TRACE ("                    1 Pattern of alternating bits '11110000'\n");
    BRCM_PLATFORM_TRACE ("                    2 Pattern of alternating bits '10101010'\n");
    BRCM_PLATFORM_TRACE ("                    3 Pseudo-Random bit sequence 15\n");
    BRCM_PLATFORM_TRACE ("                    4 Pattern of All '1' bits\n");
    BRCM_PLATFORM_TRACE ("                    5 Pattern of All '0' bits\n");
    BRCM_PLATFORM_TRACE ("                    6 Pattern of alternating bits '00001111'\n");
    BRCM_PLATFORM_TRACE ("                PHY: (1 - 2)\n");
    BRCM_PLATFORM_TRACE ("                    1 Transmitter set to transmit data at 1Ms/s\n");
    BRCM_PLATFORM_TRACE ("                    2 Transmitter set to transmit data at 2Ms/s\n");
}

int8_t bt_le_enhanced_tx_test( int32_t argc, const char* argv[] )
{
    uint8_t chan_num = 0;
    uint8_t pattern = 0;
    uint8_t length = 0;
    uint8_t phy = 0;

    VERIFY_BT_MFG_INITED();

    if (argc != 4)
    {
        print_usage_le_enhanced_transmitter_test();
        return -1;
    }

    chan_num = atoi(argv[0]);
    if (chan_num <= 39)
    {
        length = atoi(argv[1]);
        pattern = atoi(argv[2]);
        if (pattern < 7)
        {
            phy = atoi(argv[3]);
            if(phy < 3)
            {
                return (execute_le_enhanced_transmitter_test(chan_num, length, pattern, phy));
            }
        }
    }
    print_usage_le_enhanced_transmitter_test();

    return 0;
}

static void print_usage_le_enhanced_receiver_test(void)
{
    BRCM_PLATFORM_TRACE ("Usage: ble_enhanced_rx <rx_channel> <PHY> <modulation_index>\n");
    BRCM_PLATFORM_TRACE ("                rx_channel = (F - 2402) / 2\n");
    BRCM_PLATFORM_TRACE ("                    Range: 0 - 39. Frequency Range : 2402 MHz to 2480 MHz\n");
    BRCM_PLATFORM_TRACE ("                PHY: (1 - 2)\n");
    BRCM_PLATFORM_TRACE ("                    1 Transmitter set to transmit data at 1Ms/s\n");
    BRCM_PLATFORM_TRACE ("                    2 Transmitter set to transmit data at 2Ms/s\n");
    BRCM_PLATFORM_TRACE ("                modulation_index: (1 - 2)\n");
    BRCM_PLATFORM_TRACE ("                    1 Assume transmitter will have a standard modulation index\n");
    BRCM_PLATFORM_TRACE ("                    2 Assume transmitter will have a stable modulation index\n");
}

static int8_t execute_le_enhanced_receiver_test(uint8_t chan_number, uint8_t phy, uint8_t nodulation_index)
{
	uint8_t result;
    uint8_t hci_le_enhanced_receiver_test[] = {0x01, 0x33, 0x20, 0x03, 0x00, 0x00, 0x00};
    uint8_t hci_le_enhanced_receiver_test_cmd_complete_event[] = {0x04, 0x0e, 0x04, 0x01, 0x33, 0x20, 0x00};

    hci_le_enhanced_receiver_test[4] = chan_number;
    hci_le_enhanced_receiver_test[5] = phy;
    hci_le_enhanced_receiver_test[6] = nodulation_index;

    BRCM_PLATFORM_TRACE( "Send LE Enhanced RX test command\n");

    bt_hci_display(hci_le_enhanced_receiver_test, sizeof(hci_le_enhanced_receiver_test));
    result = bt_mfg_send_hci(hci_le_enhanced_receiver_test, sizeof(hci_le_enhanced_receiver_test),
                             hci_le_enhanced_receiver_test_cmd_complete_event, \
                             sizeof(hci_le_enhanced_receiver_test_cmd_complete_event));
    if(result != 0)
    {
        BRCM_PLATFORM_TRACE( "ERROR\n");
        return -1;
    }
    BRCM_PLATFORM_TRACE( "SUCCESS\n");
    return 0;
}

int8_t bt_le_enhanced_rx_test( int32_t argc, const char* argv[] )
{
    uint8_t chan_num = 0;
    uint8_t modulation_index = 0;
    uint8_t phy = 0;

    VERIFY_BT_MFG_INITED();

    if (argc != 3)
    {
        print_usage_le_enhanced_receiver_test();
        return -1;
    }

    chan_num = atoi(argv[0]);
    if (chan_num <= 39)
    {
        phy = atoi(argv[1]);
        if(phy < 3)
        {
            modulation_index = atoi(argv[2]);
            if(modulation_index < 3)
            {
                return (execute_le_enhanced_receiver_test(chan_num, phy, modulation_index));
            }
        }
    }
    print_usage_le_enhanced_receiver_test();

    return 0;
}

static void print_usage_radio_rx_test(void)
{
    BRCM_PLATFORM_TRACE ("Usage: bt_rx <bd_addr> <frequency> <modulation_type> <logical_channel> <bb_packet_type> <packet_length> <test_period>\n");
    BRCM_PLATFORM_TRACE ("                bd_addr: BD_ADDR of Tx device (6 bytes)\n");
    BRCM_PLATFORM_TRACE ("                frequency = (2402 - 2480) receive frequency in MHz\n");
    BRCM_PLATFORM_TRACE ("                modulation_type: sets the data pattern\n");
    BRCM_PLATFORM_TRACE ("                    0: 0x00 8-bit Pattern\n");
    BRCM_PLATFORM_TRACE ("                    1: 0xFF 8-bit Pattern\n");
    BRCM_PLATFORM_TRACE ("                    2: 0xAA 8-bit Pattern\n");
    BRCM_PLATFORM_TRACE ("                    3: 0xF0 8-bit Pattern\n");
    BRCM_PLATFORM_TRACE ("                    4: PRBS9 Pattern\n");
    BRCM_PLATFORM_TRACE ("                logical_channel: sets the logical channel to Basic Rate (BR) or Enhanced Data Rate (EDR) for ACL packets\n");
    BRCM_PLATFORM_TRACE ("                    0: EDR\n");
    BRCM_PLATFORM_TRACE ("                    1: BR\n");
    BRCM_PLATFORM_TRACE ("                bb_packet_type: baseband packet type to use\n");
    BRCM_PLATFORM_TRACE ("                    3: DM1\n");
    BRCM_PLATFORM_TRACE ("                    4: DH1 / 2-DH1\n");
    BRCM_PLATFORM_TRACE ("                    8: 3-DH1\n");
    BRCM_PLATFORM_TRACE ("                    10: DM3 / 2-DH3\n");
    BRCM_PLATFORM_TRACE ("                    11: DH3 / 3-DH3\n");
    BRCM_PLATFORM_TRACE ("                    12: EV4 / 2-EV5\n");
    BRCM_PLATFORM_TRACE ("                    13: EV5 / 3-EV5\n");
    BRCM_PLATFORM_TRACE ("                    14: DM5 / 2-DH5\n");
    BRCM_PLATFORM_TRACE ("                    15: DH5 / 3-DH5\n");
    BRCM_PLATFORM_TRACE ("                packet_length: 0 - 65535. Device will limit the length to the max for the baseband packet type\n");
    BRCM_PLATFORM_TRACE ("                test_period : 1 ~ 200. Total test time(sec)\n");
}

static int execute_radio_rx_test(const char *bdaddr, uint8_t frequency, uint8_t modulation_type, \
									uint8_t logical_channel, uint8_t bb_packet_type, uint32_t packet_length, uint8_t test_period)
{
    int params[6];
    uint8_t i;
    uint8_t result;
    uint8_t in_buffer[255];

    sscanf(bdaddr, "%02x%02x%02x%02x%02x%02x", &params[0], &params[1], &params[2], &params[3], &params[4], &params[5]);
    uint8_t hci_radio_rx_test[] = {0x01, 0x52, 0xfc, 0x0e, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t hci_radio_rx_test_cmd_complete_event[] = {0x04, 0x0e, 0x04, 0x01, 0x52, 0xfc, 0x00};

    for( i = 0; i < 6; i++ )
    {
        hci_radio_rx_test[i+4] = params[5-i];
    }
    hci_radio_rx_test[10] = 0xe8;                          //low byte of report period in ms (1sec = 1000ms, 0x03e8)
    hci_radio_rx_test[11] = 0x03;                          //high byte
    hci_radio_rx_test[12] = frequency - 2402;
    hci_radio_rx_test[13] = modulation_type;               //data pattern (3: 0xAA 8-bit Pattern)
    hci_radio_rx_test[14] = logical_channel;               //logical_Channel (0:ACL EDR, 1:ACL Basic)
    hci_radio_rx_test[15] = bb_packet_type;                //modulation type (BB_Packet_Type. 3:DM1, 4: DH1 / 2-DH1)
    hci_radio_rx_test[16] = packet_length & 0xff;          //low byte of packet_length
    hci_radio_rx_test[17] = (packet_length>>8) & 0xff;     //high byte of packet_length

    bt_hci_display(hci_radio_rx_test, sizeof(hci_radio_rx_test));
    result = bt_mfg_send_hci(hci_radio_rx_test, sizeof(hci_radio_rx_test),\
                    hci_radio_rx_test_cmd_complete_event, \
                    sizeof(hci_radio_rx_test_cmd_complete_event));
    if(result != 0)
    {
        BRCM_PLATFORM_TRACE( "ERROR\n");
        return -1;
    }
    BRCM_PLATFORM_TRACE("\nRadio RX Test is running.\n");

    /*loop and handle the Rx Test statistics report */
    for ( i=0 ; i < test_period ; i++)
    {
        BRCM_PLATFORM_TRACE("Statistics monitoring %d sec!\n",i);
        /* read statistics report*/
        result = bt_bus_receive(in_buffer, 36, DEFAULT_MAX_READ_TIMEOUT);
        if (result == 0)
        {
            BRCM_PLATFORM_TRACE("Statistics Report received:\n");
            bt_hci_display(in_buffer, 36);

            if ((in_buffer[0]==0x04 && in_buffer[1]==0xFF && in_buffer[2]==0x21 && in_buffer[3]==0x07))
            {
                BRCM_PLATFORM_TRACE ("  [Rx Test statistics]\n");
                BRCM_PLATFORM_TRACE ("    Sync_Timeout_Count:     0x%x\n",in_buffer[4]|in_buffer[5]<<8|in_buffer[6]<<16|in_buffer[7]<<24);
                BRCM_PLATFORM_TRACE ("    HEC_Error_Count:        0x%x\n",in_buffer[8]|in_buffer[9]<<8|in_buffer[10]<<16|in_buffer[11]<<24);
                BRCM_PLATFORM_TRACE ("    Total_Received_Packets: 0x%x\n",in_buffer[12]|in_buffer[13]<<8|in_buffer[14]<<16|in_buffer[15]<<24);
                BRCM_PLATFORM_TRACE ("    Good_Packets:           0x%x\n",in_buffer[16]|in_buffer[17]<<8|in_buffer[18]<<16|in_buffer[19]<<24);
                BRCM_PLATFORM_TRACE ("    CRC_Error_Packets:      0x%x\n",in_buffer[20]|in_buffer[21]<<8|in_buffer[22]<<16|in_buffer[23]<<24);
                BRCM_PLATFORM_TRACE ("    Total_Received_Bits:    0x%x\n",in_buffer[24]|in_buffer[25]<<8|in_buffer[26]<<16|in_buffer[27]<<24);
                BRCM_PLATFORM_TRACE ("    Good_Bits:              0x%x\n",in_buffer[28]|in_buffer[29]<<8|in_buffer[30]<<16|in_buffer[31]<<24);
                BRCM_PLATFORM_TRACE ("    Error_Bits:             0x%x\n",in_buffer[32]|in_buffer[33]<<8|in_buffer[34]<<16|in_buffer[35]<<24);
            }
        }
    }

    bt_hci_reset_ex();

    /* flush uart data */
    while(result == 0)
    {
        result = bt_bus_receive(in_buffer, 1, DEFAULT_READ_TIMEOUT);
    }
    return 0;
}

int8_t bt_radio_rx_test( int32_t argc, const char* argv[] )
{
    uint32_t frequency;
    uint8_t modulation_type;
    uint8_t logical_channel;
    uint8_t bb_packet_type;
    uint32_t packet_length;
    uint8_t test_period;
    uint8_t modulation_type_mapping[] = { 0x1, //  0x00 8-bit Pattern
                                          0x2, // 0xFF 8-bit Pattern
                                          0x3, // 0xAA 8-bit Pattern
                                          0x9, // 0xF0 8-bit Pattern
                                          0x4  // PRBS9 Pattern
                                         };
    VERIFY_BT_MFG_INITED();

    if (argc != 7)
    {
        print_usage_radio_rx_test();
        return -1;
    }

    if(strlen(argv[0])==12)
    {
        frequency = atoi(argv[1]);
        if ((frequency >= 2402) && (frequency <= 2480))
        {
            modulation_type = atoi(argv[2]);
            if ((modulation_type <= 4))
            {
                logical_channel = atoi(argv[3]);
                if ((logical_channel <= 1))
                {
                    bb_packet_type = atoi(argv[4]);
                    if ((bb_packet_type >= 3) && (bb_packet_type <= 15))
                    {
                        packet_length = atoi(argv[5]);
                        if ((packet_length <= 0xffff))
                        {
                            test_period = atoi(argv[6]);
                            if((test_period <= 200))
                            {
                                return execute_radio_rx_test(argv[0], frequency, modulation_type_mapping[modulation_type], logical_channel, bb_packet_type, packet_length, test_period);
                            }
                        }
                    }
                }
            }
        }
    }
    print_usage_radio_rx_test();

    return 0;
}

