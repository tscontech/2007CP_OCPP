#include "hci_uart.h"
#include "uart/uart.h"
//#define DBG

bool rtk_hci_uart_init(rtk_hci_uart_config *rtk_hci_uart)
{
	UART_OBJ *pUartInfo = (UART_OBJ*)malloc(sizeof(UART_OBJ));
	pUartInfo->port = TEST_ITH_PORT;
	pUartInfo->parity = rtk_hci_uart->parity;
	pUartInfo->txPin = TEST_GPIO_TX;
	pUartInfo->rxPin = TEST_GPIO_RX;
	pUartInfo->baud = rtk_hci_uart->baudrate;
	pUartInfo->timeout = 0;
	pUartInfo->mode = TEST_MODE;
	pUartInfo->forDbg = false;
#ifdef DBG
	printf("rtk_hci_uart->parity: %d\n", rtk_hci_uart->parity);
	printf("rtk_hci_uart->word_length: %d\n", rtk_hci_uart->word_length);
	printf("rtk_hci_uart->baudrate: %d\n", rtk_hci_uart->baudrate);
	printf("rtk_hci_uart->rx_disabled: %d\n", rtk_hci_uart->rx_disabled);
	printf("rtk_hci_uart->tx: %d\n", pUartInfo->txPin);
	printf("rtk_hci_uart->rx: %d\n", pUartInfo->rxPin);
#endif
	itpRegisterDevice(TEST_PORT, &TEST_DEVICE);
	ioctl(TEST_PORT, ITP_IOCTL_INIT, (void*)pUartInfo);
	free(pUartInfo);

	return true;
}

bool rtk_hci_uart_deinit(void)
{
	iteUartTerminate(ITH_UART0);
	return true; 
}

bool rtk_hci_uart_tx(uint8_t *p_buf, uint16_t len, bool(*tx_cb)(void))
{
	/*int i = 0;
	if (printFlag)
	{
		while (len>i)
		{
		printf("uart %d tx: %x, len: %d\n", TEST_PORT, p_buf[i++], i);
		}
	}*/
	
	write(TEST_PORT, p_buf, len);
	if (tx_cb)
		tx_cb();

	return true;
}

bool rtk_hci_uart_rx_disable()
{
	printf("%s, %d\n", __FILE__, __LINE__);
	return true;
}

bool rtk_hci_uart_rx_enable(void)
{
	printf("%s, %d\n", __FILE__, __LINE__);
	return true; 
}
