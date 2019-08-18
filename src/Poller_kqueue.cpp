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

#if FREEBSD

#include <stdlib.h>
#include <string.h>
#include "Poller_kqueue.h"
#include "dprint.h"

/* returns 0 on success, or E* if an error occurs */
int Poller_kqueue::init(void)
{
	// initialize a kernel queue for use by the class
	mKernelQueue = kqueue();
	if(mKernelQueue == -1)
		return errno;
	// initialize everything to zero
	mNumChanges = 0;
	mMaxChanges = 0;
	mNumResults = 0;
	mChanges = 0;
	mResults = 0;
	mNumFds = 0;
	mMaxFds = 0;
	mCurResult = 0;

	Poller::init();

	return 0;	// success
}

void Poller_kqueue::shutdown(void)
{
	/* kernel queue events are automatically deleted when the socket is
	 * closed, so we don't need to do anything special.
	 */
	if(mChanges != 0) {
		free(mChanges);
		mChanges = 0;
	}
	if(mResults != 0) {
		free(mResults);
		mResults = 0;
	}
	mNumResults = 0;
	mNumChanges = 0;
	mMaxChanges = 0;
	mNumFds = 0;

	Poller::shutdown();
}

int Poller_kqueue::add(int fd, Client *client, short eventmask)
{
	if(mNumFds == mMaxFds) {
		mMaxFds += 128;
		/* note that we allocate 2* the amount since each event
		 * is returned separately
		 */
		mResults = (struct kevent*)realloc(mResults, 2 * sizeof(struct kevent) * mMaxFds);
	}
	mNumFds++;
	return setMaskInternal(fd, eventmask, client);
}

int Poller_kqueue::del(int fd)
{
	if (mNumFds == 0) { 
		return EINVAL;	// oops, one too many ::del() calls
	}

	if (mNumChanges + 2 > mMaxChanges) {
		mMaxChanges += 128;
		mChanges = (struct kevent*)realloc(mChanges, sizeof(struct kevent) * mMaxChanges);
	}

	struct kevent *ke;

	ke = &mChanges[mNumChanges++];
	ke->ident = fd;
	ke->filter = EVFILT_READ;
	ke->flags = EV_DELETE;

	ke = &mChanges[mNumChanges++];
	ke->ident = fd;
	ke->filter = EVFILT_WRITE;
	ke->flags = EV_DELETE;

	mNumFds--;

	return 0;
}

int Poller_kqueue::setMaskInternal(int fd, short eventmask, Client *client)
{
	struct kevent *ke;

	if(mNumChanges + 2 > mMaxChanges) {
		mMaxChanges += 128;
		mChanges=(struct kevent*)realloc(mChanges,sizeof(struct kevent) * mMaxChanges);
	}

	if (eventmask & POLLIN) {
		ke = &mChanges[mNumChanges++];
		memset(ke, 0, sizeof(struct kevent));
		ke->ident = fd;
		ke->filter = EVFILT_READ;
		ke->flags = EV_ADD | EV_ENABLE;
		if (client)
			ke->udata = client;
	}

	if (eventmask & POLLOUT) {
		//printf("Poller_kqueue::setMaskInternal : setting write on fd %d\n", fd);

		ke = &mChanges[mNumChanges++];
		memset(ke, 0, sizeof(struct kevent));
		ke->ident = fd;
		ke->filter = EVFILT_WRITE;
		ke->flags = EV_ADD | EV_ENABLE;
		if (client)
			ke->udata = client;
	}

	return 0;
}

int Poller_kqueue::clearMaskInternal(int fd, short eventmask)
{
	struct kevent *ke;

	if(mNumChanges + 2 > mMaxChanges) {
		mMaxChanges += 128;
		mChanges=(struct kevent*)realloc(mChanges,sizeof(struct kevent) * mMaxChanges);
	}

	if (!(eventmask & POLLIN)) {
		ke = &mChanges[mNumChanges++];
		memset(ke, 0, sizeof(struct kevent));
		ke->ident = fd;
		ke->filter = EVFILT_READ;
		ke->flags = EV_DELETE;
	}

	if (!(eventmask & POLLOUT)) {

		ke = &mChanges[mNumChanges++];
		memset(ke, 0, sizeof(struct kevent));
		ke->ident = fd;
		ke->filter = EVFILT_WRITE;
		ke->flags = EV_DELETE;
	}

	return 0;
}

int Poller_kqueue::setMask(int fd, short eventmask)
{
	int n;

	n = setMaskInternal(fd, eventmask, 0);
	if (n)
		return n;

	/* clear the unset bit(s).  this might not actually be set
	 * but to avoid keeping state we just attempt to unset it
	 * anyway
	 */
	n = clearMaskInternal(fd, eventmask);
	if (n)
		return n;

	return 0;
}

int Poller_kqueue::orMask(int fd, short eventmask)
{
	return setMaskInternal(fd, eventmask, 0);
}

int Poller_kqueue::andMask(int fd, short eventmask)
{
	return clearMaskInternal(fd, eventmask);
}

int Poller_kqueue::waitForEvents(int timeout_millisec)
{
	struct timespec to;

	if (timeout_millisec >= 0) {
		to.tv_sec = timeout_millisec / 1000;
		to.tv_nsec = (timeout_millisec % 1000) * 1000000;	// nanosec
	}

/*	printf("Poller_kqueue::waitForEvents : %d changes\n",
		mNumChanges);
	printf("Poller_kqueue::waitForEvents : %d fds\n",
		mNumFds);

	for(int i=0; i < mNumChanges; i++) {
		printf("Poller_kqueue::waitForEvents : fd=%d filt=%d flags=%d\n",
			mChanges[i].ident,
			mChanges[i].filter,
			mChanges[i].flags);
	}
*/

	mNumResults = kevent(mKernelQueue, mChanges, mNumChanges,
			mResults, 2*mMaxFds,
			(timeout_millisec >= 0) ? &to : (struct timespec *) 0);

	mCurResult = 0;
	mNumChanges = 0;	// reset
	if(mNumResults == -1) {
		int err = errno;
		DPRINT(("Poller_kqueue::waitForEvents : kevent : %s (errno %d)\n",
			strerror(err), err));
		return err;
	}
/*	printf("Poller_kqueue::waitForEvents : %d pending events\n",
		mNumResults);
*/
	if(mNumResults == 0)
		return EWOULDBLOCK;
	return 0;
}

int Poller_kqueue::getNextEvent(PollEvent *e)
{
	if(mCurResult == mNumResults)
		return EWOULDBLOCK;	// no more events
	struct kevent *ke = &mResults[mCurResult++];
	memset(e, 0, sizeof(struct PollEvent));
	e->fd = ke->ident;
	if (ke->filter == EVFILT_READ)
		e->revents = POLLIN;
	else if (ke->filter == EVFILT_WRITE)
		e->revents = POLLOUT;
	else
		e->revents = POLLERR;	// huh, what's this?
	e->client = (Client*)ke->udata;
	return 0;
}

int Poller_kqueue::waitAndDispatchEvents(int timeout_millisec)
{
	struct PollEvent pe;

	if(waitForEvents(timeout_millisec))
		return EWOULDBLOCK;
	while(getNextEvent(&pe) == 0) {
		Client *client = pe.client;
		if(client)
			client->notifyPollEvent(&pe);
	}
	return 0;
}

#endif /* HAVE_KQUEUE */
