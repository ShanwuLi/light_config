#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "logic_expr_parse.h"
#include "light_config_parse.h"
#include "light_config_output.h"

static struct lc_ctrl_blk lc_cb;

static struct lc_output_file_info output_merged_menu_cfg_file = {
	.name = ".lc",
	.content_prefix = NULL,
	.content_suffix = NULL,
};

static struct lc_output_file_info output_mk_file = {
	.name = "config.mk",
	.content_prefix = NULL,
	.content_suffix = NULL,
};

static struct lc_output_file_info output_header_file = {
	.name = "config.h",
	.content_prefix = "#ifndef __CONFIG_H__\n#define __CONFIG_H__\n\n",
	.content_suffix = "\n#endif /* __CONFIG_H__ */\n",
};

int main(int argc, char *argv[])
{
	int ret;

	if (argc != 3) {
		printf("Usage: %s <menu config file> <default config file>\n", argv[0]);
		return 0;
	}

	ret = light_config_init(&lc_cb, 128 * 1024 * 1024, 8192, 10240, 16 * 1024 * 1024);
	if (ret < 0) {
		lc_err("Fail to init light config, ret:%d\n", ret);
		return ret;
	}

	ret = light_config_parse_cfg_file(&lc_cb, argv[2], true);
	if (ret < 0) {
		printf("Fail to parse default cfg file, ret:%d\n", ret);
		goto out;
	}

	ret = light_config_parse_cfg_file(&lc_cb, argv[1], false);
	if (ret < 0) {
		printf("Fail to parse menu cfg file, ret:%d\n", ret);
		goto out;
	}

	ret = light_config_output_cfg_to_file(&lc_cb, output_merged_menu_cfg_file,
	                                      output_mk_file, output_header_file);
	if (ret < 0) {
		lc_err("Fail to output cfg file, ret:%d\n", ret);
		goto out;
	}

	/* get the time */
	// time_t rawtime;
	// struct tm * timeinfo;

	// time(&rawtime);
	// timeinfo = localtime(&rawtime);
	// printf("start: %s", asctime(timeinfo));

	//printf("mem_used:%llu, ret:%d\n", lc_cb.mem_blk_ctrl.used, ret);
	//lc_dump_cfg(&lc_cb.default_cfg_head);
	//lc_dump_cfg(&lc_cb.menu_cfg_head);

	/* get the time */
	// time(&rawtime);
	// timeinfo = localtime(&rawtime);
	// printf("end: %s", asctime(timeinfo));

out:
	light_config_free(&lc_cb);
	return ret;
}
