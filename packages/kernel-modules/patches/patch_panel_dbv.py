import sys

p = "/root/linux-msm8953/drivers/gpu/drm/panel/msm8953-generated/panel-lenovo-cd-18781y-hx83100a.c"
s = open(p, encoding="utf-8").read()
orig = s

old = '\tmipi_dsi_usleep_range(&dsi_ctx, 5000, 6000);\n\n\tdev_info(&ctx->dsi->dev, "amber: panel_on done, accum_err=%d\\n",'
new = ('\tmipi_dsi_usleep_range(&dsi_ctx, 5000, 6000);\n'
       '\n'
       '\t/* amber: panel DBV (integrated backlight control test) */\n'
       '\tmipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x51, 0xff);\n'
       '\tdev_info(&ctx->dsi->dev, "amber: DBV 0x51=0xff written\\n");\n'
       '\n'
       '\tdev_info(&ctx->dsi->dev, "amber: panel_on done, accum_err=%d\\n",')
if new in s:
    print("already applied")
    sys.exit(0)
if s.count(old) != 1:
    print("FAIL count", s.count(old))
    sys.exit(1)
s = s.replace(old, new, 1)
open(p, "w", encoding="utf-8").write(s)
print("wrote")
