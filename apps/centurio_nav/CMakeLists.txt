project(CFE_CENTURIO_NAV_APP C)

set(APP_SRC_FILES
  fsw/src/centurio_nav.c
  fsw/src/centurio_nav_cmds.c
)

add_cfe_app(centurio_nav ${APP_SRC_FILES})

target_include_directories(centurio_nav PUBLIC fsw/inc)
target_link_libraries(centurio_nav m)
