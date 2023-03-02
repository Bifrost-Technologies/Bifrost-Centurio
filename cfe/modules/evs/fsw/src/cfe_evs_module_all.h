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
 *
 * Encapsulates all EVS module internal header files, as well
 * as the public API from all other CFE core modules, OSAL, and PSP.
 *
 * This simplifies the set of include files that need to be put at the
 * start of every source file.
 */

#ifndef CFE_EVS_MODULE_ALL_H
#define CFE_EVS_MODULE_ALL_H

/********************* Include Files  ************************/

#include "cfe.h" /* All CFE+OSAL public API definitions */
#include "cfe_platform_cfg.h"

#include "cfe_msgids.h"
#include "cfe_perfids.h"

#include "cfe_evs_core_internal.h"

#include "cfe_evs_events.h" /* EVS event IDs */
#include "cfe_evs_task.h"   /* EVS internal definitions */
#include "cfe_evs_log.h"    /* EVS log file definitions */
#include "cfe_evs_utils.h"  /* EVS utility function definitions */

#endif /* CFE_EVS_MODULE_ALL_H */
