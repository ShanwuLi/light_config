#include <stdlib.h>
#include <stdio.h>

/*************************************************************************************
 * @brief: Checks if the character at the given position in the input string matches
 *         the specified character.
 * 
 * @param input: Pointer to the input string.
 * @param pos: Pointer to an integer representing the current position,
 *             used for reading and updating the position.
 * @param c: Character to be matched.
 * 
 * @return: 0: Indicates a successful match. -1: Indicates a failed match,
 *             either the input character does not match the specified character
 *             or the input position is out of the string's length.
 ************************************************************************************/
static int match(char *input, int *pos, char c)
{
	if (input[*pos] == c) {
		(*pos)++;
		return 0;
	}

	printf("Error: %c [%d]Error: expected '%c'\n", input[*pos], *pos, c);
	return -((*pos) + 1);
}

/*************************************************************************************
 * @brief: parse the expression.
 * 
 * @param input: Pointer to the input string.
 * @param pos: Pointer to an integer representing the current position,
 *             used for reading and updating the position.
 * @return: value.
 ************************************************************************************/
static int parse_expr_tail(char *input, int *pos, int left);
static int parse_term(char *input, int *pos);
static int parse_expr(char *input, int *pos)
{
	int term = parse_term(input, pos);
	if (term < 0)
		return term;

	return parse_expr_tail(input, pos, term);
}

/*************************************************************************************
 * @brief: parse the expression tail.
 * 
 * @param input: Pointer to the input string.
 * @param pos: Pointer to an integer representing the current position,
 *             used for reading and updating the position.
 * @param left: The left value.
 * @return: value.
 ************************************************************************************/
static int parse_expr_tail(char *input, int *pos, int left)
{
	int ret;
	int term_val;

	switch (input[*pos]) {
	case '&':
		ret = match(input, pos, '&');
		if (ret < 0)
			return ret;

		term_val = parse_term(input, pos);
		if (term_val < 0)
			return term_val;

		left = left & term_val;
		ret = parse_expr_tail(input, pos, left);
		break;

	case '|':
		ret = match(input, pos, '|');
		if (ret < 0)
			return ret;

		term_val = parse_term(input, pos);
		if (term_val < 0)
			return term_val;

		left = left | term_val;
		ret = parse_expr_tail(input, pos, left);
		break;

	default:
		ret = left;
		break;
	}

	return ret;
}

/*************************************************************************************
 * @brief: parse the term.
 * 
 * @param input: Pointer to the input string.
 * @param pos: Pointer to an integer representing the current position,
 *             used for reading and updating the position.
 * @return: value.
 ************************************************************************************/
static int parse_term(char *input, int *pos)
{
	int ret;
	int expr;

	switch (input[*pos]) {
	case '(':
		ret = match(input, pos, '(');
		if (ret < 0)
			return ret;

		expr = parse_expr(input, pos);
		ret = match(input, pos, ')');
		if (ret < 0)
			return ret;

		break;

	case '!':
		ret = match(input, pos, '!');
		if (ret < 0)
			return ret;

		ret = parse_term(input, pos);
		if (ret < 0)
			return ret;

		expr = (ret == 0) ? 1 : 0;
		break;

	case 'y':
		ret = match(input, pos, 'y');
		if (ret < 0)
			return ret;

		expr = 1;
		break;

	case 'n':
		ret = match(input, pos, 'n');
		if (ret < 0)
			return ret;

		expr = 0;
		break;

	default:
		printf("Error: unexpected token '%c'\n", input[*pos]);
		expr = -((*pos) + 1);
		break;
	}

	return expr;
}

/*************************************************************************************
 * @brief: parse the logic expression.
 * 
 * @param input: Pointer to the input string.
 * @return: >= 0: value.
 *          <  0: error equal to -(error position + 1);
 ************************************************************************************/
int lc_parse_logic_express(char *input)
{
	int pos = 0;
	return parse_expr(input, &pos);
}
