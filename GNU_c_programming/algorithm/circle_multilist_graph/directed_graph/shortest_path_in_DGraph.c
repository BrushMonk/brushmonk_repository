#pragma once
#include <inttypes.h>
#include "DGraph.c"

struct binomial_node
{   struct tree_node *node;
    size_t degree;
    struct binomial_node *left_child;
    struct binomial_node *parent;
    struct binomial_node *sibling;};

struct binomial_heap
{   struct binomial_node *head;
    /* priority queue for positioning to decrease key */
    struct binomial_node *prior_queue[NODE_NUM];};

static struct binomial_node *link_binomial_trees(struct binomial_node *front
, struct binomial_node *rear, struct binomial_node *first)
{
    if (front->node->dist <= rear->node->dist)
    {
        front->sibling = rear->sibling;
        rear->parent = front;
        rear->sibling = front->left_child;
        front->left_child = rear;
        front->degree++;
    }
    else
    {
        if (front == first) rear = first;
        front->parent = rear;
        front->sibling = rear->left_child;
        rear->left_child = front;
        rear->degree++;
        front = rear;
    }
    return front;
}

static struct tree_node *extract_min_binomial_node(struct binomial_heap *heap)
{
    struct binomial_node *min = heap->head, *prev_of_min = NULL;
    struct binomial_node *i;
    *i = (struct binomial_node){0};
    for (i->sibling = heap->head; i->sibling != NULL; i = i->sibling)
        if (i->sibling->node->dist < min->node->dist)
        {
            min = i->sibling;
            prev_of_min = i;
        }
    /* delete minimum-key node */
    if (prev_of_min == NULL)
        heap->head = min->sibling;
    else prev_of_min->sibling = min->sibling;
    /* sort binomial trees in the degree of root */
    struct binomial_node *cur1 = heap->head;
    struct binomial_node *cur2 = min->left_child;
    for (i = min->left_child; i != NULL; i = i->sibling)
        i->parent = NULL;
    /* sort binomial trees in the degree of root */
    heap->head = (cur1->degree < cur2->degree) ? cur1 : cur2;
    *i = (struct binomial_node){0};
    while (cur1 != NULL && cur2 != NULL)
    {
        if (cur1->degree < cur2->degree)
        {
            i->sibling = cur1;
            cur1 = cur1->sibling;
        }
        else
        {
            i->sibling = cur2;
            cur2 = cur2->sibling;
        }
        i = i->sibling;
    }
    while (cur1 != NULL)
    {
        i->sibling = cur1;
        cur1 = cur1->sibling;
        i = i->sibling;
    }
    while (cur2 != NULL)
    {
        i->sibling = cur2;
        cur2 = cur2->sibling;
        i = i->sibling;
    }
    /* merge binomial trees from head of binomial_heap */
    for (i = heap->head; i->sibling != NULL;)
    {
        struct binomial_node *next_of_i = i->sibling;
        if ((i->degree != next_of_i->degree) ||
        ((next_of_i->sibling != NULL) && (next_of_i->degree == next_of_i->sibling->degree)))
            i = i->sibling;
        else i = link_binomial_trees(i, next_of_i, heap->head);
    }
    heap->prior_queue[min->node->node_id] = NULL;
    return min->node;
}

/* worst complexity of inserting a node in binomial heap is O(log n) */
static void insert_a_node_in_binomial_heap(struct binomial_heap *heap, struct tree_node *const cand)
{
    struct binomial_node *new_node = (struct binomial_node *)malloc(sizeof(struct binomial_node));
    memset(new_node, 0, sizeof(struct binomial_node));
    new_node->node = cand;
    /* set new_node as the first binomial tree in binomial_heap */
    new_node->sibling = heap->head;
    heap->head = new_node;
    /* add new node in first of heap and unite binomial trees one by one */
    struct binomial_node *cur, *next_of_cur;
    for (cur = new_node; cur->sibling != NULL;)
    /* worst complexity of inserting a node in binomial heap is O(log n) */
    {
        next_of_cur = cur->sibling;
        if (next_of_cur->sibling != NULL &&
        next_of_cur->degree == next_of_cur->sibling->degree) cur = cur->sibling;
        else if (cur->degree != next_of_cur->degree) break;
        else cur = link_binomial_trees(cur, next_of_cur, heap->head);
    }
    heap->prior_queue[cand->node_id] = new_node;
    return;
}

#define NO_NODEID -1
#define WRONG_DIST -2
#define DECR_SUCCESS 0
static int decrease_binomial_key(struct binomial_heap *heap, int id, int64_t new_dist)
{
    struct tree_node *decr_node = heap->prior_queue[id]->node;
    if (decr_node == NULL)
    {
        fprintf(stderr, "No decreased_id %d in binomial_heap.\n", id);
        return NO_NODEID;
    }
    if (decr_node->dist <= new_dist)
    {
        fprintf(stderr, "decreased_dist %" PRId64" <=  new_dist %" PRId64".\n", decr_node->dist, new_dist);
        return WRONG_DIST;
    }
    decr_node->dist = new_dist;
    for (struct binomial_node *cur = heap->prior_queue[id];
    cur->parent != NULL && cur->node->dist < cur->parent->node->dist;
    cur = cur->parent)
    {
        int64_t tmp_dist = cur->node->dist;
        cur->node->dist = cur->parent->node->dist;
        cur->parent->node->dist = tmp_dist;
        int tmp_id = cur->node->node_id;
        cur->node->node_id = cur->parent->node->node_id;
        cur->parent->node->node_id = tmp_id;
        tmp_id = cur->node->parent_id;
        cur->node->parent_id = cur->parent->node->parent_id;
        cur->parent->node->parent_id = tmp_id;
        heap->prior_queue[cur->node->node_id] = cur;
        heap->prior_queue[cur->parent->node->node_id] = cur->parent;
    }
    return DECR_SUCCESS;
}

