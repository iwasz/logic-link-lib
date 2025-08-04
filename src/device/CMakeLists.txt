target_sources(${PROJECT_NAME}
  PRIVATE
    device.cc
    logicLink.cc
    factory.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    device.ccm
    logicLink.ccm
    factory.ccm
)
