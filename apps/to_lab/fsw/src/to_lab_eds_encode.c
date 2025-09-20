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

#include "cfe_config.h"
#include "cfe_sb.h"
#include "cfe_msg.h"
#include "cfe_error.h"

#include "to_lab_app.h"
#include "to_lab_encode.h"

#include "edslib_datatypedb.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"

#include "cfe_hdr_eds_datatypes.h"

CFE_Status_t TO_LAB_EncodeOutputMessage(const CFE_SB_Buffer_t *SourceBuffer, const void **DestBufferOut,
                                        size_t *DestSizeOut)
{
    EdsLib_Id_t                           EdsId;
    EdsLib_DataTypeDB_TypeInfo_t          TypeInfo;
    CFE_SB_SoftwareBus_PubSub_Interface_t PubSubParams;
    CFE_SB_Publisher_Component_t          PublisherParams;
    uint16                                TopicId;
    int32                                 EdsStatus;
    CFE_Status_t                          ResultStatus;
    size_t                                SourceBufferSize;

    static EdsPackedBuffer_CFE_HDR_TelemetryHeader_t NetworkBuffer;

    const EdsLib_DatabaseObject_t *EDS_DB = CFE_Config_GetObjPointer(CFE_CONFIGID_MISSION_EDS_DB);

    ResultStatus = CFE_MSG_GetSize(&SourceBuffer->Msg, &SourceBufferSize);
    if (ResultStatus != CFE_SUCCESS)
    {
        return ResultStatus;
    }

    CFE_MissionLib_Get_PubSub_Parameters(&PubSubParams, &SourceBuffer->Msg.BaseMsg);
    CFE_MissionLib_UnmapPublisherComponent(&PublisherParams, &PubSubParams);
    TopicId = PublisherParams.Telemetry.TopicId;

    EdsStatus = CFE_MissionLib_GetArgumentType(&CFE_SOFTWAREBUS_INTERFACE, EDS_INTERFACE_ID(CFE_SB_Telemetry), TopicId,
                                               1, 1, &EdsId);
    if (EdsStatus != CFE_MISSIONLIB_SUCCESS)
    {
        return CFE_STATUS_UNKNOWN_MSG_ID;
    }

    EdsStatus = EdsLib_DataTypeDB_PackCompleteObject(EDS_DB, &EdsId, NetworkBuffer, SourceBuffer,
                                                     8 * sizeof(NetworkBuffer), SourceBufferSize);
    if (EdsStatus != EDSLIB_SUCCESS)
    {
        return CFE_SB_INTERNAL_ERR;
    }

    EdsStatus = EdsLib_DataTypeDB_GetTypeInfo(EDS_DB, EdsId, &TypeInfo);
    if (EdsStatus != EDSLIB_SUCCESS)
    {
        return CFE_SB_INTERNAL_ERR;
    }

    *DestSizeOut   = (TypeInfo.Size.Bits + 7) / 8;
    *DestBufferOut = NetworkBuffer;

    return CFE_SUCCESS;
}
