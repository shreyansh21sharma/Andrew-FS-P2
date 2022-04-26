#define FUSE_USE_VERSION 31

#include <fstream>
#include <iostream>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <grpc++/grpc++.h>
#include <sys/stat.h>
#include "client_helper.h"
#include "afsgrpc.grpc.pb.h"
#include "dirent.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using afsgrpc::HelloRequest;
using afsgrpc::HelloReply; 
using afsgrpc::Greeter;

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/first_file";
static ClientHelper *ctx;

char fs_path[PATH_MAX];

static int myafs_getattr(const char *path, struct stat *stbuf,  struct fuse_file_info *fi)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));

        res = ctx->afsfuse_getFileStat(path, stbuf);

	if(res<0) {
            res = -ENOENT;
        }

        printf("FUSE Last Modification Time: %d\n", stbuf->st_mtime);

	return res;
}

static int myafs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{

        ctx->afsfuse_listDir(path, buf, filler);
	return 0;
}

unsigned long
hash(unsigned char *str)
{   
    unsigned long hash = 1329;
    int c;
    
    while (c = *str++)
        hash = ((hash << 5) + hash) + c;
    
    return hash;
}


static int myafs_open(const char *path, struct fuse_file_info *fi)
{
        char *buf;
        int size;
        int rc;
        int fd;
        int isStale = 0;
        int isFetched = 0;
        char cacheFileName[80];
        struct stat cacheFileInfo;
        struct stat remoteFileInfo;
        char cache_directory[PATH_MAX];
        cache_directory[0] = '\0';

        snprintf(cacheFileName, 80, "%lu", hash((unsigned char *)path));

        strncat(cache_directory, fs_path, PATH_MAX);
        strncat(cache_directory, cacheFileName, PATH_MAX);
        printf("path: %s\n", cache_directory);

        fd = open(cache_directory,   O_APPEND | O_RDWR);

        if(fd == -1) {
            printf("Open Return: %d\n", fd);

            int fetchcount = 5;
            while(fetchcount>0)
            {
                fetchcount--;
                printf("FetchCounnt: %d", fetchcount);
                rc = ctx->afsfuse_fetch(path, &buf, &size);
                if (rc>=0) {
                    break;
                }
            }

            isFetched = 1;

            fd = creat(cache_directory, S_IRWXU);
            if(fd==-1) {
                printf("Create Error\n");
                return -errno;
            }
            fd = open(cache_directory,  O_APPEND | O_RDWR);
            if(fd==-1) printf("Reopen Error\n"); 
        } else {

            lstat(cache_directory, &cacheFileInfo);
            myafs_getattr(path, &remoteFileInfo, NULL);

            if(remoteFileInfo.st_mtime > cacheFileInfo.st_mtime) {
                isStale = 1;
            }

            if(isStale) {
                rc = ftruncate(fd, 0);
                if(rc<0) {
                    return -errno;
                }
                rc = ctx->afsfuse_fetch(path, &buf, &size);
                if (rc<0) {
                    return -ENOENT;
                }
                isFetched = 1;
            }
        }

        printf("File descr: %d Size:%d\n", fd, size);


        if(isFetched) {
            write(fd, buf, size);
            printf("Check123");
            fsync(fd);
        }

        printf("File Contents: %s\n", buf);
//        fi->fh_old = 0;
        fi->fh = fd; 

	return 0;
}

static int myafs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
        int rc = 0;

        rc = pread(fi->fh, buf, size, offset);
        if(rc < 0) {
            return -errno;
        }

	return rc;
}

static int myafs_write(const char *path, const char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    int rc = 0;
    struct stat info;

    //if(offset==0)
    //{
        ftruncate(fi->fh, 0);
    //}

    printf("File closed: %d\n", fcntl(fi->fh, F_GETFD));
    printf("File closed err: %d\n", errno);
    printf("Write File descriptor: %d\n", fi->fh);
    printf("SAMPLE BUFFER PRINT: \n");
    for(int i=0; i<size; i++) {
            printf("%c", buf[i]);
        }
    rc= write(fi->fh, buf, size);
    fstat(fi->fh, &info);
    printf("\nWrite return: %d\n", info.st_mtime);
    if(rc < 0) {
        printf("Write Error: %d\n", errno);
        int fd;
        char cacheFileName[80];
        char cache_directory[PATH_MAX];
        cache_directory[0] = '\0';

        snprintf(cacheFileName, 80, "%lu", hash((unsigned char *)path));
        strncat(cache_directory, fs_path, PATH_MAX);
        strncat(cache_directory, cacheFileName, PATH_MAX);

        fd = open(cache_directory,  O_APPEND | O_RDWR);

        printf("Newdile fd: %d\n", fd);
        lseek(fd,offset,SEEK_SET);

        for(int i=0; i<size; i++) {
            printf("%c", buf[i]);
        }
        rc = write(fd, buf, size);
        close(fd);
        if(rc<0) {
            printf("Return error: %d\n", rc);
            printf("Reqrite error %d\n", errno);
            return -errno;
        }
    }

    return rc;
}

