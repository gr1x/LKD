# Contiguous memory allocations


## Recompile kernel

```bash
wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.15.tar.x
tar xvf linux-4.15.tar.xz
cd linux-4.15
cp -v /boot/config-4.15.0-48-generic .config or zcat /proc/config.gz > .config
sudo apt-get install build-essential libncurses-dev bison flex libssl-dev libelf-dev
make clean && make mrproper
```

```bash
cat .config | grep CMA 
CONFIG_CMA=y
CONFIG_CMA_DEBUG=y
CONFIG_CMA_DEBUGFS=y
CONFIG_CMA_AREAS=7
CONFIG_CMA_SIZE_MBYTES=2048
CONFIG_DMA_CMA=y
CONFIG_HAVE_DMA_CONTIGUOUS=y
```

```
make -j9
sudo make modules_install
sudo make install
```



## Boot args

https://www.kernel.org/doc/html/v4.14/admin-guide/kernel-parameters.html

```
cma=nn[MG]@[start[MG][-end[MG]]]
                [ARM,X86,KNL]
                Sets the size of kernel global memory area for
                contiguous memory allocations and optionally the
                placement constraint by the physical address range of
                memory allocations. A value of 0 disables CMA
                altogether. For more information, see
                include/linux/dma-contiguous.h
```

```
 "cma=1G@1-2G"
```



## dmesg | grep cma 
> [    0.000000] Command line: BOOT_IMAGE=/boot/vmlinuz-4.15.0CMA20190430 root=UUID=3865b135-75b9-4465-9640-2d139cb46b38 ro debian-installer/custom-installation=/custom find_preseed=/preseed.cfg auto preseed/file=/floppy/preseed.cfg automatic-ubiquity noprompt priority=critical locale=en_US console-setup/modelcode=evdev cma=1G@1-2G
> [    0.000000] cma: early_cma(1G@1-2G)
> [    0.000000] cma: dma_contiguous_reserve(limit 140000000)
> [    0.000000] cma: dma_contiguous_reserve: reserving 1024 MiB for global area
> [    0.000000] cma: cma_declare_contiguous(size 0x0000000040000000, base 0x0000000000000001, limit 0x0000000080000000 alignment 0x0000000000000000)
> [    0.000000] cma: Reserved 1024 MiB at 0x0000000040000000
> [    0.000000] Kernel command line: BOOT_IMAGE=/boot/vmlinuz-4.15.0CMA20190430 root=UUID=3865b135-75b9-4465-9640-2d139cb46b38 ro debian-installer/custom-installation=/custom find_preseed=/preseed.cfg auto preseed/file=/floppy/preseed.cfg automatic-ubiquity noprompt priority=critical locale=en_US console-setup/modelcode=evdev cma=1G@1-2G
> [    0.000000] Memory: 2404960K/4193716K available (12300K kernel code, 2471K rwdata, 4192K rodata, 2396K init, 2416K bss, 740180K reserved, 1048576K cma-reserved)
> [    5.460514] cma: cma_alloc(cma 0000000087b8192a, count 1, align 0)
> [    5.476537] cma: cma_alloc(): returned 000000004596fdab
> [    5.477712] cma: cma_alloc(cma 0000000087b8192a, count 1, align 0)
> [    5.480302] cma: cma_alloc(): returned 00000000bd6e8859
> [    5.482125] cma: cma_alloc(cma 0000000087b8192a, count 1, align 0)
> [    5.484726] cma: cma_alloc(): returned 0000000003c52ffc



## Meminfo

> cat /proc/meminfo | grep Cma
> CmaTotal:        1048576 kB
> CmaFree:         1044036 kB



## Kernel Configuration

### Obtain running kernel's configuration

> cat /proc/config.gz | grep CMA
>
> cat /boot/config-4.15.0-20-generic | grep CMA



## Debugfs

```bash
# cd /sys/kernel/debug/cma/cma-reserved

# cat /proc/meminfo | grep Cma
CmaTotal:        1048576 kB
CmaFree:         1039940 kB

# echo 102400 > alloc
cat /proc/meminfo | grep Cma
CmaTotal:        1048576 kB
CmaFree:          630340 kB

# dmesg -c
[ 1039.979682] cma: cma_alloc(cma 0000000087b8192a, count 102400, align 0)
[ 1039.985118] cma: cma_alloc(): returned 0000000088c1ffab

# echo 102400 > free 
# dmesg -c
[ 1116.206155] cma: cma_release(page 0000000088c1ffab)
```



## Error

**dma_alloc_coherent() failed with improperly initialized dev structure?**

```bash
echo 8 > /dev/cma_testdmesg -c
[ 1405.768516] cma_test: loading out-of-tree module taints kernel.
[ 1405.768696] cma_test: module verification failed: signature and/or required key missing - tainting kernel
[ 1405.772654] misc cma_test: registered.
[ 1405.772655] misc cma_test: dma_set_coherent_mask ret: 0
[ 1405.772655] misc cma_test: System supports 32-bit DMA
[ 1433.392305] user_copy size in MB: 8
[ 1433.392308] user_copy size in Byte: 8388608
[ 1433.392317] misc cma_test: no mem in CMA area for size: 0x800000 Bytes
```

Simply pass NULL dev in *dma_alloc_coherent()* to bypass, should be a real bus device in real-world driver module.

```bash
# insmod cma_test.ko 
# dmesg -c
[ 2172.055692] misc cma_test: registered.
[ 2172.055693] misc cma_test: dma_set_coherent_mask ret: 0
[ 2172.055694] misc cma_test: System supports 32-bit DMA

# cat /proc/meminfo  | grep Cma
CmaTotal:        1048576 kB
CmaFree:         1044036 kB

# echo 128 > /dev/cma_test 
# dmesg -c
[  100.306201] user_copy size in MB: 128
[  100.306204] user_copy size in Byte: 134217728
[  100.306213] cma: cma_alloc(cma 000000005be58dbc, count 32768, align 8)
[  100.314271] cma: cma_alloc(): returned 000000005439870b
[  100.339067] misc cma_test: allocate CM at virtual address: 0x00000000ece62c73 address: 0x00000000745cf12e size:128MiB

# cat /proc/meminfo | grep Cma
CmaTotal:        1048576 kB
CmaFree:          912964 kB

# echo 512 > /dev/cma_test ; dmesg -c
[  230.596028] user_copy size in MB: 512
[  230.596029] user_copy size in Byte: 536870912
[  230.596032] cma: cma_alloc(cma 000000005be58dbc, count 131072, align 8)
[  230.605321] cma: cma_alloc(): returned 00000000a7144736
[  230.727276] misc cma_test: allocate CM at virtual address: 0x000000004cc7dd48 address: 0x0000000003461614 size:512MiB

# cat /proc/meminfo  | grep Cma
CmaTotal:        1048576 kB
CmaFree:          388676 kB

# dmesg -c
# cat /dev/cma_test 
# dmesg -c
[  380.081874] cma: cma_release(page 000000005439870b)
[  380.086066] misc cma_test: free CM at virtual address: 0x00000000ece62c73 dma address: 0x00000000745cf12e size:128MiB
# cat /dev/cma_test ; dmesg -c
[  392.369980] cma: cma_release(page 00000000a7144736)
[  392.375710] misc cma_test: free CM at virtual address: 0x000000004cc7dd48 dma address: 0x0000000003461614 size:512MiB
```

Wait to be testified with real PCI device ...

