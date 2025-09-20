/************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
** File: to_lab_app.c
**
** Purpose:
**  his file contains the source code for the TO lab application
**
** Notes:
**
*************************************************************************/

#include "to_lab_app.h"
#include "to_lab_eventids.h"
#include "to_lab_dispatch.h"
#include "to_lab_cmds.h"

#include "to_lab_eds_dictionary.h"
#include "to_lab_eds_dispatcher.h"

static const EdsDispatchTable_TO_LAB_Application_CFE_SB_Telecommand_t TO_LAB_TC_DISPATCH_TABLE = {
    .CMD     = {.AddPacketCmd_indication     = TO_LAB_AddPacketCmd,
            .NoopCmd_indication          = TO_LAB_NoopCmd,
            .EnableOutputCmd_indication  = TO_LAB_EnableOutputCmd,
            .RemoveAllCmd_indication     = TO_LAB_RemoveAllCmd,
            .RemovePacketCmd_indication  = TO_LAB_RemovePacketCmd,
            .ResetCountersCmd_indication = TO_LAB_ResetCountersCmd,
            .SendDataTypesCmd_indication = TO_LAB_SendDataTypesCmd},
    .SEND_HK = {.indication = TO_LAB_SendHkCmd}};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* TO_LAB_TaskPipe() -- Process command pipe message           */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void TO_LAB_TaskPipe(const CFE_SB_Buffer_t *SbBufPtr)
{
    CFE_Status_t      status;
    CFE_SB_MsgId_t    MsgId;
    CFE_MSG_Size_t    MsgSize;
    CFE_MSG_FcnCode_t MsgFc;

    status = EdsDispatch_TO_LAB_Application_Telecommand(SbBufPtr, &TO_LAB_TC_DISPATCH_TABLE);

    if (status != CFE_SUCCESS)
    {
        CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);
        ++TO_LAB_Global.HkTlm.Payload.CommandErrorCounter;

        if (status == CFE_STATUS_UNKNOWN_MSG_ID)
        {
            CFE_EVS_SendEvent(TO_LAB_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "L%d TO: Invalid Msg ID Rcvd 0x%x status=0x%08x", __LINE__,
                              (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)status);
        }
        else if (status == CFE_STATUS_WRONG_MSG_LENGTH)
        {
            CFE_MSG_GetSize(&SbBufPtr->Msg, &MsgSize);
            CFE_MSG_GetFcnCode(&SbBufPtr->Msg, &MsgFc);
            CFE_EVS_SendEvent(TO_LAB_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Invalid length for command: ID = 0x%X, CC = %d, length = %u",
                              (unsigned int)CFE_SB_MsgIdToValue(MsgId), (int)MsgFc, (unsigned int)MsgSize);
        }
        else
        {
            CFE_MSG_GetFcnCode(&SbBufPtr->Msg, &MsgFc);
            CFE_EVS_SendEvent(TO_LAB_FNCODE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "L%d TO: Invalid Function Code Rcvd In Ground Command 0x%x", __LINE__,
                              (unsigned int)MsgFc);
            ++TO_LAB_Global.HkTlm.Payload.CommandErrorCounter;
        }
    }
} /* End of TO_LAB_TaskPipe() */
