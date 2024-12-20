/******************************************************************************
 * Copyright (c) 2013-2016 Realtek Semiconductor Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/


#ifndef WLANCONFIG_H
#define WLANCONFIG_H

/*
 * Include user defined options first. Anything not defined in these files
 * will be set to standard values. Override anything you dont like!
 */
#include "../platform/include/rtwlan_config.h"

/*
 * Public General Config ******************************************************
 */

#define CONFIG_WLAN 1

#ifndef DRV_NAME
    #define DRV_NAME        					"undefine"
#endif

#ifndef DRIVERVERSION
    #define DRIVERVERSION						"undefine"
#endif

#ifndef CONFIG_PLATFOMR_CUSTOMER_RTOS
    //#define PLATFORM_FREERTOS
#else
    //#define PLATFORM_CUSTOMER_RTOS
	//#define CONFIG_LWIP_LAYER       			0
#endif

#ifndef CONFIG_LWIP_LAYER
	#define CONFIG_LWIP_LAYER       			1
#endif

#ifndef CONFIG_LITTLE_ENDIAN
    #define CONFIG_LITTLE_ENDIAN    			1 // 0 for CONFIG_BIG_ENDIAN
#endif


/*
 * Performance and Size Related Config ****************************************
 */
//CONFIG_THROUGHPUT_LEVEL must set according to customer plaftorm or device, not means that the value of CONFIG_THROUGHPUT_LEVEL is bigger, the throughput is higher.
//CONFIG_THROUGHPUT_LEVEL == 1  MAX_TX_SKB_DATA_NUM=7 MAX_RX_SKB_DATA_NUM=1
//CONFIG_THROUGHPUT_LEVEL == 2  MAX_TX_SKB_DATA_NUM=15 MAX_RX_SKB_DATA_NUM=1  if no log "Wait for tx skbdata", no need to use CONFIG_THROUGHPUT_LEVEL 2
//CONFIG_THROUGHPUT_LEVEL == 3  MAX_TX_SKB_DATA_NUM=30 MAX_RX_SKB_DATA_NUM=1  if no log "Wait for tx skbdata", no need to use CONFIG_THROUGHPUT_LEVEL 3
//CONFIG_THROUGHPUT_LEVEL == 4  MAX_TX_SKB_DATA_NUM=15 MAX_RX_SKB_DATA_NUM=REORDER_WSIZE CONFIG_TX_BUS_AGG_NUM=3 CONFIG_RX_BUS_AGG_NUM=5 RX_AGGREGATION=1 CONFIG_RECV_REORDERING_CTRL=1
//CONFIG_THROUGHPUT_LEVEL == 5  MAX_TX_SKB_DATA_NUM=7 MAX_RX_SKB_DATA_NUM=REORDER_WSIZE CONFIG_TX_BUS_AGG_NUM=5 CONFIG_RX_BUS_AGG_NUM=8 RX_AGGREGATION=1 CONFIG_RECV_REORDERING_CTRL=1
#if !defined(CONFIG_THROUGHPUT_LEVEL) || (CONFIG_THROUGHPUT_LEVEL == 0)
#undef CONFIG_THROUGHPUT_LEVEL
#define CONFIG_THROUGHPUT_LEVEL					1   //1  2  3  4  5
#endif

#if !defined(CONFIG_TX_BUS_AGG_NUM) || (CONFIG_TX_BUS_AGG_NUM == 0)
#if (CONFIG_THROUGHPUT_LEVEL == 4)
#define CONFIG_TX_BUS_AGG_NUM                   3
#elif (CONFIG_THROUGHPUT_LEVEL == 5)
#define CONFIG_TX_BUS_AGG_NUM                   5
#else
#define CONFIG_TX_BUS_AGG_NUM                   1  //1  3   5   8
#endif
#endif

#if !defined(CONFIG_RX_BUS_AGG_NUM) || (CONFIG_RX_BUS_AGG_NUM == 0)
#if (CONFIG_THROUGHPUT_LEVEL == 4)
#define CONFIG_RX_BUS_AGG_NUM                   5  
#elif (CONFIG_THROUGHPUT_LEVEL == 5)
#define CONFIG_RX_BUS_AGG_NUM                   8  
#else
#define CONFIG_RX_BUS_AGG_NUM                   1  //1  3  5  8
#endif
#endif

#if (!defined(RX_AGGREGATION) || !RX_AGGREGATION) && (CONFIG_RX_BUS_AGG_NUM > 1)
	#define RX_AGGREGATION                      1
#endif
#if RX_AGGREGATION && (!defined(CONFIG_RECV_REORDERING_CTRL))
	#define CONFIG_RECV_REORDERING_CTRL         1
