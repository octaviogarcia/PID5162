#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pablo Pessolani");
MODULE_DESCRIPTION("A simple example Linux module.");
MODULE_VERSION("0.01");

rwlock_t *tasklist_ptr;

static int __init lkm_example_init(void) {
	
char *tasklist_name = "tasklist_lock";
unsigned long sym_addr = kallsyms_lookup_name(tasklist_name);

 tasklist_ptr = (rwlock_t *) sym_addr;
 printk(KERN_INFO "Hello, DVS! tasklist_ptr=%X\n", tasklist_ptr);
 return 0;
}
static void __exit lkm_example_exit(void) {
 printk(KERN_INFO "Goodbye, DVS!\n");
}
module_init(lkm_example_init);
module_exit(lkm_example_exit);