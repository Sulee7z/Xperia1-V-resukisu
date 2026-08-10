ifeq ($(SEMC_CFG_GENERATE_IDD_PARTITION), true)
INSTALLED_APPSLOGIMAGE_TARGET := $(PRODUCT_OUT)/appslog.img
INSTALLED_DIAGIMAGE_TARGET := $(PRODUCT_OUT)/diag.img

# $(1) output file name
define create-dummy-file
	# making 8kbyte file for removing ext4 superblocks.
	# See read_ext4_superblock and is_ext4_superblock_valid on fs_mgr.cpp
	@dd if=/dev/zero of=$(1) bs=4096 count=2
endef

$(INSTALLED_APPSLOGIMAGE_TARGET):
	$(call pretty,"Target appslog fs image: $(INSTALLED_APPSLOGIMAGE_TARGET)")
	$(call create-dummy-file,$(INSTALLED_APPSLOGIMAGE_TARGET))

$(INSTALLED_DIAGIMAGE_TARGET):
	$(call pretty,"Target diag fs image: $(INSTALLED_DIAGIMAGE_TARGET)")
	$(call create-dummy-file,$(INSTALLED_DIAGIMAGE_TARGET))
endif
