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

/*! @file
 * @brief Purpose:
 *  @details Provide version identifiers for the ELF to cFE Table Converter. @n
 *  See @ref cfsversions for version and build number and description
 *
 */
#ifndef ELF2CFETBL_VERSION_H
#define ELF2CFETBL_VERSION_H

/*
 * Development Build Macro Definitions
 */
#define ELF2CFETBL_BUILD_NUMBER     22 /*!< @brief Number of commits since baseline */
#define ELF2CFETBL_BUILD_BASELINE   "equuleues-rc1" /*!< @brief Development Build: git tag that is the base for the current */
#define ELF2CFETBL_BUILD_DEV_CYCLE  "equuleus-rc2" /**< @brief Development: Release name for current development cycle */
#define ELF2CFETBL_BUILD_CODENAME   "Equuleus" /**< @brief: Development: Code name for the current build */

/*
 * Version Macros, see \ref cfsversions for definitions.
 */
#define ELF2CFETBL_MAJOR_VERSION 3  /*!< @brief Major version number */
#define ELF2CFETBL_MINOR_VERSION 1  /*!< @brief Minor version number */
#define ELF2CFETBL_REVISION      0  /*!< @brief Revision version number. Value of 0 indicates a development version.*/

/**
 * @brief Last official release.
 */
#define ELF2CFETBL_LAST_OFFICIAL "v3.1.0"

/*!
 * @brief Mission revision.
 *
 * Reserved for mission use to denote patches/customizations as needed.
 * Values 1-254 are reserved for mission use to denote patches/customizations as needed. NOTE: Reserving 0 and 0xFF for
 * cFS open-source development use (pending resolution of nasa/cFS#440)
 */
#define ELF2CFETBL_MISSION_REV 0xFF

/*
 * Tools to construct version string
 */
#define ELF2CFETBL_STR_HELPER(x) #x /*!< @brief Helper function to concatenate strings from integer macros */
#define ELF2CFETBL_STR(x) \
    ELF2CFETBL_STR_HELPER(x) /*!< @brief Helper function to concatenate strings from integer macros */

/*! @brief Development Build Version Number.
 * @details Baseline git tag + Number of commits since baseline. @n
 * See @ref cfsversions for format differences between development and release versions.
 */
#define ELF2CFETBL_VERSION ELF2CFETBL_BUILD_BASELINE ELF2CFETBL_STR(ELF2CFETBL_BUILD_NUMBER)

/**
 * @brief Max Version String length.
 * 
 * Maximum length that an elf2cfetbl version string can be.
 * 
 */
#define ELF2CFETBL_CFG_MAX_VERSION_STR_LEN 256

#endif /* ELF2CFETBL_VERSION_H */
