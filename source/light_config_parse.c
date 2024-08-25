#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "logic_expr_parse.h"
#include "light_config_parse.h"
#include "light_config_parse_sm.h"

#define container_of(ptr, struct_type, member) \
	((struct_type *)((char *)(ptr) - (char *)(&(((struct_type *)0)->member))))

#define lc_list_next_entry(entry, entry_type, member) \
	container_of((entry)->member.next, entry_type, member)

#define lc_list_for_each_entry(pos, list_head, entry_type, member) \
	for ((pos) = container_of((list_head)->next, entry_type, member); \
	     &(pos)->member != (list_head); \
	     (pos) = container_of((pos)->member.next, entry_type, member))

#define lc_list_for_each_entry_safe(pos, temp, list_head, entry_type, member) \
	for ((pos) = container_of((list_head)->next, entry_type, member), \
	     (temp) = lc_list_next_entry(pos, entry_type, member); \
	     &(pos)->member != (list_head); \
	     (pos) = (temp), (temp) = lc_list_next_entry((temp), entry_type, member))

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

#define lc_err(fmt, ...)       printf("[LC_ERR]"fmt, ##__VA_ARGS__)

struct lc_parse_ctrl_blk;
typedef int (*parse_func_t)(struct lc_ctrl_blk *ctrl_blk,
                   struct lc_parse_ctrl_blk *cb, char ch);

struct lc_parse_ctrl_blk {
	bool item_en;
	bool ref_en;
	int match_state;
	int select;
	int char_idx;
	int name_idx;
	int value_idx;
	int expr_idx;
	int path_idx;
	int temp_idx;
	int arr_elem_num;
	int arr_elem_idx;
	int ref_name_idx;
	int curr_state;
	int next_state;
	uint16_t assign_type;
	parse_func_t parsing;
	parse_func_t parse_terminal;
	struct lc_cfg_item *default_item;
};

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
 * @brief: push a cfg file to the stack.
 * 
 * @param cfg_file_stk: cfg file stack.
 * @param item: item of the cfg file.
 * @return: zero on success, other on failure.
 ************************************************************************************/
