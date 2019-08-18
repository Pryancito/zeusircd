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

int Poller_poll::init()
{
	DPRINT(("init()\n"));

	// Allocate things indexed by file descriptor.
	m_fd2client_used = 0;
	m_fd2client_alloc = 16;
	m_fd2pfdnum = (int *)malloc(sizeof(int) * m_fd2client_alloc);
	if (!m_fd2pfdnum)
		return ENOMEM;

	// Allocate things indexed by client number.
	m_pfds_used = 0;
	m_pfds_alloc = 16;
	m_clients = (Client **)malloc(sizeof(Client *) * m_pfds_alloc);
	if (!m_clients)
		return ENOMEM;
	m_pfds = (struct pollfd *)malloc(sizeof(struct pollfd) * m_pfds_alloc);
	if (!m_pfds)
		return ENOMEM;

	Poller::init();
	return 0;
}

void Poller_poll::shutdown()
{
	if (m_fd2pfdnum) {
		free(m_fd2pfdnum);
		m_fd2pfdnum = NULL;
		free(m_clients);
		m_clients = NULL;
		free(m_pfds);
		m_pfds = NULL;
	}
	Poller::shutdown();
}

int Poller_poll::add(int fd, Client *client, short eventmask)
{
	int i, n;
	// Resize arrays indexed by fd if fd is beyond what we've seen.
	if (fd >= m_fd2client_alloc) {
		n = m_fd2client_alloc * 2;
		if (n < fd + 1)
			n = fd + 1;

		int *pn = (int *)realloc(m_fd2pfdnum, n * sizeof(int));
		if (!pn)
			return ENOMEM;
		// Clear new elements
		for (i=m_fd2client_alloc; i<n; i++)
			pn[i] = -1;
		m_fd2pfdnum = pn;

		m_fd2client_alloc = n;
	}

	// Resize things indexed by client number if we've run out of spots.
	if (m_pfds_used == m_pfds_alloc) {
		n = m_pfds_alloc * 2;

		Client **clients= (Client **) realloc(m_clients, n * sizeof(Client *));
		if (!clients)
			return ENOMEM;
		m_clients = clients;

		struct pollfd *pfds = (struct pollfd *) realloc(m_pfds, n * sizeof(struct pollfd));
		if (!pfds)
			return ENOMEM;
		m_pfds = pfds;

		m_pfds_alloc = n;
	}

	// Update things indexed by file descriptor.
	m_clients[m_pfds_used] = client;
	m_pfds[m_pfds_used].fd = fd;
	m_pfds[m_pfds_used].events = eventmask;

	// Update things indexed by file descriptor.
	m_fd2pfdnum[fd] = m_pfds_used;

	// Update limits.
	if (fd >= m_fd2client_used)
		m_fd2client_used = fd+1;
	m_pfds_used++;

	DPRINT(("add(%d, %p, %x) pfdnum %d m_pfds_used %d\n",
		fd, client, eventmask, m_pfds_used -1, m_pfds_used));
	return 0;
}

int Poller_poll::del(int fd)
{
	// Sanity checks
	if (fd < 0 || fd >= m_fd2client_used) {
		LOG_ERROR(("del(%d): fd out of range\n", fd));
		return EINVAL;
	}
	assert(m_pfds_used > 0);

	// Note where the Client was in the pollfd / m_clients array
	int pfdnum = m_fd2pfdnum[fd];
	DPRINT(("del(%d): pfdnum %d m_pfds_used %d on entry\n", fd, pfdnum, m_pfds_used));
	if (pfdnum == -1)
		return ENOENT;
	assert(pfdnum >= 0);
	assert(pfdnum < m_pfds_used);

	// Remove from arrays indexed by pfdnum.  Close up hole so poll() doesn't barf.
	if (pfdnum != m_pfds_used - 1) {
		m_clients[pfdnum] = m_clients[m_pfds_used - 1];
		m_pfds[pfdnum] = m_pfds[m_pfds_used - 1];
		m_fd2pfdnum[m_pfds[pfdnum].fd] = pfdnum;
	}
	m_clients[m_pfds_used - 1] = NULL;
	m_pfds[m_pfds_used - 1].fd = -1;

	// Remove from arrays indexed by fd.
	m_fd2pfdnum[fd] = -1;

	// Update limits
	while (m_fd2client_used && (m_fd2pfdnum[m_fd2client_used - 1] == -1))
		m_fd2client_used--;
	m_pfds_used--;

	return 0;
}

