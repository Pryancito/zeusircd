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
#include "dprint.h"
#include "Poller_poll.h"

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <cstring>
#include <thread>

int Poller_epoll::init(void)
{
	// initialize a kernel queue for use by the class
	mKernelQueue = epoll_create1(0);
	if(mKernelQueue == -1)
		return errno;
	// initialize everything to zero
	mNumResults = 0;
	mNumFds = 0;
	mMaxFds = 0;
	mCurResult = 0;

	Poller::init();

	return 0;	// success
}

void Poller_epoll::shutdown(void)
{
	/* kernel queue events are automatically deleted when the socket is
	 * closed, so we don't need to do anything special.
	 */
	mNumResults = 0;
	mNumFds = 0;

	close(mKernelQueue);
	Poller::shutdown();
}

int Poller_epoll::add(int fd, Client *client, short eventmask)
{
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	//Client *cli= (Client *) malloc(sizeof(Client));
	//cli = client;
	fdmap.emplace ( fd, std::pair<epoll_event, Client *>(ev, client) );
	epoll_ctl(mKernelQueue, EPOLL_CTL_ADD, fd, &fdmap[fd].first);
	mNumFds++;
	return 0;
}

int Poller_epoll::del(int fd)
{
	if (mNumFds == 0) { 
		return EINVAL;	// oops, one too many ::del() calls
	}

	epoll_ctl(mKernelQueue, EPOLL_CTL_DEL, fd, &fdmap[fd].first);
	//delete fdmap[fd].second;
	fdmap.erase(fd);
	mNumFds--;
	return 0;
}

int Poller_epoll::setMaskInternal(int fd, short eventmask, Client *client)
{
	return 0;
}

int Poller_epoll::clearMaskInternal(int fd, short eventmask)
{
	return 0;
}

int Poller_epoll::setMask(int fd, short eventmask)
{
	return 0;
}

int Poller_epoll::orMask(int fd, short eventmask)
{
	return 0;
}

int Poller_epoll::andMask(int fd, short eventmask)
{
	return 0;
}

int Poller_epoll::waitForEvents(int timeout_millisec)
{
	mNumResults = epoll_wait(mKernelQueue, events, 100, timeout_millisec);
	mCurResult = 0;
	if(mNumResults == -1) {
		int err = errno;
		DPRINT(("Poller_epoll::waitForEvents : kevent : %s (errno %d)\n",
			strerror(err), err));
		return err;
	}
	if(mNumResults == 0)
		return EWOULDBLOCK;
	return 0;
}

int Poller_epoll::getNextEvent(PollEvent *e)
{
	if(mCurResult == mNumResults)
		return EWOULDBLOCK;	// no more events
	struct epoll_event *ke = &events[mCurResult++];
	memset(e, 0, sizeof(struct PollEvent));
	e->fd = ke->data.fd;
	e->revents = ke->events;
	e->client = fdmap[ke->data.fd].second;
	return 0;
}

int Poller_epoll::waitAndDispatchEvents(int timeout_millisec)
{
	struct PollEvent pe;

	if(waitForEvents(timeout_millisec))
		return EWOULDBLOCK;
	while(getNextEvent(&pe) == 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		Client *client = pe.client;
		if(client)
			client->notifyPollEvent(&pe);
	}
	return 0;
}
