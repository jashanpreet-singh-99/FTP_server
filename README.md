# FTP_server

This repository contains code for a server-client application that performs various file operations. The code is written in C language and utilizes socket programming to establish communication between the server and client.

## Prerequisites

To compile and run this code, you need to have the following:

* C compiler (GCC recommended)
* Linux operating system

## Getting Started

1. Clone the repository to your local machine.
```
git clone https://github.com/your-username/repository-name.git
```
2. Navigate to the project directory.
```
cd repository-name
```
3. Compile the code using the C compiler.
```
gcc server.c -o server
gcc client.c -o client
```
4. Run the server and client applications.
```
./server
./client
```

## Usage

The client application provides the following commands for interacting with the server:

* `quit`: Close the connection with the server.
* `findfile <filename>`: Find information about a file on the server.
* `sgetfiles <size1> <size2>`: Retrieve files within a specific size range.
* `dgetfiles <date1> <date2>`: Retrieve files modified within a specific date range.
* `getfiles <filenames>`: Retrieve all files on the server.
* `gettargz <extension-list>`: Retrieve all files with given extension on the server as a compressed tar archive.

## Command Examples

* To find information about a file named "example.txt":
```
findfile example.txt
```
* To retrieve files within a size range (e.g., 100 bytes to 500 bytes):
```
sgetfiles 100 500
```
* To retrieve files modified within a date range (e.g., 2023-01-01 to 2023-05-01):
```
dgetfiles 2023-01-01 2023-05-01
```
* To retrieve all files on the server:
```
getfiles sample.txt sam.txt resule.pdf -z
```
* To retrieve all files on the server as a compressed tar archive:
```
gettargz pdf c o
```
## License

This project is licensed under the MIT License.

## Acknowledgments

* [GitHub](https://github.com) for hosting repositories and collaboration.
* [Stack Overflow](https://stackoverflow.com) for the invaluable community support.
