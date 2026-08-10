/*
 * Copyright (C) 2014 Sony Mobile Communications Inc.
 * All rights, including trade secret rights, reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <tlcore.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <gelf.h>

#include "zlib_wrap.h"
#include "snappy_wrap.h"

#define EMERG 1
#define WARN 0

#define ASSERT(flag, x...)					\
do {								\
	int ret = x;						\
								\
	if (ret < 0) {						\
		fprintf(stderr, "%s <%d>: %s\n", __FILE__,	\
			 __LINE__, strerror(-ret));		\
	if (flag)						\
		return ret;					\
	}							\
} while (0)

enum ops {
	CORE = 1,
	PLATFORM,
	ALL
};

static struct tlcore *g_tlcore;

static int is_section_core(char *name)
{
	char *core_name = "core";
	int32_t len = strnlen(name, TLCORE_MAX_NAME_SZ_V6);
	int32_t core_len = strnlen(core_name, TLCORE_MAX_CORE_NAME_SZ), pos;
	pos = len - core_len;
	return !strncmp(name + pos, core_name, core_len);
}

Elf32_Ehdr common_ehdr32 = {
	.e_ident = {
		[EI_MAG0] = ELFMAG0,
		[EI_MAG1] = ELFMAG1,
		[EI_MAG2] = ELFMAG2,
		[EI_MAG3] = ELFMAG3,
		[EI_CLASS] = ELFCLASS32,
		[EI_DATA] = ELFDATA2LSB,
		[EI_VERSION] = EV_CURRENT,
		[EI_OSABI] = ELFOSABI_NONE },
	.e_type = ET_CORE,
	.e_machine = EM_ARM,
	.e_version = EV_CURRENT,
	.e_phoff = sizeof(Elf32_Ehdr),
	.e_ehsize = sizeof(Elf32_Ehdr),
	.e_phentsize = sizeof(Elf32_Phdr),
};

Elf32_Phdr common_phdr32 = {
.p_type = PT_LOAD,
.p_flags = (PF_R | PF_W | PF_X)
};

Elf64_Ehdr common_ehdr64 = {
	.e_ident = {
		[EI_MAG0] = ELFMAG0,
		[EI_MAG1] = ELFMAG1,
		[EI_MAG2] = ELFMAG2,
		[EI_MAG3] = ELFMAG3,
		[EI_CLASS] = ELFCLASS64,
		[EI_DATA] = ELFDATA2LSB,
		[EI_VERSION] = EV_CURRENT,
		[EI_OSABI] = ELFOSABI_NONE },
	.e_type = ET_CORE,
	.e_machine = EM_X86_64,
	.e_version = EV_CURRENT,
	.e_phoff = sizeof(Elf64_Ehdr),
	.e_ehsize = sizeof(Elf64_Ehdr),
	.e_phentsize = sizeof(Elf64_Phdr),
};

Elf64_Phdr common_phdr64 = {
	.p_type = PT_LOAD,
	.p_flags = (PF_R | PF_W | PF_X)
};

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
	ASSERT(WARN, tlcore_obj->tlhdr->get_dumper_version(dumper_version,
						sizeof(dumper_version)));
	ASSERT(WARN, tlcore_obj->tlhdr->get_platform_name(platform_name,
						sizeof(platform_name)));
	ASSERT(WARN, tlcore_obj->tlhdr->get_product_name(product_name,
						sizeof(product_name)));
	ASSERT(WARN, tlcore_obj->tlhdr->get_build_id(build_id,
						sizeof(build_id)));
	ASSERT(WARN, tlcore_obj->tlhdr->get_build_variant(build_variant,
						sizeof(build_variant)));
	ASSERT(WARN, tlcore_obj->tlhdr->get_imei(imei, sizeof(imei)));
	ASSERT(WARN, tlcore_obj->tlhdr->get_serial_number(serial,
						sizeof(serial)));
	ASSERT(WARN, tlcore_obj->tlhdr->get_dump_result(&dump_result));
	ASSERT(EMERG, tlcore_obj->tlhdr->get_scn_count(&scn_count));
	ASSERT(WARN, tlcore_obj->tlhdr->get_crash_time(crash_time,
						sizeof(crash_time)));
	ASSERT(WARN, tlcore_obj->tlhdr->get_dumper_name(dumper_name,
						sizeof(dumper_name)));
	ASSERT(WARN, tlcore_obj->tlhdr->get_dump_type(&dump_type));
	ASSERT(WARN, tlcore_obj->tlhdr->get_device_type(&device_type));
	ASSERT(WARN, tlcore_obj->tlhdr->get_signature((char *)signature,
						sizeof(signature)));
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

char *creat_elf64_header(int32_t count, uint64_t *phys_addr,
			uint64_t *raw_size, uint64_t *elfhdr_len)
{
	int32_t total_offset, i;
	char *hdr;
	Elf64_Ehdr ehdr64 = common_ehdr64;
	Elf64_Phdr phdr64 = common_phdr64;
	Elf64_Phdr *p_phdr64 = NULL;
	*elfhdr_len = sizeof(Elf64_Ehdr) + ((sizeof(Elf64_Phdr) * count));
	hdr = malloc(*elfhdr_len);
	if (!hdr) {
		printf("failed to allocate hdr\n");
		return NULL;
	}
	memset(hdr, 0, *elfhdr_len);
	ehdr64.e_phnum = count;
	memcpy(hdr, &ehdr64, sizeof(Elf64_Ehdr));
	total_offset = ehdr64.e_phoff + ehdr64.e_phentsize * ehdr64.e_phnum;
	for (i = 0, p_phdr64 = (Elf64_Phdr *)(hdr + sizeof(Elf64_Ehdr));
		 i < count; i++, p_phdr64++) {
		memcpy(p_phdr64, &phdr64, sizeof(Elf64_Phdr));
		p_phdr64->p_offset = total_offset;
		p_phdr64->p_vaddr = phys_addr[i];
		p_phdr64->p_paddr = phys_addr[i];
		p_phdr64->p_filesz = raw_size[i];
		p_phdr64->p_memsz = raw_size[i];
		total_offset += p_phdr64->p_memsz;
	}

	return hdr;
}

char *creat_elf32_header(int32_t count, uint64_t *phys_addr,
				uint64_t *raw_size, uint64_t *elfhdr_len)
{
	int32_t total_offset, i;
	char *hdr;
	Elf32_Ehdr ehdr32 = common_ehdr32;
	Elf32_Phdr phdr32 = common_phdr32;
	Elf32_Phdr *p_phdr32 = NULL;
	*elfhdr_len = sizeof(Elf32_Ehdr) +
			((sizeof(Elf32_Phdr) * count));
	hdr = malloc(*elfhdr_len);
	if (!hdr) {
		printf("failed to allocate hdr\n");
		return NULL;
	}

	memset(hdr, 0, *elfhdr_len);
	ehdr32.e_phnum = count;
	memcpy(hdr, &ehdr32, sizeof(Elf32_Ehdr));
	total_offset = ehdr32.e_phoff + ehdr32.e_phentsize * ehdr32.e_phnum;
	for (i = 0, p_phdr32 = (Elf32_Phdr *)(hdr + sizeof(Elf32_Ehdr));
		 i < count; i++, p_phdr32++) {
		memcpy(p_phdr32, &phdr32, sizeof(Elf32_Phdr));
		p_phdr32->p_offset = total_offset;
		p_phdr32->p_vaddr = (uint32_t)phys_addr[i];
		p_phdr32->p_paddr = (uint32_t)phys_addr[i];
		p_phdr32->p_filesz = (uint32_t)raw_size[i];
		p_phdr32->p_memsz = (uint32_t)raw_size[i];
		total_offset += p_phdr32->p_memsz;
	}

	return hdr;
}

static int create_elf_header(char *name)
{
	uint16_t arch_type, major, minor;
	uint32_t count = 0, scn_type;
	int ret = -EINVAL;
	uint64_t raw_size[64], phys_addr[64], elfhdr_len;
	struct tlcore_section *scn;
	char *hdr;
	FILE *fp;

	scn = g_tlcore->section_head;
	while (scn != NULL) {
		char name_ll[TLCORE_MAX_CORE_NAME_SZ] = {0};
		ret = scn->header->get_core_name(scn, name_ll, sizeof(name_ll));
		printf("name %s name_ll %s\n", name, name_ll);
		if (ret < 0) {
			scn = scn->next;
			continue;
		}

		if (!strncmp(name, name_ll, TLCORE_MAX_CORE_NAME_SZ)) {
			ASSERT(EMERG, scn->header->get_scn_type(scn,
						&scn_type));
			ASSERT(EMERG, scn->header->get_raw_size(scn,
						&raw_size[count]));
			ASSERT(EMERG, scn->header->get_phys_addr(scn,
						&phys_addr[count]));
			count++;
		}
		scn = scn->next;
	}

	printf("count for section %s is %d\n", name, count);
	ret = g_tlcore->tlhdr->get_version(&major, &minor);
	if (ret < 0) {
		printf("Failed to get major and minor version\n");
		return -EINVAL;
	}

	if (major >= 6) {
		ret = g_tlcore->tlhdr->get_arch_type(&arch_type);
		if (ret < 0)
			printf("Arch type not set\n");

		if (arch_type == TLCORE_ARCH_TYPE_ARM_64 ||
				arch_type == TLCORE_ARCH_TYPE_X86_64)
			hdr = creat_elf64_header(count, phys_addr, raw_size,
					&elfhdr_len);
		else if (arch_type == TLCORE_ARCH_TYPE_ARM_32 ||
				arch_type == TLCORE_ARCH_TYPE_X86_32)
			hdr = creat_elf32_header(count, phys_addr, raw_size,
					&elfhdr_len);
	} else {

		hdr = creat_elf32_header(count, phys_addr, raw_size,
					&elfhdr_len);
	}

	if (!hdr) {
		printf("Failed to create elf header\n");
		return -EINVAL;
	}

	fp = fopen(name, "w+");
	if (!fp) {
		printf("Failed to open file %s\n", name);
		return -EINVAL;
	}

	if (!fwrite(hdr, elfhdr_len, 1, fp))
		printf("Failed: %s\n", strerror(errno));

	free(hdr);
	fflush(fp);
	fclose(fp);
	return elfhdr_len;
}

static int segmented_scn_read(struct tlcore_section *scn, FILE *fp,
		char *scn_name)
{
	int ret = 0;
	uint32_t block_count;
	uint32_t max_comp_blk_size;
	uint32_t uncomp_blksize, comp_blksize;
	char *comp_block, *uncomp_block;

	ASSERT(EMERG, scn->data->get_raw_block_size(scn,
				&uncomp_blksize));
	printf("%d RAW BLK SIZE FOR SCN %s\n", uncomp_blksize,
			scn_name);
	ASSERT(EMERG, scn->data->get_block_count(scn, &block_count));
	printf("%d blocks in section %s\n", block_count, scn_name);
	ASSERT(EMERG, scn->data->get_max_compressed_block_size(scn,
					&max_comp_blk_size));
	comp_block = malloc(max_comp_blk_size);
	if (comp_block == NULL) {
		printf("memory allocation failed\n");
		return -errno;
	}

	uncomp_block = malloc(uncomp_blksize);
	if (uncomp_block == NULL) {
		printf("memory allocation failed\n");
		return -errno;
	}
	snappy_init();
	do {
		ASSERT(EMERG, scn->data->block_read(scn, comp_block,
					&comp_blksize));
		if (!comp_blksize) {
			printf("Data Finished\n");
			break;
		}
		snappy_uncompression(comp_block, comp_blksize,
				&uncomp_block, &uncomp_blksize);
		if (!fwrite(uncomp_block, uncomp_blksize, 1, fp))
			printf("Failed to write to file@%s\n",
					scn_name);
	} while (comp_blksize);
	if (comp_block)
		free(comp_block);
	snappy_end();
	return ret;
}

static int straight_scn_read(struct tlcore_section *scn, FILE *fp,
		char *scn_name)
{
	int ret = 0;
	char read_buf[4096];
	uint64_t read_size;

	do {
		ASSERT(WARN, scn->data->read(scn, read_buf,
					sizeof(read_buf), &read_size));
		if (read_size && !fwrite(read_buf, read_size, 1, fp))
			printf("Failed to write %ld bytes to file@ %s\n"
					, read_size, scn_name);
	} while (read_size);
	return ret;
}

static int extract_scn(char *scn_name)
{
	int ret = 0;
	FILE *fp, *fp_dest;
	char str[64] = {0};

	snprintf(str, sizeof(str), "dump_%s", scn_name);
	fp_dest = fopen(str, "w");
	if (!fp_dest) {
		printf("File cant be opened: %s\n", strerror(errno));
		return errno;
	}

	fp = fopen(scn_name, "r");
	printf("Name of the file %s\n", str);
	ret = decompress(fp, fp_dest);
	if (ret != Z_OK) {
		zerr(ret);
		return ret;
	}
	unlink(scn_name);
	fclose(fp_dest);
	return ret;
}

static int read_section(struct tlcore_section *scn, char *scn_name)
{
	uint8_t dummy = 0x13;
	struct stat sb;
	uint16_t data_fmt = -1, data_type;
	uint64_t scn_size, scn_raw_size;
	int32_t hdr_len;
	FILE *fp;
	int ret;

	ASSERT(EMERG, scn->header->get_data_format(scn, &data_fmt));
	printf("data format %s\n", data_fmt ?
			"SEGMENTED DATA" : "STRAIGHT DATA");
	ASSERT(EMERG, scn->header->get_data_type(scn, &data_type));
	ASSERT(EMERG, scn->header->get_size(scn, &scn_size));
	printf(" size %lu\n", scn_size);


	if (is_section_core(scn_name) && stat(scn_name, &sb)) {

		hdr_len = create_elf_header(scn_name);
		if (hdr_len < 0) {
			printf("Failed to create elf header\n");
			return -EINVAL;
		}
	}

	fp = fopen(scn_name, "a+");
	if (fp == NULL) {
		printf("Failed to open file %s\n", scn_name);
		return -EINVAL;
	}

	if (!scn_size) {

		if (data_type == TLCORE_DATA_TYPE_DUMMY) {
			ASSERT(EMERG, scn->header->get_raw_size(scn,
						&scn_raw_size));
			printf("Raw size %ld\n", scn_raw_size);
			while (scn_raw_size) {
				fwrite(&dummy, sizeof(uint8_t), 1, fp);
				scn_raw_size--;
			}
		}
		fclose(fp);
		return 0;
	}

	if (data_fmt) {
		ret = segmented_scn_read(scn, fp, scn_name);
		if (ret) {
			printf("Error in reading segmented scn\n");
			return ret;
		}
	} else {
		ret = straight_scn_read(scn, fp, scn_name);
		if (ret) {
			printf("Error in reading straight scn\n");
			return ret;
		}
	}

	fclose(fp);
	if ((data_type == TLCORE_DATA_TYPE_ZLIB) ||
			(data_type == TLCORE_DATA_TYPE_ZLIB_RAW))
		extract_scn(scn_name);
	return 0;
}

void print_usage(void)
{
	printf("Error in usage!\n\n\n");
	printf("Usage:\n\t./rd_extract [options] <tlcore path><filename>\n");
	printf("\nOptions:\n");
	printf("\t-c  - Extract Core files only\n");
	printf("\t-p  - Extract Platform files only\n");
	printf("\t-cp - Extract both Core and Platform files\n\n");
	printf("Ex:\n\t./rd_extract -c/-p/-cp ./tlcore\n\n\n\n");
	return;
}

int main(int argc, char **argv)
{
	struct tlcore *tlcore_obj;
	struct tlcore_section *scn_head, *scn;
	char name[TLCORE_MAX_NAME_SZ_V6];
	char core_name[TLCORE_MAX_CORE_NAME_SZ];
	int32_t ret, option;

	if (argc < 3) {
		print_usage();
		return -EINVAL;
	}

	if (!strncmp(argv[1], "-cp", 3))
		option = ALL;
	else if (!strncmp(argv[1], "-p", 2))
		option = PLATFORM;
	else if (!strncmp(argv[1], "-c", 2))
		option = CORE;
	else {
		print_usage();
		return -EINVAL;
	}

	tlcore_obj = init_tlcore(argv[2], TLCORE_READONLY);
	printf("\ninit_tlcore returned - %p\n", tlcore_obj);
	if (!tlcore_obj)
		return -EINVAL;

	g_tlcore = tlcore_obj;
	ret = show_tlhdr(tlcore_obj);
	if (ret < 0) {
		printf("tlcore_header Read Failed.\n");
		return ret;
	}
	/*function to read kernel log*/
	scn_head = tlcore_obj->section_head;

	switch (option) {
	case ALL:
	case CORE:
		/*Read core sections*/
		scn = scn_head;
		while (scn) {
			memset(core_name, 0, sizeof(core_name));
			ret = scn->header->get_core_name(scn, core_name,
					sizeof(core_name));
			if (ret < 0 ||
				!strnlen(core_name,
					TLCORE_MAX_CORE_NAME_SZ)) {
				scn = scn->next;
				continue;
			}
			printf("core name %s\n", core_name);

		ret = read_section(scn, core_name);
		if (ret < 0)
			return -EINVAL;

			scn = scn->next;
		}
		if (option != ALL)
			break;
	case PLATFORM:
		/*Read Platform sections*/
		scn = scn_head;
		while (scn) {
			memset(name, 0, sizeof(name));
			ASSERT(WARN, scn->header->get_name(scn, name,
					sizeof(name)));
			if (!strnlen(name, TLCORE_MAX_NAME_SZ_V6)) {
				scn = scn->next;
				continue;
			}

		ret = read_section(scn, name);
		if (ret < 0)
			return -EINVAL;

			scn = scn->next;
		}
		break;
	default:
		print_usage();
		break;
	}

	ASSERT(EMERG, finish_tlcore(tlcore_obj));
	printf("*****************************************************\n");
	printf("********************* FINISH ************************\n");
	printf("*****************************************************\n");
	return 0;
}
