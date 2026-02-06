#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <pthread.h>
#include <cmath>
#include <queue>
#include <map>

using namespace std;

int thread_count[4] = {1,2,4,8};
int data_percentages[3] = {10,50,100};
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int nodes;
map<int,vector<int>> graph_global;

vector<int> distances;
queue<int> queue_;
vector<int> level_count;
int Edges_total;
int Edges_usable;

int reachable = 0;
int max_distance = 0;
int per_thread = 0;

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
    }//This cannot be parallelised until there is a queue

}

void* MaxDistanceFind(void* arg)
{
    int thread_index = *((int*)arg);
    delete (int*)arg;
    int start = per_thread*thread_index;
    int end_limit = ((per_thread*thread_index+per_thread)>=nodes)?nodes:(per_thread*thread_index+per_thread);
    int local_reachable = 0;
    int local_maximum = 0;
    //Max distance
    for (int i = start; i < end_limit; i++)
    {
        if (distances[i] != -1)
        {
            local_reachable++;
            if (distances[i] > local_maximum)
                local_maximum = distances[i];
        }
    }
    pthread_mutex_lock(&lock);
    reachable += local_reachable;
    max_distance = max(local_maximum,max_distance);
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

void* Nodes_At_K_Distance(void* arg)
{
    int thread_index = *((int*)arg);
    delete (int*)arg;
    int start = per_thread*thread_index;
    int end_limit = ((per_thread*thread_index+per_thread)>=nodes)?nodes:(per_thread*thread_index+per_thread);

    vector<int> level_count_local(max_distance + 1, 0);
    //Nodes a k distance
    for (int i = start; i < end_limit; i++)
    {
        if (distances[i] != -1)
            level_count_local[distances[i]]++;
    }
    pthread_mutex_lock(&lock);
    for (int i = 0; i <= max_distance; i++)
        level_count[i] += level_count_local[i];
    pthread_mutex_unlock(&lock);    
    pthread_exit(NULL);
}

void getOutput(int elapsed_ms,int dat_count,int thr)
{
    cout << "\nData Percentage "<<data_percentages[dat_count]<<"%";
    cout << "\nTime : " << elapsed_ms<<"ms";
    cout << "\nThreads Used : " << thread_count[thr];    
    cout << "\nChunk Size : " << per_thread<<" elements processed.";        
    cout << "\nSource : 0";
    cout << "\nReachable : " << reachable;
    cout << "\nMaximum distance : " << max_distance;
    for (int k = 0; k <= max_distance; k++)
        cout << "\n" << k << " " << level_count[k];
    cout << "\n";
}


int main()
{
    cout<<"\n------------------------\n\tV2\n------------------------\n";

    for(int dat_count = 0; dat_count < 3; dat_count++)    
    {
        for( int thr = 0; thr < 4; thr++ )
        {
            ifstream input_file("web.txt");
            string line;
            input_file >> nodes >> Edges_total;

            Edges_usable = Edges_total*data_percentages[dat_count]/100;

            int u, v;
            for (int i = 0; i < Edges_usable; i++)
            {
                input_file >> u >> v;
                if (u > nodes)nodes=u;
                graph_global[u].push_back(v);
            }
            input_file.close();

            pthread_t* threads_A = new pthread_t[thread_count[thr]];
            pthread_t* threads_B = new pthread_t[thread_count[thr]];
            per_thread = (int)ceil((nodes)/thread_count[thr]);
            
            struct timespec t_start, t_end;
            clock_gettime(CLOCK_MONOTONIC, &t_start);
            compute();
            for(int i=0;i<thread_count[thr];i++)
            {
                int* ind = new int;
                *ind = i;
                pthread_create(threads_A+i, NULL, MaxDistanceFind, ind);
            }
            for (int i = 0; i < thread_count[thr]; i++)
            {
                pthread_join(threads_A[i], NULL);
            }

            level_count.assign(max_distance + 1, 0);

            for(int i=0;i<thread_count[thr];i++)
            {
                int* ind = new int;
                *ind = i;
                pthread_create(threads_B+i, NULL, Nodes_At_K_Distance, ind);
            }
            for (int i = 0; i < thread_count[thr]; i++)
            {
                pthread_join(threads_B[i], NULL);
            }

            clock_gettime(CLOCK_MONOTONIC, &t_end);
            double elapsed_ms =
            (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
            (t_end.tv_nsec - t_start.tv_nsec) / 1e6;
            getOutput(elapsed_ms,dat_count,thr);

            delete[]threads_A;
            delete[]threads_B;
            map<int,vector<int>>().swap(graph_global);
            vector<int>().swap(distances);
            vector<int>().swap(level_count);
            queue<int>().swap(queue_);
            nodes = 0;
            Edges_total = 0;
            Edges_usable = 0;
            reachable = 0;
            max_distance = 0;
            per_thread = 0;
        }
    }
    return 0;
}
