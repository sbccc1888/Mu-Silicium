/*
 * chiron's memory map.
 *
 * Taken from the firmware that actually brings this handset up:
 *   edk2-msm8998  Silicon/QC/Msm8998/QcomPkg/Library/PlatformMemoryMapLib/
 *                 PlatformMemoryMapLib.c
 * translated field by field. Every address below was already in use by firmware that
 * boots on chiron; nothing here is derived.
 *
 * Translation, and nothing else:
 *   Mem4G / Mem6G / Mem8G  ->  AddMem
 *     The old tree picks a RAM size at runtime with those; Mu-Silicium's
 *     EFI_MEMORY_HOB_OPTION has no equivalent and AddMem adds the same HOB.
 *   WRITE_THROUGH  ->  WRITE_THROUGH_XN
 *     WRITE_THROUGH is not defined in Silicon/Silicium/SiliciumPkg/Include/Library/
 *     MemoryMapLib.h.
 *   the 8 GiB "RAM Partition" pair is dropped: this handset's own device tree gives
 *     0x40000000 + 0xC0000000 then 0x100000000 + 0xBD8C0000, which is the 6 GiB set.
 *   the LG_PIL_FIXED branch of the reference is dropped; the plain one is used.
 *
 * The layout matters as much as the numbers. SEC takes its own stack, heap and page
 * tables from this table, so they must be memory the firmware really owns -- the block
 * from 0x9F800000 upward. An earlier version of this file put "UEFI Stack" at
 * 0x40000000, 1.5 GB below the image, and the firmware died as soon as it used it.
 *
 * Three numbers appear in more than one file and must agree:
 *   UEFI FD         0x9FC00000 / 0x300000  = chiron.fdf FD_BASE/FD_SIZE
 *                                         = chiron.toml base/size
 *                                         = where the boot shim copies the image
 *   Display Reserved 0x9D400000 / 0x2400000 = chiron.conf FB_BASE
 */
#include <Library/MemoryMapLib.h>

