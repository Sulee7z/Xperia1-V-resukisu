/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Protocol Buffer Serializer, for the types needed by IDD
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include "serializer.h"

enum {
  WIRE_TYPE_VARINT = 0,
  WIRE_TYPE_LENGTH_DELIMITED = 2
};

/* Warning: Can only handle field_nr 0 -> 15 */
static size_t write_tag(char* buffer_p, int type, int field_nr)
{
  *buffer_p = (field_nr) << 3 | type;
  return 1;
}

static size_t write_varint32(char* buffer_p, uint32_t value)
{
  size_t bytes = 0;
  if (value >= 0x80)
  {
    buffer_p[bytes++] = (char)value | 0x80;
    value >>= 7;
    if (value >= 0x80)
    {
      buffer_p[bytes++] = (char)value | 0x80;
      value >>= 7;
      if (value >= 0x80)
      {
        buffer_p[bytes++] = (char)value | 0x80;
        value >>= 7;
        if (value >= 0x80)
        {
          buffer_p[bytes++] = (char)value | 0x80;
          value >>= 7;
        }
      }
    }
  }
  buffer_p[bytes++] = (char)value;
  return bytes;
}

static size_t write_varint64(char* buffer, int64_t value)
{
  uint32_t hi_word = value >> 32;
  uint32_t lo_word = value;
  unsigned bytes;

  /* Optimization for small numbers. */
  if (hi_word == 0)
  {
    bytes = write_varint32(buffer, lo_word);
  }
  else
  {
    /* Over 32 bits. We can write the first 29 bits directly (takes four bytes)
     *  and we will use at least 5 bytes.
     */
    bytes = 5;
    buffer[0] = (char)(lo_word)       | 0x80;
    buffer[1] = (char)(lo_word >> 7)  | 0x80;
    buffer[2] = (char)(lo_word >> 14) | 0x80;
    buffer[3] = (char)(lo_word >> 21) | 0x80;

    if (hi_word < 8)
    {
      /* 35 bits can fit in 5 bytes */
      buffer[4] = (hi_word << 4) | (lo_word >> 28);
    }
    else
    {
      /* 35-64 bits used. */
      buffer[4] = ((hi_word & 7) << 4) | (lo_word >> 28) | 0x80;
      hi_word >>= 3;
      while (hi_word >= 0x80)
      {
        buffer[bytes++] = hi_word | 0x80;
        hi_word >>= 7;
      }
      buffer[bytes++] = hi_word;
    }
  }
  return bytes;
}

static inline size_t write_string_field(char* buffer_p, int tag, const char* data_p, size_t length)
{
  size_t off;
  off = write_tag(buffer_p, WIRE_TYPE_LENGTH_DELIMITED, tag);
  off += write_varint32(&buffer_p[off], length);
  if (data_p != NULL)
    memcpy(&buffer_p[off], data_p, length);
  off += length;
  return off;
}

static inline size_t write_bytes_field(char* buffer_p, int tag, const char* data_p, size_t length)
{
  return write_string_field(buffer_p, tag, data_p, length);
}

static size_t varint32_size(uint32_t value)
{
  if (value < (1 << 7))
  {
    return 1;
  }
  else if (value < (1 << 14))
  {
    return 2;
  }
  else if (value < (1 << 21))
  {
    return 3;
  }
  else if (value < (1 << 28))
  {
    return 4;
  }
  return 5;
}

static size_t varint64_size(int64_t value)
{
  uint32_t hi_word = value >> 32;

  if (hi_word == 0)
  {
    return varint32_size((uint32_t)value);
  }

  if (hi_word < (1 << 3))
  {
    return 5;
  }
  else if (hi_word < (1 << 10))
  {
    return 6;
  }
  else if (hi_word < (1 << 17))
  {
    return 7;
  }
  else if (hi_word < (1 << 24))
  {
    return 8;
  }
  else if (hi_word < (1U << 31))
  {
    return 9;
  }

  return 10;
}

