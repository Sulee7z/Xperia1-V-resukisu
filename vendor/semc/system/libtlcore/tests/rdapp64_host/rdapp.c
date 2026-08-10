/*
 * Copyright (C) 2014 Sony Mobile Communications AB.
 * All rights, including trade secret rights, reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <tlcore.h>
#include "snappy_wrap.h"
#include <sys/syscall.h>
#include <linux/reboot.h>

#define EMERG 1
#define WARN 0

struct dump_scn {
	uint64_t phys_addr;
	uint64_t dump_addr;
	uint64_t size;
	struct dump_scn *next;
};

#define DUMP 1
#define DUMMY 2

static struct dump_scn *dump_list;
static struct dump_scn *dummy_list;

#define ORIGINAL_BLK_SZ 0x200000
#define DEVMEM "/dev/mem"

#define ASSERT(flag, x...) \
do { \
	int ret = x; \
	if (ret < 0) { \
		fprintf(stderr, "%s <%d>: %s\n", __FILE__, __LINE__, \
			strerror(-ret)); \
	if (flag) \
		return ret; \
	} \
} while (0)

#define IOMEM "/var/tlcore/rdapp64/iomem"
#define KLOG "/var/tlcore/rdapp64/klog"

int show_tlhdr(struct tlcore *tlcore_obj)
{
	char platform_name[TLCORE_MAX_PLATFORM_NAME_SZ];
	uint16_t major = 0, minor = 0, i = 0;
	char dumper_version[TLCORE_MAX_DUMPER_VERSION_SZ];
	char product_name[TLCORE_MAX_PRODUCT_NAME_SZ];
	char build_id[TLCORE_MAX_BUILD_ID_SZ];
	char build_variant[TLCORE_MAX_BUILD_VARIANT_SZ];
	char imei[TLCORE_MAX_IMEI_SZ];
	char serial[TLCORE_MAX_SERIAL_NO_SZ];
	uint32_t dump_result = 0;
	uint32_t scn_count = 0;
	unsigned char signature[TLCORE_MAX_SIGNATURE_SZ];
	char crash_time[TLCORE_MAX_CRASH_TIME_SZ];
	char dumper_name[TLCORE_MAX_DUMPER_NAME_SZ];
	uint16_t dump_type = 0;
	uint16_t device_type = 0, arch_type = 0;
	unsigned char *sign;

	ASSERT(WARN, tlcore_obj->tlhdr->get_version(&major, &minor));
	ASSERT(WARN, tlcore_obj->tlhdr->get_dumper_version(dumper_version));
	ASSERT(WARN, tlcore_obj->tlhdr->get_platform_name(platform_name));
	ASSERT(WARN, tlcore_obj->tlhdr->get_product_name(product_name));
	ASSERT(WARN, tlcore_obj->tlhdr->get_build_id(build_id));
	ASSERT(WARN, tlcore_obj->tlhdr->get_build_variant(build_variant));
	ASSERT(WARN, tlcore_obj->tlhdr->get_imei(imei));
	ASSERT(WARN, tlcore_obj->tlhdr->get_serial_number(serial));
	ASSERT(WARN, tlcore_obj->tlhdr->get_dump_result(&dump_result));
	ASSERT(EMERG, tlcore_obj->tlhdr->get_scn_count(&scn_count));
	ASSERT(WARN, tlcore_obj->tlhdr->get_crash_time(crash_time));
	ASSERT(WARN, tlcore_obj->tlhdr->get_dumper_name(dumper_name));
	ASSERT(WARN, tlcore_obj->tlhdr->get_dump_type(&dump_type));
	ASSERT(WARN, tlcore_obj->tlhdr->get_device_type(&device_type));
	ASSERT(WARN, tlcore_obj->tlhdr->get_signature((char *)signature));
	ASSERT(WARN, tlcore_obj->tlhdr->get_arch_type(&arch_type));

	printf("Major Version - %d \t Minor version - %d\n", major, minor);
	printf("Dumper Version - %s\n", dumper_version);
	printf("Platform name - %s\n", platform_name);
	printf("Product name - %s\n", product_name);
	printf("Build Id : %s\n", build_id);
	printf("build_variant : %s\n", build_variant);
	printf("IMEI - %s\n", imei);
	printf("Serial - %s\n", serial);
	printf("dump_result - %d\n", dump_result);
	printf("Section Count - %d\n", scn_count);
	sign = signature;
	printf("Signature - ");
	for (i = TLCORE_MAX_SIGNATURE_SZ; i > 0; i--) {
		printf("%02x:", *sign);
		sign++;
	}

	printf("\b ");
	printf("\n");
	printf("Crash time - %s\n", crash_time);
	printf("Dumper name - %s\n", dumper_name);
	printf("dump_type - %d\n", dump_type);
	printf("device_type - %d\n", device_type);
	printf("arch_type - %d\n", arch_type);
	return 0;
}

int tlhdr_write(struct tlcore *tlcore_obj)
{
	char platform_name[TLCORE_MAX_PLATFORM_NAME_SZ] = "rhine";
	char dumper_version[TLCORE_MAX_DUMPER_VERSION_SZ] = "2.1";
	char product_name[TLCORE_MAX_PRODUCT_NAME_SZ] = "togari";
	char build_id[TLCORE_MAX_BUILD_ID_SZ] = "2.A.0.1.32";
	char build_variant[TLCORE_MAX_BUILD_VARIANT_SZ] = "userdebug";
	char imei[TLCORE_MAX_IMEI_SZ] = "123456789";
	char serial[TLCORE_MAX_SERIAL_NO_SZ] = "FATOS";
	uint32_t dump_result = 0;
	char crash_time[TLCORE_MAX_CRASH_TIME_SZ] = "11:32:56";
	char dumper_name[TLCORE_MAX_DUMPER_NAME_SZ] = "test_dumper";
	uint16_t dump_type = 2;
	uint16_t device_type = 2;
	uint16_t arch_type = 2;

	printf("tlhdr_write\n");
	ASSERT(EMERG, tlcore_obj->tlhdr->set_dumper_version(dumper_version));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_platform_name(platform_name));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_product_name(product_name));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_build_id(build_id));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_build_variant(build_variant));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_imei(imei));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_serial_number(serial));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_dump_result(dump_result));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_crash_time(crash_time));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_dumper_name(dumper_name));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_dump_type(dump_type));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_device_type(device_type));
	ASSERT(EMERG, tlcore_obj->tlhdr->set_arch_type(arch_type));
	return 0;
}

void print_usage(void)
{
	printf("Error : specify path to tlcore and path to iomem\n");
	printf("Ex Usage:\n\trdapp ./tlcore ./iomem\n");
	return;
}

int open_memdev()
{
	int32_t fd;
	fd = open(DEVMEM, O_RDONLY);
	if (fd < 0) {
		printf("Failed to open /dev/mem : %s\n", strerror(errno));
		return -errno;
	}

	return fd;
}

void close_memdev(int32_t fd, char *vaddr, uint64_t offset, uint64_t size)
{
	if (munmap((void *)vaddr - offset, size + offset))
		printf("munmap(%d) %s\n", errno, strerror(errno));

	close(fd);
	return;
}

char *get_vaddr(int32_t fd, uint64_t phys_addr, uint64_t raw_size, uint64_t
		*offset)
{
	char *vaddr;

	*offset = phys_addr&0xFFF;
	printf("aligned offset %lx\n", *offset);

	vaddr = (char *) mmap(0, raw_size + *offset, PROT_READ,
			MAP_PRIVATE, fd, phys_addr - *offset);
	if (MAP_FAILED == vaddr) {
		printf("mmap failed(%d) %s\n", errno, strerror(errno));
		return NULL;
	}

	vaddr += *offset;
	return vaddr;
}

static int create_dump_section(struct tlcore *tlcore_obj, uint16_t data_fmt,
		uint16_t data_type, uint64_t raw_size, uint64_t phys_addr,
		uint64_t dump_addr, const char *plat_name, char *core_name)
{
	uint32_t comp_data_len, blksize = ORIGINAL_BLK_SZ;
	uint64_t written = 0, tl_write = 0;
	char *vaddr, *raw_data, *comp_data;
	struct tlcore_section *scn;
	uint64_t align_offset;
	int32_t fd;

	fd = open_memdev();
	if (fd < 0)
		return fd;

	vaddr = get_vaddr(fd, phys_addr, raw_size, &align_offset);
	if (!vaddr) {
		fprintf(stderr, "Failed to get vaddr for phys addr %lx\n",
				phys_addr);
		return -EINVAL;
	}

	scn = tlcore_obj->append_section(TLCORE_SECTION_TYPE_BIN);
	if (!scn) {
		fprintf(stderr, "Failed to create section\n");
		return -EINVAL;
	}

	ASSERT(EMERG, scn->header->set_scn_attribute(scn,
				TLCORE_SECTION_ATTRIBUTE_VENDOR));
	ASSERT(EMERG, scn->header->set_name(scn, plat_name));
	ASSERT(EMERG, scn->header->set_core_name(scn, core_name));
	ASSERT(EMERG, scn->header->set_data_format(scn, data_fmt));
	ASSERT(EMERG, scn->header->set_data_type(scn, data_type));
	ASSERT(EMERG, scn->header->set_raw_size(scn, raw_size));
	ASSERT(EMERG, scn->header->set_phys_addr(scn, phys_addr));
	ASSERT(EMERG, scn->header->set_dump_addr(scn, dump_addr));

	switch (data_fmt) {
	case TLCORE_DATA_FORMAT_STRAIGHT:
		/*TODO*/
		break;

	case TLCORE_DATA_FORMAT_SEGMENT:
		ASSERT(EMERG, scn->data->block_init(scn, ORIGINAL_BLK_SZ));
		while (written < raw_size) {
			blksize = (raw_size - written) > ORIGINAL_BLK_SZ ?
				ORIGINAL_BLK_SZ : (raw_size - written);
			raw_data = calloc(1, blksize);
			if (!raw_data) {
				printf("Failed to allocte memory for" \
					"ORIGINAL_BLK_SZ - %s\n",
					strerror(errno));
				return -errno;
			}

			memcpy(raw_data, vaddr + written, blksize);
			snappy_init();
			snappy_compression(raw_data, blksize,
					&comp_data, &comp_data_len);
			ASSERT(EMERG, scn->data->block_write(scn, comp_data,
					comp_data_len));

			written += blksize;
			tl_write += comp_data_len;
			printf("%ld total bytes read from RAM," \
					"uncomp block %d written to tlcore" \
					"%ld\n", written, blksize, tl_write);
			snappy_end();
			free(raw_data);
		}
		break;

	default:
		printf("Invalid data fmt type\n");
		return -EINVAL;
	}

	close_memdev(fd, vaddr, align_offset, raw_size);
	ASSERT(EMERG, scn->commit(scn));
	return 0;
}

