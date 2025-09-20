###########################################################
# CENTURIO_NAV mission build setup
###########################################################

set(CENTURIO_NAV_MISSION_CONFIG_FILE_LIST
  centurio_nav_fcncodes.h
  centurio_nav_interface_cfg.h
  centurio_nav_mission_cfg.h
  centurio_nav_perfids.h
  centurio_nav_msg.h
  centurio_nav_msgdefs.h
  centurio_nav_msgstruct.h
  centurio_nav_topicids.h
)

# Map generated/override sources like sample_app does
if (CFE_EDS_ENABLED_BUILD)
  set(CENTURIO_NAV_CFGFILE_SRC_centurio_nav_interface_cfg "centurio_nav_eds_designparameters.h")
  set(CENTURIO_NAV_CFGFILE_SRC_centurio_nav_tbldefs       "centurio_nav_eds_typedefs.h")
  set(CENTURIO_NAV_CFGFILE_SRC_centurio_nav_tblstruct     "centurio_nav_eds_typedefs.h")
  set(CENTURIO_NAV_CFGFILE_SRC_centurio_nav_msgdefs       "centurio_nav_eds_typedefs.h")
  set(CENTURIO_NAV_CFGFILE_SRC_centurio_nav_msgstruct     "centurio_nav_eds_typedefs.h")
  set(CENTURIO_NAV_CFGFILE_SRC_centurio_nav_fcncodes      "centurio_nav_eds_cc.h")
endif()

foreach(CENTURIO_NAV_CFGFILE ${CENTURIO_NAV_MISSION_CONFIG_FILE_LIST})
  get_filename_component(CFGKEY "${CENTURIO_NAV_CFGFILE}" NAME_WE)
  if (DEFINED CENTURIO_NAV_CFGFILE_SRC_${CFGKEY})
    set(DEFAULT_SOURCE GENERATED_FILE "${CENTURIO_NAV_CFGFILE_SRC_${CFGKEY}}")
  else()
    set(DEFAULT_SOURCE FALLBACK_FILE "${CMAKE_CURRENT_LIST_DIR}/config/default_${CENTURIO_NAV_CFGFILE}")
  endif()
  generate_config_includefile(
    FILE_NAME           "${CENTURIO_NAV_CFGFILE}"
    ${DEFAULT_SOURCE}
  )
endforeach()
