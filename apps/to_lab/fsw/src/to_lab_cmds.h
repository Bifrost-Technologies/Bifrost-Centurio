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
 *   Define TO Lab Application header file
 */

#ifndef TO_LAB_CMDS_H
#define TO_LAB_CMDS_H

#include "common_types.h"
#include "cfe_error.h"
#include "to_lab_msg.h"

/******************************************************************************/

/*
** Prototypes Section
*/
CFE_Status_t TO_LAB_AddPacketCmd(const TO_LAB_AddPacketCmd_t *data);
CFE_Status_t TO_LAB_NoopCmd(const TO_LAB_NoopCmd_t *data);
CFE_Status_t TO_LAB_EnableOutputCmd(const TO_LAB_EnableOutputCmd_t *data);
CFE_Status_t TO_LAB_RemoveAllCmd(const TO_LAB_RemoveAllCmd_t *data);
CFE_Status_t TO_LAB_RemovePacketCmd(const TO_LAB_RemovePacketCmd_t *data);
CFE_Status_t TO_LAB_ResetCountersCmd(const TO_LAB_ResetCountersCmd_t *data);
CFE_Status_t TO_LAB_SendDataTypesCmd(const TO_LAB_SendDataTypesCmd_t *data);
CFE_Status_t TO_LAB_SendHkCmd(const TO_LAB_SendHkCmd_t *data);

/******************************************************************************/

#endif
