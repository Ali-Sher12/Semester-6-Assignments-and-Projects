#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;

int thread_count = 8;
int per_thread = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

float** Matrix;
float checkSum = 0;

int M = 0,N = 0,NZ = 0;

void* calculate(void* arg)
{
    int thread_index = *((int*)arg);
    int start = per_thread*thread_index;
    int end_limit = ((per_thread*thread_index+per_thread)>=M*N)?M*N:(per_thread*thread_index+per_thread);
    for(int i=start;i<end_limit;i++)
    {
        pthread_mutex_lock(&lock);
        checkSum += (Matrix[(int)(i/N)][i%N] * (((i%N)+1)%1000));
        pthread_mutex_unlock(&lock);
    }        
    pthread_exit(NULL); 
}

void getOutput()
{
    cout<<"M: "<<M;
    cout<<"\nN: "<<N;
    cout<<"\nNZ: "<<NZ;
    cout<<"\nCheckSum: "<<checkSum<<endl;    
}

void printData()
{
    for(int i=0;i<M;i++)
    {
        for(int j=0;j<N;j++)
        {
            cout<<Matrix[i][j]<<" ";
        }
        cout<<"\n";
    }
}

int main()
{
    ifstream input_file("sample.mtx");
    // using a the first few rows of the data set during development
    input_file>>M;
    input_file>>N;
    input_file>>NZ;

    Matrix = new float*[M];
    for(int i=0;i<M;i++)
    {
        Matrix[i] = new float[N]{0};
    }

    for(int i=0;i<NZ;i++)
    {
        int r,c;
        input_file>>r;
        input_file>>c;
        input_file>>Matrix[r-1][c-1];
    }
    input_file.close();
    pthread_t* threads = new pthread_t[thread_count];
    per_thread = (int)ceil((M*N)/thread_count);
    for(int i=0;i<thread_count;i++)
    {
        int* ind = new int;
        *ind = i;
        pthread_create(threads+i, NULL, calculate, ind);
    }

    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
    }

    getOutput();

    for(int i=0;i<M;i++)
    {
        delete[]Matrix[i];
    }
    delete[]Matrix;
    delete[]threads;
    return 0;
}