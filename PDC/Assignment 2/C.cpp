#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <omp.h>

using namespace std;

int nodes = 0;
int num_edges = 0;
long long total_triangles = 0;
vector<vector<int>> graph_global;

long long compute()
{
    long long triangles = 0;

    vector<int> deg(nodes);
    for (int i = 0; i < nodes; i++)
        deg[i] = graph_global[i].size();

    #pragma omp parallel for schedule(dynamic) reduction(+:triangles)
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
                        triangles++;
                    i++; j++;
                }
                else if (graph_global[u][i] < graph_global[v][j]) i++;
                else j++;
            }
        }
    }

    return triangles;
}

void getOutput(double elapsed_ms)
{
    cout << "\nTotal Vertices : " << nodes;
    cout << "\nTotal Edges    : " << num_edges;
    cout << "\nTriangles      : " << total_triangles;
    cout << "\nTime           : " << elapsed_ms << " ms\n";
}

int main(int argc, char* argv[])
{
    string filename = (argc > 1) ? argv[1] : "Data/dataset1.txt";

    int num_threads = 4;
    if (argc > 2) num_threads = atoi(argv[2]);
    omp_set_num_threads(num_threads);
    cout << "Using " << num_threads << " threads.\n";

    // ── Pass 1: find max node ID ──────────────────────────────────────────
    ifstream input_file(filename);
    if (!input_file.is_open()) {
        cerr << "Cannot open file: " << filename << "\n";
        return 1;
    }

    int u, v;
    while (input_file >> u >> v)
    {
        if (u == v) continue;
        if (u > nodes) nodes = u;
        if (v > nodes) nodes = v;
    }
    nodes += 1;
    input_file.close();

    // ── Pass 2: load edges ────────────────────────────────────────────────
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

    // ── Sort and deduplicate ──────────────────────────────────────────────
    for (int i = 0; i < nodes; i++)
    {
        sort(graph_global[i].begin(), graph_global[i].end());
        graph_global[i].erase(
            unique(graph_global[i].begin(), graph_global[i].end()),
            graph_global[i].end()
        );
    }

    // ── Count edges ───────────────────────────────────────────────────────
    for (int i = 0; i < nodes; i++)
        num_edges += graph_global[i].size();
    num_edges /= 2;

    // ── Run compute ───────────────────────────────────────────────────────
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    total_triangles = compute();

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed_ms = (t_end.tv_sec  - t_start.tv_sec)  * 1000.0 +
                        (t_end.tv_nsec - t_start.tv_nsec) / 1e6;

    getOutput(elapsed_ms);

    return 0;
}