static int create_text_section_from_file(struct tlcore_section *scn,
		uint16_t data_fmt, uint16_t data_type,
		char *path, const char *name)
{
	int ret, fd;
	char *read_buf[4096];
	uint64_t written, readbuf_size, total_written = 0;

	ASSERT(EMERG, scn->header->set_scn_attribute(scn,
				TLCORE_SECTION_ATTRIBUTE_REPORT));
	ASSERT(EMERG, scn->header->set_name(scn, name));
	ASSERT(EMERG, scn->header->set_data_format(scn, data_fmt));
	ASSERT(EMERG, scn->header->set_data_type(scn, data_type));

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("Failed to open %s - %s\n", path, strerror(errno));
		return -errno;
	}

	do {
		memset(read_buf, 0, sizeof(read_buf));
		readbuf_size = read(fd, read_buf, sizeof(read_buf));
		ASSERT(EMERG, scn->data->write(scn, read_buf,
					readbuf_size, &written));
		total_written += written;
	} while (readbuf_size);

	ASSERT(EMERG, scn->header->set_raw_size(scn, total_written));
	printf("Written %ld\n", written);
	ret = scn->commit(scn);
	if (ret < 0)
		return ret;
	close(fd);
	return 0;
}

static int seek_for_overlaps(void)
{
	struct dump_scn *new, *dump, *dummy;

	dump = dump_list;
	dummy = dummy_list;
	while (dump != NULL) {
		while (dummy != NULL) {
			if ((dump->phys_addr <= dummy->phys_addr) &&
				((dump->phys_addr + dump->size) >
						dummy->phys_addr)) {
				new = malloc(sizeof(struct dump_scn));
				if (!new) {
					printf("failed to allocte memory:%s\n",
						strerror(errno));
					return -EINVAL;
				}
				if (dump->phys_addr < dummy->phys_addr) {
					new->phys_addr = dummy->phys_addr +
							 dummy->size + 1;
					new->dump_addr = new->phys_addr;
					new->size = (dump->phys_addr +
						dump->size) - (dummy->phys_addr
						+ dummy->size);
					dump->size = (dummy->phys_addr -
							dump->phys_addr)-1;
					new->next = dump->next;
					dump->next = new;
				} else {
					new->phys_addr = dummy->phys_addr +
								dummy->size + 1;
					new->size = (dump->phys_addr +
							dump->size) -
							(dummy->phys_addr +
							dummy->size);
					dump->phys_addr = new->phys_addr;
					dump->dump_addr = dump->phys_addr;
					dump->size = new->size;
					free(new);
				}
			}
			dummy = dummy->next;
		}
		dummy = dummy_list;
		dump = dump->next;
	}
	return 0;
}

