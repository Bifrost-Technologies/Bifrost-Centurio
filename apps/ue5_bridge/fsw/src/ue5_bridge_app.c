/************************************************************************
 * UE5 Bridge application - source
 ************************************************************************/
#include "ue5_bridge_app.h"
#include "cfe_config.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"
#include "cfe_msg.h"
#include "common_types.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "ue5_bridge_eventids.h"
#include "centurio_nav_fcncodes.h"

UE5_BRIDGE_Data_t UE5_BRIDGE_Data;

static void UE5_BRIDGE_SendJson(const char *s)
{
    if (OS_ObjectIdDefined(UE5_BRIDGE_Data.TxSocket))
    {
        int32 rc = OS_SocketSendTo(UE5_BRIDGE_Data.TxSocket, s, strlen(s), &UE5_BRIDGE_Data.TxAddr);
        if (rc < 0)
        {
            CFE_EVS_SendEvent(UE5_BRIDGE_SEND_ERR_EID, CFE_EVS_EventType_ERROR, "UE5_BRIDGE: UDP send failed rc=%ld", (long)rc);
        }
    }
}

/* --- Minimal JSON parsing helpers (no external deps) --- */
static const char *find_key(const char *json, const char *key)
{
    /* find "key" in json */
    static char pat[64];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(json, pat);
    if (!p) return NULL;
    p = strchr(p + 1, ':');
    if (!p) return NULL;
    return p + 1; /* start of value */
}

static bool parse_float(const char *json, const char *key, float *out)
{
    const char *p = find_key(json, key);
    if (!p) return false;
    float v;
    if (sscanf(p, "%f", &v) == 1) { *out = v; return true; }
    return false;
}

static bool parse_double(const char *json, const char *key, double *out)
{
    const char *p = find_key(json, key);
    if (!p) return false;
    double v;
    if (sscanf(p, "%lf", &v) == 1) { *out = v; return true; }
    return false;
}

static bool parse_uint8(const char *json, const char *key, uint8 *out)
{
    const char *p = find_key(json, key);
    if (!p) return false;
    unsigned v;
    if (sscanf(p, "%u", &v) == 1 && v <= 0xFF) { *out = (uint8)v; return true; }
    return false;
}

static bool parse_string(const char *json, const char *key, char *buf, size_t buflen)
{
    const char *p = find_key(json, key);
    if (!p) return false;
    while (*p == ' ' || *p == '\t') ++p;
    if (*p != '"') return false;
    ++p;
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < buflen) { buf[i++] = *p++; }
    buf[i] = '\0';
    return true;
}

static bool parse_type(const char *json, char *buf, size_t buflen)
{
    return parse_string(json, "type", buf, buflen);
}

