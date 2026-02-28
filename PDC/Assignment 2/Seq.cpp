#include <iostream>
#include <fstream>
#include <vector>
#include <map>

using namespace std;

int nodes = 0;
int total_trinagles = 0;
map<int,vector<int>> graph_global;

int compute()
{
    int triangles = 0;

    vector<int> deg(nodes);

    for (int i = 0; i < nodes; i++)
        deg[i] = graph_global[i].size();

    for (int u = 0; u < nodes; u++) 
    {
        for (int v : graph_global[u]) 
        {
            if ((deg[v] < deg[u]) || (deg[v] == deg[u] && v < u)) continue;
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

void getOutput(int elapsed_ms)
{
    cout << "\nTotal Vertices "<<nodes;
    cout << "\nEdges "<<graph_global.size();    
    cout << "\nTime : " << elapsed_ms<<"ms";
    cout << "\nTotal Triangles : " << total_trinagles;
    cout << "\n";
}

bool alreadyPresent(int u,int v)
{
    for(int i=0;i<graph_global[u].size();i++)
        if(graph_global[u].at(i) == v)
            return true;
    
    return false;
}

int main()
{
//        ifstream input_file("Data/dataset1.txt");
//        ifstream input_file("Data/dataset1.txt");
        ifstream input_file("Data/dataset3.txt");        
        string line;
        
        int u, v;
        while(input_file >> u >> v)
        {            
            if(u==v)continue;
            if(alreadyPresent(u,v))
            if(u>nodes)nodes = u;
            if(v>nodes)nodes = v;            
            graph_global[u].push_back(v);
            graph_global[v].push_back(u);            
        }
        input_file.close();
        cout<<"Data Read.\n";

        struct timespec t_start, t_end;
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        total_trinagles = compute();
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        double elapsed_ms =
        (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
        (t_end.tv_nsec - t_start.tv_nsec) / 1e6;

        getOutput(elapsed_ms);
        
    return 0;
}