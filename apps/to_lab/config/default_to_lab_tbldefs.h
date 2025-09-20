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
 *   Specification for the TO_LAB table related
 *   constant definitions.
 *
 * @note
 *   These Macro definitions have been put in this file (instead of
 *   to_lab_tbl.h). DO NOT PUT ANY TYPEDEFS OR
 *   STRUCTURE DEFINITIONS IN THIS FILE!
 *   ADD THEM TO to_lab_tbl.h IF NEEDED!
 */
#ifndef TO_LAB_TBLDEFS_H
#define TO_LAB_TBLDEFS_H

#include "common_types.h"
#include "to_lab_mission_cfg.h"
#include "cfe_sb_extern_typedefs.h"

/************************************************************************
 * Macro Definitions
 ************************************************************************/

typedef struct
{
    CFE_SB_MsgId_t Stream;
    CFE_SB_Qos_t   Flags;
    uint16         BufLimit;
} TO_LAB_Sub_t;

#endif
