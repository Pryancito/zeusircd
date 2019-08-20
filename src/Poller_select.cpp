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
#include "Poller_select.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

int Poller_select::init()
{
	DPRINT(("Poller_select::init()\n"));

	m_max_fd = -1;
	m_cur_fd = -1;
	m_rbits = 0;

	memset(m_clients, 0, FD_SETSIZE*sizeof(m_clients[0]));

	// Clear file descriptor sets
	FD_ZERO(&m_readfds);
	FD_ZERO(&m_testrfds);
	FD_ZERO(&m_testwfds);
	FD_ZERO(&m_writefds);
#ifdef Poller_URGENT
	FD_ZERO(&m_exceptfds);
	FD_ZERO(&m_testefds);
#endif

	Poller::init();

	return 0;
}

void Poller_select::shutdown()
{
	if (m_max_fd != -1) {
		m_max_fd = -1;
		m_cur_fd = -1;
		m_rbits = 0;
	}
	Poller::shutdown();
}

int Poller_select::add(int fd, Client *client, short eventmask)
{
	if ((fd < 0) || (fd >= FD_SETSIZE)) {
		LOG_ERROR(("add(fd %d): fd out of range\n", fd));
		return EINVAL;
	}
	if (m_clients[fd]) {
		LOG_ERROR(("add(fd %d): already monitoring that fd!\n", fd));
		return EALREADY;
	}

	// Update limits.
	if (fd > m_max_fd)
		m_max_fd = fd;

	// Finally, add this fd and its eventmask.
	m_clients[fd] = client;
	setMask(fd,eventmask);

	DPRINT(("add(fd %d, %p, %x) max_fd %d\n",
		fd, client, eventmask, m_max_fd));
	return 0;
}

int Poller_select::del(int fd)
{
	// Sanity checks
	if ((fd < 0) || (fd >= FD_SETSIZE)) {
		LOG_ERROR(("del(fd %d): fd out of range\n", fd));
		return EINVAL;
	}
	if (!m_clients[fd]) {
		LOG_ERROR(("del(fd %d): not monitoring that fd!\n", fd));
		return EALREADY;
	}
	assert(m_max_fd >= fd);

	// Remove this fd
	setMask(fd,0);

	// Fix bug reported by Jeon: make sure getNextEvent doesn't
	// return a deleted client
	FD_CLR(fd,&m_testrfds);
	FD_CLR(fd,&m_testwfds);
#ifdef Poller_URGENT
	FD_CLR(fd,&m_testefds);
#endif

	m_clients[fd] = NULL;

	// Update limits
	while ((m_max_fd >= 0) && (m_clients[m_max_fd] == NULL)) 
		m_max_fd--;
		
	return 0;
}

int Poller_select::setMask(int fd, short eventmask)
{
	// Sanity checks
	if ((fd < 0) || (fd >= FD_SETSIZE)) {
		LOG_ERROR(("setMask(fd %d): fd out of range\n", fd));
		return EINVAL;
	}
	if (!m_clients[fd]) {
		LOG_ERROR(("setMask(fd %d): not monitoring that fd!\n", fd));
		return EALREADY;
	}
	assert(m_max_fd >= fd);

	FD_CLR(fd,&m_readfds);
	if (eventmask & POLLIN) FD_SET(fd,&m_readfds);
	FD_CLR(fd,&m_writefds);
	if (eventmask & POLLOUT) FD_SET(fd,&m_writefds);
#ifdef Poller_URGENT
	FD_CLR(fd,&m_exceptfds);
	if (eventmask & POLLPRI) FD_SET(fd,&m_exceptfds);
#endif

	DPRINT(("setMask(fd %d, %x)\n", fd, eventmask));

	return 0;
}

int Poller_select::orMask(int fd, short eventmask)
{
	// Sanity checks
	if ((fd < 0) || (fd >= FD_SETSIZE)) {
		LOG_ERROR(("orMask(fd %d): fd out of range\n", fd));
		return EINVAL;
	}
	if (!m_clients[fd]) {
		LOG_ERROR(("orMask(fd %d): not monitoring that fd!\n", fd));
		return EALREADY;
	}
	assert(m_max_fd >= fd);

	if (eventmask & POLLIN) FD_SET(fd,&m_readfds);
	if (eventmask & POLLOUT) FD_SET(fd,&m_writefds);
#ifdef Poller_URGENT
	if (eventmask & POLLPRI) FD_SET(fd,&m_exceptfds);
#endif

	DPRINT(("orMask(fd %d, %x)\n", fd, eventmask));

	return 0;
}

