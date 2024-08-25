#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "logic_expr_parse.h"
#include "light_config_parse.h"

static struct lc_ctrl_blk lc_cb;

int main(int argc, char *argv[])
{
	int ret;
	time_t rawtime;
	struct tm * timeinfo;

	/* get the time */
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	printf("start: %s", asctime(timeinfo));

	ret = light_config_init(&lc_cb, 128 * 1024 * 1024, 8192, 16, 16 * 1024 * 1024);
	printf("ret = %d\n", ret);

	ret = light_config_parse_cfg_file(&lc_cb, "test_default.cfg", true);
	if (ret < 0) {
		printf("Fail to parse default cfg file, ret:%d\n", ret);
	}

	ret = light_config_parse_cfg_file(&lc_cb, "test_menu.cfg", false);
	if (ret < 0) {
		printf("Fail to parse menu cfg file, ret:%d\n", ret);
	}

	printf("mem_used:%llu, ret:%d\n", lc_cb.mem_blk_ctrl.used, ret);
	lc_dump_cfg(&lc_cb.default_cfg_head);
	printf("menu cfg dump:\n\n\n");
	lc_dump_cfg(&lc_cb.menu_cfg_head);

	/* get the time */
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	printf("end: %s", asctime(timeinfo));

	return ret;
}