static void dispatch_cmd_json(const char *json)
{
    char type[64] = {0};
    if (!parse_type(json, type, sizeof(type)))
    {
        CFE_EVS_SendEvent(UE5_BRIDGE_RX_ERR_EID, CFE_EVS_EventType_ERROR, "UE5_BRIDGE RX: missing type");
        return;
    }

    if (strcmp(type, "noop") == 0)
    {
        CENTURIO_NAV_NoopCmd_t cmd; memset(&cmd, 0, sizeof(cmd));
    CFE_MSG_Init((CFE_MSG_Message_t *)&cmd.CommandHeader, CFE_SB_ValueToMsgId(CENTURIO_NAV_CMD_MID), sizeof(cmd));
    CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)&cmd.CommandHeader, CENTURIO_NAV_NOOP_CC);
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&cmd.CommandHeader, true);
        CFE_EVS_SendEvent(UE5_BRIDGE_CMD_INF_EID, CFE_EVS_EventType_INFORMATION, "UE5_BRIDGE: NOOP sent");
        return;
    }
    if (strcmp(type, "reset") == 0)
    {
        CENTURIO_NAV_ResetCountersCmd_t cmd; memset(&cmd, 0, sizeof(cmd));
    CFE_MSG_Init((CFE_MSG_Message_t *)&cmd.CommandHeader, CFE_SB_ValueToMsgId(CENTURIO_NAV_CMD_MID), sizeof(cmd));
    CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)&cmd.CommandHeader, CENTURIO_NAV_RESET_COUNTERS_CC);
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&cmd.CommandHeader, true);
        CFE_EVS_SendEvent(UE5_BRIDGE_CMD_INF_EID, CFE_EVS_EventType_INFORMATION, "UE5_BRIDGE: RESET sent");
        return;
    }
    if (strcmp(type, "set_mode") == 0)
    {
        uint8 mode;
        if (!parse_uint8(json, "mode", &mode)) { CFE_EVS_SendEvent(UE5_BRIDGE_RX_ERR_EID, CFE_EVS_EventType_ERROR, "UE5_BRIDGE: set_mode missing mode"); return; }
        CENTURIO_NAV_SetModeCmd_t cmd; memset(&cmd, 0, sizeof(cmd));
    CFE_MSG_Init((CFE_MSG_Message_t *)&cmd.CommandHeader, CFE_SB_ValueToMsgId(CENTURIO_NAV_CMD_MID), sizeof(cmd));
    CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)&cmd.CommandHeader, CENTURIO_NAV_SET_MODE_CC);
        cmd.Payload.Mode = mode;
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&cmd.CommandHeader, true);
        CFE_EVS_SendEvent(UE5_BRIDGE_CMD_INF_EID, CFE_EVS_EventType_INFORMATION, "UE5_BRIDGE: MODE %u", (unsigned)mode);
        return;
    }
    if (strcmp(type, "set_throttle") == 0)
    {
        float pct;
        if (!parse_float(json, "percent", &pct)) { CFE_EVS_SendEvent(UE5_BRIDGE_RX_ERR_EID, CFE_EVS_EventType_ERROR, "UE5: throttle missing percent"); return; }
        if (pct < 0) { pct = 0; }
        if (pct > 100) { pct = 100; }
        CENTURIO_NAV_SetThrottleCmd_t cmd; memset(&cmd, 0, sizeof(cmd));
    CFE_MSG_Init((CFE_MSG_Message_t *)&cmd.CommandHeader, CFE_SB_ValueToMsgId(CENTURIO_NAV_CMD_MID), sizeof(cmd));
    CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)&cmd.CommandHeader, CENTURIO_NAV_SET_THROTTLE_CC);
        cmd.Payload.Percent = pct;
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&cmd.CommandHeader, true);
        CFE_EVS_SendEvent(UE5_BRIDGE_CMD_INF_EID, CFE_EVS_EventType_INFORMATION, "UE5_BRIDGE: THR %.1f", (double)pct);
        return;
    }
    if (strcmp(type, "set_target_pos") == 0)
    {
        double lat, lon; float alt;
        if (!parse_double(json, "lat", &lat) || !parse_double(json, "lon", &lon) || !parse_float(json, "alt_m", &alt))
        { CFE_EVS_SendEvent(UE5_BRIDGE_RX_ERR_EID, CFE_EVS_EventType_ERROR, "UE5: target_pos missing lat/lon/alt_m"); return; }
        CENTURIO_NAV_SetTargetPosCmd_t cmd; memset(&cmd, 0, sizeof(cmd));
    CFE_MSG_Init((CFE_MSG_Message_t *)&cmd.CommandHeader, CFE_SB_ValueToMsgId(CENTURIO_NAV_CMD_MID), sizeof(cmd));
    CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)&cmd.CommandHeader, CENTURIO_NAV_SET_TARGET_POS_CC);
        cmd.Payload.LatDeg = lat; cmd.Payload.LonDeg = lon; cmd.Payload.AltM = alt;
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&cmd.CommandHeader, true);
        CFE_EVS_SendEvent(UE5_BRIDGE_CMD_INF_EID, CFE_EVS_EventType_INFORMATION, "UE5_BRIDGE: POS lat=%.6f lon=%.6f alt=%.1f", lat, lon, (double)alt);
        return;
    }
    if (strcmp(type, "set_target_vel") == 0)
    {
        float vn, ve, vd;
        if (!parse_float(json, "vn", &vn) || !parse_float(json, "ve", &ve) || !parse_float(json, "vd", &vd))
        { CFE_EVS_SendEvent(UE5_BRIDGE_RX_ERR_EID, CFE_EVS_EventType_ERROR, "UE5: target_vel missing vn/ve/vd"); return; }
        CENTURIO_NAV_SetTargetVelCmd_t cmd; memset(&cmd, 0, sizeof(cmd));
    CFE_MSG_Init((CFE_MSG_Message_t *)&cmd.CommandHeader, CFE_SB_ValueToMsgId(CENTURIO_NAV_CMD_MID), sizeof(cmd));
    CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)&cmd.CommandHeader, CENTURIO_NAV_SET_TARGET_VEL_CC);
        cmd.Payload.VN_MS = vn; cmd.Payload.VE_MS = ve; cmd.Payload.VD_MS = vd;
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&cmd.CommandHeader, true);
        CFE_EVS_SendEvent(UE5_BRIDGE_CMD_INF_EID, CFE_EVS_EventType_INFORMATION, "UE5_BRIDGE: VEL vn=%.2f ve=%.2f vd=%.2f", (double)vn, (double)ve, (double)vd);
        return;
    }
    if (strcmp(type, "set_target_att") == 0)
    {
        float yaw, pitch, roll;
        if (!parse_float(json, "yaw", &yaw) || !parse_float(json, "pitch", &pitch) || !parse_float(json, "roll", &roll))
        { CFE_EVS_SendEvent(UE5_BRIDGE_RX_ERR_EID, CFE_EVS_EventType_ERROR, "UE5: target_att missing yaw/pitch/roll"); return; }
        CENTURIO_NAV_SetTargetAttCmd_t cmd; memset(&cmd, 0, sizeof(cmd));
    CFE_MSG_Init((CFE_MSG_Message_t *)&cmd.CommandHeader, CFE_SB_ValueToMsgId(CENTURIO_NAV_CMD_MID), sizeof(cmd));
    CFE_MSG_SetFcnCode((CFE_MSG_Message_t *)&cmd.CommandHeader, CENTURIO_NAV_SET_TARGET_ATT_CC);
        cmd.Payload.YawDeg = yaw; cmd.Payload.PitchDeg = pitch; cmd.Payload.RollDeg = roll;
    CFE_SB_TransmitMsg((CFE_MSG_Message_t *)&cmd.CommandHeader, true);
        CFE_EVS_SendEvent(UE5_BRIDGE_CMD_INF_EID, CFE_EVS_EventType_INFORMATION, "UE5_BRIDGE: ATT yaw=%.1f pitch=%.1f roll=%.1f", (double)yaw, (double)pitch, (double)roll);
        return;
    }

    CFE_EVS_SendEvent(UE5_BRIDGE_RX_ERR_EID, CFE_EVS_EventType_ERROR, "UE5_BRIDGE: unknown type '%s'", type);
}

