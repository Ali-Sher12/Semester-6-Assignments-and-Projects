#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <pthread.h>
#include <queue>

using namespace std;

int data_percentages[3] = {10,50,100};

int nodes;
vector<vector<int>> graph_global;

vector<int> distances;
queue<int> queue_;
vector<int> level_count;
int Edges_total;
int Edges_usable;

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

void getOutput(int dat_count)
{
    cout << "\nData Percentage "<<data_percentages[dat_count]<<"%";
    cout << "\nSource : 0";
    cout << "\nReachable : " << reachable;
    cout << "\nMaximum distance : " << max_distance;
    for (int k = 0; k <= max_distance; k++)
        cout << "\n" << k << " " << level_count[k];
    cout << "\n";
}


int main()
{
    cout<<"\n------------------------\n\tV1\n------------------------\n";

    for(int dat_count = 0; dat_count < 3; dat_count++)
    {
        ifstream input_file("web.txt");
        string line;
        input_file >> nodes >> Edges_total;

        Edges_usable = Edges_total*data_percentages[dat_count]/100;

        int u, v;
        graph_global.resize(nodes);
        for (int i = 0; i < Edges_usable; i++)
        {
            input_file >> u >> v;
            if (u >= 0 && u < nodes && v >= 0 && v < nodes)
            // This condition was brought to you by chatGPT
            graph_global[u].push_back(v);
        }
        input_file.close();

        compute();
        getOutput(dat_count);
    }

    return 0;
}