int Poller_select::andMask(int fd, short eventmask)
{
	// Sanity checks
	if ((fd < 0) || (fd >= FD_SETSIZE)) {
		LOG_ERROR(("andMask(fd %d): fd out of range\n", fd));
		return EINVAL;
	}
	if (!m_clients[fd]) {
		LOG_ERROR(("andMask(fd %d): not monitoring that fd!\n", fd));
		return EALREADY;
	}
	assert(m_max_fd >= fd);

	if (!(eventmask & POLLIN)) FD_CLR(fd,&m_readfds);
	if (!(eventmask & POLLOUT)) FD_CLR(fd,&m_writefds);
#ifdef Poller_URGENT
	if (!(eventmask & POLLPRI)) FD_CLR(fd,&m_exceptfds);
#endif

	DPRINT(("andMask(fd %d, %x)\n", fd, eventmask));

	return 0;
}

/**
 Sleep at most timeout_millisec waiting for an I/O readiness event
 on the file descriptors we're watching.  Fills internal array
 of readiness events.  Call getNextEvent() repeatedly to read its
 contents.
 @return 0 on success, EWOULDBLOCK if no events ready
 */
int Poller_select::waitForEvents(int timeout_millisec)
{
	int err;
	struct timeval timeout;

	// Set timeout
	timeout.tv_sec  = timeout_millisec/1000;
	timeout.tv_usec = (timeout_millisec % 1000) * 1000;

	// Reset file descriptor sets
	// FIXME: can't do this on some operating systems.
	m_testrfds=m_readfds;
	m_testwfds=m_writefds;
#ifdef Poller_URGENT
	m_testefds=m_exceptfds;
#endif

	// Wait for I/O events the clients are interested in.
	m_rbits = select(m_max_fd+1, &m_testrfds, &m_testwfds, 
#ifdef Poller_URGENT
	&m_testefds, 
#else
	(fd_set *)0,
#endif
	&timeout);
	if (m_rbits == -1) {
		err = errno;
		DPRINT(("waitForEvents: select() returned -1, errno %d\n", err));
		return err;
	}

	m_cur_fd = -1;		// it will get preincremented by getNextEvent
	DPRINT(("waitForEvents: got %d events\n", m_rbits));
	return m_rbits ? 0 : EWOULDBLOCK;
}

/**
 Get the next event that was found by waitForEvents.
 @return 0 on success, EWOULDBLOCK if no more events
 */
int Poller_select::getNextEvent(Poller::PollEvent *e)
{
	int revents;
	DPRINT(("getNextEvent: m_rbits %d\n",m_rbits));

	if (m_rbits < 1)
		return EWOULDBLOCK;

	// Find the next fd with bits set
	for (revents=0; m_cur_fd++ < m_max_fd; ) {
		// Avoiding branches inside this loop only improves benchmark by 2%,
		// so don't bother.
		if (FD_ISSET(m_cur_fd, &m_testrfds)) revents = POLLIN;
		if (FD_ISSET(m_cur_fd, &m_testwfds)) revents |= POLLOUT;
#ifdef Poller_URGENT
		if (FD_ISSET(m_cur_fd, &m_testefds)) revents |= POLLPRI;
#endif
		if (revents)
			break;
	}
	if (!revents) {
		// None found
		return EWOULDBLOCK;
	}

	// Found one.  Update count of set bits for next call.
	if (revents & POLLIN)  m_rbits--;
	if (revents & POLLOUT) m_rbits--;
#ifdef Poller_URGENT
	if (revents & POLLPRI) m_rbits--;
#endif

	// Fill event struct.
	e->fd = m_cur_fd;
	e->revents = revents;
	e->client = m_clients[m_cur_fd];

	DPRINT(("getNextEvent: fd %d revents %x m_rbits %d\n",
		e->fd, e->revents, m_rbits));

	return 0;
}

int Poller_select::waitAndDispatchEvents(int timeout_millisec)
{
	int err;
	PollEvent event;

	err = waitForEvents(timeout_millisec);
	if (err)
		return err;

	// Pump any network traffic into the appropriate Clients
	while (m_rbits > 0) {
		err = getNextEvent(&event);
		if (err) {
			if (err != EWOULDBLOCK)
				DPRINT(("waitAndDispatchEvents: getNextEvent() returned %d\n", err));
			break;
		}
		err = event.client->notifyPollEvent(&event);
		if (err) {
			DPRINT(("waitAndDispatchEvents: %p->notifyPollEvent(fd %d) returned %d, deleting\n",
				event.client, event.fd, err));
			del(event.fd);
		}
	}

	return 0;
}
