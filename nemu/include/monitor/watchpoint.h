#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char* expr;
  uint32_t new_val;
  uint32_t old_val;
} WP;

WP* new_wp();
void free_wp(WP *wp);

#endif
