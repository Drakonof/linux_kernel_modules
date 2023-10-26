#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>
#include <linux/timer.h>

#define _MODULE_NAME         "timer_led_blink"
#define _MODULE_NAME_TO_RP   "timer_led_blink: "

static struct timer_list _timer;

void timer_callback(struct timer_list *p_data)
{
    gpio_set_value(4,0);
}


static int __init _module_init(void)
{
    if (0 != gpio_request(4, "blink_led_gpio")) {
        pr_err(_MODULE_NAME_TO_RP "gpio_request failed\n");
        return -1;
    }

    if (0 != gpio_direction_output(4, 0)) {
        pr_err(_MODULE_NAME_TO_RP "gpio_direction_output failed\n");
        gpio_free(4);
        return -1;
    }

    gpio_set_value(4, 1);

    timer_setup(&_timer, timer_callback, 0);
    //timer - the timer to be modifie, expires - new timeout in jiffies
    mod_timer(&_timer, jiffies + msecs_to_jiffies(1000));

    return 0;
}

static void __exit _module_exit(void)
{
    gpio_free(4);
    del_timer(&_timer);
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("timer_led_blink LKM");
MODULE_VERSION("0.01");


module_init(_module_init);
module_exit(_module_exit);