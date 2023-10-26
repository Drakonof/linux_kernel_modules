// sudo apt-get install device-tree-compiler

#include <linux/module.h>
#include <linux/init.h>
//?
#include <linux/mod_devicetable.h>
//?
#include <linux/property.h>
//?
#include <linux/platform_device.h>
//?
#include <linux/of_device.h>

#define _MODULE_NAME         "dt-probe"
#define _MODULE_NAME_TO_RP   "dt-probe: "


static int dt_probe(struct platform_device *p_dev) 
{
    struct device *_p_dev = &p_dev->dev;

    const char *label;

    int val = 0, ret = 0;

    if (0 == device_property_present(_p_dev, "label")) {
    	pr_err(_MODULE_NAME_TO_RP "label device_property_present failed\n");
    	return -1;
    }

   if (0 == device_property_present(_p_dev, "my_value")) {
    	pr_err(_MODULE_NAME_TO_RP "my_value device_property_present failed\n");
    	return -1;
    }

    ret = device_property_read_string(_p_dev, "label", &label);
    if (0 != ret) {
    	pr_err(_MODULE_NAME_TO_RP "device_property_read_string failed\n");
    	return -1;
    }

    pr_info(_MODULE_NAME_TO_RP "label: %s\n", label);

    ret = device_property_read_u32(_p_dev, "val", &val);
    if (0 != ret) {
    	pr_err(_MODULE_NAME_TO_RP "device_property_read_u32 failed\n");
    	return -1;
    }

    pr_info(_MODULE_NAME_TO_RP "val: %d\n", val);

    return 0;
}

static int dt_remove(struct platform_device *p_dev)
{
	pr_info(_MODULE_NAME_TO_RP "removed\n");
	return 0;
}


static struct of_device_id _driver_ids[] = {
	{
		.compatible = "brightlight,mydev",
	}, { /* sentinel */ }
};

static struct platform_driver _driver = {
	.probe = dt_probe,
	.remove = dt_remove,
	.driver = {
		.name = "dt_probe_driver",
		.of_match_table = _driver_ids,
	},
};


static int __init _module_init(void)
{
	pr_info(_MODULE_NAME_TO_RP "Hello dt-probe\n");

	if (platform_driver_register(&_driver)) {
		pr_err(_MODULE_NAME_TO_RP "platform_driver_registration failed\n");
    	return -1;
	}

	return 0;
}


static void __exit _module_exit(void)
{
	platform_driver_unregister(&_driver);
	pr_info(_MODULE_NAME_TO_RP "Goodbye dt_probe\n");
}



MODULE_DEVICE_TABLE(of, _driver_ids);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem Shimko");
MODULE_DESCRIPTION("dt-probe LKM");
MODULE_VERSION("0.01");


module_init(_module_init);
module_exit(_module_exit);