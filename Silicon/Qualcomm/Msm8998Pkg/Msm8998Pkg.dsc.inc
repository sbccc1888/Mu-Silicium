##
#  Microsoft Mu / Project Silicium -- Qualcomm Snapdragon 835 (MSM8998)
#
#  Modelled on StarlordPkg, the smallest existing SoC package: this file carries
#  the SoC's own numbers, and QcomPkg supplies every driver.
#
#  MSM8998 is ARMv8.0-A. Every other package in this tree builds with
#  -march=armv8-a+crypto+rcpc, and rcpc (LDAPR/STLR unordered) is an ARMv8.3
#  feature that does not exist on this core: it would be an illegal instruction at
#  run time, not a build error. So the arch line here drops rcpc. Crypto is kept
#  out of the baseline too -- the Kryo 280 cores have it, but nothing here needs it
#  and leaving it out keeps the image runnable if a variant lacks it.
#
#  Addresses: GIC and timer from the phone's own device tree
#  (Platforms/Msm8998Pkg/Device/xiaomi/chiron/DeviceTreeBlob/Android/Android-chiron.dtb,
#  arm,gic-v3 reg = <0x17a00000 0x10000 0x17b00000 0x100000>).
##
[Defines]
  #
  # Timer: MSM8998 has no virtual-timer offset workaround; the generic timer is
  # readable from EL1 directly.
  #
  USE_PHYSICAL_TIMER                         = 1

  #
  # KEP errata. These are per-core facts, not per-SoC guesses:
  #   ACTLR_EL1 unimplemented  -- Kryo 280 implements ACTLR_EL1, so 0.
  #   AMCNTENSET0_EL0          -- not used by this tree's timer path, 0.
  #   GICv3 without IRM flag   -- MSM8998's GIC-500 predates the IRM flag, so 1.
  #   PSCI MEM_PROTECT failing -- kept at 1 to match the other packages until a
  #                               phone says otherwise; it only affects a call
  #                               this firmware does not make on the boot path.
  #
  HAS_ACTLR_EL1_UNIMPLEMENTED_ERRATA         = 0
  HAS_AMCNTENSET0_EL0_UNIMPLEMENTED_ERRATA   = 0
  HAS_GIC_V3_WITHOUT_IRM_FLAG_SUPPORT_ERRATA = 1
  HAS_PSCI_MEMPROTECT_FAILING_ERRATA         = 1

!include QcomPkg/QcomPkg.dsc.inc

[BuildOptions]
  # No +rcpc: ARMv8.3, absent on Kryo 280 (ARMv8.0-A).
  *_*_AARCH64_CC_FLAGS = -march=armv8-a

[PcdsFixedAtBuild]
  #
  # ACPI
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemTableId|0x38393838394D53
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemRevision|0x660

  #
  # CPU: 8 cores, 2 clusters (4x Kryo 280 Gold + 4x Kryo 280 Silver)
  #
  gArmPlatformTokenSpaceGuid.PcdCoreCount|8
  gArmPlatformTokenSpaceGuid.PcdClusterCount|2

  #
  # GIC: from Android-chiron.dtb, arm,gic-v3 reg
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x17A00000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x17B00000

  #
  # Timer interrupts: PPI 17 (secure) and 18 (non-secure) are the architectural
  # assignments; MSM8998 does not remap them.
  #
  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|17
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|18

  #
  # SMBIOS
  #
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosProcessorModel|"Snapdragon (TM) 835 @ 2.45 GHz"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosProcessorPartNumber|"MSM8998"
