##
#  chiron: the files that go INTO the volume.
#
#  Every source module Mu-Silicium builds, then every prebuilt Qualcomm driver
#  this device ships. TLMMDxe matters: ButtonsDxe failed with
#    "Failed to locate TLMMProtocol, Status = (0xE)"
#  and TLMM is what publishes it.
##

  INF MdeModulePkg/Core/Dxe/DxeMain.inf
  INF MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  INF MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  INF MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  INF MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  INF MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  INF SiliciumPkg/Drivers/SimpleFbDxe/SimpleFbDxe.inf

  # never appears. This ordering is the one measured to work.
  # on every poll, which buries the kernel log. lisa's build does not report that
  # failure, and all four of its depex protocols are satisfiable here.


  # USB: the original chiron set, which is the state in which the kernel booted and put its
  # log on the panel. Nothing here is needed to start XNU -- the loader talks to the panel,
  # not to USB -- so the USB stack is kept minimal while the kernel is being brought up.
  # OnePlus 5 (cheeseburger) USB list, the only one measured to work on MSM8998.
  # Its four Qcom USB binaries are byte-identical to chiron's (md5 verified):
  #   UsbfnDwc3Dxe 7e007be2e8 / UsbConfigDxe e797e9f1d5 /
  #   UsbDeviceDxe dae9c331ec / UsbMsdDxe 26fc0dbc9f
  # so what is being adopted is the ORDER AND THE SET, not a foreign binary.
  # Deliberately absent: UsbInitDxe, UsbfnChgDxe, UsbTypeCDxe -- those are chiron-only and
  # are what emits "UsbConfigPortsQueryConnectionChange: Failed to open USB Type-C Protocol"
  # on every poll, burying the kernel log.

  INF MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
  INF MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  # Starts the secondary cores through PSCI, from the ARM_CORE_INFO table that
  # PeilessSec already publishes as gArmMpCoreInfoGuid. Depex is TRUE, so it always runs.
  # Without it there is no EFI_MP_SERVICES_PROTOCOL at all on this port.
  INF ArmPkg/Drivers/ArmPsciMpServicesDxe/ArmPsciMpServicesDxe.inf
  INF ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  INF ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf
  INF ArmPkg/Drivers/TimerDxe/TimerDxe.inf
  INF MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  INF MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  INF MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  INF EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
  INF EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  INF EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf
  INF EmbeddedPkg/SimpleTextInOutSerial/SimpleTextInOutSerial.inf
  INF MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf

  # OnePlus 5 ships these and chiron did not: the FAT driver and the Unicode
  # collation it needs, plus the device-path driver they are built on.
  # Without Fat the firmware has no filesystem at all.
  INF FatPkg/EnhancedFatDxe/Fat.inf
  INF MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  INF MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  INF MdeModulePkg/Universal/PrintDxe/PrintDxe.inf

  #
  # The loader asks for gEfiUsbDeviceProtocolGuid to offer a session over USB;

  INF MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  INF MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf

  INF Binaries/chiron/QcomPkg/Drivers/EnvDxe/EnvDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SmemDxe/SmemDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/DALSYSDxe/DALSYSDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/HWIODxe/HWIODxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ChipInfoDxe/ChipInfoDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/PlatformInfoDxe/PlatformInfoDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/TLMMDxe/TLMMDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/GLinkDxe/GLinkDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ULogDxe/ULogDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/NpaDxe/NpaDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ClockDxe/ClockDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SPMIDxe/SPMIDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/PmicDxe/PmicDxeLa.inf
  INF Binaries/chiron/QcomPkg/Drivers/AdcDxe/AdcDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ChargerExDxe/ChargerExDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/TsensDxe/TsensDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ASN1X509Dxe/ASN1X509Dxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/HashDxe/HashDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SecRSADxe/SecRSADxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/VerifiedBootDxe/VerifiedBootDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/LimitsDxe/LimitsDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/MdtpDxe/MdtpDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/QdssDxe/QdssDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SdccDxe/SdccDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/UFSDxe/UFSDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/DisplayDxe/DisplayDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ButtonsDxe/ButtonsDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/MpPowerDxe/MpPowerDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/I2CDxe/I2CDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SPIDxe/SPIDxe.inf
  # depex, and nothing else in the chiron bundle provides it, so without this module
  # in its depex, and nothing in Mu-Silicium installs that protocol: the driver could never
  # fastboot server failed with EFI_NOT_FOUND. lisa's build of the same driver does not
  # depend on it, and all 15 protocols it does need are satisfiable here.
  INF Binaries/chiron/QcomPkg/Application/QcomChargerApp/QcomChargerApp.inf
  INF Binaries/chiron/QcomPkg/Drivers/FontDxe/FontDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/RNGDxe/RngDxe.inf

  # USB device stack.  It lives at the END of the list on purpose:
  # DXE dispatches in file order, and these wait on PlatformInfoDxe, SmemDxe,
  # ChipInfoDxe and the arch protocols -- all of which are listed above.  Placed
  # earlier their depexes are never satisfied and gEfiUsbfnIoProtocolGuid is
  # never published, which is what "usb: no UsbFnIo protocol" meant.
  #
  # The order inside the group is the OnePlus 5's, the only one measured to work
  # on MSM8998.  UsbConfigDxe is last: it starts the controller, and it wants
  # the device protocol (UsbDeviceDxe) and the bus driver in place first.
  # chiron's own USB modules: the controller's depex wants what they publish
  INF Binaries/chiron/QcomPkg/Drivers/QcomChargerDxe/QcomChargerDxeLA.inf
  #
  # OnePlus 5's USB set and order -- the only one measured to work on MSM8998
  # (its four Qcom USB binaries are byte-identical to chiron's).  UsbConfigDxe is
  # LAST: it starts the controller, and it needs the device protocol from
  # UsbDeviceDxe plus the bus driver before it can connect to a host.
  # UsbInitDxe / UsbfnChgDxe / UsbTypeCDxe stay out: they are chiron-only and
  # poll for a USB Type-C protocol this build does not publish.
  # chiron has a USB Type-C port: UsbConfigDxe reads its state to decide a
  # connection, so these two are required even though OnePlus 5 omits them.
  INF Binaries/chiron/QcomPkg/Drivers/UsbTypeCDxe/UsbTypeCDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/UsbfnChgDxe/UsbfnChgDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/UsbfnDwc3Dxe/UsbfnDwc3Dxe.inf
  INF MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  INF MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  INF MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/UsbMsdDxe/UsbMsdDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/UsbDeviceDxe/UsbDeviceDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/UsbConfigDxe/UsbConfigDxe.inf
  # UsbConfigDxe installs E722B03F-B250-42CE-8EBD-5BD51812D037, which is exactly
  # what UsbInitDxe's depex waits for, so it must come after it.  UsbInitDxe is
  # what calls UsbInitStartController() and makes the gadget visible.
  INF Binaries/chiron/QcomPkg/Drivers/UsbInitDxe/UsbInitDxe.inf
  # CHIRON_EDK2_FASTBOOT_IN_THE_FV
  #
  # Same two modules as the DSC, and the reason is the patch this is copied from
  # (edk2-judypn/0001-judypn-usb-fastboot.patch):
  #
  #     INF EmbeddedPkg/Application/AndroidFastboot/AndroidFastbootApp.inf
  #
  # FastbootTransportUsbDxe publishes gAndroidFastbootTransportProtocolGuid on top of
  # gUsbDeviceProtocolGuid, and AndroidFastbootApp consumes it.  Both are stock EDK2, so if they run
  # here the fastboot server is the firmware's rather than the loader's.
  INF EmbeddedPkg/Drivers/AndroidFastbootTransportUsbDxe/FastbootTransportUsbDxe.inf
  INF EmbeddedPkg/Application/AndroidFastboot/AndroidFastbootApp.inf
