target_sources(${PROJECT_NAME}
  # PRIVATE
  #   analysis.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    core.ccm
    constants.ccm
    exception.ccm
)

add_subdirectory(eventQueue)