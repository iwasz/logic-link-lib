
target_sources(${PROJECT_NAME}
  PRIVATE
    analysis.cc
    decompress.cc
    rearrange.cc
    generate.cc
    downsample.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    processing.ccm
    generate.ccm
    downsample.ccm
    polyPoints.ccm
)

# TODO doeasn't work
# set_property (DIRECTORY "src" PROPERTY COMPILE_OPTIONS "-O3")
# set_property (SOURCE DIRECTORY "src/processing/" PROPERTY COMPILE_OPTIONS "-O3")
# set_directory_properties(PROPERTY COMPILE_OPTIONS "-O3")
