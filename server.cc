#include <iostream>
#include <memory>
#include <string>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <grpc++/grpc++.h>

#include "afsgrpc.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using afsgrpc::HelloRequest;
using afsgrpc::FetchRequest;
using afsgrpc::FetchReply;
using afsgrpc::StoreRequest;
using afsgrpc::StoreReply;
using afsgrpc::StatRequest;
using afsgrpc::StatReply;
using afsgrpc::ListDirRequest;
using afsgrpc::ListDirReply;
using afsgrpc::HelloReply;
using afsgrpc::Greeter;
using afsgrpc::OutputInfo;
using afsgrpc::String;
using afsgrpc::MkdirRequest;

char server_directory[PATH_MAX];
int NEW_MAX_MESSAGE_SIZE = 1024 * 1024 * 1024 ; //1024MB
char crashCase[PATH_MAX];

class GreeterServiceImpl final : public Greeter::Service {
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }

  Status rpc_fetch(ServerContext* context, const FetchRequest* request,
               FetchReply* reply) override {

    int fd;

    char path[PATH_MAX];
    path[0] = '\0';
    struct stat info;

    strncat(path, server_directory, PATH_MAX);
    strncat(path, (request->path()).c_str(), PATH_MAX);

    printf("AFS PATH: %s\n", path);

    fd = open(path, O_RDONLY);

    if(fd == -1) {
        reply->set_error(-1);
        return Status::OK;
    }

    fstat(fd, &info);

    char *buf = (char *)malloc(info.st_size);

    lseek(fd, 0, SEEK_SET);
    read(fd, buf, info.st_size);
    close(fd);

    printf("Read string: %s\n", buf);

    reply->set_error(0);
    reply->set_buf(std::string(buf,info.st_size));
    reply->set_size(info.st_size);
    return Status::OK;
    
  }

  Status rpc_store(ServerContext* context, const StoreRequest* request,
               StoreReply* reply) override {

      int fd;

      char path[PATH_MAX];
      path[0] = '\0';

      char tempFilePath[PATH_MAX];
      tempFilePath[0]='\0';

      strncat(path, server_directory, PATH_MAX);
      strncat(path, (request->path()).c_str(), PATH_MAX);

      strncat(tempFilePath, path, PATH_MAX);
      strncat(tempFilePath, ".tmp", PATH_MAX);

      remove(tempFilePath);

      printf("AFS PATH: %s\n", path);

      fd = open(tempFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);

      if(fd == -1) {
          reply->set_error(-1);
          return Status::OK;
      }

      printf("Received String: %s\n", (request->buf()).c_str());
      printf("Size: %d\n", request->size());
      write(fd, (request->buf()).data(), request->size());
      close(fd);

      if(strcmp(crashCase,"crashAfterClose")==0)
      {
          int a = 1/0;
      }

      rename(tempFilePath, path);
      reply->set_error(0);
      return Status::OK;
  }

  Status rpc_getFileStat(ServerContext* context, const StatRequest* request,
                     StatReply* reply) override {

      int rc = 0;
      struct stat *stbuf;
      char path[PATH_MAX];
      path[0] = '\0';

      strncat(path, server_directory, PATH_MAX);
      strncat(path, (request->path()).c_str(), PATH_MAX);

      stbuf = (struct stat *)malloc(sizeof(struct stat));

      rc = lstat(path, stbuf);

      if(rc !=0 ) {
          reply->set_error(-1);
          free(stbuf);
          return Status::OK;
      }

      printf("Last Modification Time: %d\n", stbuf->st_mtime);
      reply->set_error(0);
      reply->set_buf(std::string((char *)stbuf,sizeof(struct stat)));
      return Status::OK;
  }

  Status rpc_listDir(ServerContext* context,
                 const ListDirRequest *request,
                 ServerWriter<ListDirReply>* writer) override {

      char path[PATH_MAX];
      path[0] = '\0';

      strncat(path, server_directory, PATH_MAX);
      strncat(path, (request->path()).c_str(), PATH_MAX);

      DIR *dp;
      struct dirent *de;
      ListDirReply reply;

      dp = opendir(path);
      if (dp == NULL) {
          reply.set_error(-1);
          writer->Write(reply);
          return Status::OK;
      }

      de = readdir(dp);
      if (de == 0) {
          reply.set_error(-1);
          writer->Write(reply);
          return Status::OK;
      }


      do {
          reply.set_error(0);
          reply.set_name(std::string(de->d_name));
          writer->Write(reply);
      } while ((de = readdir(dp)) != NULL);

      return Status::OK;
  }

  Status rpc_rmdir(ServerContext* context, const String* input,
                                         OutputInfo* reply) override {

        char path[PATH_MAX];
        path[0] = '\0';

        strncat(path, server_directory, PATH_MAX);
        strncat(path, (input->str()).c_str(), PATH_MAX);

        int res = rmdir(path);

        if (res == -1) {
            perror(strerror(errno));
            reply->set_err(errno);
            return Status::OK;
        } else {
            reply->set_err(0);
        }

        return Status::OK;
    }

    Status rpc_unlink(ServerContext* context, const String* input,
                                         OutputInfo* reply) override {

        char path[PATH_MAX];
        path[0] = '\0';
        strncat(path, server_directory, PATH_MAX);
        strncat(path, (input->str()).c_str(), PATH_MAX);
        int res = unlink(path);
        if (res == -1) {
            perror(strerror(errno));
            reply->set_err(errno);
            return Status::OK;
        } else {
            reply->set_err(0);
        }
        return Status::OK;
    }

    Status rpc_mkdir(ServerContext* context, const MkdirRequest* input,
                                         OutputInfo* reply) override {		
        char path[PATH_MAX];
        path[0] = '\0';
        strncat(path, server_directory, PATH_MAX);
        strncat(path, (input->s()).c_str(), PATH_MAX);
        int res = mkdir(path, input->mode());


        if (res == -1) {
            perror(strerror(errno)); 
            reply->set_err(errno);
            return Status::OK;
        } else {
            reply->set_err(0);
        }

        return Status::OK;
    }

};

void RunServer() {
  std::string server_address("localhost:50051");
  GreeterServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.SetMaxReceiveMessageSize(NEW_MAX_MESSAGE_SIZE);
  builder.SetMaxSendMessageSize(NEW_MAX_MESSAGE_SIZE);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server successfully initialized on on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {

  if(argc!=3) {
      std::cout << "No path given! \nUsage: server <server_directory>" << std::endl;
      return -1;
  }

  strncpy(server_directory, argv[1], PATH_MAX);

  strncpy(crashCase, argv[2], PATH_MAX);

  std::cout << server_directory << std::endl;

  RunServer();

  return 0;
}