int add_dump_info(uint64_t start, uint64_t end, uint16_t type)
{
	struct dump_scn *new, *temp;

	new = calloc(1, sizeof(struct dump_scn));
	if (!new) {
		printf("failed to allocte memory - %s\n", strerror(errno));
		return -errno;
	}

	new->phys_addr = start;
	new->dump_addr = start;
	new->size = (end - start) - 1;
	new->next = NULL;

	switch (type) {
	case DUMP:
		if (!dump_list)
			dump_list = new;
		else {
			temp = dump_list;
			while (temp->next)
				temp = temp->next;
			temp->next = new;
		}

		break;

	case DUMMY:
		if (!dummy_list)
			dummy_list = new;
		else {
			temp = dummy_list;
			while (temp->next)
				temp = temp->next;

			temp->next = new;
		}

		break;

	default:
		printf("Invalid type in add dump info\n");
		return -EINVAL;
	}

	return 0;
}

int get_memory_info()
{
	FILE *file;
	char buffer[80], name[32], cont[32];
	uint64_t start_addr, end_addr;
	int ret;

	file = fopen(IOMEM, "r");
	if (!file) {
		printf("failed to open IOMEM\n");
		return -EINVAL;
	}

	printf("before loop\n");
	while (NULL != fgets(buffer, sizeof(buffer), file)) {
		if (sscanf(buffer, "%16lx-%16lx : %31s %31s", &start_addr,
					&end_addr, name, cont) == 4) {
			if (!strncmp(name, "System", 6) && !strncmp(cont,
							"RAM", 3)) {
				printf("%016lx-%016lx : %31s\n", start_addr,
						end_addr, name);
				add_dump_info(start_addr, end_addr, DUMP);
			}
			if (!(strncmp(name, "Crash\n", 5))) {
				printf("%016lx-%016lx : %31s\n", start_addr,
						end_addr, name);
				add_dump_info(start_addr, end_addr, DUMMY);
			}
		}
	}

	ret = seek_for_overlaps();
	if (ret < 0) {
		printf("Seeking for overlaps failed.\n");
		return ret;
	}
	fclose(file);
	return 0;
}