static int lc_cfg_file_push(struct lc_ctrl_blk *ctrl_blk, struct lc_cfg_file_item *item)
{
	void *name;
	struct lc_cfg_file_stk *stk = &(ctrl_blk->cfg_file_stk);

	if (stk->sp >= stk->depth) {
		lc_err("cfg file num[%lld] overflow[%lld]\n", stk->sp, stk->depth);
		return LC_PARSE_RES_ERR_INC_FILE_NUM_OVERFLOW;
	}
	
	name = malloc(strlen(item->file_name) + 1);
	if (name == NULL) {
		lc_err("malloc failed\n");
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	memcpy(name, item->file_name, strlen(item->file_name) + 1);
	stk->item_stk[stk->sp].file_name = name;
	stk->item_stk[stk->sp].line_num = item->line_num;
	stk->item_stk[stk->sp].position = item->position;
	stk->sp++;

	return 0;
}

/*************************************************************************************
 * @brief: push a cfg file to the stack.
 * 
 * @param cfg_file_stk: cfg file stack.
 * @param item: item of the cfg file.
 * @return: zero on success, other on failure.
 ************************************************************************************/
static int lc_cfg_file_pop(struct lc_ctrl_blk *ctrl_blk, struct lc_cfg_file_item *item)
{
	void *name;
	struct lc_cfg_file_stk *stk = &(ctrl_blk->cfg_file_stk);

	if (stk->sp < 0) {
		lc_err("cfg file num[%lld] overflow\n", stk->sp);
		return LC_PARSE_RES_ERR_INC_FILE_NUM_OVERFLOW;
	}

	stk->sp--;
	name = stk->item_stk[stk->sp].file_name;
	memcpy(ctrl_blk->file_name_buff, name, strlen(name) + 1);
	item->file_name = ctrl_blk->file_name_buff;
	item->line_num = stk->item_stk[stk->sp].line_num;
	item->position = stk->item_stk[stk->sp].position;
	free(name);
	return 0;
}

/*************************************************************************************
 * @brief: dump a cfg list.
 *
 * @param name: name.
 * @param cfg_head: head of the list of the item.
 *
 * @return: void.
 ************************************************************************************/
void lc_dump_cfg(struct lc_cfg_list *cfg_head)
{
	struct lc_cfg_item *pos;

	if (lc_list_is_empty(&cfg_head->node))
		return;

	lc_list_for_each_entry(pos, &cfg_head->node, struct lc_cfg_item, node) {
		lc_err("name:%s, len:%d, en:%d, value:%s, len:%d, assign_type:%d, hash:0x%llx\n",
		        pos->name, pos->name_len, pos->enable, pos->value, pos->value_len,
		        pos->assign_type, pos->name_hashval);
	}
}

/*************************************************************************************
 * @brief: free memory.
 *
 * @param ctrl_blk: control block.
 *
 * @return: cfg_item.
 ************************************************************************************/
void light_config_free(struct lc_ctrl_blk *ctrl_blk)
{
	struct lc_mem_blk *pos;
	struct lc_mem_blk *tmp;

	if (lc_list_is_empty(&ctrl_blk->mem_blk_ctrl.node))
		return;

	lc_list_for_each_entry_safe(pos, tmp, &ctrl_blk->mem_blk_ctrl.node,
	    struct lc_mem_blk, node) {
		free(pos);
	}
}

/*************************************************************************************
 * @brief: update cache of cfg list.
 *
 * @param cfg_head: head of the list of the item.
 * @param item: cfg item.
 *
 * @return: void.
 ************************************************************************************/
static void lc_cfg_list_update_cache(struct lc_cfg_list *cfg_head, struct lc_cfg_item *item)
{
	int i;
	int min_cnt_idx = 0;
	uint64_t min_cnt = cfg_head->cache.ref_cnt[0];

	/* find the index of the cache of min refcnt */
	for (i = 1; i < LC_CFG_ITEMS_CACHE_LINE_NUM; i++) {
		if (cfg_head->cache.ref_cnt[i] < min_cnt) {
			min_cnt = cfg_head->cache.ref_cnt[i];
			min_cnt_idx = i;
		}
	}

	cfg_head->cache.items[min_cnt_idx] = item;
	cfg_head->cache.ref_cnt[min_cnt_idx] = 1;
	lc_info("update cache, name:%s, hash:0x%llx\n", item->name, item->name_hashval);
}

/*************************************************************************************
 * @brief: find a cfg item.
 *
 * @param cfg_head: head of the list of the item.
 * @param name: name.
 *
 * @return: cfg_item.
 ************************************************************************************/
static struct lc_cfg_item *lc_find_cfg_item(struct lc_cfg_list *cfg_head, char *name)
{
	int i;
	struct lc_cfg_item *pos = NULL;

	if (lc_list_is_empty(&cfg_head->node))
		return NULL;

	/* find the item in cache */
	for (i = 0; i < LC_CFG_ITEMS_CACHE_LINE_NUM; i++) {
		if (cfg_head->cache.items[i] == NULL)
			continue;

		if (cfg_head->cache.items[i]->name_hashval == lc_bkdr_hash(name) &&
		    strcmp(cfg_head->cache.items[i]->name, name) == 0) {
			cfg_head->cache.ref_cnt[i]++;
			return cfg_head->cache.items[i];
		}
	}

	/* find the item in list */
	lc_info("find item[%s] in list\n", name);
	lc_list_for_each_entry_reverse(pos, &cfg_head->node, struct lc_cfg_item, node) {
		if (pos->name_hashval == lc_bkdr_hash(name)) {
			if (strcmp(pos->name, name) != 0)
				continue;

			lc_cfg_list_update_cache(cfg_head, pos);
			return pos;
		}
	}

	return NULL;
}

/*************************************************************************************
 * @brief: init lc control block.
 *
 * @param ctrl_blk: control block.
 * @param line_buff_size: size of the line buff.
 * @param mem_uplimit: memory uplimit, if over the memory uplimit, return error.
 * @param each_mem_blk_size: size of each memory block.
 * @param cfg_file_num_max: max number of include files.
 * @param cfg_file_stk: include file stack.
 * 
 * @return: void.
 ************************************************************************************/
static void lc_cb_init(struct lc_ctrl_blk *ctrl_blk, size_t line_buff_size,
                       size_t mem_uplimit, size_t each_mem_blk_size,
                       size_t cfg_file_num_max, void *cfg_file_stk)
{
	ctrl_blk->line_buff_size = line_buff_size;
	ctrl_blk->colu_num = 0;
	ctrl_blk->line_buff = ctrl_blk->buff_pool;
	ctrl_blk->file_name_buff = ctrl_blk->line_buff + line_buff_size;
	ctrl_blk->inc_path_buff = ctrl_blk->file_name_buff + line_buff_size;
	ctrl_blk->item_name_buff = ctrl_blk->inc_path_buff + line_buff_size;
	ctrl_blk->item_value_buff = ctrl_blk->item_name_buff + line_buff_size;
	ctrl_blk->logic_expr_buff = ctrl_blk->item_value_buff + line_buff_size;
	ctrl_blk->ref_name_buff = ctrl_blk->logic_expr_buff + line_buff_size;
	ctrl_blk->temp_buff = ctrl_blk->ref_name_buff + line_buff_size;

	ctrl_blk->mem_blk_ctrl.uplimit = mem_uplimit;
	ctrl_blk->mem_blk_ctrl.used = 0;
	ctrl_blk->mem_blk_ctrl.each_blk_size = each_mem_blk_size;
	ctrl_blk->mem_blk_ctrl.curr_blk_size = 0;
	ctrl_blk->mem_blk_ctrl.curr_blk_rest = 0;
	ctrl_blk->mem_blk_ctrl.ptr = NULL;

	ctrl_blk->cfg_file_stk.depth = cfg_file_num_max + 8;
	ctrl_blk->cfg_file_stk.sp = 0;
	ctrl_blk->cfg_file_stk.item_stk = cfg_file_stk;

	ctrl_blk->default_cfg_head.name = "default config";
	ctrl_blk->menu_cfg_head.name = "menu config";
	lc_list_init(&ctrl_blk->menu_cfg_head.node);
	lc_list_init(&ctrl_blk->default_cfg_head.node);
	lc_list_init(&ctrl_blk->mem_blk_ctrl.node);
	memset(&(ctrl_blk->menu_cfg_head.cache), 0, sizeof(struct lc_cfg_item_cache));
	memset(&(ctrl_blk->default_cfg_head.cache), 0, sizeof(struct lc_cfg_item_cache));
}

/*************************************************************************************
 * @brief: allocate memory for cfg item.
 *
 * @param ctrl_blk: control block.
 * @param size: size of memory.
 * 
 * @return: pointer to allocated memory.
 ************************************************************************************/
static void *lc_malloc(struct lc_ctrl_blk *ctrl_blk, int size)
{
	void *p;
	struct lc_mem_blk *mem_blk;
	struct lc_mem_blk_ctrl *mem_blk_ctrl = &ctrl_blk->mem_blk_ctrl;

	if (mem_blk_ctrl->used + size > mem_blk_ctrl->uplimit ||
	    size > mem_blk_ctrl->each_blk_size)
		return NULL;

	if (mem_blk_ctrl->curr_blk_rest < size) {
		mem_blk = malloc(sizeof(struct lc_mem_blk) + mem_blk_ctrl->each_blk_size);
		if (mem_blk == NULL)
			return NULL;

		mem_blk->buff = mem_blk + sizeof(struct lc_mem_blk);
		mem_blk_ctrl->curr_blk_size = mem_blk_ctrl->each_blk_size;
		mem_blk_ctrl->curr_blk_rest = mem_blk_ctrl->each_blk_size;
		mem_blk_ctrl->ptr = mem_blk->buff;
		lc_list_add_node_at_tail(&ctrl_blk->mem_blk_ctrl.node, &mem_blk->node);
	}

	p = mem_blk_ctrl->ptr;
	mem_blk_ctrl->used += size;
	mem_blk_ctrl->curr_blk_rest -= size;
	mem_blk_ctrl->ptr += size;
	return p;
}

/*************************************************************************************
 * @brief: add cfg item to list.
 *
 * @param ctrl_blk: control block.
 * @param cfg_head: head of the list of the item.
 * @param name: name, eg: TEST.
 * @param value: value, eg: TEST = item value.
 * @param assign_type: assign type, eg: =, +=, ?= , :=.
 * @param enable: enable, if true, the item is enable, else disable.
 * 
 * @return: void.
 ************************************************************************************/
static int lc_add_cfg_item(struct lc_ctrl_blk *ctrl_blk, struct lc_list_node *cfg_head,
                           char *name, uint16_t name_len, char *value, uint16_t value_len,
                           uint16_t assign_type, bool enable)
{
	uint32_t mem_size;
	struct lc_cfg_item *cfg_item;

	if (name_len == 0) {
		lc_err("name is null\n");
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	if (value_len == 0) {
		value[0] = enable ? 'y' : 'n';
		value[1] = '\0';
		value_len = 1;
	}

	mem_size = sizeof(struct lc_cfg_item) + name_len + value_len;
	cfg_item = lc_malloc(ctrl_blk, mem_size + sizeof(size_t));
	if (cfg_item == NULL) {
		lc_err("no mem to allocate cfg item\n");
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	if ((value[value_len - 1] == 'n') && (value_len == 1))
		enable = false;

	cfg_item->name = (char *)cfg_item + sizeof(struct lc_cfg_item);
	cfg_item->name_len = name_len;
	cfg_item->value = cfg_item->name + name_len + 1;
	cfg_item->value_len = value_len;
	cfg_item->name_hashval = lc_bkdr_hash(name);
	cfg_item->assign_type = assign_type;
	cfg_item->enable = enable;
	memcpy(cfg_item->name, name, name_len);
	memcpy(cfg_item->value, value, value_len);
	cfg_item->name[name_len] = '\0';
	cfg_item->value[value_len] = '\0';
	lc_list_add_node_at_tail(cfg_head, &cfg_item->node);

	return 0;
}

/*************************************************************************************
 * @brief: init cfg items.
 *
 * @param ctrl_blk: control block.
 *
 * @return: cfg_item.
 ************************************************************************************/
static int lc_cfg_items_init(struct lc_ctrl_blk *ctrl_blk)
{
	int ret;
	char *ptr = NULL;

	/* get top directory */
	ptr = getcwd(ctrl_blk->temp_buff, ctrl_blk->line_buff_size);
	if (ptr == NULL)
		return LC_PASER_RES_ERR_FILE_NOT_FOUND;

	/* add top directory cfg item (LC_TOPDIR = "./xxx") to default cfg list */
	ret = lc_add_cfg_item(ctrl_blk, &ctrl_blk->default_cfg_head.node,
	                     "LC_TOPDIR", 9, ctrl_blk->temp_buff,
	                      strlen(ctrl_blk->temp_buff),
	                      LC_ASSIGN_TYPE_DIRECT, true);
	if (ret < 0) {
		lc_err("LC_TOPDIR item add fail\n");
		return ret;
	}

	/* add top directory cfg item (LC_TOPDIR = "./xxx") to menu cfg list */
	ret = lc_add_cfg_item(ctrl_blk, &ctrl_blk->menu_cfg_head.node,
	                     "LC_TOPDIR", 9, ctrl_blk->temp_buff,
	                      strlen(ctrl_blk->temp_buff),
	                      LC_ASSIGN_TYPE_DIRECT, true);
	if (ret < 0) {
		lc_err("LC_TOPDIR item add fail\n");
		return ret;
	}

	return 0;
}

/*************************************************************************************
 * @brief: init .
 *
 * @param ctrl_blk: control block.
 * @param mem_uplimit: memory upper limit.
 * @param line_buff_size: line buffer size.
 * @param cfg_file_num_max: max number of cfg files.
 * @param each_mem_blk_size: each memory block size.
 *
 * @return: cfg_item.
 ************************************************************************************/
int light_config_init(struct lc_ctrl_blk *ctrl_blk, size_t mem_uplimit,
                      size_t line_buff_size, size_t cfg_file_num_max,
                      size_t each_mem_blk_size)
{
	int ret;
	size_t mem_size;

	if (ctrl_blk == NULL) {
		lc_err("ctrl_blk is NULL\n");
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	if ((line_buff_size < LC_LINE_BUFF_SIZE_MIN) ||
	    (line_buff_size > LC_LINE_BUFF_SIZE_MAX) ||
	    (mem_uplimit > LC_MEM_UPLIMIT)) {
		lc_err("line_buff_size[%llu] or mem_uplimit[%llu] is too large\n",
		        line_buff_size, mem_uplimit);
		return LC_PARSE_RES_ERR_LINE_BUFF_OVERFLOW;
	}

	mem_size = line_buff_size * 8 + cfg_file_num_max * sizeof(struct lc_cfg_file_item);
	printf("mem_size[%llu]\n", mem_size);
	ctrl_blk->buff_pool = malloc(mem_size);
	if (ctrl_blk->buff_pool == NULL) {
		lc_err("malloc buff_pool[%llu bytes] failed\n", mem_size);
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	/* initialize ctrl_blk */
	lc_cb_init(ctrl_blk, line_buff_size, mem_uplimit, each_mem_blk_size,
	         cfg_file_num_max, ctrl_blk->buff_pool + line_buff_size * 8);

	ret = lc_cfg_items_init(ctrl_blk);
	if (ret < 0) {
		free(ctrl_blk->buff_pool);
		lc_err("lc_cfg_items_init failed, ret:%d\n", ret);
		return ret;
	}

	return 0;
}

/*************************************************************************************
 * @brief: parse select function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parsing_func_select(struct lc_ctrl_blk *ctrl_blk,
                        struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->select < 0)
		return 0;

	if (pcb->select == pcb->arr_elem_num)
		ctrl_blk->temp_buff[pcb->temp_idx++] = ch;
	return 0;
}

/*************************************************************************************
 * @brief: parse menu function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parsing_func_menu(struct lc_ctrl_blk *ctrl_blk,
                      struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state != 0)
		return 0;

	if (ch != pcb->default_item->value[pcb->arr_elem_idx])
		pcb->match_state = -1;

	if ((pcb->match_state == 0) && ((pcb->arr_elem_idx + 1) == pcb->default_item->value_len))
		pcb->match_state = 1;

	pcb->temp_idx++;
	pcb->arr_elem_idx++;

	return 0;
}

/*************************************************************************************
 * @brief: parse select terminal function.
 *
 * @param ctrl_blk: control block.
 * @param cb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parse_terminal_func_select(struct lc_ctrl_blk *ctrl_blk,
                              struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->select < 0)
		return 0;

	if (pcb->arr_elem_num > 2) {
		lc_err("element num[%d] must be 2 with the [@<MACRO] ? ([x], [y])\n",
		        pcb->arr_elem_num);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	ctrl_blk->temp_buff[pcb->temp_idx] = '\0';
	memcpy(ctrl_blk->item_value_buff, ctrl_blk->temp_buff, pcb->temp_idx + 1);
	pcb->value_idx += pcb->temp_idx;
	return 0;
}

/*************************************************************************************
 * @brief: parse menu terminal function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parse_terminal_func_menu(struct lc_ctrl_blk *ctrl_blk,
                             struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state != 1) {
		lc_err("menu element[%s] not found in [%s]\n", pcb->default_item->value,
		        ctrl_blk->item_name_buff);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	memcpy(ctrl_blk->item_value_buff, pcb->default_item->value, pcb->default_item->value_len);
	pcb->value_idx += pcb->default_item->value_len;
	return 0;
}

/*************************************************************************************
 * @brief: find the cfg item, if found, copy the value to dst.
 * 
 * @param ctrl_blk: control block.
 * @param cfg_head: cfg list head.
 * @param dst: destination buffer.
 * @param item_name: item name.
 * 
 * @return 0 on success, negative value if failure.
 ************************************************************************************/
static int lc_find_cfg_item_and_cpy_val(struct lc_ctrl_blk *ctrl_blk,
                                        struct lc_cfg_list *cfg_head,
                                        char *dst, char *item_name)
{
	struct lc_cfg_item *ref_item;

	ref_item = lc_find_cfg_item(cfg_head, item_name);
	if (ref_item == NULL)
		return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;

	memcpy(dst, ref_item->value, ref_item->value_len);
	dst[ref_item->value_len] = '\0';

	return ref_item->value_len;
}

/*************************************************************************************
 * @brief: find the cfg item, if found, copy the value to dst.
 * 
 * @param ctrl_blk: control block.
 * @param cfg_head: cfg list head.
 * @param item_name: item name.
 * @param en: enable flag we got.
 * 
 * @return 0 on success, negative value if failure.
 ************************************************************************************/
static int lc_find_cfg_item_and_get_en(struct lc_ctrl_blk *ctrl_blk,
                                       struct lc_cfg_list *cfg_head,
                                       char *item_name, bool *en)
{
	struct lc_cfg_item *ref_item = lc_find_cfg_item(cfg_head, item_name);
	if (ref_item == NULL)
		return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;

	*en = ref_item->enable;
	return 0;
}

/*************************************************************************************
 * @brief: parse the line.
 * 
 * @param ctrl_blk: control block.
 * @param line_num:  line number.
 * @param is_default_cfg: is default config file or not(menu_config).
 * 
 * @param zero on success, negative value on error.
 ************************************************************************************/
int light_config_parse_cfg_line(struct lc_ctrl_blk *ctrl_blk,
                       size_t line_num, bool is_default_cfg)
{
	int ret;
	char ch;
	struct lc_cfg_list *cfg_list;
	struct lc_parse_ctrl_blk cb;

	/* check parameters */
	if (ctrl_blk == NULL) {
		lc_err("ctrl_blk is NULL\n");
		return LC_PARSE_RES_ERR_CTRL_BLK_INVALID;
	}

	cfg_list = is_default_cfg ? &ctrl_blk->default_cfg_head
	                          : &ctrl_blk->menu_cfg_head;
	/* initialize parse control block */
	memset(&cb, 0, sizeof(struct lc_parse_ctrl_blk));
	cb.item_en = true;
	cb.assign_type = LC_ASSIGN_TYPE_DIRECT;
	ch = ctrl_blk->line_buff[cb.char_idx++];
	ctrl_blk->colu_num = 0;

	/* parse line accroding to state machine */
	while (ch != '\0') {
		cb.next_state = lc_parse_state_get_next(cb.curr_state, ch);
		if (cb.next_state < 0)
			return cb.next_state;

		/* parse line */
		switch (cb.next_state) {
		case 1:
			ctrl_blk->item_name_buff[cb.name_idx++] = ch;
			break;

		case 2:
			ctrl_blk->item_name_buff[cb.name_idx] = '\0';
			break;

		case 3:
			cb.assign_type = LC_ASSIGN_TYPE_IMMEDIATE;
			break;

		case 4:
			cb.assign_type = LC_ASSIGN_TYPE_ADDITION;
			break;

		case 5:
			cb.assign_type = LC_ASSIGN_TYPE_CONDITIONAL;
			break;
		
		case 9:
			cb.ref_name_idx = 0;
			break;
		
		case 10:
			ctrl_blk->ref_name_buff[cb.ref_name_idx++] = ch;
			break;
		
		case 11:
			ctrl_blk->ref_name_buff[cb.ref_name_idx] = '\0';
			ret = lc_find_cfg_item_and_get_en(ctrl_blk, cfg_list,
			                 ctrl_blk->ref_name_buff, &cb.ref_en);
			if (ret < 0) {
				lc_err("ref [%s] not found in [%s] line[%llu], col[%llu]\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return ret;
			}
			cb.item_en = cb.item_en && cb.ref_en;
			break;

		case 22:
			ctrl_blk->inc_path_buff[cb.path_idx++] = ch;
			break;

		case 23:
			ctrl_blk->inc_path_buff[cb.path_idx] = '\0';
			break;
		
		case 24:
			cb.ref_name_idx = 0;
			break;

		case 25:
			ctrl_blk->ref_name_buff[cb.ref_name_idx++] = ch;
			break;

		case 26:
			ctrl_blk->ref_name_buff[cb.ref_name_idx] = '\0';
			ret = lc_find_cfg_item_and_cpy_val(ctrl_blk, cfg_list,
			                ctrl_blk->inc_path_buff + cb.path_idx,
			                ctrl_blk->ref_name_buff);
			if (ret < 0) {
				lc_err("ref [%s] not found in [%s] line[%llu], col[%llu]\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return ret;
			}
			cb.path_idx += ret;
			break;
	
		case 27:
			cb.item_en = false;
			break;
		
		case 100:
			if (is_default_cfg)
				return -cb.next_state - 1;
			cb.value_idx = 0;
			break;

		case 101:
			cb.ref_name_idx = 0;
			break;
		
		case 102:
			ctrl_blk->ref_name_buff[cb.ref_name_idx++] = ch;
			break;
		
		case 103:
			ctrl_blk->ref_name_buff[cb.ref_name_idx] = '\0';
			ret = lc_find_cfg_item_and_cpy_val(ctrl_blk, &ctrl_blk->menu_cfg_head,
			                              ctrl_blk->item_value_buff + cb.value_idx,
			                              ctrl_blk->ref_name_buff);
			if (ret < 0) {
				lc_err("ref [%s] not found in [%s] line[%llu], col[%llu]\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return ret;
			}
			cb.value_idx += ret;
			break;

		case 104:
			ctrl_blk->item_value_buff[cb.value_idx++] = ch;
			break;

		case 105:
			ret = lc_find_cfg_item_and_cpy_val(ctrl_blk, &ctrl_blk->default_cfg_head,
			                    ctrl_blk->item_value_buff, ctrl_blk->item_name_buff);
			if (ret < 0) {
				lc_err("ref [%s] not found in [%s] line[%llu], col[%llu]\n",
				        ctrl_blk->item_name_buff, ctrl_blk->default_cfg_head.name,
				        line_num, ctrl_blk->colu_num);
				return ret;
			}
			cb.value_idx += ret;
			break;
		
		case 150:
			cb.ref_name_idx = 0;
			break;

		case 151:
			ctrl_blk->ref_name_buff[cb.ref_name_idx++] = ch;
			break;
		
		case 152:
			ctrl_blk->ref_name_buff[cb.ref_name_idx] = '\0';
			ret = lc_find_cfg_item_and_get_en(ctrl_blk, &ctrl_blk->menu_cfg_head,
			                                ctrl_blk->ref_name_buff, &cb.ref_en);
			if (ret < 0) {
				lc_err("ref [%s] not found in [%s] line[%llu], col[%llu]\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return ret;
			}
			break;
		
		case 154:
			cb.select = (cb.ref_en ? 0 : 1);
			cb.parsing = lc_parsing_func_select;
			cb.parse_terminal = lc_parse_terminal_func_select;
			break;
		
		case 200:
			cb.select = -1;
			break;
		
		case 204:
			cb.default_item = lc_find_cfg_item(&ctrl_blk->default_cfg_head,
			                                     ctrl_blk->item_name_buff);
			if (cb.default_item == NULL) {
				lc_err("item [%s] not found in [%s]", ctrl_blk->item_name_buff,
				         ctrl_blk->default_cfg_head.name);
				return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;
			}
			cb.parsing = lc_parsing_func_menu;
			cb.parse_terminal = lc_parse_terminal_func_menu;
			break;
		
		case 7000:
			cb.temp_idx = 0;
			cb.arr_elem_num = 0;
			break;
		
		case 7002:
			cb.arr_elem_idx = 0;
			cb.match_state = ((cb.match_state > 0) ? cb.match_state : 0);
			break;

		case 7003:
			cb.parsing(ctrl_blk, &cb, ch);
			break;
		
		case 7004:
			cb.arr_elem_num++;
			break;

		case 7006:
			ret = cb.parse_terminal(ctrl_blk, &cb, ch);
			if (ret < 0)
				return ret;
			break;

		case 7050:
			ctrl_blk->item_value_buff[cb.value_idx] = '\0';
			ret = lc_find_cfg_item_and_get_en(ctrl_blk, &ctrl_blk->default_cfg_head,
			                                  ctrl_blk->item_name_buff, &cb.ref_en);
			cb.item_en = cb.item_en && ((ret < 0) ? true : cb.ref_en);
			break;
		
		case 7052:
			cb.expr_idx = 0;
			break;
		
		case 7053:
			ctrl_blk->logic_expr_buff[cb.expr_idx++] = ch;
			break;
		
		case 7055:
			cb.ref_name_idx = 0;
			break;
		
		case 7056:
			ctrl_blk->ref_name_buff[cb.ref_name_idx++] = ch;
			break;

		case 7057:
			ctrl_blk->ref_name_buff[cb.ref_name_idx] = '\0';
			ret = lc_find_cfg_item_and_get_en(ctrl_blk, &ctrl_blk->menu_cfg_head,
			                                ctrl_blk->ref_name_buff, &cb.ref_en);
			if (ret < 0) {
				lc_err("ref [%s] not found in [%s] line[%llu], col[%llu]\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return ret;
			}
			ctrl_blk->logic_expr_buff[cb.expr_idx++] = (cb.ref_en ? 'y' : 'n');
			break;

		case 8000:
			ctrl_blk->item_value_buff[cb.value_idx++] = ch;
			cb.ref_name_idx = 0;
			break;

		case 8001:
			cb.ref_name_idx = 0;
			break;

		case 8002:
			ctrl_blk->ref_name_buff[cb.ref_name_idx++] = ch;
			break;

		case 8003:
			ctrl_blk->ref_name_buff[cb.ref_name_idx] = '\0';
			ret = lc_find_cfg_item_and_cpy_val(ctrl_blk, cfg_list,
			             ctrl_blk->item_value_buff + cb.value_idx,
			             ctrl_blk->ref_name_buff);
			if (ret < 0) {
				lc_err("ref [%s] not found in [%s] line[%llu], col[%llu]\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return ret;
			}
			cb.value_idx += ret;
			break;

		case LC_PARSE_STATE_DEPEND_CFG:
			ctrl_blk->logic_expr_buff[cb.expr_idx] = '\0';
			if (cb.expr_idx > 0) {
				lc_info("logic_expr:%s\n", ctrl_blk->logic_expr_buff);
				ret = lc_parse_logic_express(ctrl_blk->logic_expr_buff);
				if (ret < 0)
					return ret;
				cb.item_en = cb.item_en && (ret == 1);
			}
			ret = lc_add_cfg_item(ctrl_blk, &cfg_list->node, ctrl_blk->item_name_buff,
			                      cb.name_idx, ctrl_blk->item_value_buff, cb.value_idx,
			                      cb.assign_type, cb.item_en);
			return LC_PARSE_RES_OK_NORMAL_CFG;

		case LC_PARSE_STATE_INCLUDE:
			lc_info("path:%s\n", ctrl_blk->inc_path_buff);
			return LC_PARSE_RES_OK_INCLUDE;

		case LC_PARSE_STATE_NORMAL_CFG:
			ctrl_blk->item_value_buff[cb.value_idx] = '\0';
			ret = lc_add_cfg_item(ctrl_blk, &cfg_list->node, ctrl_blk->item_name_buff,
			                     cb.name_idx, ctrl_blk->item_value_buff, cb.value_idx,
			                     cb.assign_type, cb.item_en);
			return LC_PARSE_RES_OK_NORMAL_CFG;
		}

		cb.curr_state = cb.next_state;
		ch = ctrl_blk->line_buff[cb.char_idx++];
		ctrl_blk->colu_num++;
	}

	return -cb.next_state - 1;
}

/*************************************************************************************
 * @brief: check parameters for parsing the cfg file.
 * 
 * @param ctrl_blk: control block.
 * @param cfg_file: cfg file name.
 * 
 * @return parse result.
 ************************************************************************************/
static int lc_parse_check_params(struct lc_ctrl_blk *ctrl_blk, char *cfg_file)
{
	if (cfg_file == NULL)
		return LC_PARSE_RES_ERR_MEMORY_FAULT;

	if (strlen(cfg_file) >= ctrl_blk->line_buff_size) {
		lc_err("file name is too long[%lld], please check it\n", strlen(cfg_file));
		return LC_PARSE_RES_ERR_LINE_BUFF_OVERFLOW;
	}

	return 0;
}

/*************************************************************************************
 * @brief: parse the subfile.
 * 
 * @param ctrl_blk: control block.
 * @param is_default_cfg: whether the subfile is default config file.
 * @param file_item: file item that need to parse.
 * 
 * @return zero on success, otherwise on failure.
 ************************************************************************************/
static int lc_parse_cfg_file_with_position(struct lc_ctrl_blk *ctrl_blk, bool is_default_cfg,
                                                         struct lc_cfg_file_item *file_item)
{
	int ret;
	char *pch;
	FILE *fp;
	size_t str_len = 0;
	size_t line_num = 0;
	char *file_name = file_item->file_name;
	struct lc_cfg_file_item inc_file_item;

	ret = lc_parse_check_params(ctrl_blk, file_name);
	if (ret < 0) {
		lc_err("lc_parse_check_params failed, ret:%d\n", ret);
		return ret;
	}

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		lc_err("Fail to open file %s\n", file_name);
		return LC_PASER_RES_ERR_FILE_NOT_FOUND;
	}

	ret = fsetpos(fp, &file_item->position);
	if (ret < 0) {
		lc_err("fgetpos fail, ret:%d\n", ret);
		goto out;
	}

	/* parse file line by line */
	while (pch = fgets(ctrl_blk->line_buff + str_len, ctrl_blk->line_buff_size - (str_len + 2), fp)) {
		line_num++;
		str_len = strlen(ctrl_blk->line_buff);
		if (str_len >= ctrl_blk->line_buff_size) {
			lc_err("line[%llu] is too long, please check it\n", line_num);
			goto out;
		}

		/* skip comment line and empty line */
		if (ctrl_blk->line_buff[0] == '#' || ctrl_blk->line_buff[0] == '\n') {
			memset(ctrl_blk->line_buff, 0, strlen(ctrl_blk->line_buff) + 1);
			str_len = 0;
			continue;
		}

		if (ctrl_blk->line_buff[str_len - 2] == '\\') {
			str_len -= 2;
			continue;
		}

		lc_info("parsing: %s\n", ctrl_blk->line_buff);
		ret = light_config_parse_cfg_line(ctrl_blk, line_num, is_default_cfg);
		if (ret < 0) {
			lc_err("Fail to parse %s default cfg line %llu, col[%llu], ret:%d\n",
			        file_name, line_num, ctrl_blk->colu_num, ret);
			goto out;
		}

		if (ret != LC_PARSE_RES_OK_DEPEND_CFG && ret != LC_PARSE_RES_OK_INCLUDE &&
		    ret != LC_PARSE_RES_OK_NORMAL_CFG) {
			lc_err("Fail to parse %s default cfg line %llu, col[%llu], ret:%d\n",
			        file_name, line_num, ctrl_blk->colu_num, ret);
			goto out;
		}

		if (ret == LC_PARSE_RES_OK_INCLUDE) {
			file_item->line_num = line_num;
			file_item->position = ftell(fp);
			ret = lc_cfg_file_push(ctrl_blk, file_item);
			if (ret < 0) {
				lc_err("lc_cfg_file_push failed, ret:%d\n", ret);
				goto out;
			}

			inc_file_item.file_name = ctrl_blk->inc_path_buff;
			inc_file_item.line_num = 0;
			inc_file_item.position = 0;
			ret = lc_cfg_file_push(ctrl_blk, &inc_file_item);
			if (ret < 0) {
				lc_err("lc_cfg_file_push failed, ret:%d\n", ret);
				goto out;
			}
			goto out;
		}

		str_len = 0;
	}

	ret = 0;
out:
	fclose(fp);
	return ret;
}

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
                                                         bool is_default_cfg)
{
	int ret;
	struct lc_cfg_file_item file_item;

	/* pus the first item */
	file_item.file_name = ctrl_blk->file_name_buff;
	file_item.line_num = 0;
	file_item.position = 0;
	memcpy(ctrl_blk->file_name_buff, cfg_file, strlen(cfg_file) + 1);

	ret = lc_cfg_file_push(ctrl_blk, &file_item);
	if (ret < 0) {
		lc_err("lc_cfg_file_push failed, ret:%d\n", ret);
		return ret;
	}

	/* parsing loop */
	while (ctrl_blk->cfg_file_stk.sp > 0) {
		ret = lc_cfg_file_pop(ctrl_blk, &file_item);
		if (ret < 0) {
			lc_err("lc_cfg_file_pop failed, ret:%d\n", ret);
			return ret;
		}

		ret = lc_parse_cfg_file_with_position(ctrl_blk, is_default_cfg, &file_item);
		if (ret < 0) {
			lc_err("lc_parse_cfg_file_with_position failed, ret:%d\n", ret);
			return ret;
		}
	}

	return 0;
}
