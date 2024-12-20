/*****************************************************************************
**
**  Name:           bta_sys_int.h
**
**  Description:    This is the private interface file for the BTA system
**                  manager.
**
**  Copyright (c) 2003-2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_SYS_INT_H
#define BTA_SYS_INT_H

#if (BTU_STACK_LITE_ENABLED == FALSE)
#include "ptim.h"
#endif

#include "btm_api.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

/*****************************************************************************
**  state table
*****************************************************************************/

/* SYS HW state */
enum
{
    BTA_SYS_HW_OFF,
    BTA_SYS_HW_STARTING,
    BTA_SYS_HW_ON,
    BTA_SYS_HW_STOPPING
};
typedef UINT8 tBTA_SYS_HW_STATE;

/* Collision callback */
#define MAX_COLLISION_REG   5

typedef struct
{
    UINT8                   id[MAX_COLLISION_REG];
    tBTA_SYS_CONN_CBACK     *p_coll_cback[MAX_COLLISION_REG];
} tBTA_SYS_COLLISION;

/* system manager control block */
typedef struct
{
#if (BTU_STACK_LITE_ENABLED == FALSE)
    tBTA_SYS_REG            *reg[BTA_ID_MAX];       /* registration structures */
    BOOLEAN                 is_reg[BTA_ID_MAX];     /* registration structures */
    tPTIM_CB                ptim_cb;                /* protocol timer list */
    BOOLEAN                 timers_disabled;        /* TRUE if sys timers disabled */
#endif
    UINT8                   task_id;                /* GKI task id */
    tBTA_SYS_HW_STATE state;
    tBTA_SYS_HW_CBACK *sys_hw_cback[BTA_SYS_MAX_HW_MODULES];    /* enable callback for each HW modules */
    UINT32                  sys_hw_module_active;   /* bitmask of all active modules */
    UINT16                  sys_features;           /* Bitmask of sys features */

#if (BTU_STACK_LITE_ENABLED == FALSE)
    tBTA_SYS_CONN_CBACK     *prm_cb;                 /* role management callback registered by DM */
    tBTA_SYS_CONN_CBACK     *ppm_cb;                 /* low power management callback registered by DM */
    tBTA_SYS_CONN_CBACK     *p_policy_cb;            /* link policy change callback registered by DM */
    tBTA_SYS_CONN_CBACK     *p_sco_cb;               /* SCO connection change callback registered by AV */
    tBTA_SYS_CONN_CBACK     *p_role_cb;              /* role change callback registered by AV */
    tBTA_SYS_COLLISION      colli_reg;               /* collision handling module */
#if ( BTM_EIR_SERVER_INCLUDED == TRUE )&&(BTA_EIR_CANNED_UUID_LIST != TRUE)
    tBTA_SYS_EIR_CBACK      *eir_cb;                /* add/remove UUID into EIR */
#endif
#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
    tBTA_SYS_SYNC_RESULT_CBACK      *sync_result_cback;
    tBTA_SYS_NOTIFY_REG_CBACK       *notify_reg_cback;
    tBTA_SYS_IPC_REG_CBACK          *ipc_reg_cback;
    tBTA_SYS_SWITCH_STATUS_CBACK    *switch_status_cback;
    tBTA_SYS_SYNC_REG_CBACK         *sync_reg_cback[2]; /* For DM and RT */
#endif
#if (BTM_SSR_INCLUDED == TRUE)
    tBTA_SYS_SSR_CFG_CBACK      *p_ssr_cb;
#endif
#endif
    /* VS event handler */
    tBTA_SYS_VS_EVT_HDLR   *p_vs_evt_hdlr;

} tBTA_SYS_CB;




/*****************************************************************************
**  Global variables
*****************************************************************************/

/* system manager control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_SYS_CB bta_sys_cb;
#else
extern tBTA_SYS_CB *bta_sys_cb_ptr;
#define bta_sys_cb (*bta_sys_cb_ptr)
#endif


/* system manager configuration structure */
extern tBTA_SYS_CFG *p_bta_sys_cfg;



/* functions used for BTA SYS HW state machine */
extern void bta_sys_hw_btm_cback( tBTM_DEV_STATUS status );
extern void bta_sys_hw_error(tBTA_SYS_HW_MSG *p_sys_hw_msg);
extern void bta_sys_hw_api_enable( tBTA_SYS_HW_MSG *p_sys_hw_msg );
extern void bta_sys_hw_api_disable(tBTA_SYS_HW_MSG *p_sys_hw_msg);
extern void bta_sys_hw_evt_enabled(tBTA_SYS_HW_MSG *p_sys_hw_msg);
extern void bta_sys_hw_evt_disabled(tBTA_SYS_HW_MSG *p_sys_hw_msg);
extern void bta_sys_hw_evt_stack_enabled(tBTA_SYS_HW_MSG *p_sys_hw_msg);

extern BOOLEAN bta_sys_sm_execute(BT_HDR *p_msg);


extern BOOLEAN utl_check_utf8 (char *string, UINT16 max_len);


#endif /* BTA_SYS_INT_H */