#endif
#if !RX_AGGREGATION && CONFIG_RECV_REORDERING_CTRL
	#error "no need to enable CONFIG_RECV_REORDERING_CTRL"
#endif

#ifndef USE_XMIT_EXTBUFF
	#define USE_XMIT_EXTBUFF					0
#endif

#ifndef CONFIG_RX_SHORTCUT
	#define CONFIG_RX_SHORTCUT					0
#endif

// remove function to reduce code
#if !defined(CONFIG_HARDWARE_8821C)
	#define NOT_SUPPORT_5G						1
#else
	#define NOT_SUPPORT_5G						0
#endif

#define SUPPORT_5G_CHANNEL				(NOT_SUPPORT_5G ? 0 : 1)

#if !defined(CONFIG_HARDWARE_8192E) && !defined(CONFIG_HARDWARE_8192F)
	#define NOT_SUPPORT_RF_MULTIPATH		1
#else
	#define NOT_SUPPORT_RF_MULTIPATH		0
#endif

#ifndef NOT_SUPPORT_VHT
	#define NOT_SUPPORT_VHT						1
#endif
#ifndef NOT_SUPPORT_80M
	#define NOT_SUPPORT_80M						1
#endif
#ifndef NOT_SUPPORT_40M
	#define NOT_SUPPORT_40M						1
#endif
#ifndef NOT_SUPPORT_OLD_CHANNEL_PLAN
	#define NOT_SUPPORT_OLD_CHANNEL_PLAN		1
#endif
#ifndef NOT_SUPPORT_BT
	#define NOT_SUPPORT_BT						1
#endif


/*
 * Wi-Fi Functions Config *****************************************************
 */
#ifndef CONFIG_IEEE80211W
	#define CONFIG_IEEE80211W					0
#endif

#ifndef CONFIG_80211N_HT
	#define CONFIG_80211N_HT					1
#endif

/* for 802.11-K,V,R */
#ifndef CONFIG_IEEE80211K
	#define CONFIG_IEEE80211K 					0
#endif
#ifndef CONFIG_LAYER2_ROAMING
	#define CONFIG_LAYER2_ROAMING 				0
    #if (CONFIG_RTW_WNM || CONFIG_IEEE80211R)
        #error "CONFIG_LAYER2_ROAMING has not been defined, define CONFIG_LAYER2_ROAMING firstly"
    #endif
#endif
#ifndef CONFIG_RTW_WNM
	#define CONFIG_RTW_WNM 						0
#endif
#ifndef CONFIG_IEEE80211R
	#define CONFIG_IEEE80211R 					0
#endif

#ifndef CONFIG_RECV_REORDERING_CTRL
	#define CONFIG_RECV_REORDERING_CTRL			0
#endif

#if CONFIG_RECV_REORDERING_CTRL && !defined(REORDER_WSIZE)
	#define REORDER_WSIZE 						32
#endif

#if CONFIG_RECV_REORDERING_CTRL && !defined(REORDER_WAIT_TIME)
	#define REORDER_WAIT_TIME					(30) // (ms)
#endif

#ifndef CONFIG_POWER_SAVING
	#define CONFIG_POWER_SAVING					0
#endif

#if CONFIG_POWER_SAVING
	#ifndef CONFIG_IPS
		#define CONFIG_IPS						1
	#endif
	#if CONFIG_IPS 
	    #define CONFIG_IPS_LEVEL_2              0//0:normal ips; 1: keepfwalive
	#endif
	#ifndef CONFIG_LPS
		#define CONFIG_LPS						1
	#endif
	#if CONFIG_LPS && !defined(CONFIG_LPS_CHK_BY_TP)
		#define CONFIG_LPS_CHK_BY_TP			1
	#endif
	#ifndef CONFIG_LPS_LCLK
		#define CONFIG_LPS_LCLK					0
	#endif
	#if CONFIG_LPS_LCLK && !defined(CONFIG_DETECT_CPWM_BY_POLLING)
		#define CONFIG_DETECT_CPWM_BY_POLLING	1
	#endif
	#if CONFIG_LPS_LCLK && !defined(LPS_RPWM_WAIT_MS)
		#define LPS_RPWM_WAIT_MS 				300
	#endif
	#ifndef CONFIG_WAIT_PS_ACK
		#define CONFIG_WAIT_PS_ACK				1
	#endif
	#ifndef CONFIG_FW_PSTIMEOUT
		#define CONFIG_FW_PSTIMEOUT				1
	#endif
#else
	#if CONFIG_IPS || CONFIG_LPS
		#error "CONFIG_POWER_SAVING has not been defined, define CONFIG_POWER_SAVING firstly"
	#endif
#endif
#if !CONFIG_LPS && CONFIG_LPS_LCLK
	#error "CONFIG_LPS has not been defined, define CONFIG_LPS firstly"
