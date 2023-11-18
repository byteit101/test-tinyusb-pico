#pragma once

#include "pico/stdio_usb.h"

#if !defined(LIB_TINYUSB_DEVICE)
#error "requires tinyusb"
#endif 

//#define TUSB_OPT_DEVICE_ENABLED 1
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE)

// Two cdc interfaces
#define CFG_TUD_CDC             (1)
#define CFG_TUD_CDC_RX_BUFSIZE  (256)
#define CFG_TUD_CDC_TX_BUFSIZE  (256)

// Mass Storage Class, for configuration FAT drive
#define CFG_TUD_MSC              (1)
// MSC Buffer size of Device Mass storage
#define CFG_TUD_MSC_EP_BUFSIZE   4096

// We use a vendor specific interface but with our own driver
#define CFG_TUD_VENDOR            (0)
