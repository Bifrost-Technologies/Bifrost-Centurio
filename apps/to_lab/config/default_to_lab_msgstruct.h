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
 * @file
 *   Specification for the TO_LAB command and telemetry
 *   message data types.
 *
 * @note
 *   Constants and enumerated types related to these message structures
 *   are defined in to_lab_msgdefs.h.
 */
#ifndef TO_LAB_MSGSTRUCT_H
#define TO_LAB_MSGSTRUCT_H

/************************************************************************
 * Includes
 ************************************************************************/

#include "to_lab_mission_cfg.h"
#include "to_lab_msgdefs.h"
#include "cfe_msg_hdr.h"

/******************************************************************************/

typedef struct
{
    CFE_MSG_TelemetryHeader_t TelemetryHeader; /**< \brief Telemetry header */
    TO_LAB_HkTlm_Payload_t    Payload;         /**< \brief Telemetry payload */
} TO_LAB_HkTlm_t;

/******************************************************************************/

typedef struct
{
    CFE_MSG_TelemetryHeader_t  TelemetryHeader; /**< \brief Telemetry header */
    TO_LAB_DataTypes_Payload_t Payload;         /**< \brief Telemetry payload */
} TO_LAB_DataTypesTlm_t;

/******************************************************************************/

/*
 * The following commands do not have any payload,
 * but should still "reserve" a unique structure type to
 * employ a consistent handler pattern.
 *
 * This matches the pattern in CFE core and other modules.
 */
typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} TO_LAB_SendHkCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} TO_LAB_NoopCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} TO_LAB_ResetCountersCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} TO_LAB_RemoveAllCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} TO_LAB_SendDataTypesCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t    CommandHeader; /**< \brief Command header */
    TO_LAB_AddPacket_Payload_t Payload;       /**< \brief Command payload */
} TO_LAB_AddPacketCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t       CommandHeader; /**< \brief Command header */
    TO_LAB_RemovePacket_Payload_t Payload;       /**< \brief Command payload */
} TO_LAB_RemovePacketCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t       CommandHeader; /**< \brief Command header */
    TO_LAB_EnableOutput_Payload_t Payload;       /**< \brief Command payload */
} TO_LAB_EnableOutputCmd_t;

#endif /* TO_LAB_MSGSTRUCT_H */
