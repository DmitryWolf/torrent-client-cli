### Torrent Client
A fully working torrent client written in C++ for experience with system calls, networks, multithreading and practice in C++.
### Dependencies
To build the project, you will need the CMake build system, as well as the OpenSSL and libcurl libraries installed in the system.

- How to install dependencies on Ubuntu 20.04:
```
$ sudo apt-get install cmake libcurl4-openssl-dev libssl-dev
```
- How to install dependencies on Mac OS via the brew package manager (https://brew.sh/):
```
$ brew install openssl cmake
```
### Usage
To launch the torrent client, use this command:
```
$ make
$ ./cmake-build/torrent-client-prototype -d <path to the directory to save the downloaded file> -p <how many percent of the file should be downloaded> <path to the torrent file>
```
To test the client, you can check the downloaded data for similarity with the data downloaded through another torrent client:
```
$ python3 checker.py <path to the first directory> <path to the second directory>
```
This checker compares byte by byte all files with the same names.
