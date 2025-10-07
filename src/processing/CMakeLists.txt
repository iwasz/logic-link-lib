target_sources(${PROJECT_NAME}
  PRIVATE
    analysis.cc
    decompress.cc
    rearrange.cc
    generate.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    processing.ccm
    generate.ccm
    downsample.ccm
)