size_t ser_sdata(char* buf, size_t buf_size, const char* package_p, const char* name_p,
                 int64_t timestamp, const char* value_p)
{
  size_t off;
  size_t package_size = package_p == NULL ? 0 : strlen(package_p);
  size_t name_size = name_p == NULL ? 0 : strlen(name_p);
  size_t value_size = value_p == NULL ? 0 : strlen(value_p);
  size_t sdata_size =
    1 + varint32_size(package_size) + package_size +
    1 + varint32_size(name_size) + name_size +
    1 + varint64_size(timestamp) +
    1 + varint32_size(value_size) + value_size;
  size_t total_size = 1 + varint32_size(sdata_size) + sdata_size;
  if (total_size > buf_size)
  {
    return 0;
  }

  /* repeated StaticData statics = 4; */
  off = write_tag(&buf[0], WIRE_TYPE_LENGTH_DELIMITED, 4);
  off += write_varint32(&buf[off], sdata_size);

  /*message StaticData {
   *   required string package = 1; */
  off += write_string_field(&buf[off], 1, package_p, package_size);
  /*   required string name = 2; */
  off += write_string_field(&buf[off], 2, name_p, name_size);
  /*  required string value = 3; */
  off += write_string_field(&buf[off], 3, value_p, value_size);

  /*  required int64 timestamp = 4; */
  off += write_tag(&buf[off], WIRE_TYPE_VARINT, 4);
  off += write_varint64(&buf[off], timestamp);

  assert(off == total_size);

  return off;

}

size_t ser_counter(char* buf, size_t buf_size, const char* package_p, const char* name_p,
                   int64_t from_timestamp, int64_t to_timestamp, int64_t value)
{
  size_t off;
  size_t package_size = package_p == NULL ? 0 : strlen(package_p);
  size_t name_size = name_p == NULL ? 0 : strlen(name_p);
  size_t counter_size =
    1 + varint32_size(package_size) + package_size +
    1 + varint32_size(name_size) + name_size +
    1 + varint64_size(from_timestamp) +
    1 + varint64_size(to_timestamp) +
    1 + varint64_size(value);
  size_t total_size = 1 + varint32_size(counter_size) + counter_size;
  if (total_size > buf_size)
  {
    return 0;
  }

  /* repeated Counter counters = 3; */
  off = write_tag(&buf[0], WIRE_TYPE_LENGTH_DELIMITED, 3);
  off += write_varint32(&buf[off], counter_size);
  /* message Counter {
   *   required string package = 1; */
  off += write_string_field(&buf[off], 1, package_p, package_size);
  /*   required string name = 2; */
  off += write_string_field(&buf[off], 2, name_p, name_size);
  /*   required int64 value = 3; */
  off += write_tag(&buf[off], WIRE_TYPE_VARINT, 3);
  off += write_varint64(&buf[off], value);
  /*   required int64 from_timestamp = 4; */
  off += write_tag(&buf[off], WIRE_TYPE_VARINT, 4);
  off += write_varint64(&buf[off], from_timestamp);
  /*   required int64 to_timestamp = 5; */
  off += write_tag(&buf[off], WIRE_TYPE_VARINT, 5);
  off += write_varint64(&buf[off], to_timestamp);

  assert(off == total_size);

  return off;
}

size_t ser_event_header(char* buf, size_t buf_size, const char* package_p, const char* name_p,
                        int64_t timestamp, size_t payload_size)
{
  size_t off;
  size_t name_size = name_p != NULL ? strlen(name_p) : 0;
  size_t package_size = package_p != NULL ? strlen(package_p) : 0;
  size_t entry_header = 1 + varint32_size(package_size) + package_size +
      1 + varint32_size(name_size) + name_size +
      1 + varint64_size(timestamp) + 1 + varint32_size(payload_size);
  size_t report_header = 1 + varint32_size(entry_header + payload_size);
  size_t total_size = report_header + entry_header;

  if (total_size > buf_size)
  {
    return 0;
  }

  /* repeated Entry entries = 2; */
  off = write_tag(&buf[0], WIRE_TYPE_LENGTH_DELIMITED, 2);
  off += write_varint32(&buf[off], entry_header + payload_size);
  /* message Entry {
   *   required string package = 1; */
  off += write_string_field(&buf[off], 1, package_p, package_size);
  /*   required string name = 2; */
  off += write_string_field(&buf[off], 2, name_p, name_size);
  /*   required int64 timestamp = 3; */
  off += write_tag(&buf[off], WIRE_TYPE_VARINT, 3);
  off += write_varint64(&buf[off], timestamp);
  /*   required bytes payload = 4; */
  off += write_tag(&buf[off], WIRE_TYPE_LENGTH_DELIMITED, 4);
  off += write_varint32(&buf[off], payload_size);

  assert(off == total_size);

  return off;
}

