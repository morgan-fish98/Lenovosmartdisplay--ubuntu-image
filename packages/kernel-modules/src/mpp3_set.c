// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/regmap.h>

static int level = 1;
module_param(level, int, 0644);

static int __init mpp3_init(void)
{
	struct device *dev;
	struct regmap *map;
	unsigned int m = 0, v = 0, e = 0, a = 0;
	int rc;

	dev = bus_find_device_by_name(&platform_bus_type, NULL,
				      "200f000.spmi:pmic@2:mpps@a000");
	if (!dev) {
		pr_err("mpp3: mpps device not found\n");
		return -ENODEV;
	}
	map = dev_get_regmap(dev->parent, NULL);
	if (!map)
		map = dev_get_regmap(dev, NULL);
	if (!map) {
		pr_err("mpp3: no regmap (parent=%s)\n", dev->parent ? dev_name(dev->parent) : "?");
		put_device(dev);
		return -ENODEV;
	}
	pr_info("mpp3: using regmap of %s\n",
		dev_get_regmap(dev->parent, NULL) ? dev_name(dev->parent) : dev_name(dev));

	rc = regmap_read(map, 0xa240, &m);
	if (rc) pr_err("mpp3: read mode rc=%d\n", rc);
	rc = regmap_read(map, 0xa241, &v);
	if (rc) pr_err("mpp3: read vin rc=%d\n", rc);
	rc = regmap_read(map, 0xa246, &e);
	if (rc) pr_err("mpp3: read en rc=%d\n", rc);
	rc = regmap_read(map, 0xa248, &a);
	if (rc) pr_err("mpp3: read aout rc=%d\n", rc);
	pr_info("mpp3: before mode=0x%02x vin=0x%02x en=0x%02x aout=0x%02x\n",
		m & 0xff, v & 0xff, e & 0xff, a & 0xff);

	/* VIN first, then mode/value, master-enable last */
	regmap_write(map, 0xa241, 0x02);
	regmap_write(map, 0xa248, 0x00);
	regmap_write(map, 0xa240, level ? 0x11 : 0x10);
	regmap_write(map, 0xa246, 0x80);

	regmap_read(map, 0xa240, &m);
	regmap_read(map, 0xa241, &v);
	regmap_read(map, 0xa246, &e);
	regmap_read(map, 0xa248, &a);
	pr_info("mpp3: after mode=0x%02x vin=0x%02x en=0x%02x aout=0x%02x (level=%d)\n",
		m & 0xff, v & 0xff, e & 0xff, a & 0xff, level);

	put_device(dev);
	return 0;
}

static void __exit mpp3_exit(void)
{
}

module_init(mpp3_init);
module_exit(mpp3_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Configure PMI8950 MPP3 on SPMI SID 2");
