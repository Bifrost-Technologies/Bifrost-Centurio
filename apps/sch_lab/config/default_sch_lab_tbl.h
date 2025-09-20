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
 *   Specification for the SCH_LAB table structures
 *
 * @note
 *   Constants and enumerated types related to these table structures
 *   are defined in sch_lab_tbldefs.h.
 */
#ifndef SCH_LAB_TBL_H
#define SCH_LAB_TBL_H

#include "sch_lab_tbldefs.h"
#include "sch_lab_tblstruct.h"

/*
** Defines
*/
#ifdef SOFTWARE_BIG_BIT_ORDER
#define SCH_PACK_32BIT(value) (uint16)((value & 0xFFFF0000) >> 16), (uint16)(value & 0x0000FFFF)
#else
#define SCH_PACK_32BIT(value) (uint16)(value & 0x0000FFFF), (uint16)((value & 0xFFFF0000) >> 16)
#endif

/*
 * There is no extra encapsulation here, this header only
 * defines the default file name to use for the SCH table
 */
#define SCH_LAB_TBL_DEFAULT_FILE "/cf/sch_lab_table.tbl"

#endif
