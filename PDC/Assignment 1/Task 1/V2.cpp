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

int thread_count = 8; // here
int per_thread = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
vector<data_> point_list;
int PRED_POS = 0,TP = 0,FP = 0,TN = 0,FN = 0;
float e = 2.71828;
float bias = -0.35;
float weights[19] = { 0.12, -0.07, 0.05, 0.09, -0.11, 0.03, 0.08, -0.02,
                      0.06, 0.04, -0.05, 0.10, -0.08, 0.07, 0.02, -0.03,
                      0.11, -0.06 };



void print_dataset(vector<data_>& point_list)
{
    for(int i=0;i<point_list.size();i++)
    {
        for(int j=0;j<18;j++)
            cout<<point_list[i].values[j]<<"   ";
        cout<<"\n";
    }
}

void* calculate_linear_score_and_prob_and_set_data(void* arg)
{// calculates everything and sets the global values
    int thread_index = *((int*)arg);
    int start = per_thread*thread_index;
    int end_limit = ((per_thread*thread_index+per_thread)>=point_list.size())?point_list.size():(per_thread*thread_index+per_thread);

    for( int gl = start; gl < end_limit; gl++ )
    {
        ////////////////////
        float lin_score = bias;
        for( int i = 0; i < 18; i++ )
        {
            lin_score+=(weights[i]*point_list[gl].values[i]);
        }
        float prob = 1/(1+pow(e,-1*lin_score));
        
        pthread_mutex_lock(&lock);
        if(prob >= 0.5)
        {
            point_list[gl].y_prime = 1;
            PRED_POS++;
        }
        if(point_list[gl].y == 1 && point_list[gl].y_prime == 1)
            TP++;
        else if(point_list[gl].y == 0 && point_list[gl].y_prime == 1)
            FP++;
        else if(point_list[gl].y == 0 && point_list[gl].y_prime == 0)
            TN++;
        else if(point_list[gl].y == 1 && point_list[gl].y_prime == 0)
            FN++;
        pthread_mutex_unlock(&lock);
        ////////////////////
    }

//    delete arg;
    pthread_exit(NULL);    
}

void getOutput(int N)
{
    cout<<"\nTotal Records: " << N << "\n";
    cout<<"PRED_POS : " << PRED_POS << "\n";
    cout<<"TP : " << TP << "\n";
    cout<<"FP : " << FP << "\n";
    cout<<"TN : " << TN << "\n";
    cout<<"FN : " << FN << "\n";
}

int main()
{
    ifstream input_file("susy.csv");
    // using a the first few rows of the data set during development
    pthread_t* threads;
    string consume;
    getline(input_file, consume);
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
    }
    input_file.close();

    threads = new pthread_t[thread_count];
    per_thread = (int)ceil(point_list.size()/thread_count);
    for(int i=0;i<thread_count;i++)
    {    
        int* ind = new int;
        *ind = i;
        pthread_create(threads+i, NULL, calculate_linear_score_and_prob_and_set_data, ind);
    }

    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
    }

    getOutput(point_list.size());

    return 0;
}