/**
  Copyright (C) DuoWoA authors. All rights reserved.
  Copyright (C) Microsoft Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/AcpiTableUpdateLib.h>
#include <Library/DeviceBootManagerLib.h>
#include <Library/PlatformBootManagerLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SecureBootHandlerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MsBootOptionsLib.h>
#include <Library/BootGraphicsLib.h>
#include <Library/BootGraphics.h>
#include <Library/PrintLib.h>

#include <Configuration/BootDevices.h>

#include "BootManager.h"
#include "XnuLoader.h"

//
// Global Variables
//
STATIC UINTN XPos = 0;
STATIC UINTN YPos = 0;

//
// BDS Combo Message
//
STATIC CONST CHAR8 *ComboMessage = "[Volume Up] Boot Manager";

VOID
EFIAPI
PlatformBootManagerBeforeConsole ()
{
  // Signal End Of DXE
  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);

  // Dispatch Deferred Images
  EfiBootManagerDispatchDeferredImages ();

  // Register Boot Options
  MsBootOptionsLibRegisterDefaultBootOptions ();

  // Register Console Variables
  EfiBootManagerUpdateConsoleVariable (ConIn,  (EFI_DEVICE_PATH_PROTOCOL *)&KeypadDevicePath,  NULL);
  EfiBootManagerUpdateConsoleVariable (ConOut, (EFI_DEVICE_PATH_PROTOCOL *)&DisplayDevicePath, NULL);
  EfiBootManagerUpdateConsoleVariable (ErrOut, (EFI_DEVICE_PATH_PROTOCOL *)&DisplayDevicePath, NULL);

  // Execute Secondary Before Console
  DeviceBootManagerBeforeConsole (NULL, NULL);
}

VOID
PostGopSetup (OUT EFI_GRAPHICS_OUTPUT_PROTOCOL **GopProtocol)
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *LocalGopProtocol;

  // Verify Parameters
  if (GopProtocol == NULL) {
    return;
  }

  // Locate GOP Protocol
  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID *)&LocalGopProtocol);
  if (EFI_ERROR (Status) && Status != EFI_UNSUPPORTED) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to Locate GOP Protocol from Handle! Status = %r\n", __FUNCTION__, Status));
    return;
  }

  // Check Status
  if (Status == EFI_UNSUPPORTED) {
    // Locate GOP Protocol
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID *)&LocalGopProtocol);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "%a: Failed to Locate GOP Protocol! Status = %r\n", __FUNCTION__, Status));
      return;
    }

    // Install GOP Protocol
    Status = gBS->InstallProtocolInterface (&gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, EFI_NATIVE_INTERFACE, LocalGopProtocol);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "%a: Failed to Install GOP Protocol onto \"ConsoleOutHandle\"! Status = %r\n", __FUNCTION__, Status));
    }
  }

  // Pass GOP Protocol
  *GopProtocol = LocalGopProtocol;
}

/*
 * CHIRON_GPU_REGISTER_PROBE
 *
 * Reads the A540's registers from the firmware and prints them, without writing any of
 * them. Six layers of the port agree with the MSM8998 kernel sources; this is the first
 * thing that can disagree with the phone.
 *
 * Read-only on purpose. Bringing the GPU up from here, while Android's kgsl may still hold
 * the block, is the kind of step that hangs with nothing to show for it. Reading first
 * answers the two questions everything else rests on: are these three windows reachable at
 * this point in the boot, and do the values look like a GPU at all.
 *
 * Every address below comes from the port's own headers -- ChironAdrenoA540Gpucc.h,
 * -Gmu.h, -CpStart.h and -Smmu.h -- so a mismatch here means those headers are wrong.
 */
STATIC
UINT32
GpuRead32 (
  IN UINT64 Address
  )
{
  return *(volatile UINT32 *)(UINTN)Address;
}

