/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief C API for accessing the IDD Daemon public api
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef IDD_PUBLIC_API_H_INCLUSION_GUARD
#define IDD_PUBLIC_API_H_INCLUSION_GUARD

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IDD_t IDD_t;

#define IDD_NO_CONFIG_AVAILABLE -99942

/* Creates and IDD instance, bound to either the default sockets
 * or by specifying the path where IDD is running.
 *
 * On error, NULL is returned.
 */
IDD_t* idd_create(void);
IDD_t* idd_create_from_path(const char* base_dir_p);

/* Destroys the IDD instance and free's all resources */
void idd_destroy(IDD_t** idd_pp);

/* Sets the counter to the value specified. Returns 0 if successful, < 0 if not. */
int idd_set_counter(IDD_t* idd, const char* package_p, const char* name_p, int64_t value);

/* Increases the counter with the delta value specified. Returns 0 if successful, < 0 if not. */
int idd_inc_counter(IDD_t* idd, const char* package_p, const char* name_p, int64_t delta);

/* Sets the static data to the value specified. Returns 0 if successful, < 0 if not. */
int idd_set_sdata(IDD_t* idd, const char* package_p, const char* name_p, const char* value_p);

/* High-level event dispatching. Returns 0 if successful, < 0 if not. */
int idd_add_event(IDD_t* idd, const char* package_p, const char* name_p, const char* payload_p, size_t payload_size);

/* High-level app event dispatching. Returns 0 if successful, < 0 if not. */
int idd_add_app_event(IDD_t* idd, const char* app_package_p,
                      const char* app_version_name_p, int app_version_code,
                      const char* timestamp_p,
                      int payload_type, const char* payload_p, size_t payload_size);

/* Registers package name and broadcast receiver class name to enable collection of app event.
 * Internally checks whether respective package names are enabled in configuration.
 *
 * Accepts broadcast receiver class name as parameter.
 *
 * Returns 1 if the package name is already enabled, 0 if disabled and < 0 on errors.
 */
int idd_register_to_use_add_app_event(IDD_t* idd, const char* app_package_p,
                                      const char* class_name_p);

/* High-level addressed event dispatching. The event is sent only to the given plugin_id.
 * plugin_id shall match the enum PluginId in the iddd.proto file. It must be between 0 and 9.
 * Returns 0 if successful, < 0 if not. */
int idd_add_plugin_event(IDD_t* idd, int plugin_id, const char* package_p, const char* name_p,
                         const char* payload_p, size_t payload_size);

/* Indicates if IDD is active.
 *
 * Returns > 0 if IDD is active, 0 if it's not, and < 0 on errors.
 *
 * Note that even though IDD is inactive, the API can be used and
 * will either return an error or will result in a no-op.
 */
int idd_is_active(IDD_t* idd);

/* Indicates if a probe is active.
 *
 * Returns > 0 if the probe is active, 0 if it's not, and < 0 on errors.
 */
int idd_is_probe_active(IDD_t* idd, const char* package_p, const char* name_p);

/* Get configuration data
 *
 * Any client may ask for config but most packages don't support configuration.
 *
 * Note: The returned file descriptor must be closed by the client.
 *
 * Returns a file descriptor to the configuration, in a package specific format.
 * Returns IDD_NO_CONFIG_AVAILABLE if no configuration is available, < 0 on errors.
 */
int idd_get_config(IDD_t* idd, const char* package_p, const char* name_p);

/* Get a file descriptor that will receive notification when config change.
 *
 * Returns a file descriptor that can be passed to idd_config_wait()
 * it can also be used for select(), poll() or epoll().
 * Returns <0 on errors.
 */
int idd_get_config_notify(IDD_t* idd);

/* Wait for config to change and make sure fd will block on read.
 *
 * Returns <0 on errors.
 */
int idd_config_wait(IDD_t* idd, int fd);

/* Sets an non-existed group id into report header. Returns 0 if successful, < 0 if not. */
int idd_set_group_id(IDD_t* idd, const char* group_id_p);

/* Deletes an existed group id from report header. Returns 0 if successful, < 0 if not. */
int idd_delete_group_id(IDD_t* idd, const char* del_group_id_p);

/* Replaces an existed group id to a specified one into report header. Returns 0 if successful, < 0 if not. */
int idd_replace_group_id(IDD_t* idd, const char* to_group_id_p, const char* from_group_id_p);

/* Returns the maxsize of the IDD payload. */
int idd_get_max_payload(void);

/* Shall data washing of personal data be done?.
 *
 * Non prototypes shall wash personal data. However the IDD configuration flag
 * iddd.Settings.wash_data can be used to disable data washing.
 *
 * Returns > 0 if data washing shall be done, 0 if no washing shall be done, and < 0 on errors.
 */
int idd_do_washing(IDD_t* idd);

#ifdef __cplusplus
}
#endif

#endif
