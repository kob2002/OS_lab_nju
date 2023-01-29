#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include "Tool.h"
using namespace std;


void Tool::generateFile(){
    ifstream myfile("D:\\m\\makeFileInSubDir.txt");
    ofstream outfile("D:\\m\\new.txt", ios::app);
    string temp;
    if (!myfile.is_open())
    {
        cout << "fail!" << endl;
    }
    while(getline(myfile,temp))
    {
        //cout<<temp<<endl;
        for(int i=0;i<temp.length();i++){
            if(temp[i]==' '||temp[i]=='\n'){
                continue;
            }
            outfile << temp[i];
        }


    }
    myfile.close();
    outfile.close();
}


 