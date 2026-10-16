ccflags-y += -I$(srctree)/drivers/input/touchscreen/ili9882_mmi \
	     -I$(srctree)/drivers/input/touchscreen/mediatek \
		 -I$(srctree)/drivers/input/touchscreen/mmi_panelinfo

ccflags-y += -DILI_CONFIG_RESUME_BY_DDI
ccflags-y += -DILI_MTK_GET_PANEL

ili9882_mmi-objs += ili9882.o
ili9882_mmi-objs += ili9882_spi.o
ili9882_mmi-objs += ili9882_qcom.o
ili9882_mmi-objs += ili9882_ic.o
ili9882_mmi-objs += ili9882_touch.o
ili9882_mmi-objs += ili9882_mp.o
ili9882_mmi-objs += ili9882_hostdl.o
ili9882_mmi-objs += ili9882_node.o

obj-$(CONFIG_INPUT_TOUCHSCREEN_ILI9882_MMI) += ili9882_mmi.o
