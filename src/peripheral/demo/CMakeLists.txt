target_sources(${PROJECT_NAME}
  PRIVATE
    demoDevice.cc
    # demoFactory.ccm
    demoInput.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    demoDevice.ccm
    # demoFactory.ccm
    demoInput.ccm
)
