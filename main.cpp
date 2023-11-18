#include <stdio.h>
#include "tusb.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#if LIB_PICO_STDIO_UART
#error "Built in UART enabled!"
#endif

#if LIB_PICO_STDIO_SEMIHOSTING
#error "Built in USB enabled!"
#endif

#if LIB_PICO_STDIO_USB
#error "Built in USB enabled!"
#endif

void our_stdio_usb_init();

//https://github.com/hathach/tinyusb/blob/master/examples/device/msc_dual_lun/src/msc_disk_dual.c
int main()
{

	bi_decl(bi_program_description("TinyUSB chaos"));
	bi_decl(bi_program_name("TinyUSB Test build"));

	tusb_init();
	our_stdio_usb_init(); // "our" becase msc + midi + vendor reset (pico)

	
	while (!tud_cdc_connected()) {
        sleep_ms(1);
    }
	while (true)
	{
	puts("Welcome, printing and mscing");
	sleep_ms(5000);
	}

}



extern "C" {


void _fstat_r()
{
}
void _isatty_r()
{
}
void _close(){}
void _lseek() {
	// TODO: warn if these are called?
}


}

