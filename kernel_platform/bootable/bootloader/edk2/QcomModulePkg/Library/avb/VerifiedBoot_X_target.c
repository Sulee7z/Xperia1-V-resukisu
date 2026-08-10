/*
 * Copyright 2017, 2022 Sony Corporation
 * All rights, including trade secret rights, reserved.
 */

#include "BootLinux.h"

VOID SonyXBootPublishAttestation(BootInfo *Info)
{
	EFI_STATUS Status = EFI_SUCCESS;
	UINT32 OsVersion = 0; // New Boot Image header information for N

	if (Info->Mode != XBOOT_MODE_SERVICE)
	{
		/* works only VB1
		   if support VB2, GetImage should be called */
		OsVersion = ((boot_img_hdr *)Info->Images[0].ImageBuffer)->os_version;
		DEBUG((EFI_D_VERBOSE, "OS_VERSION from Android header: %x\n", OsVersion));
		Status = XBootPublishAttestation(OsVersion);
	}

	if (Status != EFI_SUCCESS)
	{
		DEBUG((EFI_D_ERROR,
				"XBoot publish Attestation returned error, aborting boot %r\n",
				Status));
		ShutdownDevice();
	}
	else
	{
		DEBUG((EFI_D_VERBOSE, "XBoot publish Attestation SUCCESSFUL\n"));
	}
}
