/*
 * chiron's memory map, as the firmware must describe it to the OS.
 *
 * Sources, per line:
 *   - DDR regions and the framebuffer: Android-chiron.dtb
 *     ("memory" reg, "linux,cma" reg, reserved-memory children)
 *   - register windows: the soc node's ranges = <0 0 0 0xffffffff>, i.e. the
 *     peripherals are identity-mapped across the low 4 GB, so each window below
 *     is the device's own reg property.
 *
 * Nothing here is a guess: a wrong region in this table is how a phone loses its
 * storage or faults on boot.
 */
#include <Library/MemoryMapLib.h>

STATIC
EFI_MEMORY_REGION_DESCRIPTOR
gMemoryDescriptor[] = {
  // Name, Address, Length, HobOption, ResourceType, ResourceAttribute, MemoryType, ArmAttribute

  // ---- DDR -----------------------------------------------------------------
  // The firmware's own working area. FD_BASE in chiron.conf is the UEFI image's
  // address and must fall inside "UEFI FD".
  {"UEFI Stack",         0x40000000, 0x00040000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  {"UEFI FD",            0x9FD00000, 0x00200000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
  {"DXE Heap",           0x80000000, 0x05800000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv,   WRITE_BACK},

  // ---- reserved, from the device tree's reserved-memory children -------------
  {"Adsp Region",        0x8AB00000, 0x00700000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"Adsp Sec Heap",      0x8B200000, 0x01E00000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"Mpss Region",        0x8D000000, 0x07000000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"Venus Region",       0x94000000, 0x00500000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"Venus Sec Heap",     0x94500000, 0x00200000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"Qseecom Region",     0x94700000, 0x00F00000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"Secure Region",      0x95600000, 0x00100000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
  {"Sp Region",          0x85800000, 0x03700000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},

  // The framebuffer the UEFI reports (chiron.conf FB_BASE = 0x9D400000).
  // WRITE_THROUGH: the panel reads it, so it must not sit dirty in a cache.
  {"Display Reserved",   0x9D400000, 0x02400000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_THROUGH},

  // ---- registers, identity-mapped (soc ranges) ------------------------------
  {"GIC Distributor",    0x17A00000, 0x00010000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"GIC Redistributors", 0x17B00000, 0x00100000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"GCC Clocks",         0x00100000, 0x000B0000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"GPUCC",              0x05065000, 0x00009000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"GPU",                0x05000000, 0x00040000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"KGSL SMMU",          0x05040000, 0x00010000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"TLMM Pinctrl",       0x03400000, 0x00300000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"UART Debug",         0x0A900000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"UFS",                0x01DA0000, 0x00026000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"UFS PHY",            0x01DA6000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"MDSS MDP",           0x0C900000, 0x00090000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"MDSS DSI",           0x0C994000, 0x00001000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"PMIC Arbiter",       0x0C000000, 0x00100000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE},
  {"SPMI",               0x0C200000, 0x00100000, AddDev, MMAP_IO, UNCACHEABLE, MmIO, NS_DEVICE}
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
