target_sources(${PROJECT_NAME}
  PRIVATE
   util.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    polyPoints.ccm
    util.ccm
    thread.ccm
)