#endif
#if !CONFIG_LPS_LCLK && CONFIG_DETECT_CPWM_BY_POLLING
	#error "CONFIG_LPS_LCLK has not been defined, define CONFIG_LPS_LCLK firstly"
#endif

#ifndef CONFIG_AUTO_RECONNECT
	#define CONFIG_AUTO_RECONNECT				1
#endif

/* For WPA2 */
#ifndef CONFIG_INCLUDE_WPA_PSK
	#define CONFIG_INCLUDE_WPA_PSK				1
#endif

#if CONFIG_INCLUDE_WPA_PSK
	#ifndef CONFIG_MULTIPLE_WPA_STA
		#define CONFIG_MULTIPLE_WPA_STA			1
	#endif
	#ifndef CONFIG_WPA2_PREAUTH
		#define CONFIG_WPA2_PREAUTH				0
	#endif
	#ifndef PSK_SUPPORT_TKIP
		#define PSK_SUPPORT_TKIP				1
	#endif
	#ifndef CONFIG_PTK_REKEY
		#define CONFIG_PTK_REKEY				0
	#endif
	#if CONFIG_PTK_REKEY
		#define CONFIG_XMIT_ACK
	#endif
#else
	#if CONFIG_MULTIPLE_WPA_STA || PSK_SUPPORT_TKIP || CONFIG_PTK_REKEY
		#error "CONFIG_INCLUDE_WPA_PSK has not been defined, define CONFIG_INCLUDE_WPA_PSK firstly"
	#endif
#endif

/* For promiscuous mode */
#ifndef CONFIG_PROMISC
	#define CONFIG_PROMISC						0
#endif

#ifndef PROMISC_DENY_PAIRWISE
	#define PROMISC_DENY_PAIRWISE				0
#endif

/* For Simple Link */
#ifndef CONFIG_INCLUDE_SIMPLE_CONFIG
	#define CONFIG_INCLUDE_SIMPLE_CONFIG		0
#endif

// for probe request with custom vendor specific IE
#ifndef CONFIG_CUSTOM_IE
	#define CONFIG_CUSTOM_IE					1
#endif

/* For multicast */
#ifndef CONFIG_MULTICAST
	#define CONFIG_MULTICAST					0
#endif

/* For STA+AP Concurrent MODE */
#ifndef CONFIG_CONCURRENT_MODE
	#define CONFIG_CONCURRENT_MODE				0
#endif

#if CONFIG_CONCURRENT_MODE
	#ifndef CONFIG_MCC_MODE
		#define CONFIG_MCC_MODE					0
	#endif
	#ifndef CONFIG_RUNTIME_PORT_SWITCH
		#if defined(CONFIG_HARDWARE_8723D) || defined(CONFIG_HARDWARE_8188F)
			#define CONFIG_RUNTIME_PORT_SWITCH	1
		#else
			#define CONFIG_RUNTIME_PORT_SWITCH	0
		#endif
	#endif
#else
	#if CONFIG_RUNTIME_PORT_SWITCH || CONFIG_MCC_MODE
		#error "CONFIG_CONCURRENT_MODE has not been defined, define CONFIG_CONCURRENT_MODE firstly"
	#endif
#endif

#ifndef NET_IF_NUM
	#define NET_IF_NUM 							(CONFIG_CONCURRENT_MODE ? 2 : 1)
#endif

/******** For EAP auth configurations *********/
#ifndef PRE_CONFIG_EAP
	#define PRE_CONFIG_EAP 						0
#endif

// DO NOT change the below config of EAP
#ifndef CONFIG_TLS
	#define CONFIG_TLS 							(PRE_CONFIG_EAP ? 1 : 0)
#endif
#ifndef CONFIG_PEAP
	#define CONFIG_PEAP 						(PRE_CONFIG_EAP ? 1 : 0)
#endif
#ifndef CONFIG_TTLS
	#define CONFIG_TTLS 						(PRE_CONFIG_EAP ? 1 : 0)
#endif

// enable 1X code in lib_wlan as default (increase 380 bytes)

#ifndef CONFIG_EAP
	#define CONFIG_EAP 							0
#endif

#ifndef EAP_REMOVE_UNUSED_CODE
	#define EAP_REMOVE_UNUSED_CODE				((CONFIG_TLS || CONFIG_PEAP || CONFIG_TTLS) ? 1 : 0)
#endif

#ifndef EAP_SSL_VERIFY_SERVER
	#define EAP_SSL_VERIFY_SERVER 				1
#endif

#if CONFIG_TLS &&  !defined(EAP_SSL_VERIFY_CLIENT)
	#define EAP_SSL_VERIFY_CLIENT 				1
#endif

