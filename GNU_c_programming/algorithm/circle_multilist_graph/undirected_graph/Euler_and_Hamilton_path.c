#pragma once
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "shortest_path_in_UDGraph.c"

static struct adj_multiline* find_next_line_in_undirc_Euler_path(struct UDGraph_info *UDGraph, int node_id)
{
    struct adj_multiline *adj_line = UDGraph->adj[node_id];
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
    struct adj_multiline **line_set = get_lines_set_in_ascd_order(UDGraph);
    struct undirc_line lines[UDGraph->line_num];
    for (size_t e = 0; e < UDGraph->line_num; e++)
    {
        lines[e].i_node = line_set[e]->i_node;
        lines[e].j_node = line_set[e]->j_node;
        lines[e].weight = line_set[e]->weight;
    }
    struct UDGraph_info *unvis_UDGraph = (struct UDGraph_info *)malloc(sizeof(struct UDGraph_info));
    init_UDGraph(unvis_UDGraph, lines, UDGraph->line_num);
    struct tree_node *start_node;
    *start_node = (struct tree_node){src, 0, 0, -1, 0};
    struct tree_node *last = NULL, *path_node;
    struct adj_multiline *cur_line;
    int cur_id = src;
    int64_t dist = 0;
    while (cur_id != -1)
    {
        if (last != NULL)
        {
            *path_node = (struct tree_node){cur_id, dist, 0, 0, 0};
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
            delete_a_line_in_UDGraph(UDGraph, (struct undirc_line){cur_line->i_node, cur_line->j_node, cur_line->weight});
        }
    }
    free(unvis_UDGraph);
    return start_node;
}

static _Atomic(int) top = -1;
static void push_nodeid_into_nodestack(struct UDGraph_info *UDGraph, int *nodestack, int64_t *weightstack, int node_id)
{
    while (UDGraph->adj[node_id] != NULL)
    {
        int adj_id = (UDGraph->adj[node_id]->i_node != node_id) ? UDGraph->adj[node_id]->i_node : UDGraph->adj[node_id]->j_node;
        int64_t tmp_weight = UDGraph->adj[node_id]->weight;
        delete_a_line_in_UDGraph(UDGraph, (struct undirc_line){UDGraph->adj[node_id]->i_node, UDGraph->adj[node_id]->j_node, UDGraph->adj[node_id]->weight});
        push_nodeid_into_nodestack(UDGraph, nodestack, weightstack, adj_id);
        if (UDGraph->adj[node_id] == NULL)
        {
            if (top == INT_MAX)
            {
                perror("node_stack overflow:");
                delete_all_lines_in_UDGraph(UDGraph);
                exit(-1);
            }
            else
            {
                top++;
                nodestack[top] = node_id;
                weightstack[top] = tmp_weight;
            }
            return;
        }
    }
    return;
}

struct tree_node* Hierholzer_algorithm_in_UDGraph(const struct UDGraph_info *UDGraph, int src)
{
    struct adj_multiline **line_set = get_lines_set_in_ascd_order(UDGraph);
    struct undirc_line lines[UDGraph->line_num];
    for (size_t e = 0; e < UDGraph->line_num; e++)
    {
        lines[e].i_node = line_set[e]->i_node;
        lines[e].j_node = line_set[e]->j_node;
        lines[e].weight = line_set[e]->weight;
    }
    struct UDGraph_info *unvis_UDGraph = (struct UDGraph_info *)malloc(sizeof(struct UDGraph_info));
    init_UDGraph(unvis_UDGraph, lines, UDGraph->line_num);
    int nodestack[INT_MAX]; int64_t weightstack[INT_MAX]; top = -1;
    push_nodeid_into_nodestack(unvis_UDGraph, nodestack, weightstack, src);
    struct tree_node *start_node;
    int64_t dist = 0;
    *start_node = (struct tree_node){nodestack[top], weightstack[top], 0, -1, 0};
    struct tree_node *last = NULL, *path_node;
    while (top == -1)
    {
        dist += weightstack[top];
        if (last != NULL)
        {
            *path_node = (struct tree_node){nodestack[top], dist, 0, 0, 0};
            insert_leaf_in_tree_node(last, path_node);
        }
        else path_node = start_node;
        last = path_node;
        top--;
    }
    free(unvis_UDGraph);
    return start_node;
}

static int *odd_deg_node;
static size_t odd_deg_num = 0;
struct tree_node *Chinese_postman_problem(const struct UDGraph_info *UDGraph, int src)
{
    for (size_t v = 0; v < NODE_NUM; v++)
        if (UDGraph->degree[v] >> 1 == 1)
        {
            odd_deg_node = (int *)realloc(odd_deg_node, ++odd_deg_num * sizeof(int));
            odd_deg_node[odd_deg_num - 1] = v;
        }
    if ( odd_deg_num == 0 || (odd_deg_num == 2 && UDGraph->degree[src] >> 1 == 1) )
        return Hierholzer_algorithm_in_UDGraph(UDGraph, src);
    else
    {
        struct tree_node *dist_path[odd_deg_num*(odd_deg_num-1)/2];
        struct undirc_line lines[odd_deg_num*(odd_deg_num-1)/2];
        for (size_t u = 0, w = 0; u < odd_deg_num; u++)
            for (size_t v = u + 1; v < odd_deg_num; v++)
            {
                dist_path[w] = Dijkstra_algorithm_in_UDGraph(UDGraph, odd_deg_node[u], odd_deg_node[v]);
                lines[w].i_node = odd_deg_node[u];
                lines[w].j_node = odd_deg_node[v];
                struct tree_node *head;
                for (head = dist_path[w]; head->next[0] != NULL; head = head->next[0]);
                ;
                lines[w].weight = head->dist;
                w++;
            }
        /* complete graph made up of odd nodes in original undirected graph */
        struct UDGraph_info *odd_nodes_CGraph = (struct UDGraph_info *)malloc(sizeof(struct UDGraph_info));
        init_UDGraph(odd_nodes_CGraph, lines, odd_deg_num*(odd_deg_num-1)/2);
    }
}