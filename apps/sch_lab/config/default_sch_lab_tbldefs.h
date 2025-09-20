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
 *   Specification for the CFS SCH_LAB table related data structures and
 *   constant definitions.
 *
 * The structure definitions in this file are closely related to the implementation
 * of the application.  Any modification to these structures will likely need
 * a correpsonding update to the source code.
 */
#ifndef SCH_LAB_TBLDEFS_H
#define SCH_LAB_TBLDEFS_H

#include "cfe_sb_extern_typedefs.h" /* for CFE_SB_MsgId_t */
#include "cfe_msg_api_typedefs.h"   /* For CFE_MSG_FcnCode_t */
#include "cfe_msgids.h"

#include "sch_lab_mission_cfg.h"

/*
** Typedefs
*/
typedef struct
{
    CFE_SB_MsgId_t    MessageID;                                 /* Message ID for the table entry */
    uint32            PacketRate;                                /* Rate: Send packet every N ticks */
    CFE_MSG_FcnCode_t FcnCode;                                   /* Command/Function code to set */
    uint16            PayloadLength;                             /* Length of additional command args */
    uint16            MessageBuffer[SCH_LAB_MAX_ARGS_PER_ENTRY]; /* Command args in 16 bit words */
} SCH_LAB_ScheduleTableEntry_t;

#endif