#if CONFIG_TTLS
	#ifndef EAP_MSCHAPv2
		#define EAP_MSCHAPv2 					1
	#endif
	#ifndef EAP_TTLS_MSCHAPv2
		#define EAP_TTLS_MSCHAPv2 				1
	#endif
	#ifndef EAP_TTLS_EAP
		#define EAP_TTLS_EAP 					0
	#endif
	#ifndef EAP_TTLS_MSCHAP
		#define EAP_TTLS_MSCHAP 				0
	#endif
	#ifndef EAP_TTLS_PAP
		#define EAP_TTLS_PAP 					0
	#endif
	#ifndef EAP_TTLS_CHAP
		#define EAP_TTLS_CHAP 					0
	#endif
#else
	#if EAP_MSCHAPv2 || EAP_TTLS_MSCHAPv2
		#error "CONFIG_TTLS has not been defined, define CONFIG_TTLS firstly"
	#endif
#endif
/******** End of EAP configurations *********/

#ifndef SUPPORT_SCAN_BUF
	#define SUPPORT_SCAN_BUF 					1
#endif

/* For WPS and P2P */
#ifndef CONFIG_WPS
	#define CONFIG_WPS 							0
#endif

#ifndef CONFIG_WPS_AP
	#define CONFIG_WPS_AP 						0
#endif
#ifndef CONFIG_P2P_NEW
	#define CONFIG_P2P_NEW 						0
#endif
#if (!SUPPORT_SCAN_BUF || !CONFIG_WPS_AP) && CONFIG_P2P_NEW
	#error "If CONFIG_P2P_NEW, need to SUPPORT_SCAN_BUF"
#endif

/* For AP_MODE */
#ifndef CONFIG_AP_MODE
	#define CONFIG_AP_MODE 						1
#endif
#if (CONFIG_AP_MODE == 1) && (CONFIG_THROUGHPUT_LEVEL == 1)
#undef CONFIG_THROUGHPUT_LEVEL
#define CONFIG_THROUGHPUT_LEVEL					3   //ap mode default use 3
#endif

extern unsigned char g_user_ap_sta_num;
extern unsigned int g_ap_sta_num;
#define USER_AP_STA_NUM							g_user_ap_sta_num
#if !defined(AP_STA_NUM)
	#define AP_STA_NUM								3 // g_ap_sta_num
#endif
#if CONFIG_AP_MODE
	#ifndef CONFIG_NATIVEAP_MLME
		#define CONFIG_NATIVEAP_MLME			1
	#endif
	#ifndef CONFIG_INTERRUPT_BASED_TXBCN
		#define CONFIG_INTERRUPT_BASED_TXBCN	0
	#endif
	#if CONFIG_INTERRUPT_BASED_TXBCN 
		#ifndef CONFIG_INTERRUPT_BASED_TXBCN_EARLY_INT
			#define CONFIG_INTERRUPT_BASED_TXBCN_EARLY_INT		0
		#endif
		#ifndef CONFIG_INTERRUPT_BASED_TXBCN_BCN_OK_ERR
			#define CONFIG_INTERRUPT_BASED_TXBCN_BCN_OK_ERR		1
		#endif
	#endif
	#ifndef CONFIG_GK_REKEY
		#define CONFIG_GK_REKEY					0
	#endif
#else
	#if CONFIG_NATIVEAP_MLME
		#error "CONFIG_AP_MODE has not been defined, define CONFIG_AP_MODE firstly"
	#endif
#endif
#if !CONFIG_INTERRUPT_BASED_TXBCN && CONFIG_INTERRUPT_BASED_TXBCN_BCN_OK_ERR
	#error "CONFIG_INTERRUPT_BASED_TXBCN has not been defined, define CONFIG_INTERRUPT_BASED_TXBCN firstly"
#endif

#ifndef USE_DEDICATED_BCN_TX
	#define USE_DEDICATED_BCN_TX				(CONFIG_AP_MODE ? 1 : 0)
#endif

#if CONFIG_AP_MODE && CONFIG_GK_REKEY && !CONFIG_MULTIPLE_WPA_STA
	#error "If CONFIG_GK_REKEY when CONFIG_AP_MODE, need to CONFIG_MULTIPLE_WPA_STA"
#endif

#if !CONFIG_AP_MODE && CONFIG_CONCURRENT_MODE
	#error "If CONFIG_CONCURRENT_MODEE, need to CONFIG_AP_MODE"
#endif 
    
/* For MP_MODE */
#ifndef CONFIG_MP_INCLUDED
	#define CONFIG_MP_INCLUDED					0
#endif

