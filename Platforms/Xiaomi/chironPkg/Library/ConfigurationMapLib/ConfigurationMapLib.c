/*
 * chiron's configuration map: the handful of values the firmware's libraries read
 * before any device is up.
 *
 * Provenance, because a wrong address here is read before anything can report it:
 *   NumCpus / NumActiveCores / MaxCoreCnt   8 -- Android-chiron.dtb has cpu@0..3 and
 *                                           cpu@100..103, two clusters of four.
 *   NumCpusFuseAddr                         NOT KNOWN for msm8998. redwood's 0x5C04C
 *                                           is an SM7325 efuse address; writing
 *                                           another SoC's efuse offset here would read
 *                                           arbitrary silicon. Left out.
 *   SharedIMEMBaseAddr                      NOT KNOWN. redwood's 0x146AA000 is
 *                                           SM7325. msm8998 puts its shared IMEM
 *                                           elsewhere; the old edk2-msm8998 port
 *                                           does not carry this key at all, which is
 *                                           a hint that this firmware generation
 *                                           does not need it. Left out.
 *   EUDEnableAddr                           NOT KNOWN, and about a debug block this
 *                                           port does not use. Left out.
 *
 * Everything below is either generic (page pools, log size, flags whose meaning does
 * not depend on the SoC) or a count that the device tree confirms. Where a value
 * would have to be invented, the key is omitted rather than guessed: an absent key
 * means "the library's own default", which is recoverable; a wrong address is not.
 *
 * This list must grow as the build reports what it needs -- the build names a
 * missing key, but never complains about a wrong one.
 */
#include <Library/ConfigurationMapLib.h>

STATIC
EFI_CONFIGURATION_ENTRY_DESCRIPTOR
gConfigurationDescriptor[] = {
  // CPU: 8 cores in two clusters of four (Android-chiron.dtb cpu@0..3, cpu@100..103)
  {"NumCpus", 8},
  {"NumActiveCores", 8},
  {"MaxCoreCnt", 8},
  {"EarlyInitCoreCnt", 2},

  // Memory policy. Values as in the other device packages; they are policy, not
  // silicon addresses.
  {"InitialPagePoolCount", 0x700},
  {"UefiMemUseThreshold", 0xE1},
  {"EnableMultiThreading", 1},
  {"EnableMultiCoreFvDecompression", 1},

  // Logs
  {"MaxLogFileSize", 0x400000},

  // Boot behaviour
  {"EnableShell", 0x1},
  {"PwrBtnShutdownFlag", 0x0},
  {"EnableDisplayThread", 0x0},
  {"EnableDisplayImageFv", 0x1},
  {"EnableACPIFallback", 0x0},

  // Storage: chiron is UFS 2.1
  {"EnableUfsIOC", 1},
  {"UEFIExitUfsSSURequired", 1},

  // USB
  {"USBHS1_Config", 0x0},
  {"UsbFnIoRevNum", 0x00010001},

  // Variables
  {"EnableVariablePolicyEngine", 0},
  {"AllowNonPersistentVarsInRetail", 0x1},
  {"EnableUEFISecAppDebugLogDump", 0x0},

  // Secure-world size: SM7325 uses 0xA00000; msm8998's TZ carveout is smaller and
  // this value only bounds the shared bridge, so it is left at the smaller default
  // rather than copied.
  {"ShmBridgememSize", 0x800000},

  // Kept at 0: the old port does not enable it, and it needs a DDR info source
  // this platform has not got.
  {"DDRInfoNotifyFlag", 0x0}
};

VOID
GetConfigurationMap (
  OUT EFI_CONFIGURATION_ENTRY_DESCRIPTOR **ConfigurationDescriptor,
  OUT UINT8                               *ConfigurationDescriptorCount
  )
{
  *ConfigurationDescriptor      = gConfigurationDescriptor;
  *ConfigurationDescriptorCount = ARRAY_SIZE (gConfigurationDescriptor);
}
