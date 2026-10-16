ccflags-y += -I$(srctree)/drivers/input/touchscreen/nova_0flash_mmi \
	     -I$(srctree)/drivers/input/touchscreen/mediatek \
		 -I$(srctree)/drivers/input/touchscreen/mmi_panelinfo

ccflags-y += -DNVT_MTK_GET_PANEL

nova_0flash_mmi-objs += nt36xxx.o
nova_0flash_mmi-objs += nt36xxx_ext_proc.o
nova_0flash_mmi-objs += nt36xxx_fw_update.o
nova_0flash_mmi-objs += nt36xxx_mp_ctrlram.o

obj-$(CONFIG_INPUT_TOUCHSCREEN_NOVA_0FLASH_MMI) += nova_0flash_mmi.o
