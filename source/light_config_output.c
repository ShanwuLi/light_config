#include <string.h>
#include <stdlib.h>
#include "light_config_output.h"

#define LC_HEADER_DEFINE_NAME       "#define "
#define LC_HEADER_DEFINE_NAME_LEN   (strlen("#define "))

/*************************************************************************************
 * @brief: convert config src name to capital
 * 
 * @param dst: destination buffer.
 * @param src: source buffer.
 * 
 * @return: void.
 ************************************************************************************/
static int lc_convert_capital(char *dst, char *src)
{
	int i = 0;
	int char_idx;

	if (strlen(src) == 0) {
		lc_err("Error: src string is empty\n");
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	/* find the '/' idx of src string */
	for (char_idx = strlen(src) - 1; char_idx >= 0; char_idx--) {
		if (src[char_idx] == '/')
			break;
	}

	/* convert src string to dst */
	for (char_idx += 1; char_idx < strlen(src); char_idx++) {
		if (src[char_idx] == '.') {
			dst[i++] = '_';
			continue;
		}

		if (src[char_idx] >= 'a' && src[char_idx] <= 'z')
			dst[i++] = src[char_idx] - 32;
		else
			dst[i++] = src[char_idx];
	}

	dst[i] = '\0';
	return 0;
}

/*************************************************************************************
 * @brief: output merged config to file
 * 
 * @param cfg_list: list of config items.
 * @param line_buffer: line buffer.
 * @param merged_menu_cfg_file: output merged config file info.
 * 
 * @return: zero for success, otherwise failed.
 ************************************************************************************/
static int lc_output_cfg_to_merged_file(struct lc_cfg_list *cfg_list, char *line_buffer,
                                        struct lc_output_file_info merged_menu_cfg_file)
{
	int char_idx;
	char *content;
	size_t line_num = 0;
	FILE *merged_cfg_fp;
	struct lc_cfg_item *pos;

	if (lc_list_is_empty(&cfg_list->node))
		return 0;

	merged_cfg_fp = fopen(merged_menu_cfg_file.name, "w");
	if (merged_cfg_fp == NULL) {
		lc_err("Error: fail to open file, %s\n", merged_menu_cfg_file.name);
		return LC_PARSE_RES_ERR_FILE_NOT_FOUND;
	}

	printf("Configuration written to %s\n", merged_menu_cfg_file.name);
	/* write the prefix of file content to file */
	if (merged_menu_cfg_file.content_prefix != NULL) {
		content = merged_menu_cfg_file.content_prefix;
		fwrite(content, strlen(content), 1, merged_cfg_fp);
	}

	/* output each cfg item to file */
	lc_list_for_each_entry(pos, &cfg_list->node, struct lc_cfg_item, node) {
		char_idx = 0;
		/* write the comment line to file */
		if (!pos->enable || line_num == 0) {
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

		/* write the value to file */
		if (!pos->enable) {
			char_idx = 0;
			/* copy variable name to line buffer */
			memcpy(line_buffer, pos->name, pos->name_len);
			char_idx += pos->name_len;

			if (pos->assign_type == LC_ASSIGN_TYPE_ADDITION) {
				memcpy(line_buffer + char_idx, " += n\n", 6);
				char_idx += 6;
			} else if (pos->assign_type == LC_ASSIGN_TYPE_CONDITIONAL) {
				memcpy(line_buffer + char_idx, " ?= n\n", 6);
				char_idx += 6;
			} else if (pos->assign_type == LC_ASSIGN_TYPE_IMMEDIATE) {
				memcpy(line_buffer + char_idx, " := n\n", 6);
				char_idx += 6;
			} else {
				memcpy(line_buffer + char_idx, " = n\n", 5);
				char_idx += 5;
			}
			
			/* write the line to file */
			fwrite(line_buffer, char_idx, 1, merged_cfg_fp);
		}

		line_num++;
	}

	/* write the suffix of file content to file */
	if (merged_menu_cfg_file.content_suffix != NULL) {
		content = merged_menu_cfg_file.content_suffix;
		fwrite(content, strlen(content), 1, merged_cfg_fp);
	}

	fclose(merged_cfg_fp);
	return 0;
}

/*************************************************************************************
 * @brief: output menu config to make file
 * 
 * @param cfg_list: list of config items.
 * @param line_buffer: line buffer.
 * @param mk_file: output makefile info.
 * 
 * @return: zero for success, otherwise failed.
 ************************************************************************************/
static int lc_output_cfg_to_mk_file(struct lc_cfg_list *cfg_list, char *line_buffer,
                                                 struct lc_output_file_info mk_file)
{
	int char_idx;
	char *content;
	FILE *mk_fp;
	size_t line_num = 0;
	struct lc_cfg_item *pos;

	if (lc_list_is_empty(&cfg_list->node))
		return 0;

	mk_fp = fopen(mk_file.name, "w");
	if (mk_fp == NULL) {
		lc_err("Error: fail to open file, %s\n", mk_file.name);
		return LC_PARSE_RES_ERR_FILE_NOT_FOUND;
	}

	printf("Configuration written to %s\n", mk_file.name);
	/* write the prefix of file content to file */
	if (mk_file.content_prefix != NULL) {
		content = mk_file.content_prefix;
		fwrite(content, strlen(content), 1, mk_fp);
	}

	/* output each cfg item to file */
	lc_list_for_each_entry(pos, &cfg_list->node, struct lc_cfg_item, node) {
		char_idx = 0;
		/* copy variable name to line buffer */
		memcpy(line_buffer + char_idx, pos->name, pos->name_len);
		char_idx += pos->name_len;

		/* write the [no] line to file */
		if (!pos->enable) {
			if (pos->assign_type == LC_ASSIGN_TYPE_DIRECT) {
				memcpy(line_buffer + char_idx, " = n\n", 5);
				char_idx += 5;
				fwrite(line_buffer, char_idx, 1, mk_fp);
			}

			line_num++;
			continue;
		}

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
		fwrite(line_buffer, char_idx, 1, mk_fp);
		line_num++;
	}

	/* write the suffix of file content to file */
	if (mk_file.content_suffix != NULL) {
		content = mk_file.content_suffix;
		fwrite(content, strlen(content), 1, mk_fp);
	}

	fclose(mk_fp);
	return 0;
}

/*************************************************************************************
 * @brief: output menu config to header file
 * 
 * @param cfg_list: list of config items.
 * @param line_buffer: line buffer.
 * @param header_file: output header config file info.
 * 
 * @return: zero for success, otherwise failed.
 ************************************************************************************/
static int lc_output_cfg_to_header_file(struct lc_cfg_list *cfg_list, char *line_buffer,
                                                 struct lc_output_file_info header_file)
{
	int ret;
	int char_idx;
	char *content;
	FILE *header_fp;
	char *define_name;
	size_t line_num = 0;
	struct lc_cfg_item *pos;

	if (lc_list_is_empty(&cfg_list->node))
		return 0;

	define_name = malloc(strlen(header_file.name) + 1);
	if (define_name == NULL) {
		lc_err("Error: fail to malloc memory\n");
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	ret = lc_convert_capital(define_name, header_file.name);
	if (ret < 0) {
		free(define_name);
		return ret;
	}

	header_fp = fopen(header_file.name, "w");
	if (header_fp == NULL) {
		free(define_name);
		lc_err("Error: fail to open file, %s\n", header_file.name);
		return LC_PARSE_RES_ERR_FILE_NOT_FOUND;
	}

	printf("Configuration written to %s\n", header_file.name);
	/* write the prefix of file content to file */
	if (header_file.content_prefix != NULL) {
		content = header_file.content_prefix;
		fwrite(content, strlen(content), 1, header_fp);
	}

	fprintf(header_fp, "#ifndef __%s__\n#define __%s__\n\n", define_name, define_name);

	/* output each cfg item to file */
	lc_list_for_each_entry(pos, &cfg_list->node, struct lc_cfg_item, node) {

		char_idx = 0;
		/* skip invalid cfg item */
		if ((!pos->enable) || (pos->assign_type != LC_ASSIGN_TYPE_DIRECT)) {
			line_num++;
			continue;
		}

		/* copy '#define ' to line buffer */
		memcpy(line_buffer + char_idx, LC_HEADER_DEFINE_NAME, LC_HEADER_DEFINE_NAME_LEN);
		char_idx += LC_HEADER_DEFINE_NAME_LEN;

		/* copy variable name to line buffer */
		memcpy(line_buffer + char_idx, pos->name, pos->name_len);
		char_idx += pos->name_len;

		if (pos->value[0] == 'y' && pos->value_len == 1) {
			/* write the line to file */
			line_buffer[char_idx++] = '\n';
			fwrite(line_buffer, char_idx, 1, header_fp);
			line_num++;
			continue;
		}

		line_buffer[char_idx++] = ' ';
		line_buffer[char_idx++] = ' ';
		line_buffer[char_idx++] = ' ';

		/* copy variable value to line buffer */
		memcpy(line_buffer + char_idx, pos->value, pos->value_len);
		char_idx += pos->value_len;
		line_buffer[char_idx++] = '\n';

		/* write the line to file */
		fwrite(line_buffer, char_idx, 1, header_fp);
		line_num++;
	}

	fprintf(header_fp, "\n#endif /* __%s__*/\n", define_name);

	/* write the suffix of file content to file */
	if (header_file.content_suffix != NULL) {
		content = header_file.content_suffix;
		fwrite(content, strlen(content), 1, header_fp);
	}

	fclose(header_fp);
	free(define_name);
	return 0;
}

/*************************************************************************************
 * @brief: output cfg item to merged, makefile and header config file.
 * 
 * @param ctrl_blk: control block.
 * @param merged_menu_cfg_file: output merged config file info.
 * @param mk_file: output makefile info.
 * @param header_file: output header config file info.
 * 
 * @return: zero for success, otherwise failed.
 ************************************************************************************/
int light_config_output_cfg_to_file(struct lc_ctrl_blk *ctrl_blk,
                 struct lc_output_file_info merged_menu_cfg_file,
                 struct lc_output_file_info mk_file,
                 struct lc_output_file_info header_file)
{
	int ret;

	printf("Configuration writing starts...\n");
	/* output the menu configuration item to file */
	ret = lc_output_cfg_to_merged_file(&ctrl_blk->menu_cfg_head,
	                 ctrl_blk->temp_buff, merged_menu_cfg_file);
	if (ret < 0) {
		lc_err("Error: out menu cfg to %s file failed, ret:%d\n",
		        merged_menu_cfg_file.name, ret);
		return ret;
	}

	/* output the menu configuration item to mk file */
	ret = lc_output_cfg_to_mk_file(&ctrl_blk->menu_cfg_head,
	                     ctrl_blk->temp_buff, mk_file);
	if (ret < 0) {
		lc_err("Error: out menu cfg to %s file failed, ret:%d\n",
		        mk_file.name, ret);
		return ret;
	}

	/* output the menu configuration item to header file */
	ret = lc_output_cfg_to_header_file(&ctrl_blk->menu_cfg_head,
	                          ctrl_blk->temp_buff, header_file);
	if (ret < 0) {
		lc_err("Error: out menu cfg to %s file failed, ret:%d\n",
		        header_file.name, ret);
		return ret;
	}

	printf("Configuration writing done\n");
	return 0;
}
