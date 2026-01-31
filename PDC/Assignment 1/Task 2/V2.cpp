#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;

int thread_count[4] = {1,2,4,8};
int per_thread = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct NZHolder
{
    int r = 0;
    int c = 0;
    double val = 0;    
    void set(int a,int b,double c_)
    {
        r=a;c=b;val=c_;
    }
};

double checkSum = 0;
int M = 0,N = 0,NZ = 0;
vector<NZHolder> valid_Matrix;

void* calculate(void* arg)
{
    int thread_index = *((int*)arg);
    int start = per_thread*thread_index;
    int end_limit = ((per_thread*thread_index+per_thread)>=NZ)?NZ:(per_thread*thread_index+per_thread);
    double sum = 0;
    for(int i=start;i<end_limit;i++)
        sum += (valid_Matrix[i].val * ((valid_Matrix[i].c+1)%1000)/1000);
            
    pthread_mutex_lock(&lock);
    checkSum += sum;
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL); 
}

void getOutput(int thr,double time_)
{
    cout<<"V2\n\nThreads Used: "<<thread_count[thr];    
    cout<<"\nM: "<<M;
    cout<<"\nN: "<<N;
    cout<<"\nNZ: "<<NZ;
    cout<<"\nCheckSum: "<<checkSum;    
    cout<<"\nElapsed Time: "<<time_<<endl; 
}

int main()
{
    for( int th = 0; th < 4; th++ )
    {
        checkSum = 0;
        ifstream input_file("webbase.mtx");
        // using a the first few rows of the data set during development
        input_file>>M;
        input_file>>N;
        input_file>>NZ;

        while(!input_file.eof())
        {
            bool data_reading_exception_done = false;
            string row;
            NZHolder holder;

            getline(input_file, row);
            stringstream temp_stream(row);
            int r;int c;double v;
            temp_stream >> r >> c;
            if (!(temp_stream >> v))
                v = 1.0;
            holder.set(r,c,v);
            valid_Matrix.push_back(holder);        
        }
        input_file.close();
        
        pthread_t* threads = new pthread_t[thread_count[th]];
        per_thread = (int)ceil((NZ)/thread_count[th]);

        struct timespec t_start, t_end;
        clock_gettime(CLOCK_MONOTONIC, &t_start);    

        for(int i=0;i<thread_count[th];i++)
        {
            int* ind = new int;
            *ind = i;
            pthread_create(threads+i, NULL, calculate, ind);
        }

        for (int i = 0; i < thread_count[th]; i++)
        {
            pthread_join(threads[i], NULL);
        }

        clock_gettime(CLOCK_MONOTONIC, &t_end);
        double elapsed_ms =
        (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
        (t_end.tv_nsec - t_start.tv_nsec) / 1e6;

        getOutput(th,elapsed_ms);
        valid_Matrix.clear();
        delete[]threads;
    }
    return 0;
}