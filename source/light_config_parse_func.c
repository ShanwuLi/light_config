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
int lc_parsing_elem_func_select(struct lc_ctrl_blk *ctrl_blk,
                     struct lc_parse_ctrl_blk *pcb, char ch)
{
	if (pcb->select < 0)
		return 0;

	if (pcb->select == pcb->arr_elem_num)
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
	if (pcb->select < 0)
		return 0;

	if (pcb->arr_elem_num > 2) {
		lc_err("element num[%d] must be 2 with the [@<MACRO>] ? ([x], [y])\n",
		        pcb->arr_elem_num);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	ctrl_blk->temp_buff[pcb->temp_idx] = '\0';
	memcpy(ctrl_blk->item_value_buff, ctrl_blk->temp_buff, pcb->temp_idx + 1);
	pcb->value_idx += pcb->temp_idx;
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

	if (ch != pcb->default_item->value[pcb->arr_elem_idx])
		pcb->match_state = -1;

	if ((pcb->match_state == 0) && ((pcb->arr_elem_idx + 1) == pcb->default_item->value_len))
		pcb->match_state = 1;

	pcb->temp_idx++;
	pcb->arr_elem_idx++;

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
		lc_err("menu element[%s] not found in [%s]\n", pcb->default_item->value,
		        ctrl_blk->item_name_buff);
		return LC_PARSE_RES_ERR_CFG_ITEM_INVALID;
	}

	memcpy(ctrl_blk->item_value_buff, pcb->default_item->value,
	       pcb->default_item->value_len);
	pcb->value_idx += pcb->default_item->value_len;
	return 0;
}

