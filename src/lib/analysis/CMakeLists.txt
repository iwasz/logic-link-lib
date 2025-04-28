add_subdirectory(debug)

target_sources(link PRIVATE
  analysis.cc
  decompress.cc
  rearrange.cc
)
