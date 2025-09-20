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
 *   This file contains the source code for the Command Ingest task.
 */

/*
**   Include Files:
*/

#include "ci_lab_app.h"
#include "ci_lab_perfids.h"
#include "ci_lab_msgids.h"
#include "ci_lab_version.h"

#include "cfe_config.h"

#include "edslib_datatypedb.h"
#include "ci_lab_eds_typedefs.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

#include "cfe_hdr_eds_datatypes.h"

/*
 * ---------------------------------------
 * In an EDS configuration - the data from the network is encoded
 * and needs to be read into an intermediate buffer first
 * ---------------------------------------
 */
CFE_Status_t CI_LAB_GetInputBuffer(void **BufferOut, size_t *SizeOut)
{
    static EdsPackedBuffer_CFE_HDR_CommandHeader_t InputBuffer;

    *BufferOut = &InputBuffer;
    *SizeOut   = sizeof(InputBuffer);

    return CFE_SUCCESS;
}

/*
 * ---------------------------------------
 * In an EDS configuration - the data from the network is encoded
 * and this function translates the intermediate buffer content
 * to an instance of the CFE_SB_Buffer_t that can be sent to SB.
 * ---------------------------------------
 */
CFE_Status_t CI_LAB_DecodeInputMessage(void *SourceBuffer, size_t SourceSize, CFE_SB_Buffer_t **DestBufferOut)
{
    int32                                 EdsStatus;
    uint32                                BitSize;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
    CFE_SB_Listener_Component_t           ListenerParams;
    EdsLib_DataTypeDB_TypeInfo_t          CmdHdrInfo;
    EdsLib_Id_t                           EdsId;
    CFE_SB_Buffer_t *                     IngestBufPtr;
    CFE_Status_t                          ResultStatus;

    const EdsLib_DatabaseObject_t *EDS_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    ResultStatus = CFE_STATUS_VALIDATION_FAILURE;
    IngestBufPtr = NULL;

    do
    {
        EdsId     = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), CFE_HDR_CommandHeader_DATADICTIONARY);
        EdsStatus = EdsLib_DataTypeDB_GetTypeInfo(EDS_DB, EdsId, &CmdHdrInfo);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            OS_printf("EdsLib_DataTypeDB_GetTypeInfo(): %d\n", (int)EdsStatus);
            break;
        }

        /* sanity check - validate the incoming packet is at least the size of a command header */
        BitSize = 8 * SourceSize;
        if (BitSize < CmdHdrInfo.Size.Bits)
        {
            OS_printf("CI_LAB: Size mismatch, BitSize=%lu (packet) / %lu (min)\n", (unsigned long)BitSize,
                      (unsigned long)CmdHdrInfo.Size.Bits);

            ResultStatus = CFE_STATUS_WRONG_MSG_LENGTH;
            break;
        }

        /* Now get a SB Buffer, as a place to put the decoded data */
        IngestBufPtr = CFE_SB_AllocateMessageBuffer(sizeof(EdsNativeBuffer_CFE_HDR_CommandHeader_t));
        if (IngestBufPtr == NULL)
        {
            CFE_EVS_SendEvent(CI_LAB_INGEST_ALLOC_ERR_EID, CFE_EVS_EventType_ERROR,
                              "CI_LAB: buffer allocation failed\n");

            ResultStatus = CFE_SB_BUF_ALOC_ERR;
            break;
        }

        /* Packet is in external wire-format byte order - unpack it and copy */
        EdsId     = EDSLIB_MAKE_ID(EDS_INDEX(CFE_HDR), CFE_HDR_CommandHeader_DATADICTIONARY);
        EdsStatus = EdsLib_DataTypeDB_UnpackPartialObject(EDS_DB, &EdsId, IngestBufPtr, SourceBuffer,
                                                          sizeof(EdsDataType_CFE_HDR_CommandHeader_t), BitSize, 0);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            OS_printf("EdsLib_DataTypeDB_UnpackPartialObject(1): %d\n", (int)EdsStatus);
            break;
        }

        /* Header decoded successfully - Now need to determine the type for the rest of the payload */
        CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &IngestBufPtr->Msg.BaseMsg);
        CFE_MissionLib_UnmapListenerComponent(&ListenerParams, &PubSubParams);

        EdsStatus = CFE_MissionLib_GetArgumentType(&CFE_SOFTWAREBUS_INTERFACE, EDS_INTERFACE_ID(CFE_SB_Telecommand),
                                                   ListenerParams.Telecommand.TopicId, 1, 1, &EdsId);
        if (EdsStatus != CFE_MISSIONLIB_SUCCESS)
        {
            OS_printf("CFE_MissionLib_GetArgumentType(): %d\n", (int)EdsStatus);
            break;
        }

        EdsStatus = EdsLib_DataTypeDB_UnpackPartialObject(EDS_DB, &EdsId, IngestBufPtr, SourceBuffer,
                                                          sizeof(EdsNativeBuffer_CFE_HDR_CommandHeader_t), BitSize,
                                                          sizeof(EdsDataType_CFE_HDR_CommandHeader_t));
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            OS_printf("EdsLib_DataTypeDB_UnpackPartialObject(2): %d\n", (int)EdsStatus);
            break;
        }

        /* Verify that the checksum and basic fields are correct, and recompute the length entry */
        EdsStatus = EdsLib_DataTypeDB_VerifyUnpackedObject(EDS_DB, EdsId, IngestBufPtr, SourceBuffer,
                                                           EDSLIB_DATATYPEDB_RECOMPUTE_LENGTH);
        if (EdsStatus != EDSLIB_SUCCESS)
        {
            OS_printf("EdsLib_DataTypeDB_VerifyUnpackedObject(): %d\n", (int)EdsStatus);
            break;
        }

        /* Finally - at this point, we should have a fully decoded buffer */
        ResultStatus = CFE_SUCCESS;
    } while (false);

    if (ResultStatus != CFE_SUCCESS && IngestBufPtr != NULL)
    {
        CFE_SB_ReleaseMessageBuffer(IngestBufPtr);
        IngestBufPtr = NULL;
    }

    *DestBufferOut = IngestBufPtr;

    return ResultStatus;
}
