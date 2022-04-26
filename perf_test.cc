// g++ -o perf_test perf_test.cc -lstdc++

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <cstring>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>

using namespace std;
using std::chrono::duration;
using std::chrono::high_resolution_clock;

#define MS 1000

int main(int argc, char** argv) {
    FILE *fp;
    char outFileName[100],expnum[10],filepath[100]={'\0'},filesizestr[100000];
    long filesize=0,i;
    
    if(argc!=4){
        printf("Error! Mention file path, test num, write size (in B)");
        exit(0);
    }
    else{
        cout<<"File path: "<<argv[1]<<" Test num: "<<argv[2]<<" Write size: "<<argv[3]<<endl;
        strcpy(filepath,argv[1]);
        strcpy(expnum,argv[2]);
        strcpy(filesizestr,argv[3]);
        filesize=stol(filesizestr);
    }

    char msg[filesize];
    for(i=0;i<filesize;i++){
        msg[i]=(char)((rand()%26)+'a');
    }
    msg[i-1]='\0';

    snprintf(outFileName, 100, "perf_stats.csv"); 
    fp = fopen(outFileName, "r");
    if(fp == NULL){        
        fp = fopen(outFileName, "w");
        fprintf(fp, "File path, Exp num, File size, Open 1 time, Open 2 time, Open 3 time, Read 1 time, Read 2 time, Read 3 time, Close time\n");
    }    
    else{
        fp = fopen(outFileName, "a");
    }

    high_resolution_clock::time_point start, end;
    duration<double, std::micro> time_elapsed_1_open, time_elapsed_2_open, time_elapsed_3_open, 
        time_elapsed_1_read, time_elapsed_2_read, time_elapsed_3_read, time_elapsed_write, time_elapsed_close;
    
    cout<<"Creating file : "<<filepath<<endl;
    start = high_resolution_clock::now();
    int _open = open(filepath, O_RDWR);
    end = high_resolution_clock::now();
    time_elapsed_1_open = std::chrono::duration_cast<duration<double, std::micro>>(end - start);

    if(_open < 0) {
        printf("Error creating file!\n");
    }

    char buf[filesize];
    start = high_resolution_clock::now();
    read(_open, buf, filesize);
    end = high_resolution_clock::now();
    time_elapsed_1_read = std::chrono::duration_cast<duration<double, std::micro>>(end - start);

    // start = high_resolution_clock::now();
    // write(_open, msg, strlen(msg));
    // end = high_resolution_clock::now();
    // time_elapsed_write = std::chrono::duration_cast<duration<double, std::micro>>(end - start);

    start = high_resolution_clock::now();
    close(_open);
    end = high_resolution_clock::now();
    time_elapsed_close = std::chrono::duration_cast<duration<double, std::micro>>(end - start);

    start = high_resolution_clock::now();
    _open = open(filepath, O_RDWR);
    end = high_resolution_clock::now();
    time_elapsed_2_open = std::chrono::duration_cast<duration<double, std::micro>>(end - start);

    start = high_resolution_clock::now();
    read(_open, buf, filesize);
    end = high_resolution_clock::now();
    time_elapsed_2_read = std::chrono::duration_cast<duration<double, std::micro>>(end - start);
    close(_open);

    start = high_resolution_clock::now();
    _open = open(filepath, O_RDWR);
    end = high_resolution_clock::now();
    time_elapsed_3_open = std::chrono::duration_cast<duration<double, std::micro>>(end - start);

    start = high_resolution_clock::now();
    read(_open, buf, filesize);
    end = high_resolution_clock::now();
    time_elapsed_3_read = std::chrono::duration_cast<duration<double, std::micro>>(end - start);
    close(_open);
    
    fprintf(fp, "%s,%s,%f,%f,%f,%f,%f,%f,%f\n", filepath, expnum, time_elapsed_1_open.count()/MS, 
        time_elapsed_2_open.count()/MS, time_elapsed_3_open.count()/MS, time_elapsed_1_read.count()/MS, 
        time_elapsed_2_read.count()/MS, time_elapsed_3_read.count()/MS, time_elapsed_close.count()/MS);

    return 0;
}