STATIC
EFI_MEMORY_REGION_DESCRIPTOR
gMemoryDescriptor[] = {
  // Name, Address, Length, HobOption, ResourceType, ResourceAttribute, MemoryType, ArmAttribute

  {"RAM Partition",     0x80000000,  0x01AC0000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Conv,    WRITE_BACK_XN},
  {"HLOS 1",            0x81AC0000,  0x03C40000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    BsData,  WRITE_BACK_XN},
  {"Hypervisor",        0x85800000,  0x00600000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Reserv,  NS_DEVICE},
  {"MPSS_EFS",          0x85E00000,  0x00200000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Reserv,  NS_DEVICE},
  {"SMEM",              0x86000000,  0x00200000,  AddMem,                MEM_RES,  UNCACHEABLE,    Reserv,  UNCACHED_UNBUFFERED_XN},
  {"TZ",                0x86200000,  0x00B00000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Reserv,  NS_DEVICE},
  {"TZApps",            0x86D00000,  0x02200000,  NoHob,                 SYS_MEM,  SYS_MEM_CAP,    Reserv,  NS_DEVICE},
  {"HLOS 2",            0x88F00000,  0x01C00000,  NoHob,                 SYS_MEM,  SYS_MEM_CAP,    Reserv,  NS_DEVICE},
  {"PIL_REGION",        0x8AB00000,  0x0B315000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Reserv,  UNCACHED_UNBUFFERED_XN},
  {"HLOS 3",            0x95E15000,  0x01DEB000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Conv,    WRITE_BACK_XN},
  {"Display Reserved",  0x9D400000,  0x02400000,  AddMem,                MEM_RES,  WRITE_THROUGH_XN, MaxMem,  WRITE_THROUGH_XN},
  {"DBI Dump",          0x9D330000,  0x000D0000,  NoHob,                 MMAP_IO,  INITIALIZED,    Conv,    NS_DEVICE},
  {"DXE Heap",          0x97C00000,  0x05730000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Conv,    WRITE_BACK},
  {"FV Region",         0x9F800000,  0x00200000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    BsData,  WRITE_BACK_XN},
  {"ABOOT FV",          0x9FA00000,  0x00200000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Reserv,  WRITE_BACK_XN},
  {"UEFI FD",           0x9FC00000,  0x00300000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    BsData,  WRITE_BACK},
  {"SEC Heap",          0x9FF00000,  0x0008C000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    BsData,  WRITE_BACK_XN},
  {"CPU Vectors",       0x9FF8C000,  0x00001000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    BsData,  WRITE_BACK},
  {"MMU PageTables",    0x9FF8D000,  0x00003000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    BsData,  WRITE_BACK_XN},
  {"UEFI Stack",        0x9FF90000,  0x00040000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    BsData,  WRITE_BACK_XN},
  {"RSRV1",             0x9FFD0000,  0x0000A000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    RtData,  WRITE_BACK_XN},
  {"TPMControl",        0x9FFDA000,  0x00003000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    RtData,  UNCACHED_UNBUFFERED_XN},
  {"Reset Data",        0x9FFDD000,  0x00004000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    RtData,  UNCACHED_UNBUFFERED_XN},
  {"RSRV3",             0x9FFE1000,  0x00001000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    RtData,  WRITE_BACK_XN},
  {"Capsule Header",    0x9FFE2000,  0x00001000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    RtData,  UNCACHED_UNBUFFERED_XN},
  {"RSRV2",             0x9FFE3000,  0x00014000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    RtData,  WRITE_BACK_XN},
  {"Log Buffer",        0x9FFF7000,  0x00008000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    RtData,  WRITE_BACK_XN},
  {"Info Blk",          0x9FFFF000,  0x00001000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    RtData,  WRITE_BACK_XN},
  {"RAM Partition",     0x40000000,  0x40000000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Conv,    WRITE_BACK_XN},
  {"RAM Partition",     0xA0000000,  0x60000000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Conv,    WRITE_BACK_XN},
  {"RAM Partition",     0x100000000, 0xBD8C0000,  AddMem,                SYS_MEM,  SYS_MEM_CAP,    Conv,    WRITE_BACK_XN},
  {"IMEM Base",         0x14680000,  0x00040000,  NoHob,                 MMAP_IO,  INITIALIZED,    Conv,    NS_DEVICE},
  {"IMEM Cookie Base",  0x146BF000,  0x00001000,  AddDev,                MMAP_IO,  INITIALIZED,    Conv,    NS_DEVICE},
  {"QDSS_STM",          0x16000000,  0x01000000,  AddDev,                MMAP_IO,  INITIALIZED,    Conv,    NS_DEVICE},
  {"BOOT_CONFIG",       0x00070000,  0x00010000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"GCC CLK CTL",       0x00100000,  0x000B0000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"UFS_RUMI",          0x00620000,  0x00020000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"RPM MSG RAM",       0x00778000,  0x00008000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"SECURITY CONTROL",  0x00780000,  0x00007000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"PRNG_CFG_PRNG",     0x00790000,  0x00010000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"MPM2_SLP_CNTR",     0x010A3000,  0x00001000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"MPM2_TSENS0",       0x010AA000,  0x00001000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"MPM2_TSENS0_TM",    0x010AB000,  0x00001000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"MPM2_PSHOLD",       0x010AC000,  0x00001000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"MPM2_TSENS1",       0x010AD000,  0x00001000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"MPM2_TSENS1_TM",    0x010AE000,  0x00001000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"A1_NOC",            0x01680000,  0x00009000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"PCIE WRAPPER AHB",  0x01C00000,  0x00007000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"UFS UFS REGS",      0x01DA0000,  0x00020000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"CRYPTO0 CRYPTO",    0x01DC0000,  0x00040000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"TCSR_TCSR_REGS",    0x01FC0000,  0x00026000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"TLMM CSR",          0x03400000,  0x00C00000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"GPMU_DRAM",         0x05026000,  0x00002000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"GPMU_BLOCK0",       0x0502A000,  0x00002000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"GPU_ISENSE",        0x05030000,  0x00002000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"GPUCC",             0x05065000,  0x00009000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"QDSS_QDSS",         0x06000000,  0x00100000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"HMSS_QLL",          0x06400000,  0x00200000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"PMIC ARB SPMI",     0x08000000,  0x02800000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"USB30_PRIM",        0x0A800000,  0x0011B000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"USB_RUMI",          0x0A920000,  0x00010000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"PERIPH_SS",         0x0C000000,  0x00200000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"MMSS",              0x0C800000,  0x00800000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"APCS_CC",           0x17800000,  0x00100000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"QTIMER",            0x17900000,  0x00030000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"APCS_GIC500_GICD",  0x17A00000,  0x00010000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"APCS_GIC500_GICR",  0x17B00000,  0x00100000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"APCS_GIC500_GICC",  0x17C00000,  0x00040000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},
  {"PCIE WRAPPER AXI",  0x1B000000,  0x01000000,  AddDev,                MMAP_IO,  UNCACHEABLE,    MmIO,    NS_DEVICE},

};

VOID
GetMemoryMap (
  OUT EFI_MEMORY_REGION_DESCRIPTOR **MemoryDescriptor,
  OUT UINT8                         *MemoryDescriptorCount
  )
{
  *MemoryDescriptor      = gMemoryDescriptor;
  *MemoryDescriptorCount = ARRAY_SIZE (gMemoryDescriptor);
}
