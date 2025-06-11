target_sources(${PROJECT_NAME}
  PUBLIC
    types.cc
    backend.cc
    frontend.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    types.ccm
    acqParams.ccm
    frontend.ccm
    backend.ccm
)
