# Assumptions:

1 . We visualize each Storage Server as seperate Physical Entities, rather than them being executable running on any folder. So Due to this, each Storage Server is different from another, based on Ports.

2 . There can be any number of storage server and clients, but it needs to be specified by the Naming Server.

3 . Write Command means that we are writing to the file rather than appending to it.

4 . Redundancy happens every 120 seconds for Each Storage Server. During this time, the Client can issue 
commands but it wont get processed immediately.

5 . Every Storage Server, has two other fixed Storage Servers where their data is Redundantly Stored

6 . All Paths need to be absolute and not relative.

7 . We have made a Distributed System, Meaning the same directory can be stored across servers, and not on one standalone server only.

8 . no backslash ./ required before issuing any filepath

9 . create before read/write/info needs to be done

# Commands Created:

1 . read path \
2 . write path \
3 . createdir path\
4 . createfile path\
5 . deletedir path\
6 . deletefile path\
7 . copydir path-1 path-2\
8 . copyfile path-1 path-2\
9 . ls \
10 . ls path\
11 . clear\
12 . info

# Contributions:

### Aaditya -
Intialization of All Servers, Accepting Client Requests, Adding new storage servers, create functionality, read, write , info, concurrency, Redundancy/Replication , concurrent file reading
### Akshat -
Error Handling, Cache
### Annamalai - 
copy functionality, delete functionality, ls functionality, Efficient Search ( Tries ), Debugging and testing , 
