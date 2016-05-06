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
#include <ncurses.h>         // sudo apt-get install libncurses-dev. For getch() getting pressed key.

#define BUF_LEN 65540 // Larger than maximum UDP packet size
#define TOTAL_PACK  30  // This value was dynamically configured in run-time with a range around 20 to 21 on my Surface. Set to 30 here to keep some room.

#include "opencv2/opencv.hpp"
using namespace cv;
#include "config.h"

timespec ts_diff(timespec start, timespec end);

int main(int argc, char * argv[]) {

    if (argc != 5) { // Test for correct number of parameters
        cerr << "Usage: " << argv[0] << " <IF#1 IP> <IF#2 IP> <Server IP> <Server Port> " << endl;
        exit(1);
    }

    string if1Address = argv[1];
    string if2Address = argv[2];
    string serverAddress = argv[3];
    unsigned short servPort1 = atoi(argv[4]); // First arg:  local port
    unsigned short servPort2 = servPort1;

    namedWindow("recv", CV_WINDOW_AUTOSIZE);
    try {
        UDPSocket sock1(if1Address, servPort1);
        UDPSocket sock2(if2Address, servPort2);

        //char buffer[BUF_LEN]; // Buffer for echo string
        
        int recvMsgSize; // Size of received message
        string sourceAddress; // Address of datagram source
        unsigned short sourcePort; // Port of datagram source

        struct timespec ts_last_if1;
        struct timespec ts_last_if2;
        clock_gettime(CLOCK_MONOTONIC, &ts_last_if1);
        clock_gettime(CLOCK_MONOTONIC, &ts_last_if2);
        
        struct timespec ts_last_HB;
        clock_gettime(CLOCK_MONOTONIC, &ts_last_HB);
        
        int timestamp_counter_if1 = 0;
        int timestamp_counter_if2 = 0;

        /* Variables for the primary interface */
        int sock1State = 0;
        int sock1TotoalPack = 0;
        int sock1PackCount = 0;
        char buffer1[BUF_LEN]; // Buffer for echo string
        char * longbuf1 = new char[PACK_SIZE * TOTAL_PACK];
        
        /* Variables for the secondary interface */
        int sock2State = 0;
        int sock2TotoalPack = 0;
        int sock2PackCount = 0;
        char buffer2[BUF_LEN]; // Buffer for echo string
        char * longbuf2 = new char[PACK_SIZE * TOTAL_PACK];
        
        /* Initialize getch() */
        initscr();
        clear();
        noecho();
        cbreak();        
        nodelay(stdscr, TRUE);  // to make getch() non-blocking.
        int key = 0;
        int enable_if1_HB = 1;
        while (1) {
          
            key = getch();
            if (key != ERR) {
              if (enable_if1_HB == 1) {
                enable_if1_HB = 0;
              } else {
                enable_if1_HB = 1;
              }
              cout << "IF1 HB toggled.\r" << endl;
            }
          
            int rv;
            struct pollfd ufds[2];
            ufds[0].fd = sock1.getDescriptor();
            ufds[0].events = POLLIN; // check for just normal data

            ufds[1].fd = sock2.getDescriptor();
            ufds[1].events = POLLIN; // check for just normal data
          
            rv = poll(ufds, 2, 1);    // The third parameter is timeout in minisecond.
            
            if (rv == -1) {
                perror("poll"); // error occurred in poll()
            } else if (rv == 0) {
                //printf("Timeout occurred!  No data after 100 miniseconds.\n");
                continue;
            }
            
            /* sock1State and sock2State: 
             * 0: Waiting for the initial packet (indicating the length of the frame packets) of a frame. 
             * 1: The initial packet has been received, in the process of receiving frame packets. 
             * 2: A complete frame has been received. This state is used for displaying the frame. 
             */
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
            
 
            /* If any of the interface has received a complete frame, go and display it. */
            if ( (sock1State==2) || (sock2State==2) ) {
                
              struct timespec ts_next;
              clock_gettime(CLOCK_MONOTONIC, &ts_next);  
              struct timespec ts_duration;
              double duration;
                
              int total_pack;
              Mat rawData;
              if (sock1State == 2) {
                /* sock1 has a frame. */
                rawData = Mat(1, PACK_SIZE * sock1TotoalPack, CV_8UC1, longbuf1);
                sock1State = 0;
                total_pack = sock1TotoalPack;
                if (sock2State == 2) {
                  /* Drop sock2's frame since we are using the frame from sock1. */
                  sock2State = 0;
                }
                //cout << "Fram displayed from Main interface.\r" << endl;
                ts_duration = ts_diff(ts_last_if1, ts_next);
                duration = (ts_duration.tv_nsec)/1000000000.0;

                ts_last_if1.tv_nsec = ts_next.tv_nsec;
                ts_last_if1.tv_sec = ts_next.tv_sec;
                cout << ts_next.tv_sec + (ts_next.tv_nsec/1000000000.0) << "," << "IF1," <<  (1 / duration) << "," << (PACK_SIZE * total_pack / duration / 1024 * 8) << "\r" << endl;
                cout << ts_next.tv_sec + (ts_next.tv_nsec/1000000000.0) << "," << "all," <<  (1 / duration) << "," << (PACK_SIZE * total_pack / duration / 1024 * 8) << "\r" << endl;

              } else if (sock2State == 2) {
                /* sock2 has a frame while sock1 doesn't have one. */
                rawData = Mat(1, PACK_SIZE * sock2TotoalPack, CV_8UC1, longbuf2);
                sock2State = 0;
                total_pack = sock2TotoalPack;
                //cout << "Fram displayed from Second interface.\r" << endl;
                ts_duration = ts_diff(ts_last_if2, ts_next);
                duration = (ts_duration.tv_nsec)/1000000000.0;
        
                ts_last_if2.tv_nsec = ts_next.tv_nsec;
                ts_last_if2.tv_sec = ts_next.tv_sec;
                cout << ts_next.tv_sec + (ts_next.tv_nsec/1000000000.0) << "," << "IF2," <<  (1 / duration) << "," << (PACK_SIZE * total_pack / duration / 1024 * 8) << "\r" << endl;
                cout << ts_next.tv_sec + (ts_next.tv_nsec/1000000000.0) << "," << "all," <<  (1 / duration) << "," << (PACK_SIZE * total_pack / duration / 1024 * 8) << "\r" << endl;
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
              //cout << (ts_next.tv_nsec/1000000)+(timestamp_counter*1000) << "," << "all," <<  (1 / duration) << "," << (PACK_SIZE * total_pack / duration / 1024 * 8) << "\r" << endl;

            }
            
            
            struct timespec ts_this_HB;
            clock_gettime(CLOCK_MONOTONIC, &ts_this_HB); 
            struct timespec ts_HB_diff = ts_diff(ts_last_HB, ts_this_HB);
            //if ( (ts_HB_diff.tv_sec>=(HEARTBEAT_PERIOD_MS/1000))
            //  && ((ts_HB_diff.tv_nsec/1000000)>=HEARTBEAT_PERIOD_MS)) {
            if ( (ts_HB_diff.tv_sec+(ts_HB_diff.tv_nsec/1000000000.0))>= (HEARTBEAT_PERIOD_MS/1000.0)) {
                char sendBuf1[1];
                sendBuf1[0] = 1;
                if (enable_if1_HB == 1) {
                  sock1.sendTo(sendBuf1, 1, serverAddress, 10005);
                }
                
                char sendBuf2[1];
                sendBuf2[0] = 2;
                sock2.sendTo(sendBuf2, 1, serverAddress, 10005);
                   
                ts_last_HB.tv_nsec = ts_this_HB.tv_nsec;
                ts_last_HB.tv_sec = ts_this_HB.tv_sec;
                
                //cout << "@@@ Heartbeat sent out. \r" << endl;
            }
 
            
        }
    } catch (SocketException & e) {
        cerr << e.what() << endl;
        exit(1);
    }

    return 0;
}

timespec ts_diff(timespec start, timespec end)
{
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}