static void createPendingFile(const char *path, struct fuse_file_info *fi)
{
    int fd;
    char cacheFileName[80];
    char cache_directory[PATH_MAX];
    cache_directory[0] = '\0';

    snprintf(cacheFileName, 80, "%lu", hash((unsigned char *)path));
    strncat(cache_directory, fs_path, PATH_MAX);
    strncat(cache_directory, "pending/", PATH_MAX);
    strncat(cache_directory, cacheFileName, PATH_MAX);
    printf("pending path: %s\n", cache_directory);
    fd = open(cache_directory, O_CREAT | O_APPEND | O_RDWR, 0777);
    
    ftruncate(fd,0);
    write(fd,path,strlen(path));
    fsync(fd);
    if(fd == -1)
    {
        printf("Could not ensure creation of pending item\n");
    }
    close(fd);
}

static void deletePendingFile(const char *path)
{
    int fd;
    char cacheFileName[80];
    char cache_directory[PATH_MAX];
    cache_directory[0] = '\0';

    snprintf(cacheFileName, 80, "%lu", hash((unsigned char *)path));
    strncat(cache_directory, fs_path, PATH_MAX);
    strncat(cache_directory, "pending/", PATH_MAX);
    strncat(cache_directory, cacheFileName, PATH_MAX);
    printf("pending path: %s\n", cache_directory);
    if(remove(cache_directory)<0)
    {
        printf("Could not ensure deletion of pending item\n");
    }
}

static int myafs_release(const char *path, struct fuse_file_info *fi)
{
    int rc = 0;
    int isModified=1;
    char *buf;
    struct stat info;
    struct stat remoteFileInfo;

    createPendingFile(path, fi);
    fsync(fi->fh);

    //client crashes
    if(strcmp(path,"/clientcrash.txt")==0)
        int a=1/0;
    memset(&info, 0, sizeof(struct stat));
    fstat(fi->fh, &info);

    int successUpdate = 0;

    if(isModified) {
        buf = (char *)malloc(info.st_size);
        printf("hello");

        lseek(fi->fh, 0, SEEK_SET);

        read(fi->fh, buf, info.st_size);

        printf("To be sent: %s\n", buf, info.st_size);

        //printf("Crash simulation %d", 1/0);

        successUpdate = ctx->afsfuse_store(path, buf, info.st_size);

        printf("hello123");
        free(buf);
    }
    rc = close(fi->fh);

    //printf("Check");
    if(!(successUpdate<0))
      deletePendingFile(path);

    return rc;
}

static int myafs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
        int fd;
        char cacheFileName[80];
        char cache_directory[PATH_MAX];
        cache_directory[0] = '\0';
    
        snprintf(cacheFileName, 80, "%lu", hash((unsigned char *)path));
        strncat(cache_directory, fs_path, PATH_MAX);
        strncat(cache_directory, cacheFileName, PATH_MAX);
        printf("path: %s\n", cache_directory);
        fflush(stdout);
        fd = open(cache_directory, O_CREAT | O_APPEND | O_RDWR, mode );
        printf("Done\n");
        if (fd == -1) {
                printf("Create Error\n");
                return -errno;
        }

        fi->fh = fd;

        ctx->afsfuse_store(path, NULL, 0);

        printf("Create file descr: %d\n", fi->fh);
        return 0;
}

//CAN REMOVE

static int myafs_rmdir(const char *path)
{
    return ctx->afsfuse_rmdir(path);
}

