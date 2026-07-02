#ifndef BOOT_DFU_H
#define BOOT_DFU_H

#include <stdbool.h>

void boot_dfu_enter(void);
void boot_dfu_task(void);
bool boot_dfu_manifest_done(void);
bool boot_dfu_timed_out(void);

#endif
