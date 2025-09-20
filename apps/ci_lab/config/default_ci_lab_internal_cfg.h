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
 *   CI_LAB Application Private Config Definitions
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
#ifndef CI_LAB_INTERNAL_CFG_H
#define CI_LAB_INTERNAL_CFG_H

/**
 * @brief The size of the input buffer
 *
 * This definition controls the maximum size message that can be ingested
 * from the UDP socket
 */
#define CI_LAB_MAX_INGEST 768

/**
 * @brief Number of packets to process per ingest cycle
 */
#define CI_LAB_MAX_INGEST_PKTS 10

/**
 * @brief SB Receive timeout
 */
#define CI_LAB_SB_RECEIVE_TIMEOUT 500

/**
 * @brief Uplink Receive timeout
 */
#define CI_LAB_UPLINK_RECEIVE_TIMEOUT OS_CHECK

/**
 * @brief The depth of the command input pipe
 *
 * This controls the depth of the SB input pipe
 */
#define CI_LAB_PIPE_DEPTH 32

#endif
