#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x14522340, "module_layout" },
	{ 0x7ca65271, "single_open" },
	{ 0xc1d878fa, "single_release" },
	{ 0x77e93f6f, "seq_printf" },
	{ 0x1a6d6e4f, "remove_proc_entry" },
	{ 0xb72ec8a3, "seq_read" },
	{ 0xb4390f9a, "mcount" },
	{ 0x52683e38, "proc_create_data" },
	{ 0x5ca8e4f6, "seq_lseek" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "3DE7834B940BA6DEFFF2ED5");

static const struct rheldata _rheldata __used
__attribute__((section(".rheldata"))) = {
	.rhel_major = 6,
	.rhel_minor = 5,
};
