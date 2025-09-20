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
 * \file
 *  This file contains the source code for the TO lab application
 */

#include "cfe_config.h"
#include "cfe_sb.h"
#include "cfe_msg.h"

#include "to_lab_app.h"
#include "to_lab_encode.h"

/*
 * --------------------------------------------
 * This implements an "encoder" that simply outputs the same pointer that was passed in.
 * This matches the traditional TO behavior where the "C" struct is passed directly to the socket.
 *
 * The only thing this needs to do get the real size of the output datagram, which should be
 * the size stored in the CFE message header.
 * --------------------------------------------
 */
CFE_Status_t TO_LAB_EncodeOutputMessage(const CFE_SB_Buffer_t *SourceBuffer, const void **DestBufferOut,
                                        size_t *DestSizeOut)
{
    CFE_Status_t   ResultStatus;
    CFE_MSG_Size_t SourceBufferSize;

    ResultStatus = CFE_MSG_GetSize(&SourceBuffer->Msg, &SourceBufferSize);

    *DestBufferOut = SourceBuffer;
    *DestSizeOut   = SourceBufferSize;

    return ResultStatus;
}
