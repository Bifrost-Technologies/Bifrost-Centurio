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
 *   Specification for the TO_LAB command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums, typedefs, or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef TO_LAB_FCNCODES_H
#define TO_LAB_FCNCODES_H

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*
** TO_LAB command codes
*/
#define TO_LAB_NOOP_CC            0 /*  no-op command     */
#define TO_LAB_RESET_STATUS_CC    1 /*  reset status      */
#define TO_LAB_ADD_PKT_CC         2 /*  add packet        */
#define TO_LAB_SEND_DATA_TYPES_CC 3 /*  send data types   */
#define TO_LAB_REMOVE_PKT_CC      4 /*  remove packet     */
#define TO_LAB_REMOVE_ALL_PKT_CC  5 /*  remove all packet */
#define TO_LAB_OUTPUT_ENABLE_CC   6 /*  output enable     */

#endif
