###########################################################
#
# UE5_BRIDGE mission build setup
#
###########################################################

set(UE5_BRIDGE_MISSION_CONFIG_FILE_LIST
  ue5_bridge_mission_cfg.h
  ue5_bridge_topicids.h
)

if (CFE_EDS_ENABLED_BUILD)
  # no EDS for bridge (JSON over UDP)
endif()

foreach(UE5_BRIDGE_CFGFILE ${UE5_BRIDGE_MISSION_CONFIG_FILE_LIST})
  get_filename_component(CFGKEY "${UE5_BRIDGE_CFGFILE}" NAME_WE)
  if (DEFINED UE5_BRIDGE_CFGFILE_SRC_${CFGKEY})
    set(DEFAULT_SOURCE GENERATED_FILE "${UE5_BRIDGE_CFGFILE_SRC_${CFGKEY}}")
  else()
    set(DEFAULT_SOURCE FALLBACK_FILE "${CMAKE_CURRENT_LIST_DIR}/config/default_${UE5_BRIDGE_CFGFILE}")
  endif()
  generate_config_includefile(
    FILE_NAME           "${UE5_BRIDGE_CFGFILE}"
    ${DEFAULT_SOURCE}
  )
endforeach()
