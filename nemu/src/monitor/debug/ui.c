#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */

  { "si", "Step one instruction exactly.", cmd_si},
  { "info", "Generic command for showing things about the program being debugged.", cmd_info },
  { "x", "Examine memory", cmd_x},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
	char *arg = strtok(NULL, " ");
	bool negative = false;
	uint64_t step = 0;

	if(arg != NULL) {
		if(*arg == '-') {
			negative = true;
			arg++;
		}
		else if(*arg =='+') {
			arg++;
		}
		while(*arg!='\0') {
			step *= 10;
			step += (*arg-'0');
			arg++;
		}
		if(negative) {
			step = (uint64_t)(step*(-1));
		}
	}
	else step = 1;
	cpu_exec(step);
	return 0;
}

static int cmd_info(char *args){
	char *arg = strtok(NULL, " ");

	if(arg == NULL) {
		printf("List of info subcommands:\n");
		printf("info r -- List of integr registers and their contents\n");	
	}
	else if(*arg == 'r') {
		for(int i = 0;i < 8;++i) {
			printf("%s:\t0x%08x\t%d\n",regsl[i],cpu.gpr[i]._32,cpu.gpr[i]._32);
		}
		printf("eip:\t0x%08x\t%d\n",cpu.eip,cpu.eip);
		/*for(int i = 0;i < 8;++i) {
			printf("%s:\t0x%08x\t%d\n",regsw[i],cpu.gpr[i]._16,cpu.gpr[i]._16);
		}
		for(int i = 0;i < 4;++i) {
			printf("%s:\t0x%08x\t%d\n",regsb[i],cpu.gpr[i]._8[0],cpu.gpr[i]._8[0]);	
		}
		for(int i = 0;i < 4;++i) {
			printf("%s:\t0x%08x\t%d\n",regsb[i+4],cpu.gpr[i]._8[1],cpu.gpr[i]._8[1]);
		}*/
	}
	else if(*arg == 'w') {

	}
	return 0;
}


static int cmd_x(char *args) {
	char *arg = strtok(NULL," ");
	int num = 0;
	uint32_t vaddr;
	bool negative = false;
	if(arg == NULL) {
		printf("Argument required (starding display address).\n");
		return 0;
	}	

	if(*arg == '-') {
		negative = true;
		arg++;
	}
	
	if(*arg!='0') {
		while(*arg!='\0') {
			num*=10;
			num+=*arg - '0';
			arg++;
		}

	}
	else
	{
		if(*(arg+1)!='x'&&*(arg+1)!='X') {
			return 0;
		}
		arg = strtok(NULL," ");
	}
	
	sscanf(arg,"%x",&vaddr);
	printf("%x\n???????????????",vaddr);
	for(int i=0;i<num;i++) {
		int addr;
		addr = negative?vaddr-i*4:vaddr+i*4;
		int value = vaddr_read(addr,4);	
		printf("0x%08x\t0x%08x\t",addr,value);
		for(int j = 0;j < 4;j++) {
			printf("%02x ",value%256);
			value/=256;
		}
		printf("\n");
	}
	return 0;
}
void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
