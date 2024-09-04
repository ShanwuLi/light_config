#include <stdlib.h>
#include <string.h>
#include "light_config_parse_func.h"

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
	int ret;

	ret = lc_find_cfg_item_and_get_en(ctrl_blk, &ctrl_blk->menu_cfg_head,
	                                  ctrl_blk->ref_name_buff, &pcb->ref_en);
	if (ret < 0) {
		lc_err("Error: ref macro[%s] not found in %s line %llu, col %llu\n",
				          ctrl_blk->ref_name_buff, ctrl_blk->file_name_buff,
				          pcb->line_num, ctrl_blk->colu_num);
		return ret;
	}

	pcb->select = (pcb->ref_en ? 0 : 1);
	pcb->parsing_elem = lc_parsing_elem_func_select;
	pcb->parse_array_terminal = lc_parse_array_terminal_func_select;
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
int lc_parsing_elem_func_select(struct lc_ctrl_blk *ctrl_blk,
                     struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->select == pcb->arr_elem_idx)
		ctrl_blk->temp_buff[pcb->temp_idx++] = ch;
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
int lc_parse_array_terminal_func_select(struct lc_ctrl_blk *ctrl_blk,
                              struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->arr_elem_idx > 2) {
		lc_err("Error: element num[%d] must be 2 with the [@<MACRO>] ? ([x], [y])\n",
		        pcb->arr_elem_idx);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	ctrl_blk->temp_buff[pcb->temp_idx] = '\0';
	memcpy(ctrl_blk->item_value_buff, ctrl_blk->temp_buff, pcb->temp_idx + 1);
	pcb->value_idx += pcb->temp_idx;
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
	pcb->default_item = lc_find_cfg_item(&ctrl_blk->default_cfg_head,
	                                      ctrl_blk->item_name_buff);
	if (pcb->default_item == NULL) {
		lc_err("Error: item[%s] not defined in default config when parsing %s line %llu, col %llu\n",
		        ctrl_blk->item_name_buff, ctrl_blk->file_name_buff, pcb->line_num, ctrl_blk->colu_num);
		return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;
	}

	pcb->parse_elem_start = lc_parse_elem_start_func_menu;
	pcb->parsing_elem = lc_parsing_elem_func_menu;
	pcb->parse_array_terminal = lc_parse_array_terminal_func_menu;
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
int lc_parse_elem_start_func_menu(struct lc_ctrl_blk *ctrl_blk,
                         struct lc_parse_ctrl_blk *pcb, char ch)
{
	pcb->match_state = ((pcb->match_state > 0) ? pcb->match_state : 0);
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
int lc_parsing_elem_func_menu(struct lc_ctrl_blk *ctrl_blk,
                   struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state != 0)
		return 0;

	if (ch != pcb->default_item->value[pcb->arr_elem_ch_idx])
		pcb->match_state = -1;

	if ((pcb->match_state == 0) && ((pcb->arr_elem_ch_idx + 1) == pcb->default_item->value_len))
		pcb->match_state = 1;

	pcb->temp_idx++;
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
int lc_parse_array_terminal_func_menu(struct lc_ctrl_blk *ctrl_blk,
                            struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->match_state != 1) {
		lc_err("Error: menu element[%s] not found in %s\n", pcb->default_item->value,
		        ctrl_blk->item_name_buff);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

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
	pcb->default_item = lc_find_cfg_item(&ctrl_blk->default_cfg_head,
	                                      ctrl_blk->item_name_buff);
	if (pcb->default_item == NULL) {
		lc_err("Error: item[%s] not defined in default config when parsing %s line %llu, col %llu\n",
		        ctrl_blk->item_name_buff, ctrl_blk->file_name_buff,
		        pcb->line_num, ctrl_blk->colu_num);
		return LC_PARSE_RES_ERR_CFG_ITEM_NOT_FOUND;
	}

	pcb->match_state = 0;
	pcb->parse_elem_start = lc_parse_elem_start_func_range;
	pcb->parsing_elem = lc_parsing_elem_func_range;
	pcb->parse_elem_end = lc_parse_elem_end_func_range;
	pcb->parse_array_terminal = lc_parse_array_terminal_func_range;

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
int lc_parse_elem_start_func_range(struct lc_ctrl_blk *ctrl_blk,
                         struct lc_parse_ctrl_blk *pcb, char ch)
{
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
int lc_parsing_elem_func_range(struct lc_ctrl_blk *ctrl_blk,
                     struct lc_parse_ctrl_blk *pcb, char ch)
{
	switch (pcb->arr_elem_idx) {
	case 0:
		if (ch > pcb->default_item->value[pcb->arr_elem_ch_idx]) {
			lc_err("Error: %s out of range of item[%s] when parsing %s line %llu, col %llu\n",
			       pcb->default_item->value, pcb->default_item->name, ctrl_blk->file_name_buff,
			       pcb->line_num, ctrl_blk->colu_num);
			return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
		}
		break;

	case 1:
		if (ch < pcb->default_item->value[pcb->arr_elem_ch_idx]) {
			lc_err("Error: %s out of range of item[%s] when parsing %s line %llu, col %llu\n",
			       pcb->default_item->value, pcb->default_item->name, ctrl_blk->file_name_buff,
			       pcb->line_num, ctrl_blk->colu_num);
			return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
		}
		break;

	default:
		lc_err("Error: item[%s] has over 2 arguments of range func in %s line %llu, col %llu\n",
		        pcb->default_item->name, ctrl_blk->file_name_buff, pcb->line_num,
		        ctrl_blk->colu_num);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

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
int lc_parse_elem_end_func_range(struct lc_ctrl_blk *ctrl_blk,
                       struct lc_parse_ctrl_blk *pcb, char ch)
{
	switch (pcb->arr_elem_idx) {
	case 0:
		if (pcb->arr_elem_ch_idx > pcb->default_item->value_len) {
			lc_err("Error: %s out of range of item[%s] when parsing %s line %llu, col %llu\n",
				pcb->default_item->value, pcb->default_item->name, ctrl_blk->file_name_buff,
				pcb->line_num, ctrl_blk->colu_num);
			return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
		}
		break;

	case 1:
		if (pcb->arr_elem_ch_idx < pcb->default_item->value_len) {
			lc_err("Error: %s out of range of item[%s] when parsing %s line %llu, col %llu\n",
				pcb->default_item->value, pcb->default_item->name, ctrl_blk->file_name_buff,
				pcb->line_num, ctrl_blk->colu_num);
			return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
		}
		break;

	default:
		lc_err("Error: item[%s] has over 2 arguments of range func in %s line %llu, col %llu\n",
		        pcb->default_item->name, ctrl_blk->file_name_buff, pcb->line_num,
		        ctrl_blk->colu_num);
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
int lc_parse_array_terminal_func_range(struct lc_ctrl_blk *ctrl_blk,
                             struct lc_parse_ctrl_blk *pcb, char ch)
{
	memcpy(ctrl_blk->item_value_buff, pcb->default_item->value,
	       pcb->default_item->value_len);
	pcb->value_idx += pcb->default_item->value_len;
	return 0;
}
