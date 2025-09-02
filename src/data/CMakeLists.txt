target_sources(${PROJECT_NAME}
  PUBLIC
    backend.cc
    frontend.cc
    types.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    acqParams.ccm
    backend.ccm
    data.ccm
    frontend.ccm
    types.ccm
)
