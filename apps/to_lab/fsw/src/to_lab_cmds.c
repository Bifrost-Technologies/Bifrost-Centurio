/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * \file
 *  This file contains the source code for the TO lab application
 */

#include "cfe.h"
#include "cfe_config.h" // For CFE_Config_GetVersionString

#include "to_lab_app.h"
#include "to_lab_cmds.h"
#include "to_lab_msg.h"
#include "to_lab_eventids.h"
#include "to_lab_msgids.h"
#include "to_lab_version.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_EnableOutput() -- TLM output enabled                     */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_EnableOutputCmd(const TO_LAB_EnableOutputCmd_t *data)
{
    const TO_LAB_EnableOutput_Payload_t *pCmd = &data->Payload;

    (void)CFE_SB_MessageStringGet(TO_LAB_Global.tlm_dest_IP, pCmd->dest_IP, "", sizeof(TO_LAB_Global.tlm_dest_IP),
                                  sizeof(pCmd->dest_IP));
    TO_LAB_Global.suppress_sendto = false;
    CFE_EVS_SendEvent(TO_LAB_TLMOUTENA_INF_EID, CFE_EVS_EventType_INFORMATION, "TO telemetry output enabled for IP %s",
                      TO_LAB_Global.tlm_dest_IP);

    if (!TO_LAB_Global.downlink_on) /* Then turn it on, otherwise we will just switch destination addresses*/
    {
        TO_LAB_openTLM();
        TO_LAB_Global.downlink_on = true;
    }

    ++TO_LAB_Global.HkTlm.Payload.CommandCounter;
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_Noop() -- Noop Handler                                   */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_NoopCmd(const TO_LAB_NoopCmd_t *data)
{
  char VersionString[TO_LAB_CFG_MAX_VERSION_STR_LEN];

  CFE_Config_GetVersionString(VersionString, TO_LAB_CFG_MAX_VERSION_STR_LEN, "TO Lab",
                        TO_LAB_VERSION, TO_LAB_BUILD_CODENAME, TO_LAB_LAST_OFFICIAL);

  CFE_EVS_SendEvent(TO_LAB_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION, "TO: NOOP command. %s", VersionString);

  ++TO_LAB_Global.HkTlm.Payload.CommandCounter;
  return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_ResetCounters() -- Reset counters                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_ResetCountersCmd(const TO_LAB_ResetCountersCmd_t *data)
{
    TO_LAB_Global.HkTlm.Payload.CommandErrorCounter = 0;
    TO_LAB_Global.HkTlm.Payload.CommandCounter      = 0;

    CFE_EVS_SendEvent(TO_LAB_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "Reset counters command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_SendDataTypes()  -- Output data types                    */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_SendDataTypesCmd(const TO_LAB_SendDataTypesCmd_t *data)
{
    int16 i;
    char  string_variable[10] = "ABCDEFGHIJ";

    /* initialize data types packet */
    CFE_MSG_Init(CFE_MSG_PTR(TO_LAB_Global.DataTypesTlm.TelemetryHeader), CFE_SB_ValueToMsgId(TO_LAB_DATA_TYPES_MID),
                 sizeof(TO_LAB_Global.DataTypesTlm));

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(TO_LAB_Global.DataTypesTlm.TelemetryHeader));

    /* initialize the packet data */
    TO_LAB_Global.DataTypesTlm.Payload.synch = 0x6969;
#if 0
    TO_LAB_Global.DataTypesTlm.Payload.bit1 = 1;
    TO_LAB_Global.DataTypesTlm.Payload.bit2 = 0;
    TO_LAB_Global.DataTypesTlm.Payload.bit34 = 2;
    TO_LAB_Global.DataTypesTlm.Payload.bit56 = 3;
    TO_LAB_Global.DataTypesTlm.Payload.bit78 = 1;
    TO_LAB_Global.DataTypesTlm.Payload.nibble1 = 0xA;
    TO_LAB_Global.DataTypesTlm.Payload.nibble2 = 0x4;
#endif
    TO_LAB_Global.DataTypesTlm.Payload.bl1 = false;
    TO_LAB_Global.DataTypesTlm.Payload.bl2 = true;
    TO_LAB_Global.DataTypesTlm.Payload.b1  = 16;
    TO_LAB_Global.DataTypesTlm.Payload.b2  = 127;
    TO_LAB_Global.DataTypesTlm.Payload.b3  = 0x7F;
    TO_LAB_Global.DataTypesTlm.Payload.b4  = 0x45;
    TO_LAB_Global.DataTypesTlm.Payload.w1  = 0x2468;
    TO_LAB_Global.DataTypesTlm.Payload.w2  = 0x7FFF;
    TO_LAB_Global.DataTypesTlm.Payload.dw1 = 0x12345678;
    TO_LAB_Global.DataTypesTlm.Payload.dw2 = 0x87654321;
    TO_LAB_Global.DataTypesTlm.Payload.f1  = 90.01;
    TO_LAB_Global.DataTypesTlm.Payload.f2  = .0000045;
    TO_LAB_Global.DataTypesTlm.Payload.df1 = 99.9;
    TO_LAB_Global.DataTypesTlm.Payload.df2 = .4444;

    for (i = 0; i < 10; i++)
        TO_LAB_Global.DataTypesTlm.Payload.str[i] = string_variable[i];

    CFE_SB_TransmitMsg(CFE_MSG_PTR(TO_LAB_Global.DataTypesTlm.TelemetryHeader), true);

    ++TO_LAB_Global.HkTlm.Payload.CommandCounter;
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_SendHousekeeping() -- HK status                          */
/* Does not increment CommandCounter                               */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_SendHkCmd(const TO_LAB_SendHkCmd_t *data)
{
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(TO_LAB_Global.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(TO_LAB_Global.HkTlm.TelemetryHeader), true);
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_AddPacket() -- Add packets                               */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_AddPacketCmd(const TO_LAB_AddPacketCmd_t *data)
{
    const TO_LAB_AddPacket_Payload_t *pCmd = &data->Payload;
    int32                             status;

    status = CFE_SB_SubscribeEx(pCmd->Stream, TO_LAB_Global.Tlm_pipe, pCmd->Flags, pCmd->BufLimit);

    if (status != CFE_SUCCESS)
        CFE_EVS_SendEvent(TO_LAB_ADDPKT_ERR_EID, CFE_EVS_EventType_ERROR, "L%d TO Can't subscribe 0x%x status %i",
                          __LINE__, (unsigned int)CFE_SB_MsgIdToValue(pCmd->Stream), (int)status);
    else
        CFE_EVS_SendEvent(TO_LAB_ADDPKT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "L%d TO AddPkt 0x%x, QoS %d.%d, limit %d", __LINE__,
                          (unsigned int)CFE_SB_MsgIdToValue(pCmd->Stream), pCmd->Flags.Priority,
                          pCmd->Flags.Reliability, pCmd->BufLimit);

    ++TO_LAB_Global.HkTlm.Payload.CommandCounter;
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_RemovePacket() -- Remove Packet                          */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_RemovePacketCmd(const TO_LAB_RemovePacketCmd_t *data)
{
    const TO_LAB_RemovePacket_Payload_t *pCmd = &data->Payload;
    int32                                status;

    status = CFE_SB_Unsubscribe(pCmd->Stream, TO_LAB_Global.Tlm_pipe);
    if (status != CFE_SUCCESS)
        CFE_EVS_SendEvent(TO_LAB_REMOVEPKT_ERR_EID, CFE_EVS_EventType_ERROR,
                          "L%d TO Can't Unsubscribe to Stream 0x%x, status %i", __LINE__,
                          (unsigned int)CFE_SB_MsgIdToValue(pCmd->Stream), (int)status);
    else
        CFE_EVS_SendEvent(TO_LAB_REMOVEPKT_INF_EID, CFE_EVS_EventType_INFORMATION, "L%d TO RemovePkt 0x%x", __LINE__,
                          (unsigned int)CFE_SB_MsgIdToValue(pCmd->Stream));
    ++TO_LAB_Global.HkTlm.Payload.CommandCounter;
    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_RemoveAll() --  Remove All Packets                       */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
CFE_Status_t TO_LAB_RemoveAllCmd(const TO_LAB_RemoveAllCmd_t *data)
{
    int32         status;
    int           i;
    TO_LAB_Sub_t *SubEntry;

    SubEntry = TO_LAB_Global.SubsTblPtr->Subs;
    for (i = 0; i < TO_LAB_MAX_SUBSCRIPTIONS; i++)
    {
        if (CFE_SB_IsValidMsgId(SubEntry->Stream))
        {
            status = CFE_SB_Unsubscribe(SubEntry->Stream, TO_LAB_Global.Tlm_pipe);

            if (status != CFE_SUCCESS)
                CFE_EVS_SendEvent(TO_LAB_REMOVEALLPTKS_ERR_EID, CFE_EVS_EventType_ERROR,
                                  "L%d TO Can't Unsubscribe to stream 0x%x status %i", __LINE__,
                                  (unsigned int)CFE_SB_MsgIdToValue(SubEntry->Stream), (int)status);
        }
    }

    CFE_EVS_SendEvent(TO_LAB_REMOVEALLPKTS_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "L%d TO Unsubscribed to all Commands and Telemetry", __LINE__);

    ++TO_LAB_Global.HkTlm.Payload.CommandCounter;
    return CFE_SUCCESS;
}
