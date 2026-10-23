##
#  chiron: the order the firmware brings its drivers up.
#
#  Every line names an INF that exists under Binaries/chiron -- this list is
#  filtered against that directory, so a driver this firmware does not carry
#  is left out instead of stopping GenFds with "is not found in packages path".
#  The .inf files in Binaries are package-relative (QcomPkg/Drivers/...); the
#  path here says which package to resolve it from.
##
APRIORI DXE {
  INF Binaries/chiron/MdeModulePkg/Core/Dxe/DxeMain.inf
  INF Binaries/chiron/MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  INF Binaries/chiron/MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  INF Binaries/chiron/MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  INF Binaries/chiron/ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  INF Binaries/chiron/ArmPkg/Drivers/ArmGic/ArmGicDxe.inf
  INF Binaries/chiron/EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
  INF Binaries/chiron/ArmPkg/Drivers/TimerDxe/TimerDxe.inf
  INF Binaries/chiron/EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/EnvDxe/EnvDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ChipInfoDxe/ChipInfoDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/PlatformInfoDxe/PlatformInfoDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SecurityDxe/SecurityDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ASN1X509Dxe/ASN1X509Dxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/HashDxe/HashDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SecRSADxe/SecRSADxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/TzDxe/TzDxeLA.inf
  INF Binaries/chiron/QcomPkg/Drivers/VerifiedBootDxe/VerifiedBootDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/LimitsDxe/LimitsDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SPMIDxe/SPMIDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/PmicDxe/PmicDxeLa.inf
  INF Binaries/chiron/QcomPkg/Drivers/QcomChargerDxe/QcomChargerDxeLA.inf
  INF Binaries/chiron/QcomPkg/Drivers/AdcDxe/AdcDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/TsensDxe/TsensDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/DALSYSDxe/DALSYSDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/HWIODxe/HWIODxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/GLinkDxe/GLinkDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SmemDxe/SmemDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ULogDxe/ULogDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/NpaDxe/NpaDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ClockDxe/ClockDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/I2CDxe/I2CDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SPIDxe/SPIDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/SdccDxe/SdccDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/MdtpDxe/MdtpDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/RNGDxe/RngDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/QdssDxe/QdssDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/DisplayDxe/DisplayDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/ButtonsDxe/ButtonsDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/UFSDxe/UFSDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/UsbConfigDxe/UsbConfigDxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/UsbfnDwc3Dxe/UsbfnDwc3Dxe.inf
  INF Binaries/chiron/QcomPkg/Drivers/UsbDeviceDxe/UsbDeviceDxe.inf
}