STATIC
VOID
ChironGpuRegisterProbe (
  VOID
  )
{
  UINT64 El;

  __asm__ volatile ("mrs %0, CurrentEL" : "=r" (El));

  Print (L"==== A540 GPU probe (read only), CurrentEL=%lx ====\n", El);

  /* Identity: an A540 reports a product id in the 0x0504 range. */
  Print (L"GPU_REVISION=%08x CHIP_ID=%08x PRODUCT_ID=%08x RBBM_STATUS=%08x\n",
         GpuRead32 (0x05000000), GpuRead32 (0x05000004), GpuRead32 (0x05000008),
         GpuRead32 (0x0500007C));

  /* Power: bit 0 of each GDSCR is SW_COLLAPSE, set meaning the domain is down. */
  Print (L"CX_GDSCR=%08x (collapse %u) GX_GDSCR=%08x (collapse %u)\n",
         GpuRead32 (0x05001004), GpuRead32 (0x05001004) & 1u,
         GpuRead32 (0x05001094), GpuRead32 (0x05001094) & 1u);
  Print (L"GPU_GX_BCR=%08x GX_DOMAIN=%08x\n",
         GpuRead32 (0x05001090), GpuRead32 (0x05065130));

  /* Clocks: bit 0 of a CBCR is the gate enable. */
  Print (L"GFX3D_CBCR=%08x (on %u) CXO_CBCR=%08x GFX3D_RCGR=%08x\n",
         GpuRead32 (0x05066098), GpuRead32 (0x05066098) & 1u,
         GpuRead32 (0x05066020), GpuRead32 (0x05066070));

  /* GMU: 0xBABEFACE in GENERAL_0 means the firmware is running. These are word offsets. */
  Print (L"GPMU_SYSRESET=%08x GENERAL_0=%08x GENERAL_1=%08x RBBM_INTR=%08x\n",
         GpuRead32 (0x05000000 + 0xA8D8 * 4), GpuRead32 (0x05000000 + 0xA8E0 * 4),
         GpuRead32 (0x05000000 + 0xA8E1 * 4), GpuRead32 (0x05000000 + 0xA8D6 * 4));

  /* CP ring. */
  Print (L"CP_RB_BASE=%08x CP_RB_CNTL=%08x CP_RPTR=%08x CP_WPTR=%08x ME_CNTL=%08x\n",
         GpuRead32 (0x05000000 + 0x800 * 4), GpuRead32 (0x05000000 + 0x802 * 4),
         GpuRead32 (0x05000000 + 0x806 * 4), GpuRead32 (0x05000000 + 0x807 * 4),
         GpuRead32 (0x05000000 + 0x832 * 4));

  /*
   * SMMU context bank 0. ACTLR is the interesting one: the generic driver writes
   * 0x70000000 there only for SMMU_MODEL_QCOM_V2, and that single register is the whole
   * difference between qcom,smmu-v2 and a stock SMMU-500. Reading 70000000 would confirm
   * the SMMU layer against silicon rather than against source.
   */
  Print (L"SMMU_CB_SCTLR=%08x (M %u) CB_ACTLR=%08x (expect 70000000) TTBR0=%08x TTBCR=%08x\n",
         GpuRead32 (0x05048000), GpuRead32 (0x05048000) & 1u,
         GpuRead32 (0x05048004), GpuRead32 (0x05048020), GpuRead32 (0x05048030));

  Print (L"==== end of A540 GPU probe ====\n");
}

VOID
EFIAPI
PlatformBootManagerAfterConsole ()
{
  EFI_STATUS                    Status      = EFI_SUCCESS;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GopProtocol = NULL;

  // Post Setup GOP
  PostGopSetup (&GopProtocol);

  // Display Boot Logo
  Status = DisplayBootGraphic (BG_SYSTEM_LOGO);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to Display Boot Logo! Status = %r\n", __FUNCTION__, Status));
  }

  // Read the GPU registers before anything else touches the block.
  ChironGpuRegisterProbe ();

  // Register Key Callback
  Status = RegisterKeyCallback ((EFI_DEVICE_PATH_PROTOCOL *)&KeypadDevicePath);
  if (!EFI_ERROR (Status) && GopProtocol != NULL) {
    // Set Screen Resolution
    UINT32 ScreenWidth  = GopProtocol->Mode->Info->HorizontalResolution;
    UINT32 ScreenHeight = GopProtocol->Mode->Info->VerticalResolution;

    // Set Combo Message Position
    XPos = (ScreenWidth - AsciiStrLen (ComboMessage) * EFI_GLYPH_WIDTH) / 2;
    YPos = ScreenHeight * 48 / 50;
  }

  // Execute Secondary After Console
  DeviceBootManagerAfterConsole ();

}

