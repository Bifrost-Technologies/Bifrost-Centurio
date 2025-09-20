project(CFE_UE5_BRIDGE_APP C)

set(APP_SRC_FILES
  fsw/src/ue5_bridge_app.c
)

# Create the app module
add_cfe_app(ue5_bridge ${APP_SRC_FILES})

target_include_directories(ue5_bridge PUBLIC fsw/inc)
