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

double checkSum = 0;
int M = 0,N = 0,NZ = 0;
vector<NZHolder> valid_Matrix;

void calculate()
{   //I had to flatten it for the other versions
    
    for(int i=0;i<NZ;i++)
    {
        checkSum += (valid_Matrix[i].val * ((valid_Matrix[i].c+1)%1000)/1000);
    }        
}

void getOutput()
{
    cout<<"M: "<<M;
    cout<<"\nN: "<<N;
    cout<<"\nNZ: "<<NZ;
    cout<<"\nCheckSum: "<<checkSum<<endl;    
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
    calculate();
    getOutput();

    valid_Matrix.clear();

    return 0;
}