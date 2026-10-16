ccflags-y += -I$(srctree)/drivers/input/touchscreen/icnl9916_tddi_mmi \
	     -I$(srctree)/drivers/input/touchscreen/mediatek \
	     -I$(srctree)/drivers/input/touchscreen/mmi_panelinfo

ccflags-y += -DCTS_MTK_GET_PANEL

icnl9916_tddi_mmi-objs += cts_i2c_driver.o
icnl9916_tddi_mmi-objs += cts_core.o
icnl9916_tddi_mmi-objs += cts_platform.o
icnl9916_tddi_mmi-objs += cts_sfctrlv2.o
icnl9916_tddi_mmi-objs += cts_spi_flash.o
icnl9916_tddi_mmi-objs += cts_firmware.o
icnl9916_tddi_mmi-objs += cts_tool.o
icnl9916_tddi_mmi-objs += cts_sysfs.o
icnl9916_tddi_mmi-objs += cts_test.o
icnl9916_tddi_mmi-objs += cts_charger_detect.o
icnl9916_tddi_mmi-objs += cts_tcs.o
icnl9916_tddi_mmi-objs += cts_hostcomm.o

obj-$(CONFIG_INPUT_TOUCHSCREEN_ICNL9916_TDDI_MMI) += icnl9916_tddi_mmi.o
