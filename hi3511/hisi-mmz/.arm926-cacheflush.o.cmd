cmd_/work/hi3511/drv/hisi-mmz/arm926-cacheflush.o := arm-hismall-linux-gcc -Wp,-MD,/work/hi3511/drv/hisi-mmz/.arm926-cacheflush.o.d  -nostdinc -isystem /opt/hisi-linux/x86-arm/gcc-3.4.3-uClibc-0.9.28/bin/../lib/gcc/arm-hisi-linux/3.4.3/include -D__KERNEL__ -Iinclude  -mlittle-endian -D__ASSEMBLY__ -mabi=apcs-gnu -mno-thumb-interwork -D__LINUX_ARM_ARCH__=5 -march=armv5te -mtune=arm9tdmi -msoft-float   -DMODULE -c -o /work/hi3511/drv/hisi-mmz/arm926-cacheflush.o /work/hi3511/drv/hisi-mmz/arm926-cacheflush.S

deps_/work/hi3511/drv/hisi-mmz/arm926-cacheflush.o := \
  /work/hi3511/drv/hisi-mmz/arm926-cacheflush.S \
    $(wildcard include/config/cpu/dcache/writethrough.h) \
  include/linux/linkage.h \
  include/linux/config.h \
    $(wildcard include/config/h.h) \
  include/asm/linkage.h \
  include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
    $(wildcard include/config/hotplug/cpu.h) \
  include/linux/compiler.h \
  include/asm/assembler.h \
  include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
    $(wildcard include/config/smp.h) \
  include/asm/pgtable.h \
  include/asm-generic/4level-fixup.h \
  include/asm/memory.h \
    $(wildcard include/config/discontigmem.h) \
  include/asm/arch/memory.h \
  include/asm/proc-fns.h \
    $(wildcard include/config/cpu/32.h) \
    $(wildcard include/config/cpu/arm610.h) \
    $(wildcard include/config/cpu/arm710.h) \
    $(wildcard include/config/cpu/arm720t.h) \
    $(wildcard include/config/cpu/arm920t.h) \
    $(wildcard include/config/cpu/arm922t.h) \
    $(wildcard include/config/cpu/arm925t.h) \
    $(wildcard include/config/cpu/arm926t.h) \
    $(wildcard include/config/cpu/sa110.h) \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/arm1020.h) \
    $(wildcard include/config/cpu/arm1020e.h) \
    $(wildcard include/config/cpu/arm1022.h) \
    $(wildcard include/config/cpu/arm1026.h) \
    $(wildcard include/config/cpu/xscale.h) \
    $(wildcard include/config/cpu/v6.h) \
  include/asm/arch/vmalloc.h \
  include/asm/procinfo.h \
  include/asm/hardware.h \
  include/asm/arch/hardware.h \
    $(wildcard include/config/hisilicon/osc/clock.h) \
    $(wildcard include/config/hisilicon/armcoreclk/scale.h) \
    $(wildcard include/config/hisilicon/ahbclk/def.h) \
    $(wildcard include/config/cpuclk/multi.h) \
    $(wildcard include/config/hisilicon/kconsole.h) \
  include/asm/sizes.h \
  include/asm/arch/cpu.h \
  include/asm/arch/platform.h \
  include/asm/page.h \
    $(wildcard include/config/cpu/copy/v3.h) \
    $(wildcard include/config/cpu/copy/v4wt.h) \
    $(wildcard include/config/cpu/copy/v4wb.h) \
    $(wildcard include/config/cpu/copy/v6.h) \
  include/asm-generic/page.h \
  include/asm/asm-offsets.h \
  include/asm/thread_info.h \
  include/asm/fpstate.h \
    $(wildcard include/config/iwmmxt.h) \

/work/hi3511/drv/hisi-mmz/arm926-cacheflush.o: $(deps_/work/hi3511/drv/hisi-mmz/arm926-cacheflush.o)

$(deps_/work/hi3511/drv/hisi-mmz/arm926-cacheflush.o):