static void insert_adj_nodes_in_binomial_heap(const struct tree_node *node, const struct DGraph_info *DGraph, struct binomial_heap *heap, _Bool flag)
{
    for (struct adj_node *next_adj = DGraph->outadj[node->node_id]; next_adj != NULL; next_adj = next_adj->next)
    {
        /* candidate inserted into unvisited set */
        struct tree_node *cand;
        *cand = (struct tree_node){next_adj->node_id,
        next_adj->weight + node->dist * flag
        , 0, node->node_id, 0, 0};
        if (decrease_binomial_key(heap, cand->node_id, cand->dist) == NO_NODEID)
            insert_a_node_in_binomial_heap(heap, cand);
        else free(cand);
    }
    return;
}

static void delete_all_nodes_in_binomial_heap(struct binomial_node *node)
{
    if (node == NULL) return;
    delete_all_nodes_in_binomial_heap(node->left_child);
    delete_all_nodes_in_binomial_heap(node->sibling);
    free(node);
    return;
}

#define DIJKSTRA 1
struct tree_node *Dijkstra_algorithm_in_DGraph(const struct DGraph_info *DGraph, int src, int dest)
{
    /* the root of shortest path tree */
    struct tree_node *SPT_root;
    /* put src node into SPT as root */
    *SPT_root = (struct tree_node){src, 0, 0, -1, 0};
    /* loop current node id */
    struct tree_node *cur = SPT_root;
    /* find the minimum-dist node from binomial heap */
    struct binomial_heap unvisited = (struct binomial_heap){0};
    unvisited.head->node = cur;
    struct tree_node *visited[NODE_NUM];
    memset(visited, 0, NODE_NUM * sizeof(struct tree_node *));
    while (cur->node_id != dest || unvisited.head != NULL)
    {
        /* find the minimum-dist node from binomial heap */
        cur = extract_min_binomial_node(&unvisited);
        insert_adj_nodes_in_binomial_heap(cur, DGraph, &unvisited, DIJKSTRA);
        visited[cur->node_id] = cur;
        if (cur->parent_id != -1)
            insert_leaf_in_tree_node(visited[cur->parent_id], cur);
    }
    delete_all_nodes_in_binomial_heap(unvisited.head);
    memset(unvisited.prior_queue, 0, NODE_NUM * sizeof(struct tree_node *));
    free(&unvisited); free(visited);
    if (cur->node_id != dest)
    {
        delete_all_nodes_in_dirc_tree(SPT_root);
        return SPT_root = NULL;
    }
    /* copy tree_node to shortest_list */
    struct tree_node *path_node, *last;
    for (struct tree_node *i = cur; i != NULL; i = i->parent)
    {
        *path_node = (struct tree_node){i->node_id, i->dist, 0, i->parent_id, i->parent, 0};
        if (last != NULL)
            path_node->child_num = 1, path_node->next = (struct tree_node **)malloc(8UL), path_node->next[0] = last;
        last = path_node;
    }
    delete_all_nodes_in_dirc_tree(SPT_root);
    return path_node;
}

#define PRIM 0
struct tree_node *Prim_algorithm_in_DGraph(const struct DGraph_info *DGraph, int src)
{
    /* the root of minimum spanning tree */
    struct tree_node *MST_root;
    /* put src node into MST as root */
    *MST_root = (struct tree_node){src, 0, 0, -1, 0};
    /* loop current node id */
    struct tree_node *cur = MST_root;
    /* find the minimum-dist node from binomial heap */
    struct binomial_heap unvisited = (struct binomial_heap){0};
    unvisited.head->node = cur;
    struct tree_node *visited[NODE_NUM];
    memset(visited, 0, NODE_NUM * sizeof(struct tree_node *));
    while (unvisited.head != NULL)
    {
        /* find the minimum-dist node from binomial heap */
        cur = extract_min_binomial_node(&unvisited);
        insert_adj_nodes_in_binomial_heap(cur, DGraph, &unvisited, PRIM);
        visited[cur->node_id] = cur;
        if (cur->parent_id != -1)
            insert_leaf_in_tree_node(visited[cur->parent_id], cur);
    }
    free(&unvisited); free(visited);
    return MST_root;
}

struct tree_node *Bellman_Ford_algorithm_in_DGraph(const struct DGraph_info *DGraph, int src, int dest)
{
}

int64_t** Floyd_algorithm_in_DGraph(const struct DGraph_info *DGraph)
{
    static int64_t dist[NODE_NUM][NODE_NUM] = {-1};
    for (int v = 0; v < NODE_NUM; v++)
    {
        dist[v][v] = 0;
        for (struct adj_node *next_adj = DGraph->outadj[v];
        next_adj != NULL; next_adj = next_adj->next)
            dist[v][next_adj->node_id] = next_adj->weight;
    }
    for (int v = 0; v < NODE_NUM; v++)
        for (int i = 0; i < NODE_NUM; i++)
            for (int j = 0; j < NODE_NUM; j++)
                if (dist[i][v] != -1 && dist[v][j] != -1 &&
                (dist[i][j] == -1 || dist[i][j] > dist[i][v] + dist[v][j]))
                    dist[i][j] = dist[i][v] + dist[v][j];
    return (int64_t **)dist;
}