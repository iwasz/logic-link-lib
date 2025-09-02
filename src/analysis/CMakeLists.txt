add_subdirectory(debug)
add_subdirectory(lowLevel)

target_sources(${PROJECT_NAME}
  PUBLIC FILE_SET CXX_MODULES FILES
    analyzer.ccm
    analysis.ccm
)