VOID
EFIAPI
PlatformBootManagerOnDemandConInConnect ()
{
  // Execute Secondary On Demand
  DeviceBootManagerOnDemandConInConnect ();
}

VOID
EFIAPI
PlatformBootManagerProcessBootCompletion (OUT EFI_BOOT_MANAGER_LOAD_OPTION *BootOption)
{
  return;
}

VOID
EFIAPI
PlatformBootManagerPriorityBoot (IN OUT UINT16 **BootNext)
{
  EFI_STATUS                   Status;
  EFI_BOOT_MANAGER_LOAD_OPTION LoadOption;
  BOOLEAN                      BootManagerFlag;

  // Unregister Key Callback
  UnregisterKeyCallback ();

  // Get Boot Manager Flag
  BootManagerFlag = EnterBootManager ();
  if (BootManagerFlag == FALSE) {
    return;
  }

  // Get Boot Manager Load Option
  Status = MsBootOptionsLibGetBootManagerMenu (&LoadOption, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to get Boot Manager Load Option! Status = %r", __FUNCTION__, Status));
    return;
  }

  // Execute Boot Manager Load Option
  EfiBootManagerBoot           (&LoadOption);
  EfiBootManagerFreeLoadOption (&LoadOption);
}

VOID
EFIAPI
PlatformBootManagerWaitCallback (IN UINT16 TimeoutRemain)
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;

  // Verify Combo Message Position
  if (XPos == 0 && YPos == 0) {
    return;
  }

  // Get Timeout Time
  UINT16 Timeout = PcdGet16 (PcdPlatformBootTimeOut);

  // Check Remaining Timeout
  if (TimeoutRemain == Timeout || TimeoutRemain == 0) {
    // Set Draw Color
    UINT8 DrawColor = (TimeoutRemain == Timeout) ? 0xFF : 0x00;

    // Set GOP Draw Color
    Color.Red = Color.Green = Color.Blue = DrawColor;

    // Print Combo Message
    AsciiPrintXY (XPos, YPos, &Color, NULL, ComboMessage);
  }
}

VOID
EFIAPI
PlatformBootManagerUnableToBoot ()
{
  EFI_STATUS Status;

  /*
   * CHIRON_NO_XNULOADER_HERE
   *
   * The Mi 6 and redwood ports hand over to XnuLoaderMain() at this point, and this tree carries that
   * call.  It cannot be used on chiron: XnuLoader.h hard-codes the payload location
   *
   *     #define XNU_PAYLOAD_BASE     0xF0000000ULL
   *     #define XNU_PAYLOAD_MAX_SIZE 0x04000000ULL
   *
   * which is redwood's, and chiron's packed image is built with --base 0x10000000.  Running it here
   * reads a header from an address that holds nothing of the sort, so the loader either rejects it or
   * walks memory that is not its own -- and the screen ends up blue with unreadable text, which is what
   * was reported.
   *
   * On chiron the XNU loader is a separate EFI application started by NXULDR (see xnuloader/), not
   * something this library launches from BDS.  So nothing is done here.  The fallback path below still
   * shows the "no boot OS" graphic and shuts down cleanly.
   */

  // Execute Secondary Unable To Boot
  DeviceBootManagerUnableToBoot ();

  // Display No Boot OS Logo
  Status = DisplayBootGraphic (BG_NO_BOOT_OS);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: Failed to Display No Boot OS Logo! Status = %r\n", __FUNCTION__, Status));
    Print (L"No Operating System Found! Please make sure that an OS is Installed.\n");
  }

  // Wait 10s
  gBS->Stall (10000000);

  // Shutdown
  gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);

  // Do Cpu Dead Loop
  CpuDeadLoop ();
}

VOID
EFIAPI
PlatformBootManagerBdsEntry ()
{
  EFI_STATUS Status;

  // Setup Secure Boot
  Status = SetupSecureBoot ();
  ASSERT_EFI_ERROR (Status);

  // Update ACPI Tables
  UpdateAcpiTables ();

  // Execute Secondary BDS Entry
  DeviceBootManagerBdsEntry ();
}
