//
// Created by 31838 on 7/7/2019.
//

#include "map.h"

#include <string.h>
#include <errno.h>
#include <common/util/pointer.h>
#include "map.h"

struct internal_map {
  size_t key_size;
  size_t value_size;

  int (*comparator)(const void *const one, const void *const two);

  int size;
  struct node *root;
};

struct node {
  struct node *parent;
  int balance;
  void *key;
  void *value;
  struct node *left;
  struct node *right;
};

map map_init(const size_t key_size,
             const size_t value_size,
             int (*const comparator)(const void *const, const void *const)) {
  struct internal_map *init;
  if (key_size == 0 || value_size == 0 || !comparator) {
    return NULL;
  }
  init = malloc(sizeof(struct internal_map));
  if (!init) {
    return NULL;
  }
  init->key_size = key_size;
  init->value_size = value_size;
  init->comparator = comparator;
  init->size = 0;
  init->root = NULL;
  return init;
}

int map_size(map me) {
  avoid_null_pointer(me);
  return me->size;
}

int map_is_empty(map me) {
  avoid_null_pointer(me);
  return map_size(me) == 0;
}

static void map_reference_parent(map me,
                                 struct node *const parent,
                                 struct node *const child) {
  avoid_null_pointer(me);
  child->parent = parent->parent;
  if (!parent->parent) {
    me->root = child;
  } else if (parent->parent->left == parent) {
    parent->parent->left = child;
  } else {
    parent->parent->right = child;
  }
}

static void map_rotate_left(map me,
                            struct node *const parent,
                            struct node *const child) {
  avoid_null_pointer(me);
  struct node *grand_child;
  map_reference_parent(me, parent, child);
  grand_child = child->left;
  if (grand_child) {
    grand_child->parent = parent;
  }
  parent->parent = child;
  parent->right = grand_child;
  child->left = parent;
}

static void map_rotate_right(map me,
                             struct node *const parent,
                             struct node *const child) {
  avoid_null_pointer(me);
  struct node *grand_child;
  map_reference_parent(me, parent, child);
  grand_child = child->right;
  if (grand_child) {
    grand_child->parent = parent;
  }
  parent->parent = child;
  parent->left = grand_child;
  child->right = parent;
}

static struct node *map_repair_left(map me,
                                    struct node *const parent,
                                    struct node *const child) {
  avoid_null_pointer(me);
  map_rotate_left(me, parent, child);
  if (child->balance == 0) {
    parent->balance = 1;
    child->balance = -1;
  } else {
    parent->balance = 0;
    child->balance = 0;
  }
  return child;
}

static struct node *map_repair_right(map me,
                                     struct node *const parent,
                                     struct node *const child) {
  avoid_null_pointer(me);
  map_rotate_right(me, parent, child);
  if (child->balance == 0) {
    parent->balance = -1;
    child->balance = 1;
  } else {
    parent->balance = 0;
    child->balance = 0;
  }
  return child;
}

static struct node *map_repair_left_right(map me,
                                          struct node *const parent,
                                          struct node *const child,
                                          struct node *const grand_child) {
  avoid_null_pointer(me);
  map_rotate_left(me, child, grand_child);
  map_rotate_right(me, parent, grand_child);
  if (grand_child->balance == 1) {
    parent->balance = 0;
    child->balance = -1;
  } else if (grand_child->balance == 0) {
    parent->balance = 0;
    child->balance = 0;
  } else {
    parent->balance = 1;
    child->balance = 0;
  }
  grand_child->balance = 0;
  return grand_child;
}

static struct node *map_repair_right_left(map me,
                                          struct node *const parent,
                                          struct node *const child,
                                          struct node *const grand_child) {
  avoid_null_pointer(me);
  map_rotate_right(me, child, grand_child);
  map_rotate_left(me, parent, grand_child);
  if (grand_child->balance == 1) {
    parent->balance = -1;
    child->balance = 0;
  } else if (grand_child->balance == 0) {
    parent->balance = 0;
    child->balance = 0;
  } else {
    parent->balance = 0;
    child->balance = 1;
  }
  grand_child->balance = 0;
  return grand_child;
}

static struct node *map_repair(map me,
                               struct node *const parent,
                               struct node *const child,
                               struct node *const grand_child) {
  avoid_null_pointer(me);
  if (parent->balance == 2) {
    if (child->balance == -1) {
      return map_repair_right_left(me, parent, child, grand_child);
    }
    return map_repair_left(me, parent, child);
  }
  if (child->balance == 1) {
    return map_repair_left_right(me, parent, child, grand_child);
  }
  return map_repair_right(me, parent, child);
}

