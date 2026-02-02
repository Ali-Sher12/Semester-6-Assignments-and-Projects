#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <pthread.h>
#include <queue>

using namespace std;

vector<vector<int>> adj;
int main()
{
    ifstream infile("web.txt");
    string line;
    int nodes, Edges;
    infile >> nodes >> Edges;
    int u, v;
    while (infile >> u >> v)
    {
        adj[u].push_back(v);
    }
    infile.close();
    
    /* ---- BFS from source 0 ---- */
    vector<int> dist(nodes, -1);
    queue<int> q;

    dist[0] = 0;
    q.push(0);

    while (!q.empty())
    {
        int curr = q.front();
        q.pop();

        for (int nxt : adj[curr])
        {
            if (nxt >= 0 && nxt < nodes && dist[nxt] == -1)
            {
                dist[nxt] = dist[curr] + 1;
                q.push(nxt);
            }
        }
    }

    /* ---- Compute statistics ---- */
    long long reachable = 0;
    int maxdist = 0;

    for (int i = 0; i < nodes; i++)
    {
        if (dist[i] != -1)
        {
            reachable++;
            if (dist[i] > maxdist)
                maxdist = dist[i];
        }
    }

    vector<long long> level_count(maxdist + 1, 0);
    for (int i = 0; i < nodes; i++)
    {
        if (dist[i] != -1)
            level_count[dist[i]]++;
    }

    /* ---- Output ---- */
    cout << "SOURCE 0\n";
    cout << "REACHABLE " << reachable << "\n";
    cout << "MAXDIST " << maxdist << "\n";

    for (int k = 0; k <= maxdist; k++)
    {
        cout << k << " " << level_count[k] << "\n";
    }

    return 0;
}
