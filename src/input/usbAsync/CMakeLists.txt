target_sources(${PROJECT_NAME}
  PRIVATE
    usbAsync.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    usbAsync.ccm
)
