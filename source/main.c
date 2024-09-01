//#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "logic_expr_parse.h"
#include "light_config_parse.h"
#include "light_config_output.h"

#define mit_license_for_mk_file "\
# MIT License\
\n# Copyright (c) 2024 light config  https://gitee.com/lsw-elecm/light_config\
\n# \
\n# Permission is hereby granted, free of charge, to any person obtaining a copy\
\n# of this software and associated documentation files (the \"Software\"), to deal\
\n# in the Software without restriction, including without limitation the rights\
\n# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\
\n# copies of the Software, and to permit persons to whom the Software is\
\n# furnished to do so, subject to the following conditions:\
\n# \
\n# The above copyright notice and this permission notice shall be included in all\
\n# copies or substantial portions of the Software.\
\n# \
\n# THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\
\n# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\
\n# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\
\n# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\
\n# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\
\n# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\
\n# SOFTWARE.\n\n"

#define mit_license_for_header_file "\
/*\
\nMIT License\
\nCopyright (c) 2024 light config https://gitee.com/lsw-elecm/light_config\
\n\
\nPermission is hereby granted, free of charge, to any person obtaining a copy\
\nof this software and associated documentation files (the \"Software\"), to deal\
\nin the Software without restriction, including without limitation the rights\
\nto use, copy, modify, merge, publish, distribute, sublicense, and/or sell\
\ncopies of the Software, and to permit persons to whom the Software is\
\nfurnished to do so, subject to the following conditions:\
\n\
\nThe above copyright notice and this permission notice shall be included in all\
\ncopies or substantial portions of the Software.\
\n\
\nTHE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\
\nIMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\
\nFITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\
\nAUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\
\nLIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\
\nOUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\
\nSOFTWARE.\
\n*/\n\n"

#define LC_ARRAR_SIZE(a)    (sizeof(a) / sizeof(a[0]))

struct lc_args {
	char *name;
	char *value;
	char *desc;
};

static struct lc_args g_lc_args[] = {
	{
		.name = "-h",
		.value = NULL,
		.desc = "show help",
	},
	{
		.name = "--help",
		.value = NULL,
		.desc = "show help",
	},
	{
		.name = "--menu_cfg",
		.value = "menu.lc",
		.desc = "specify menu config file, menu.lc as default",
	},
	{
		.name = "--default_cfg",
		.value = "default.lc",
		.desc = "specify default config file, default.lc as default",
	},
	{
		.name = "--out_merged_cfg",
		.value = NULL,
		.desc = "specify merged config file, .lc as default",
	},
	{
		.name = "--out_mk_file",
		.value = NULL,
		.desc = "specify output mk file, config.mk as default",
	},
	{
		.name = "--out_h_file",
		.value = NULL,
		.desc = "specify output header file, config.h as default",
	},
};

static struct lc_ctrl_blk lc_cb;

static struct lc_output_file_info output_merged_menu_cfg_file = {
	.name = ".lc",
	.content_prefix = NULL,
	.content_suffix = NULL,
};

static struct lc_output_file_info output_mk_file = {
	.name = "config.mk",
	.content_prefix = mit_license_for_mk_file,
	.content_suffix = NULL,
};

static struct lc_output_file_info output_header_file = {
	.name = "config.h",
	.content_prefix = mit_license_for_header_file,
	.content_suffix = NULL,
};

static void lc_help(void)
{
	printf("Usage:\n");

	for (int i = 0; i < LC_ARRAR_SIZE(g_lc_args); i++)
		printf("%-20s %s\n", g_lc_args[i].name, g_lc_args[i].desc);
}

static bool lc_argv_is_matched(char *argv, char *name)
{
	int i = 0;

	if (strlen(argv) < (strlen(name) + 1) || argv[strlen(name)] != '=')
		return false;

	for (i = 0; i < strlen(name); i++) {
		if (argv[i] != name[i])
			return false;
	}

	return true;
}

static int lc_parse_argvs(int argc, char *argv[])
{
	int i;
	int j;
	bool matched = false;

	for (i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0))
			return -1;
	}

	for (i = 1; i < argc; i++) {
		for (j = 0; j < LC_ARRAR_SIZE(g_lc_args); j++) {
			matched = lc_argv_is_matched(argv[i], g_lc_args[j].name);
			if (matched) {
				g_lc_args[j].value = argv[i] + strlen(g_lc_args[j].name) + 1;
				break;
			}
		}

		if (!matched)
			return -1;
	}

	/* must specify menu_cfg and default_cfg */
	if (g_lc_args[2].value == NULL || g_lc_args[3].value == NULL)
		return -1;
	
	/* set output file name */
	if (g_lc_args[4].value != NULL)
		output_merged_menu_cfg_file.name = g_lc_args[4].value;
	
	if (g_lc_args[5].value != NULL)
		output_mk_file.name = g_lc_args[5].value;
	
	if (g_lc_args[6].value != NULL)
		output_header_file.name = g_lc_args[6].value;

	return 0;
}

int main(int argc, char *argv[])
{
	int ret;

	ret = lc_parse_argvs(argc, argv);
	if (ret < 0) {
		lc_help();
		return ret;
	}

	ret = light_config_init(&lc_cb, 128 * 1024 * 1024, 8192, 10240, 16 * 1024 * 1024);
	if (ret < 0) {
		lc_err("Fail to init light config, ret:%d\n", ret);
		return ret;
	}

	ret = light_config_parse_cfg_file(&lc_cb, g_lc_args[3].value, true);
	if (ret < 0) {
		printf("Fail to parse default cfg file, ret:%d\n", ret);
		goto out;
	}

	ret = light_config_parse_cfg_file(&lc_cb, g_lc_args[2].value, false);
	if (ret < 0) {
		printf("Fail to parse menu cfg file, ret:%d\n", ret);
		goto out;
	}

	printf("Parse done.\n");
	ret = light_config_output_cfg_to_file(&lc_cb, output_merged_menu_cfg_file,
	                                      output_mk_file, output_header_file);
	if (ret < 0) {
		lc_err("Fail to output cfg file, ret:%d\n", ret);
		goto out;
	}

	printf("Genarate done.\n");

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
	light_config_deinit(&lc_cb);
	return ret;
}
