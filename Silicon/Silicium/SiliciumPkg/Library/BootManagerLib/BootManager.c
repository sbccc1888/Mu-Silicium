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
#include <Protocol/BlockIo.h>
#include <Protocol/PartitionInfo.h>
#include <Library/BaseLib.h>

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
 * CHIRON_GPU_PROBE_LOG
 *
 * The probe prints to the panel, but a photo of a panel is a poor way to read eight
 * registers, and this port has no serial console. So the same text also goes to a raw
 * block on the "logfs" partition, where Android can read it with dd after the reset.
 *
 * The GPT walk is lifted from xnuloader, where it has been reading and writing this exact
 * partition for a while -- including the fix for a search that compared one character too
 * many and therefore never matched. Using the same partition means the readback command is
 * one this project already knows works:
 *
 *     adb shell "dd if=/dev/block/sda10 bs=4096 count=2048" > probe-log.bin
 */
STATIC
EFI_STATUS
GpuProbeFindLogPartition (
  OUT EFI_BLOCK_IO_PROTOCOL  **OutBlockIo
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                         *Handles;
  UINTN                               Count;
  UINTN                               Index;
  CONST CHAR16                        Want[] = L"oops";
  EFI_PARTITION_INFO_PROTOCOL        *Info;
  EFI_BLOCK_IO_PROTOCOL              *Bio;

  /*
   * Ask the firmware, do not re-derive the GPT. EFI_PARTITION_INFO_PROTOCOL is filled in
   * from the table the firmware has already parsed, and the same handle gives up its block
   * IO directly -- so no LBA is ever computed, and none can be computed wrong.
   *
   * This is the route xnuloader uses, and it is the better one: an earlier version of this
   * function read LBA 1, checked for "EFI PART", walked the entry array and compared names
   * by hand, which is four chances to be wrong that the protocol does not offer.
   */
  *OutBlockIo = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPartitionInfoProtocolGuid,
                  NULL,
                  &Count,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < Count; Index++) {
    Status = gBS->HandleProtocol (Handles[Index], &gEfiPartitionInfoProtocolGuid, (VOID **)&Info);
    if (EFI_ERROR (Status) || (Info == NULL) || (Info->Type != PARTITION_TYPE_GPT)) {
      continue;
    }

    if (StrCmp (Info->Info.Gpt.PartitionName, Want) != 0) {
      continue;
    }

    Status = gBS->HandleProtocol (Handles[Index], &gEfiBlockIoProtocolGuid, (VOID **)&Bio);
    if (EFI_ERROR (Status) || (Bio == NULL) || (Bio->Media == NULL)) {
      continue;
    }

    /* A media with no usable sector size cannot be written to meaningfully. */
    if ((Bio->Media->BlockSize == 0) || (Bio->Media->BlockSize > 4096)) {
      continue;
    }

    *OutBlockIo = Bio;
    FreePool (Handles);
    return EFI_SUCCESS;
  }

  FreePool (Handles);

  /*
   * No oops partition. logfs is the only other candidate and it is a live FAT12 volume; a
   * raw write there damages firmware-owned data. Reporting and writing nothing is the right
   * trade: a missing log is recoverable, a wrecked volume is not.
   */
  return EFI_NOT_FOUND;
}

STATIC
UINT8  mGpuProbeLog[4096] __attribute__ ((aligned (4096)));
STATIC UINTN  mGpuProbeLogLen;

STATIC
VOID
GpuProbeLog (
  IN CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST  Args;
  UINTN    Remaining;

  // AsciiVSPrint, not UnicodeVSPrintAsciiFormat: the buffer is bytes, and the "Unicode"
  // in that name refers to the destination type, not the source. Mismatching them is what
  // two build failures in a row were about.
  VA_START (Args, Format);
  Remaining = sizeof (mGpuProbeLog) - mGpuProbeLogLen;
  if (Remaining > 2) {
    mGpuProbeLogLen += AsciiVSPrint (
                         (CHAR8 *)mGpuProbeLog + mGpuProbeLogLen,
                         Remaining,
                         Format,
                         Args
                         );
  }

  VA_END (Args);
}

