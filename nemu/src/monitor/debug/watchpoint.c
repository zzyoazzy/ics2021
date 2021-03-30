#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <stdlib.h>

#define NR_WP 32


static WP wp_pool[NR_WP];
static WP *head, *free_;

static int ID;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */


WP* new_wp() {
  if(free_==NULL)return NULL;
  WP *current = free_;
  free_ = free_->next;
  ID++;
  return current;
}

void free_wp(WP *wp) {
  if(wp == NULL)return;
  if(wp->expr!=NULL)
  {
    free(wp->expr);
    wp->expr = NULL;
  }
  wp->old_val = wp->new_val = 0;
  if(free_==NULL)
  {
	free_ = wp;
	wp->NO = ID;
	wp->next = NULL;
	return;
  }
  WP *current = free_;
  while(current->next!=NULL)
  {
	  current = current->next;
  }
  current->next = wp;
  wp->NO = current->NO+1;
  wp->next = NULL;
}
