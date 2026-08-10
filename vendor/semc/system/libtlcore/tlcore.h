/*
 * Copyright (C) 2014 Sony Mobile Communications Inc.
 * All rights, including trade secret rights, reserved.
 */
#ifndef __TLCORE_H__
#define __TLCORE_H__

#include <stdint.h>

#define TLCORE_MAX_DUMPER_VERSION_SZ 32
#define TLCORE_MAX_PLATFORM_NAME_SZ 32
#define TLCORE_MAX_PRODUCT_NAME_SZ 32
#define TLCORE_MAX_BUILD_ID_SZ 64
#define TLCORE_MAX_BUILD_VARIANT_SZ 16
#define TLCORE_MAX_IMEI_SZ 32
#define TLCORE_MAX_SERIAL_NO_SZ 32
#define TLCORE_MAX_SIGNATURE_SZ 20
#define TLCORE_MAX_CRASH_TIME_SZ 32
#define TLCORE_MAX_DUMPER_NAME_SZ 32
#define TLCORE_MAX_NAME_SZ_V6 256
#define TLCORE_MAX_NAME_SZ 32
#define TLCORE_MAX_CORE_NAME_SZ 32

#ifdef __cplusplus
extern "C" {
#endif
/**
 * enum tlcore_mode_type - It defines the mode in which application wants
 *			to use the library.
 *
 * TLCORE_READONLY :
 *	Select if you want to read a tlcore.
 *
 * TLCORE_READWRITE :
 *	Select if you want to both create a new tlcore.
 *
 * TLCORE_UPDATE :
 *	Select if an existing tlcore needs to be updated. This is mostly used
 *	for reserved/uncommitted sections where it is needed to update the
 *	sections at a later time.
 */
enum tlcore_mode_type {
	TLCORE_READONLY = 1,
	TLCORE_READWRITE,
	TLCORE_UPDATE
};

/**
 * @init_tlcore		- Initialize tlcore object to load tlcore file
 * @mode		- mode in which library will be used. Must be one of
 *			enum tlcore_mode_type.
 * @path[IN]		- tlcore path which should be read.
 * @retval		- struct tlcore * for success, NULL if error.
 */
struct tlcore *init_tlcore(const char *path, uint32_t mode);

/**
 * @finish_tlcore	- Release all resources used by library & signs tlcore.
 * @tlcore[IN]		- tlcore that needs to be finished.
 * @retval		- 0 for success. Others for error.
 */
int finish_tlcore(struct tlcore *tlcore);

/**
 * enum tlcore_dump_type - It defines the type of dump. Useful while reading
 *			the dump so that it is helpful in interpretting
 *			the format.
 *
 * TLCORE_DUMP_TYPE_SYSTEM :
 *	Select if complete system dump is being dumped to tlcore
 *
 * TLCORE_DUMP_TYPE_SSR :
 *	Select if a subsystem system dump is being dumped to tlcore
 *
 * TLCORE_DUMP_TYPE_NATIVE :
 *	Select if some native crash is being dumped to tlcore
 *
 * TLCORE_DUMP_TYPE_JAVA :
 *	Select if a java crash/anr/etc.. are being dumped to tlcore
 *
 * TLCORE_DUMP_TYPE_POST :
 *	Select if system dump fails and failure reason is dumped to
 *      partial tlcore by postdumper
 */
enum tlcore_dump_type {
	TLCORE_DUMP_TYPE_SYSTEM = 1,
	TLCORE_DUMP_TYPE_SSR,
	TLCORE_DUMP_TYPE_NATIVE,
	TLCORE_DUMP_TYPE_JAVA,
	TLCORE_DUMP_TYPE_POST,
	TLCORE_DUMP_TYPE_MAX
};

/**
 * enum tlcore_arch_type - This field defines the type of architecture on which
 *		crash got triggered.
 *
 * TLCORE_ARCH_TYPE_X86_32 - select this if system is 32-bit Intel(x86).
 *
 * TLCORE_ARCH_TYPE_X86_64 - select this if system is 64-bit Intel(x86).
 *
 * TLCORE_ARCH_TYPE_ARM_32 - select this if system is 32-bit ARM.
 *
 * TLCORE_ARCH_TYPE_ARM_64 - select this if system is 64-bit ARM.
 */
enum tlcore_arch_type {
	TLCORE_ARCH_TYPE_X86_32 = 1,
	TLCORE_ARCH_TYPE_X86_64,
	TLCORE_ARCH_TYPE_ARM_32,
	TLCORE_ARCH_TYPE_ARM_64,
	TLCORE_ARCH_TYPE_MAX
};

/**
 * enum tlcore_device_type - It defines the device type that the tlcore
 *			corresponds.
 *
 * TLCORE_DEVICE_TYPE_PHONE :
 *	Implies that the dump corresponds to a phone.
 *
 * TLCORE_DEVICE_TYPE_ACC :
 *	Implies that the dump corresponds to other accessories Ex: smart watch.
 */
enum tlcore_device_type {
	TLCORE_DEVICE_TYPE_PHONE = 1,
	TLCORE_DEVICE_TYPE_ACC,
	TLCORE_DEVICE_TYPE_MAX
};

/**
 * TLCORE_SECTION_TYPE_*** - It is used to define the section type in section
 *		header It defines the type of nature of section data. That is,
 *		Binary/Text
 *
 * TLCORE_SECTION_TYPE_TEXT :
 *	SECTION_TYPE_TEXT is set if the section data is of text in nature.
 *	Example: All logs, cmdline info etc..
 *
 * TLCORE_SECTION_TYPE_BIN :
 *	SECTION_TYPE_BINARY is set while dumping any bindary data.
 */

#define TLCORE_SECTION_TYPE_TEXT (1 << 0)
#define TLCORE_SECTION_TYPE_BIN (1 << 1)

/*
 * TLCORE_SECTION_ATTRIBUTE_*** - Used to specify the owners who would need
 *				these sections.
 *
 * TLCORE_SECTION_ATTRIBUTE_VENDOR :
 *	This is set if the section is specific to Vendor(QMC, Intel..)
 *
 * TLCORE_SECTION_ATTRIBUTE_REPORT :
 *	This is set if section needs to be reported by module(crashmonitor etc)
 */

#define TLCORE_SECTION_ATTRIBUTE_VENDOR (1 << 0)
#define TLCORE_SECTION_ATTRIBUTE_REPORT (1 << 1)

/*
 * enum tlcore_scn_data_type - It is used to define the data type in section
 *			header. It specifies the type(Raw/Compressed) of data
 *			stored in the section.
 *
 * TLCORE_DATA_TYPE_INCOMPLETE :
 *	Default value of a section when dumping is started. This value is
 *	modified once the dumping of a section is completed with appropriate
 *	value that the tlcore creator passes. User should never set it to
 *	DATA_TYPE_INCOMPLETE. But when you are reading a section and get the
 *	the value as DATA_TYPE_INCOMPLETE, it means that this section is not
 *	dumped fully and was exited in middle.
 *
 * TLCORE_DATA_TYPE_RAW :
 *	This needs to be set for raw uncompressed data.
 *	For ex: if you want to dump logs, they can be dumped directly without
 *	any compression.
 *
 * TLCORE_DATA_TYPE_DUMMY :
 *	This needs to be set if you are dumnping a section that you know
 *	is invalid. For ex, when you are dumping the complete DDR memory, a
 *	few portion of it is used for the dumper to execute its job. So we know
 *	we are overwriting some portion. In these cases setting these sections
 *	to dummy is apt becausethe tlcore library fills this with 0's there by
 *	avoiding any false implications while debugging.
 *
 * TLCORE_DATA_TYPE_ZLIB :
 *	This needs to be set if the compression used for the data in this
 *	particular section is ZLIB and a ZLIB header is written on top of the.
 *	compressed data.
 *
 * TLCORE_DATA_TYE_ZLIB_RAW :
 *	This needs to be set if the compression used for the data in this
 *	particular section is ZLIB and no ZLIB header was written on top of
 *	compressed data.
 *
 * TLCORE_DATA_TYPE_SNAPPY :
 *	This needs to be set if the compression used for the data in this
 *	particular section is SNAPPY.
 *
 * TLCORE_DATA_TYPE_RESERVED :
 *	Default Value of section when being reserved for further use.
 *	This value is to notify that any section marked with this flag,
 *	need not be extracted if size of this section is zero after tlcore
 *	creation.
 */
enum tlcore_scn_data_type {
	TLCORE_DATA_TYPE_INCOMPLETE,
	TLCORE_DATA_TYPE_RAW,
	TLCORE_DATA_TYPE_DUMMY,
	TLCORE_DATA_TYPE_ZLIB,
	TLCORE_DATA_TYPE_ZLIB_RAW,
	TLCORE_DATA_TYPE_SNAPPY,
	TLCORE_DATA_TYPE_RESERVED,
	TLCORE_DATA_TYPE_MAX
};

/**
 * enum tlcore_scn_data_fmt -	It is used to define data formt in section
 *			header. It specifies the format of data written into the
 *			section(Segmented or straightly written).
 *
 * TLCORE_DATA_FORMAT_STRAIGHT :
 *	This needs to be set when raw data is written sequentially into tlcore.
 *	For Example, we can write CPU registers starightly with out breaking
 *	them into segments and writing individually.
 *
 * TLCORE_DATA_FORMAT_SEGMENT :
 *	This needs to be set when raw data is broken into segments of smaller
 *	size, compressed and then written into tlcore. Mostly when you are
 *	dumping large sections of DDR/some memory, this is the standatd way
 *	that needs to be followed.
 *
 */
enum tlcore_scn_data_fmt {
	TLCORE_DATA_FORMAT_STRAIGHT = 0,
	TLCORE_DATA_FORMAT_SEGMENT,
	TLCORE_DATA_FORMAT_MAX
};

/**
 * struct tlcore_header	- Defines the tlcore header.
 *
 * @get_version :
 *	get major and minor version of tlcore file.
 *	@major[OUT]	-	 Major version of tlcore format.
 *	@minor[OUT]	-	 Minor version of tlcore format.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_platform_name :
 *	Get the name of the platform from tlcore file.
 *	@platform_name[OUT] -	Name of the platform.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_PLATFORM_NAME_SZ bytes are copied.
 *
 * @set_platform_name :
 *	Set the name of the platform in tlcore file.
 *	@platform_name[IN] -	Name of the platform.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_product_name :
 *	Get the name of the product from tlcore file.
 *	@product_name[OUT] -	Name of the product.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_PRODUCT_NAME_SZ bytes are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_product_name :
 *	Set the name of the product in tlcore.
 *	@product_name[IN] -	Name of the product.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_build_id :
 *	Get the build id from tlcore file.
 *	@build_id[OUT] -	The build id to which the tlcore belongs to.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_BUILD_ID_SZ bytes are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_build_id :
 *	Set the build id in tlcore file.
 *	@build_id[IN] -		  The build id to which the tlcore belongs to.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_build_variant :
 *	Get the build variant from tlcore file.
 *	@build_variant[OUT] -	The build variant to which the tlcore belongs.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_BUILD_VARIANT_SZ bytes are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_build_variant :
 *	Set the build variant in tlcore file.
 *	@build_variant[IN] -	The build variant to which the tlcore belongs.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_imei :
 *	Get the imei number from tlcore file.
 *	@imei[OUT] -	The imei number of the phone to which the tlcore
 *				belongs.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_IMEI_SZ bytes are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_imei :
 *	Set the imei number in tlcore file.
 *	@imei[IN]	-	The imei number of the phone to which the tlcore
 *				belongs.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_serial_number :
 *	Get the serial number from tlcore file.
 *	@serial_number[OUT] -	The serial number of the phone to which the
 *				tlcore belongs.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_SERIAL_NO_SZ bytes are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_serial_number :
 *	Set the serial number in tlcore file.
 *	@serial_number[IN] -	The serial number of the phone to which the
 *				tlcore belongs.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_dump_result :
 *	Get the dump result from tlcore file. Indicates if dumping crash into
 *	tlcore was completed succesfully or not.
 *	@dump_result[OUT] -	The dump result. It suggests whether dumping
 *				was success or failure.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_dump_result :
 *	Set the dump result in tlcore file. Indicates if dumping crash into
 *	tlcore was completed succesfully or not.
 *	@dump_result[IN] -	The dump result. Dumper application can set
 *				anything here to suggest success or failure in
 *				dumping
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_scn_count :
 *	Get the total sections count from tlcore file.
 *	@scn_count[OUT] -	Total number of sections in tlcore.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_signature :
 *	Get the signature from tlcore file.
 *	@signature[OUT] -	The signature of tlcore file.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_SIGNATURE_SZ bytes are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_crash_time :
 *	Get the crash time from tlcore file.
 *	@crash_time[OUT] -	The crash time that the dump belongs to.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_CRASH_TIME_SZ bytes are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_crash_time :
 *	Set the crash time in tlcore file.
 *	@crash_time[IN] -	The crash time that the dump belongs to.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_dumper_name :
 *	Get the dumper name from tlcore file.
 *	@dumper_name[OUT] -	Name of the dumper that created tlcore.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_DUMPER_NAME_SZ bytes are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_dumper_name :
 *	Set the dumper name in tlcore file.
 *	@dumper_name[IN] -	Name of the dumper that created tlcore.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_dumper_version :
 *	Get the dumper version from tlcore file.
 *	@dumper_version[OUT] -	The dumper version that created the tlcore.
 *	@size[IN] -		size bytes to be copied. A maximum of
 *				TLCORE_MAX_DUMPER_VERSION_SZ bytes are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_dumper_version :
 *	Set the dumper version in tlcore file.
 *	@dumper_version[IN] -	The dumper version that created tlcore.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_dump_type :
 *	Get the type of dump from tlcore file.
 *	@dump_type[OUT] -	Specifies the type of dump.The value read
 *				in dump_type will be one among the fields of
 *				enum tlcore_dump_type
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_dump_type :
 *	Set the type of dump in tlcore file.
 *	@dump_type[IN] -	Specifies the type of dump. The value being
 *				passed in dump_type should be one among the
 *				fields of enum tlcore_dump_type
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_device_type :
 *	Get the type of device on which crash occurred from tlcore file.
 *	@device_type[OUT] -	Specifies the type of device. The value read
 *				in device_type will be one among the fields of
 *				enum tlcore_device_type
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_device_type :
 *	Set the type of device on which crash occurred into tlcore file.
 *	@device_type[IN] -	Specifies the type of device. The value being
 *				passed in device_type should be one among the
 *				fields of enum tlcore_device_type
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_arch_type :
 *	Get the type of arch on which crash occurred from tlcore file.
 *	@arch_type[OUT] -	Specifies the type of device. The value read
 *				in arch_type will be one among the fields of
 *				enum tlcore_arch_type
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_arch_type :
 *	Set the type of arch on which crash occurred into tlcore file.
 *	@arch_type[IN] -	Specifies the type of device. The value being
 *				passed in arch_type should be one among the
 *				fields of enum tlcore_arch_type
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 */
struct tlcore_header {
	int (*get_version) (uint16_t *major, uint16_t *minor);
	int (*get_platform_name) (char *platform_name, uint32_t size);
	int (*set_platform_name) (const char *platform_name);
	int (*get_product_name) (char *product_name, uint32_t size);
	int (*set_product_name) (const char *product_name);
	int (*get_build_id) (char *build_id, uint32_t size);
	int (*set_build_id) (const char *build_id);
	int (*get_build_variant) (char *variant, uint32_t size);
	int (*set_build_variant) (const char *variant);
	int (*get_imei) (char *imei, uint32_t size);
	int (*set_imei) (const char *imei);
	int (*get_serial_number) (char *serial_number, uint32_t size);
	int (*set_serial_number) (const char *serial_number);
	int (*get_dump_result) (uint32_t *dump_result);
	int (*set_dump_result) (uint32_t dump_result);
	int (*get_scn_count) (uint32_t *scn_count);
	int (*get_signature) (char *signature, uint32_t size);
	int (*get_crash_time) (char *crash_time, uint32_t size);
	int (*set_crash_time) (const char *crash_time);
	int (*get_dumper_name) (char *dumper_name, uint32_t size);
	int (*set_dumper_name) (const char *dumper_name);
	int (*get_dumper_version) (char *dumper_version, uint32_t size);
	int (*set_dumper_version) (const char *dumper_version);
	int (*get_dump_type) (uint16_t *dump_type);
	int (*set_dump_type) (uint16_t dump_type);
	int (*get_device_type) (uint16_t *device_type);
	int (*set_device_type) (uint16_t device_type);
	int (*get_arch_type) (uint16_t *arch_type);
	int (*set_arch_type) (uint16_t arch_type);
};

/**
 * struct tlcore_section - Defines a section. Holds pointers to its respective
 *			section headers and section data. Also we can iterate
 *			over the sections using next() function.
 *
 * @next() :
 *	gets the next section in tlcore.
 *	Returns a valid section pointer or a NULL if failed.
 *
 * @header :
 *	Pointer to the section's corresponding header.
 *
 * @data :
 *	Pointer to the section's corresponding data.
 *
 * @reserve :
 *	This reserves a section header and a section data which the user
 *	might want to write to it later.
 *	@reserve_size[IN] - size of section data to be reserved in the
 *	tlcore file.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @commit :
 *	This function commits scndata to tlcore file which internally updates
 *	the section size in section header.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @next :
 *	Points to the next section. But this is not for the direct use by
 *	applications. Internally library will use it.
 */
struct tlcore_section {
	struct tlcore_section* (*get_next) (struct tlcore_section *scn);
	struct tlcore_section_header *header;
	struct tlcore_section_data *data;
	int (*reserve) (struct tlcore_section *scn, uint64_t reserve_size);
	int (*commit) (struct tlcore_section *scn);
	struct tlcore_section *next;
};

/**
 * struct section_header - Defines the Section header. This information defines
 *		what is contained in its corresponding section data.
 *
 * @get_scn_type :
 *	Get the type of Section from section header. i.e, text/binary
 *	@scn_type[OUT] - Specifies the type of Section. The value read
 *			in scn_type will be one among the #defines of
 *			SECTION_TYPE_***
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_scn_attribute:
 *	Get the attributes of the section.
 *	@scn_attr[OUT] - Specifies the attributes associated with a section.
 *			The value read in scn_attr can contain one or more
 *			of TLCORE_SECTION_ATTRIBUTE_***
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_scn_attribute:
 *	Set the attributes of the section.
 *	@scn_attr[IN] - Should pass the attributes associated with a section.
 *			The value read in scn_attr can contain one or more
 *			of TLCORE_SECTION_ATTRIBUTE_***
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_data_format :
 *	Get the format type from section header. i.e, straight/segmented
 *	@data_format[OUT] - Specifies the data format. The value read
 *			in data_format will be one among the fields of
 *			enum tlcore_scn_data_fmt
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_data_format :
 *	Set the data format Section in section header. i.e straight/segmented..
 *	@data_format[IN] -	Specifies the type of Section. The value being
 *				passed in data_format should be one among the
 *				fields of enum tlcore_scn_data_fmt.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_data_type :
 *	Get the data type used in this section. i.e, raw/zlib compressed etc..
 *	@data_type[OUT] -	Specifies the data datype. The value read
 *				in data_type will be one among the fields of
 *				enum tlcore_scn_data_type
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_data_type :
 *	Set the data type used in this section. i.e, raw/zlib compressed etc..
 *	@data_type[IN] -	Specifies the data datype. The value being
 *				passed in data_type should be one among the
 *				fields of enum tlcore_scn_data_type.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_size :
 *	Get the size of the section data in tlcore.
 *	@size[OUT] -		This is the size of the section data in tlcore.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_raw_size :
 *	Get the raw size of the dumped section. It returns the same value as
 *	get_size() if the data was not compressed while writing into tlcore.
 *	@raw_size[OUT] -	Specifies the size of the section in tlcore.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_raw_size :
 *	Set the raw size of the dumped section. raw size is uncompressed size.
 *	@raw_size[IN] -		Specifies the size of the section in tlcore.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_phys_addr :
 *	Get the section's physical address from section header.
 *	@phys_addr[OUT] -	Specifies the physical address from which the
 *				dump is collected into this section. If this is
 *				not same as dump address, then dump address is
 *				the address from which data is dumped into the
 *				tlcore.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_phys_addr :
 *	Set the section's physical address into section header.
 *	@phys_addr[OUT] -	Specifies the physical address from which the
 *				dump is collected into this section.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_dump_addr :
 *	Get the section's dump address from section header. Mostly this address
 *	is same as physical address. But in few cases(if dumper wants to use a
 *	specific region taht is being dumped), the actual data in this region
 *	is cloned into another region. In this case, the new address to which
 *	the data is cloned is dump address.
 *	@dump_addr[OUT] -	Specifies the dump address from which the
 *				dump is collected into this section.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_dump_addr :
 *	Set the section's dump address in section header. Mostly this address
 *	is same as physical address. But in few cases(if dumper wants to use a
 *	specific region taht is being dumped), the actual data in this region
 *	is cloned into another region. In this case, the new address to which
 *	the data is cloned is dump address.
 *	@dump_addr[OUT] -	Specifies the dump address from which the
 *				dump is collected into this section.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_name :
 *	Get the section name from section header.
 *	@name[OUT] -		Name of the section.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_DUMPER_NAME_SZ bytes for v4, v5 and a
 *				maximum of TLCORE_MAX_NAME_SZ_V6 bytes for v6
 *				are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_name :
 *	Set the section name into section header.
 *	@name[IN] -		Name of the section.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_core_name :
 *	Get the section name from section header.
 *	@name[OUT] -		Name of the section.
 *	@size[IN] -		Size bytes to be copied. A maximum of
 *				TLCORE_MAX_CORE_NAME_SZ bytes are copied.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_core_name :
 *	Set the section name into section header.
 *	@name[IN] -		Name of the section.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @private_data :
 *	This is section header private data. This data is used by library.
 *
 */
struct tlcore_section_header {
	int (*get_scn_type) (struct tlcore_section *scn, uint32_t *scn_type);
	int (*set_scn_attribute) (struct tlcore_section *scn,
			uint32_t scn_attr);
	int (*get_scn_attribute) (struct tlcore_section *scn,
			uint32_t *scn_attr);
	int (*get_data_format) (struct tlcore_section *scn,
			uint16_t *data_format);
	int (*set_data_format) (struct tlcore_section *scn,
			uint16_t data_format);
	int (*get_data_type) (struct tlcore_section *scn, uint16_t *data_type);
	int (*set_data_type) (struct tlcore_section *scn, uint16_t data_type);
	int (*get_size) (struct tlcore_section *scn, uint64_t *size);
	int (*get_raw_size) (struct tlcore_section *scn, uint64_t *raw_size);
	int (*set_raw_size) (struct tlcore_section *scn, uint64_t raw_size);
	int (*get_phys_addr) (struct tlcore_section *scn, uint64_t *phys_addr);
	int (*set_phys_addr) (struct tlcore_section *scn, uint64_t phys_addr);
	int (*get_dump_addr) (struct tlcore_section *scn, uint64_t *dump_addr);
	int (*set_dump_addr) (struct tlcore_section *scn, uint64_t dump_addr);
	int (*get_name) (struct tlcore_section *scn, char *name, uint32_t size);
	int (*set_name) (struct tlcore_section *scn, const char *name);
	int (*get_core_name) (struct tlcore_section *scn, char *name,
								uint32_t size);
	int (*set_core_name) (struct tlcore_section *scn, const char *name);
	void *private_data;
};

/**
 * struct tlcore_section_data - Functions that help application to read/write
 *				into sections.
 *
 * @write :
 *	This function writes the section data to tlcore file.
 *	@in_buf[IN] -	data which should be written to tlcore file
 *	@in_buf_size[IN] -	size of data which should write in to tlcore.
 *	@written_size[OUT] -	size written to tlcore
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @read :
 *	This function reads the section data from tlcore file.
 *	@out_buf[OUT]	-	data read from tlcore file
 *	@out_buf_size[IN] -	size of data which should be read from tlcore.
 *	@read_size[OUT]	-	size of data read from tlcore.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @set_pos :
 *	This function sets the file position indicator for the file stream in
 *	the section.
 *	@ pos[IN]	-	postion in the section
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_pos :
 *	This function gets the file position indicator for the file stream in
 *	the section.
 *	@pos[OUT]	-	postion in the section
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @block_init :
 *	This function writes the header information of segmented section.
 *	@raw_block_size	- Raw size of block before compression.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @block_write :
 *	This function writes the section data for segmented write into tlcore
 *	file.
 *	@block[in]	-	data which should be written as one block.
 *	@block_size[in]	-	size of block.
 *	Returns 0 for success. For error case while writing a block, file
 *	pointer is set to the end of the previous block.
 *
 * @get_raw_block_size :
 *	This function gets the size of raw block size.
 *	@raw_block_size[OUT] -	Raw size of block before compression.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_block_count :
 *	This function gets number of blocks in the section.
 *	@block_count[OUT]	- number of blocks in the section.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @get_max_compressed_block_size :
 *	This function gives the size of the largest block in the
 *	section. This should be used by the applications to know
 *	the size of the buffer to be allocated before calling
 *	block_read
 *	@max_block_size[OUT] - This is filled with the largest
 *	available compressed block size in the section.
 *	Returns 0 for success. Appropriate error(errno.h) otherwise.
 *
 * @block_read :
 *	This function reads the block for segmented data format.
 *	@block[OUT]  - data read from block, size of this buffer
 *			should get from get_raw_block_size.
 *	@read_block_size[OUT] -	size of block read from tlcore
 *	Returns 0 for success. For error case while writing a block, file
 *	pointer is set to the end of the previous block.
 *
 * @private_data :
 *	This is section header private data. This data is used by library.
 */
struct tlcore_section_data {
	int (*write) (struct tlcore_section *scn, void *in_buf,
			uint64_t in_buf_size, uint64_t *written_size);
	int (*read) (struct tlcore_section *scn, void *out_buf,
			uint64_t out_buf_size, uint64_t *read_size);
	int (*set_pos) (struct tlcore_section *scn, uint64_t pos);
	int (*get_pos) (struct tlcore_section *scn, uint64_t *pos);
	int (*block_init) (struct tlcore_section *scn, uint32_t raw_block_size);
	int (*block_write) (struct tlcore_section *scn, void *block,
			uint32_t block_size);
	int (*get_raw_block_size) (struct tlcore_section *scn,
			uint32_t *raw_block_size);
	int (*get_block_count) (struct tlcore_section *scn,
			uint32_t *block_count);
	int (*get_max_compressed_block_size) (struct tlcore_section *scn,
			uint32_t *max_block_size);
	int (*block_read) (struct tlcore_section *scn, void *block,
			uint32_t *read_block_size);
	void *private_data;
};

/**
 * struct tlcore - The most abstract structure that holds all sections together
 *
 * @tlhdr :
 *	tlhdr is initialized during generate_tlcore/load_tlcore. It contains
 *	the function pointers to write/read various sections of tlcore.
 *
 * @seaction_head :
 *	Head of the section information. Used to traverse and read/write the
 *	section information.
 *
 * @append_section :
 *	Creates a new Section and appends it to the previous section.
 *	@scn_type[IN]	-	Specifies the type of section. The value being
 *				passed in dump_type should be one among the
 *				fields of SECTION_TYPE_***
 *	Returns a valid (struct tlcore_section*) pointer to the newly created
 *	section, NULL in case of error.
 *
 * @get_mode :
 *	Returns the mode READONLY/READWRITE
 */
struct tlcore {
	struct tlcore_header *tlhdr;
	struct tlcore_section *section_head;
	struct tlcore_section* (*append_section) (uint16_t scn_type);
	uint16_t (*get_mode) (void);
};

#ifdef __cplusplus
}
#endif
#endif
