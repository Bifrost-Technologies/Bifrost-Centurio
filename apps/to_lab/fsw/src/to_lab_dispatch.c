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

#include "to_lab_app.h"
#include "to_lab_dispatch.h"
#include "to_lab_cmds.h"
#include "to_lab_msg.h"
#include "to_lab_eventids.h"
#include "to_lab_msgids.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/*  TO_LAB_ProcessGroundCommand() -- Process local message           */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_LAB_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t FcnCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &FcnCode);

    switch (FcnCode)
    {
        case TO_LAB_NOOP_CC:
            TO_LAB_NoopCmd((const TO_LAB_NoopCmd_t *)SBBufPtr);
            break;

        case TO_LAB_RESET_STATUS_CC:
            TO_LAB_ResetCountersCmd((const TO_LAB_ResetCountersCmd_t *)SBBufPtr);
            break;

        case TO_LAB_SEND_DATA_TYPES_CC:
            TO_LAB_SendDataTypesCmd((const TO_LAB_SendDataTypesCmd_t *)SBBufPtr);
            break;

        case TO_LAB_ADD_PKT_CC:
            TO_LAB_AddPacketCmd((const TO_LAB_AddPacketCmd_t *)SBBufPtr);
            break;

        case TO_LAB_REMOVE_PKT_CC:
            TO_LAB_RemovePacketCmd((const TO_LAB_RemovePacketCmd_t *)SBBufPtr);
            break;

        case TO_LAB_REMOVE_ALL_PKT_CC:
            TO_LAB_RemoveAllCmd((const TO_LAB_RemoveAllCmd_t *)SBBufPtr);
            break;

        case TO_LAB_OUTPUT_ENABLE_CC:
            TO_LAB_EnableOutputCmd((const TO_LAB_EnableOutputCmd_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(TO_LAB_FNCODE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "L%d TO: Invalid Function Code Rcvd In Ground Command 0x%x", __LINE__,
                              (unsigned int)FcnCode);
            ++TO_LAB_Global.HkTlm.Payload.CommandErrorCounter;
            break;
    }
}

void TO_LAB_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    /* For SB return statuses that imply a message: process it. */
    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        case TO_LAB_CMD_MID:
            TO_LAB_ProcessGroundCommand(SBBufPtr);
            break;

        case TO_LAB_SEND_HK_MID:
            TO_LAB_SendHkCmd((const TO_LAB_SendHkCmd_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(TO_LAB_MID_ERR_EID, CFE_EVS_EventType_ERROR, "L%d TO: Invalid Msg ID Rcvd 0x%x",
                              __LINE__, (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }
}
