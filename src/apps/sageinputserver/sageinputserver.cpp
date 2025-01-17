/**************************************************************************************************
* THE OMICRON PROJECT
 *-------------------------------------------------------------------------------------------------
 * Copyright 2010-2015		Electronic Visualization Laboratory, University of Illinois at Chicago
 * Authors:										
 *  Arthur Nishimoto		anishimoto42@gmail.com
 *-------------------------------------------------------------------------------------------------
 * Copyright (c) 2010-2015, Electronic Visualization Laboratory, University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted 
 * provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this list of conditions 
 * and the following disclaimer. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the documentation and/or other 
 * materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF 
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *************************************************************************************************/
#include <omicron.h>
#include <vector>
#include "omicron/InputServer.h"

#include <time.h>
#include <iostream>
using namespace omicron;

#ifdef WIN32
#include <ws2tcpip.h>
#include <winsock2.h>
#define itoa _itoa
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GESTURE TYPES
enum GESTURE_TYPE
{
    GESTURE_NULL,           // nothing... default... unrecognizable gesture
    GESTURE_SINGLE_TOUCH,   // 1 finger at a time
    GESTURE_DOUBLE_CLICK,   // 1 finger, twice in a row
    GESTURE_BIG_TOUCH,      // 1 big blob
    GESTURE_ZOOM,           // 2 finger pinch gesture
    GESTURE_MULTI_TOUCH_HOLD,  // 4 or more fingers at a time in one place
    GESTURE_MULTI_TOUCH_SWIPE, // 4 or more fingers at a time moving
};

// 0 for no sage... for testing only
#define USE_SAGE 1


/*****************************************************************************/
// globals
/*****************************************************************************/

#define round(fp) (int)((fp) >= 0 ? (fp) + 0.5 : (fp) - 0.5)


/////////  sage communication stuff  /////////
int DIM_PORT = 20005;
char sageHost[100];
char pqServer[100];
char myIP[16];
char msg[1024]; // will hold queued up messages
int sock;

int touchResW = 1600;
int touchResH = 1200;

bool sageConnected = false;

#ifndef SOCKET_ERROR
#define SOCKET_ERROR    -1
#endif

float timeLastEventSent = 0;

class SAGEInputServer: public InputServer{
public:
	void connectToSage();
	void handleEvent(Event*);

	void pointerToSAGEEvent(Event*);
	void wandToSAGEEvent(Event*);

	void queueMessage(char*);
	void sendToSage();

