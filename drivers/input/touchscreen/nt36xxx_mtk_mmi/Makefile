ccflags-y += -I$(srctree)/drivers/input/touchscreen/nt36xxx_mtk_mmi \
	     -I$(srctree)/drivers/input/touchscreen/mediatek \
	     -I$(srctree)/drivers/input/touchscreen/mmi_panelinfo

nt36xxx_mtk_mmi-objs += nt36xxx.o
nt36xxx_mtk_mmi-objs += nt36xxx_ext_proc.o
nt36xxx_mtk_mmi-objs += nt36xxx_fw_update.o
nt36xxx_mtk_mmi-objs += nt36xxx_mp_ctrlram.o

obj-$(CONFIG_INPUT_TOUCHSCREEN_NT36XXX_MTK_MMI) += nt36xxx_mtk_mmi.o
