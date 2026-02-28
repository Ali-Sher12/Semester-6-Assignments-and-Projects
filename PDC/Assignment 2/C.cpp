#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <omp.h>

using namespace std;

int nodes = 0;
int num_edges = 0;
int total_triangles = 0;
vector<vector<int>> graph_global;

void compute()
{
    vector<int> deg(nodes);
    for (int i = 0; i < nodes; i++)
        deg[i] = graph_global[i].size();

    #pragma omp parallel for schedule(dynamic) reduction(+:total_triangles)
    for (int u = 0; u < nodes; u++)
    {
        for (int v : graph_global[u])
        {
            if (deg[v] < deg[u]) continue;
            if (deg[v] == deg[u] && v < u) continue;

            int i = 0, j = 0;
            while (i < (int)graph_global[u].size() && j < (int)graph_global[v].size())
            {
                if (graph_global[u][i] == graph_global[v][j])
                {
                    int w = graph_global[u][i];
                    if ((deg[w] > deg[u] || (deg[w] == deg[u] && w > u)) &&
                        (deg[w] > deg[v] || (deg[w] == deg[v] && w > v)))
                        total_triangles++;
                    i++; j++;
                }
                else if (graph_global[u][i] < graph_global[v][j]) i++;
                else j++;
            }
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

    omp_set_num_threads(4);
    cout << "Using 4 threads.\n";

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