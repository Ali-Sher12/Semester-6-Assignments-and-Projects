#include <iostream>
#include <pthread.h>
#include <sstream>
#include <fstream>
#include <cmath>
#include <vector>
#include <stdlib.h>

using namespace std;


struct data_
{
    float values[18];
    float y = 0,y_prime = 0;
    void setValue(int index,string value)
    {
        // way better than writing my own function
        if( index == 0 )
            y = stof(value);
        else
            values[index-1] =  stof(value);
    }
};
int dat_percentage[3] = {10,50,100};
int thread_count[4] = {1,2,4,8};
int per_thread = 0;
int no_of_cores = 8;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
vector<data_> point_list;
int PRED_POS = 0,TP = 0,FP = 0,TN = 0,FN = 0;
float e = 2.71828;
float bias = -0.35;
float weights[19] = { 0.12, -0.07, 0.05, 0.09, -0.11, 0.03, 0.08, -0.02,
                      0.06, 0.04, -0.05, 0.10, -0.08, 0.07, 0.02, -0.03,
                      0.11, -0.06 };

void* calculate_linear_score_and_prob_and_set_data(void* arg)
{// calculates everything and sets the global values
    int thread_index = *((int*)arg);
    int start = per_thread*thread_index;
    int end_limit = ((per_thread*thread_index+per_thread)>=point_list.size())?point_list.size():(per_thread*thread_index+per_thread);

    int TP_local = 0,FP_local = 0,TN_local = 0,FN_local = 0,PRED_POS_local = 0;

    for( int gl = start; gl < end_limit; gl++ )
    {
        ////////////////////
        float lin_score = bias;
        for( int i = 0; i < 18; i++ )
        {
            lin_score+=(weights[i]*point_list[gl].values[i]);
        }
        float prob = 1/(1+pow(e,-1*lin_score));
        
        if(prob >= 0.5)
        {
            point_list[gl].y_prime = 1;
            PRED_POS_local++;
        }
        if(point_list[gl].y == 1 && point_list[gl].y_prime == 1)
        TP_local++;
        else if(point_list[gl].y == 0 && point_list[gl].y_prime == 1)
        FP_local++;
        else if(point_list[gl].y == 0 && point_list[gl].y_prime == 0)
        TN_local++;
        else if(point_list[gl].y == 1 && point_list[gl].y_prime == 0)
        FN_local++;
        ////////////////////
    }
    pthread_mutex_lock(&lock);
    PRED_POS += PRED_POS_local;
    TP+=TP_local;
    FP+=FP_local;
    TN+=TN_local;
    FN+=FN_local;
    pthread_mutex_unlock(&lock);

//    delete arg;
    pthread_exit(NULL);    
}

void getOutput(int N,double time,int dat,int thr)
{
    cout<<"\nThreads Used: "<<thread_count[thr];    
    cout<<"\nPercentage of data: "<<dat_percentage[dat]<<"%";
    cout<<"\nChunkSize: "<<per_thread;    
    cout<<"\nTotal Records: " << N << "\n";
    cout<<"PRED_POS : " << PRED_POS << "\n";
    cout<<"TP : " << TP << "\n";
    cout<<"FP : " << FP << "\n";
    cout<<"TN : " << TN << "\n";
    cout<<"FN : " << FN << "\n";
    cout<<"\nElapsed Time: "<<time<<"ms"<<endl;    
}

int main()
{
    int total_num_lines = 5000001;
    cout<<"\n------------------------\n\tV3\n------------------------\n";
    for (int thr = 0; thr < 4; thr++ )    
    {
        for (int dat_count = 0; dat_count < 3; dat_count++ )
        {
            PRED_POS = 0,TP = 0,FP = 0,TN = 0,FN = 0;
            ifstream input_file("susy.csv");
            // using a the first few rows of the data set during development
            string consume;
            getline(input_file, consume);
            int data_limit_counter = 0;
            while(!input_file.eof())
            {
                bool data_reading_exception_done = false;
                string row;
                data_ holder;
                getline(input_file, row);
                stringstream temp_stream(row);
                int i = 0;
                while (!temp_stream.eof())
                {
                    string word;
                    getline(temp_stream, word, ',');
                    if(word=="")
                    {   //This condition is to fix '\n' at the end of the csv
                        data_reading_exception_done = true;break;       
                    }

                    holder.setValue(i,word);
                    i++;
                }
                if(data_reading_exception_done) break;
                point_list.push_back(holder);        
                data_limit_counter++;       
                if(data_limit_counter >= (total_num_lines*dat_percentage[dat_count]/100))
                {
                    cout<<"\n"<<data_limit_counter<<"<--\n";
                    break;
                }
            }
            input_file.close();

            struct timespec t_start, t_end;
            clock_gettime(CLOCK_MONOTONIC, &t_start);    

            pthread_t* threads = new pthread_t[thread_count[thr]];
            per_thread = (int)ceil(point_list.size()/thread_count[thr]);
            for(int i=0;i<thread_count[thr];i++)
            {    
                int* ind = new int;
                *ind = i;
                pthread_create(threads+i, NULL, calculate_linear_score_and_prob_and_set_data, ind);
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(i%no_of_cores, &cpuset);
                pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpuset);                        
            }

            for (int i = 0; i < thread_count[thr]; i++)
            {
                pthread_join(threads[i], NULL);
            }

            clock_gettime(CLOCK_MONOTONIC, &t_end);
            double elapsed_ms =
            (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
            (t_end.tv_nsec - t_start.tv_nsec) / 1e6;

            getOutput(point_list.size(),elapsed_ms,dat_count,thr);
        }
    }

    return 0;
}
