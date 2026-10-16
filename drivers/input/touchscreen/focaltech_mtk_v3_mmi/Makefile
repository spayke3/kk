ccflags-y += -I$(srctree)/drivers/input/touchscreen/focaltech_mtk_v3_mmi \
	     -I$(srctree)/drivers/input/touchscreen/mediatek \
	     -I$(srctree)/drivers/input/touchscreen/mmi_panelinfo

ccflags-y += -DFOCALTECH_FB_PANEL_EN

focaltech_mtk_v3_mmi-objs += focaltech_core.o
focaltech_mtk_v3_mmi-objs += focaltech_ex_fun.o
focaltech_mtk_v3_mmi-objs += focaltech_ex_mode.o
focaltech_mtk_v3_mmi-objs += focaltech_gesture.o
focaltech_mtk_v3_mmi-objs += focaltech_esdcheck.o
focaltech_mtk_v3_mmi-objs += focaltech_point_report_check.o
focaltech_mtk_v3_mmi-objs += focaltech_flash.o
focaltech_mtk_v3_mmi-objs += focaltech_spi.o

obj-$(CONFIG_INPUT_TOUCHSCREEN_FTS_MTK_V3_MMI) := focaltech_mtk_v3_mmi.o
