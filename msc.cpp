#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <cstring>

// based on https://github.com/hathach/tinyusb/blob/master/examples/device/cdc_msc/src/msc_disk.c



#include "hardware/flash.h"
#include "hardware/regs/addressmap.h"

namespace FileSystem
{

static_assert(PICO_FLASH_SIZE_BYTES == 2*1024*1024);

constexpr uint32_t Size = (1*1024*1024); // 12MB disk, 4MB code
constexpr uint32_t BlockSize = 4096;
constexpr uint32_t Count = Size / BlockSize;

static_assert(FLASH_SECTOR_SIZE == BlockSize);

constexpr uint32_t Offset = (PICO_FLASH_SIZE_BYTES - Size);

constexpr uint32_t MemoryOffset = XIP_NOCACHE_NOALLOC_BASE + FileSystem::Offset;

	int32_t write(uint32_t block, /*uint32_t offset, */const void *buffer, uint32_t count);
	int32_t read(uint32_t block, /*uint32_t offset, */void *buffer, uint32_t count);


};


// whether host does safe-eject
static bool ejected = false;

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
	const char vid[] = "TUSB";
	const char pid[] = "Test Flash";
	const char rev[] = "1.0";

	memcpy(vendor_id  , vid, strlen(vid));
	memcpy(product_id , pid, strlen(pid));
	memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    if (ejected) {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
        return false;
    }
	return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
	*block_size = FileSystem::BlockSize;
	*block_count = FileSystem::Count;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    if (load_eject) {
        if (start) {
            // load disk storage
            ejected = false;
        } else {
            // unload disk storage
            ejected = true;
			//  log_printf("Got Eject\n");
        }
    }
    return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
  	if (lba >= FileSystem::Count)
	{
		//printf("BRig count! %ld\n", lba);
		return -1;
	}
	if (offset != 0)
	{
		//printf("NRonzero coffset! %ld\n", offset);
		return -1;
	}
	return FileSystem::read(lba, buffer, bufsize / FileSystem::BlockSize);
}
namespace FileSystem
{
	int32_t read(uint32_t block, /*uint32_t offset, */void *buffer, uint32_t count)
	{
		// read flash via XIP mapped space, without caching
		std::memcpy(buffer, reinterpret_cast<const void *>(MemoryOffset + block * BlockSize), count * BlockSize);
		return count * BlockSize;
	}
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
  	if (lba >= FileSystem::Count)
	{
		//printf("BiWg count! %ld\n", lba);
		return -1;
	}
	if (offset != 0)
	{

		//printf("NWonzero coffset! %ld\n", offset);
		return -1;
	}
	return FileSystem::write(lba, buffer, bufsize / FileSystem::BlockSize);
}
namespace FileSystem
{
	int32_t write(uint32_t block, /*uint32_t offset, */const void *buffer, uint32_t count)
	{
		uint32_t off = Offset + block * BlockSize;
		uint32_t size = count * BlockSize;
        // Flash erase/program must run in an atomic section because the XIP bit gets disabled.
		uint32_t ints = save_and_disable_interrupts();
		flash_range_erase(off, size);
		flash_range_program(off, static_cast<const uint8_t*>(buffer), size);
		restore_interrupts(ints);
		return size;
	}
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {
	// printf("Got cmd: [%d]\n", (uint32_t)scsi_cmd[0]);
    int32_t resplen = 0;
    switch (scsi_cmd[0]) {
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            // Sync the logical unit if needed.
            break;

        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            // negative means error -> tinyusb could stall and/or response with failed status
            resplen = -1;
            break;
    }
    return resplen;
}


/*16010073.332147] usb-storage 8-2.1.2.1.2:1.5: USB Mass Storage device detected
[16010073.332480] scsi host9: usb-storage 8-2.1.2.1.2:1.5
[16010073.334590] cdc_acm 8-2.1.2.1.2:1.6: ttyACM1: USB ACM device
[16010074.364745] scsi host9: scsi scan: INQUIRY result too short (5), using 36

[16010074.365312] sd 9:0:0:0: Attached scsi generic sg8 type 0
[16010074.369795] sd 9:0:0:0: [sdh] 3072 4096-byte logical blocks: (12.6 MB/12.0 MiB)
[16010074.373961] sd 9:0:0:0: [sdh] Write Protect is off
[16010074.373965] sd 9:0:0:0: [sdh] Mode Sense: 03 00 00 00
[16010074.377847] sd 9:0:0:0: [sdh] No Caching mode page found
[16010074.377851] sd 9:0:0:0: [sdh] Assuming drive cache: write through
[16010074.481462] sd 9:0:0:0: [sdh] Attached SCSI removable disk

*/
