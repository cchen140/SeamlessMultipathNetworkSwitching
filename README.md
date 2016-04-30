# Seamless Multipath Network Switching for Live Video Streaming Applications
Multipath is the future.

# Build and Run
Once in the source code folder, run the following commands:
```
cd build
cmake . && make
./VideoConsumer 10000 &
./VideoProvider 127.0.0.1 10000
```
