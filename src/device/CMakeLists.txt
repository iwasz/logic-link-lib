target_sources(${PROJECT_NAME}
  PRIVATE
    # factory.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    device.ccm
    input.ccm
    factory.ccm
)

add_subdirectory(usb)
add_subdirectory(demo)