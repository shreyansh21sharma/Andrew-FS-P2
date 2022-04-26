#define FUSE_USE_VERSION 31

#include <iostream>
#include <memory>
#include <string>
#include <time.h>
#include <grpc++/grpc++.h>
#include <sys/stat.h>
#include <fuse.h>
#include "afsgrpc.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using afsgrpc::HelloRequest;
using afsgrpc::HelloReply;
using afsgrpc::FetchRequest;
using afsgrpc::FetchReply;
using afsgrpc::StoreRequest;
using afsgrpc::StoreReply;
using afsgrpc::StatRequest;
using afsgrpc::StatReply;
using afsgrpc::ListDirRequest;
using afsgrpc::ListDirReply;
using afsgrpc::Greeter;
using afsgrpc::OutputInfo;
using afsgrpc::String;
using afsgrpc::MkdirRequest;

class ClientHelper{

public:
ClientHelper(std::shared_ptr<Channel> channel)
	: stub_(Greeter::NewStub(channel)) {}

std::string SayHello(const std::string& user) {
		HelloRequest request;
		request.set_name(user);
		HelloReply reply;
		ClientContext context;
		Status status = stub_->SayHello(&context, request, &reply);

		if (status.ok()) {
			return reply.message();
		} else {
			return "RPC failed";
		}
	}


int afsfuse_fetch (const std::string& path, char **buf, int *size) {
	FetchRequest request;
	request.set_path(path);


	FetchReply *reply = new FetchReply();

	ClientContext context;

	Status status = stub_->rpc_fetch(&context, request, reply);

	if (status.ok()) {
                std::cout << reply->buf() <<std::endl;
		*buf = (char *)(reply->buf()).data();
                printf("%s\n", *buf);
		*size = reply->size();
		return 0;
	} else {
        printf("RPC failed");
		return -1;
	}
}


int afsfuse_store (const std::string& path, char *buf, int size) {
	StoreRequest request;
	request.set_path(path);
	request.set_size(size);
	request.set_buf(std::string(buf, size));

	StoreReply reply;

	ClientContext context;

	Status status = stub_->rpc_store(&context, request, &reply);

	if (status.ok()) {
		return reply.error();
	} else {
        printf("RPC failed");
		return -1;
	}
}

int afsfuse_getFileStat(const std::string& path, struct stat *stbuf) {
	StatRequest request;
	request.set_path(path);

	StatReply reply;

	ClientContext context;

	Status status = stub_->rpc_getFileStat(&context, request, &reply);

	if (status.ok()) {

                if(reply.error()<0) {
                    return -1;
                } 

		memcpy(stbuf,(struct stat *)(reply.buf()).data(), sizeof(struct stat));
		printf("gRPC: Mod time: %d\n", stbuf->st_mtime);
		return 0;
	} else {
        printf("RPC failed");
		return -1;
	}
}

int afsfuse_listDir(const std::string& path, void *buf, fuse_fill_dir_t filler) {
        ListDirRequest request;
        request.set_path(path);

        ListDirReply reply;

        ClientContext context;

        std::unique_ptr<ClientReader<ListDirReply> > reader(
            stub_->rpc_listDir(&context, request));

        while (reader->Read(&reply)) {
            if(reply.error()==0) {
                filler(buf, (reply.name()).c_str(), NULL, 0, static_cast<fuse_fill_dir_flags>(0));
            }
        }

        Status status = reader->Finish();

        if (status.ok()) {
            return 0;
        } else {
            printf("RPC failed");
            return -1;
        }
}

int afsfuse_rmdir(std::string path) {
      String input;
      ClientContext context;
      input.set_str(path);
      OutputInfo result;

      Status status = stub_->rpc_rmdir(&context, input, &result);

      if (result.err() != 0) {
          printf("error: afsfuse_rmdir() fails");
      }
      return -result.err();
  }

  int afsfuse_unlink(std::string path) {
      String input;
      ClientContext context;
      input.set_str(path);
      OutputInfo result;

      Status status = stub_->rpc_unlink(&context, input, &result);
      if (result.err() != 0) {
          printf("error: afsfuse_rmdir() fails");
      }
      return -result.err();
  }

  int afsfuse_mkdir(std::string path, mode_t mode){
      MkdirRequest input;
      ClientContext context;
      input.set_s(path);
      input.set_mode(mode);
      OutputInfo result;

      Status status = stub_->rpc_mkdir(&context, input, &result);

      if(!status.ok())
      {
          return -1;
      }
      if (result.err() != 0) {
          printf("error: afsfuse_mkdir() fails");
      }

      return -result.err();
  }

private:
  std::unique_ptr<Greeter::Stub> stub_;

};