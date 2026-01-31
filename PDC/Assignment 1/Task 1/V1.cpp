#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;

int dat_percentage[3] = {10,50,100};
int PRED_POS = 0,TP = 0,FP = 0,TN = 0,FN = 0;
float e = 2.71828;
float bias = -0.35;
float weights[19] = { 0.12, -0.07, 0.05, 0.09, -0.11, 0.03, 0.08, -0.02,
                      0.06, 0.04, -0.05, 0.10, -0.08, 0.07, 0.02, -0.03,
                      0.11, -0.06 };

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


void calculate_linear_score_and_prob_and_set_data(data_& dat)
{// calculates everything and sets the global values
    float lin_score = bias;
    for( int i = 0; i < 18; i++ )
    {
        lin_score+=(weights[i]*dat.values[i]);
    }
    float prob = 1/(1+pow(e,-1*lin_score));

    if(prob >= 0.5)
    {
        dat.y_prime = 1;
        PRED_POS++;
    }

    if(dat.y == 1 && dat.y_prime == 1)
        TP++;
    else if(dat.y == 0 && dat.y_prime == 1)
        FP++;
    else if(dat.y == 0 && dat.y_prime == 0)
        TN++;
    else if(dat.y == 1 && dat.y_prime == 0)
        FN++;
}

void getOutput(int N,double time,int dat)
{
    cout<<"\nThreads Used: 1";    
    cout<<"\nPercentage of data: "<<dat_percentage[dat]<<"%";
    cout<<"\nChunkSize: Complete";    
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
    cout<<"\n------------------------\n\tV1\n------------------------\n";
    for (int dat_count = 0; dat_count < 3; dat_count++ )
    {
        PRED_POS = 0,TP = 0,FP = 0,TN = 0,FN = 0;
        ifstream input_file("susy.csv");
        // using a the first few rows of the data set during development

        vector<data_> point_list;
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

        for(int i=0;i<point_list.size();i++)
        {    
            calculate_linear_score_and_prob_and_set_data(point_list[i]);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        double elapsed_ms =
        (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
        (t_end.tv_nsec - t_start.tv_nsec) / 1e6;

        getOutput(point_list.size(),elapsed_ms,dat_count);
    }
    return 0;
}