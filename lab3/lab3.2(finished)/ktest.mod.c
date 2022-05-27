#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0xb3753869, "module_layout" },
	{ 0x461f3e7c, "sysfs_remove_group" },
	{ 0xceaad73f, "filp_close" },
	{ 0xa5cc5d9f, "kthread_stop" },
	{ 0x19f142b0, "sysfs_create_group" },
	{ 0x9626a279, "mm_kobj" },
	{ 0x76d31a6d, "wake_up_process" },
	{ 0xe8bc695c, "kthread_create_on_node" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x69acdf38, "memcpy" },
	{ 0xf05c7b8, "__x86_indirect_thunk_r15" },
	{ 0xce8b1878, "__x86_indirect_thunk_r14" },
	{ 0x38dab310, "filp_open" },
	{ 0x4482cdb, "__refrigerator" },
	{ 0x8ce83336, "freezing_slow_path" },
	{ 0x1000e51, "schedule" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x54496b4, "schedule_timeout_interruptible" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0x7ab88a45, "system_freezing_cnt" },
	{ 0xc5bb1bc3, "mmput" },
	{ 0xa916b694, "strnlen" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0x1d19f77b, "physical_mask" },
	{ 0x36e58bcd, "pv_ops" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0xb2b15899, "get_task_mm" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0xee0fb577, "set_user_nice" },
	{ 0x9e61bb05, "set_freezable" },
	{ 0x95eb05bd, "kernel_write" },
	{ 0x1b44c663, "current_task" },
	{ 0x66b1fab3, "pid_task" },
	{ 0xbe7f5f72, "find_get_pid" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xc5850110, "printk" },
	{ 0x3c80c06c, "kstrtoull" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "9D58DF42153AA547F880A02");
