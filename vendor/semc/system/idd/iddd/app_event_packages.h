/*
 * Copyright 2018 Sony Corporation
 */

#include <stddef.h>

/**
 * Initializing the linked list structure by reading REGISTERED_APP_PACKAGES_FNAME file.
 */
void app_event_packages_init(void);

/**
 * Destroy the global list CELL.
 */
void app_event_packages_shutdown(void);

/**
 * Request sent from apps to register their package to use app event.
 * return 0 on failure and 1 on success.
 */
int app_event_packages_register(const char *package_p, const char *class_p, int status);

/**
 * Fetches all the registered packages from REGISTERED_APP_PACKAGES_FNAME file.
 * Check the status of the respective package and if status is changed from
 * Disabled -> Enabled OR Enabled -> Disabled update the REGISTERED_APP_PACKAGES_FNAME file
 * accordingly. Build the "updated" char array with only Disabled -> Enabled packages.
 */
void app_event_packages_update(char* updated, size_t size);
