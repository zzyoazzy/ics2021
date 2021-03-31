#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <stdlib.h>

#define NR_WP 32


static WP wp_pool[NR_WP];
static WP *head, *free_;

static int ID;
WP* p_current;

void init_wp_pool() {
  int i;
  for (i = 0;  i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  p_current = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */


WP* new_wp() {
  if(free_==NULL)return NULL;
  WP *current = free_;
  free_ = free_->next;
  current->next = NULL;
  ID++;
  WP *tail = head;
  if(tail==NULL)
  {
	head = current;
  }
  else 
  {
	while(tail->next!=NULL)tail = tail->next;
	tail->next = current;
  }
  return current;
}

void free_wp(WP *wp) {
  if(wp == NULL)return;
  WP *prev = head;
  if(head == wp)
  {
	head = wp->next;
  }
  else
  {
    while(prev->next!=wp)prev = prev->next;
	assert(prev!=NULL);
	prev->next = wp->next;
  }
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
int set_watchpoint(char *e) {
  bool success;
  uint32_t ans = expr(e, &success);
  if(!success)return -1;
  WP *wp = new_wp();
  if(wp == NULL)return -2;
  wp->expr = (char*)malloc(sizeof(char)*(strlen(e)+1));
  strcpy(wp->expr,e);
  wp->old_val = ans;
  return wp->NO;
}

bool delete_watchpoint(int NO) {
  WP *current = head;
  while(current && current->NO!=NO)current = current->next;
  if(!current)return false;
  else free_wp(current);
  return true;
}

void delete_all_watchpoints() {
  WP *current = head, *prev = head;
  while(current){
    current = current->next;
	free_wp(prev);
	prev = current;
  }
}

void list_watchpoint() {
  WP *current = head;
  if(head == NULL)printf("No watchpoints\n");
  else printf("NO  Expr		 Old Value\n");
  while(current)
  {
	printf("%d   %s		 0x%x\n",current->NO,current->expr,current->old_val);
	current = current->next;
  }
}

WP* scan_watchpoint() {
  if(p_current == NULL)p_current = head;
  while(p_current)
  {
	bool success;
	uint32_t ans = expr(p_current->expr,&success);
    if(ans!=p_current->old_val)
	{
	  p_current->new_val = ans;
	  return p_current;
	}
	p_current = p_current->next;
  }
  return NULL;
}

