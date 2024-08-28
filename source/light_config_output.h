#ifndef __LIGHT_CONFIG_OUTPUT_H__
#define __LIGHT_CONFIG_OUTPUT_H__

#include "light_config_parse.h"

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
                          char *merged_default_cfg_file_name);

#endif
