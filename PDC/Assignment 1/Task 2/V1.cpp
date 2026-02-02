#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;

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

int size_data_percentage = 0;
int dat_percentage[3] = {10,50,100};
double checkSum = 0;
int M = 0,N = 0,NZ = 0;
vector<NZHolder> valid_Matrix;

void calculate()
{   //I had to flatten it for the other versions
    
    for(int i=0;i<size_data_percentage;i++)
    {
        checkSum += (valid_Matrix[i].val * ((valid_Matrix[i].c+1)%1000)/1000);
    }        
}

void getOutput(double time,int dat)
{
    cout<<"\nThreads Used: 1";    
    cout<<"\nPercentage of data: "<<dat_percentage[dat]<<"%";
    cout<<"\nChunkSize: Complete";    
    cout<<"\nM: "<<M;
    cout<<"\nN: "<<N;
    cout<<"\nNZ: "<<NZ;
    cout<<"\nCheckSum: "<<checkSum;
    cout<<"\nElapsed Time: "<<time<<"ms"<<endl;
}

int main()
{
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

    cout<<"\n------------------------\n\tV1\n------------------------\n";
    for (int dat_count = 0; dat_count < 3; dat_count++ )
    {

        size_data_percentage = NZ * dat_percentage[dat_count]/100;
        checkSum = 0;        
        struct timespec t_start, t_end;
        clock_gettime(CLOCK_MONOTONIC, &t_start);    
        
        calculate();
        
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        double elapsed_ms =
        (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
        (t_end.tv_nsec - t_start.tv_nsec) / 1e6;
        
        getOutput(elapsed_ms,dat_count);
    }
    valid_Matrix.clear();
    return 0;
}
