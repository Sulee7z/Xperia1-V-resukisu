/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Protocol Buffer Serializer, for the types needed by IDD
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <stdint.h>

size_t ser_event_header(char* buf, size_t buf_size, const char* package_p, const char* name_p, int64_t timestamp, size_t payload_size);
size_t ser_counter(char* buf, size_t buf_size, const char* package_p, const char* name_p, int64_t from_timestamp, int64_t to_timestamp, int64_t value);
size_t ser_sdata(char* buf, size_t buf_size, const char* package_p, const char* name_p, int64_t timestamp, const char* value_p);

const char* ser_header(const char* prod_id_p,
                       const char* revision_p,
                       const char* variant_p,
                       const char* build_type_p,
                       const char* imei_p,
                       const char* config_checksum_p,
                       int32_t timezone,
                       size_t* size_p);

size_t serialize_di_raw_incoming(char* buffer_p, const char* data_p, size_t size);

/* NOTE: key_size should be in bits */
size_t serialize_encrypted_data(char* buffer,
                                const char* key_p,
                                size_t key_size,
                                const char* iv_p,
                                size_t iv_size,
                                size_t encrypted_len);

size_t serialize_string(char* buf, size_t buf_size, const char* str_p, size_t str_len, int tag_nr);
