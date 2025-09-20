/************************************************************************
 * centurio_nav app main
 ************************************************************************/
#include "centurio_nav.h"
#include "centurio_nav_eventids.h"
#include "centurio_nav_msg.h"
#include "centurio_nav_msgids.h"
#include "centurio_nav_fcncodes.h"
#include <string.h>

/* Ensure version string length macro is available */
#include "centurio_nav_internal_cfg.h"
#ifndef CENTURIO_NAV_CFG_MAX_VERSION_STR_LEN
#ifdef OS_CFG_MAX_VERSION_STR_LEN
#define CENTURIO_NAV_CFG_MAX_VERSION_STR_LEN OS_CFG_MAX_VERSION_STR_LEN
#else
#define CENTURIO_NAV_CFG_MAX_VERSION_STR_LEN 64
#endif
#endif

CENTURIO_NAV_Data_t CENTURIO_NAV_Data;

void CENTURIO_NAV_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(CENTURIO_NAV_PERF_ID_MAIN);

    status = CENTURIO_NAV_Init();
    if (status != CFE_SUCCESS)
    {
        CENTURIO_NAV_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    while (CFE_ES_RunLoop(&CENTURIO_NAV_Data.RunStatus) == true)
    {
        CFE_ES_PerfLogExit(CENTURIO_NAV_PERF_ID_MAIN);

        status = CFE_SB_ReceiveBuffer(&SBBufPtr, CENTURIO_NAV_Data.CommandPipe, CFE_SB_PEND_FOREVER);

        CFE_ES_PerfLogEntry(CENTURIO_NAV_PERF_ID_MAIN);

        if (status == CFE_SUCCESS)
        {
            /* Minimal dispatch: only HK and CMD mids to start */
            CFE_MSG_FcnCode_t FcnCode;
            CFE_SB_MsgId_t    MsgId;
            CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

            if (CFE_SB_MsgId_Equal(MsgId, CFE_SB_ValueToMsgId(CENTURIO_NAV_SEND_HK_MID)))
            {
                /* Update nav state slightly to simulate motion */
                CENTURIO_NAV_Data.Nav.YawDeg += 0.5f;
                if (CENTURIO_NAV_Data.Nav.YawDeg > 180.0f) { CENTURIO_NAV_Data.Nav.YawDeg -= 360.0f; }

                /* Populate HK payload */
                CENTURIO_NAV_Data.HkTlm.Payload.CommandCounter      = CENTURIO_NAV_Data.CmdCounter;
                CENTURIO_NAV_Data.HkTlm.Payload.CommandErrorCounter = CENTURIO_NAV_Data.ErrCounter;
                CENTURIO_NAV_Data.HkTlm.Payload.SystemStatus        = CENTURIO_NAV_Data.Nav.SystemStatus;
                CENTURIO_NAV_Data.HkTlm.Payload.NavFixType          = CENTURIO_NAV_Data.Nav.NavFixType;
                CENTURIO_NAV_Data.HkTlm.Payload.LatitudeDeg         = CENTURIO_NAV_Data.Nav.LatitudeDeg;
                CENTURIO_NAV_Data.HkTlm.Payload.LongitudeDeg        = CENTURIO_NAV_Data.Nav.LongitudeDeg;
                CENTURIO_NAV_Data.HkTlm.Payload.AltitudeM           = CENTURIO_NAV_Data.Nav.AltitudeM;
                CENTURIO_NAV_Data.HkTlm.Payload.VelNorthMS          = CENTURIO_NAV_Data.Nav.VelNorthMS;
                CENTURIO_NAV_Data.HkTlm.Payload.VelEastMS           = CENTURIO_NAV_Data.Nav.VelEastMS;
                CENTURIO_NAV_Data.HkTlm.Payload.VelDownMS           = CENTURIO_NAV_Data.Nav.VelDownMS;
                CENTURIO_NAV_Data.HkTlm.Payload.YawDeg              = CENTURIO_NAV_Data.Nav.YawDeg;
                CENTURIO_NAV_Data.HkTlm.Payload.PitchDeg            = CENTURIO_NAV_Data.Nav.PitchDeg;
                CENTURIO_NAV_Data.HkTlm.Payload.RollDeg             = CENTURIO_NAV_Data.Nav.RollDeg;
                CENTURIO_NAV_Data.HkTlm.Payload.ThrottlePercent     = CENTURIO_NAV_Data.Nav.ThrottlePercent;

                /* timestamp + publish HK */
                CFE_SB_TimeStampMsg(CFE_MSG_PTR(CENTURIO_NAV_Data.HkTlm.TelemetryHeader));
                CFE_SB_TransmitMsg(CFE_MSG_PTR(CENTURIO_NAV_Data.HkTlm.TelemetryHeader), true);
            }
            else if (CFE_SB_MsgId_Equal(MsgId, CFE_SB_ValueToMsgId(CENTURIO_NAV_CMD_MID)))
            {
                CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &FcnCode);
                switch (FcnCode)
                {
                    case CENTURIO_NAV_NOOP_CC:
                        CENTURIO_NAV_Data.CmdCounter++;
                        CFE_EVS_SendEvent(CENTURIO_NAV_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION, "CENTURIO_NAV: NOOP");
                        break;
                    case CENTURIO_NAV_RESET_COUNTERS_CC:
                        CENTURIO_NAV_Data.CmdCounter = 0;
                        CENTURIO_NAV_Data.ErrCounter = 0;
                        CFE_EVS_SendEvent(CENTURIO_NAV_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "CENTURIO_NAV: RESET");
                        break;
                    case CENTURIO_NAV_SET_MODE_CC:
                    {
                        size_t actSize = 0;
                        CFE_MSG_GetSize(&SBBufPtr->Msg, &actSize);
                        if (actSize == sizeof(CENTURIO_NAV_SetModeCmd_t))
                        {
                            const CENTURIO_NAV_SetModeCmd_t *cmd = (const CENTURIO_NAV_SetModeCmd_t *)SBBufPtr;
                            CENTURIO_NAV_Data.Target.Mode      = cmd->Payload.Mode;
                            CENTURIO_NAV_Data.Nav.SystemStatus = cmd->Payload.Mode; /* simple mapping */
                            CENTURIO_NAV_Data.CmdCounter++;
                        }
                        else
                        {
                            CENTURIO_NAV_Data.ErrCounter++;
                            CFE_EVS_SendEvent(CENTURIO_NAV_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                                              "SET_MODE bad len %lu", (unsigned long)actSize);
                        }
                        break;
                    }
                    case CENTURIO_NAV_SET_TARGET_POS_CC:
                    {
                        size_t actSize = 0; CFE_MSG_GetSize(&SBBufPtr->Msg, &actSize);
                        if (actSize == sizeof(CENTURIO_NAV_SetTargetPosCmd_t))
                        {
                            const CENTURIO_NAV_SetTargetPosCmd_t *cmd = (const CENTURIO_NAV_SetTargetPosCmd_t *)SBBufPtr;
                            CENTURIO_NAV_Data.Target.LatDeg = cmd->Payload.LatDeg;
                            CENTURIO_NAV_Data.Target.LonDeg = cmd->Payload.LonDeg;
                            CENTURIO_NAV_Data.Target.AltM   = cmd->Payload.AltM;
                            CENTURIO_NAV_Data.CmdCounter++;
                        }
                        else { CENTURIO_NAV_Data.ErrCounter++; }
                        break;
                    }
                    case CENTURIO_NAV_SET_TARGET_VEL_CC:
                    {
                        size_t actSize = 0; CFE_MSG_GetSize(&SBBufPtr->Msg, &actSize);
                        if (actSize == sizeof(CENTURIO_NAV_SetTargetVelCmd_t))
                        {
                            const CENTURIO_NAV_SetTargetVelCmd_t *cmd = (const CENTURIO_NAV_SetTargetVelCmd_t *)SBBufPtr;
                            CENTURIO_NAV_Data.Target.VN_MS = cmd->Payload.VN_MS;
                            CENTURIO_NAV_Data.Target.VE_MS = cmd->Payload.VE_MS;
                            CENTURIO_NAV_Data.Target.VD_MS = cmd->Payload.VD_MS;
                            CENTURIO_NAV_Data.CmdCounter++;
                        }
                        else { CENTURIO_NAV_Data.ErrCounter++; }
                        break;
                    }
                    case CENTURIO_NAV_SET_TARGET_ATT_CC:
                    {
                        size_t actSize = 0; CFE_MSG_GetSize(&SBBufPtr->Msg, &actSize);
                        if (actSize == sizeof(CENTURIO_NAV_SetTargetAttCmd_t))
                        {
                            const CENTURIO_NAV_SetTargetAttCmd_t *cmd = (const CENTURIO_NAV_SetTargetAttCmd_t *)SBBufPtr;
                            CENTURIO_NAV_Data.Target.YawDeg   = cmd->Payload.YawDeg;
                            CENTURIO_NAV_Data.Target.PitchDeg = cmd->Payload.PitchDeg;
                            CENTURIO_NAV_Data.Target.RollDeg  = cmd->Payload.RollDeg;
                            CENTURIO_NAV_Data.CmdCounter++;
                        }
                        else { CENTURIO_NAV_Data.ErrCounter++; }
                        break;
                    }
                    case CENTURIO_NAV_SET_THROTTLE_CC:
                    {
                        size_t actSize = 0; CFE_MSG_GetSize(&SBBufPtr->Msg, &actSize);
                        if (actSize == sizeof(CENTURIO_NAV_SetThrottleCmd_t))
                        {
                            const CENTURIO_NAV_SetThrottleCmd_t *cmd = (const CENTURIO_NAV_SetThrottleCmd_t *)SBBufPtr;
                            if (cmd->Payload.Percent < 0.0f)
                                CENTURIO_NAV_Data.Nav.ThrottlePercent = 0.0f;
                            else if (cmd->Payload.Percent > 100.0f)
                                CENTURIO_NAV_Data.Nav.ThrottlePercent = 100.0f;
                            else
                                CENTURIO_NAV_Data.Nav.ThrottlePercent = cmd->Payload.Percent;
                            CENTURIO_NAV_Data.CmdCounter++;
                        }
                        else { CENTURIO_NAV_Data.ErrCounter++; }
                        break;
                    }
                    default:
                        CENTURIO_NAV_Data.ErrCounter++;
                        CFE_EVS_SendEvent(CENTURIO_NAV_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR, "CENTURIO_NAV: Unknown CC %u", (unsigned)FcnCode);
                        break;
                }
            }
            else
            {
                CFE_EVS_SendEvent(CENTURIO_NAV_PIPE_ERR_EID, CFE_EVS_EventType_ERROR, "CENTURIO_NAV: Unknown MID");
            }
        }
        else
        {
            CFE_EVS_SendEvent(CENTURIO_NAV_PIPE_ERR_EID, CFE_EVS_EventType_ERROR, "CENTURIO_NAV: SB pipe read error");
            CENTURIO_NAV_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_PerfLogExit(CENTURIO_NAV_PERF_ID_MAIN);
    CFE_ES_ExitApp(CENTURIO_NAV_Data.RunStatus);
}

CFE_Status_t CENTURIO_NAV_Init(void)
{
    CFE_Status_t status;
    char         VersionString[CENTURIO_NAV_CFG_MAX_VERSION_STR_LEN];

    memset(&CENTURIO_NAV_Data, 0, sizeof(CENTURIO_NAV_Data));
    CENTURIO_NAV_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("centurio_nav: EVS register error 0x%08lX\n", (unsigned long)status);
        return status;
    }

    CFE_MSG_Init(CFE_MSG_PTR(CENTURIO_NAV_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(CENTURIO_NAV_HK_TLM_MID), sizeof(CENTURIO_NAV_Data.HkTlm));

    /* Defaults similar to sample_app */
    CENTURIO_NAV_Data.Nav.LatitudeDeg  = 37.6213;
    CENTURIO_NAV_Data.Nav.LongitudeDeg = -122.3790;
    CENTURIO_NAV_Data.Nav.AltitudeM    = 10.0f;
    CENTURIO_NAV_Data.Nav.YawDeg       = 0.0f;
    CENTURIO_NAV_Data.Nav.PitchDeg     = 0.0f;
    CENTURIO_NAV_Data.Nav.RollDeg      = 0.0f;
    CENTURIO_NAV_Data.Nav.SystemStatus = 1; /* SAFE/OK */
    CENTURIO_NAV_Data.Nav.NavFixType   = 3; /* 3D */

    status = CFE_SB_CreatePipe(&CENTURIO_NAV_Data.CommandPipe, CENTURIO_NAV_PIPE_DEPTH, CENTURIO_NAV_PIPE_NAME);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(CENTURIO_NAV_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR, "centurio_nav: Create pipe err 0x%08lX", (unsigned long)status);
        return status;
    }

    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CENTURIO_NAV_SEND_HK_MID), CENTURIO_NAV_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(CENTURIO_NAV_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR, "centurio_nav: Sub HK err 0x%08lX", (unsigned long)status);
        return status;
    }

    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CENTURIO_NAV_CMD_MID), CENTURIO_NAV_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(CENTURIO_NAV_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR, "centurio_nav: Sub CMD err 0x%08lX", (unsigned long)status);
        return status;
    }

    CFE_Config_GetVersionString(VersionString, sizeof(VersionString), "centurio_nav", "v0.1.0", "", 0);
    CFE_EVS_SendEvent(CENTURIO_NAV_INIT_INF_EID, CFE_EVS_EventType_INFORMATION, "centurio_nav initialized. %s", VersionString);

    return CFE_SUCCESS;
}
