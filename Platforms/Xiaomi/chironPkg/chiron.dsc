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
  # The MMU covers Base + Size. MemoryInitPeiLib asserts Size != 0 and computes
  # SystemMemoryTop = Base + Size; with the default 0 nothing above 0x40000000 was
  # mapped, and the loader's dc civac at 0x10f0000000 (4.23 GiB) took a translation
  # fault (ESR 0x96000145). This phone's device tree gives the RAM as
  #   0x40000000 + 0xC0000000, then 0x100000000 + 0xBD8C0000
  # ending at 0x1BD8C0000, which is what this declares.
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x17D8C0000

  #
  # CPU stack. Must be the same span MemoryMapLib calls "UEFI Stack", and must not
  # fall inside "UEFI FD" (0x9FC00000+0x300000): this PCD said 0x9FE00000, which is
  # the middle of the firmware image, so the first push would have overwritten the
  # code it was running.
  #
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x9FF90000
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
  #
  # MP services: starts the secondary cores through PSCI, from the ARM_CORE_INFO table
  # PeilessSec publishes as gArmMpCoreInfoGuid. Listed in DXE.inc (the FDF side) too --
  # EDK2 needs the module in both places.
  #
  ArmPkg/Drivers/ArmPsciMpServicesDxe/ArmPsciMpServicesDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
    Binaries/chiron/QcomPkg/Drivers/QcomChargerDxe/QcomChargerDxeLA.inf
    MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
    MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
    MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
    Binaries/chiron/QcomPkg/Drivers/UsbfnChgDxe/UsbfnChgDxe.inf
    Binaries/chiron/QcomPkg/Drivers/UsbTypeCDxe/UsbTypeCDxe.inf
    Binaries/chiron/QcomPkg/Drivers/UsbfnDwc3Dxe/UsbfnDwc3Dxe.inf
    MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
    MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
    MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
    Binaries/chiron/QcomPkg/Drivers/UsbMsdDxe/UsbMsdDxe.inf
    Binaries/chiron/QcomPkg/Drivers/UsbDeviceDxe/UsbDeviceDxe.inf
    Binaries/chiron/QcomPkg/Drivers/UsbConfigDxe/UsbConfigDxe.inf
    Binaries/chiron/QcomPkg/Drivers/UsbInitDxe/UsbInitDxe.inf
    #
    # CHIRON_REGISTER_THE_STANDARD_FASTBOOT_APP
    #
    # Mi 6's EDK2 port does fastboot by registering EDK2's own application and letting the stock
    # USB stack serve it (edk2-judypn/0001-judypn-usb-fastboot.patch):
    #
    #     [Components.common]
    #       EmbeddedPkg/Application/AndroidFastboot/AndroidFastbootApp.inf
    #
    # and the matching INF line in the FV.  This firmware has never built it, so the only fastboot
    # server here has been the loader's own -- which is why everything about USB lives in loader.c
    # instead of in the stack that already knows how to do it.
    #
    # AndroidFastbootApp needs a transport; FastbootTransportUsbDxe publishes
    # gAndroidFastbootTransportProtocolGuid on top of gUsbDeviceProtocolGuid, which is the protocol
    # this firmware already produces.  Both are EDK2 modules, so the whole chain is generic.
    #
    EmbeddedPkg/Application/AndroidFastboot/AndroidFastbootApp.inf
    EmbeddedPkg/Drivers/AndroidFastbootTransportUsbDxe/FastbootTransportUsbDxe.inf
    #
    # OnePlus 5 build that was measured to work on MSM8998 (md5-identical files).
    #
    # UsbDeviceDxe is the one that installs gEfiUsbDeviceProtocolGuid.  The loader
    # asks for that protocol when it offers a session over USB; without it it
    # prints "usb: device protocol not found" and gives up, so a system image can
    # never be transferred and the phone cannot be driven over USB at all.
    #
    # UsbInitDxe, UsbfnChgDxe and UsbTypeCDxe stay out: they poll for a USB Type-C
    # protocol this firmware does not publish, which buries the kernel log.
    #

