// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "HashMap.h"


int32_t hash_new_node(hm_node_ptr *node, uint32_t dep) {
    //create new hash table
    if (!node) {
        return -1;
    }
    hm_node_ptr new_node = (hm_node*) calloc(1, sizeof(hm_node));
    if (!new_node) {
        return -1;
    }
    new_node->dep = dep;
    *node = new_node;
    return 0;
}

avlnode_ptr hash_search(hm_node_ptr node, str_t * key) {
    if (!node) {
        return NULL;
    }
    uint32_t mid_key = hash(key, node->dep);
    while (1) {
        if (node->len_of_list[mid_key] < MAX_HASH_DEP) {
            //avlnode_ptr mid = avl_search(node->top[mid_key], key);
            avlnode_ptr mid1 = avl_search(node->top[mid_key], key);
            return mid1;
        } else {
            node = node->top[mid_key];
            mid_key = hash(key, node->dep);
        }
    }
}

int32_t hash_insert(hm_node_ptr node, str_t * key, void *meta) {
    if (!node) {
        return -1;
    }
    uint32_t mid_key = hash(key, node->dep);
    while (node->len_of_list[mid_key] >= MAX_HASH_DEP) {
        node = node->top[mid_key];
        mid_key = hash(key, node->dep);
    }

    avlnode_ptr new_avl_node;
    if (-1 == avl_new_node(&new_avl_node, key, meta)) {
        return -1;
    }
    if (!node->top[mid_key]) {
        node->top[mid_key] = new_avl_node;
        node->len_of_list[mid_key]++;
    } else {
        int32_t ans = avl_insert_node((avlnode_ptr *) &node->top[mid_key], new_avl_node);
        node->len_of_list[mid_key]++;
        if (ans == -1) {
            return -1;
        }
    }
    __hash_remake(node, mid_key);
    return 1;
}

int32_t hash_delete(hm_node_ptr node, str_t * key) {
    if (!node) {
        return 0;
    }
    uint32_t mid_key = hash(key, node->dep);
    while (1) {
        if (node->len_of_list[mid_key] < MAX_HASH_DEP) {
            avlnode_ptr mid_node = node->top[mid_key];
            int32_t stat = avl_remove_node(&mid_node, key);
            node->top[mid_key] = mid_node;
            if (stat == 1) {
                node->len_of_list[mid_key]--;
            }
            break;
        } else {
            node = node->top[mid_key];
            mid_key = hash(key, node->dep);
        }
    }
    return 1;
}

int32_t hash_erase(hm_node_ptr node) {
    if (!node) {
        return 0;
    }
    for (uint32_t i = 0; i < MAX_HASH_NODE; i++) {
        if (node->len_of_list[i] > MAX_HASH_DEP) {
            hash_erase(node->top[i]);
        } else {
            if (node->len_of_list[i] > 0) {
                avl_erase(node->top[i]);
            }
        }
    }
    if (node) {
        free(node);
    }
    return 1;
}

int32_t __hash_remake(hm_node_ptr node, uint32_t index) {
    //if == then it is flag that make new hash, if > that it is hash
    if (node->len_of_list[index] != MAX_HASH_DEP) {
        return 0;
    }
    hm_node_ptr new_node;
    hash_new_node(&new_node, node->dep+1);
    avlnode_ptr go = node->top[index];
    __hash_insert_avl_in_hash(new_node, go);
    node->top[index] = new_node;
    node->len_of_list[index]++;
    return 1;
}

int32_t __hash_insert_avl_in_hash(hm_node_ptr node, avlnode_ptr go) {
    if (!go) {
        return 0;
    }
    if (go->left) {
        __hash_insert_avl_in_hash(node, go->left);
    }
    if (go->right) {
        __hash_insert_avl_in_hash(node, go->right);
    }
    go->left = go->right = go->parent = NULL;
    go->hight = 1;
    int32_t mid_key = hash(go->key, node->dep);
    node->len_of_list[mid_key]++;
    if (!node->top[mid_key]) {
        node->top[mid_key] = go;
    } else {
        avl_insert_node((avlnode_ptr *) &node->top[mid_key], go);
    }

    return 1;
}