void UE5_BRIDGE_SendNavAsJson(const CENTURIO_NAV_HkTlm_t *Hk)
{
    /* Build a minimal JSON payload from centurio_nav HK */
    /* Note: ensure buffer size safety in production */
    int len = snprintf(UE5_BRIDGE_Data.JsonBuf, sizeof(UE5_BRIDGE_Data.JsonBuf),
        "{\"type\":\"nav\",\"lat\":%.8f,\"lon\":%.8f,\"alt\":%.3f,\"vn\":%.3f,\"ve\":%.3f,\"vd\":%.3f,\"yaw\":%.2f,\"pitch\":%.2f,\"roll\":%.2f,\"thr\":%.1f,\"status\":%u,\"fix\":%u}",
        Hk->Payload.LatitudeDeg, Hk->Payload.LongitudeDeg, Hk->Payload.AltitudeM,
        Hk->Payload.VelNorthMS, Hk->Payload.VelEastMS, Hk->Payload.VelDownMS,
        Hk->Payload.YawDeg, Hk->Payload.PitchDeg, Hk->Payload.RollDeg,
        Hk->Payload.ThrottlePercent,
        (unsigned)Hk->Payload.SystemStatus, (unsigned)Hk->Payload.NavFixType);
    if (len > 0)
    {
        UE5_BRIDGE_SendJson(UE5_BRIDGE_Data.JsonBuf);
    }
}

void UE5_BRIDGE_PollRx(void)
{
    if (!OS_ObjectIdDefined(UE5_BRIDGE_Data.RxSocket))
        return;

    uint32 state = 0;
    if (OS_SelectSingle(UE5_BRIDGE_Data.RxSocket, &state, 0) == OS_SUCCESS && (state & OS_STREAM_STATE_READABLE))
    {
        OS_SockAddr_t from;
        int32 n = OS_SocketRecvFrom(UE5_BRIDGE_Data.RxSocket, UE5_BRIDGE_Data.CmdBuf, sizeof(UE5_BRIDGE_Data.CmdBuf)-1, &from, 0);
        if (n > 0)
        {
            UE5_BRIDGE_Data.CmdBuf[n] = '\0';
            dispatch_cmd_json(UE5_BRIDGE_Data.CmdBuf);
        }
    }
}

