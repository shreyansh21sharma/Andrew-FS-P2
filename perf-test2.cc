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

int main(int argc, char** argv) {
    FILE *fp;
    char outFileName[100];
    snprintf(outFileName, 100, "perf_cli_stats.csv"); 
    fp = fopen(outFileName, "r");
    if(fp == NULL){        
        fp = fopen(outFileName, "w");
        fprintf(fp, "Open time, Read time, Write time, Close time\n");
    }    
    else{
        fp = fopen(outFileName, "a");
    }

    high_resolution_clock::time_point start, end;
    duration<double, std::nano> time_elapsed_open, time_elapsed_read, time_elapsed_write, time_elapsed_close;
    
    start = high_resolution_clock::now();
    int _open = creat("/home/afsproj/grpc/examples/cpp/afs-grpc-1/cmake/build/tmp/fuse/test.txt", O_RDWR | O_APPEND);
    end = high_resolution_clock::now();
    time_elapsed_open = std::chrono::duration_cast<duration<double, std::nano>>(end - start);

    if(_open < 0) {
        printf("Error creating file!\n");
    }

    char buf[100];
    start = high_resolution_clock::now();
    read(_open, buf, 100);
    end = high_resolution_clock::now();
    time_elapsed_read = std::chrono::duration_cast<duration<double, std::nano>>(end - start);
    
    start = high_resolution_clock::now();
    write(_open, "hello world", strlen("hello world")+1);
    end = high_resolution_clock::now();
    time_elapsed_write = std::chrono::duration_cast<duration<double, std::nano>>(end - start);

    start = high_resolution_clock::now();
    close(_open);
    end = high_resolution_clock::now();
    time_elapsed_close = std::chrono::duration_cast<duration<double, std::nano>>(end - start);
    
    fprintf(fp, "%f,%f,%f,%f\n", time_elapsed_open.count(), time_elapsed_read.count(), 
            time_elapsed_write.count(), time_elapsed_close.count());

    return 0;
}