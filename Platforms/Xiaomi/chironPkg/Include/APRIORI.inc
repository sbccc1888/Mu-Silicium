##
#  chiron: dispatch ORDER only.
#
#  APRIORI DXE records the order in which modules are dispatched. It does NOT add
#  them to the volume -- DXE.inc does that. Every INF here must therefore also be
#  in DXE.inc, and must never be written outside this block: a bare INF inside
#  [FV.FvMain] adds a second copy of the file and GenFv rejects the duplicate GUID.
##
APRIORI DXE {
  INF MdeModulePkg/Core/Dxe/DxeMain.inf
  INF MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  INF MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  INF MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  INF MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  INF ArmPkg/Drivers/ArmPsciMpServicesDxe/ArmPsciMpServicesDxe.inf
  INF ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  INF ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf
  INF ArmPkg/Drivers/TimerDxe/TimerDxe.inf
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
}
