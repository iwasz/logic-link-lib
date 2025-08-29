target_sources(${PROJECT_NAME}
  PRIVATE
    factory.cc
    logicLinkDevice.cc
    usbAsyncInput.cc
    usbDevice.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    factory.ccm
    logicLinkDevice.ccm
    usbAsyncInput.ccm
    usbDevice.ccm
    usbInput.ccm
)
