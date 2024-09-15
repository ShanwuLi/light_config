#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "logic_expr_parse.h"
#include "light_config_parse.h"
#include "light_config_parse_sm.h"
#include "light_config_parse_func.h"

/*************************************************************************************
 * @brief: parse line and get the hash value (murmur2 Hash Function).
 * 
 * @param str: string to be parsed.
 * 
 * @return: hash value.
 ************************************************************************************/
uint64_t murmur_hash2_64a(void *str)
{
	int len = strlen(str);
	uint64_t seed = 21788233;
	unsigned char *data2;
	uint64_t m = 0xc6a4a7935bd1e995LLU;
	int r = 47;
	uint64_t h = seed ^ (len * m);
	uint64_t *data = (uint64_t *)str;
	uint64_t *end = data + (len / 8);

	while(data != end) {
		uint64_t k = *data++;
		
		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	data2 = (unsigned char*)data;
	switch(len & 7) {
	case 7: h ^= ((uint64_t)data2[6]) << 48;
	case 6: h ^= ((uint64_t)data2[5]) << 40;
	case 5: h ^= ((uint64_t)data2[4]) << 32;
	case 4: h ^= ((uint64_t)data2[3]) << 24;
	case 3: h ^= ((uint64_t)data2[2]) << 16;
	case 2: h ^= ((uint64_t)data2[1]) << 8;
	case 1: h ^= ((uint64_t)data2[0]);
			h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;
	return h;
} 

/*************************************************************************************
 * @brief: list node init.
 * @param node: node of list.
 * @return none.
 ************************************************************************************/
void lc_list_init(struct lc_list_node *node)
{
	node->next = node;
	node->prev = node;
}

/*************************************************************************************
 * @brief: Determine whether this list is empty.
 * @param node: node of list.
 * @return none.
 ************************************************************************************/
bool lc_list_is_empty(struct lc_list_node *head)
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
 * @param ctrl_blk: control block.
 * @param item: item of the cfg file.
 * @return: zero on success, other on failure.
 ************************************************************************************/
static int lc_cfg_file_push(struct lc_ctrl_blk *ctrl_blk, struct lc_cfg_file_item *item)
{
	void *name;
	struct lc_cfg_file_stk *stk = &(ctrl_blk->cfg_file_stk);

	stk->sp++;
	if (stk->sp >= stk->depth) {
		lc_err("Error: push fail, cfg file num[%lld] overflow[%llu]\n", stk->sp, stk->depth);
		return LC_PARSE_RES_ERR_INC_FILE_NUM_OVERFLOW;
	}
	
	name = malloc(strlen(item->file_name) + 1);
	if (name == NULL) {
		lc_err("Error: malloc failed\n");
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	memcpy(name, item->file_name, strlen(item->file_name) + 1);
	stk->item_stk[stk->sp].file_name = name;
	stk->item_stk[stk->sp].line_num = item->line_num;
	stk->item_stk[stk->sp].position = item->position;

	return 0;
}

/*************************************************************************************
 * @brief: push a cfg file to the stack.
 * 
 * @param ctrl_blk: control block.
 * @param item: item of the cfg file.
 * @return: zero on success, other on failure.
 ************************************************************************************/
static int lc_cfg_file_pop(struct lc_ctrl_blk *ctrl_blk, struct lc_cfg_file_item *item)
{
	void *name;
	struct lc_cfg_file_stk *stk = &(ctrl_blk->cfg_file_stk);

	if (stk->sp <= 0) {
		lc_err("Error: pop fial, cfg file num[%lld] overflow\n", stk->sp);
		return LC_PARSE_RES_ERR_INC_FILE_NUM_OVERFLOW;
	}

	name = stk->item_stk[stk->sp].file_name;
	memcpy(ctrl_blk->file_name_buff, name, strlen(name) + 1);
	item->file_name = ctrl_blk->file_name_buff;
	item->line_num = stk->item_stk[stk->sp].line_num;
	item->position = stk->item_stk[stk->sp].position;
	free(name);
	stk->sp--;
	return 0;
}

/*************************************************************************************
 * @brief: get the previous cfg file name.
 * 
 * @param ctrl_blk: control block.
 * @return: previous cfg file name.
 ************************************************************************************/
static char *lc_cfg_file_get_prev_name(struct lc_ctrl_blk *ctrl_blk)
{
	void *name;
	struct lc_cfg_file_stk *stk = &(ctrl_blk->cfg_file_stk);

	if (stk->sp >= stk->depth) {
		lc_err("Error: cfg stk get prev fail, num[%lld] overflow[%llu]\n",
		        stk->sp, stk->depth);
		return NULL;
	}

	name = ((stk->sp <= 0) ? NULL : stk->item_stk[stk->sp].file_name);
	return name;
}

/*************************************************************************************
 * @brief: push a ident item to the stack.
 * 
 * @param ctrl_blk: control block.
 * @param item: the ident item.
 * 
 * @return: zero on success, other on failure.
 ************************************************************************************/
static int lc_line_ident_push(struct lc_ctrl_blk *ctrl_blk, struct lc_line_indent_item *item)
{
	struct lc_line_ident_stk *stk = &(ctrl_blk->line_ident_stk);

	stk->sp++;
	if (stk->sp >= stk->depth) {
		lc_err("Error: line ident push fail, num[%lld] overflow[%llu]\n",
		        stk->sp, stk->depth);
		return LC_PARSE_RES_ERR_INC_FILE_NUM_OVERFLOW;
	}

	stk->item_stk[stk->sp].item = item->item;
	stk->item_stk[stk->sp].indent_num = item->indent_num;
	return 0;
}

/*************************************************************************************
 * @brief: pop a ident item to the stack.
 * 
 * @param ctrl_blk: control block.
 *
 * @return: zero on success, other on failure.
 ************************************************************************************/
static int lc_line_ident_pop(struct lc_ctrl_blk *ctrl_blk)
{
	struct lc_line_ident_stk *stk = &(ctrl_blk->line_ident_stk);

	if (stk->sp <= 0) {
		lc_err("Error: line ident push fail, num[%lld] overflow[%llu]\n",
		        stk->sp, stk->depth);
		return LC_PARSE_RES_ERR_INC_FILE_NUM_OVERFLOW;
	}

	stk->sp--;
	return 0;
}

/*************************************************************************************
 * @brief: peek a ident item to the stack.
 * 
 * @param ctrl_blk: control block.
 * @param item: the ident item.
 * 
 * @return: zero on success, other on failure.
 ************************************************************************************/
static int lc_line_ident_peek_top(struct lc_ctrl_blk *ctrl_blk, struct lc_line_indent_item *item)
{
	struct lc_line_ident_stk *stk = &(ctrl_blk->line_ident_stk);

	if (stk->sp <= 0 || stk->sp >= stk->depth) {
		lc_err("Error: line ident peek stk top fail, num[%lld] overflow[%llu]\n",
		        stk->sp, stk->depth);
		return LC_PARSE_RES_ERR_INC_FILE_NUM_OVERFLOW;
	}

	item->indent_num = stk->item_stk[stk->sp].indent_num;
	item->item = stk->item_stk[stk->sp].item;
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
void lc_dump_cfg_item_list(struct lc_cfg_list *cfg_head)
{
	struct lc_cfg_item *pos;

	if (lc_list_is_empty(&cfg_head->node)) {
		lc_err("Error: cfg list is empty\n");
		return;
	}

	lc_list_for_each_entry(pos, &cfg_head->node, struct lc_cfg_item, node) {
		lc_err("Error: name:%s, len:%d, en:%d, value:%s, len:%d, assign_type:%d, hash:0x%llx\n",
		        pos->name, pos->name_len, pos->enable, pos->value, pos->value_len,
		        pos->assign_type, (ull_t)(pos->name_hashval));
	}
}

/*************************************************************************************
 * @brief: free memory.
 *
 * @param ctrl_blk: control block.
 *
 * @return: cfg_item.
 ************************************************************************************/
void light_config_deinit(struct lc_ctrl_blk *ctrl_blk)
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

		if (cfg_head->cache.items[i]->name_hashval == murmur_hash2_64a(name) &&
		    strcmp(cfg_head->cache.items[i]->name, name) == 0) {
			cfg_head->cache.ref_cnt[i]++;
			return cfg_head->cache.items[i];
		}
	}

	/* find the item in list */
	lc_info("find item[%s] in list\n", name);
	lc_list_for_each_entry_reverse(pos, &cfg_head->node, struct lc_cfg_item, node) {
		if (pos->name_hashval == murmur_hash2_64a(name)) {
			if (strcmp(pos->name, name) != 0)
				continue;

			lc_cfg_list_update_cache(cfg_head, pos);
			return pos;
		}
	}

	return NULL;
}

/*************************************************************************************
 * @brief: get the tail item of the cfg list.
 *
 * @param cfg_head: head of the list of the item.
 * @param name: name.
 *
 * @return: cfg_item.
 ************************************************************************************/
static struct lc_cfg_item *lc_get_tail_item(struct lc_cfg_list *cfg_head)
{
	if (lc_list_is_empty(&cfg_head->node))
		return NULL;

	return lc_list_last_entry(&cfg_head->node, struct lc_cfg_item, node);
}

/*************************************************************************************
 * @brief: init lc control block.
 *
 * @param ctrl_blk: control block.
 * @param line_buff_size: size of the line buff.
 * @param mem_uplimit: memory uplimit, if over the memory uplimit, return error.
 * @param each_mem_blk_size: size of each memory block.
 * @param cfg_file_num_max: max number of include files.
 * @param line_ident_max: ident number of each line.
 * 
 * @return: void.
 ************************************************************************************/
static void lc_cb_init(struct lc_ctrl_blk *ctrl_blk, ull_t line_buff_size,
                       ull_t mem_uplimit, ull_t each_mem_blk_size,
                       ull_t cfg_file_num_max, ul_t line_ident_max)
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

	ctrl_blk->cfg_file_stk.depth = cfg_file_num_max;
	ctrl_blk->cfg_file_stk.sp = 0;
	ctrl_blk->cfg_file_stk.item_stk = ctrl_blk->buff_pool + line_buff_size * 8;

	ctrl_blk->line_ident_stk.depth = line_ident_max;
	ctrl_blk->line_ident_stk.sp = 0;
	ctrl_blk->line_ident_stk.item_stk = (void *)(ctrl_blk->cfg_file_stk.item_stk + cfg_file_num_max);

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
                           uint8_t assign_type, uint8_t value_type, bool enable)
{
	uint32_t mem_size;
	struct lc_cfg_item *cfg_item;

	if (name_len >= ctrl_blk->line_buff_size || value_len >= ctrl_blk->line_buff_size) {
		lc_err("Error: name_len[%d] or value_len[%d] over buffer\n", name_len, value_len);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	if (name_len == 0) {
		lc_err("Error: name is null\n");
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	if (value_len == 0) {
		value[0] = enable ? 'y' : 'n';
		value[1] = '\0';
		value_len = 1;
	}

	mem_size = sizeof(struct lc_cfg_item) + name_len + value_len;
	cfg_item = lc_malloc(ctrl_blk, mem_size + sizeof(ull_t));
	if (cfg_item == NULL) {
		lc_err("Error: no mem to allocate cfg item\n");
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	if ((value[value_len - 1] == 'n') && (value_len == 1))
		enable = false;

	cfg_item->name = (char *)cfg_item + sizeof(struct lc_cfg_item);
	cfg_item->name_len = name_len;
	cfg_item->value = cfg_item->name + name_len + 1;
	cfg_item->value_len = value_len;
	cfg_item->name_hashval = murmur_hash2_64a(name);
	cfg_item->assign_type = assign_type;
	cfg_item->value_type = value_type;
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
	int i;
	int ret;
	char *ptr = NULL;
	struct lc_cfg_item *cfg_item;
	struct lc_line_indent_item ident_item;

	/* get top directory */
	ptr = getcwd(ctrl_blk->temp_buff, ctrl_blk->line_buff_size);
	if (ptr == NULL)
		return LC_PARSE_RES_ERR_FILE_NOT_FOUND;
	
	/* translate path to linux format */
	for (i = 0; i < strlen(ctrl_blk->temp_buff); i++) {
		if (ctrl_blk->temp_buff[i] == '\\')
			ctrl_blk->temp_buff[i] = '/';
	}

	/* add top directory cfg item (LC_TOPDIR = "./xxx") to default cfg list */
	ret = lc_add_cfg_item(ctrl_blk, &ctrl_blk->default_cfg_head.node,
	                     "LC_TOPDIR", 9, ctrl_blk->temp_buff,
	                      strlen(ctrl_blk->temp_buff),
	                      LC_ASSIGN_TYPE_DIRECT,
	                      LC_VALUE_TYPE_NORMAL, true);
	if (ret < 0) {
		lc_err("Error: LC_TOPDIR item add fail\n");
		return ret;
	}

	ret = lc_add_cfg_item(ctrl_blk, &ctrl_blk->default_cfg_head.node,
	                     "LC_SPACE", 8, " ", 1,
	                      LC_ASSIGN_TYPE_DIRECT,
	                      LC_VALUE_TYPE_NORMAL, true);
	if (ret < 0) {
		lc_err("Error: LC_TOPDIR item add fail\n");
		return ret;
	}

	/* add top directory cfg item (LC_TOPDIR = "./xxx") to menu cfg list */
	ret = lc_add_cfg_item(ctrl_blk, &ctrl_blk->menu_cfg_head.node,
	                     "LC_TOPDIR", 9, ctrl_blk->temp_buff,
	                      strlen(ctrl_blk->temp_buff),
	                      LC_ASSIGN_TYPE_DIRECT,
	                      LC_VALUE_TYPE_NORMAL, true);
	if (ret < 0) {
		lc_err("Error: LC_TOPDIR item add fail\n");
		return ret;
	}

	/* push LC_TOPDIR to line ident stack */
	cfg_item = lc_get_tail_item(&ctrl_blk->menu_cfg_head);
	ident_item.indent_num = 0;
	ident_item.item = cfg_item;
	ret = lc_line_ident_push(ctrl_blk, &ident_item);
	if (ret < 0) {
		lc_err("Error: line ident push fail\n");
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
int light_config_init(struct lc_ctrl_blk *ctrl_blk, ull_t mem_uplimit,
                      ull_t line_buff_size, ull_t cfg_file_num_max,
                      ul_t line_ident_max, ull_t each_mem_blk_size)
{
	int ret;
	ull_t mem_size;

	if (ctrl_blk == NULL) {
		lc_err("Error: ctrl_blk is NULL\n");
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	if ((line_buff_size < LC_LINE_BUFF_SIZE_MIN) ||
	    (line_buff_size > LC_LINE_BUFF_SIZE_MAX) ||
	    (mem_uplimit > LC_MEM_UPLIMIT)) {
		lc_err("Error: line_buff_size[%llu] or mem_uplimit[%llu] is too large\n",
		        line_buff_size, mem_uplimit);
		return LC_PARSE_RES_ERR_LINE_BUFF_OVERFLOW;
	}

	mem_size = line_buff_size * 8 + cfg_file_num_max * sizeof(struct lc_cfg_file_item) +
	           line_ident_max * sizeof(struct lc_line_indent_item);
	ctrl_blk->buff_pool = malloc(mem_size);
	if (ctrl_blk->buff_pool == NULL) {
		lc_err("Error: malloc buff_pool[%llu bytes] failed\n", mem_size);
		return LC_PARSE_RES_ERR_MEMORY_FAULT;
	}

	/* initialize ctrl_blk */
	lc_cb_init(ctrl_blk, line_buff_size, mem_uplimit, each_mem_blk_size,
	           cfg_file_num_max, line_ident_max);

	ret = lc_cfg_items_init(ctrl_blk);
	if (ret < 0) {
		free(ctrl_blk->buff_pool);
		lc_err("Error: lc_cfg_items_init failed, ret:%d\n", ret);
		return ret;
	}

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
 * @brief: process ident line with dependency config.
 * 
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * 
 * @param zero on success, negative value on error.
 ************************************************************************************/
static int lc_process_ident_dependency(struct lc_ctrl_blk *ctrl_blk,
                                      struct lc_parse_ctrl_blk *pcb)
{
	int ret;
	struct lc_cfg_item *cfg_item;
	struct lc_line_indent_item ident_item;

	cfg_item = lc_get_tail_item(&ctrl_blk->menu_cfg_head);
	if (cfg_item == NULL) {
		lc_err("Error: lc_get_tail_item failed\n");
		return LC_PARSE_RES_ERR_CTRL_BLK_INVALID;
	}

	ret = lc_line_ident_peek_top(ctrl_blk, &ident_item);
	if (ret < 0) {
		lc_err("Error: peek stk top failed, ret:%d\n", ret);
		return ret;
	}

	if (pcb->line_ident_num == ident_item.indent_num) {
		pcb->item_en = pcb->item_en && ident_item.item->enable;
		return 0;
	}

	if (pcb->line_ident_num == ident_item.indent_num + 1) {
		ident_item.indent_num = pcb->line_ident_num;
		ident_item.item = cfg_item;
		pcb->item_en = pcb->item_en && cfg_item->enable;
		ret = lc_line_ident_push(ctrl_blk, &ident_item);
		if (ret < 0)
			lc_err("Error: ident push failed, ret:%d\n", ret);
		return ret;
	}

	if (pcb->line_ident_num == ident_item.indent_num - 1) {
		ret = lc_line_ident_pop(ctrl_blk);
		if (ret < 0) {
			lc_err("Error: ident pop failed, ret:%d\n", ret);
			return ret;
		}

		ret = lc_line_ident_peek_top(ctrl_blk, &ident_item);
		if (ret < 0) {
			lc_err("Error: peek stk top failed, ret:%d\n", ret);
			return ret;
		}
		pcb->item_en = pcb->item_en && ident_item.item->enable;
		return 0;
	}

	lc_err("Error: indent num[%lu] is invalid\n", pcb->line_ident_num);
	return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
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
static int light_config_parse_cfg_line(struct lc_ctrl_blk *ctrl_blk,
                                ull_t line_num, bool is_default_cfg)
{
	int ret;
	char ch;
	struct lc_cfg_list *cfg_list;
	struct lc_parse_ctrl_blk cb;

	/* check parameters */
	if (ctrl_blk == NULL) {
		lc_err("Error: ctrl_blk is NULL\n");
		return LC_PARSE_RES_ERR_CTRL_BLK_INVALID;
	}

	cfg_list = is_default_cfg ? &ctrl_blk->default_cfg_head
	                          : &ctrl_blk->menu_cfg_head;
	/* initialize parse control block */
	memset(&cb, 0, sizeof(struct lc_parse_ctrl_blk));
	cb.item_en = true;
	cb.assign_type = LC_ASSIGN_TYPE_DIRECT;
	cb.line_num = line_num;
	ch = ctrl_blk->line_buff[cb.char_idx++];
	ctrl_blk->colu_num = 0;

	/* parse line accroding to state machine */
	while (ch != '\0') {
		cb.next_state = lc_parse_state_get_next(cb.curr_state, ch);
		if (cb.next_state < 0)
			return cb.next_state;

		/* parse line */
		switch (cb.next_state) {
		case 0:
			cb.line_ident_num++;
			break;

		case 1:
			ret = lc_process_ident_dependency(ctrl_blk, &cb);
			if (ret < 0) {
				lc_err("Error: lc_process_ident failed, ret:%d\n", ret);
				return ret;
			}
			ctrl_blk->item_name_buff[cb.name_idx++] = ch;
			break;

		case 2:
			ctrl_blk->item_name_buff[cb.name_idx] = '\0';
			break;

		case 3:
			if (is_default_cfg) {
				lc_err("Error: only allow use '=' in %s line %llu, col %llu\n",
				        ctrl_blk->file_name_buff, line_num, ctrl_blk->colu_num);
				return -cb.next_state - 1;
			}
			cb.assign_type = LC_ASSIGN_TYPE_IMMEDIATE;
			break;

		case 4:
			if (is_default_cfg) {
				lc_err("Error: only allow use '=' in %s line %llu, col %llu\n",
				        ctrl_blk->file_name_buff, line_num, ctrl_blk->colu_num);
				return -cb.next_state - 1;
			}
			cb.assign_type = LC_ASSIGN_TYPE_ADDITION;
			break;

		case 5:
			if (is_default_cfg) {
				lc_err("Error: only allow use '=' in %s line %llu, col %llu\n",
				        ctrl_blk->file_name_buff, line_num, ctrl_blk->colu_num);
				return -cb.next_state - 1;
			}
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
			cb.ref_item = lc_find_cfg_item(&ctrl_blk->menu_cfg_head, ctrl_blk->ref_name_buff);
			if (cb.ref_item == NULL) {
				lc_err("Error: ref macro[%s] not found in %s line %llu, col %llu\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;
			}
			cb.item_en = cb.item_en && cb.ref_item->enable;
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
				lc_err("Error: ref macro[%s] not found in %s line %llu, col %llu\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return ret;
			}
			cb.path_idx += ret;
			break;
	
		case 27:
			cb.item_en = false;
			break;

		/* skip comment line */
		case 28:
			return LC_PARSE_RES_OK_NORMAL_CFG;
		
		case 100:
			if (is_default_cfg)
				return -cb.next_state - 1;
			cb.value_idx = 0;
			cb.value_type = LC_VALUE_TYPE_MENU;
			cb.default_item = lc_find_cfg_item(&ctrl_blk->default_cfg_head,
			                                     ctrl_blk->item_name_buff);
			cb.item_en = cb.item_en && ((cb.default_item == NULL) ?
			                           true : cb.default_item->enable);
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
				lc_err("Error: ref macro[%s] not found in %s line %llu, col %llu\n",
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
				lc_err("Error: ref macro[%s] not found in %s line %llu, col %llu\n",
				        ctrl_blk->item_name_buff, ctrl_blk->file_name_buff,
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
			cb.ref_item = lc_find_cfg_item(&ctrl_blk->menu_cfg_head, ctrl_blk->ref_name_buff);
			if (cb.ref_item == NULL) {
				lc_err("Error: ref macro[%s] not found in %s line %llu, col %llu\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;
			}
			break;
		
		case 154:
			ret = lc_parse_func_select_init(ctrl_blk, &cb, ch);
			if (ret < 0)
				return -cb.next_state - 1;;
			break;
		
		case 204:
			ret = lc_parse_func_menu_init(ctrl_blk, &cb, ch);
			if (ret < 0)
				return -cb.next_state - 1;
			break;
		
		case 216:
			ret = lc_parse_func_compare_init(ctrl_blk, &cb, ch);
			if (ret < 0)
				return -cb.next_state - 1;
			break;
		
		case 224:
			ret = lc_parse_func_range_init(ctrl_blk, &cb, ch);
			if (ret < 0)
				return -cb.next_state - 1;
			break;
		
		case 7000:
			cb.temp_idx = 0;
			break;
		
		case 7002:
			cb.arr_elem_ch_idx = 0;
			if (cb.parse_elem_start != NULL) {
				ret = cb.parse_elem_start(ctrl_blk, &cb, ch);
				if (ret < 0)
					return -cb.next_state - 1;
			}
			break;

		case 7003:
			if (cb.parsing_elem != NULL) {
				ret = cb.parsing_elem(ctrl_blk, &cb, ch);
				if (ret < 0)
					return -cb.next_state - 1;
			}
			cb.arr_elem_ch_idx++;
			break;
		
		case 7004:
			if (cb.parse_elem_end != NULL) {
				ret = cb.parse_elem_end(ctrl_blk, &cb, ch);
				if (ret < 0)
					return -cb.next_state - 1;
			}
			cb.arr_elem_idx++;
			break;

		case 7006:
			if (cb.parse_array_terminal != NULL) {
				ret = cb.parse_array_terminal(ctrl_blk, &cb, ch);
				if (ret < 0)
					return ret;
			}
			break;
		
		case 7007:
			cb.ref_name_idx = 0;
			break;
		
		case 7008:
			ctrl_blk->ref_name_buff[cb.ref_name_idx++] = ch;
			break;
		
		case 7009:
			ctrl_blk->ref_name_buff[cb.ref_name_idx] = '\0';
			cb.ref_item = lc_find_cfg_item(&ctrl_blk->menu_cfg_head, ctrl_blk->ref_name_buff);
			if (cb.ref_item == NULL) {
				lc_err("Error: ref macro[%s] not found in %s line %llu, col %llu\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;
			}
			if (cb.parsing_elem != NULL) {
				for (int i = 0; i < cb.ref_item->value_len; i++) {
					ret = cb.parsing_elem(ctrl_blk, &cb, cb.ref_item->value[i]);
					if (ret < 0)
						return -cb.next_state - 1;
					cb.arr_elem_ch_idx++;
				}
				break;
			}
			cb.arr_elem_idx += cb.ref_item->value_len;
			break;

		case 7050:
			ctrl_blk->item_value_buff[cb.value_idx] = '\0';
			if (!cb.item_en)
				ctrl_blk->line_buff[cb.char_idx] = '\n';
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
			cb.ref_item = lc_find_cfg_item(&ctrl_blk->menu_cfg_head, ctrl_blk->ref_name_buff);
			if (cb.ref_item == NULL) {
				lc_err("Error: ref macro[%s] not found in %s line %llu, col %llu\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          line_num, ctrl_blk->colu_num);
				return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;
			}
			ctrl_blk->logic_expr_buff[cb.expr_idx++] = (cb.ref_item->enable ? 'y' : 'n');
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
				lc_err("Error: ref macro[%s] not found in %s line %llu, col %llu\n",
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
			                      cb.assign_type, cb.value_type, cb.item_en);
			if (ret < 0) {
				lc_err("Error: lc_add_cfg_item failed, ret:%d\n", ret);
				return ret;
			}
			return LC_PARSE_RES_OK_NORMAL_CFG;

		case LC_PARSE_STATE_INCLUDE:
			lc_info("path:%s\n", ctrl_blk->inc_path_buff);
			ret = lc_process_ident_dependency(ctrl_blk, &cb);
			if (ret < 0) {
				lc_err("Error: lc_process_ident failed, ret:%d\n", ret);
				return ret;
			}
			ret = lc_add_cfg_item(ctrl_blk, &cfg_list->node, "-include ", 9, 
			                      ctrl_blk->inc_path_buff, cb.path_idx,
			                      LC_ASSIGN_TYPE_PATH, LC_VALUE_TYPE_PATH, cb.item_en);
			if (ret < 0) {
				lc_err("Error: lc_add_cfg_item failed, ret:%d\n", ret);
				return ret;
			}
			return cb.item_en ? LC_PARSE_RES_OK_INCLUDE : LC_PARSE_RES_OK_NORMAL_CFG;

		case LC_PARSE_STATE_NORMAL_CFG:
			ctrl_blk->item_value_buff[cb.value_idx] = '\0';
			ret = lc_add_cfg_item(ctrl_blk, &cfg_list->node, ctrl_blk->item_name_buff,
			                     cb.name_idx, ctrl_blk->item_value_buff, cb.value_idx,
			                     cb.assign_type, cb.value_type, cb.item_en);
			if (ret < 0) {
				lc_err("Error: lc_add_cfg_item failed, ret:%d\n", ret);
				return ret;
			}
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
	long int len;

	if (cfg_file == NULL)
		return LC_PARSE_RES_ERR_MEMORY_FAULT;

	len = strlen(cfg_file);
	if (len >= ctrl_blk->line_buff_size) {
		lc_err("Error: file name is too long[%ld], please check it\n", len);
		return LC_PARSE_RES_ERR_LINE_BUFF_OVERFLOW;
	}

	return 0;
}

/*************************************************************************************
 * @brief: get the line content of the file.
 * 
 * @param ctrl_blk: control block.
 * @param fp: file pointer.
 * @param buff_offset: offset of buffer.
 * 
 * @return pointer of line.
 ************************************************************************************/
static char *lc_get_line(struct lc_ctrl_blk *ctrl_blk, FILE *fp, ull_t buff_offset)
{
	char *pch;

	pch = fgets(ctrl_blk->line_buff + buff_offset,
	            ctrl_blk->line_buff_size - (buff_offset + 2),
	            fp);
	return pch;
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
static int lc_parse_cfg_file_with_item(struct lc_ctrl_blk *ctrl_blk, bool is_default_cfg,
                                                      struct lc_cfg_file_item *file_item)
{
	int ret;
	char *pch;
	FILE *fp;
	ull_t len = 0;
	char *prev_file_name;
	ull_t line_num = file_item->line_num;
	char *file_name = file_item->file_name;
	struct lc_cfg_file_item inc_file_item;

	ret = lc_parse_check_params(ctrl_blk, file_name);
	if (ret < 0) {
		lc_err("Error: lc_parse_check_params failed, ret:%d\n", ret);
		return ret;
	}

	prev_file_name = lc_cfg_file_get_prev_name(ctrl_blk);
	prev_file_name = (prev_file_name == NULL ? "top file" : prev_file_name);

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		lc_err("Error: fail to open file %s, in %s line %llu\n",
		        file_name, prev_file_name, file_item->line_num);
		return LC_PARSE_RES_ERR_FILE_NOT_FOUND;
	}

	ret = fsetpos(fp, &file_item->position);
	if (ret < 0) {
		lc_err("Error: fgetpos fail, ret:%d\n", ret);
		goto out;
	}

	/* parse file line by line */
	for (pch = lc_get_line(ctrl_blk, fp, len); pch; pch = lc_get_line(ctrl_blk, fp, len)) {

		line_num++;
		len = strlen(ctrl_blk->line_buff);
		if (len >= ctrl_blk->line_buff_size) {
			lc_err("Error: line %llu is too long, please check it\n", line_num);
			goto out;
		}

		/* skip comment line and empty line */
		if (ctrl_blk->line_buff[0] == '#' || ctrl_blk->line_buff[0] == '\n') {
			memset(ctrl_blk->line_buff, 0, strlen(ctrl_blk->line_buff) + 1);
			len = 0;
			continue;
		}

		if (ctrl_blk->line_buff[len - 2] == '\\') {
			len -= 2;
			continue;
		}

		lc_info("parsing: %s\n", ctrl_blk->line_buff);
		ret = light_config_parse_cfg_line(ctrl_blk, line_num, is_default_cfg);
		if (ret < 0) {
			lc_err("Error: fail to parse %s line %llu, col %llu, ret:%d\n",
			        file_name, line_num, ctrl_blk->colu_num, ret);
			goto out;
		}

		if (ret != LC_PARSE_RES_OK_INCLUDE && ret != LC_PARSE_RES_OK_NORMAL_CFG) {
			lc_err("Error: fail to parse %s line %llu, col[%llu], ret:%d\n",
			        file_name, line_num, ctrl_blk->colu_num, ret);
			goto out;
		}

		if (ret == LC_PARSE_RES_OK_INCLUDE) {
			file_item->line_num = line_num;
			fgetpos(fp, &file_item->position);
			ret = lc_cfg_file_push(ctrl_blk, file_item);
			if (ret < 0) {
				lc_err("Error: lc_cfg_file_push failed, ret:%d\n", ret);
				goto out;
			}

			inc_file_item.file_name = ctrl_blk->inc_path_buff;
			inc_file_item.line_num = 0;
			memset(&inc_file_item.position, 0, sizeof(fpos_t));
			ret = lc_cfg_file_push(ctrl_blk, &inc_file_item);
			if (ret < 0) {
				lc_err("Error: lc_cfg_file_push failed, ret:%d\n", ret);
				goto out;
			}
			goto out;
		}

		len = 0;
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

	/* push the first item */
	file_item.file_name = ctrl_blk->file_name_buff;
	file_item.line_num = 0;

	memset(&file_item.position, 0, sizeof(fpos_t));
	memcpy(ctrl_blk->file_name_buff, cfg_file, strlen(cfg_file) + 1);

	ret = lc_cfg_file_push(ctrl_blk, &file_item);
	if (ret < 0) {
		lc_err("Error: lc_cfg_file_push failed, ret:%d\n", ret);
		return ret;
	}

	/* parsing loop */
	while (ctrl_blk->cfg_file_stk.sp > 0) {
		ret = lc_cfg_file_pop(ctrl_blk, &file_item);
		if (ret < 0) {
			lc_err("Error: lc_cfg_file_pop failed, ret:%d\n", ret);
			return ret;
		}

		if (file_item.line_num == 0)
			printf("Parsing %s\n", file_item.file_name);

		ret = lc_parse_cfg_file_with_item(ctrl_blk, is_default_cfg, &file_item);
		if (ret < 0) {
			lc_err("Error: fail to parse %s, ret:%d\n", file_item.file_name, ret);
			return ret;
		}
	}

	return 0;
}
