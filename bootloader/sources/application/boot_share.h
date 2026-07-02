#ifndef BOOT_SHARE_H
#define BOOT_SHARE_H

#include <stdbool.h>

#include "app_data.h"

void boot_share_get(sys_boot_t *boot);
bool boot_share_set(const sys_boot_t *boot);
bool boot_share_erased_or_invalid(const sys_boot_t *boot);

#endif
