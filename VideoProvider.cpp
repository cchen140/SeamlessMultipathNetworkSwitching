/*
 *   C++ UDP socket client for live image upstreaming
 *   Modified from http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/practical/UDPEchoClient.cpp
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

#include "PracticalSocket.h"      // For UDPSocket and SocketException
#include <iostream>               // For cout and cerr
#include <cstdlib>                // For atoi()
#include <ncurses.h>               // sudo apt-get install libncurses-dev. For getch() getting pressed key.

using namespace std;

#include "opencv2/opencv.hpp"
using namespace cv;
#include "config.h"

#define REDUNDANT_FRAME_PERIOD  10  // The smaller the more reliable.

timespec ts_diff(timespec start, timespec end);

int main(int argc, char * argv[]) {
    if ((argc < 4) || (argc > 4)) { // Test for correct number of arguments
        cerr << "Usage: " << argv[0] << " <Client IF1> <Client IF2> <Client Port>\n";
        exit(1);
    }

    //string servAddress = argv[1]; // First arg: server address
    string client1Address = argv[1];
    string client2Address = argv[2];
    unsigned short servPort = Socket::resolveService(argv[3], "udp");

    string primaryInterface = client1Address;
    string secondInterface = client2Address;
    
    try {
        UDPSocket sock;
        int jpegqual =  ENCODE_QUALITY; // Compression Parameter

        Mat frame, send;
        vector < uchar > encoded;
        VideoCapture cap(1); // Grab the camera
        namedWindow("send", CV_WINDOW_AUTOSIZE);
        if (!cap.isOpened()) {
            cerr << "OpenCV Failed to open camera";
            exit(1);
        }

        
        struct timespec ts_last;
        clock_gettime(CLOCK_MONOTONIC, &ts_last);
        
        int sock1FrameCount = 0;
        int key = 0;
        
        
        /* Initialize getch() */
        initscr();
        clear();
        noecho();
        cbreak();        
        nodelay(stdscr, TRUE);  // to make getch() non-blocking.
        
        while (1) {
          
            key = getch();  // Get key press
            if (key != ERR) {
              /* A key press is detected, exchange the role of primary and secondary interfaces. */
              string temp = primaryInterface;
              primaryInterface = secondInterface;
              secondInterface = temp;
              cout << "@ Primary: " << primaryInterface << "\tScond: " << secondInterface << "\r" << endl;
            }
                       
          
            cap >> frame;
            if(frame.size().width==0)continue;//simple integrity check; skip erroneous data...
            resize(frame, send, Size(FRAME_WIDTH, FRAME_HEIGHT), 0, 0, INTER_LINEAR);
            vector < int > compression_params;
            compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
            compression_params.push_back(jpegqual);

            imencode(".jpg", send, encoded, compression_params);
            imshow("send", send);
            int total_pack = 1 + (encoded.size() - 1) / PACK_SIZE;

            /* Begin to transmit one frame to client's primary interface. */
            int ibuf[1];
            ibuf[0] = total_pack;
            sock.sendTo(ibuf, sizeof(int), primaryInterface, servPort);

            //cout << "Total Pack: " << total_pack << endl;
            for (int i = 0; i < total_pack; i++) {
                sock.sendTo( & encoded[i * PACK_SIZE], PACK_SIZE, primaryInterface, servPort);
            }
            /* End of transmitting one from to client's primary interface. */
            
            /* Transmit the redundant frame. */
            sock1FrameCount++;
            if (sock1FrameCount >= REDUNDANT_FRAME_PERIOD) {
              sock.sendTo(ibuf, sizeof(int), secondInterface, servPort);
              for (int i = 0; i < total_pack; i++) {
                sock.sendTo( & encoded[i * PACK_SIZE], PACK_SIZE, secondInterface, servPort);
              }
              //cout << "@ Redundant frame has sent out.\r" << endl;
              sock1FrameCount = 0;
            }

            waitKey(FRAME_INTERVAL);

            struct timespec ts_next;
            clock_gettime(CLOCK_MONOTONIC, &ts_next);
            struct timespec ts_duration = ts_diff(ts_last, ts_next);
              
            double duration = (ts_duration.tv_nsec)/1000000000.0;
            //cout << "\teffective FPS:" << (1 / duration) << " \tkbps:" << (PACK_SIZE * total_pack / duration / 1024 * 8) << "\r" << endl;
            
            
            // Timestamp's unit is second in the following log.
            cout << ts_next.tv_sec + (ts_next.tv_nsec/1000000000.0) << "," << "server," <<  (1 / duration) << "," << (PACK_SIZE * total_pack / duration / 1024 * 8) << "\r" << endl;

            ts_last.tv_nsec = ts_next.tv_nsec;
            ts_last.tv_sec = ts_next.tv_sec;
        }
        // Destructor closes the socket

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