/*
 * backlight - Button backlight cpcap fix for Motorola Defy
 *
 * hooking taken from "n - for testing kernel function hooking" by Nothize
 * require symsearch module by Skrilaz
 *
 * Copyright (C) 2011 CyanogenDefy - GPL
 */

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/proc_fs.h>

#include "hook.h"
#include "../symsearch/symsearch.h"

#define TAG "disableversioncheck"

// module parameters (see end of file for descriptions)
static short hook_enable = 1;

// internals
static short hooked = 0;

struct load_info {
	Elf_Ehdr *hdr;
	unsigned long len;
	Elf_Shdr *sechdrs;
	char *secstrings, *strtab;
	unsigned long symoffs, stroffs;
	struct _ddebug *debug;
	unsigned int num_debug;
	struct {
		unsigned int sym, str, mod, vers, info, pcpu;
	} index;
};

static const struct kernel_symbol *resolve_symbol(struct module *mod,
						  const struct load_info *info,
						  const char *name,
						  char ownername[])
{
    struct module *owner;
	const struct kernel_symbol *sym;
	const unsigned long *crc;
	int err;

	mutex_lock(&module_mutex);
	sym = find_symbol(name, &owner, &crc,
			  !(mod->taints & (1 << TAINT_PROPRIETARY_MODULE)), true);
	if (!sym)
		goto unlock;

	/*if (!check_version(info->sechdrs, info->index.vers, name, mod, crc,
			   owner)) {
		sym = ERR_PTR(-EINVAL);
		goto getname;
	}*/

	err = ref_module(mod, owner);
	if (err) {
		sym = ERR_PTR(err);
		goto getname;
	}

getname:
	/* We must make copy under the lock if we failed to get ref. */
	strncpy(ownername, module_name(owner), MODULE_NAME_LEN);
unlock:
	mutex_unlock(&module_mutex);
	return sym;
    
    //return HOOK_INVOKE(resolve_symbol, mod, info, name, ownername);
}



/*
 * Module init & exit
 */
struct hook_info g_hi[] = {
	HOOK_INIT(resolve_symbol),
	HOOK_INIT_END
};

static int __init disableversioncheck_init(void) {

	printk(KERN_INFO TAG": disable version check for kernel-modules...\n");
	if (hook_enable) {
		hook_init();
		hooked = 1;
	}
	return 0;
}

static void __exit disableversioncheck_exit(void) {
    printk(KERN_INFO TAG": RE-enable version check for kernel-modules...\n");
	if (hooked) {
		hook_exit();
		hooked = 0;
	}
}

module_init(disableversioncheck_init);
module_exit(disableversioncheck_exit);

MODULE_ALIAS(TAG);
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Disable version-check for kernel modules");
MODULE_AUTHOR("Michael Zimmermann");
MODULE_LICENSE("GPL");