static void map_insert_balance(map me, struct node *const item) {
  avoid_null_pointer(me);
  struct node *grand_child = NULL;
  struct node *child = item;
  struct node *parent = item->parent;
  while (parent) {
    if (parent->left == child) {
      parent->balance--;
    } else {
      parent->balance++;
    }
    /* If balance is zero after modification, then the tree is balanced. */
    if (parent->balance == 0) {
      return;
    }
    /* Must re-balance if not in {-1, 0, 1} */
    if (parent->balance > 1 || parent->balance < -1) {
      /* After one repair, the tree is balanced. */
      map_repair(me, parent, child, grand_child);
      return;
    }
    grand_child = child;
    child = parent;
    parent = parent->parent;
  }
}

static struct node *map_create_node(map me,
                                    const void *const key,
                                    const void *const value,
                                    struct node *const parent) {
  avoid_null_pointer(me);
  struct node *const insert = malloc(sizeof(struct node));
  if (!insert) {
    return NULL;
  }
  insert->parent = parent;
  insert->balance = 0;
  insert->key = malloc(me->key_size);
  if (!insert->key) {
    free(insert);
    return NULL;
  }
  memcpy(insert->key, key, me->key_size);
  insert->value = malloc(me->value_size);
  if (!insert->value) {
    free(insert->key);
    free(insert);
    return NULL;
  }
  memcpy(insert->value, value, me->value_size);
  insert->left = NULL;
  insert->right = NULL;
  me->size++;
  return insert;
}

int map_put(map me, void *const key, void *const value) {
  avoid_null_pointer(me);
  struct node *traverse;
  if (!me->root) {
    struct node *insert = map_create_node(me, key, value, NULL);
    if (!insert) {
      return -ENOMEM;
    }
    me->root = insert;
    return 0;
  }
  traverse = me->root;
  for (;;) {
    const int compare = me->comparator(key, traverse->key);
    if (compare < 0) {
      if (traverse->left) {
        traverse = traverse->left;
      } else {
        struct node *insert = map_create_node(me, key, value, traverse);
        if (!insert) {
          return -ENOMEM;
        }
        traverse->left = insert;
        map_insert_balance(me, insert);
        return 0;
      }
    } else if (compare > 0) {
      if (traverse->right) {
        traverse = traverse->right;
      } else {
        struct node *insert = map_create_node(me, key, value, traverse);
        if (!insert) {
          return -ENOMEM;
        }
        traverse->right = insert;
        map_insert_balance(me, insert);
        return 0;
      }
    } else {
      memcpy(traverse->value, value, me->value_size);
      return 0;
    }
  }
}

static struct node *map_equal_match(map me, const void *const key) {
  avoid_null_pointer(me);
  struct node *traverse = me->root;
  if (!traverse) {
    return NULL;
  }
  for (;;) {
    const int compare = me->comparator(key, traverse->key);
    if (compare < 0) {
      if (traverse->left) {
        traverse = traverse->left;
      } else {
        return NULL;
      }
    } else if (compare > 0) {
      if (traverse->right) {
        traverse = traverse->right;
      } else {
        return NULL;
      }
    } else {
      return traverse;
    }
  }
}

int map_get(void *const value, map me, void *const key) {
  avoid_null_pointer(me);
  struct node *const traverse = map_equal_match(me, key);
  if (!traverse) {
    return 0;
  }
  memcpy(value, traverse->value, me->value_size);
  return 1;
}

int map_contains(map me, void *const key) {
  return map_equal_match(me, key) != NULL;
}

static struct node *map_repair_pivot(map me,
                                     struct node *const item,
                                     const int is_left_pivot) {
  struct node *const child = is_left_pivot ? item->right : item->left;
  struct node *const grand_child =
    child->balance == 1 ? child->right : child->left;
  return map_repair(me, item, child, grand_child);
}

static void map_trace_ancestors(map me, struct node *item) {
  avoid_null_pointer(me);
  struct node *child = item;
  struct node *parent = item->parent;
  while (parent) {
    if (parent->left == child) {
      parent->balance++;
    } else {
      parent->balance--;
    }
    /* The tree is balanced if balance is -1 or +1 after modification. */
    if (parent->balance == -1 || parent->balance == 1) {
      return;
    }
    /* Must re-balance if not in {-1, 0, 1} */
    if (parent->balance > 1 || parent->balance < -1) {
      child = map_repair_pivot(me, parent, parent->left == child);
      parent = child->parent;
      /* If balance is -1 or +1 after modification or the parent is */
      /* NULL, then the tree is balanced. */
      if (!parent || child->balance == -1 || child->balance == 1) {
        return;
      }
    } else {
      child = parent;
      parent = parent->parent;
    }
  }
}

