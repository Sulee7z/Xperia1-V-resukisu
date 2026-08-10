# This mk file is for only target build.
# For vendor modules.

PRODUCT_PACKAGES += \
    libidd \
    iddd \
    idd-logreader \
    libprotobuf-cpp-2.3.0-lite \
    vendor.semc.system.idd@1.1-service \
    idd.fstab \
    startup-prober \
    startup-prober.rc

PRODUCT_PACKAGES_DEBUG += \
    iddc

ifeq ($(TARGET_BUILD_VARIANT), user)
PRODUCT_PACKAGES += iddd_user_conf
else
PRODUCT_PACKAGES += iddd_debug_conf
endif


ifeq ($(TARGET_BUILD_VARIANT), user)
PRODUCT_PACKAGES += \
    idd_report.pem \
    idd_config.pem
endif

ifneq ($(SOMC_TARGET_PRODUCT),)
PRODUCT_PROPERTY_OVERRIDES += vendor.idd.product.name=$(SOMC_TARGET_PRODUCT)
else
PRODUCT_PROPERTY_OVERRIDES += vendor.idd.product.name=$(TARGET_PRODUCT)
endif

