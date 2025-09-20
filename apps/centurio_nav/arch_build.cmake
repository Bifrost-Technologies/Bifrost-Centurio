###########################################################
# CENTURIO_NAV platform build setup
###########################################################

set(CENTURIO_NAV_PLATFORM_CONFIG_FILE_LIST
  centurio_nav_internal_cfg.h
  centurio_nav_platform_cfg.h
  centurio_nav_perfids.h
  centurio_nav_msgids.h
)

foreach(CENTURIO_NAV_CFGFILE ${CENTURIO_NAV_PLATFORM_CONFIG_FILE_LIST})
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
