#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "light_config_parse.h"
#include "light_config_parse_sm.h"


#define container_of(ptr, struct_type, member) \
	((struct_type *)((char *)(ptr) - (char *)(&(((struct_type *)0)->member))))

#define lc_list_for_each_entry(pos, list_head, entry_type, member) \
	for ((pos) = container_of((list_head)->next, entry_type, member); \
	     &(pos)->member != (list_head); \
	     (pos) = container_of((pos)->member.next, entry_type, member))

#define lc_list_for_each_entry_reverse(pos, list_head, entry_type, member) \
	for ((pos) = container_of((list_head)->prev, entry_type, member); \
	     &(pos)->member != (list_head); \
	     (pos) = container_of((pos)->member.prev, entry_type, member))

#define lc_list_first_entry(head, entry_type, member) \
	container_of((head)->next, entry_type, member)


#ifdef LC_DEBUG
#define lc_info(fmt, ...)      printf("[LC_INFO] "fmt, ##__VA_ARGS__)
#define lc_warn(fmt, ...)      printf("[LC_WARN] "fmt, ##__VA_ARGS__)
#else
#define lc_info(fmt, ...)      ((void)fmt)
#define lc_warn(fmt, ...)      ((void)fmt)
#endif 

#define lc_err(fmt, ...)      printf("[LC_ERR]"fmt, ##__VA_ARGS__)

/*************************************************************************************
 * @brief: parse line and get the hash value (BKDR Hash Function).
 * 
 * @param str: string to be parsed.
 * 
 * @return: hash value.
 ************************************************************************************/
static uint64_t lc_bkdr_hash(char *str)
{
	uint64_t seed = 1313171;
	uint64_t hash = 0;

	while (*str)
		hash = hash * seed + (*str++);
 
	return (hash & 0x7FFFFFFFFFFFFFFF);
}

/*************************************************************************************
 * @brief: list node init.
 * @param node: node of list.
 * @return none.
 ************************************************************************************/
static void lc_list_init(struct lc_list_node *node)
{
	node->next = node;
	node->prev = node;
}

/*************************************************************************************
 * @brief: Determine whether this list is empty.
 * @param node: node of list.
 * @return none.
 ************************************************************************************/
static bool lc_list_is_empty(struct lc_list_node *head)
{
	if (head->next == head || head->prev == head)
		return true;

	return false;
}

/*************************************************************************************
 * @brief: add node at tail.
 * 
 * @param head: head of list.
 * @param node: node of list.
 * @return: none.
 ************************************************************************************/
static void lc_list_add_node_at_tail(struct lc_list_node *head, struct lc_list_node *node)
{
	struct lc_list_node *tail = head->prev;

	node->prev = tail;
	node->next = head;
	tail->next = node;
	head->prev = node;
}

/*************************************************************************************
 * @brief: find a cfg item.
 *
 * @name: name.
 * @cfg_head: head of the list of the item.
 *
 * @return: cfg_item.
 ************************************************************************************/
static void lc_dump_cfg(struct lc_list_node *cfg_head)
{
	struct lc_cfg_item *pos;

	if (lc_list_is_empty(cfg_head))
		return;

	lc_list_for_each_entry(pos, cfg_head, struct lc_cfg_item, node) {
		lc_info("name:%s, len:%d, en:%d, value:%s, len:%d, assign_type:%d, hash:0x%llx\n",
		         pos->name, pos->name_len, pos->enable, pos->value, pos->value_len,
		         pos->assign_type, pos->name_hashval);
	}
}

/*************************************************************************************
 * @brief: find a cfg item.
 *
 * @name: name.
 * @cfg_head: head of the list of the item.
 *
 * @return: cfg_item.
 ************************************************************************************/
static struct lc_cfg_item *lm_find_cfg_item(char *name, struct lc_list_node *cfg_head)
{
	struct lc_cfg_item *pos;

	if (lc_list_is_empty(cfg_head))
		return NULL;

	lc_list_for_each_entry_reverse(pos, cfg_head, struct lc_cfg_item, node) {
		if (pos->name_hashval == lc_bkdr_hash(name)) {
			if (strcmp(pos->name, name) == 0)
				return pos;
		}
	}

	return NULL;
}