#if CONFIG_MP_INCLUDED
	#ifndef CONFIG_MP_IWPRIV_SUPPORT
		#define CONFIG_MP_IWPRIV_SUPPORT		1
	#endif
	#ifndef HAL_EFUSE_MEMORY
		#define HAL_EFUSE_MEMORY				0
	#endif
	#ifndef CONFIG_MP_NORMAL_IWPRIV_SUPPORT
		#define CONFIG_MP_NORMAL_IWPRIV_SUPPORT	0
	#endif
#else
	#if CONFIG_MP_IWPRIV_SUPPORT || HAL_EFUSE_MEMORY || CONFIG_MP_NORMAL_IWPRIV_SUPPORT
		#error "CONFIG_MP_INCLUDED has not been defined, define CONFIG_MP_INCLUDED firstly"
	#endif
#endif

#ifndef MP_DRIVER
	#define MP_DRIVER							(CONFIG_MP_INCLUDED ? 1 : 0)
#endif

#ifndef CONFIG_MAC_ADDRESS
	#define CONFIG_MAC_ADDRESS					0
#endif

#ifndef CONFIG_WOWLAN
	#define CONFIG_WOWLAN						0
#endif

#if CONFIG_WOWLAN
	#ifndef CONFIG_SUSPEND
		#define CONFIG_SUSPEND					1
	#endif
	#ifndef CONFIG_RTW_SDIO_KEEP_IRQ
		#define CONFIG_RTW_SDIO_KEEP_IRQ		0
	#endif
	#ifndef CONFIG_GPIO_WAKEUP
		#define CONFIG_GPIO_WAKEUP				1
	#endif
	#ifndef CONFIG_WOWLAN_AWAKE_PATTERN
		#define CONFIG_WOWLAN_AWAKE_PATTERN		1//1:icmp awake  2:tcp udp unicast awake
	#endif	
#else
	#if CONFIG_SUSPEND || CONFIG_RTW_SDIO_KEEP_IRQ || CONFIG_GPIO_WAKEUP || CONFIG_WOWLAN_AWAKE_PATTERN
		#error "CONFIG_WOWLAN has not been defined, define CONFIG_WOWLAN firstly"
	#endif	
#endif

#ifndef CONFIG_DFS
	#define CONFIG_DFS							1
#endif



/*
 * Hareware/Firmware Related Config *******************************************
 */
#ifndef CONFIG_FW_C2H_PKT
	#define CONFIG_FW_C2H_PKT					1
#endif

#ifndef PHYDM_LINUX_CODING_STYLE
	#define PHYDM_LINUX_CODING_STYLE			1
#endif

#ifndef PHYDM_NEW_INTERFACE
	#define PHYDM_NEW_INTERFACE					(PHYDM_LINUX_CODING_STYLE ? 1 : 0)
#endif

#ifndef RTW_NOTCH_FILTER
	#define RTW_NOTCH_FILTER					0
#endif

#ifndef CONFIG_EMBEDDED_FWIMG
	#define CONFIG_EMBEDDED_FWIMG				1
#endif

#ifndef CONFIG_PHY_SETTING_WITH_ODM
	#define CONFIG_PHY_SETTING_WITH_ODM			1
#endif

#ifndef HAL_MAC_ENABLE
	#define HAL_MAC_ENABLE						1
#endif

#ifndef HAL_BB_ENABLE
	#define HAL_BB_ENABLE						1
#endif

#ifndef HAL_RF_ENABLE
	#define HAL_RF_ENABLE						1
#endif

#ifndef CONFIG_FAKE_EFUSE
	#define CONFIG_FAKE_EFUSE					0
#endif

#define RTL8192E_SUPPORT 0
#define RTL8812A_SUPPORT 0
#define RTL8821A_SUPPORT 0
#define RTL8723B_SUPPORT 0
#define RTL8188E_SUPPORT 0
#define RTL8188F_SUPPORT 0
#define RTL8821C_SUPPORT 0
#define RTL8723D_SUPPORT 0
#define RTL8192F_SUPPORT 0

/* For DM support */
#ifndef RATE_ADAPTIVE_SUPPORT
	#define RATE_ADAPTIVE_SUPPORT				0
#endif

#ifndef CONFIG_ODM_REFRESH_RAMASK
	#define CONFIG_ODM_REFRESH_RAMASK			0
#endif

#if defined(CONFIG_HARDWARE_8188F)
	#define CONFIG_RTL8188F
	#undef RTL8188F_SUPPORT
	#define RTL8188F_SUPPORT					1
#elif defined(CONFIG_HARDWARE_8192E)
	#define CONFIG_RTL8192E
	#undef RTL8192E_SUPPORT
	#define RTL8192E_SUPPORT					1
#elif defined(CONFIG_HARDWARE_8821C)
	#define CONFIG_RTL8821C
	#undef RTL8821C_SUPPORT
	#define RTL8821C_SUPPORT					1