int Poller_poll::setMask(int fd, short eventmask)
{
	int i = m_fd2pfdnum[fd];
	if (i == -1) {
		DPRINT(("setMask(fd %d, %x): fd no longer in Poller\n", fd, eventmask));
		return ENOENT;
	}
	assert(i >= 0);
	assert(i < m_pfds_used);
	m_pfds[i].events = eventmask;

	DPRINT(("setMask(%d, %x): new mask %x\n", fd, eventmask, m_pfds[i].events));

	return 0;
}

int Poller_poll::orMask(int fd, short eventmask)
{
	int i = m_fd2pfdnum[fd];
	assert(i >= 0);
	assert(i < m_pfds_used);
	m_pfds[i].events |= eventmask;

	DPRINT(("orMask(%d, %x): new mask %x\n", fd, eventmask, m_pfds[i].events));

	return 0;
}

int Poller_poll::andMask(int fd, short eventmask)
{
	int i = m_fd2pfdnum[fd];
	assert(i >= 0);
	assert(i < m_pfds_used);
	m_pfds[i].events &= eventmask;

	DPRINT(("andMask(%d, %x): new mask %x\n", fd, eventmask, m_pfds[i].events));

	return 0;
}

/**
 Sleep at most timeout_millisec waiting for an I/O readiness event
 on the file descriptors we're watching.  Fills internal array
 of readiness events.  Call getNextEvent() repeatedly to read its
 contents.
 @return 0 on success, EWOULDBLOCK if no events ready
 */
int Poller_poll::waitForEvents(int timeout_millisec)
{
	int err;

	// Wait for I/O events the clients are interested in.
	m_rfds = poll(m_pfds, m_pfds_used, timeout_millisec);
	if (m_rfds == -1) {
		err = errno;
		m_cur_pfdnum = -1;
		DPRINT(("waitForEvents: poll() returned -1, errno %d\n", err));
		return err;
	}
	m_cur_pfdnum = m_pfds_used;

	LOG_TRACE(("waitForEvents: got %d events\n", m_rfds));
	return m_rfds ? 0 : EWOULDBLOCK;
}

/**
 Get the next event that was found by waitForEvents.
 @return 0 on success, EWOULDBLOCK if no more events
 */
int Poller_poll::getNextEvent(PollEvent *e)
{
	if (m_rfds < 1)
		return EWOULDBLOCK;

	// Iterate downwards because otherwise if notifypollEvent()
	// calls add() or del(), we might skip an fd or process one
	// that hasn't been through poll() yet.
	if (m_cur_pfdnum > m_pfds_used)
		m_cur_pfdnum = m_pfds_used;
	while (--m_cur_pfdnum > -1) {
		if (m_pfds[m_cur_pfdnum].revents)
			break;
	}
	if (m_cur_pfdnum == -1)
		return EWOULDBLOCK;

	int fd = m_pfds[m_cur_pfdnum].fd;

	// Sanity checks
	assert((0 <= fd) && (fd < m_fd2client_used));
	int j = m_fd2pfdnum[fd];
	assert((0 <= j) && (j < m_pfds_used));
	assert(j == m_cur_pfdnum);

	e->fd = fd;
	e->revents = m_pfds[m_cur_pfdnum].revents;
	e->client = m_clients[j];

	m_rfds--;
	LOG_TRACE(("getNextEvent: fd %d revents %x j %d m_rfds %d\n",
		e->fd, e->revents, j, m_rfds));

	return 0;
}
int Poller_poll::waitAndDispatchEvents(int timeout_millisec)
{
	int err;
	PollEvent event;

	err = waitForEvents(timeout_millisec);
	if (err)
		return err;

	// Pump any network traffic into the appropriate Clients
	while (m_rfds > 0) {
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
