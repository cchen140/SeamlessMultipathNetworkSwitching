/*
 *   C++ UDP socket server for live image upstreaming
 *   Modified from http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/practical/UDPEchoServer.cpp
 *   Copyright (C) 2015
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PracticalSocket.h" // For UDPSocket and SocketException
#include <iostream>          // For cout and cerr
#include <cstdlib>           // For atoi()

/* Added by CY */
#include <sys/poll.h>        // http://beej.us/guide/bgnet/output/html/multipage/pollman.html

#define BUF_LEN 65540 // Larger than maximum UDP packet size
#define TOTAL_PACK  30  // This value was dynamically configured in run-time with a range around 20 to 21 on my Surface. Set to 30 here to keep some room.

#include "opencv2/opencv.hpp"
using namespace cv;
#include "config.h"

int main(int argc, char * argv[]) {

    if (argc != 2) { // Test for correct number of parameters
        cerr << "Usage: " << argv[0] << " <Server Port>" << endl;
        exit(1);
    }

    unsigned short servPort1 = atoi(argv[1]); // First arg:  local port
    unsigned short servPort2 = servPort1+1;

    namedWindow("recv", CV_WINDOW_AUTOSIZE);
    try {
        UDPSocket sock1(servPort1);
        UDPSocket sock2(servPort2);

        //char buffer[BUF_LEN]; // Buffer for echo string
        
        int recvMsgSize; // Size of received message
        string sourceAddress; // Address of datagram source
        unsigned short sourcePort; // Port of datagram source

        clock_t last_cycle = clock();

        int sock1State = 0;
        int sock1TotoalPack = 0;
        int sock1PackCount = 0;
        char buffer1[BUF_LEN]; // Buffer for echo string
        char * longbuf1 = new char[PACK_SIZE * TOTAL_PACK];
        int sock2State = 0;
        int sock2TotoalPack = 0;
        int sock2PackCount = 0;
        char buffer2[BUF_LEN]; // Buffer for echo string
        char * longbuf2 = new char[PACK_SIZE * TOTAL_PACK];
        while (1) {
            int rv;
            struct pollfd ufds[2];
            ufds[0].fd = sock1.getDescriptor();
            ufds[0].events = POLLIN; // check for just normal data

            ufds[1].fd = sock2.getDescriptor();
            ufds[1].events = POLLIN; // check for just normal data
          
            rv = poll(ufds, 2, 100);    // The third parameter is timeout in minisecond.
            
            if (rv == -1) {
                perror("poll"); // error occurred in poll()
            } else if (rv == 0) {
                //printf("Timeout occurred!  No data after 100 miniseconds.\n");
                continue;
            }
            
            // check for events on s1:
            if (ufds[0].revents & POLLIN) {
              if (sock1State == 0) {
                recvMsgSize = sock1.recvFrom(buffer1, BUF_LEN, sourceAddress, sourcePort);
                if (recvMsgSize > sizeof(int)) {
                  ;
                } else {
                  sock1TotoalPack = ((int * ) buffer1)[0];
                  sock1State = 1;
                  sock1PackCount = 0;
                }
                
              } else if (sock1State == 1) {
                recvMsgSize = sock1.recvFrom(buffer1, BUF_LEN, sourceAddress, sourcePort);
                if (recvMsgSize != PACK_SIZE) {
                  cerr << "Received unexpected size pack:" << recvMsgSize << endl;
                  //continue;
                  sock1State = 0; // Something wrong happens, so reset the state.
                } else {
                  memcpy( & longbuf1[sock1PackCount * PACK_SIZE], buffer1, PACK_SIZE);
                  sock1PackCount++;
                  if (sock1PackCount == sock1TotoalPack) {
                    // One frame data is complete.
                    sock1State = 2;
                  }
                }      
              }    
            }
            
            // check for events on s2:
            if (ufds[1].revents & POLLIN) {
              if (sock2State == 0) {
                recvMsgSize = sock2.recvFrom(buffer2, BUF_LEN, sourceAddress, sourcePort);
                if (recvMsgSize > sizeof(int)) {
                  ;
                } else {
                  sock2TotoalPack = ((int * ) buffer2)[0];
                  sock2State = 1;
                  sock2PackCount = 0;
                }
                
              } else if (sock2State == 1) {
                recvMsgSize = sock2.recvFrom(buffer2, BUF_LEN, sourceAddress, sourcePort);
                if (recvMsgSize != PACK_SIZE) {
                  cerr << "Received unexpected size pack:" << recvMsgSize << endl;
                  //continue;
                  sock2State = 0; // Something wrong happens, so reset the state.
                } else {
                  memcpy( & longbuf2[sock2PackCount * PACK_SIZE], buffer2, PACK_SIZE);
                  sock2PackCount++;
                  if (sock2PackCount == sock2TotoalPack) {
                    // One frame data is complete.
                    sock2State = 2;
                  }
                }              
              }           
            }
   
 
            if ( (sock1State==2) || (sock2State==2) ) {
              Mat rawData;
              if (sock1State == 2) {
                rawData = Mat(1, PACK_SIZE * sock1TotoalPack, CV_8UC1, longbuf1);
                sock1State = 0;
                if (sock2State == 2) {
                  sock2State = 0;
                }
                cout << "Fram displayed from Main interface." << endl;
              } else if (sock2State == 2) {
                rawData = Mat(1, PACK_SIZE * sock2TotoalPack, CV_8UC1, longbuf2);
                sock2State = 0;
                cout << "Fram displayed from Second interface." << endl;
              }
              
              //Mat rawData = Mat(1, PACK_SIZE * total_pack, CV_8UC1, longbuf);
              Mat frame = imdecode(rawData, CV_LOAD_IMAGE_COLOR);
              if (frame.size().width == 0) {
                  cerr << "decode failure!" << endl;
                  continue;
              }
              imshow("recv", frame);
              //free(longbuf);

              waitKey(1);
              clock_t next_cycle = clock();
              double duration = (next_cycle - last_cycle) / (double) CLOCKS_PER_SEC;
              //cout << "\teffective FPS:" << (1 / duration) << " \tkbps:" << (PACK_SIZE * total_pack / duration / 1024 * 8) << endl;

              cout << next_cycle - last_cycle;
              last_cycle = next_cycle;
            }
 
            
        }
    } catch (SocketException & e) {
        cerr << e.what() << endl;
        exit(1);
    }

    return 0;
}