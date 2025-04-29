add_subdirectory(debug)

target_sources(${PROJECT_NAME} PRIVATE
  analysis.cc
  decompress.cc
  rearrange.cc
)
