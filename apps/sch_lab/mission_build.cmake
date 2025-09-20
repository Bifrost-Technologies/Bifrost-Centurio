###########################################################
#
# SCH_LAB mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the SCH_LAB configuration
set(SCH_LAB_MISSION_CONFIG_FILE_LIST
  sch_lab_interface_cfg.h
  sch_lab_mission_cfg.h
  sch_lab_perfids.h
  sch_lab_tbldefs.h
  sch_lab_tbl.h
  sch_lab_tblstruct.h
)

if (CFE_EDS_ENABLED_BUILD)

  # In an EDS-based build, these files come generated from the EDS tool
  set(SCH_LAB_CFGFILE_SRC_sch_lab_interface_cfg "sch_lab_eds_designparameters.h")
  set(SCH_LAB_CFGFILE_SRC_sch_lab_tbldefs       "sch_lab_eds_typedefs.h")
  set(SCH_LAB_CFGFILE_SRC_sch_lab_tblstruct     "sch_lab_eds_typedefs.h")

endif(CFE_EDS_ENABLED_BUILD)

# Create wrappers around the all the config header files
# This makes them individually overridable by the missions, without modifying
# the distribution default copies
foreach(SCH_LAB_CFGFILE ${SCH_LAB_MISSION_CONFIG_FILE_LIST})
  get_filename_component(CFGKEY "${SCH_LAB_CFGFILE}" NAME_WE)
  if (DEFINED SCH_LAB_CFGFILE_SRC_${CFGKEY})
    set(DEFAULT_SOURCE GENERATED_FILE "${SCH_LAB_CFGFILE_SRC_${CFGKEY}}")
  else()
    set(DEFAULT_SOURCE FALLBACK_FILE "${CMAKE_CURRENT_LIST_DIR}/config/default_${SCH_LAB_CFGFILE}")
  endif()
  generate_config_includefile(
    FILE_NAME           "${SCH_LAB_CFGFILE}"
    ${DEFAULT_SOURCE}
  )
endforeach()
