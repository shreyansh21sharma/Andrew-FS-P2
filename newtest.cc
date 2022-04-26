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
#include <fstream>
#include <vector>

using namespace std;
using std::chrono::duration;
using std::chrono::high_resolution_clock;

double avg (vector<double> const& v ) {
    double return_value = 0.0;
    int n = v.size();
    
    for ( int i=0; i < n; i++) {
        return_value += v[i];
    }
    return ( return_value / (n));
}

vector<double> perfTest(const char *path, const char *cache_path) {
    vector<double> openResultFirst(100, 0);
    vector<double> readResultFirst(100, 0);
    vector<double> closeResultFirst(100, 0);
    vector<double> openResult(100, 0);
    vector<double> readResult(100, 0);
    vector<double> closeResult(100, 0);
    vector<double> result(6, 0);

for(int i = 0; i < 1; i++) {
        // OPEN
        high_resolution_clock::time_point start, end;
        duration<double, std::micro> time_elapsed_open, time_elapsed_read, time_elapsed_write, time_elapsed_close;
        
        start = high_resolution_clock::now();
        int _open = open(path, O_RDONLY);
        end = high_resolution_clock::now();
        time_elapsed_open = std::chrono::duration_cast<duration<double, std::micro>>(end - start);
        openResultFirst[i] = time_elapsed_open.count();

        if(_open < 0) {
            cout << "Error creating file" << endl;
        }

        // READ
        start = high_resolution_clock::now();
        lseek(_open, 0, SEEK_SET);
        size_t totalSize = lseek(_open, 0, SEEK_END);
        lseek(_open, 0, SEEK_SET);

        off_t currentOffset = 0;
        std::string buf;

        do {
            buf.resize(totalSize);
            size_t readSize = pread(_open, &buf[0], totalSize, currentOffset);
            if (readSize == -1) {
                cout << "Read failed" << endl;
            }
            currentOffset += totalSize;
        } while (currentOffset < totalSize);

        end = high_resolution_clock::now();
        time_elapsed_read = std::chrono::duration_cast<duration<double, std::micro>>(end - start);
        readResultFirst[i] = time_elapsed_read.count();


        // CLOSE
        start = high_resolution_clock::now();
        close(_open);
        end = high_resolution_clock::now();
        time_elapsed_close = std::chrono::duration_cast<duration<double, std::micro>>(end - start);
        closeResultFirst[i] = time_elapsed_close.count();
        //unlink(cache_path);
    }

    for(int i = 0; i < 1; i++) {
        // OPEN
        high_resolution_clock::time_point start, end;
        duration<double, std::micro> time_elapsed_open, time_elapsed_read, time_elapsed_write, time_elapsed_close;
        
        start = high_resolution_clock::now();
        int _open = open(path, O_RDONLY);
        end = high_resolution_clock::now();
        time_elapsed_open = std::chrono::duration_cast<duration<double, std::micro>>(end - start);
        openResult[i] = time_elapsed_open.count();

        if(_open < 0) {
            cout << "Error creating file" << endl;
        }

        // READ
        start = high_resolution_clock::now();
        lseek(_open, 0, SEEK_SET);
        size_t totalSize = lseek(_open, 0, SEEK_END);
        lseek(_open, 0, SEEK_SET);

        off_t currentOffset = 0;
        std::string buf;

        do {
            buf.resize(totalSize);
            size_t readSize = pread(_open, &buf[0], totalSize, currentOffset);
            if (readSize == -1) {
                cout << "Read failed" << endl;
            }
            currentOffset += totalSize;
        } while (currentOffset < totalSize);

        end = high_resolution_clock::now();
        time_elapsed_read = std::chrono::duration_cast<duration<double, std::micro>>(end - start);
        readResult[i] = time_elapsed_read.count();


        // CLOSE
        start = high_resolution_clock::now();
        close(_open);
        end = high_resolution_clock::now();
        time_elapsed_close = std::chrono::duration_cast<duration<double, std::micro>>(end - start);
        closeResult[i] = time_elapsed_close.count();
    }

    result[0] = avg(openResultFirst);
    result[1] = avg(openResult);
    cout << "PATH: " << path << endl;
    cout << "OPEN FIRST: " << result[0] << " OPEN AVERAGE: " << result[1] << endl;
    
    result[2] = avg(readResultFirst);
    result[3] = avg(readResult);
    cout << "READ FIRST: " << result[2] << " READ AVERAGE: " << result[3] << endl;

    result[4] = avg(closeResultFirst);
    result[5] = avg(closeResult);
    cout << "CLOSE FIRST: " << result[4] << " CLOSE AVERAGE: " << result[5] << endl;
    return result;
}

int main(int argc, char** argv) {   
    vector<double> result(6, 0);

    //result = perfTest("/home/afsproj/grpc/examples/cpp/afs-grpc-newchanges/cmake/build/tmp/fuse/fuse1-mb", "/home/afsproj/clientcache/1-mb.out");
    result = perfTest("/home/afsproj/grpc/examples/cpp/afs-grpc-newchanges/cmake/build/tmp/fuse/fuse32-mb.txt", "/home/afsproj/clientcache/8-mb.out");
    //result = perfTest("/home/afsproj/grpc/examples/cpp/afs-grpc-newchanges/cmake/build/tmp/fuse/fuse16-mb", "/home/afsproj/clientcache/16-mb.out");
    // result = perfTest("/home/afsproj/grpc/examples/cpp/afs-grpc-newchanges/cmake/build/tmp/fuse/fuse32-mb.out", "/home/afsproj/clientcache/32-mb.out");
    // result = perfTest("/home/afsproj/grpc/examples/cpp/afs-grpc-newchanges/cmake/build/tmp/fuse/fuse64-mb.out", "/home/afsproj/clientcache/64-mb.out");
    // result = perfTest("/home/afsproj/grpc/examples/cpp/afs-grpc-newchanges/cmake/build/tmp/fuse/fuse128-mb.out", "/home/afsproj/clientcache/128-mb.out");
    // result = perfTest("/home/afsproj/grpc/examples/cpp/afs-grpc-newchanges/cmake/build/tmp/fuse/fuse256-mb.out", "/home/afsproj/clientcache/256-mb.out");
    // result = perfTest("/home/afsproj/grpc/examples/cpp/afs-grpc-newchanges/cmake/build/tmp/fuse/fuse512-mb.out", "/home/afsproj/clientcache/512-mb.out");
    // result = perfTest("/home/afsproj/grpc/examples/cpp/afs-grpc-newchanges/cmake/build/tmp/fuse/fuse1024-mb.out", "/home/afsproj/clientcache/1024-mb.out");
    return 0;
}