/*************************************************************************************
 * @brief: get the column number of control block.
 *
 * @ctrl_blk: control block.
 * 
 * @return: column number.
 ************************************************************************************/
int light_config_get_column_num(struct lc_ctrl_blk *ctrl_blk)
{
	if (ctrl_blk == NULL)
		return LC_PARSE_RES_ERR_CTRL_BLK_INVALID;

	return ctrl_blk->colu_num;
}

/*************************************************************************************
 * @brief: get the line buffer of control block.
 *
 * @ctrl_blk: control block.
 * 
 * @return: line buffer.
 ************************************************************************************/
char *light_config_get_line_buff(struct lc_ctrl_blk *ctrl_blk)
{
	if (ctrl_blk == NULL)
		return NULL;

	return ctrl_blk->line_buff;
}

/*************************************************************************************
 * @brief: get the include file path of control block.
 *
 * @ctrl_blk: control block.
 * 
 * @return: include file path.
 ************************************************************************************/
char *light_config_get_inc_file_path(struct lc_ctrl_blk *ctrl_blk)
{
	if (ctrl_blk == NULL)
		return NULL;

	return ctrl_blk->inc_file_path;
}

/*************************************************************************************
 * @brief: init lc control block.
 *
 * @ctrl_blk: control block.
 * @mem_uplimit: memory uplimit, if over the memory uplimit, return error.
 * @line_buff_size: size of the line buff.
 * 
 * @return: void.
 ************************************************************************************/
static void lc_cb_init(struct lc_ctrl_blk *ctrl_blk, uint32_t mem_uplimit,
                                                 uint32_t line_buff_size)
{
	ctrl_blk->mem_uplimit = mem_uplimit;
	ctrl_blk->mem_used = 0;
	ctrl_blk->line_buff_size = line_buff_size;
	ctrl_blk->colu_num = 0;

	ctrl_blk->line_buff = ctrl_blk->buff_pool;
	ctrl_blk->inc_file_path = ctrl_blk->line_buff + line_buff_size + LC_LINE_BUFFS_GAP;
	ctrl_blk->item_name_buff = ctrl_blk->inc_file_path + line_buff_size + LC_LINE_BUFFS_GAP;
	ctrl_blk->item_value_buff = ctrl_blk->item_name_buff + line_buff_size + LC_LINE_BUFFS_GAP;
	ctrl_blk->logic_expr_buff = ctrl_blk->item_value_buff + line_buff_size + LC_LINE_BUFFS_GAP;
	ctrl_blk->ref_name_buff = ctrl_blk->logic_expr_buff + line_buff_size + LC_LINE_BUFFS_GAP;
	ctrl_blk->temp_buff = ctrl_blk->ref_name_buff + line_buff_size + LC_LINE_BUFFS_GAP;

	lc_list_init(&ctrl_blk->default_cfg_head);
	lc_list_init(&ctrl_blk->menu_cfg_head);
}

/*************************************************************************************
 * @brief: add cfg item to list.
 *
 * @ctrl_blk: control block.
 * @cfg_head: head of the list of the item.
 * @name: name, eg: TEST.
 * @value: value, eg: TEST = item value.
 * @assign_type: assign type, eg: =, +=, ?= , :=.
 * @enable: enable, if true, the item is enable, else disable.
 * 
 * @return: void.
 ************************************************************************************/
