#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/jiffies.h>

#define _MODULE_NAME         "kthread-demo"
#define _MODULE_NAME_TO_RP   "kthread-demo: "

static struct task_struct *kthread_1;
static struct task_struct *kthread_2;

static int t1 = 1, t2 = 2;

/* watch_Variable to monitor with the waitqueues */
static long int watch_var = 0;

/* Static declaration of waitqueue */
DECLARE_WAIT_QUEUE_HEAD(wq1);

/* Dynamic declaration of waitqueue */
static wait_queue_head_t wq2;

int kthread_function(void *p_kthread_nr)
{
	unsigned int i = 0;
	int kthread_nr = *(int *) p_kthread_nr;

	while(0 == kthread_should_stop()) {
		pr_info(_MODULE_NAME_TO_RP "thread %d is executed! counter val: %d\n", kthread_nr, i++);
		msleep(kthread_nr * 1000);
	}

    switch(kthread_nr) {
        case 1:
            wait_event(wq1, watch_var == 11);
            pr_info(_MODULE_NAME_TO_RP "watch_var is now 11!\n");
            break;
        case 2:
            while(wait_event_timeout(wq2, watch_var == 22, msecs_to_jiffies(5000)) == 0) {
                pr_err(_MODULE_NAME_TO_RP "Watch_var is still not 22, but timeout elapsed!\n");
            }

            pr_info(_MODULE_NAME_TO_RP "watch_var is now 22!\n");
        break;
        default:
        break;
    }

    pr_info(_MODULE_NAME_TO_RP "thread %d finished execution!\n", kthread_nr);
    return 0;
}

static ssize_t module_write(struct file *p_file, const char *p_user_buf, size_t count, loff_t *offs)
{
    int to_copy, not_copied, delta;
    char buffer[16];
    pr_info(_MODULE_NAME_TO_RP "write callback called\n");

    memset(buffer, 0, sizeof(buffer));

    to_copy = min(count, sizeof(buffer));


    not_copied = copy_from_user(buffer, p_user_buf, to_copy);

    delta = to_copy - not_copied;

    if(kstrtol(buffer, 10, &watch_var) == -EINVAL) {
        pr_info(_MODULE_NAME_TO_RP "error converting input!\n");
    }

    pr_info(_MODULE_NAME_TO_RP "watch_var is now %ld\n", watch_var);

    wake_up(&wq1);
    wake_up(&wq2);

    return delta;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = module_write
};



static int __init _module_init(void)
{
    pr_info(_MODULE_NAME_TO_RP "Hello kernel\n");
    init_waitqueue_head(&wq2);

    if(register_chrdev(0x64, _MODULE_NAME, &fops) != 0) {
        pr_err(_MODULE_NAME_TO_RP "register_chrdev failed!\n");
        return -1;
    }

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
        unregister_chrdev(0x64, _MODULE_NAME);
    	return -1;
    }

    kthread_2 = kthread_run(kthread_function, &t2, "kthread_2");
    if (kthread_2 != NULL) {
    	pr_info(_MODULE_NAME_TO_RP "thread 2 was created and is running now!\n");
    }
    else {
    	pr_err(_MODULE_NAME_TO_RP "kthread_run failed\n");
        unregister_chrdev(0x64, _MODULE_NAME);
        kthread_stop(kthread_1);
    	return -1;
    }

    pr_info(_MODULE_NAME_TO_RP "both threads are running now!\n");

    return 0;
}


static void __exit _module_exit(void)
{
    watch_var = 11;
    wake_up(&wq1);
    mdelay(10);
    watch_var = 22;
    wake_up(&wq2);
    mdelay(10);
	kthread_stop(kthread_1);
	kthread_stop(kthread_2);
    unregister_chrdev(0x64, _MODULE_NAME);
	pr_info(_MODULE_NAME_TO_RP "Goodbye kernel\n");
}



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("kthread-demo LKM");
MODULE_VERSION("0.01");


module_init(_module_init);
module_exit(_module_exit);