	bool isSAGEConnected();
}; //class

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SAGEInputServer::connectToSage(){
	if (!USE_SAGE) 
		return;
	
    // socket stuff
    int error;
    struct sockaddr_in peer;
    struct hostent *myInfo;
    char myHostname[128];

    peer.sin_family = AF_INET;
    peer.sin_port = htons(DIM_PORT);
    peer.sin_addr.s_addr = inet_addr(sageHost);

    sock = socket(AF_INET, SOCK_STREAM, 0);
	
    if (sock < 0)
    {
		printf("\nERROR: Couldn't open socket on port %d!\n", DIM_PORT);
		exit(0);
    } 

    // get my IP for the message header to SAGE
    gethostname(myHostname, 128);
    myInfo = gethostbyname(myHostname);
    sprintf(myIP, "%s", inet_ntoa(*(struct in_addr*) myInfo->h_addr));

    // ignore SIGPIPE
    int set = 1;
    //setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int)); //Windows doesn't have SIGPIPE
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&set, sizeof(int));
    setsockopt(sock, SOL_SOCKET, TCP_NODELAY, (const char *)&set, sizeof(int));

	
    while ((error = connect(sock, (struct sockaddr*)&peer, sizeof(peer))) != 0)
    {
		// close the failed one...
		closesocket(sock);  
		sageConnected = false;

		printf("\nTrying to reconnect to sage on %s:%d... failed, socket error code: %d", sageHost, DIM_PORT, errno);
		Sleep(1000);

		// recreate the socket...
		sock = socket(AF_INET, SOCK_STREAM, 0);

		// ignore SIGPIPE
		int set = 1;
		//setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int)); //Windows doesn't have SIGPIPE
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&set, sizeof(int));
		setsockopt(sock, SOL_SOCKET, TCP_NODELAY, (const char *)&set, sizeof(int));
    }
	
    Sleep(1000);
    printf("\nConnected to sage on: %s\n", sageHost);

	createClient( sageHost, DIM_PORT, DataMode::data_omicron, sock );
    sageConnected = true;
}
int triggerFlag = 0;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SAGEInputServer::handleEvent(Event* evt){

	if( evt->getServiceType() == Service::Pointer ){
		// SAGE
		//pointerToSAGEEvent( evt );

		// SAGE2
		char* eventPacket = createOmicronPacketFromEvent(evt);
		if( isSAGEConnected() )
		{
			sendToClients(eventPacket);
		}
		else
		{
			// Remove client

		}
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SAGEInputServer::pointerToSAGEEvent(Event* evt)
{
	char msgData[256];
	int id = evt->getSourceId();
	float xPos = evt->getPosition().x();
	float yPos = 1.0 - evt->getPosition().y(); // Flip y position for SAGE
	int eventType = evt->getType();
	bool validEvent = false;

	int gestureType = GESTURE_SINGLE_TOUCH;

	memset( msgData, 0, 256 );

	// Get gesture type from event flag
	if( (evt->getFlags() & Event::Click) == Event::Click )
		gestureType = GESTURE_DOUBLE_CLICK;
	if( (evt->getFlags() & 1 << 16) == 1 << 16 )
		gestureType = GESTURE_BIG_TOUCH;
	if( (evt->getFlags() & 1 << 17) == 1 << 17 )
		gestureType = GESTURE_MULTI_TOUCH_HOLD;
	if( (evt->getFlags() & 1 << 18) == 1 << 18 )
		gestureType = GESTURE_MULTI_TOUCH_SWIPE;
	if( eventType == Event::Zoom )
	{
		gestureType = GESTURE_ZOOM;
		eventType = evt->getExtraDataFloat(3);
	}

	// Remap eventType to match SAGE Touch lifepoint
	switch( eventType )
	{
		case Event::Down: eventType = 1; break; // Begin
		case Event::Move: eventType = 2; break; // Middle
		case Event::Up: eventType = 3; break; // End
	}

	// Single touch - Double touch - Big/Palm touch
	if( gestureType == GESTURE_SINGLE_TOUCH || gestureType == GESTURE_DOUBLE_CLICK || gestureType == GESTURE_BIG_TOUCH )
	{
		sprintf(msgData, "%s:pqlabs%d pqlabs %d %f %f %d\n", 
				myIP, id, gestureType, xPos, yPos, eventType);
		validEvent = true;
	}

	// Multi-touch hold (4+ finger)
	else if( gestureType == GESTURE_MULTI_TOUCH_HOLD )
	{
		int pointsSize = 5;

		sprintf(msgData, "%s:pqlabs%d pqlabs %d %f %f %d %d\n", 
			myIP, id, gestureType, xPos, yPos, pointsSize, eventType);
		validEvent = true;
	}

	// Multi-touch swipe (4+ finger)
	else if( gestureType == GESTURE_MULTI_TOUCH_SWIPE )
	{
		// Values have no effect?
		float dx = 0;
		float dy = 0;
		int pointsSize = 5;

		sprintf(msgData, "%s:pqlabs%d pqlabs %d %f %f %f %f %d %d\n", 
				myIP, id, gestureType, xPos, yPos, dx, dy, pointsSize, eventType);
		validEvent = true;
	}

	// Zoom touch
	else if( gestureType == GESTURE_ZOOM )
	{
		eventType = evt->getExtraDataFloat(3);

		float amount = evt->getExtraDataFloat(2); // Zoom delta
		sprintf(msgData, "%s:pqlabs%d pqlabs %d %f %f %f %d\n", 
				myIP, id, gestureType, xPos, yPos, amount, eventType);
		validEvent = true;
	}

	if( validEvent ){
		
		queueMessage(msgData);
		sendToSage();
	}

}

void SAGEInputServer::queueMessage(char *newMsg) 
{
    if (!USE_SAGE) return;

    if (strlen(msg) == 0) {
	sprintf(msg, "%s", newMsg);
     }
    else {
	sprintf(msg, "%s%s", msg, newMsg);
    }
}

void SAGEInputServer::sendToSage()
{
    if (!USE_SAGE || strlen(msg) == 0)
		return;
    
    if (send(sock, msg, strlen(msg), 0) == SOCKET_ERROR)
    {
		printf("\nDisconnected from sage... reconnecting\n");
		connectToSage();   // reconnect automatically
    }
	
	memset( msg, 0, 1024 );
}

bool SAGEInputServer::isSAGEConnected()
{
    if( !USE_SAGE )
		return false;
    
    if (send(sock, msg, strlen(msg), 0) == SOCKET_ERROR)
    {
		printf("\nDisconnected from sage... reconnecting\n");
		connectToSage();
		return false;
    }

	memset( msg, 0, 1024 );
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void main(int argc, char** argv)
{
	omsg("OmicronSDK - SAGEInputServer");
	omsg("Copyright (C) 2010-2015 Electronic Visualization Laboratory\nUniversity of Illinois at Chicago");
	omsg("======================================================");
	omsg("");

	SAGEInputServer app;

	// Read config file name from command line or use default one.
	const char* cfgName = "sageTouch.cfg";
	if(argc == 2) cfgName = argv[1];

	Config* cfg = new Config(cfgName);

	DataManager* dm = DataManager::getInstance();

	// Add a default filesystem data source using current work dir.
	dm->addSource(new FilesystemDataSource("./"));
	dm->addSource(new FilesystemDataSource(OMICRON_DATA_PATH));

	ServiceManager* sm = new ServiceManager();
	sm->setupAndStart(cfg);

	Sleep(1000);

	Setting& stRoot = cfg->getRootSetting()["config"];

	if( stRoot.exists("sagePort") )
	{
		DIM_PORT = cfg->getIntValue("sagePort", stRoot, 20005);
		ofmsg("SAGE port set to %1%", %DIM_PORT);
	}

	bool sageIPSet = false;
	if( stRoot.exists("sageHostIP") )
	{
		strcpy( sageHost, cfg->getStringValue("sageHostIP", stRoot, "").c_str() );
		sageIPSet = true;
		app.connectToSage();
	}
	else
	{
		std::cout << "sageHostIP not in configuration file. Enter SAGE IP address: " << std::endl;
		String sageIP;
		std::cin >> sageIP;
		std::cout << "IP address: " << sageIP << std::endl;
		strcpy( sageHost, sageIP.c_str() );
		sageIPSet = true;
		app.connectToSage();
	}

	

	bool printOutput = false;
	bool runServer = true;

	float xPos = 0;
	float yPos = 0.5;
    float increment = 0.00001f;

	while(runServer)
	{
		sm->poll();
		// Get events
        int av = sm->getAvailableEvents();
        //ofmsg("------------------------loop %1%  av %2%", %i++ %av);
        if(av != 0)
        {
			/*
            // TODO: Instead of copying the event list, we can lock the main one.
            Event evts[OMICRON_MAX_EVENTS];
            sm->getEvents(evts, OMICRON_MAX_EVENTS);
            for( int evtNum = 0; evtNum < av; evtNum++)
            {
                app.handleEvent(&evts[evtNum]);
            }
			*/
        }
#ifdef WIN32
		Sleep(1);
#else
		usleep(1000);
#endif	
	}

	sm->stop();
	delete sm;
	delete cfg;
	delete dm;
}
