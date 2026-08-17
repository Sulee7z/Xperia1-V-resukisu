/*
 * Copyright 2017, 2022 Sony Corporation
 * All rights, including trade secret rights, reserved.
 */

#include "BootLinux.h"

#include <Library/MazeLib.h>
#include <Library/SoumenProtocolWrapper.h>

EFI_STATUS FinalizeXBootServices(BootInfo *Info)
{
	EFI_STATUS Status;

	/* Should shut down and not return in case longpress is too short */
	Status = XBootFinalizeLongpress();
	if (EFI_ERROR(Status))
	{
		DEBUG((EFI_D_ERROR, "Failed to Finalize Longpress: %r\n", Status));
	}

	/* Stop maze service */
	Status = MazeStop();
	if(EFI_ERROR (Status))
	{
		return Status;
	}

	/* Stop soumen service */
	Status = SoumenStop();
	if(EFI_ERROR (Status))
	{
		return Status;
	}

	return EFI_SUCCESS;
}
