/************************************************************************
 * UE5 Bridge application - header
 ************************************************************************/
#ifndef UE5_BRIDGE_APP_H
#define UE5_BRIDGE_APP_H

#include "cfe.h"
#include "common_types.h"
#include "ue5_bridge_platform_cfg.h"
#include "ue5_bridge_mission_cfg.h"
#include "ue5_bridge_perfids.h"

#include "centurio_nav_msgids.h"
#include "centurio_nav_msg.h"

#include "osapi.h"

typedef struct
{
    char         Ue5Ip[64];
    OS_SockAddr_t TxAddr;
    osal_id_t    TxSocket; /* OSAL socket handle */
    osal_id_t    RxSocket; /* JSON RX socket */
    uint16       RxPort;   /* configured RX port */
    uint16       TxPort;   /* configured TX port */
    OS_SockAddr_t RxBind;

    CFE_SB_PipeId_t CmdPipe;

    /* Buffer for minimal JSON messages */
    char   JsonBuf[512];
    size_t JsonLen;
    char   CmdBuf[512];
    size_t CmdLen;

} UE5_BRIDGE_Data_t;

extern UE5_BRIDGE_Data_t UE5_BRIDGE_Data;

void UE5_BRIDGE_AppMain(void);
void UE5_BRIDGE_Init(void);
void UE5_BRIDGE_PollRx(void);
void UE5_BRIDGE_SendNavAsJson(const CENTURIO_NAV_HkTlm_t *Hk);

#endif
