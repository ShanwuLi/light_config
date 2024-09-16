#include <stdlib.h>
#include <string.h>
#include "light_config_parse_func.h"

/*************************************************************************************
 * @brief: parse select function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parsing_elem_func_select(struct lc_ctrl_blk *ctrl_blk,
                             struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->select == pcb->arr_elem_idx)
		ctrl_blk->temp_buff[pcb->arr_elem_ch_idx] = ch;
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
static int lc_parse_elem_end_func_select(struct lc_ctrl_blk *ctrl_blk,
                               struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->arr_elem_idx > 2) {
		lc_err("Error: element num[%d] must be 2 with the [@<MACRO>] ? ([x], [y])\n",
		        pcb->arr_elem_idx);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	if (pcb->select == pcb->arr_elem_idx) {
		ctrl_blk->temp_buff[pcb->arr_elem_ch_idx] = '\0';
		memcpy(ctrl_blk->item_value_buff, ctrl_blk->temp_buff, pcb->arr_elem_ch_idx);
		pcb->value_idx += pcb->arr_elem_ch_idx;
	}

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
static int lc_parse_array_terminal_func_select(struct lc_ctrl_blk *ctrl_blk,
                                     struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->arr_elem_idx > 2) {
		lc_err("Error: element num[%d] must be 2 with the [@<MACRO>] ? ([x], [y])\n",
		        pcb->arr_elem_idx);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	pcb->match_state = 0;
	return 0;
}

/*************************************************************************************
 * @brief: parse select init function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
int lc_parse_func_select_init(struct lc_ctrl_blk *ctrl_blk,
                    struct lc_parse_ctrl_blk *pcb, char ch)
{
	pcb->parsing_elem = lc_parsing_elem_func_select;
	pcb->parse_elem_end = lc_parse_elem_end_func_select;
	pcb->parse_array_terminal = lc_parse_array_terminal_func_select;

	if (pcb->match_state != 0) {
		pcb->select = (pcb->match_state == 1 ? 0 : 1);
		return 0;
	}

	if (pcb->ref_item == NULL) {
		lc_err("Error: ref macro[%s] not found in %s line %llu, col %llu\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          pcb->line_num, ctrl_blk->colu_num);
		return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;
	}

	pcb->select = (pcb->ref_item->enable ? 0 : 1);
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
static int lc_parse_elem_start_func_menu(struct lc_ctrl_blk *ctrl_blk,
                               struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state < 0)
		pcb->match_state = 0;
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
static int lc_parsing_elem_func_menu(struct lc_ctrl_blk *ctrl_blk,
                           struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state != 0 || pcb->arr_elem_ch_idx >= pcb->default_item->value_len)
		return 0;

	if (ch != pcb->default_item->value[pcb->arr_elem_ch_idx])
		pcb->match_state = -1;

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
static int lc_parse_elem_end_func_menu(struct lc_ctrl_blk *ctrl_blk,
                             struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state == 0 && pcb->arr_elem_ch_idx == pcb->default_item->value_len)
		pcb->match_state = 1;

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
static int lc_parse_array_terminal_func_menu(struct lc_ctrl_blk *ctrl_blk,
                                   struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state != 1) {
		lc_err("Error: menu element[%s] not found in %s\n", pcb->default_item->value,
		        ctrl_blk->item_name_buff);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	pcb->match_state = 0;
	memcpy(ctrl_blk->item_value_buff, pcb->default_item->value,
	       pcb->default_item->value_len);
	pcb->value_idx += pcb->default_item->value_len;
	return 0;
}

/*************************************************************************************
 * @brief: parse menu init function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
int lc_parse_func_menu_init(struct lc_ctrl_blk *ctrl_blk, struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->default_item == NULL) {
		lc_err("Error: item[%s] not defined in default config when parsing %s line %llu, col %llu\n",
		        ctrl_blk->item_name_buff, ctrl_blk->file_name_buff, pcb->line_num, ctrl_blk->colu_num);
		return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;
	}

	pcb->parse_array_terminal = lc_parse_array_terminal_func_menu;
	if (!pcb->item_en) {
		pcb->match_state = 1;
		return 0;
	}

	pcb->parse_elem_start = lc_parse_elem_start_func_menu;
	pcb->parse_elem_end = lc_parse_elem_end_func_menu;
	pcb->parsing_elem = lc_parsing_elem_func_menu;

	return 0;
}

/*************************************************************************************
 * @brief: parse range function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parse_elem_start_func_range(struct lc_ctrl_blk *ctrl_blk,
                                struct lc_parse_ctrl_blk *pcb, char ch)
{
	pcb->curr_sign = 0;
	pcb->match_state = 0; /* compare */
	pcb->select = 0; /* dot location */
	return 0;
}

