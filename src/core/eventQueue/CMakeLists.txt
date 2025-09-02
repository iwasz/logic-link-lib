target_sources(${PROJECT_NAME}
  # PRIVATE
  #   analysis.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    eventQueue.ccm
    event.ccm
    alarm.ccm
)
