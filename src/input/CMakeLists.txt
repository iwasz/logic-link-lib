target_sources(${PROJECT_NAME}
  PUBLIC FILE_SET CXX_MODULES FILES
    input.ccm
)

add_subdirectory(usbAsync)
add_subdirectory(demoInput)