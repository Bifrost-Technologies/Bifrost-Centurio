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
 *   Specification for the TO_LAB table structures
 *
 * Provides default definitions for TO_LAB table structures
 *
 * @note This file may be overridden/superceded by mission-provided defintions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef TO_LAB_TBLSTRUCT_H
#define TO_LAB_TBLSTRUCT_H

/*************************************************************************
 * Includes
 *************************************************************************/
#include "to_lab_tbldefs.h"

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*************************************************************************
 * Type Definitions
 *************************************************************************/

typedef struct
{
    TO_LAB_Sub_t Subs[TO_LAB_MAX_SUBSCRIPTIONS];
} TO_LAB_Subs_t;

#endif
