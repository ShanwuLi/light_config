#ifndef __LIGHT_CONFIG_H__
#define __LIGHT_CONFIG_H__

#include <stdbool.h>
#include <stdint.h>

#define LC_DEBUG
#define LC_LINE_BUFF_SIZE                  (2048)
#define LC_LINE_BUFFS_GAP                  (16)
#define LC_MEM_UPLIMIT                     (128 * 1024 * 1024)

enum lc_parse_res {
	/* error code */
	LC_PARSE_RES_ERR_CTRL_BLK_INVALID = -99,
	LC_PARSE_RES_ERR_CFG_ITEM_INVALID,
	LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND,
	LC_PARSE_RES_ERR_INCLUDE_INVALID,
	LC_PARSE_RES_ERR_LINE_BUFF_OVERFLOW,
	LC_PARSE_RES_ERR_MEMORY_FAULT,
	LC_PARSE_RES_ERR_MEMORY_OVERFLOW,
	LC_PARSE_RES_ERR_MENU_CFG_INVALID,
	LC_PARSE_RES_ERR_DIRECT_CFG_INVALID,
	LC_PARSE_RES_ERR_NORMAL_CFG_INVALID,
	LC_PASER_RES_ERR_FILE_NOT_FOUND,
	/* ok code */
	LC_PASER_RES_OK_DEPEND_CFG = 0,
	LC_PASER_RES_OK_NORMAL_CFG,
	LC_PASER_RES_OK_INCLUDE,
};

enum lc_assign_type {
	LC_ASSIGN_TYPE_DIRECT,       /*  = */
	LC_ASSIGN_TYPE_IMMEDIATE,    /* := */
	LC_ASSIGN_TYPE_CONDITIONAL,  /* ?= */
	LC_ASSIGN_TYPE_ADDITION,     /* += */
};

struct lc_list_node {
	struct lc_list_node *next;
	struct lc_list_node *prev;
};

struct lc_cfg_item {
	uint64_t name_hashval;
	uint32_t assign_type;
	uint32_t name_len;
	uint32_t value_len;
	bool enable;
	char *name;
	char *value;
	struct lc_list_node node;
};

/*************************************************************************************
 * @brief: control block of the config file.
 * 
 * @param mem_uplimit: memory uplimit, if over the memory uplimit, return error.
 * @param mem_used: memory used.
 * @param line_buff_size: size of the line buff.
 * @param menu_cfg_head: head of the menu list of the item.
 * @param default_cfg_head: head of the default list of the item.
 * @param line_buff_size: size of the line buff.
 * @param colu_num: column number of the line.
 * @param inc_file_path: path of the include file, eg: -include "config.cfg".
 * @param item_name_buff: buff of the item name. eg.: TEST.
 * @param item_value_buff: buff of the item value, eg.: TEST = item value.
 * @param logic_expr_buff: buff of the logic expression, eg.: ((y|n)&(y&n)|y&!n)|n.
 * @param ref_name_buff: buff of the reference name, eg.: <TEST>
 ************************************************************************************/
struct lc_ctrl_blk {
	uint32_t mem_uplimit;
	uint32_t mem_used;
	uint32_t line_buff_size;
	uint32_t colu_num;
	struct lc_list_node menu_cfg_head;
	struct lc_list_node default_cfg_head;
	char *line_buff;
	char *inc_file_path;
	char *item_name_buff;
	char *item_value_buff;
	char *logic_expr_buff;
	char *ref_name_buff;
	char *temp_buff;
	void *buff_pool;
};

/*************************************************************************************
 * @brief: init .
 *
 * @ctrl_blk: control block.
 * @mem_uplimit: memory upper limit.
 * @line_buff_size: line buffer size.
 *
 * @return: cfg_item.
 ************************************************************************************/
int light_config_init(struct lc_ctrl_blk *ctrl_blk, uint32_t mem_uplimit,
                                                 uint32_t line_buff_size);

/*************************************************************************************
 * @brief: get the column number of control block.
 *
 * @ctrl_blk: control block.
 * 
 * @return: column number.
 ************************************************************************************/
int light_config_get_column_num(struct lc_ctrl_blk *ctrl_blk);

/*************************************************************************************
 * @brief: get the line buffer of control block.
 *
 * @ctrl_blk: control block.
 * 
 * @return: line buffer.
 ************************************************************************************/
char *light_config_get_line_buff(struct lc_ctrl_blk *ctrl_blk);

/*************************************************************************************
 * @brief: get the include file path of control block.
 *
 * @ctrl_blk: control block.
 * 
 * @return: include file path.
 ************************************************************************************/
char *light_config_get_inc_file_path(struct lc_ctrl_blk *ctrl_blk);

/*************************************************************************************
 * @brief: parse the line.
 * 
 * @ctrl_blk: control block.
 * 
 * @return parse result.
 ************************************************************************************/
int light_config_parse_line(struct lc_ctrl_blk *ctrl_blk);

#endif
