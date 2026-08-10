/*
 * Copyright 2014 Sony Corporation
 */

/**
 * @brief Handles file2probe for files other than Android logs
 *
 * @author Leif Martensson (leif.martensson@sonyericsson.com)
 */

#ifndef CONFIG_H_INCLUSION_GUARD
#define CONFIG_H_INCLUSION_GUARD

#include  "../api/idd_api.h"
#include  "util.h"
#include  "rules.h"

/**
 * Read the IDD configuration.
 *
 * @param idd_p The open IDD instance to read from.
 */
void config_read(IDD_t* idd_p);

/**
 * Add Android log probes fom IDD configuration. These are typically IDD plugins that need its own
 * filter on the Android logs. Given in the IDD configuration param log_probe.
 *
 * @param probe The list probe list to add new Android log probes to.
 */
void config_probes_add(struct Probe** probe);

#endif //CONFIG_H_INCLUSION_GUARD
