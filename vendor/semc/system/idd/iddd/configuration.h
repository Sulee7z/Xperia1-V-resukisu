/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Configuration Update Handling
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef CONFIGURATION_H_INCLUSION_GUARD
#define CONFIGURATION_H_INCLUSION_GUARD

#define CONFIG_REQ_ERROR (-1)

/* Writes a new configuration, into file
 * 'input/config'
 */
void write_config(const char* config_p, size_t config_size);

/* Loads a new configuration, if available, from
 * 'input/config'.
 *
 * @param force If 1 then load a new configuration regardless of
 *              user consent mismatching.
 *
 * @param force If 1 then load a new configuration regardless of
 *              user consent mismatching.
 * @retval 1 New configuration is verified ok
 * @retval 0 Otherwise
 */
int load_new_configuration(int force);

/* Initializes the configuration module and
 * loads persistent state from disk.
 */
void config_init(void);

int get_config_file_path(const char* package_p, const char* name_p, char* out_path_p, int out_path_size);

/* Opens the configuration file for the specific package and
 * name for reading.
 *
 * Returns NULL on failure.
 */
FILE* config_open_file(const char* package_p, const char* name_p);

/* Deinitializes the configuration module.
 */
void config_shutdown(void);

/* Runs additional actions that IDDD has been asked
 * to perform by a configuration part.
 */
void run_configuration_actions(void);

/* load iddd.conf from /vendor/etc/ to input folder and rename as config
 * This will be treated as the input of load_new_configuration.
 *
 * @param force If 1 then load iddd.conf in disregard of exisiting input configuration
 */
void load_default_iddd_config(int force);

/**
 * Make a ConfigRequest protobuf (See configuration.proto).
 *
 * @param buf_p Where the protobuf is placed.
 * @param max_size Max length of the protobuf.
 *
 * @return Length of the created protobuf.
 */
int config_request_get(char* buf_p, int max_size);

/**
 * Get a file descriptor that will receive notification when a new
 * configuration is loaded.
 *
 * Returns -1 on failure
 */
int config_change_notify_get(void);

/**
 * Save the configuration for user consent with anonymous.
 *
 */
void save_anonymous_config(void);

/**
 * Load the configuration for user consent with anonymous.
 *
 */
int load_anonymous_config(void);
#endif
