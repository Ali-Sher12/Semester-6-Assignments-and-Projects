#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <pthread.h>
#include <queue>

using namespace std;

int data_percentages[3] = {10,50,100};

int nodes, Edges;
vector<vector<int>> graph_global;
vector<int> distances;
queue<int> queue_;
vector<int> level_count;

int reachable = 0;
int max_distance = 0;

void compute()
{
    //BFS
    distances.assign(nodes, -1);
    distances[0] = 0;
    queue_.push(0);
    while (!queue_.empty())
    {
        int current_node = queue_.front();
        queue_.pop();

        for (int i = 0; i < graph_global[current_node].size(); i++)
        {
            int neighbor = graph_global[current_node][i];
            if (distances[neighbor] == -1)
            {
                distances[neighbor] = distances[current_node] + 1;
                queue_.push(neighbor);
            }
        }
    }

    //Max distance
    for (int i = 0; i < nodes; i++)
    {
        if (distances[i] != -1)
        {
            reachable++;
            if (distances[i] > max_distance)
                max_distance = distances[i];
        }
    }

    //Nodes a k distance
    level_count.assign(max_distance + 1, 0);
    for (int i = 0; i < nodes; i++)
    {
        if (distances[i] != -1)
            level_count[distances[i]]++;
    }

}

void getOutput()
{
    cout << "SOURCE 0\n";
    cout << "REACHABLE " << reachable << "\n";
    cout << "MAXDIST " << max_distance << "\n";
    for (int k = 0; k <= max_distance; k++)
    {
        cout << k << " " << level_count[k] << "\n";
    }    
}


int main()
{
    cout<<"\n------------------------\n\tV1\n------------------------\n";
    ifstream input_file("web.txt");
    string line;
    input_file >> nodes >> Edges;
    int u, v;
    graph_global.resize(nodes);
    while (input_file >> u >> v)
    {
        graph_global[u].push_back(v);
    }
    input_file.close();
    compute();
    getOutput();

    return 0;
}