static int lc_add_cfg_item(struct lc_ctrl_blk *ctrl_blk, struct lc_list_node *cfg_head,
                        char *name, uint32_t name_len, char *value, uint32_t value_len,
                        uint32_t assign_type, bool enable)
{
	uint32_t mem_size;
	struct lc_cfg_item *cfg_item;

	if ((name_len == 0) || (value_len == 0)) {
		lc_err("name or value is null\n");
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	mem_size = sizeof(struct lc_cfg_item) + name_len + value_len + 4 * sizeof(uint32_t);
	cfg_item = malloc(mem_size);
	if (cfg_item == NULL) {
		lc_err("no mem to allocate cfg item\n");
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	ctrl_blk->mem_used += mem_size;
	if (ctrl_blk->mem_used > ctrl_blk->mem_uplimit) {
		lc_err("mem used too large\n");
		return LC_PARSE_RES_ERR_MEMORY_OVERFLOW;
	}

	cfg_item->name = (char *)cfg_item + sizeof(struct lc_cfg_item) + sizeof(uint32_t);
	cfg_item->value = cfg_item->name + name_len + sizeof(uint32_t);
	cfg_item->name_len = name_len;
	cfg_item->value_len = value_len;
	cfg_item->name_hashval = lc_bkdr_hash(name);
	cfg_item->assign_type = assign_type;
	cfg_item->enable = enable;
	memcpy(cfg_item->name, name, name_len);
	memcpy(cfg_item->value, value, value_len);
	cfg_item->name[name_len - 1] = '\0';
	cfg_item->value[value_len - 1] = '\0';
	lc_list_add_node_at_tail(cfg_head, &cfg_item->node);

	return 0;
}

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
                                                 uint32_t line_buff_size)
{
	int ret;
	char *ptr = NULL;

	if (ctrl_blk == NULL) {
		lc_err("ctrl_blk is NULL\n");
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	if ((line_buff_size > LC_LINE_BUFF_SIZE) || (mem_uplimit > LC_MEM_UPLIMIT)) {
		lc_err("line_buff_size[%d] or mem_uplimit[%d] is too large\n",
		        line_buff_size, mem_uplimit);
		return LC_PARSE_RES_ERR_LINE_BUFF_OVERFLOW;
	}

	ctrl_blk->buff_pool = malloc(line_buff_size * 10);
	if (ctrl_blk->buff_pool == NULL) {
		lc_err("malloc buff_pool[%d bytes] failed\n", line_buff_size * 10);
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	lc_cb_init(ctrl_blk, mem_uplimit, line_buff_size);

	/* get top directory */
	ptr = getcwd(ctrl_blk->temp_buff, line_buff_size);
	if (ptr == NULL)
		return LC_PASER_RES_ERR_FILE_NOT_FOUND;

	/* add top directory cfg item (LC_TOPDIR = "./xxx") to default cfg list */
	ret = lc_add_cfg_item(ctrl_blk, &ctrl_blk->default_cfg_head, "LC_TOPDIR", 10,
	                      ctrl_blk->temp_buff, strlen(ctrl_blk->temp_buff) + 1,
	                      LC_ASSIGN_TYPE_DIRECT, true);
	if (ret < 0) {
		lc_err("LC_TOPDIR item add fail\n");
		return ret;
	}

	lc_dump_cfg(&ctrl_blk->default_cfg_head);
	return 0;
}

/*************************************************************************************
 * @brief: state machine to parse the config file
 * 
 * @ctrl_blk: control block.
 * @param options: options to select. options = "Axx, Bxxx, xxx,"
 * @param select: select.
 * 
 * @return next state of the state machine.
 ************************************************************************************/
static bool lc_parse_options_match(struct lc_ctrl_blk *ctrl_blk,
                                   char *options, char *select)
{
	int i = 0;
	char ch = *options;

	while (ch) {
		ch = *options++;

		if (ch != ']') {
			ctrl_blk->temp_buff[i] = ch;
			continue;
		}

		ctrl_blk->temp_buff[i] = '\0';
		lc_info("tmp:%s\n", ctrl_blk->temp_buff);
		if (strcmp(ctrl_blk->temp_buff, select) == 0)
			return true;
		i = 0;
	}

	return false;
}

/*************************************************************************************
 * @brief: parse the line.
 * 
 * @ctrl_blk: control block.
 * 
 * @return parse result.
 ************************************************************************************/
int light_config_parse_line(struct lc_ctrl_blk *ctrl_blk)
{
	int i = 0;
	int curr_state = 0;
	int next_state = 0;
	char ch = ctrl_blk->line_buff[i++];

	if (ctrl_blk == NULL) {
		lc_err("ctrl_blk is NULL\n");
		return LC_PARSE_RES_ERR_CTRL_BLK_INVALID;
	}

	ctrl_blk->colu_num = 0;

	while (ch != '\0') {
		next_state = light_config_parse_state_get_next(curr_state, ch);
		if (next_state < 0)
			return next_state;

		if (next_state == LC_PARSE_STATE_INCLUDE)
			return LC_PASER_RES_OK_INCLUDE;

		curr_state = next_state;
		ch = ctrl_blk->line_buff[i++];
		ctrl_blk->colu_num++;
	}

	return LC_PASER_RES_OK_NORMAL_CFG;
}
