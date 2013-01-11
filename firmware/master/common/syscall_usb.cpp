#include <sifteo/abi.h>
#include "svmmemory.h"
#include "svmruntime.h"
#include "usb/usbdevice.h"

extern "C" {
  void _SYS_usb_write(const uint8_t * data, unsigned len) {
#ifndef SIFTEO_SIMULATOR
    UsbDevice::write(data, len);
#endif
  }
}
