#include <linux/module.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/hrtimer.h>

#define _MODULE_NAME         "hrtimerk"
#define _MODULE_NAME_TO_RP   "hrtimer: "

static struct hrtimer _hrtimer;

u64 start;

static enum hrtimer_restart test_hrtimer_handler(struct hrtimer *p_timer) {
	u64 _jiffies = jiffies;

	pr_info(_MODULE_NAME_TO_RP "jiffies now %u\n", 
		          jiffies_to_msecs(_jiffies - start));

	return HRTIMER_NORESTART;
}


static int __init _module_init(void)
{
    hrtimer_init(&_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    _hrtimer.function = &test_hrtimer_handler;
    start = jiffies;
    hrtimer_start(&_hrtimer, ms_to_ktime(100), HRTIMER_MODE_REL);

    return 0;
}

static void __exit _module_exit(void)
{
    hrtimer_cancel(&_hrtimer);
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("hrtimer LKM");
MODULE_VERSION("0.01");


module_init(_module_init);
module_exit(_module_exit);