static void map_delete_balance(map me,
                               struct node *item,
                               const int is_left_deleted) {
  avoid_null_pointer(me);
  if (is_left_deleted) {
    item->balance++;
  } else {
    item->balance--;
  }
  /* If balance is -1 or +1 after modification, then the tree is balanced. */
  if (item->balance == -1 || item->balance == 1) {
    return;
  }
  /* Must re-balance if not in {-1, 0, 1} */
  if (item->balance > 1 || item->balance < -1) {
    item = map_repair_pivot(me, item, is_left_deleted);
    if (!item->parent || item->balance == -1 || item->balance == 1) {
      return;
    }
  }
  map_trace_ancestors(me, item);
}

static void map_remove_no_children(map me, const struct node *const traverse) {
  avoid_null_pointer(me);
  struct node *const parent = traverse->parent;
  /* If no parent and no children, then the only node is traverse. */
  if (!parent) {
    me->root = NULL;
    return;
  }
  /* No re-reference needed since traverse has no children. */
  if (parent->left == traverse) {
    parent->left = NULL;
    map_delete_balance(me, parent, 1);
  } else {
    parent->right = NULL;
    map_delete_balance(me, parent, 0);
  }
}

static void map_remove_one_child(map me, const struct node *const traverse) {
  avoid_null_pointer(me);
  struct node *const parent = traverse->parent;
  /* If no parent, make the child of traverse the new root. */
  if (!parent) {
    if (traverse->left) {
      traverse->left->parent = NULL;
      me->root = traverse->left;
    } else {
      traverse->right->parent = NULL;
      me->root = traverse->right;
    }
    return;
  }
  /* The parent of traverse now references the child of traverse. */
  if (parent->left == traverse) {
    if (traverse->left) {
      parent->left = traverse->left;
      traverse->left->parent = parent;
    } else {
      parent->left = traverse->right;
      traverse->right->parent = parent;
    }
    map_delete_balance(me, parent, 1);
  } else {
    if (traverse->left) {
      parent->right = traverse->left;
      traverse->left->parent = parent;
    } else {
      parent->right = traverse->right;
      traverse->right->parent = parent;
    }
    map_delete_balance(me, parent, 0);
  }
}

static void map_remove_two_children(map me, const struct node *const traverse) {
  avoid_null_pointer(me);
  struct node *item;
  struct node *parent;
  const int is_left_deleted = traverse->right->left != NULL;
  if (!is_left_deleted) {
    item = traverse->right;
    parent = item;
    item->balance = traverse->balance;
    item->parent = traverse->parent;
    item->left = traverse->left;
    item->left->parent = item;
  } else {
    item = traverse->right->left;
    while (item->left) {
      item = item->left;
    }
    parent = item->parent;
    item->balance = traverse->balance;
    item->parent->left = item->right;
    if (item->right) {
      item->right->parent = item->parent;
    }
    item->left = traverse->left;
    item->left->parent = item;
    item->right = traverse->right;
    item->right->parent = item;
    item->parent = traverse->parent;
  }
  if (!traverse->parent) {
    me->root = item;
  } else if (traverse->parent->left == traverse) {
    item->parent->left = item;
  } else {
    item->parent->right = item;
  }
  map_delete_balance(me, parent, is_left_deleted);
}

static void map_remove_element(map me, struct node *const traverse) {
  avoid_null_pointer(me);
  if (!traverse->left && !traverse->right) {
    map_remove_no_children(me, traverse);
  } else if (!traverse->left || !traverse->right) {
    map_remove_one_child(me, traverse);
  } else {
    map_remove_two_children(me, traverse);
  }
  free(traverse->key);
  free(traverse->value);
  free(traverse);
  me->size--;
}

int map_remove(map me, void *const key) {
  avoid_null_pointer(me);
  struct node *const traverse = map_equal_match(me, key);
  if (!traverse) {
    return 0;
  }
  map_remove_element(me, traverse);
  return 1;
}

void map_clear(map me) {
  while (me->root) {
    map_remove_element(me, me->root);
  }
}

map map_destroy(map me) {
  avoid_null_pointer(me);
  map_clear(me);
  free(me);
  return NULL;
}
