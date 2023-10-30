#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>

#define _MODULE_NAME         "kthread-demo"
#define _MODULE_NAME_TO_RP   "kthread-demo: "

static struct task_struct *kthread_1;
static struct task_struct *kthread_2;

static int t1 = 1, t2 = 2;

int kthread_function(void *p_kthread_nr)
{
	unsigned int i = 0;
	int kthread_nr = *(int *) p_kthread_nr;

	while(0 == kthread_should_stop()) {
		pr_info(_MODULE_NAME_TO_RP "thread %d is executed! counter val: %d\n", kthread_nr, i++);
		msleep(kthread_nr * 1000);
	}

    pr_info(_MODULE_NAME_TO_RP "thread %d finished execution!\n", kthread_nr);
    return 0;
}

static int __init _module_init(void)
{
    pr_info(_MODULE_NAME_TO_RP "Hello kernel\n");

    /*
    struct task_struct * kthread_create (int (* threadfn(void *data),
                                         void * data,
                                         const char namefmt[]);
    */
    kthread_1 = kthread_create(kthread_function, &t1, "kthread_1");
    if (kthread_1 != NULL) {
    	wake_up_process(kthread_1);
    	pr_info(_MODULE_NAME_TO_RP "thread 1 was created and is running now!\n");
    }
    else {
    	pr_err(_MODULE_NAME_TO_RP "kthread_create failed\n");
    	return -1;
    }

    kthread_2 = kthread_run(kthread_function, &t2, "kthread_2");
    if (kthread_2 != NULL) {
    	pr_info(_MODULE_NAME_TO_RP "thread 2 was created and is running now!\n");
    }
    else {
    	pr_err(_MODULE_NAME_TO_RP "kthread_run failed\n");
        kthread_stop(kthread_1);
    	return -1;
    }

    pr_info(_MODULE_NAME_TO_RP "both threads are running now!\n");

    return 0;
}


static void __exit _module_exit(void)
{
	kthread_stop(kthread_1);
	kthread_stop(kthread_2);
	pr_info(_MODULE_NAME_TO_RP "Goodbye kernel\n");
}



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("kthread-demo LKM");
MODULE_VERSION("0.01");


module_init(_module_init);
module_exit(_module_exit);