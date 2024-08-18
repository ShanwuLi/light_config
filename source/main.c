#include <stdio.h>
#include <string.h>
#include "logic_expr_parse.h"
#include "light_config_parse.h"

static int num_of_parse_calls = 0;
static struct lc_ctrl_blk lc_cb;
int lm_read_default_cfg(struct lc_ctrl_blk *cb, char *cfg_file)
{
	int ret;
	char *pch;
	FILE *fp;
	static int line_num = 0;

	num_of_parse_calls++;
	printf("+++++++++++++++++++++++++++++++++++++++num_of_parse_calls:%d\n", num_of_parse_calls);
	if (num_of_parse_calls > 1000)
		return -1000;

	if (cfg_file == NULL) {
		return -88;
	}

	fp = fopen(cfg_file, "r");
	if (fp == NULL) {
		printf("Fail to open file %s\n", cfg_file);
		return -8;
	}

	line_num = 0;
	while (pch = fgets(cb->line_buff, 2048, fp)) {
		line_num++;

		/* skip comment line and empty line */
		if (cb->line_buff[0] == '#' || cb->line_buff[0] == '\n') {
			memset(cb->line_buff, 0, strlen(cb->line_buff) + 1);
			continue;
		}

		ret = light_config_parse_line(cb);
		if (ret != LC_PASER_RES_OK_INCLUDE && ret != LC_PASER_RES_OK_DEPEND_CFG && ret != LC_PASER_RES_OK_NORMAL_CFG) {
			printf("Fail to parse %s default cfg line %d, col[%d], ret:%d\n", cfg_file, line_num, cb->colu_num, ret);
			goto out;
		}

		if (ret == LC_PASER_RES_OK_INCLUDE) {
			printf("parsing subfile:%s\n", cb->inc_file_path);
			ret = lm_read_default_cfg(cb, cb->inc_file_path);
			if (ret < 0)
				goto out;
		}
	}

	ret = 0;

out:
	fclose(fp);
	return ret;
}

int main(int argc, char *argv[])
{
	int ret;


	ret = light_config_init(&lc_cb, 16 * 1024 * 1024, 2048);
	printf("ret = %d\n", ret);

	ret = lm_read_default_cfg(&lc_cb, "test.cfg");
	printf("ret:%d\n", ret);
	return 0;
}
