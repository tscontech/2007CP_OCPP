/*
 * Copyright (c) 2004 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * Onvif.
 *
 * @author
 * @version 1.0
 */
#ifndef ONVIF_H
#define ONVIF_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_PRINT(...)
//#define DEBUG_PRINT     printf
#define MAX_IPCAM       16

//=============================================================================
//                              Structure Definition
//=============================================================================

/**
 * IPCam's Onvif conntrack
 */
typedef struct ONVIFConn {
    int discovery_type;
    char uuid[47];
    char buf[MAX_IPCAM][8192];
    int buf_len[MAX_IPCAM];
} onvifConn;

/**
 * IPCam's device info
 */
typedef struct ONVIFData {
    char xaddrs[1024];
    char camera_name[1024];
    char username[128];
    char password[128];
} onvifData;

//=============================================================================
//                              Public Function Definition
//=============================================================================

/**
 * Onvif Init
 *
 * @param conn The Onvif conntrack
 */
void OnviInit(onvifConn *conn);

/**
 * Onvif de-init
 */
void OnvifDeinit(void);

/**
 * Onvif Discovery
 *
 * @param conn The Onvif conntrack
 * @return The number of IPCam
 */
int OnvifDiscovery(onvifConn *conn);

/**
 * Get IPCam's device info from @camIdx Onvif conntrack
 *
 * @param camIdx The index of IPCam
 * @param conn The Onvif conntrack
 * @param data The IPCam's device info
 */
void OnvifDataParser(int camIdx, onvifConn *conn, onvifData *data);

/**
 * Count the number of IP in xAddrs list
 *
 * @param xAddrs The IPCam's xAddrs
 * @return The number of IP
 */
int OnvifGetxAddrsNum(char *xAddrs);

/**
 * Transfer xAddrs to IP address
 *
 * @param idx The index of IP address in xAddrs
 * @param ip The ip addrees you want
 * @param xAddrs The IPCam's xAddrs
 */
void OnvifTransferAddr(int idx, char *ip, char *xAddrs);


#ifdef __cplusplus
}
#endif

#endif