static int myafs_unlink(const char *path)
{
    //Remove from cache if present
    int fd;
    char cacheFileName[80];
    char cache_directory[PATH_MAX];
    cache_directory[0] = '\0';
    
    snprintf(cacheFileName, 80, "%lu", hash((unsigned char *)path));
    strncat(cache_directory, fs_path, PATH_MAX);
    strncat(cache_directory, cacheFileName, PATH_MAX);
    printf("path: %s\n", cache_directory);

    struct stat buffer;   
    if(stat (cache_directory, &buffer) == 0) //check if file exists
    {
        remove(cache_directory);   //it is ok even if it fails to remove local cache
    }

    return ctx->afsfuse_unlink(path);
}

static int myafs_mkdir(const char *path, mode_t mode)
{
    return ctx->afsfuse_mkdir(path, mode);
}

void retryRelease(const char* cache_directory, const char* fileName)
{
    std::string pathname;
    std::ifstream input(std::string(cache_directory)+"pending/"+std::string(fileName));
    std::getline(input, pathname);
    printf("Pathname: %s\n", pathname);

    struct stat info;
    struct stat cacheFileInfo;
    struct stat remoteFileInfo;
    memset(&info, 0, sizeof(struct stat));
    char *buf;

    buf = (char *)malloc(info.st_size);
    int fd;

    char thisPath[PATH_MAX];

    thisPath[0] = '\0';
    strncat(thisPath, cache_directory, PATH_MAX);
    strncat(thisPath, fileName, PATH_MAX);

    fd = open(thisPath, O_CREAT | O_APPEND | O_RDWR, 0777);
    fstat(fd, &info);

    lseek(fd, 0, SEEK_SET);
    
    read(fd, buf, info.st_size);
    printf("hello: %s",buf);

    lstat(thisPath, &cacheFileInfo);
    myafs_getattr(pathname.c_str(), &remoteFileInfo, NULL);

    //if(remoteFileInfo.st_mtime < cacheFileInfo.st_mtime)
    //{
        ctx->afsfuse_store(pathname, buf, info.st_size);
    //}
    printf("To be sent: %s\n", buf, info.st_size);

    //printf("Crash simulation %d", 1/0);

    deletePendingFile(pathname.c_str());

    free(buf);
}

void *myafs_init(struct fuse_conn_info *conn,
		      struct fuse_config *cfg)
{
    int fd;
    char pending_path[PATH_MAX];
    pending_path[0] = '\0';

    strncat(pending_path, fs_path, PATH_MAX);
    strncat(pending_path, "pending", PATH_MAX);
    mkdir(pending_path, 0777);

    char cache_directory[PATH_MAX];
    cache_directory[0] = '\0';
    strncat(cache_directory, fs_path, PATH_MAX);

    //goes through all pending. Reads first line as path & saves those files

    DIR *dir;
    dir = opendir(pending_path);
    if (dir) {
    while (auto f = readdir(dir)) {
        if (!f->d_name || f->d_name[0] == '.')
            continue; // Skip everything that starts with a dot
        
        printf("File: %s\n", f->d_name);
        retryRelease(cache_directory, f->d_name);
        }
    closedir(dir);
    }

}

static int myafs_fsync(const char *path, int, struct fuse_file_info *fi)
{
    return fsync(fi->fh);
}

struct myafs_fuse_operations:fuse_operations
{
    myafs_fuse_operations ()
    {
        getattr    = myafs_getattr;
        readdir    = myafs_readdir;
        open       = myafs_open;
        read       = myafs_read;
        write      = myafs_write;
        create     = myafs_create;
        //flush      = myafs_release;
        //Consciously choosing release over flush
        release    = myafs_release;
        rmdir      = myafs_rmdir;
        unlink     = myafs_unlink;
        mkdir      = myafs_mkdir;
        fsync       = myafs_fsync;
        init       = myafs_init;
    }
};

static struct myafs_fuse_operations myafs_oper;

int main(int argc, char *argv[])
{
        ClientHelper greeter(
      grpc::CreateChannel("20.122.11.232:50051", grpc::InsecureChannelCredentials()));

        ctx = &greeter;

        strncpy(fs_path, realpath(argv[argc-2], NULL), PATH_MAX);
        strncat(fs_path, "/", PATH_MAX);
        argv[argc-2] = argv[argc-1];
        argv[argc-1] = NULL;
        argc--;

        printf("FS PATH: %s\n", fs_path);

	return fuse_main(argc, argv, &myafs_oper, NULL);
}