#elif defined(CONFIG_HARDWARE_8723D)
	#define CONFIG_RTL8723D
	#undef RTL8723D_SUPPORT
	#define RTL8723D_SUPPORT					1
#elif defined(CONFIG_HARDWARE_8723B)
	#define CONFIG_RTL8723B
	#undef RTL8723B_SUPPORT
	#define RTL8723B_SUPPORT					1
#elif defined(CONFIG_HARDWARE_8703B)
	#define CONFIG_RTL8703B
	#undef RTL8703B_SUPPORT
	#define RTL8703B_SUPPORT					1
	#define CONFIG_BT_COEXIST					1
#elif defined(CONFIG_HARDWARE_8192F)
	#define CONFIG_RTL8192F
	#undef RTL8192F_SUPPORT
	#define RTL8192F_SUPPORT					1
#else
	#define CONFIG_RTL8188E
	#undef RTL8188E_SUPPORT
	#define RTL8188E_SUPPORT					1
	#undef RATE_ADAPTIVE_SUPPORT
	#define RATE_ADAPTIVE_SUPPORT				1
#endif

// adaptivity
#define RTW_ADAPTIVITY_EN_DISABLE				0
#define RTW_ADAPTIVITY_EN_ENABLE				1
#ifndef CONFIG_RTW_ADAPTIVITY_EN
	#define CONFIG_RTW_ADAPTIVITY_EN			RTW_ADAPTIVITY_EN_DISABLE
#endif
#define RTW_ADAPTIVITY_MODE_NORMAL				0
#define RTW_ADAPTIVITY_MODE_CARRIER_SENSE		1
#ifndef CONFIG_RTW_ADAPTIVITY_MODE
	#define CONFIG_RTW_ADAPTIVITY_MODE			RTW_ADAPTIVITY_MODE_CARRIER_SENSE
#endif
#ifndef CONFIG_RTW_ADAPTIVITY_DML
	#define CONFIG_RTW_ADAPTIVITY_DML			0
#endif

#ifndef POWER_BY_RATE_SUPPORT
	#define POWER_BY_RATE_SUPPORT				0
#endif

#ifndef CONFIG_EMPTY_EFUSE_PG_ENABLE
	#define CONFIG_EMPTY_EFUSE_PG_ENABLE		0
#endif



/*
 * Interface Related Config ***************************************************
 */
#if defined(USE_SDIO_INTERFACE)
	#define CONFIG_SDIO_HCI
#elif defined(USE_USB_INTERFACE)
	#define CONFIG_USB_HCI
#else
	#define CONFIG_GSPI_HCI
#endif

#ifndef CONFIG_USB_TX_NUM_ENABLE
	#ifdef CONFIG_USB_HCI
		#define CONFIG_USB_TX_NUM_ENABLE		0
	#endif
#endif
#ifndef CONFIG_USB_TX_AGGREGATION
	#if CONFIG_USB_TX_NUM_ENABLE
		#define CONFIG_USB_TX_AGGREGATION		1
	#endif
#endif
#ifndef CONFIG_USB_RX_AGGREGATION
	#ifdef CONFIG_USB_HCI
		#define CONFIG_USB_RX_AGGREGATION		0
	#endif
#endif
#if defined(CONFIG_USB_HCI) && (!CONFIG_USB_RX_AGGREGATION) && (CONFIG_RX_BUS_AGG_NUM > 1)
	#define CONFIG_USB_RX_AGGREGATION           1
#endif
#if defined(CONFIG_USB_HCI) && (!CONFIG_USB_TX_AGGREGATION) && (CONFIG_TX_BUS_AGG_NUM > 1)
	#error "no need to increase CONFIG_TX_BUS_AGG_NUM, CONFIG_USB_TX_AGGREGATION is not enabled"
#endif

#ifndef CONFIG_XMIT_THREAD_MODE
	#ifdef CONFIG_SDIO_HCI
		#define CONFIG_XMIT_THREAD_MODE			1
	#else
		#define CONFIG_XMIT_THREAD_MODE			0
	#endif
#endif

#ifndef CONFIG_RECV_THREAD_MODE
	#ifdef CONFIG_USB_HCI
		#define CONFIG_RECV_THREAD_MODE			1 /* Wlan IRQ Polling  Mode*/
	#else
		#define CONFIG_RECV_THREAD_MODE			0 /* Wlan IRQ Polling  Mode*/
	#endif
#endif

#ifndef CONFIG_ISR_THREAD_MODE_POLLING
	#define CONFIG_ISR_THREAD_MODE_POLLING		0 /* Wlan IRQ Polling  Mode*/
#endif

