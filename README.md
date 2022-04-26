# AFS like FS

## Server
The source code is in myafs/server.cc

- rpc_fetch: Returns the data of the specified file or creates a new file, and returns it with its size.
- rpc_store: Stores a given data in the specified file.
- rpc_getFileStat: Returns the stats of the file specified.
- rpc_listDir: Returns the contents of the specified directory.
- rpc_rmdir: Removes the specified directory(if present)
- rpc_unlink: Removes a specified file(if present)
- rpc_mkdir:Creates a directory at the specified path


## Client
The source code is in myafs/client.cc
The client supports the following POSIX APIs:

- open()
- close()
- creat()
- unlink()
- mkdir()
- rmdir()
- read()
- write()
- pread()
- pwrite()
- stat()
- fsync()

 
## Build Steps
The project uses Cmake.

Building is handled via Cmake and needs it installed on the machine.

Once in the myafs folder <br/>

mkdir -p cmake/build
cd cmake/build 
cmake -DgRPC_INSTALL=ON  -DgRPC_BUILD_TESTS=OFF  -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR  ../..
make -j 20


## Run Steps
Server: myafs/server <server_data_directory>

Client: myafs/client -d <cache_directory> <mount_directory>

## Rubric

*Implementation Locations:*

* *1.1  POSIX Compliance:* client.cc
<br/><br/>

* *1.2 AFS Protocol and Semantics*

  * *Whole File Caching:*
    * myafs/client.cc: myafs_open() (Line 70), myafs_read() (line 153), myafs_write(Line 166), myafs_release (Line 260), myafs_create(Line 305), myafs_unlink (Line 339)
  * *Flush on close:*
    * myafs/client.cc: myafs_release() (Line 260), myafs_write(Line 166), client_helper.h: afsfuse_store()(Line 76)
  * *Last writer wins:*
    * myafs/client.cc: myafs_release() (Line 260)
  * *Stale Cache:*
    * myafs/client.cc: myafs_open() (Line 70)
<br/><br/>

* *1.3 Durability: Server Side Persistence* 
  * myafs/server.cc: rpc_store(Line 84)
<br/><br/>

* *1.4 Crash Recovery Protocol*
  * *Client Side Crash Recovery*
     * myafs/client.cc: myafs_init()(Line 412), createPendingFile()(Line 218), deletePendingFile()(Line 242), retryRelease(Line 366), myafs_release() (Line 260)
  * *Server Side Crash Recovery:*
    * myafs/server.cc: rpc_store(Line 84)
<br/><br/>

All other aspects covered in the report and the presentation.
Thank you!