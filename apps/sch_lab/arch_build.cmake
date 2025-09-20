###########################################################
#
# SCH_LAB platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the SCH_LAB configuration
set(SCH_LAB_PLATFORM_CONFIG_FILE_LIST
  sch_lab_perfids.h
)

# Create wrappers around the all the config header files
# This makes them individually overridable by the missions, without modifying
# the distribution default copies
foreach(SCH_LAB_CFGFILE ${SCH_LAB_PLATFORM_CONFIG_FILE_LIST})
  generate_config_includefile(
    FILE_NAME           "${SCH_LAB_CFGFILE}"
    FALLBACK_FILE       "${CMAKE_CURRENT_LIST_DIR}/config/default_${SCH_LAB_CFGFILE}"
  )
endforeach()
