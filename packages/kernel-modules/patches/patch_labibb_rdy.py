import sys

p = "/root/linux-msm8953/drivers/regulator/qcom-labibb-regulator.c"
s = open(p, encoding="utf-8").read()
orig = s


def rep(old, new, label):
    global s
    if new in s:
        print("SKIP", label)
        return
    if s.count(old) != 1:
        print("FAIL count=%d" % s.count(old), label)
        sys.exit(1)
    s = s.replace(old, new, 1)
    print("ok", label)


rep(
    "#define REG_LABIBB_ENABLE_CTL\t\t0x46\n"
    " #define LABIBB_CONTROL_ENABLE\t\tBIT(7)",
    "#define REG_LABIBB_MODULE_RDY\t\t0x45\n"
    " #define LABIBB_MODULE_RDY_EN\t\tBIT(7)\n"
    "#define REG_LABIBB_ENABLE_CTL\t\t0x46\n"
    " #define LABIBB_CONTROL_ENABLE\t\tBIT(7)",
    "defines",
)

rep(
    "static const struct regulator_ops qcom_labibb_ops = {\n"
    "\t.enable\t\t\t= regulator_enable_regmap,",
    "static int qcom_labibb_enable(struct regulator_dev *rdev)\n"
    "{\n"
    "\tstruct labibb_regulator *vreg = rdev_get_drvdata(rdev);\n"
    "\tint ret;\n"
    "\n"
    "\t/* PMI8950: mark the module ready before setting ENABLE */\n"
    "\tret = regmap_update_bits(rdev->regmap,\n"
    "\t\t\t\t vreg->base + REG_LABIBB_MODULE_RDY,\n"
    "\t\t\t\t LABIBB_MODULE_RDY_EN, LABIBB_MODULE_RDY_EN);\n"
    "\tif (ret)\n"
    "\t\treturn ret;\n"
    "\n"
    "\treturn regulator_enable_regmap(rdev);\n"
    "}\n"
    "\n"
    "static const struct regulator_ops qcom_labibb_ops = {\n"
    "\t.enable\t\t\t= qcom_labibb_enable,",
    "enable-fn",
)

if s == orig:
    print("nothing changed")
    sys.exit(0)
open(p, "w", encoding="utf-8").write(s)
print("wrote")
