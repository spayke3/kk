ccflags-y += -I$(srctree)/drivers/input/touchscreen/himax_mtk_mmi \
	     -I$(srctree)/drivers/input/touchscreen/mediatek \
		 -I$(srctree)/drivers/input/touchscreen/mmi_panelinfo

ccflags-y += -DCONFIG_TOUCHSCREEN_HIMAX_DEBUG \
		-DCONFIG_TOUCHSCREEN_HIMAX_INSPECT
ccflags-y += -DCONFIG_TOUCHSCREEN_HIMAX_INCELL
ccflags-y += -DCONFIG_HMX_DB

himax_mtk_mmi-objs += himax_platform.o
himax_mtk_mmi-objs += himax_common.o
himax_mtk_mmi-objs += himax_debug.o
himax_mtk_mmi-objs += himax_ic_incell_core.o
himax_mtk_mmi-objs += himax_inspection.o
himax_mtk_mmi-objs += himax_ic_HX83112.o

obj-$(CONFIG_INPUT_TOUCHSCREEN_HIMAX_MTK_MMI) += himax_mtk_mmi.o
