#include "light_config_output.h"
#include <string.h>

/*************************************************************************************
 * @brief: output merged config to file
 * 
 * @param cfg_list: list of config items.
 * @param line_buffer: line buffer.
 * @param merged_cfg_file_name: merged config file name.
 * 
 * @return: zero for success, otherwise failed.
 ************************************************************************************/
static int lc_output_merged_cfg_to_file(struct lc_cfg_list *cfg_list,
                       char *line_buffer, char *merged_cfg_file_name)
{
	int char_idx;
	FILE *merged_cfg_fp;
	struct lc_cfg_item *pos;

	if (lc_list_is_empty(&cfg_list->node))
		return 0;

	merged_cfg_fp = fopen(merged_cfg_file_name, "w");
	if (merged_cfg_fp == NULL) {
		printf("Fail to open file, %s\n", merged_cfg_file_name);
		return LC_PASER_RES_ERR_FILE_NOT_FOUND;
	}

	printf("Writing %s\n", merged_cfg_file_name);
	lc_list_for_each_entry(pos, &cfg_list->node, struct lc_cfg_item, node) {

		char_idx = 0;
		if (!pos->enable) {
			line_buffer[char_idx++] = '#';
			line_buffer[char_idx++] = ' ';
		}

		/* copy variable name to line buffer */
		memcpy(line_buffer + char_idx, pos->name, pos->name_len);
		char_idx += pos->name_len;

		line_buffer[char_idx++] = ' ';
		/* copy variable value to line buffer */
		if (pos->assign_type == LC_ASSIGN_TYPE_IMMEDIATE)
			line_buffer[char_idx++] = ':';

		if (pos->assign_type == LC_ASSIGN_TYPE_CONDITIONAL)
			line_buffer[char_idx++] = '?';

		if (pos->assign_type == LC_ASSIGN_TYPE_ADDITION)
			line_buffer[char_idx++] = '+';

		line_buffer[char_idx++] = '=';
		line_buffer[char_idx++] = ' ';

		/* copy variable value to line buffer */
		memcpy(line_buffer + char_idx, pos->value, pos->value_len);
		char_idx += pos->value_len;
		line_buffer[char_idx++] = '\n';

		/* write the line to file */
		fwrite(line_buffer, char_idx, 1, merged_cfg_fp);
	}

	fclose(merged_cfg_fp);
	return 0;
}

/*************************************************************************************
 * @brief: output cfg item to merged, makefile and header config file.
 * 
 * @param ctrl_blk: control block.
 * @param mk_file_name: makefile name.
 * @param head_file_name: header config file name.
 * @param merged_cfg_file_name: merged config file name.
 * @param merged_default_cfg_file_name: merged default config file name.
 * 
 * @return: zero for success, otherwise failed.
 ************************************************************************************/
int light_config_output_cfg_to_file(struct lc_ctrl_blk *ctrl_blk,
                          char *mk_file_name, char *head_file_name,
                          char *merged_menu_cfg_file_name,
                          char *merged_default_cfg_file_name)
{
	int ret;

	/* output the default configuration item to file */
	ret = lc_output_merged_cfg_to_file(&ctrl_blk->default_cfg_head,
	            ctrl_blk->temp_buff, merged_default_cfg_file_name);
	if (ret < 0) {
		printf("Out default cfg to file failed, ret:%d\n", ret);
		return ret;
	}

	/* output the menu configuration item to file */
	ret = lc_output_merged_cfg_to_file(&ctrl_blk->menu_cfg_head,
	            ctrl_blk->temp_buff, merged_menu_cfg_file_name);
	if (ret < 0) {
		printf("Out menu cfg to file failed, ret:%d\n", ret);
		return ret;
	}

	return 0;
}
