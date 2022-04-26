from threading import Thread
import os
import datetime
client=16

mountdir="/home/afsproj/grpc/examples/cpp/afs-test/cmake/build/tmp/fuse"
buff = ""
for i in range(1024*1024):
    buff=buff+"a"

def task(i):
    temp = open(mountdir+str(i),"w+")
    t1=datetime.datetime.now()
    temp.write(buff)
    t2=datetime.datetime.now()
    temp.close()

if __name__ == "__main__":
	threads=[]
	for i in range(client):
    	    threads.append(Thread(target=task,args=[i]))

	for thread in threads:
	    thread.start()
	for thread in threads:
	    thread.join()