#ifndef CONFIG_ISR_THREAD_MODE_INTERRUPT		  /* Wlan IRQ Interrupt  Mode*/
	#if !defined(CONFIG_USB_HCI) && !defined(CONFIG_SDIO_HCI)
		#define CONFIG_ISR_THREAD_MODE_INTERRUPT			1
	#else
		#define CONFIG_ISR_THREAD_MODE_INTERRUPT			0
	#endif
#endif

#if CONFIG_ISR_THREAD_MODE_POLLING && CONFIG_ISR_THREAD_MODE_INTERRUPT
	#error "CONFIG_ISR_THREAD_MODE_POLLING and CONFIG_ISR_THREAD_MODE_INTERRUPT are mutually exclusive. "
#endif

#ifndef CONFIG_RECV_TASK_THREAD_MODE
	#define CONFIG_RECV_TASK_THREAD_MODE		0
#endif

#ifndef USE_SKB_AS_XMITBUF
	#ifdef CONFIG_USB_HCI
		#if CONFIG_USB_TX_NUM_ENABLE
		#define USE_SKB_AS_XMITBUF				0
		#else
		#define USE_SKB_AS_XMITBUF				1
		#endif
	#else
		#define USE_SKB_AS_XMITBUF				1
	#endif
#endif


/*
 * BT Related Config **********************************************************
 */
#ifndef CONFIG_BT_EN
	#define CONFIG_BT_EN						0
#endif
#if CONFIG_BT_EN
	#define CONFIG_FTL_ENABLED					1
#endif

#ifndef CONFIG_BT_COEXIST
	#define CONFIG_BT_COEXIST					0
#endif
#if CONFIG_BT_COEXIST
	#undef NOT_SUPPORT_BT
	#define NOT_SUPPORT_BT						0
	#ifndef CONFIG_BT_MAILBOX
		#define CONFIG_BT_MAILBOX				1
	#endif
	#ifndef CONFIG_BT_EFUSE
		#define CONFIG_BT_EFUSE					1
	#endif
	#ifndef CONFIG_BT_TWO_ANTENNA
		#define CONFIG_BT_TWO_ANTENNA			0
	#endif
	#ifndef HAL_EFUSE_MEMORY
		#if defined(CONFIG_RTL8723D) || defined(CONFIG_RTL8821C) || defined(CONFIG_RTL8703B)
			#define HAL_EFUSE_MEMORY			1
		#else
			#define HAL_EFUSE_MEMORY			0
		#endif
	#endif
#else
	#if CONFIG_BT_MAILBOX || CONFIG_BT_EFUSE || CONFIG_BT_TWO_ANTENNA
		#error "CONFIG_BT_COEXIST has not been defined, define CONFIG_BT_COEXIST firstly"
	#endif
#endif

/*
 * Others *********************************************************************
 */
#ifndef CONFIG_WIFI_CRITICAL_CODE_SECTION
	#define CONFIG_WIFI_CRITICAL_CODE_SECTION
#endif

#ifndef CONFIG_MEMORY_ACCESS_ALIGNED
	#define CONFIG_MEMORY_ACCESS_ALIGNED		1
#endif

#ifndef BAD_MIC_COUNTERMEASURE
	#define BAD_MIC_COUNTERMEASURE 				1
#endif

#ifndef DEFRAGMENTATION
	#define DEFRAGMENTATION						1
#endif

#ifndef WIFI_LOGO_CERTIFICATION
	#define WIFI_LOGO_CERTIFICATION 			0
#endif
#ifndef RX_AGGREGATION
	#define RX_AGGREGATION						(WIFI_LOGO_CERTIFICATION ? 1 : 0)
#endif
#ifndef RX_AMSDU
	#define RX_AMSDU							(WIFI_LOGO_CERTIFICATION ? 1 : 0)
#endif

#ifndef USE_MUTEX_FOR_SPINLOCK
	#define USE_MUTEX_FOR_SPINLOCK				1
#endif

#ifndef CONFIG_WIFI_SPEC
	#define CONFIG_WIFI_SPEC					0
#endif

#ifndef ENABLE_HWPDN_PIN
	#define ENABLE_HWPDN_PIN					1
#endif

#ifndef BE_I_CUT
	#define BE_I_CUT							1
#endif

#ifndef CONFIG_NEW_SIGNAL_STAT_PROCESS
	#define CONFIG_NEW_SIGNAL_STAT_PROCESS		1
#endif
#ifndef CONFIG_SKIP_SIGNAL_SCALE_MAPPING
	#define CONFIG_SKIP_SIGNAL_SCALE_MAPPING	1
#endif

