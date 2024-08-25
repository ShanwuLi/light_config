#ifndef __LIGHT_CONFIG_SM_H__
#define __LIGHT_CONFIG_SM_H__

#include <stdint.h>

enum lc_parse_state {
	LC_PARSE_STATE_INCLUDE = 99,
	LC_PARSE_STATE_DEPEND_CFG = 7999,
	LC_PARSE_STATE_NORMAL_CFG = 8999,
};

/*************************************************************************************
 * @brief: state machine to parse the config file
 * 
 * @param current_state: current state of the state machine.
 * @param ch: current character to be parsed.
 * 
 * @return next state of the state machine.
 ************************************************************************************/
int lc_parse_state_get_next(int current_state, char ch);

#endif