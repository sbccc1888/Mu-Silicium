##
#  Xiaomi Mi MIX 2 (chiron) -- Qualcomm MSM8998 / Snapdragon 835
#
#  Modelled on redwoodPkg (which follows lisa). The SoC package is
#  Silicon/Qualcomm/Msm8998Pkg, written for this port because Mu-Silicium has no
#  msm8998 support: its 26 SoC packages start at SM6xxx.
#
#  Values that differ from redwood, and where each comes from:
#    PcdSystemMemoryBase   0x40000000   Android-chiron.dtb "memory" reg
#    stack                 0x9FE00000   inside chiron's DDR, above the DXE heap
#    FB                    1080x2160    the panel, and what the old port used
#    PcdPlatformType       "LA"         msm8998 ships the LA (Linux Android) TZ
##
[Defines]
  PLATFORM_NAME                  = chiron
  PLATFORM_GUID                  = 3F8A61D4-2E57-4B9C-A1D3-6C0B5E7F9248
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/chironPkg
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = RELEASE|DEBUG
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = chironPkg/chiron.fdf
  USE_CUSTOM_DISPLAY_DRIVER      = 0

  #
  # MSM8998 has no variants in this port.
  #
  SOC_TYPE                       = 0

!include Msm8998Pkg/Msm8998Pkg.dsc.inc

[PcdsFixedAtBuild]
  #
  # DDR Memory: Android-chiron.dtb memory reg = <0x00 0x40000000 0x00 0xc0000000 ...>
  #
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000

  #
  # UEFI Stack: inside the DDR the map calls "UEFI FD" at 0x9FD00000.
  #
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x9FE00000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x40000

  #
  # SMBIOS
  #
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemManufacturer|"Xiaomi"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemModel|"Mi MIX 2"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailModel|"chiron"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailSku|"Mi_MIX_2_chiron"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemBoardModel|"Mi MIX 2"

  #
  # Simple Frame Buffer: the 5.99" 1080x2160 panel.
  #
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferWidth|1080
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferHeight|2160
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferColorDepth|32

  #
  # Platform PEI: msm8998 uses the LA TzDxe (TzDxeLA.efi is in its Binaries).
  #
  gQcomPkgTokenSpaceGuid.PcdPlatformType|"LA"

  #
  # Storage: chiron is UFS 2.1, not 3.0.
  #
  gQcomPkgTokenSpaceGuid.PcdStorageIsUfs3|FALSE

[LibraryClasses]
  #
  # Memory Libraries
  #
  MemoryMapLib|chironPkg/Library/MemoryMapLib/MemoryMapLib.inf

  #
  # QCOM Libraries -- PlatformPeiLib consumes this before any device is up.
  #
  ConfigurationMapLib|chironPkg/Library/ConfigurationMapLib/ConfigurationMapLib.inf

[Components]
