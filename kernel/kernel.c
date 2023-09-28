#include <cpu.h>
#include <kernel/arch/x86_64/cmos.h>
#include <kernel/arch/x86_64/debug_console.h>
#include <kernel/arch/x86_64/ports.h>
#include <kernel/misc.h>
#include <kernel/string.h>
#include <kernel/version.h>
#include <klog.h>
#include <misc/kprintf.h>
#include <multiboot.h>
#include <multiboot2.h>
#include <stdint.h>

static uintptr_t highest_valid_address = 0;
extern uintptr_t kernel_end;
static uintptr_t highest_kernel_address = (uintptr_t)&kernel_end;

static void parse_multiboot2(void *mboot) {
  // mboot_is_2 = 1;
  KLOGV("multiboot", "Started with a Multiboot 2 loader");

  struct multiboot2_tag_mmap *mmap = NULL;
  for (struct multiboot2_tag *tag = (struct multiboot2_tag *)(mboot + 8);
       tag->type != MULTIBOOT2_TAG_TYPE_END;
       tag = (struct multiboot2_tag *)((multiboot2_uint8_t *)tag +
                                       ((tag->size + 7) & ~7))) {
    if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
      mmap = (struct multiboot2_tag_mmap *)tag;
    } else if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE) {
      struct multiboot2_tag_module *module =
          (struct multiboot2_tag_module *)tag;

      uintptr_t addr = (uintptr_t)module->mod_end;
      if (addr > highest_kernel_address)
        highest_kernel_address = addr;
    }
  }

  if (!mmap) {
    KLOGE("multiboot", "unable to boot without memory map from loader");
    arch_hcf();
  }

  multiboot2_memory_map_t *mmap_entry;
  for (mmap_entry = mmap->entries; (multiboot2_uint8_t *)mmap_entry <
                                   (multiboot2_uint8_t *)mmap + mmap->size;
       mmap_entry = (multiboot2_memory_map_t *)((unsigned long)mmap_entry +
                                                mmap->entry_size)) {
    KLOGV("mmap",
          "base_addr = 0x%x%x,"
          " length = 0x%x%x, type = 0x%x\n",
          (unsigned)(mmap_entry->addr >> 32),
          (unsigned)(mmap_entry->addr & 0xFFFFFFFF),
          (unsigned)(mmap_entry->len >> 32),
          (unsigned)(mmap_entry->len & 0xFFFFFFFF), (unsigned)mmap_entry->type);

    if (mmap_entry->type == MULTIBOOT2_MEMORY_AVAILABLE && mmap_entry->len &&
        mmap_entry->addr + mmap_entry->len - 1 > highest_valid_address) {
      highest_valid_address = mmap_entry->addr + mmap_entry->len - 1;
    }
  }

  /* Round the max address up a page */
  highest_kernel_address =
      (highest_kernel_address + 0xFFF) & 0xFFFFFFFFFFFFF000;
}

static void parse_multiboot(multiboot_info_t *mboot) {

  KLOGV("multiboot", "Started with a Multiboot 1 loader");

  if (!(mboot->flags & MULTIBOOT_INFO_MEM_MAP)) {
    KLOGE("multiboot", "unable to boot without memory map from loader\n");
    arch_hcf();
  }

  multiboot_memory_map_t *mmap;

  KLOGV("multiboot", "mmap_addr = 0x%x, mmap_length = 0x%x\n",
        (unsigned)mboot->mmap_addr, (unsigned)mboot->mmap_length);

  if ((uintptr_t)mboot->mmap_addr + mboot->mmap_length >
      highest_kernel_address) {
    highest_kernel_address = (uintptr_t)mboot->mmap_addr + mboot->mmap_length;
  }

  for (mmap = (multiboot_memory_map_t *)(uintptr_t)mboot->mmap_addr;
       (unsigned long)mmap < mboot->mmap_addr + mboot->mmap_length;
       mmap = (multiboot_memory_map_t *)((unsigned long)mmap + mmap->size +
                                         sizeof(mmap->size))) {

    KLOGV("mmap",
          " size = 0x%x, base_addr = 0x%x%08x,"
          " length = 0x%x%08x, type = 0x%x\n",
          (unsigned)mmap->size, (unsigned)(mmap->addr >> 32),
          (unsigned)(mmap->addr & 0xffffffff), (unsigned)(mmap->len >> 32),
          (unsigned)(mmap->len & 0xffffffff), (unsigned)mmap->type);
    if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE && mmap->len &&
        mmap->addr + mmap->len - 1 > highest_valid_address) {
      highest_valid_address = mmap->addr + mmap->len - 1;
    }
  }

  if (mboot->flags & MULTIBOOT_INFO_MODS) {
    KLOGV("multiboot", "mods_count = %d, mods_addr = 0x%x\n",
          (int)mboot->mods_count, (int)mboot->mods_addr);

    multiboot_module_t *mod;
    size_t i;
    for (i = 0, mod = (multiboot_module_t *)(uintptr_t)mboot->mods_addr;
         i < mboot->mods_count; i++, mod++) {
      KLOGV("multiboot", " mod_start = 0x%x, mod_end = 0x%x, cmdline = %s\n",
            (unsigned)mod->mod_start, (unsigned)mod->mod_end,
            (char *)(uintptr_t)mod->cmdline);

      const uintptr_t addr = mod->mod_end;
      if (addr > highest_kernel_address) {
        highest_kernel_address = addr;
      }
    }
  }

  /* Round the max address up a page */
  highest_kernel_address =
      (highest_kernel_address + 0xFFF) & 0xFFFFFFFFFFFFF000;
}
void multiboot_initialize(void *mboot, uint32_t mboot_magic_number) {
  /* Parse multiboot data so we can get memory map, modules, command line, etc.
   */
  if (mboot_magic_number == 0x36d76289) {
    parse_multiboot2(mboot);
  } else {
    parse_multiboot(mboot);
  }
}

/**
 * @brief x86-64 multiboot C entrypoint.
 *
 * Called by the x86-64 longmode bootstrap.
 */
void kmain(void *mboot, uint32_t mboot_magic_number/*, void *esp*/) {
  fpu_initialize();
  debugcon_init();
  arch_clock_initialize();
  /* Parse multiboot data so we can get memory map, modules, command line, etc.
   */
  multiboot_initialize(mboot, mboot_magic_number);

  // kprintf("\e[1;1H\e[2J"); // clear screen
  kprintf("Welcome to \x1B[33m%s\x1B[37m v", __kernel_name);
  kprintf(__kernel_version_format, __kernel_version_major,
          __kernel_version_minor, __kernel_version_patch,
          __kernel_version_suffix);
  kprintf("\nbuild_date: %s %s\n", __kernel_build_date, __kernel_build_time);
  date_t date = read_rtc();
  uint64_t epoch_time = read_epoch_time();
  kprintf("date: %d-%d-%d %d:%d\n", date.year, date.month, date.day, date.hour,
          date.minute);
  kprintf("epoch_time: %llu\n\n", epoch_time);

  const char *TAG = "Kernel";
  KLOGV(TAG, "some verbose from kernel");
  KLOGD(TAG, "some debugging from kernel");
  KLOGI(TAG, "some info from kernel");
  KLOGW(TAG, "Some warning from kernel!");
  KLOGE(TAG, "Some Error from kernel!!");

  arch_hcf();
}
