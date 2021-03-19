#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_REGISTER,			//register
  TK_HEX,				//hex
  TK_DEC,				//dec
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-", '-'},			// minus/negative
  {"\\*", '*'},			// multiplication/dereference
  {"/", '/'},			// division
  {"\\(", '('},			// left bracket
  {"\\)", ')'},			// right bracket
  {"\\$[a-z]+", TK_REGISTER}, // register
  {"0[xX][0-9a-fA-F]+", TK_HEX},//hex
  {"[0-9]+", TK_DEC},	//dec
  {"==", TK_EQ},        // equal

};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;			//init nr_token

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
		  case TK_NOTYPE:	
			break;
		  default:
			tokens[nr_token].type = rules[i].token_type;
			if(substr_len>32)panic("expression out of buffer!");
			memcpy(tokens[nr_token].str,e+position, sizeof(char)*(substr_len+1) );
			nr_token++;
		}
		position += substr_len;
		break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}




bool check_parentheses(int p, int q) {

	int num = 0;
	if(tokens[p].type!='{'||tokens[p].type!=')')return false;
	for(int i = p; i <= q; ++i) {
		if(tokens[i].type == '(')num++;
		else if(tokens[i].type == ')')num--;
		
		if(num<0)return false;
	}
	
	return num == 0;
}

uint32_t token_value(int index) {
	int token_type = tokens[index].type;
	int ans = 0;
	char *tk_p = tokens[index].str;
	switch(token_type) {
		case TK_DEC:
			sscanf(tk_p, "%d", &ans);
			break;
		case TK_HEX:
			sscanf(tk_p+2, "%x", &ans);
			break;
		case TK_REGISTER:
			for(int i = 0; i<8; ++i) {
				if(strcmp(tk_p+1, regsl[i])==0) {
					ans = cpu.gpr[i]._32;
					break;
				}
				else if(strcmp(tk_p+1, regsw[i])==0) {
					ans = cpu.gpr[i]._16;
					break;
				}
				else if(strcmp(tk_p+1, regsb[i])==0) {
					if(i<4)
						ans = cpu.gpr[i]._8[0];
					else ans = cpu.gpr[i-4]._8[1];
					break;
				}
				else if(strcmp(tk_p+1, "eip")==0) {
					ans = cpu.eip;
					break;
				}
			}	
			break;
		default:
			assert(0);
	}
	return ans;
}

int sign_priority[5][5] = \
{
//  _ , + ,- ,*, /
	{0, 0, 0, 0, 0,},
	{1, 1, 1, 1, 1,},
	{1, 1, 1, 1, 1,},
	{1, 0, 0, 1, 1,},
	{1, 0, 0, 1, 1,},
};


int get_index(int token_type) {
	switch (token_type) { 
		case '+':
			return 1;
		case '-':
			return 2;
		case '*':
			return 3;
		case '/':
			return 4;
		default:
			return 0;
	} 
}

int check_priority(int p, int q) {
	return sign_priority[get_index(tokens[p].type)][get_index(tokens[q].type)];
}

int find_dominant_op(int p, int q) {	
	int op = p;
	int bracket_count = 0;
	for(int i = p; i <= q; i++) {
		int token_type = tokens[i].type;
		if(token_type == '(')
		{
			bracket_count++;
			continue;
		}
		else if(token_type == ')')
		{
			bracket_count--;
			continue;
		}
		if(bracket_count == 0 && check_priority(op, i)==0 )op = i;
	}
	return op;
}

uint32_t eval(int p, int q) {
	if(p > q) {
		panic("bad expression\n");
	}
	else if(p == q) {
		return token_value(p);
	}
	else if(check_parentheses(p,q) == true) {
		return eval(p+1,q-1);
	}
	else {
		int op = find_dominant_op(p, q);
		printf("op=%d\n",op);
		uint32_t val1 = eval(p, op-1), val2 = eval(op+1, q);
		switch (tokens[op].type)
		{
			case '+':
				return val1+val2;
			case '-':
				return val1-val2;
			case '*':
				return val1*val2;
			case '/':
				return val1/val2;
			default:
				assert(0);
		}
		
		return 0;	
	}
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  *success = true;
  return eval(0,nr_token-1);
}
