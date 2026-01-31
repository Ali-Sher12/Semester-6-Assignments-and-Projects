#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;

float checkSum = 0;
int M = 0,N = 0,NZ = 0;
vector<vector<float>> Matrix;

void calculate()
{ //I originally came up with this interpretation for V1
    for(int i=0;i<M;i++)
    {
        for(int j=0;j<N;j++)
        {
            checkSum += (Matrix[i][j] * ((j+1)%1000));
        }
    }
}

// void calculate()
// {   //I had to flatten it for the other versions
//     for(int i=0;i<M*N;i++)
//     {
//         checkSum += (Matrix[(int)(i/N)][i%N] * (((i%N)+1)%1000));
//     }        
// }

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
    ifstream input_file("webbase.mtx");
    // using a the first few rows of the data set during development
    input_file>>M;
    input_file>>N;
    input_file>>NZ;

    Matrix.resize(M);
    for(int i=0;i<M;i++)
    {
        Matrix[i].resize(N,0);
    }

    for(int i=0;i<NZ;i++)
    {
        int r,c;
        input_file>>r;
        input_file>>c;
        input_file>>Matrix[r-1][c-1];
    }
    input_file.close();
    calculate();
    getOutput();

    for(int i=0;i<M;i++)
    {
        Matrix[i].clear();
    }
    Matrix.clear();

    return 0;
}