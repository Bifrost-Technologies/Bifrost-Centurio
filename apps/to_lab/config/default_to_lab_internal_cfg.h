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
 *   TO_LAB Application Private Config Definitions
 *
 * This provides default values for configurable items that are internal
 * to this module and do NOT affect the interface(s) of this module.  Changes
 * to items in this file only affect the local module and will be transparent
 * to external entities that are using the public interface(s).
 *
 * @note This file may be overridden/superceded by mission-provided defintions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef TO_LAB_INTERNAL_CFG_H
#define TO_LAB_INTERNAL_CFG_H

/*****************************************************************************/

/**
 * @brief Main loop task delay
 */
#define TO_LAB_TASK_MSEC 500 /* run at 2 Hz */

/**
 * @brief Telemetry pipe timeout
 */
#define TO_LAB_TLM_PIPE_TIMEOUT CFE_SB_POLL

/**
 * @brief Maximum number of telemetry packets to send each wakeup
 */
#define TO_LAB_MAX_TLM_PKTS OS_QUEUE_MAX_DEPTH

/**
 * Depth of pipe for commands to the TO_LAB application itself
 */
#define TO_LAB_CMD_PIPE_DEPTH 8

/**
 * Depth of pipe for telemetry forwarded through the TO_LAB application
 */
#define TO_LAB_TLM_PIPE_DEPTH OS_QUEUE_MAX_DEPTH

#endif