/*************************************************************************************
 * @brief: parse range function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parsing_elem_func_range(struct lc_ctrl_blk *ctrl_blk,
                            struct lc_parse_ctrl_blk *pcb, char ch)
{
	char default_ch;
	int default_vel_ch_idx;

	if (ch == '-' && pcb->curr_sign == 0) {
		pcb->curr_sign = 1;
		return 0;
	}

	if (ch == '.' && pcb->select == 0)
		pcb->select = pcb->arr_elem_ch_idx;

	if (pcb->match_state != 0 || pcb->arr_elem_ch_idx >= pcb->default_item->value_len)
		return 0;

	default_vel_ch_idx = pcb->arr_elem_ch_idx - pcb->curr_sign + pcb->default_sign;
	default_ch = pcb->default_item->value[default_vel_ch_idx];
	pcb->match_state = ch - default_ch;

	return 0;
}

/*************************************************************************************
 * @brief: compare current value to default value.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 *
 * @return: current value - default value.
 ************************************************************************************/
static int lc_compare_range(struct lc_ctrl_blk *ctrl_blk, struct lc_parse_ctrl_blk *pcb)
{
	int ret = 0;

	if (pcb->curr_sign != pcb->default_sign)
		return pcb->default_sign - pcb->curr_sign;

	if (pcb->select != pcb->location) {
		ret = pcb->select - pcb->location;
		goto out;
	}

	if (pcb->match_state != 0) {
		ret = pcb->match_state;
		goto out;
	}

	if (pcb->arr_elem_ch_idx != pcb->default_item->value_len)
		ret = pcb->arr_elem_ch_idx - pcb->default_item->value_len;

out:
	if (pcb->default_sign + pcb->curr_sign != 0)
		ret = -ret;

	return ret;
}

