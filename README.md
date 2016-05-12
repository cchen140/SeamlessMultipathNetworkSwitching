# SmartStream: A Seamless Multipath Network Switching Protocol for Live Video Streaming Applications

# Virtual Webcam Setup
1. Assume your host is Windows and testing envrionemnt is Ubuntu VM
2. In the host, download and install Magic Camera from http://www.shiningmorning.com/
3. In the host, download and install multimedia code from http://www.codecguide.com/download_k-lite_codec_pack_basic.htm
4. In the host, download and unzip the video demos from https://drive.google.com/open?id=0B2nbvtY-PJ1rejlkR1JNTkFnUkE
5. In the host, start Magic Camera and play an example video
6. Start your VM
7. In virtual box setting of the VM, select Devices > WebCams > Magic Camera Capture
8. Inside the VM, install the cheese to check the web camera can be seen inside the VM
```   
sudo apt-get install cheese
cheese
```

You should see the cheese program in your VM shows the demo video

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
