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

/* PSP coverage stub replacement for dosFsLib.h */
#ifndef OVERRIDE_DOSFSLIB_H
#define OVERRIDE_DOSFSLIB_H

#include "PCS_dosFsLib.h"
#include <vxWorks.h>

/* ----------------------------------------- */
/* mappings for declarations in dosFsLib.h */
/* ----------------------------------------- */
#define DOS_CHK_ONLY        PCS_DOS_CHK_ONLY
#define DOS_CHK_REPAIR      PCS_DOS_CHK_REPAIR
#define DOS_CHK_VERB_0      PCS_DOS_CHK_VERB_0
#define DOS_CHK_VERB_SILENT PCS_DOS_CHK_VERB_SILENT
#define DOS_OPT_BLANK       PCS_DOS_OPT_BLANK

#define dosFsVolFormat PCS_dosFsVolFormat

#endif