static void free_dump_list(uint16_t type)
{
	struct dump_scn *head, *next;

	if (type == DUMP)
		head = dump_list;
	else if (type == DUMMY)
		head = dummy_list;

	while (head != NULL) {
		next = head->next;
		free(head);
		head = next;
	}

	return;
}

int create_tlcore(struct tlcore *tlcore_obj)
{
	int ret;
	struct tlcore_section *scn;
	struct dump_scn *dump_section;

	ret = tlhdr_write(tlcore_obj);
	if (ret < 0) {
		printf("tlhdr Write Failed.\n");
		return ret;
	}

	if (!tlcore_obj->section_head)
		printf("No section added\n");

	scn = tlcore_obj->append_section(TLCORE_SECTION_TYPE_TEXT);
	if (!scn) {
		printf("Failed to create section\n");
		return -EINVAL;
	}

	ret = scn->reserve(scn, 4098);
	if (ret < 0) {
		printf("Section reserve failed.\n");
		return ret;
	}

	ret = create_text_section_from_file(scn, TLCORE_DATA_FORMAT_STRAIGHT,
			TLCORE_DATA_TYPE_RAW, IOMEM, "iomem.log");
	if (ret < 0) {
		printf("Failed to create section iomem.log\n");
		return -EINVAL;
	}

	scn = tlcore_obj->append_section(TLCORE_SECTION_TYPE_TEXT);
	if (!scn) {
		printf("Failed to create section\n");
		return -EINVAL;
	}

	ret = create_text_section_from_file(scn, TLCORE_DATA_FORMAT_STRAIGHT,
			TLCORE_DATA_TYPE_RAW, KLOG, "kernel.log");
	if (ret < 0) {
		printf("Failed to create section iomem.log\n");
		return -EINVAL;
	}

	ret = get_memory_info();
	if (ret < 0) {
		printf("failed to get memory info\n");
		return -EINVAL;
	}


	dump_section = dump_list;
	while (dump_section) {
		printf("phy %lx dump %lx size %lx\n", dump_section->phys_addr,
				dump_section->dump_addr, dump_section->size);

		ret = create_dump_section(tlcore_obj,
				TLCORE_DATA_FORMAT_SEGMENT,
				TLCORE_DATA_TYPE_SNAPPY, dump_section->size,
				dump_section->phys_addr,
				dump_section->dump_addr, "DDRSC0.BIN",
				"vmcore");
		dump_section = dump_section->next;
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct tlcore *tlcore_obj;
	int ret;

	if (argc < 2) {
		print_usage();
		return -EINVAL;
	}

	tlcore_obj = init_tlcore(argv[1], TLCORE_READWRITE);
	printf("\ninit_tlcore returned - %p\n", tlcore_obj);
	if (!tlcore_obj)
		return -EINVAL;

	ret = create_tlcore(tlcore_obj);
	if (ret < 0) {
		printf("creat_tlcore Failed.\n");
		return ret;
	}

	printf("Write Completed Successfully\n");
	printf("Following are the details written into tlhdr\n");
	ret = show_tlhdr(tlcore_obj);
	if (ret < 0) {
		printf("tlcore_header Read Failed.\n");
		return ret;
	}

	free_dump_list(DUMP);
	free_dump_list(DUMMY);
	ret = finish_tlcore(tlcore_obj);
	if (ret < 0) {
		printf("finish failed.\n");
		return ret;
	}

	sync();
	printf("\n*****************************************************\n");
	printf("********************* FINISH ************************\n");
	printf("*****************************************************\n");
	syscall(__NR_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
			LINUX_REBOOT_CMD_RESTART2, "rdapp_host");
	return 0;
}
