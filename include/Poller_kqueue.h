/*  Poller_kqueue - FreeBSD 4.1 kernel queue wrapper class
 *  Copyright (C) 2000 Michael R. Elkins <me@sigpipe.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef FREEBSD

#ifndef POLLER_KQUEUE
#define POLLER_KQUEUE
#include "Poller.h"
#include <sys/event.h>

/**
 An implementation of Poller for FreeBSD4.1's kqueue() system call.
 Note: vanilla FreeBSD4.1 may require a patch; see the file
 fbsd-41-kqueue.diff
*/

class Poller_kqueue : public Poller {

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
	struct kevent *mChanges;	// changes to the kernel queue
	struct kevent *mResults;	// results from last kevent() call
	int mNumChanges;		// number of pending changes
	int mMaxChanges;		// number of alloc'd elements in mChanges/mResults
	int mNumResults;			// number of elements in mResults;
	int mNumFds;			// number of fds being watched
	int mMaxFds;
	int mCurResult;			// counter for getNextEvent()
};

#endif
#endif