void UE5_BRIDGE_Init(void)
{
    memset(&UE5_BRIDGE_Data, 0, sizeof(UE5_BRIDGE_Data));
    strncpy(UE5_BRIDGE_Data.Ue5Ip, UE5_BRIDGE_UE5_IP_DEFAULT, sizeof(UE5_BRIDGE_Data.Ue5Ip)-1);

    CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);

    CFE_SB_CreatePipe(&UE5_BRIDGE_Data.CmdPipe, UE5_BRIDGE_PIPE_DEPTH, UE5_BRIDGE_PIPE_NAME);
    /* Subscribe to centurio_nav HK so we can mirror it to UE5 */
    CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CENTURIO_NAV_HK_TLM_MID), UE5_BRIDGE_Data.CmdPipe);

    /* Open UDP TX socket to UE5 */
    UE5_BRIDGE_Data.TxSocket = OS_OBJECT_ID_UNDEFINED;
    OS_SocketOpen(&UE5_BRIDGE_Data.TxSocket, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
    OS_SocketAddrInit(&UE5_BRIDGE_Data.TxAddr, OS_SocketDomain_INET);
    UE5_BRIDGE_Data.TxPort = UE5_BRIDGE_JSON_TX_PORT;
    OS_SocketAddrSetPort(&UE5_BRIDGE_Data.TxAddr, UE5_BRIDGE_Data.TxPort);
    OS_SocketAddrFromString(&UE5_BRIDGE_Data.TxAddr, UE5_BRIDGE_Data.Ue5Ip);

    /* Open UDP RX socket for JSON control */
    UE5_BRIDGE_Data.RxSocket = OS_OBJECT_ID_UNDEFINED;
    OS_SocketOpen(&UE5_BRIDGE_Data.RxSocket, OS_SocketDomain_INET, OS_SocketType_DATAGRAM);
    OS_SocketAddrInit(&UE5_BRIDGE_Data.RxBind, OS_SocketDomain_INET);
    UE5_BRIDGE_Data.RxPort = UE5_BRIDGE_JSON_RX_PORT;
    OS_SocketAddrSetPort(&UE5_BRIDGE_Data.RxBind, UE5_BRIDGE_Data.RxPort);
    /* 0.0.0.0 bind */
    OS_SocketAddrFromString(&UE5_BRIDGE_Data.RxBind, "0.0.0.0");
    int32 brc = OS_SocketBind(UE5_BRIDGE_Data.RxSocket, &UE5_BRIDGE_Data.RxBind);
    if (brc != OS_SUCCESS)
    {
        CFE_EVS_SendEvent(UE5_BRIDGE_RX_ERR_EID, CFE_EVS_EventType_ERROR, "UE5_BRIDGE: RX bind failed rc=%ld", (long)brc);
    }
    else
    {
        CFE_EVS_SendEvent(UE5_BRIDGE_RX_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "UE5_BRIDGE RX listening on 0.0.0.0:%u", (unsigned)UE5_BRIDGE_Data.RxPort);
    }

    CFE_EVS_SendEvent(UE5_BRIDGE_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "UE5_BRIDGE initialized. TX %s:%u",
                      UE5_BRIDGE_Data.Ue5Ip, (unsigned)UE5_BRIDGE_Data.TxPort);
}

void UE5_BRIDGE_AppMain(void)
{
    CFE_SB_Buffer_t *SBBufPtr;
    CFE_Status_t status;
    uint32 RunStatus = CFE_ES_RunStatus_APP_RUN;

    CFE_ES_PerfLogEntry(UE5_BRIDGE_MAIN_TASK_PERF_ID);

    UE5_BRIDGE_Init();

    while (CFE_ES_RunLoop(&RunStatus) == true)
    {
        CFE_ES_PerfLogExit(UE5_BRIDGE_MAIN_TASK_PERF_ID);
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, UE5_BRIDGE_Data.CmdPipe, 100);
        CFE_ES_PerfLogEntry(UE5_BRIDGE_MAIN_TASK_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            CFE_SB_MsgId_t MsgId;
            CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);
            if (CFE_SB_MsgId_Equal(MsgId, CFE_SB_ValueToMsgId(CENTURIO_NAV_HK_TLM_MID)))
            {
                UE5_BRIDGE_SendNavAsJson((const CENTURIO_NAV_HkTlm_t*)SBBufPtr);
            }
        }

        UE5_BRIDGE_PollRx();
    }

    CFE_ES_ExitApp(RunStatus);
}
