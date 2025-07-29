target_sources(${PROJECT_NAME}
  PRIVATE
    device.cc
  #   decompress.cc
    logicLink.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    device.ccm
    logicLink.ccm
    # demoDevice.ccm
)