/*************************************************************************************
 * @brief: parse range function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parse_elem_end_func_range(struct lc_ctrl_blk *ctrl_blk,
                              struct lc_parse_ctrl_blk *pcb, char ch)
{
	int compare_curr_default;

	pcb->select = (pcb->select == 0) ? pcb->arr_elem_ch_idx : pcb->select;
	compare_curr_default = lc_compare_range(ctrl_blk, pcb);

	switch (pcb->arr_elem_idx) {
	case 0:
		if (compare_curr_default > 0) {
			lc_err("Error: %s out of range left of item[%s] when parsing %s line %llu, col %llu, ret:%d\n",
				pcb->default_item->value, pcb->default_item->name, ctrl_blk->file_name_buff,
				pcb->line_num, ctrl_blk->colu_num, compare_curr_default);
			//printf("select:%d, arr_elem_ch_idx:%d, default_sign:%d, curr_sign:%d, match_state:%d\n",
			//        pcb->select, pcb->arr_elem_ch_idx, pcb->default_sign, pcb->curr_sign, pcb->match_state);
			return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
		}
		break;

	case 1:
		if (compare_curr_default < 0) {
			lc_err("Error: %s out of range right of item[%s] when parsing %s line %llu, col %llu, ret:%d\n",
				pcb->default_item->value, pcb->default_item->name, ctrl_blk->file_name_buff,
				pcb->line_num, ctrl_blk->colu_num, compare_curr_default);
			//printf("select:%d, arr_elem_ch_idx:%d, default_sign:%d, curr_sign:%d, match_state:%d\n",
			//        pcb->select, pcb->arr_elem_ch_idx, pcb->default_sign, pcb->curr_sign, pcb->match_state);
			return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
		}
		break;

	default:
		lc_err("Error: item[%s] has over 2 arguments of range func in %s line %llu, col %llu\n",
		        pcb->default_item->name, ctrl_blk->file_name_buff, pcb->line_num,
		        ctrl_blk->colu_num);
		//printf("select:%d, arr_elem_ch_idx:%d, default_sign:%d, curr_sign:%d, match_state:%d\n",
		//        pcb->select, pcb->arr_elem_ch_idx, pcb->default_sign, pcb->curr_sign, pcb->match_state);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

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
static int lc_parse_array_terminal_func_range(struct lc_ctrl_blk *ctrl_blk,
                                    struct lc_parse_ctrl_blk *pcb, char ch)
{
	pcb->match_state = 0;
	memcpy(ctrl_blk->item_value_buff, pcb->default_item->value,
	       pcb->default_item->value_len);
	pcb->value_idx += pcb->default_item->value_len;
	return 0;
}

/*************************************************************************************
 * @brief: parse range init function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
int lc_parse_func_range_init(struct lc_ctrl_blk *ctrl_blk, struct lc_parse_ctrl_blk *pcb, char ch)
{
	int i;
	int dot_num = 0;

	if (pcb->default_item == NULL) {
		lc_err("Error: item[%s] not defined in default config when parsing %s line %llu, col %llu\n",
		        ctrl_blk->item_name_buff, ctrl_blk->file_name_buff,
		        pcb->line_num, ctrl_blk->colu_num);
		return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;
	}

	pcb->default_sign = 0;
	if (pcb->default_item->value[0] == '-')
		pcb->default_sign = 1;

	pcb->location = pcb->default_item->value_len;
	for (i = 0; i < pcb->default_item->value_len; i++) {
		if (pcb->default_item->value[i] == '.') {
			pcb->location = i;
			dot_num++;
		}
	}

	if (dot_num > 1) {
		lc_err("Error: invalid item[%s] when parsing %s line %llu, col %llu\n",
		        ctrl_blk->item_name_buff, ctrl_blk->file_name_buff,
		        pcb->line_num, ctrl_blk->colu_num);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	pcb->parse_elem_start = lc_parse_elem_start_func_range;
	pcb->parsing_elem = lc_parsing_elem_func_range;
	pcb->parse_elem_end = lc_parse_elem_end_func_range;
	pcb->parse_array_terminal = lc_parse_array_terminal_func_range;

	return 0;
}

/*************************************************************************************
 * @brief: parse compare function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parse_elem_start_func_compare(struct lc_ctrl_blk *ctrl_blk,
                                  struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->arr_elem_idx == 2) {
		lc_err("Error: element over num:%d of compare func\n", pcb->arr_elem_idx);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	pcb->temp_idx = 0;
	return 0;
}

/*************************************************************************************
 * @brief: parse compare function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parsing_elem_func_compare(struct lc_ctrl_blk *ctrl_blk,
                              struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state != 0)
		return 0;

	switch (pcb->arr_elem_idx) {
	case 0:
		ctrl_blk->temp_buff[pcb->arr_elem_ch_idx] = ch;
		break;
	
	case 1:
		if (pcb->arr_elem_ch_idx > pcb->location) {
			pcb->match_state = -1;
			return 0;
		}
		if (ch != ctrl_blk->temp_buff[pcb->arr_elem_ch_idx]) {
			pcb->match_state = -1;
			return 0;
		}
		break;
	
	default:
		lc_err("Error: element over num:%d of compare func\n", pcb->arr_elem_idx);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	return 0;
}

/*************************************************************************************
 * @brief: parse compare function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parse_elem_end_func_compare(struct lc_ctrl_blk *ctrl_blk,
                              struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state != 0)
		return 0;

	switch (pcb->arr_elem_idx) {
	case 0:
		pcb->location = pcb->arr_elem_ch_idx;
		break;
	
	case 1:
		if (pcb->arr_elem_ch_idx == pcb->location)
			pcb->match_state = 1;
		else
			pcb->match_state = -1;
		break;
	
	default:
		lc_err("Error: element over num:%d of compare func\n", pcb->arr_elem_idx);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	return 0;
}

/*************************************************************************************
 * @brief: parse compare terminal function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
static int lc_parse_array_terminal_func_compare(struct lc_ctrl_blk *ctrl_blk,
                                      struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state != -1 && pcb->match_state != 1) {
		lc_err("Error: element invalid num:%d of compare func\n", pcb->arr_elem_idx);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	return 0;
}

/*************************************************************************************
 * @brief: parse range init function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
int lc_parse_func_compare_init(struct lc_ctrl_blk *ctrl_blk, struct lc_parse_ctrl_blk *pcb, char ch)
{
	pcb->temp_idx = 0;
	pcb->location = 0;
	pcb->match_state = 0;

	pcb->parse_elem_start = lc_parse_elem_start_func_compare;
	pcb->parsing_elem = lc_parsing_elem_func_compare;
	pcb->parse_elem_end = lc_parse_elem_end_func_compare;
	pcb->parse_array_terminal = lc_parse_array_terminal_func_compare;

	return 0;
}