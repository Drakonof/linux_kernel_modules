#include <linux/module.h>
#include <linux/init.h>

static int __init init(void)
{
	pr_info("Hello kernel\n");
	return 0;
}


static void __exit exit(void)
{
	pr_info("Goodbye kernel");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("hello kernel LKM");

module_init(init);
module_exit(exit);