#ifndef Poller_poll_H
#define Poller_poll_H
/*--------------------------------------------------------------------------
 Copyright 1999,2000, Dan Kegel http://www.kegel.com/
 See the file COPYING
 (Also freely licensed to Disappearing, Inc. under a separate license
 which allows them to do absolutely anything they want with it, without
 regard to the GPL.)  

 This module is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This module is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--------------------------------------------------------------------------*/
#include <assert.h>
#include <errno.h>
#include <sys/epoll.h>   
#include <time.h>   
#include <unistd.h>
#include <map>
#include "Poller.h"

/** 
 * A class to monitor a set of file descriptors for readiness events.
 * Current an efficient wrapper around the poll() system call.
 * This is the code you would have written yourself if you had had the time...
 */
class Poller_epoll : public Poller {

public:
	int init();
	void shutdown();
	int add(int fd, Client *client, short eventmask);
	int del(int fd);
	int setMask(int fd,short eventmask);
	int orMask(int fd,short eventmask);
	int andMask(int fd,short eventmask);
	int waitForEvents(int timeout_millisec);
	int getNextEvent(PollEvent *e);
	int waitAndDispatchEvents(int timeout_millisec);

private:
	int setMaskInternal(int fd, short eventmask, Client *client);
	int clearMaskInternal(int fd, short eventmask);
	int mKernelQueue;		// kernel queue handle
	struct epoll_event events[100];
	int mNumResults;			// number of elements in mResults;
	int mNumFds;			// number of fds being watched
	int mMaxFds;
	int mCurResult;			// counter for getNextEvent()
	//Client **m_clients;
	std::map<int, std::pair<epoll_event, Client *>> fdmap;
};
#endif
