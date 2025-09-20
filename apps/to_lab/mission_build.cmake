###########################################################
#
# TO_LAB mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the TO_LAB configuration
set(TO_LAB_MISSION_CONFIG_FILE_LIST
  to_lab_fcncodes.h
  to_lab_interface_cfg.h
  to_lab_mission_cfg.h
  to_lab_perfids.h
  to_lab_msg.h
  to_lab_msgdefs.h
  to_lab_msgstruct.h
  to_lab_tbl.h
  to_lab_tbldefs.h
  to_lab_tblstruct.h
  to_lab_topicids.h
)

if (CFE_EDS_ENABLED_BUILD)

  # In an EDS-based build, these files come generated from the EDS tool
  set(TO_LAB_CFGFILE_SRC_to_lab_interface_cfg "to_lab_eds_designparameters.h")
  set(TO_LAB_CFGFILE_SRC_to_lab_tbldefs       "to_lab_eds_typedefs.h")
  set(TO_LAB_CFGFILE_SRC_to_lab_tblstruct     "to_lab_eds_typedefs.h")
  set(TO_LAB_CFGFILE_SRC_to_lab_msgdefs       "to_lab_eds_typedefs.h")
  set(TO_LAB_CFGFILE_SRC_to_lab_msgstruct     "to_lab_eds_typedefs.h")
  set(TO_LAB_CFGFILE_SRC_to_lab_fcncodes      "to_lab_eds_cc.h")

endif(CFE_EDS_ENABLED_BUILD)

# Create wrappers around the all the config header files
# This makes them individually overridable by the missions, without modifying
# the distribution default copies
foreach(TO_LAB_CFGFILE ${TO_LAB_MISSION_CONFIG_FILE_LIST})
  get_filename_component(CFGKEY "${TO_LAB_CFGFILE}" NAME_WE)
  if (DEFINED TO_LAB_CFGFILE_SRC_${CFGKEY})
    set(DEFAULT_SOURCE GENERATED_FILE "${TO_LAB_CFGFILE_SRC_${CFGKEY}}")
  else()
    set(DEFAULT_SOURCE FALLBACK_FILE "${CMAKE_CURRENT_LIST_DIR}/config/default_${TO_LAB_CFGFILE}")
  endif()
  generate_config_includefile(
    FILE_NAME           "${TO_LAB_CFGFILE}"
    ${DEFAULT_SOURCE}
  )
endforeach()
