#ifndef __LOGIC_OPS_H__
#define __LOGIC_OPS_H__

/*************************************************************************************
 * @brief: parse the logic expression.
 * 
 * @param input: Pointer to the input string.
 * @return: >= 0: value.
 *          <  0: error equal to -(error position + 1);
 ************************************************************************************/
int lc_parse_logic_express(char *input);

#endif