const char* ser_header(const char* prod_id_p,
                       const char* revision_p,
                       const char* variant_p,
                       const char* build_type_p,
                       const char* imei_p,
                       const char* config_checksum_p,
                       int32_t timezone,
                       size_t* size_p)
{
  size_t prod_id_size = strlen(prod_id_p);
  size_t revision_size = strlen(revision_p);
  size_t variant_size = strlen(variant_p);
  size_t imei_size = strlen(imei_p);
  size_t build_type_size = strlen(build_type_p);
  size_t config_checksum_size = strlen(config_checksum_p);
  size_t header_size =
    1 + varint32_size(prod_id_size) + prod_id_size +
    1 + varint32_size(revision_size) + revision_size +
    1 + varint32_size(variant_size) + variant_size +
    1 + varint32_size(imei_size) + imei_size +
    1 + varint32_size(build_type_size) + build_type_size +
    1 + varint32_size(config_checksum_size) + config_checksum_size +
    1 + varint32_size(timezone) +
    1 + 1;
  size_t total_size =
    1 + varint32_size(header_size) + header_size;
  char* buf = malloc(total_size);
  size_t off = 0;

  if (buf != NULL)
  {
    /*  required Header header = 1; */
    off += write_tag(&buf[off], WIRE_TYPE_LENGTH_DELIMITED, 1);
    off += write_varint32(&buf[off], header_size);
    /* message Header {
     *  required string product_id = 1; */
    off += write_string_field(&buf[off], 1, prod_id_p, prod_id_size);
    /*  required string software_version = 2; */
    off += write_string_field(&buf[off], 2, revision_p, revision_size);
    /*  required string software_variant = 3; */
    off += write_string_field(&buf[off], 3, variant_p, variant_size);
    /*  required string imei = 4; */
    off += write_string_field(&buf[off], 4, imei_p, imei_size);
    /*  optional string build_type = 5; */
    off += write_string_field(&buf[off], 5, build_type_p, build_type_size);
    /*  optional Family family = 6 [default = OSE]; */
    /* enum Family { OSE = 0; ANDROID = 1; } */
    off += write_tag(&buf[off], WIRE_TYPE_VARINT, 6);
    off += write_varint32(&buf[off], 1);
    /*  optional string config_checksum = 7; */
    off += write_string_field(&buf[off], 7, config_checksum_p, config_checksum_size);
    /*  optional sint32 timezone = 9; */
    off += write_tag(&buf[off], WIRE_TYPE_VARINT, 9);
    off += write_varint32(&buf[off], timezone);
    assert(off == total_size);
  }
  *size_p = total_size;
  return buf;
}

size_t serialize_di_raw_incoming(char* buffer_p, const char* data_p, size_t size)
{
  size_t off = 4;
  // 4 bytes length first, big endian - but we wait with it.
  off += write_tag(&buffer_p[off], WIRE_TYPE_LENGTH_DELIMITED, 1);
  off += write_varint32(&buffer_p[off], size);
  memcpy(&buffer_p[off], data_p, size);
  off += size;
  *(uint32_t*)&buffer_p[0] = htonl(off - 4);
  return off;
}

/* Writes the necessary fields - but not the actual encrypted payload. */
size_t serialize_encrypted_data(char* buffer_p,
                                const char* key_p,
                                size_t key_size,
                                const char* iv_p,
                                size_t iv_size,
                                size_t encrypted_len)
{
  size_t off = 0;
  //  optional bytes encryption_key = 6;
  off += write_bytes_field(&buffer_p[off], 6, key_p, key_size / 8);
  //  optional bytes encryption_iv = 7;
  off += write_bytes_field(&buffer_p[off], 7, iv_p, iv_size);
  //  optional bytes encrypted_compressed_data = 8;
  off += write_tag(&buffer_p[off], WIRE_TYPE_LENGTH_DELIMITED, 8);
  off += write_varint32(&buffer_p[off], encrypted_len);
  return off;
}

size_t serialize_string(char* buf, size_t buf_size, const char* str_p, size_t str_len, int tag_nr)
{
  size_t off;
  size_t total_size = 1 + varint32_size(str_len) + str_len;

  if (total_size > buf_size)
  {
    return 0;
  }
  off = write_string_field(&buf[0], tag_nr, str_p, str_len);
  assert(off == total_size);
  return total_size;
}
