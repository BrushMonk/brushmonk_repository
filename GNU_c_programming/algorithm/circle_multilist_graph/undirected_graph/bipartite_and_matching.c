#pragma once
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "UDGraph.c"

/* x node subset from bipartite graph */
static int *nodex;
static size_t x_num = 0;
/* y node subset from bipartite graph */
static int *nodey;
static size_t y_num = 0;

static _Bool color_nodes_from_a_node_in_UDGraph(const struct UDGraph_info *UDGraph, int node_id, _Bool init_color, int *color_set)
{
    color_set[node_id] = init_color;
    struct adj_multiline *adj_line = UDGraph->adj[node_id];
    if (color_set[node_id] == 0 && adj_line != NULL)
    {
        nodex = (int *)realloc(nodex, ++x_num * sizeof(int));
        nodex[x_num - 1] = node_id;
    }
    else if (color_set[node_id] == 1 && adj_line != NULL)
    {
        nodey = (int *)realloc(nodey, ++y_num * sizeof(int));
        nodey[y_num - 1] = node_id;
    }
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

static struct adj_multiline *get_match_line(const struct UDGraph_info *UDGraph, int node_id)
{
    struct adj_multiline *adj_line = UDGraph->adj[node_id];
    while (adj_line != NULL && adj_line->ismarked == 0)
        adj_line = (adj_line->i_node == node_id) ? adj_line->i_next : adj_line->j_next;
    return adj_line;
}

static size_t update_augmenting_path_in_UWGraph(const struct UDGraph_info *UDGraph, int x_node, _Bool *isvisited)
{
    struct adj_multiline *adj_line = UDGraph->adj[x_node];
    while (adj_line != NULL)
    {
        int y_node = (adj_line->i_node != x_node) ? adj_line->i_node : adj_line->j_node;
        /* visit those unvisited nodes */
        if (!isvisited[y_node])
        {
            isvisited[y_node] = 1;
            struct adj_multiline *y_match_line = get_match_line(UDGraph, y_node);
            int y_match = -1;
            if (y_match_line != NULL)
                y_match = (y_match_line->i_node != y_node) ? y_match_line->i_node : y_match_line->j_node;
            /* find new match x node of y node node and recur itself until y node doesn't have match x node */
            if (y_match_line == NULL || update_augmenting_path_in_UWGraph(UDGraph, y_match, isvisited))
            {
                /* if y node has match x node, let match line unmarked */
                if (y_match_line != NULL)
                    y_match_line->ismarked = 0;
                /* let this unvisited x node become y's match node.
                if y node doesn't have match x node, let new x node be y's match node. */
                adj_line->ismarked = 1;
                return 1;
            }
        }
        /* if we can't find augmenting path from current y node, try another y node and repeat it. */
        adj_line = (adj_line->i_node == x_node) ? adj_line->i_next : adj_line->j_next;
    }
    /* if every adjacency node of x in alternating path is matched,
    we are unable to find a augmenting path. Therefore this is the maximum alternating path.
    Then we jump out of this recursion. */
    return 0;
}

struct matching *Hungarian_algorithm_in_UWGraph(const struct UDGraph_info *UDGraph)
{
    if (is_bipartite(UDGraph) == 0)
    {
        fputs("The undirected graph is not bipartite.\n", stderr);
        return NULL;
    }
    struct matching *max_matching; *max_matching = (struct matching){0};
    _Bool isvisited[NODE_NUM] = {0};
    for (size_t xcount = 0; xcount < x_num; xcount++)
    {
        /* reset all nodes unvisited in UDGraph */
        memset(isvisited, 0, sizeof(isvisited));
        if (get_match_line(UDGraph, nodex[xcount]) == NULL)
            max_matching->line_num += update_augmenting_path_in_UWGraph(UDGraph, nodex[xcount], isvisited);
    }
    memset(isvisited, 0, sizeof(isvisited));
    max_matching->matched_line = get_all_marked_lines_in_UDGraph(UDGraph, max_matching->line_num, isvisited);
    return max_matching;
}

static size_t update_min_augmenting_path_in_UDGraph(const struct UDGraph_info *UDGraph, int x_node, _Bool *isvisited, int64_t *node_weight, int64_t *slack)
{
    struct adj_multiline *adj_line = UDGraph->adj[x_node];
    while (adj_line != NULL)
    {
        int y_node = (adj_line->i_node != x_node) ? adj_line->i_node : adj_line->j_node;
        /* visit those unvisited nodes */
        if (!isvisited[y_node])
        {
            /* if this line is in subGraph */
            if (node_weight[x_node] - node_weight[y_node] == adj_line->weight)
            {
                isvisited[y_node] = 1;
                struct adj_multiline *y_match_line = get_match_line(UDGraph, y_node);
                int y_match = -1;
                if (y_match_line != NULL)
                    y_match = (y_match_line->i_node != y_node) ? y_match_line->i_node : y_match_line->j_node;
                /* find new match x node of y node node and recur itself until y node doesn't have match x node */
                if (y_match_line == NULL || update_min_augmenting_path_in_UDGraph(UDGraph, y_match, isvisited, node_weight, slack))
                {
                    /* if y node has match x node, let match line unmarked */
                    if (y_match_line != NULL)
                        y_match_line->ismarked = 0;
                    /* let this unvisited x node become y's match node.
                    if y node doesn't have match x node, let new x node be y's match node. */
                    adj_line->ismarked = 1;
                    return 1;
                }
            }
            /* if this line is not in subGraph, record the minimum weight decrement for visited node */
            else if (adj_line->weight - node_weight[x_node] + node_weight[y_node] < *slack)
                *slack = (adj_line->weight - node_weight[x_node] + node_weight[y_node]);
        }
        /* if we can't find augmenting path from current y node, try another y node and repeat it. */
        adj_line = (adj_line->i_node == x_node) ? adj_line->i_next : adj_line->j_next;
    }
    /* if every adjacency node of x in alternating path is matched,
    we are unable to find a augmenting path. Therefore this is the maximum alternating path.
    Then we jump out of this recursion. */
    return 0;
}

struct matching* min_Kuhn_Munkres_algorithm_in_UDGraph(const struct UDGraph_info *UDGraph)
{
    if (is_bipartite(UDGraph) == 0)
    {
        fputs("The undirected graph is not bipartite.\n", stderr);
        return NULL;
    }
    int64_t node_weight[NODE_NUM] = {0};
    _Bool isvisited[NODE_NUM] = {0};
    /* slack value used for decreasing node weight */
    int64_t *slack;
    struct matching *perf_matching; *perf_matching = (struct matching){0};
    for (size_t xcount = 0; xcount < x_num; xcount++)
    {
        if (UDGraph->adj[nodex[xcount]] != NULL)
            node_weight[nodex[xcount]] = UDGraph->adj[nodex[xcount]]->weight;
    }
    for (size_t xcount = 0; xcount < x_num; xcount++)
    {
        while (1)
        {
            /* reset all nodes unvisited in UDGraph */
            memset(isvisited, 0, sizeof(isvisited));
            *slack = LLONG_MAX;
            if (update_min_augmenting_path_in_UDGraph(UDGraph, nodex[xcount], isvisited, node_weight, slack))
            {
                perf_matching->line_num++;
                break;
            }
            else
            {
                for (size_t i = 0; i < NODE_NUM; i++)
                {
                    if (isvisited[nodex[i]])
                        /* increase visited x node weight by slack value */
                        node_weight[nodex[i]] += *slack;
                    if (isvisited[nodey[i]])
                        /* increase visited y node weight by slack value */
                        node_weight[nodey[i]] += *slack;
                }
            }
        }
    }
    perf_matching->matched_line = get_all_marked_lines_in_UDGraph(UDGraph, perf_matching->line_num);
    return perf_matching;
}

static size_t update_max_augmenting_path_in_UDGraph(const struct UDGraph_info *UDGraph, int x_node, _Bool *isvisited, int64_t *node_weight, int64_t *slack)
{
    struct adj_multiline *adj_line = UDGraph->adj[x_node];
    while (adj_line != NULL)
    {
        int y_node = (adj_line->i_node != x_node) ? adj_line->i_node : adj_line->j_node;
        /* visit those unvisited nodes */
        if (!isvisited[y_node])
        {
            /* if this line is in subGraph */
            if (node_weight[x_node] + node_weight[y_node] == adj_line->weight)
            {
                isvisited[y_node] = 1;
                struct adj_multiline *y_match_line = get_match_line(UDGraph, y_node);
                int y_match = -1;
                if (y_match_line != NULL)
                    y_match = (y_match_line->i_node != y_node) ? y_match_line->i_node : y_match_line->j_node;
                /* find new match x node of y node node and recur itself until y node doesn't have match x node */
                if (y_match_line == NULL || update_max_augmenting_path_in_UDGraph(UDGraph, y_match, isvisited, node_weight, slack))
                {
                    /* if y node has match x node, let match line unmarked */
                    if (y_match_line != NULL)
                        y_match_line->ismarked = 0;
                    /* let this unvisited x node become y's match node.
                    if y node doesn't have match x node, let new x node be y's match node. */
                    adj_line->ismarked = 1;
                    return 1;
                }
            }
            /* if this line is not in subGraph, record the minimum weight variation for visited node */
            else if (node_weight[x_node] + node_weight[y_node] - adj_line->weight < *slack)
                *slack = (node_weight[x_node] + node_weight[y_node] - adj_line->weight);
        }
        /* if we can't find augmenting path from current y node, try another y node and repeat it. */
        adj_line = (adj_line->i_node == x_node) ? adj_line->i_next : adj_line->j_next;
    }
    /* if every adjacency node of x in alternating path is matched,
    we are unable to find a augmenting path. Therefore this is the maximum alternating path.
    Then we jump out of this recursion. */
    return 0;
}

struct matching* max_Kuhn_Munkres_algorithm_in_UDGraph(const struct UDGraph_info *UDGraph)
{
    if (is_bipartite(UDGraph) == 0)
    {
        fputs("The undirected graph is not bipartite.\n", stderr);
        return NULL;
    }
    int64_t node_weight[NODE_NUM] = {0};
    _Bool isvisited[NODE_NUM] = {0};
    /* slack value used for variating node weight */
    int64_t *slack;
    struct matching *perf_matching; *perf_matching = (struct matching){0};
    for (size_t xcount = 0; xcount < x_num; xcount++)
    {
        struct adj_multiline *cur = UDGraph->adj[nodex[xcount]], *last;
        while (cur != NULL)
        {
            last = cur;
            cur = (cur->i_node == nodex[xcount]) ? cur->i_next : cur->j_next;
        }
        if (last != NULL)
            node_weight[nodex[xcount]] = last->weight;
    }
    for (size_t xcount = 0; xcount < x_num; xcount++)
    {
        while (1)
        {
            /* reset all nodes unvisited in UDGraph */
            memset(isvisited, 0, sizeof(isvisited));
            *slack = LLONG_MAX;
            if (update_max_augmenting_path_in_UDGraph(UDGraph, nodex[xcount], isvisited, node_weight, slack))
            {
                perf_matching->line_num++;
                break;
            }
            else
            {
                for (size_t i = 0; i < NODE_NUM; i++)
                {
                    if (isvisited[nodex[i]])
                        /* increase visited x node weight by slack value */
                        node_weight[nodex[i]] -= *slack;
                    if (isvisited[nodey[i]])
                        /* decrease visited y node weight by slack value */
                        node_weight[nodey[i]] += *slack;
                }
            }
        }
    }
    perf_matching->matched_line = get_all_marked_lines_in_UDGraph(UDGraph, perf_matching->line_num);
    return perf_matching;
}