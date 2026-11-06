// amberctl - live register/GPIO poking for amber bring-up.
//
//   echo "d846:0,!50,d846:80" > /sys/kernel/debug/amberctl/cmd
//        <addr>:<val>  hex PMIC register write (absolute, e.g. d846)
//        !<ms>         sleep
//        g<pin>:<val>  TLMM GPIO drive (pin 0..141, val 0/1)
//        i<pin>        TLMM GPIO read -> dmesg
//
// Every op is logged to dmesg so the result can be read back over serial.
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/string.h>

#define TLMM_BASE	0x01000000UL
#define TLMM_STRIDE	0x1000UL
#define WLED_DEV	"200f000.spmi:pmic@3:leds@d800"

static struct regmap *wled_map;
static struct dentry *dbg_dir;

static int pmic_write(unsigned int addr, unsigned int val)
{
	int rc;

	if (!wled_map) {
		pr_err("amberctl: no regmap\n");
		return -ENODEV;
	}
	rc = regmap_write(wled_map, addr, val);
	pr_info("amberctl: pmic[0x%04x] = 0x%02x (rc=%d)\n", addr, val, rc);
	return rc;
}

static void gpio_drive(unsigned int pin, unsigned int val)
{
	unsigned long phys = TLMM_BASE + TLMM_STRIDE * (unsigned long)pin;
	void __iomem *ctl, *io;
	u32 c, o;

	if (pin > 141) {
		pr_err("amberctl: gpio %u out of range\n", pin);
		return;
	}
	ctl = ioremap(phys, 0x8);
	if (!ctl) {
		pr_err("amberctl: ioremap failed\n");
		return;
	}
	io = ctl + 4;

	c = readl(ctl);
	c &= ~(0x7u << 2);	/* func0 = GPIO */
	c |= (1u << 9);		/* output enable */
	writel(c, ctl);

	o = readl(io);
	if (val)
		o |= (1u << 1);
	else
		o &= ~(1u << 1);
	writel(o, io);

	pr_info("amberctl: gpio%u = %u (ctl=%08x io=%08x)\n",
		pin, val, readl(ctl), readl(io));
	iounmap(ctl);
}

static void gpio_read(unsigned int pin)
{
	unsigned long phys = TLMM_BASE + TLMM_STRIDE * (unsigned long)pin;
	void __iomem *ctl;
	u32 c, o;

	if (pin > 141)
		return;
	ctl = ioremap(phys, 0x8);
	if (!ctl)
		return;
	c = readl(ctl);
	o = readl(ctl + 4);
	pr_info("amberctl: gpio%u ctl=%08x io=%08x in=%u out=%u oe=%u\n",
		pin, c, o, o & 1, (o >> 1) & 1, (c >> 9) & 1);
	iounmap(ctl);
}

static ssize_t cmd_write(struct file *f, const char __user *ubuf,
			 size_t n, loff_t *off)
{
	char *buf, *tok, *save;
	size_t len = min_t(size_t, n, 1024);

	buf = kzalloc(len + 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	if (copy_from_user(buf, ubuf, len)) {
		kfree(buf);
		return -EFAULT;
	}
	buf[len] = 0;

	save = buf;
	while ((tok = strsep(&save, ",")) != NULL) {
		unsigned int a, v;

		while (*tok == ' ' || *tok == '\n' || *tok == '\t')
			tok++;
		if (!*tok)
			continue;

		if (tok[0] == '!') {
			if (kstrtouint(tok + 1, 10, &v) == 0)
				msleep(v);
			continue;
		}
		if (tok[0] == 'i') {
			if (kstrtouint(tok + 1, 10, &v) == 0)
				gpio_read(v);
			continue;
		}
		if (tok[0] == 'g') {
			char *colon = strchr(tok, ':');
			if (colon) {
				*colon = 0;
				if (kstrtouint(tok + 1, 10, &a) == 0 &&
				    kstrtouint(colon + 1, 10, &v) == 0)
					gpio_drive(a, v);
			}
			continue;
		}
		{
			char *colon = strchr(tok, ':');
			if (colon) {
				*colon = 0;
				if (kstrtouint(tok, 16, &a) == 0 &&
				    kstrtouint(colon + 1, 16, &v) == 0)
					pmic_write(a, v);
			}
		}
	}

	kfree(buf);
	return n;
}

static const struct file_operations cmd_fops = {
	.owner = THIS_MODULE,
	.write = cmd_write,
};

static int __init amberctl_init(void)
{
	struct device *dev;

	dev = bus_find_device_by_name(&platform_bus_type, NULL, WLED_DEV);
	if (dev) {
		wled_map = dev_get_regmap(dev, NULL);
		if (!wled_map && dev->parent) {
			wled_map = dev_get_regmap(dev->parent, NULL);
			if (wled_map)
				pr_info("amberctl: regmap from parent %s\n",
					dev_name(dev->parent));
		}
		if (!wled_map && dev->parent && dev->parent->parent) {
			wled_map = dev_get_regmap(dev->parent->parent, NULL);
			if (wled_map)
				pr_info("amberctl: regmap from grandparent %s\n",
					dev_name(dev->parent->parent));
		}
		put_device(dev);
	}
	if (!wled_map)
		pr_warn("amberctl: wled regmap not found, PMIC writes disabled\n");

	dbg_dir = debugfs_create_dir("amberctl", NULL);
	if (!dbg_dir)
		return -ENOMEM;
	debugfs_create_file("cmd", 0200, dbg_dir, NULL, &cmd_fops);

	pr_info("amberctl: ready (regmap %s)\n", wled_map ? "ok" : "missing");
	return 0;
}

static void __exit amberctl_exit(void)
{
	debugfs_remove_recursive(dbg_dir);
}

module_init(amberctl_init);
module_exit(amberctl_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("amber live register/GPIO control");
