#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <immintrin.h>

using namespace std;

int nodes = 0;
int num_edges = 0;
int total_triangles = 0;
vector<vector<int>> graph_global;
vector<int> deg;

int intersect_simd(int you, int vee)
{
    int count = 0;
    int sA = graph_global[you].size();
    int sB = graph_global[vee].size();
    int i = 0, j = 0;

    while (i + 8 <= sA && j + 8 <= sB)
    {
        __m256i va = _mm256_loadu_si256((__m256i*)&graph_global[you][i]);

        if (graph_global[you][i + 7] < graph_global[vee][j]) { i += 8; continue; }
        if (graph_global[vee][j + 7] < graph_global[you][i]) { j += 8; continue; }

        for (int r = 0; r < 8; r++)
        {
            __m256i splat = _mm256_set1_epi32(graph_global[vee][j + r]);
            __m256i cmp   = _mm256_cmpeq_epi32(va, splat);
            int mask      = _mm256_movemask_epi8(cmp);

            if (mask != 0)
            {
                int matched_idx = __builtin_ctz(mask) / 4;
                int w = graph_global[you][i + matched_idx];

                if ((deg[w] > deg[you] || (deg[w] == deg[you] && w > you)) &&
                    (deg[w] > deg[vee] || (deg[w] == deg[vee] && w > vee)))
                    count++;
            }
        }

        if (graph_global[you][i + 7] < graph_global[vee][j + 7]) i += 8;
        else j += 8;
    }

    while (i < sA && j < sB)
    {
        if (graph_global[you][i] == graph_global[vee][j])
        {
            int w = graph_global[you][i];
            if ((deg[w] > deg[you] || (deg[w] == deg[you] && w > you)) &&
                (deg[w] > deg[vee] || (deg[w] == deg[vee] && w > vee)))
                count++;
            i++; j++;
        }
        else if (graph_global[you][i] < graph_global[vee][j]) i++;
        else j++;
    }

    return count;
}

void compute()
{

    deg.resize(nodes);
    for (int i = 0; i < nodes; i++)
        deg[i] = graph_global[i].size();

    for (int u = 0; u < nodes; u++)
    {
        for (int v1 = 0; v1 < graph_global[u].size(); v1++)
        {
            int v = graph_global[u][v1];
            if ((deg[v] < deg[u]) || (deg[v] == deg[u] && v < u)) continue;
            total_triangles += intersect_simd(u, v);
        }
    }
}

void getOutput(double elapsed_ms)
{
    cout << "\nTotal Vertices : " << nodes;
    cout << "\nTotal Edges    : " << num_edges;
    cout << "\nTriangles      : " << total_triangles;
    cout << "\nTime           : " << elapsed_ms << " ms\n";
}

int main()
{
    string filename = "Data/dataset2.txt";

    ifstream input_file(filename);
    int u, v;
    while (input_file >> u >> v)
    {
        if (u == v) continue;
        if (u > nodes) nodes = u;
        if (v > nodes) nodes = v;
    }
    nodes += 1;
    input_file.close();

    graph_global.resize(nodes);

    input_file.open(filename);
    while (input_file >> u >> v)
    {
        if (u == v) continue;
        graph_global[u].push_back(v);
        graph_global[v].push_back(u);
    }
    input_file.close();
    cout << "Data Read.\n";

    for (int i = 0; i < nodes; i++)
    {
        sort(graph_global[i].begin(), graph_global[i].end());
        graph_global[i].erase(
            unique(graph_global[i].begin(), graph_global[i].end()),
            graph_global[i].end()
        );
    }

    for (int i = 0; i < nodes; i++)
        num_edges += graph_global[i].size();
    num_edges /= 2;

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    compute();

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed_ms = (t_end.tv_sec  - t_start.tv_sec)  * 1000.0 +
                        (t_end.tv_nsec - t_start.tv_nsec) / 1e6;

    getOutput(elapsed_ms);

    return 0;
}