/*
 * Copyright 2017, 2022 Sony Corporation
 * All rights, including trade secret rights, reserved.
 */

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BootCounterLib.h>
#include "Library/XHwresetProtocolWrapper.h"
#include <Library/XBoot.h>
#ifdef FEATURE_SONY_RAMDUMP
#include <Library/ResetCfgProtocolWrapper.h>
#endif /* FEATURE_SONY_RAMDUMP */
#include <Library/XResetReasonApiLib.h>
#include <Library/XResetReasonDefs.h>
#include <Library/PSM.h>
#include <Library/XShipModeLib.h>

#ifdef FEATURE_SONY_RAMDUMP
static const UINT32 restore_reasons[] =
{
	SONY_RREASON_CRASH,
	SONY_RREASON_CLEAR,
	SONY_RREASON_NO_RD_IMAGE,
	SONY_RREASON_RD_EARLY_CRASH_HWWD,
	SONY_RREASON_RD_CRASH_HWWD,
	SONY_RREASON_RD_NO_SPACE,
	SONY_RREASON_REBOOT_RD_CRASH,
	SONY_RREASON_REBOOT_RD_APPERR,
};

#define RAMDUMP_RESTORE_REASONS_ARRAY_SIZE (sizeof(restore_reasons)/ \
					sizeof(restore_reasons[0]))
#endif

EFI_STATUS SonySuppliments(VOID)
{
	EFI_STATUS Status;
	XBOOT_MODE Mode;

	/* increase boot counter */
	Status = IncBootCounter();
	if (EFI_ERROR (Status))
	{
		DEBUG((EFI_D_ERROR,
		       "IncBootCounter returned error %d\n", Status));
		return Status;
	}

	/* Sony Xboot Init */
	DEBUG((EFI_D_INFO, "XBoot Init\n"));
	Status = XBootInitLoader();
	if (EFI_ERROR (Status))
	{
		DEBUG((EFI_D_ERROR,
			"XBootInitLoader returned error %d\n", Status));
		return Status;
	}

	/* Sony MakeDecisions */
	DEBUG((EFI_D_INFO, "XBoot MakeDecisions\n"));
	Status = XBootMakeDecisions();
	if (EFI_ERROR (Status))
	{
		DEBUG((EFI_D_ERROR,
			"XBootMakeDecisions returned error %d\n", Status));
		return Status;
	}

	/* Print decided Xboot Mode */
	Status = XBootGetBootMode(&Mode);
	if (EFI_ERROR (Status))
	{
		DEBUG((EFI_D_ERROR, "GetBootmode returned error %d\n", Status));
		return Status;
	}
	Status = PSM_XBoot_PrintMode(Mode);
	if (Status != EFI_SUCCESS) {
		DEBUG((EFI_D_ERROR, "XBoot_PrintMode returned error %d\n", Status));
		return Status;
	}

	if (XIsShipMode()) {
		Status = XResetShipMode();
		if (EFI_ERROR (Status))
		{
			DEBUG((EFI_D_ERROR,
				"XResetShipMode returned error %d\n", Status));
			return Status;
		}
	}

	/* Sony Enable HW reset */
	DEBUG((EFI_D_INFO, "Enable XHwreset\n"));
	Status = XHwresetEnableApi();
	if(EFI_ERROR (Status))
	{
		DEBUG((EFI_D_ERROR, "XHwresetEnableApi returned error %d\n", Status));
		return Status;
	}

	return EFI_SUCCESS;
}

#ifdef FEATURE_SONY_RAMDUMP
VOID CleanRamdumpData(VOID);
VOID RestoreRamdumpDebugRegion(VOID);
EFI_STATUS SonyRamdumpSuppliments(VOID)
{
#ifdef FEATURE_XBOOT
	UINT32 i;

	/* Sony set default reset type as Warm Reset */
	ResetCfgWarmReset();

	/* Sony erase backup data on all resets except crash/failure cases */
	UINT32 rr = GetSonyResetReason();
	for (i = 0; i < RAMDUMP_RESTORE_REASONS_ARRAY_SIZE; i++) {
		if (rr == restore_reasons[i]) {
			RestoreRamdumpDebugRegion();
			break;
		}
	}
	if ((rr != SONY_RREASON_CRASH) && (rr != SONY_RREASON_CLEAR))
		CleanRamdumpData();
#endif /* FEATURE_XBOOT */

	return EFI_SUCCESS;
}
#endif /* FEATURE_SONY_RAMDUMP */
