#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
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
	{ 0xe00b4984, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x23205cc9, __VMLINUX_SYMBOL_STR(usb_deregister) },
	{ 0x3371153, __VMLINUX_SYMBOL_STR(usb_register_driver) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xe914e41e, __VMLINUX_SYMBOL_STR(strcpy) },
	{ 0xbbf59390, __VMLINUX_SYMBOL_STR(usb_control_msg) },
	{ 0x728d8849, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0xfe57f7e2, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0x86a4889a, __VMLINUX_SYMBOL_STR(kmalloc_order_trace) },
	{ 0x1414c049, __VMLINUX_SYMBOL_STR(usb_register_dev) },
	{ 0xf82eacf7, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xe2cb65c5, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x379cab39, __VMLINUX_SYMBOL_STR(usb_deregister_dev) },
	{ 0xb8a418f6, __VMLINUX_SYMBOL_STR(dev_set_drvdata) },
	{ 0xe43fc9a7, __VMLINUX_SYMBOL_STR(dev_get_drvdata) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0xc3165563, __VMLINUX_SYMBOL_STR(usb_clear_halt) },
	{ 0x50eedeb8, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xc5849d3c, __VMLINUX_SYMBOL_STR(usb_bulk_msg) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0x14ee158f, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0xa22311d0, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x68dfc59f, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0xc4554217, __VMLINUX_SYMBOL_STR(up) },
	{ 0xdd1a2871, __VMLINUX_SYMBOL_STR(down) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("usb:v0547p1002d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v21E1p0000d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v21E1p0001d*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v21E1p0005d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "247CE196EDB02147B0D7D05");
