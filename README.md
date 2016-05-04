# Seamless Multipath Network Switching for Live Video Streaming Applications
Multipath is the future.

# Environment Setup
1. This program uses OpenCV library. Please follow the steps given in "InstallingOpenCV.txt" to set the OpenCV up.
2. It also uses ncurses library (a Linux version of windows's conio.h) for getting key press.
Please run the following command to install the ncurses libaray:
```
sudo apt-get install libncurses-dev
```

# Build
Once in the source code folder, run the following commands:
```
cd build
cmake . && make
```
To clean the build, just delete all the files in the "build" folder except "CMakeList.txt" file.

# Run
To run the program, use the following commands and arguements.
```
cd build
./VideoProvider <client's primary IP> <client's secondary IP> <client's port>
./VideoConsumer <client's primary IP> <client's secondary IP> <client's port>
```
An example with the client having IP addresses 192.168.1.5 and 192.168.1.6 as primary and secondary network interfaces is as follows:
```
cd build
./VideoProvider 192.168.1.5 192.168.1.6 10000
./VideoConsumer 192.168.1.5 192.168.1.6 10000
```
