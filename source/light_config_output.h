#ifndef __LIGHT_CONFIG_OUTPUT_H__
#define __LIGHT_CONFIG_OUTPUT_H__

#include "light_config_parse.h"

struct lc_output_file_info {
	char *name;
	char *content_prefix;
	char *content_suffix;
};

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
                 struct lc_output_file_info header_file);

#endif
