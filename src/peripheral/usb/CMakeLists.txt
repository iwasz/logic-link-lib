target_sources(${PROJECT_NAME}
  PRIVATE
    usbFactory.cc
    logicLinkDevice.cc
    usbAsyncInput.cc
    usbDevice.cc

  PUBLIC FILE_SET CXX_MODULES FILES
    usbFactory.ccm
    logicLinkDevice.ccm
    usbAsyncInput.ccm
    usbDevice.ccm
    usbInput.ccm
)
