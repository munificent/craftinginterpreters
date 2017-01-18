#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct sNode {
  struct sNode* prev;
  struct sNode* next;
  char* string;
} Node;

// Insert a new node containing [string] after [prev], or at the beginning of
// the list if [prev] is NULL.
void insert(Node** list, Node* prev, const char* string) {
  // Create the new node and copy the string to the heap.
  Node* node = malloc(sizeof(Node));
  node->string = malloc(strlen(string) + 1);
  strcpy(node->string, string);

  if (prev == NULL) {
    if (*list != NULL) (*list)->prev = node;
    node->prev = NULL;
    node->next = *list;
    *list = node;
  } else {
    node->next = prev->next;
    if (node->next != NULL) node->next->prev = node;
    prev->next = node;
    node->prev = prev;
  }
}

Node* find(Node* list, const char* string) {
  while (list != NULL) {
    if (strcmp(string, list->string) == 0) {
      return list;
    }
    
    list = list->next;
  }
  
  // Not found.
  return NULL;
}

void delete(Node** list, Node* node) {
  // Unlink it.
  if (node->prev != NULL) node->prev->next = node->next;
  if (node->next != NULL) node->next->prev = node->prev;
  
  // If we're deleting the head, update it.
  if (*list == node) *list = node->next;
  
  free(node->string);
  free(node);
}

void dump(Node* list) {
  while (list != NULL) {
    printf("%p [prev %p next %p] %s\n",
           list, list->prev, list->next, list->string);
    list = list->next;
  }
}

int main(int argc, const char* argv[]) {
  printf("Hello, World!\n");
  
  Node* list = NULL;
  insert(&list, NULL, "four");
  insert(&list, NULL, "one");
  insert(&list, find(list, "one"), "two");
  insert(&list, find(list, "two"), "three");
  
  dump(list);
  printf("-- delete three --\n");
  delete(&list, find(list, "three"));
  dump(list);

  printf("-- delete one --\n");
  delete(&list, find(list, "one"));
  dump(list);

  return 0;
}