STATIC
VOID
GpuProbeFlush (
  VOID
  )
{
  EFI_BLOCK_IO_PROTOCOL  *Bio;
  EFI_STATUS              Status;

  if (mGpuProbeLogLen == 0) {
    return;
  }

  Status = GpuProbeFindLogPartition (&Bio);
  if (EFI_ERROR (Status)) {
    return;
  }

  /*
   * Block 0 of the partition. The protocol gave us the partition's own block IO, so block 0
   * is its first sector -- no LBA arithmetic, which is what the hand-rolled finder used to
   * introduce.
   */
  Bio->WriteBlocks (Bio, Bio->Media->MediaId, 0, sizeof (mGpuProbeLog), mGpuProbeLog);
}
/*
 * CHIRON_GPU_REGISTER_PROBE
 *
 * Reads the A540's registers from the firmware and prints them, without writing any of
 * them. Six layers of the port agree with the MSM8998 kernel sources; this is the first
 * thing that can disagree with the phone.
 *
 * Read-only on purpose. Bringing the GPU up from here, while Android's kgsl may still hold
 * the block, is a step that hangs with nothing to show for it.
 *
 * Runs in five stages, each flushed to the oops partition before the next begins, so a run
 * that stops still leaves behind how far it got. The stages are ordered by risk, not by
 * tidiness: GPUCC was already in the UEFI memory map before this port touched anything, so
 * it goes first, and the GPU block and SMMU -- the two windows that had to be added -- come
 * after, where their failure cannot hide the values from the block that was always mapped.
 *
 * Every offset is a word offset from a5xx_reg.h multiplied by four. They come from the set
 * the kernel reads in adreno_a5xx.c around line 104, the function that dumps device state
 * once something has gone wrong, rather than from what looked plausible. An earlier version
 * of this probe read GPU_REVISION/CHIP_ID/PRODUCT_ID at 0x05000000 and RBBM_STATUS at 0x7C;
 * none of those are registers this part uses.
 */
STATIC
UINT32
GpuRead32 (
  IN UINT64 Address
  )
{
  return *(volatile UINT32 *)(UINTN)Address;
}

#define GPU_REG(word)   (0x05000000ULL + (UINT64)(word) * 4ULL)
#define GPUCC_REG(off)  (0x05065000ULL + (UINT64)(off))

