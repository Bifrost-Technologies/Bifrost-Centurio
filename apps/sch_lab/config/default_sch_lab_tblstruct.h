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
 *   Specification for the CFS SCH_LAB table encapsulation structures
 *
 * Provides default definitions for SCH_LAB table structures
 *
 * @note This file may be overridden/superceded by mission-provided defintions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef SCH_LAB_TBLSTRUCT_H
#define SCH_LAB_TBLSTRUCT_H

#include "sch_lab_mission_cfg.h"
#include "sch_lab_tbldefs.h"

typedef struct
{
    uint32                       TickRate; /* Ticks per second to configure for timer (0=default) */
    SCH_LAB_ScheduleTableEntry_t Config[SCH_LAB_MAX_SCHEDULE_ENTRIES];
} SCH_LAB_ScheduleTable_t;

#endif
