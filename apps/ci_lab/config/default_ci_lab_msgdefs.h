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
 *   Specification for the CI_LAB command and telemetry
 *   message constant definitions.
 *
 *  For CI_LAB this is only the function/command code definitions
 */
#ifndef CI_LAB_MSGDEFS_H
#define CI_LAB_MSGDEFS_H

#include "ci_lab_fcncodes.h"

/*************************************************************************/
/*
** Payload definition (CI_LAB housekeeping)...
*/
typedef struct
{
    uint8  CommandErrorCounter;
    uint8  CommandCounter;
    uint8  EnableChecksums;
    uint8  SocketConnected;
    uint8  Spare1[8];
    uint32 IngestPackets;
    uint32 IngestErrors;
    uint32 Spare2;
} CI_LAB_HkTlm_Payload_t;

#endif
