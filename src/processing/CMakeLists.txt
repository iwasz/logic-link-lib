target_sources(${PROJECT_NAME}
  PRIVATE
    analysis.cc
    decompress.cc
    rearrange.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    processing.ccm
)
