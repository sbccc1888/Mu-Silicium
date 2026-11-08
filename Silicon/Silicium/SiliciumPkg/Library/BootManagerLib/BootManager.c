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
  OUT EFI_BLOCK_IO_PROTOCOL  **OutBlockIo,
  OUT UINT64                  *OutFirstLba
  )
{
  EFI_STATUS                  Status;
  EFI_HANDLE                 *Handles;
  UINTN                       Count;
  UINTN                       Index;
  STATIC UINT8                Table[64 * 1024] __attribute__ ((aligned (8)));
  STATIC UINT8                Header[4096] __attribute__ ((aligned (8)));
  /*
   * oops, not logfs. logfs is a FAT12 volume the phone's own UEFI owns, and writing raw at
   * its LBA 0 would wreck the boot sector and the first FAT -- on every automatic run.
   * "oops" is the raw 16 MiB panic-log area Android keeps for exactly this and never reads
   * during a normal boot, which is why xnuloader falls back to it too.
   */
  CONST CHAR16                Want[] = L"oops";

  *OutBlockIo = NULL;
  *OutFirstLba = 0;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &Count,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < Count; Index++) {
    EFI_BLOCK_IO_PROTOCOL  *Bio;
    UINT64                  BlockSize;
    UINT64                  EntriesLba;
    UINT64                  EntryCount;
    UINT64                  EntrySize;
    UINT64                  Bytes;
    UINT64                  Read;
    UINT32                  I;
    UINT64                  E;

    Status = gBS->HandleProtocol (Handles[Index], &gEfiBlockIoProtocolGuid, (VOID **)&Bio);
    if (EFI_ERROR (Status) || (Bio == NULL) || (Bio->Media == NULL) || Bio->Media->LogicalPartition) {
      continue;
    }

    BlockSize = Bio->Media->BlockSize;
    if ((BlockSize < 512) || (BlockSize > 4096)) {
      continue;
    }

    Status = Bio->ReadBlocks (Bio, Bio->Media->MediaId, 1, BlockSize, Header);
    if (EFI_ERROR (Status)) {
      continue;
    }

    /* "EFI PART" -- a protective MBR sits at LBA 0, the GPT header at LBA 1. */
    if ((Header[0] != 'E') || (Header[1] != 'F') || (Header[2] != 'I') || (Header[3] != ' ')) {
      continue;
    }

    EntriesLba = *(UINT64 *)(Header + 72);
    I          = *(UINT32 *)(Header + 80);
    EntryCount = I;
    I          = *(UINT32 *)(Header + 84);
    EntrySize  = I;
    Bytes      = EntryCount * EntrySize;
    if ((EntrySize < 128) || (Bytes == 0) || (Bytes > sizeof (Table))) {
      continue;
    }

    Read   = (Bytes + BlockSize - 1) / BlockSize * BlockSize;
    Status = Bio->ReadBlocks (Bio, Bio->Media->MediaId, EntriesLba, Read, Table);
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (E = 0; E < EntryCount; E++) {
      CONST UINT8   *Entry = Table + E * EntrySize;
      CONST CHAR16  *Name  = (CONST CHAR16 *)(Entry + 56);
      UINTN          K;

      for (K = 0; (K < 36) && (Want[K] != 0); K++) {
        if (Name[K] != Want[K]) {
          break;
        }
      }

      if ((Want[K] == 0) && (Name[K] == 0)) {
        *OutFirstLba = *(UINT64 *)(Entry + 32);
        *OutBlockIo  = Bio;
        FreePool (Handles);
        return EFI_SUCCESS;
      }
    }
  }

  FreePool (Handles);

  /*
   * No oops partition. logfs is the only other candidate and it is a live FAT12 volume; a
   * raw write there damages firmware-owned data. Rather than do that quietly, report and
   * write nothing -- a missing log is recoverable, a wrecked volume is not.
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
  UINT64                  FirstLba;
  EFI_STATUS              Status;

  if (mGpuProbeLogLen == 0) {
    return;
  }

  Status = GpuProbeFindLogPartition (&Bio, &FirstLba);
  if (EFI_ERROR (Status)) {
    Print (L"GPU probe: no oops partition (logfs is FAT12 and is left alone), %r\n", Status);
    return;
  }

  Status = Bio->WriteBlocks (Bio, Bio->Media->MediaId, FirstLba, sizeof (mGpuProbeLog), mGpuProbeLog);
  Print (L"GPU probe: log written to oops LBA %lx, %r\n", FirstLba, Status);
}
/*
 * CHIRON_GPU_REGISTER_PROBE
 *
 * Reads the A540's registers from the firmware and prints them, without writing any of
 * them. Six layers of the port agree with the MSM8998 kernel sources; this is the first
 * thing that can disagree with the phone.
 *
 * Read-only on purpose. Bringing the GPU up from here, while Android's kgsl may still hold
 * the block, is the kind of step that hangs with nothing to show for it.
 *
 * Every offset is a word offset from a5xx_reg.h multiplied by four, and they were taken
 * from the set the kernel itself reads in adreno_a5xx.c around line 104 -- the function
 * that dumps device state once something has gone wrong -- rather than picked for looking
 * plausible. An earlier version of this probe read GPU_REVISION/CHIP_ID/PRODUCT_ID at
 * 0x05000000 and RBBM_STATUS at 0x7C. None of those are registers this part uses: the
 * kernel takes the GPU identity from the device tree, and RBBM_STATUS is word 0x4F5.
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
  UINT32  Status;
  UINT32  Status3;
  UINT32  Int0;
  UINT32  HwFault;
  UINT32  Cx;
  UINT32  Gx;
  UINT32  Bcr;
  UINT32  Domain;
  UINT32  Gate;
  UINT32  Cxo;
  UINT32  Rcgr;
  UINT32  Sysreset;
  UINT32  Gen0;
  UINT32  Gen1;
  UINT32  GpmuIntr;
  UINT32  CpBase;
  UINT32  CpCntl;
  UINT32  CpRptr;
  UINT32  CpWptr;
  UINT32  MeCntl;
  UINT32  SmmuSctlr;
  UINT32  SmmuActlr;
  UINT32  SmmuTtbr0;
  UINT32  SmmuTtbcr;

  __asm__ volatile ("mrs %0, CurrentEL" : "=r" (El));

  /*
   * Read everything once into locals before printing. Interleaving reads with Print calls
   * means a fault in the fourth read cannot be told apart from a hang in the third print.
   */

  GpuProbeLog ("probe: reading Status\n");  Status   = GpuRead32 (GPU_REG (0x4F5));

  GpuProbeLog ("probe: reading Status3\n");  Status3  = GpuRead32 (GPU_REG (0x530));

  GpuProbeLog ("probe: reading Int0\n");  Int0     = GpuRead32 (GPU_REG (0x4E1));

  GpuProbeLog ("probe: reading HwFault\n");  HwFault  = GpuRead32 (GPU_REG (0xB1A));


  GpuProbeLog ("probe: reading Cx\n");  Cx       = GpuRead32 (GPUCC_REG (0x01004));

  GpuProbeLog ("probe: reading Gx\n");  Gx       = GpuRead32 (GPUCC_REG (0x01094));

  GpuProbeLog ("probe: reading Bcr\n");  Bcr      = GpuRead32 (GPUCC_REG (0x01090));

  GpuProbeLog ("probe: reading Domain\n");  Domain   = GpuRead32 (GPUCC_REG (0x00130));

  GpuProbeLog ("probe: reading Gate\n");  Gate     = GpuRead32 (GPUCC_REG (0x01098));

  GpuProbeLog ("probe: reading Cxo\n");  Cxo      = GpuRead32 (GPUCC_REG (0x01020));

  GpuProbeLog ("probe: reading Rcgr\n");  Rcgr     = GpuRead32 (GPUCC_REG (0x01070));


  GpuProbeLog ("probe: reading Sysreset\n");  Sysreset = GpuRead32 (GPU_REG (0xA8D8));

  GpuProbeLog ("probe: reading Gen0\n");  Gen0     = GpuRead32 (GPU_REG (0xA8E0));

  GpuProbeLog ("probe: reading Gen1\n");  Gen1     = GpuRead32 (GPU_REG (0xA8E1));

  GpuProbeLog ("probe: reading GpmuIntr\n");  GpmuIntr = GpuRead32 (GPU_REG (0xA8D6));


  GpuProbeLog ("probe: reading CpBase\n");  CpBase   = GpuRead32 (GPU_REG (0x800));

  GpuProbeLog ("probe: reading CpCntl\n");  CpCntl   = GpuRead32 (GPU_REG (0x802));

  GpuProbeLog ("probe: reading CpRptr\n");  CpRptr   = GpuRead32 (GPU_REG (0x806));

  GpuProbeLog ("probe: reading CpWptr\n");  CpWptr   = GpuRead32 (GPU_REG (0x807));

  GpuProbeLog ("probe: reading MeCntl\n");  MeCntl   = GpuRead32 (GPU_REG (0x832));

  SmmuSctlr = GpuRead32 (0x05048000);
  SmmuActlr = GpuRead32 (0x05048004);
  SmmuTtbr0 = GpuRead32 (0x05048020);
  SmmuTtbcr = GpuRead32 (0x05048030);

  /*
   * Write what was read before printing any of it. The panel path goes through the UEFI
   * console, which blits the framebuffer and is the slowest thing here; if it faults
   * there is still a complete file.
   */
  GpuProbeFlush ();

  /* ---- now say it, on the panel and on disk ---- */
  Print (L"==== A540 GPU probe (read only), CurrentEL=%lx ====\n", El);
  GpuProbeLog ("==== A540 GPU probe (read only), CurrentEL=%lx ====\n", El);

  Print (L"RBBM_STATUS=%08x STATUS3=%08x INT_0=%08x CP_HW_FAULT=%08x\n",
         Status, Status3, Int0, HwFault);
  GpuProbeLog ("RBBM_STATUS=%08x STATUS3=%08x INT_0=%08x CP_HW_FAULT=%08x\n",
               Status, Status3, Int0, HwFault);

  Print (L"CX_GDSCR=%08x (collapse %u) GX_GDSCR=%08x (collapse %u)\n",
         Cx, Cx & 1u, Gx, Gx & 1u);
  GpuProbeLog ("CX_GDSCR=%08x (collapse %u) GX_GDSCR=%08x (collapse %u)\n",
               Cx, Cx & 1u, Gx, Gx & 1u);

  Print (L"GPU_GX_BCR=%08x GX_DOMAIN=%08x\n", Bcr, Domain);
  GpuProbeLog ("GPU_GX_BCR=%08x GX_DOMAIN=%08x\n", Bcr, Domain);

  Print (L"GFX3D_CBCR=%08x (on %u) CXO_CBCR=%08x GFX3D_RCGR=%08x\n",
         Gate, Gate & 1u, Cxo, Rcgr);
  GpuProbeLog ("GFX3D_CBCR=%08x (on %u) CXO_CBCR=%08x GFX3D_RCGR=%08x\n",
               Gate, Gate & 1u, Cxo, Rcgr);

  Print (L"GPMU_SYSRESET=%08x GENERAL_0=%08x GENERAL_1=%08x RBBM_INTR=%08x\n",
         Sysreset, Gen0, Gen1, GpmuIntr);
  GpuProbeLog ("GPMU_SYSRESET=%08x GENERAL_0=%08x GENERAL_1=%08x RBBM_INTR=%08x\n",
               Sysreset, Gen0, Gen1, GpmuIntr);

  Print (L"CP_RB_BASE=%08x CNTL=%08x RPTR=%08x WPTR=%08x ME_CNTL=%08x\n",
         CpBase, CpCntl, CpRptr, CpWptr, MeCntl);
  GpuProbeLog ("CP_RB_BASE=%08x CNTL=%08x RPTR=%08x WPTR=%08x ME_CNTL=%08x\n",
               CpBase, CpCntl, CpRptr, CpWptr, MeCntl);

  Print (L"SMMU_SCTLR=%08x (M %u) ACTLR=%08x (expect 70000000) TTBR0=%08x TTBCR=%08x\n",
         SmmuSctlr, SmmuSctlr & 1u, SmmuActlr, SmmuTtbr0, SmmuTtbcr);
  GpuProbeLog ("SMMU_SCTLR=%08x (M %u) ACTLR=%08x (expect 70000000) TTBR0=%08x TTBCR=%08x\n",
               SmmuSctlr, SmmuSctlr & 1u, SmmuActlr, SmmuTtbr0, SmmuTtbcr);

  Print (L"==== end of A540 GPU probe ====\n");
  GpuProbeLog ("==== end of A540 GPU probe ====\n");
  GpuProbeFlush ();
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
