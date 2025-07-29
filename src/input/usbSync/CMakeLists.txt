target_sources(${PROJECT_NAME}
  PRIVATE
    request.cc
    synchronous.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    request.ccm
    usb.ccm
)
