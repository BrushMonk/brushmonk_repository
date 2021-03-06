#pragma once
#include "shortest_path_in_UDGraph.c"

/* get a copy of undirected graph */
static struct UDGraph_info *get_UDGraph_copy(const struct UDGraph_info *UDGraph)
{
    struct adj_line **lines_set = get_lines_set_in_ascd_order(UDGraph);
    struct undirc_line lines[UDGraph->line_num];
    for (size_t e = 0; e < UDGraph->line_num; e++)
        lines[e].i_node = lines_set[e]->i_node,
        lines[e].j_node = lines_set[e]->j_node,
        lines[e].weight = lines_set[e]->weight;
    free(lines_set);
    struct UDGraph_info *UDGraph_copy = (struct UDGraph_info *)malloc(sizeof(struct UDGraph_info));
    init_UDGraph(UDGraph_copy, lines, UDGraph->line_num);
    return UDGraph_copy;
}

static struct adj_line* find_next_line_in_undirc_Euler_path(struct UDGraph_info *UDGraph, int node_id)
{
    struct adj_line *adj_line = UDGraph->adj[node_id];
    while (adj_line != NULL)
    {
        /* if the adjacenct line is not a bridge, jump out of this loop. */
        if ( is_a_bridge_in_UDGraph(UDGraph, adj_line->i_node, adj_line->j_node) == 0 )
            break;
        adj_line = (adj_line->i_node == node_id) ? adj_line->i_next : adj_line->j_next;
    }
    return adj_line;
}

struct tree_node *Fleury_algorithm_in_UDGraph(const struct UDGraph_info *UDGraph, int src)
{
    struct UDGraph_info *unvis_UDGraph = get_UDGraph_copy(UDGraph);
    struct tree_node *start_node;
    *start_node = (struct tree_node){src, 0, 0, -1, 0};
    struct tree_node *last = NULL, *path_node;
    struct adj_line *cur_line;
    int cur_id = src;
    int64_t dist = 0;
    while (cur_id != -1)
    {
        if (last != NULL)
        {
            *path_node = (struct tree_node){cur_id, dist, 0, -1, 0};
            insert_leaf_in_tree_node(last, path_node);
        }
        else path_node = start_node;
        last = path_node;
        cur_line = find_next_line_in_undirc_Euler_path(unvis_UDGraph, cur_id);
        if (cur_line == NULL) cur_id = -1;
        else
        {
            dist += cur_line->weight;
            cur_id = cur_line->i_node == cur_id ? cur_line->j_node : cur_line->i_node;
            delete_a_line_in_UDGraph(unvis_UDGraph, (struct undirc_line){cur_line->i_node, cur_line->j_node, cur_line->weight});
        }
    }
    free(unvis_UDGraph);
    return start_node;
}

static _Atomic(int) stack_top = -1;
/* traverse loop in UDGraph and delete the lines where traversal passed by */
static void trav_loop_until_end_and_push_to_stack_in_UDGraph(struct UDGraph_info *UDGraph, int *idstack, int64_t *weightstack, int node_id)
{
    int64_t tmp_weight;
    while (UDGraph->adj[node_id] != NULL)
    {
        int adj_id = (UDGraph->adj[node_id]->i_node != node_id) ? UDGraph->adj[node_id]->i_node : UDGraph->adj[node_id]->j_node;
        tmp_weight = UDGraph->adj[node_id]->weight;
        delete_a_line_in_UDGraph(UDGraph, (struct undirc_line){UDGraph->adj[node_id]->i_node, UDGraph->adj[node_id]->j_node, UDGraph->adj[node_id]->weight});
        trav_loop_until_end_and_push_to_stack_in_UDGraph(UDGraph, idstack, weightstack, adj_id);
    }
    /* push the lineless node into the stack */
    if (stack_top == NODE_NUM << 8)
    {
        perror("node_stack overflow:");
        delete_all_lines_in_UDGraph(UDGraph);
        exit(-1);
    }
    else
        stack_top++, idstack[stack_top] = node_id, weightstack[stack_top] = tmp_weight;
    return;
}

struct tree_node* Hierholzer_algorithm_in_UDGraph(const struct UDGraph_info *UDGraph, int src)
{
    struct UDGraph_info *unvis_UDGraph = get_UDGraph_copy(UDGraph);
    int idstack[NODE_NUM << 8]; int64_t weightstack[NODE_NUM << 8]; stack_top = -1;
    trav_loop_until_end_and_push_to_stack_in_UDGraph(unvis_UDGraph, idstack, weightstack, src);
    struct tree_node *last = NULL, *path_node;
    while (stack_top != -1)
    {
        if (last != NULL)
        {
            *path_node = (struct tree_node){idstack[stack_top], weightstack[stack_top], 0, -1, 0};
            insert_leaf_in_tree_node(path_node, last);
        }
        last = path_node, stack_top--;
    }
    free(unvis_UDGraph);
    path_node->dist = 0;
    int64_t tmp_dist = 0;
    for (struct tree_node *i = path_node; i != NULL; i = i->next[0])
        tmp_dist += i->dist, i->dist = tmp_dist;
    return path_node;
}

static int volatile *odd_deg_node;
static _Atomic(size_t) odd_deg_num = 0;
struct tree_node *Chinese_postman_problem(const struct UDGraph_info *UDGraph, int src)
{
    for (size_t v = 0; v < NODE_NUM; v++)
        if (UDGraph->degree[v] >> 1 == 1)
        {
            odd_deg_node = (int *)realloc(odd_deg_node, ++odd_deg_num * sizeof(int));
            odd_deg_node[odd_deg_num - 1] = v;
        }
    if (!UDGraph->degree[src])
    {
        fprintf(stderr, "There is no postman path from node %d.\n", src);
        return NULL;
    }
    else if ( odd_deg_num == 0 || odd_deg_num == 2 && UDGraph->degree[src] >> 1 == 1 )
        return Hierholzer_algorithm_in_UDGraph(UDGraph, src);
    else
    {
        int64_t **dist = Floyd_algorithm_in_UDGraph(UDGraph);
        struct undirc_line lines[odd_deg_num*(odd_deg_num-1)/2];
        /* complete graph made up of odd nodes in original undirected graph */
        struct UDGraph_info *odd_nodes_CGraph = (struct UDGraph_info *)malloc(sizeof(struct UDGraph_info));
        init_UDGraph(odd_nodes_CGraph, lines, odd_deg_num*(odd_deg_num-1)/2);
    }
}