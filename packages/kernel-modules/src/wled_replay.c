// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/spmi.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/backlight.h>

static int brightness = 4095;
module_param(brightness, int, 0644);
static int settle_ms = 50;
module_param(settle_ms, int, 0644);

static struct regmap *map;

static void wr(u16 reg, u8 val)
{
	int rc = regmap_write(map, reg, val);
	if (rc)
		pr_err("wled_replay: write %04x=%02x failed %d\n", reg, val, rc);
}

static int __init wled_replay_init(void)
{
	struct backlight_device *bd;
	struct device *d;
	u32 lvl = clamp_t(int, brightness, 0, 4095);
	u8 lsb = lvl & 0xff, msb = (lvl >> 8) & 0x0f;
	int i, rc;
	u32 v = 0;

	bd = backlight_device_get_by_name("backlight");
	pr_info("wled_replay: bd=%p\n", bd);
	if (!bd)
		return -ENODEV;
	d = bd->dev.parent;
	pr_info("wled_replay: bd parent=%s\n", d ? dev_name(d) : "?");
	if (!d || !d->parent)
		return -ENODEV;
	pr_info("wled_replay: wled parent=%s\n", dev_name(d->parent));
	map = dev_get_regmap(d->parent, NULL);
	pr_info("wled_replay: map=%p\n", map);
	if (!map)
		return -ENODEV;

	rc = regmap_read(map, 0xd846, &v);
	pr_info("wled_replay: test read d846 rc=%d val=%02x\n", rc, v);
	if (rc)
		return rc;

	pr_info("wled_replay: start level=%u\n", lvl);
	wr(0xd846, 0x00);
	msleep(20);
	wr(0xd946, 0x00);
	for (i = 0; i < 4; i++) {
		u16 b = 0xd950 + i * 0x10;
		wr(b + 0, 0x80);
		wr(b + 1, 0x04);
		wr(b + 2, 0x08);
		wr(b + 3, 0x00);
		wr(b + 6, 0x00);
		wr(b + 7, lsb);
		wr(b + 8, msb);
	}
	wr(0xd94a, 0x4c);
	wr(0xd94b, 0x03);
	wr(0xd84c, 0x8b);
	wr(0xd84d, 0x01);
	wr(0xd84e, 0x03);
	wr(0xd850, 0x07);
	wr(0xd851, 0x01);
	wr(0xd852, 0x01);
	wr(0xd853, 0x02);
	wr(0xd854, 0x03);
	wr(0xd855, 0x04);
	wr(0xd856, 0x09);
	wr(0xd857, 0x03);
	wr(0xd85e, 0x82);
	wr(0xd8e4, 0x3f);
	wr(0xd8e5, 0x32);
	wr(0xd8f0, 0x04);
	wr(0xd8f1, 0x1f);
	wr(0xd8f2, 0x03);
	wr(0xd8f3, 0x06);
	wr(0xd8f4, 0x09);
	wr(0xd0, 0xa5);
	wr(0xd8e2, 0x09);
	wr(0xd946, 0xf0);
	wr(0xd947, 0x0f);
	udelay(1000);
	wr(0xd947, 0x00);
	wr(0xd846, 0x80);
	msleep(10);

	{
		u32 f = 0, r = 0, en = 0;
		rc = regmap_read(map, 0xd808, &f);
		regmap_read(map, 0xd810, &r);
		regmap_read(map, 0xd846, &en);
		pr_info("wled_replay: done rc=%d d846=%02x d808=%02x d810=%02x\n",
			rc, en, f, r);
	}
	return 0;
}

static void __exit wled_replay_exit(void) { }

module_init(wled_replay_init);
module_exit(wled_replay_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Replay Android WLED register state");
