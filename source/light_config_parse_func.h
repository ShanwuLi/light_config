#ifndef __LIGHT_CONFIG_PARSE_FUNC_H__
#define __LIGHT_CONFIG_PARSE_FUNC_H__

#include "light_config_parse.h"

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
                    struct lc_parse_ctrl_blk *pcb, char ch);

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
                     struct lc_parse_ctrl_blk *pcb, char ch);

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
                              struct lc_parse_ctrl_blk *pcb, char ch);

/*************************************************************************************
 * @brief: parse menu init function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
int lc_parse_func_menu_init(struct lc_ctrl_blk *ctrl_blk, struct lc_parse_ctrl_blk *pcb, char ch);

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
                         struct lc_parse_ctrl_blk *pcb, char ch);

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
                   struct lc_parse_ctrl_blk *pcb, char ch);

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
                            struct lc_parse_ctrl_blk *pcb, char ch);

/*************************************************************************************
 * @brief: parse range init function.
 *
 * @param ctrl_blk: control block.
 * @param pcb: parse control block.
 * @param ch: char.
 *
 * @return: zero on success, else error code.
 ************************************************************************************/
int lc_parse_func_range_init(struct lc_ctrl_blk *ctrl_blk, struct lc_parse_ctrl_blk *pcb, char ch);

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
                         struct lc_parse_ctrl_blk *pcb, char ch);

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
                     struct lc_parse_ctrl_blk *pcb, char ch);

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
                      struct lc_parse_ctrl_blk *pcb, char ch);

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
                             struct lc_parse_ctrl_blk *pcb, char ch);

#endif
