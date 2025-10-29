target_sources(${PROJECT_NAME}
  PUBLIC
    backend.cc
    frontend.cc
    types.cc
    block.cc
    downSampler.cc
    blockArray.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    acqParams.ccm
    backend.ccm
    data.ccm
    frontend.ccm
    types.ccm
    queue.ccm
    bitSpan.ccm
    owningBitSpan.ccm
    block.ccm
    downSampler.ccm
    blockArray.ccm
)
