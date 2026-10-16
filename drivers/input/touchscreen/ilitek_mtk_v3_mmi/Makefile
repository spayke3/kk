ccflags-y += -I$(srctree)/drivers/input/touchscreen/ilitek_mtk_v3_mmi \
	     -I$(srctree)/drivers/input/touchscreen/mediatek \
	     -I$(srctree)/drivers/input/touchscreen/mmi_panelinfo

ilitek_mtk_v3_mmi-objs += ilitek_v3.o
ilitek_mtk_v3_mmi-objs += ilitek_v3_spi.o
ilitek_mtk_v3_mmi-objs += ilitek_v3_mtk.o
ilitek_mtk_v3_mmi-objs += ilitek_v3_ic.o
ilitek_mtk_v3_mmi-objs += ilitek_v3_touch.o
ilitek_mtk_v3_mmi-objs += ilitek_v3_mp.o
ilitek_mtk_v3_mmi-objs += ilitek_v3_hostdl.o
ilitek_mtk_v3_mmi-objs += ilitek_v3_node.o

obj-$(CONFIG_INPUT_TOUCHSCREEN_ILITEK_MTK_V3_MMI) += ilitek_mtk_v3_mmi.o