#ifdef CONFIG_HARDWARE_8821C
	#ifndef FW_IQK
		#define FW_IQK							1
	#endif
	#ifndef RTW_HALMAC
		#define RTW_HALMAC						1
	#endif
	#ifndef LOAD_FW_HEADER_FROM_DRIVER
		#define LOAD_FW_HEADER_FROM_DRIVER		1
	#endif
	#ifndef RTW_HALMAC_SIZE_OPTIMIZATION
		#define RTW_HALMAC_SIZE_OPTIMIZATION	1
	#endif
	#ifndef CONFIG_NO_FW
		#define CONFIG_NO_FW					0
	#endif
	#ifndef CONFIG_PHY_CAPABILITY_QUERY
		#define CONFIG_PHY_CAPABILITY_QUERY		0
	#endif

	#if NOT_SUPPORT_VHT
		#undef NOT_SUPPORT_VHT
		#define NOT_SUPPORT_VHT 				0
	#endif

	#ifndef CONFIG_80211AC_VHT  //todo
		#define CONFIG_80211AC_VHT              0
	#endif	
	#if CONFIG_80211AC_VHT
		#if !CONFIG_80211N_HT
			#define CONFIG_80211N_HT            1
		#endif
	#endif

	#ifndef CONFIG_IO_CHECK_IN_ANA_LOW_CLK
	    #define CONFIG_IO_CHECK_IN_ANA_LOW_CLK  1
	#endif
#else
	#if FW_IQK || RTW_HALMAC || LOAD_FW_HEADER_FROM_DRIVER || RTW_HALMAC_SIZE_OPTIMIZATION || SUPPORT_5G_CHANNEL
		#error "CONFIG_HARDWARE_8821C has not been defined, define CONFIG_HARDWARE_8821C firstly"
	#endif
#endif

#ifndef CONFIG_ADDRESS_ALIGNMENT
	#define CONFIG_ADDRESS_ALIGNMENT			0
#endif

#if CONFIG_ADDRESS_ALIGNMENT && !defined(ALIGNMENT_SIZE)
	#define ALIGNMENT_SIZE 						32
#endif

#ifndef WLAN_WRAPPER_VERSION
	#define WLAN_WRAPPER_VERSION				1
#endif

#ifndef TIME_THRES
	#define TIME_THRES							20
#endif

#ifndef CONFIG_TIMER_THREAD
	#define CONFIG_TIMER_THREAD					0
#endif

// for ali
#ifndef CONFIG_CONNECT_FAST_FEATURE
	#define CONFIG_CONNECT_FAST_FEATURE         0
#endif

#ifndef CONFIG_NO_REFERENCE_FOR_COMPILER
	#define CONFIG_NO_REFERENCE_FOR_COMPILER    0
#endif

#ifndef CONFIG_AVIOD_STACKFLOW_FOR_64PLATFORM
	#define CONFIG_AVIOD_STACKFLOW_FOR_64PLATFORM  0
#endif

#ifndef CONFIG_QUICKEN_INIT
	#define CONFIG_QUICKEN_INIT                 0
#endif

#ifndef CONFIG_OPEN_LPS_ONLYFOR_BTCO
	#define CONFIG_OPEN_LPS_ONLYFOR_BTCO         0
#endif

#ifndef CONFIG_CUSTOMIZE_PHYREG_OF_PWR_BY_RTAE_AND_LIMIT
	#define CONFIG_CUSTOMIZE_PHYREG_OF_PWR_BY_RTAE_AND_LIMIT  0
#endif
//for ali end

#ifndef CONFIG_MP_LOG_FUNC_INDEPENDENT
	#define CONFIG_MP_LOG_FUNC_INDEPENDENT      0
#endif

#ifndef CONFIG_RETRY_LIMIT
	#define CONFIG_RETRY_LIMIT      			48
#endif

#ifndef SHRINK_DATA_SIZE
	#define SHRINK_DATA_SIZE                    0
#endif
#if SHRINK_DATA_SIZE  
    #undef NO_CALLER
	#define NO_CALLER                           1
#endif

/*
 * Debug Related Config *******************************************************
 */
#ifndef CONFIG_DEBUG
	#define CONFIG_DEBUG						1
#endif

#ifndef CONFIG_DEBUG_RTL871X
	#define CONFIG_DEBUG_RTL871X				0
#endif

#ifndef CONFIG_MEM_MONITOR
	#define CONFIG_MEM_MONITOR					MEM_MONITOR_SIMPLE
#endif

#ifndef CONFIG_TRACE_SKB
	#define CONFIG_TRACE_SKB					0
#endif

#ifndef WLAN_INTF_DBG
	#define WLAN_INTF_DBG						0
#endif

// for Debug message
#ifndef DBG
	#define DBG 								0
#endif

#ifndef CONFIG_IWPRIV_DBG
	#define CONFIG_IWPRIV_DBG 					1
#endif

#endif //WLANCONFIG_H

