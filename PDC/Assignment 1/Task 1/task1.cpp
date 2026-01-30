#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;

struct data_
{
    string values[19];
};

int main()
{
    ifstream input_file("sus.csv");// using a the first few rows of teh data set during development

    vector<data_> point_list;

    while(!input_file.eof())
    {
        string row;
        data_ holder;
        getline(input_file, row);
        stringstream temp_stream(row);
        int i = 0;
        while (!temp_stream.eof())
        {
            string word;
            getline(temp_stream, word, ',');
            holder.values[i] = word;
            i++;
        }
        point_list.push_back(holder);
    }
    
    for(int i=0;i<point_list.size();i++)
    {
        for(int j=0;j<19;j++)
            cout<<point_list[i].values[j]<<"   ";
        cout<<"\n";
    }

    input_file.close();
    return 0;
}