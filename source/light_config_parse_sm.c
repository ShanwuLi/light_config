#include "light_config_parse_sm.h"

/*************************************************************************************
 * @brief: state machine to parse the config file
 * 
 * @param current_state: current state of the state machine.
 * @param ch: current character to be parsed.
 * 
 * @return next state of the state machine.
 ************************************************************************************/
int lc_parse_state_get_next(int current_state, char ch)
{
	switch (current_state) {
	/*---------------------------------- name ----------------------------- */
	case 0:
		if (ch == '-')
			return 12;
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
			return 1;
		break;

	case 1:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
		    (ch >= '0' && ch <= '9') || ch == '_')
			return 1;
		if (ch == ' ')
			return 2;
		if (ch == '-')
			return 8;
		break;

	case 2:
		if (ch == ' ')
			return 2;
		if (ch == ':')
			return 3;
		if (ch == '+')
			return 4;
		if (ch == '?')
			return 5;
		if (ch == '=')
			return 6;
		if (ch == '"')
			return 21;
		break;

	case 3:
		if (ch == '=')
			return 6;
		break;

	case 4:
		if (ch == '=')
			return 6;
		break;

	case 5:
		if (ch == '=')
			return 6;
		break;

	case 6:
		if (ch == ' ')
			return 7;
		break;

	case 7:
		if (ch == ' ')
			return 7;
		if (ch == '[')
			return 100;
		if (ch == '<')
			return 8001;
		return 8000;

	case 8:
		if (ch == '<')
			return 9;
		if (ch == 'n')
			return 27;
		break;

	case 9:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
			return 10;
		break;

	case 10:
		if (ch == '>')
			return 11;
		if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
		    (ch >= 'a' && ch <= 'z') || ch == '_')
			return 10;
		break;

	case 11:
		if (ch == ' ')
			return 2;
		break;

	case 12:
		if (ch == 'i')
			return 13;
		break;

	case 13:
		if (ch == 'n')
			return 14;
		break;
	
	case 14:
		if (ch == 'c')
			return 15;
		break;
	
	case 15:
		if (ch == 'l')
			return 16;
		break;
	
	case 16:
		if (ch == 'u')
			return 17;
		break;
	
	case 17:
		if (ch == 'd')
			return 18;
		break;

	case 18:
		if (ch == 'e')
			return 19;
		break;
	
	case 19:
		if (ch == ' ')
			return 20;
		if (ch == '-')
			return 8;
		break;

	case 20:
		if (ch == ' ')
			return 20;
		if (ch == '"')
			return 21;
		break;

	case 21:
		if (ch == '<')
			return 24;
		if (ch != '"' && ch != ' ')
			return 22;
		break;

	case 22:
		if (ch == '<')
			return 24;
		if (ch == '"')
			return 23;
		if (ch != ' ')
			return 22;
		break;

	case 23:
		if (ch == '\n')
			return LC_PARSE_STATE_INCLUDE;
		break;

	case 24:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
			return 25;
		break;

	case 25:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
		    (ch >= '0' && ch <= '9') || ch == '_')
			return 25;
		if (ch == '>')
			return 26;
		break;

	case 26:
		if (ch == '<')
			return 24;
		if (ch == '"')
			return 23;
		if (ch != ' ')
			return 22;
		break;
	
	case 27:
		if (ch == ' ')
			return 2;
		break;

	/*---------------------------------- dependence config ----------------------------- */
	case 100:
		if (ch == ']')
			return 7050;
		if (ch == '<')
			return 101;
		if (ch == '*')
			return 105;
		if (ch == '@')
			return 200;
		return 104;

	case 101:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
			return 102;
		break;

	case 102:
		if (ch == '>')
			return 103;
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
		    (ch >= '0' && ch <= '9') || ch == '_')
			return 102;
		break;

	case 103:
		if (ch == '<')
			return 101;
		if (ch == ']')
			return 7050;
		return 104;

	case 104:
		if (ch == '<')
			return 101;
		if (ch == ']')
			return 7050;
		return 104;

	case 105:
		if (ch == ']')
			return 7050;
		break;

	/*----------------------------- @<MACRO> ? ([x], [y]) ---------------------------*/
	case 150:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
			return 151;
		break;
	
	case 151:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
		    (ch >= '0' && ch <= '9') || ch == '_')
			return 151;
		if (ch == '>')
			return 152;
		break;
	
	case 152:
		if (ch == ' ')
			return 153;
		if (ch == '?')
			return 154;
		break;
	
	case 153:
		if (ch == ' ')
			return 153;
		if (ch == '?')
			return 154;
		break;

	case 154:
		if (ch == ' ')
			return 155;
		if (ch == '(')
			return 7000;
		break;
	
	case 155:
		if (ch == ' ')
			return 155;
		if (ch == '(')
			return 7000;
		break;

	/*----------------------------------- functions -------------------------------*/
	case 200:
		if (ch == '<')
			return 150;
		if (ch == 'm')
			return 201;
		if (ch == 't')
			return 205;
		if (ch == 'a')
			return 210;
		if (ch == 'r')
			return 220;
		break;

	case 201:
		if (ch == 'e')
			return 202;
		break;
	
	case 202:
		if (ch == 'n')
			return 203;
		break;
	
	case 203:
		if (ch == 'u')
			return 204;
		break;

	case 204:
		if (ch == '(')
			return 7000; /* menu */
		break;

	case 205:
		if (ch == 'r')
			return 206;
		break;

	case 206:
		if (ch == 'i')
			return 207;
		break;
	
	case 207:
		if (ch == 'o')
			return 208;
		break;
	
	case 208:
		if (ch == 'p')
			return 209;
		break;
	
	case 209:
		if (ch == '(')
			return 7000; /* triop */
		break;

	case 210:
		if (ch == 'd')
			return 211;
		break;

	case 211:
		if (ch == 'd')
			return 212;
		break;

	case 212:
		if (ch == '_')
			return 213;
		break;

	case 213:
		if (ch == 'p')
			return 214;
		break;

	case 214:
		if (ch == 'r')
			return 215;
		break;

	case 215:
		if (ch == 'e')
			return 216;
		break;

	case 216:
		if (ch == 'f')
			return 217;
		break;

	case 217:
		if (ch == 'i')
			return 218;
		break;

	case 218:
		if (ch == 'x')
			return 219;
		break;

	case 219:
		if (ch == '(')
			return 7000; /* aadd_prefix */
		break;

	case 220:
		if (ch == 'a')
			return 221;

	case 221:
		if (ch == 'n')
			return 222;
		break;
	
	case 222:
		if (ch == 'g')
			return 223;
		break;
	
	case 223:
		if (ch == 'e')
			return 224;
		break;

	case 224:
		if (ch == '(')
			return 7000; /* range */
		break;

	/*---------------------------------- dependences ----------------------------- */
	case 7000:
		if (ch == ' ')
			return 7001;
		if (ch == '[')
			return 7002;
		break;

	case 7001:
		if (ch == ' ')
			return 7001;
		if (ch == '[')
			return 7002;
		if (ch == ')')
			return 7006;
		break;

	case 7002:
		if (ch == '<')
			return 7007;
		if (ch != ']')
			return 7003;
		break;
	
	case 7003:
		if (ch == ']')
			return 7004;
		if (ch == '<')
			return 7007;
		return 7003;

	case 7004:
		if (ch == ',')
			return 7005;
		if (ch == ')')
			return 7006;
		break;

	case 7005:
		if (ch == ' ')
			return 7001;
		if (ch == '[')
			return 7002;
		break;

	case 7006:
		if (ch == ']')
			return 7050;
		break;
	
	case 7007:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
			return 7008;
		break;
	
	case 7008:
		if (ch == '>')
			return 7009;
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
		    (ch >= '0' && ch <= '9') || ch == '_')
			return 7008;
		break;
	
	case 7009:
		if (ch == '<')
			return 7007;
		if (ch == ']')
			return 7004;
		return 7003;

	case 7050:
		if (ch == ' ')
			return 7051;
		if (ch == '&')
			return 7052;
		if (ch == '\n')
			return LC_PARSE_STATE_DEPEND_CFG;
		break;

	case 7051:
		if (ch == ' ')
			return 7051;
		if (ch == '&')
			return 7052;
		break;

	case 7052:
		if (ch == '!' || ch == '(')
			return 7053;
		if (ch == ' ')
			return 7054;
		if (ch == '<')
			return 7055;
		break;

	case 7053:
		if (ch == '&' || ch == '|' || ch == '!' || ch == '(' || ch == ')')
			return 7053;
		if (ch == ' ')
			return 7054;
		if (ch == '<')
			return 7055;
		if (ch == '\n')
			return LC_PARSE_STATE_DEPEND_CFG;
		break;

	case 7054:
		if (ch == '&' || ch == '|' || ch == '!' || ch == '(' || ch == ')')
			return 7053;
		if (ch == ' ')
			return 7054;
		if (ch == '<')
			return 7055;
		break;

	case 7055:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
			return 7056;
		break;

	case 7056:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
		    (ch >= '0' && ch <= '9') || ch == '_')
			return 7056;
		if (ch == '>')
			return 7057;
		break;

	case 7057:
		if (ch == '\n')
			return LC_PARSE_STATE_DEPEND_CFG;
		if (ch == '&' || ch == '|' || ch == '!' || ch == '(' || ch == ')')
			return 7053;
		if (ch == ' ')
			return 7054;
		break;

	/*---------------------------------- normal config ----------------------------- */
	case 8000:
		if (ch == '\n')
			return LC_PARSE_STATE_NORMAL_CFG;
		if (ch == '<')
			return 8001;
		return 8000;

	case 8001:
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
			return 8002;
		break;

	case 8002:
		if (ch == '>')
			return 8003;
		if ((ch >= 'A' && ch <= 'Z' ) || (ch >= 'a' && ch <= 'z') ||
		    (ch >= '0' && ch <= '9') || ch == '_')
			return 8002;
		break;

	case 8003:
		if (ch == '<')
			return 8001;
		if (ch == '\n')
			return LC_PARSE_STATE_NORMAL_CFG;
		return 8000;

	default:
		break;
	}

	return -current_state - 1;
}