STATIC
VOID
ChironGpuRegisterProbe (
  VOID
  )
{
  UINT64  El;
  UINT32  A;
  UINT32  B;
  UINT32  C;
  UINT32  D;
  UINT32  E;

  __asm__ volatile ("mrs %0, CurrentEL" : "=r" (El));

  GpuProbeLog ("==== A540 GPU probe (read only), CurrentEL=%lx ====\n", El);

  /* ---- stage 1: GPUCC, the block that was always mapped ---- */
  GpuProbeLog ("stage 1: GPUCC\n");
  A = GpuRead32 (GPUCC_REG (0x01004));   /* CX GDSCR  */
  B = GpuRead32 (GPUCC_REG (0x01094));   /* GX GDSCR  */
  C = GpuRead32 (GPUCC_REG (0x01090));   /* GX BCR    */
  D = GpuRead32 (GPUCC_REG (0x00130));   /* GX domain */
  E = GpuRead32 (GPUCC_REG (0x01098));   /* gfx3d gate */
  GpuProbeLog ("  CX_GDSCR=%08x GX_GDSCR=%08x BCR=%08x DOMAIN=%08x GATE=%08x\n",
               A, B, C, D, E);
  A = GpuRead32 (GPUCC_REG (0x01020));   /* CXO gate  */
  B = GpuRead32 (GPUCC_REG (0x01070));   /* gfx3d RCGR */
  GpuProbeLog ("  CXO_CBCR=%08x GFX3D_RCGR=%08x\n", A, B);
  GpuProbeFlush ();

  /* ---- stage 2: GPU status, the first read into the added window ---- */
  GpuProbeLog ("stage 2: GPU block status\n");
  A = GpuRead32 (GPU_REG (0x4F5));       /* RBBM_STATUS       */
  B = GpuRead32 (GPU_REG (0x530));       /* RBBM_STATUS3      */
  C = GpuRead32 (GPU_REG (0x4E1));       /* RBBM_INT_0_STATUS */
  D = GpuRead32 (GPU_REG (0xB1A));       /* CP_HW_FAULT       */
  GpuProbeLog ("  RBBM_STATUS=%08x STATUS3=%08x INT_0=%08x CP_HW_FAULT=%08x\n",
               A, B, C, D);
  GpuProbeFlush ();

  /* ---- stage 3: GMU ---- */
  GpuProbeLog ("stage 3: GMU\n");
  A = GpuRead32 (GPU_REG (0xA8D8));      /* CM3_SYSRESET */
  B = GpuRead32 (GPU_REG (0xA8E0));      /* GENERAL_0    */
  C = GpuRead32 (GPU_REG (0xA8E1));      /* GENERAL_1    */
  D = GpuRead32 (GPU_REG (0xA8D6));      /* RBBM_INTR    */
  GpuProbeLog ("  SYSRESET=%08x GENERAL_0=%08x GENERAL_1=%08x RBBM_INTR=%08x\n",
               A, B, C, D);
  GpuProbeLog ("  (GENERAL_0 == BABEFACE means the GMU firmware is running)\n");
  GpuProbeFlush ();

  /* ---- stage 4: CP ring ---- */
  GpuProbeLog ("stage 4: CP ring\n");
  A = GpuRead32 (GPU_REG (0x800));       /* RB_BASE  */
  B = GpuRead32 (GPU_REG (0x802));       /* RB_CNTL  */
  C = GpuRead32 (GPU_REG (0x806));       /* RB_RPTR  */
  D = GpuRead32 (GPU_REG (0x807));       /* RB_WPTR  */
  E = GpuRead32 (GPU_REG (0x832));       /* ME_CNTL  */
  GpuProbeLog ("  RB_BASE=%08x CNTL=%08x RPTR=%08x WPTR=%08x ME_CNTL=%08x\n",
               A, B, C, D, E);
  GpuProbeFlush ();

  /* ---- stage 5: SMMU context bank 0 ---- */
  GpuProbeLog ("stage 5: SMMU\n");
  A = GpuRead32 (0x05048000);            /* SCTLR */
  B = GpuRead32 (0x05048004);            /* ACTLR */
  C = GpuRead32 (0x05048020);            /* TTBR0 */
  D = GpuRead32 (0x05048030);            /* TTBCR */
  GpuProbeLog ("  SCTLR=%08x ACTLR=%08x TTBR0=%08x TTBCR=%08x\n", A, B, C, D);
  GpuProbeLog ("  (ACTLR == 70000000 means qcom,smmu-v2, not a stock SMMU-500)\n");
  GpuProbeFlush ();

  GpuProbeLog ("==== end of A540 GPU probe: all five stages completed ====\n");
  GpuProbeFlush ();

  /*
   * Only now, with everything already on disk, put it on the panel too. Print goes through
   * the UEFI console, which blits the framebuffer -- the slowest and most fragile thing
   * this function does, and no longer the only way the data can get out.
   */
  Print (L"GPU probe complete; log on the oops partition.\n");
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

  /*
   * CHIRON_REBOOT_INSTEAD_OF_SHUT_DOWN
   *
   * This used to stall ten seconds and then call ResetSystem(EfiResetShutdown), which on
   * chiron leaves the phone off and needing a physical press of the power button. Every
   * diagnostic run then cost a manual intervention, and worse, the panel went dark before
   * anything on it could be read.
   *
   * A cold reboot gets back to Android on its own, so a probe can be flashed, read, and
   * the phone is usable again without anyone touching it. EfiResetCold is asked for first;
   * if the platform refuses it the warm reset is tried, because on this port warm has been
   * the one that works and there is no reason to end in a dead loop if either succeeds.
   *
   * The stall stays, but shorter: long enough to read the panel, not long enough to make a
   * test cycle tedious.
   */
  gBS->Stall (5000000);

  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);

  // Neither reset returned, which should not happen. Fall through rather than hang
  // silently, so the dead loop is at least preceded by something on the panel.
  Print (L"Reset did not take effect; halting.\n");
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
