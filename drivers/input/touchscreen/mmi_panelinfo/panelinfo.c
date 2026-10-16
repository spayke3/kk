// SPDX-License-Identifier: GPL-2.0-only

#include <linux/string.h>
#include <linux/printk.h>
#include <linux/init.h>

char active_panel_name[50] = {0};

int tpd_get_panel(void)
{
	if (strlen(active_panel_name)) {
		pr_debug("%s: got active_panel_name=%s\n", __func__, active_panel_name);
		return 0;
	}

	//bringup, parse panel name from cmdline
	pr_debug("%s: enter\n", __func__);
	if (saved_command_line) {
		char *sub;
		char key_prefix[] = "mipi_mot_vid_";

		pr_debug("%s: saved_command_line is %s\n", __func__, saved_command_line);
		sub = strstr(saved_command_line, key_prefix);
		if (sub) {
			char *d;
			int n, len, len_max = 50;

			d = strstr(sub, " ");
			if (d) {
				n = strlen(sub) - strlen(d);
			} else {
				n = strlen(sub);
			}

			if (n > len_max)
				len = len_max;
			else
				len = n;

			strncpy(active_panel_name, sub, len);
			pr_debug("%s: active_panel_name=%s\n", __func__, active_panel_name);

		} else {
			pr_err("%s: active panel not found!", __func__);
			return -1;
		}
	} else {
		pr_err("%s: saved_command_line null!", __func__);
		return -1;
	}

	return 0;
}
