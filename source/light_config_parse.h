#ifndef __LIGHT_CONFIG_H__
#define __LIGHT_CONFIG_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

//#define LC_DEBUG
#define LC_INC_FILES_NUM_MAX               (102400)
#define LC_CFG_ITEMS_CACHE_LINE_NUM        (512)
#define LC_LINE_BUFF_SIZE_MIN              (4096)
#define LC_LINE_BUFF_SIZE_MAX              (8192)
#define LC_MEM_UPLIMIT                     (128 * 1024 * 1024)

enum lc_parse_res {
	/* error code */
	LC_PARSE_RES_ERR_CTRL_BLK_INVALID = -99,
	LC_PARSE_RES_ERR_CFG_TYPE_INVALID,
	LC_PARSE_RES_ERR_CFG_ITEM_INVALID,
	LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND,
	LC_PARSE_RES_ERR_INCLUDE_INVALID,
	LC_PARSE_RES_ERR_LINE_BUFF_OVERFLOW,
	LC_PARSE_RES_ERR_INC_FILE_NUM_OVERFLOW,
	LC_PARSE_RES_ERR_MEMORY_FAULT,
	LC_PARSE_RES_ERR_MEMORY_OVERFLOW,
	LC_PARSE_RES_ERR_MENU_CFG_INVALID,
	LC_PARSE_RES_ERR_DIRECT_CFG_INVALID,
	LC_PARSE_RES_ERR_NORMAL_CFG_INVALID,
	LC_PASER_RES_ERR_FILE_NOT_FOUND,
	/* ok code */
	LC_PARSE_RES_OK_DEPEND_CFG = 0,
	LC_PARSE_RES_OK_NORMAL_CFG,
	LC_PARSE_RES_OK_INCLUDE,
};

enum lc_assign_type {
	LC_ASSIGN_TYPE_DIRECT = 0,   /*  = */
	LC_ASSIGN_TYPE_IMMEDIATE,    /* := */
	LC_ASSIGN_TYPE_CONDITIONAL,  /* ?= */
	LC_ASSIGN_TYPE_ADDITION,     /* += */
};

struct lc_list_node {
	struct lc_list_node *next;
	struct lc_list_node *prev;
};

struct lc_cfg_item {
	bool enable;
	uint16_t assign_type;
	uint16_t name_len;
	uint16_t value_len;
	uint64_t name_hashval;
	char *name;
	char *value;
	struct lc_list_node node;
};

struct lc_cfg_item_cache {
	uint64_t ref_cnt[LC_CFG_ITEMS_CACHE_LINE_NUM];
	struct lc_cfg_item *items[LC_CFG_ITEMS_CACHE_LINE_NUM];
};

struct lc_cfg_list {
	char *name;
	struct lc_cfg_item_cache cache;
	struct lc_list_node node;
};

struct lc_cfg_file_item {
	char *file_name;
	fpos_t position;
	size_t line_num;
};

struct lc_cfg_file_stk {
	ssize_t depth;
	ssize_t sp;
	struct lc_cfg_file_item *item_stk;
};

struct lc_mem_blk_ctrl {
	size_t uplimit;
	size_t used;
	size_t each_blk_size;
	size_t curr_blk_size;
	size_t curr_blk_rest;
	void *ptr;
	struct lc_list_node node;
};

struct lc_mem_blk {
	void *buff;
	struct lc_list_node node;
};

#define lc_exit(ret)            exit(ret)

/*************************************************************************************
 * @brief: control block of the config file.
 * 
 * @param menu_cfg_head: head of the menu list of the item.
 * @param default_cfg_head: head of the default list of the item.
 * @param line_buff_size: size of the line buff.
 * @param colu_num: column number of the line.
 * @param file_name: file name of the config file.
 * @param inc_path_buff: path of the include file, eg: -include "config.cfg".
 * @param item_name_buff: buff of the item name. eg.: TEST.
 * @param item_value_buff: buff of the item value, eg.: TEST = item value.
 * @param logic_expr_buff: buff of the logic expression, eg.: ((y|n)&(y&n)|y&!n)|n.
 * @param ref_name_buff: buff of the reference name, eg.: <TEST>
 * @param temp_buff: buff of the temp.
 * @buff_pool: memory pool.
 * @cfg_items_mem_head: head of the list of the item.
 * @cfg_file_name_stk: memory of the file name stack.
 ************************************************************************************/
struct lc_ctrl_blk {
	size_t line_buff_size;
	size_t colu_num;
	char *line_buff;
	char *file_name_buff;
	char *inc_path_buff;
	char *item_name_buff;
	char *item_value_buff;
	char *logic_expr_buff;
	char *ref_name_buff;
	char *temp_buff;
	void *buff_pool;
	struct lc_cfg_list menu_cfg_head;
	struct lc_cfg_list default_cfg_head;
	struct lc_mem_blk_ctrl mem_blk_ctrl;
	struct lc_cfg_file_stk cfg_file_stk;
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
int light_config_init(struct lc_ctrl_blk *ctrl_blk, size_t mem_uplimit,
                      size_t line_buff_size, size_t cfg_file_num_max,
                      size_t each_mem_blk_size);

/*************************************************************************************
 * @brief: parse the config file.
 * 
 * @ctrl_blk: control block.
 * @cfg_file: config file name.
 * @is_default_cfg: whether the subfile is default config file.
 * 
 * @return zero on success, otherwise on failure.
 ************************************************************************************/
int light_config_parse_cfg_file(struct lc_ctrl_blk *ctrl_blk, char *cfg_file,
                                                         bool is_default_cfg);

/*************************************************************************************
 * @brief: dump a cfg list.
 *
 * @name: name.
 * @cfg_head: head of the list of the item.
 *
 * @return: void.
 ************************************************************************************/
void lc_dump_cfg(struct lc_cfg_list *cfg_head);


#endif
