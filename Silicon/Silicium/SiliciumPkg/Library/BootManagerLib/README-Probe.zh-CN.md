# GPU 探针的构建与验证

## 构建

官方脚本，不要手拼 build 参数：

```sh
cd /root/hk/Mu-Silicium
python3 build_uefi.py -d chiron -r DEBUG
```

45 秒完成，49 个镜像通过 ImageValidation，产物：

| 文件 | 大小 | 说明 |
|---|---|---|
| `Build/chironPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd` | 3,145,728 | 完整 FV，LZMA 压缩 |
| `Mu-chiron.img` | 1,261,568 | Android boot image，可直接 fastboot boot |

## 怎么确认探针真的在里面（这一步花了很久）

直接 `strings` 搜 `SILICIUM_UEFI.fd` 或 `Mu-chiron.img` 都找不到探针，**不代表没编进去**。
真实原因是三层包装：

1. `Mu-chiron.img` 是 Android boot image，内核段是 **gzip** 压缩的 `SILICIUM_UEFI.fd`
2. `SILICIUM_UEFI.fd` 的 `FVMAIN_COMPACT` 里，`FVMAIN` 由 **_GUIDED LZMA 段_** 承载
3. LTO（`-flto`）让字符串只在最终链接产物出现，`BootManager.obj` 里是没有的

正确的检查方法：

```python
# 1. 跳到 FVMAIN 的 GUIDED 段
LZMA_GUID = uuid.UUID("ee4e5898-3914-4259-9d6e-dc7bd79403cf").bytes_le
# 2. 段的 4 字节头 + 16 字节 GUID + 2 字节 DataOffset + 2 字节 Attributes
# 3. 用 lzma.decompress(payload, format=lzma.FORMAT_ALONE) 解
```

实测结果：

```
section@0x10018 type=0x02 (GUIDED) size=0xc3553
  lzma/alone -> 4321288 bytes
  BdsDxe: True
  A540 GPU probe: True
```

解压后能直接读到探针的格式字符串：

```
==== A540 GPU probe (read only), CurrentEL=%lx ====
CX_GDSCR=%08x (collapse %u) GX_GDSCR=%08x (collapse %u)
CP_RB_BASE=%08x CP_RB_CNTL=%08x CP_RPTR=%08x CP_WPTR=%08x ME_CNTL=%08x
==== end of A540 GPU probe ====
```

## 三个踩过的坑

**1. `BootManager.obj` 里没有字符串是正常的。** `-flto` 下函数体在链接时才生成。
要在 `.efi` 或解压后的 FV 里找，不要在 `.obj` 里找。

**2. `SILICIUM_UEFI.fd` 里搜 `BdsDxe` 找不到也是正常的。** 因为它在 LZMA 段里。
`strings` 对压缩数据无效。

**3. `Mu-chiron.img` 的 kernel 段是 gzip。** 大小 1,255,613 字节，解压后 3,145,904 字节
（3,145,728 的 FV + 176 字节 boot shim）。
