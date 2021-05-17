#pragma once
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "UDGraph.c"

static int *nodex;
static size_t x_num = 0;
static int *nodey;
static size_t y_num = 0;

static _Bool color_nodes_from_a_node_in_UDGraph(const struct UDGraph_info *UDGraph, int node_id, _Bool init_color, int *color_set)
{
    color_set[node_id] = init_color;
    if (color_set[node_id] == 0)
    {
        nodex = (int *)realloc(nodex, ++x_num * sizeof(int));
        nodex[x_num - 1] = node_id;
    }
    else
    {
        nodey = (int *)realloc(nodey, ++y_num * sizeof(int));
        nodey[y_num - 1] = node_id;
    }
    struct adj_multiline *adj_line = UDGraph->adj[node_id];
    while (adj_line != NULL)
    {
        int adj_id = (adj_line->i_node != node_id) ? adj_line->j_node : adj_line->i_node;
        if (color_set[adj_id] == -1)
            return color_nodes_from_a_node_in_UDGraph(UDGraph, adj_id, !init_color, color_set);
        else if (color_set[adj_id] != init_color)
            return 0;
        adj_line = (adj_line->i_node == node_id) ? adj_line->i_next : adj_line->j_next;
    }
    return 1;
}

static _Bool is_bipartite(const struct UDGraph_info *UDGraph)
{
    int color_set[NODE_NUM] = {-1};
    for (int v = 0; v < NODE_NUM; v++)
        if (color_set[v] == -1 && color_nodes_from_a_node_in_UDGraph(UDGraph, v, 0, color_set) == 0)
        {
            free(nodex); free(nodey);
            x_num = 0; y_num = 0;
            return 0;
        }
    return 1;
}

/* a matching in undircted graph */
struct matching
{   struct adj_multiline **matched_line;
    size_t line_num;};

_Bool find_augmenting_path(const struct UDGraph_info *UDGraph, struct matching *opti_matching, int node_id, _Bool *isvisited, _Bool *ismatched, size_t *line_count)
{
    struct adj_multiline *adj_line = UDGraph->adj[node_id];
    while (adj_line != NULL)
    {
        int adj_id = (adj_line->i_node != node_id) ? adj_line->i_node : adj_line->j_node;
        if (!isvisited[adj_id])
        {
            isvisited[adj_id] = 1;
            if (ismatched[adj_id] == 0 || find_augmenting_path(UDGraph, opti_matching, adj_id, isvisited, ismatched, line_count))
            {
                if (ismatched[adj_id] == 0)
                {
                    opti_matching->line_num++;
                    opti_matching->matched_line = (struct adj_multiline **)realloc(opti_matching->matched_line, opti_matching->line_num * 8UL);
                    *line_count = 0;
                    ismatched[adj_id] = 1;
                    ismatched[node_id] = 1;
                }
                adj_line->ismarked = !adj_line->ismarked;
                if (adj_line->ismarked)
                    opti_matching->matched_line[*line_count++] = adj_line;
                return 1;
            }
        }
        adj_line = (adj_line->i_node == node_id) ? adj_line->i_next : adj_line->j_next;
    }
    return 0;
}

struct adj_multiline** Hungarian_algorithm_in_UDGraph(const struct UDGraph_info *UDGraph, size_t *segment_num)
{
    if (is_bipartite(UDGraph) == 0)
    {
        fputs("The undirected graph is not bipartite.\n", stderr);
        return NULL;
    }
    *segment_num = 0;
    struct matching *opti_matching;
    *opti_matching = (struct matching){0};
    _Bool isvisited[NODE_NUM] = {0}; _Bool ismatched[NODE_NUM] = {0};
    size_t *line_count;
    for (int i = 0; i < x_num; i++)
    {
        memset(isvisited, 0, sizeof(isvisited));
        if (ismatched[nodex[i]] = 0 ||
        find_augmenting_path(UDGraph, opti_matching, nodex[i], isvisited, ismatched, line_count) == 0)
            return opti_matching;
    }
}

struct undirc_line* Kuhn_Munkres_algorithm_in_UDGraph(const struct UDGraph_info *UDGraph)
{

}