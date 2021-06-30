/*!
 * @file       poll.cpp
 * @brief      Defines everything for interfaces with OS level sockets.
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 5, 2020
 * @copyright  Copyright &copy; 2020 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 *
 * It is this file, along with poll.hpp, that must be modified to make
 * fastcgi++ work on Windows.
 */

/*******************************************************************************
* Copyright (C) 2020 Eddie Carle [eddie@isatec.ca]                             *
*                                                                              *
* This file is part of fastcgi++.                                              *
*                                                                              *
* fastcgi++ is free software: you can redistribute it and/or modify it under   *
* the terms of the GNU Lesser General Public License as  published by the Free *
* Software Foundation, either version 3 of the License, or (at your option)    *
* any later version.                                                           *
*                                                                              *
* fastcgi++ is distributed in the hope that it will be useful, but WITHOUT ANY *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    *
* FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for     *
* more details.                                                                *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.           *
*******************************************************************************/

#include "fastcgi++/poll.hpp"
#include "fastcgi++/log.hpp"
#ifdef FASTCGIPP_LINUX
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#elif defined FASTCGIPP_UNIX
#include <algorithm>
#include <unistd.h>
#include <cstring>
#elif defined FASTCGIPP_WINDOWS
#include <algorithm>
#endif

extern int getLastSocketError();
#ifdef FASTCGIPP_LINUX
const unsigned Fastcgipp::Poll::Result::pollIn = EPOLLIN;
const unsigned Fastcgipp::Poll::Result::pollErr = EPOLLERR;
const unsigned Fastcgipp::Poll::Result::pollHup = EPOLLHUP;
const unsigned Fastcgipp::Poll::Result::pollRdHup = EPOLLRDHUP;
#elif defined FASTCGIPP_UNIX
const unsigned Fastcgipp::Poll::Result::pollIn = POLLIN;
const unsigned Fastcgipp::Poll::Result::pollErr = POLLERR;
const unsigned Fastcgipp::Poll::Result::pollHup = POLLHUP;
const unsigned Fastcgipp::Poll::Result::pollRdHup = POLLRDHUP;
#elif defined FASTCGIPP_WINDOWS
const unsigned Fastcgipp::Poll::Result::pollIn = POLLRDNORM;
const unsigned Fastcgipp::Poll::Result::pollErr = POLLERR;
const unsigned Fastcgipp::Poll::Result::pollHup = POLLHUP;
#endif

int Fastcgipp::closesocket(socket_t fd)
{
#if defined(FASTCGIPP_WINDOWS)
	return ::closesocket(fd);
#else
	return ::close(fd);
#endif
}
int Fastcgipp::shutdown(socket_t fd, bool bRead, bool Write)
{
	int nOper = 0;
#if defined(FASTCGIPP_WINDOWS)
	if (bRead && Write)
	{
		nOper = SD_BOTH;
	}
	else if (bRead)
	{
		nOper = SD_RECEIVE;
	}
	else if (Write)
	{
		nOper = SD_SEND;
	}
#else
	if (bRead && Write)
	{
		nOper = SHUT_RDWR;
	}
	else if (bRead)
	{
		nOper = SHUT_RD;
	}
	else if (Write)
	{
		nOper = SHUT_WR;
	}
#endif
	else
	{
		return -1;
	}
	return ::shutdown(fd, nOper);
}
bool Fastcgipp::setNonBlocking(socket_t fd)
{
#if defined(FASTCGIPP_WINDOWS)
	unsigned long nonblocking = 1;
	return ioctlsocket(fd, FIONBIO, &nonblocking) != SOCKET_ERROR;
#else
	return fcntl(
		fd,
		F_SETFL,
		fcntl(fd, F_GETFL) | O_NONBLOCK)
		>= 0;
#endif
}
void Fastcgipp::set_reuse(socket_t sock)
{
#if defined(FASTCGIPP_LINUX) || defined(FASTCGIPP_UNIX)
	int x = 1;
	if (::setsockopt(
		sock,
		SOL_SOCKET,
		SO_REUSEADDR,
		&x,
		sizeof(int)) != 0)
		WARNING_LOG("Socket setsockopt(SO_REUSEADDR, 1) error on fd " \
			<< sock << ": " << strerror(getLastSocketError()))
#elif defined(FASTCGIPP_WINDOWS)
	bool x = true;
	if (::setsockopt(
		sock,
		SOL_SOCKET,
		SO_REUSEADDR,
		(const char*)&x,
		sizeof(int)) != 0)
		WARNING_LOG("Socket setsockopt(SO_REUSEADDR, 1) error on fd " \
			<< sock << ": " << strerror(getLastSocketError()))
#else
	WARNING_LOG("SocketGroup::set_reuse_address(true) not implemented");
#endif
}
Fastcgipp::Poll::Poll()
#ifdef FASTCGIPP_LINUX
    :m_poll(epoll_create1(0))
#endif
{}

Fastcgipp::Poll::~Poll()
{
#ifdef FASTCGIPP_LINUX
    close(m_poll);
#endif
}

Fastcgipp::Poll::Result Fastcgipp::Poll::poll(int timeout)
{
	int pollResult;
#ifdef FASTCGIPP_LINUX
	epoll_event epollEvent;
	pollResult = epoll_wait(
		m_poll,
		&epollEvent,
		1,
		timeout);
#elif defined FASTCGIPP_UNIX
	pollResult = ::poll(
		m_poll.data(),
		m_poll.size(),
		timeout);
#elif defined FASTCGIPP_WINDOWS
	pollResult = ::WSAPoll(
		m_poll.data(),
		m_poll.size(),
		timeout);
#endif

	Result result;
	int err = getLastSocketError();
#if defined(FASTCGIPP_WINDOWS)
	if (pollResult < 0)
	{
		if (err == WSAENOTSOCK)
		{
			++err;
			--err;
		}
		FAIL_LOG("Error on poll: " << err)
	}
#else
	if (pollResult < 0 && err != EINTR)
	{
		const char* cError = std::strerror(err);
		FAIL_LOG("Error on poll: " << cError)
	}
#endif
    else if(pollResult>0)
    {
        result.m_data = true;
#ifdef FASTCGIPP_LINUX
        result.m_socket = epollEvent.data.fd;
        result.m_events = epollEvent.events;
#elif defined FASTCGIPP_UNIX
        const auto fd = std::find_if(
                m_poll.begin(),
                m_poll.end(),
                [] (const pollfd& x)
                {
                    return x.revents != 0;
                });
        if(fd == m_poll.end())
            FAIL_LOG("poll() gave a result >0 but no revents are non-zero")
        result.m_socket = fd->fd;
        result.m_events = fd->revents;
#elif defined FASTCGIPP_WINDOWS
		const auto fd = std::find_if(
			m_poll.begin(),
			m_poll.end(),
			[](const pollfd& x)
			{
				return x.revents != 0;
			});
		if (fd == m_poll.end())
			FAIL_LOG("poll() gave a result >0 but no revents are non-zero")
		result.m_socket = fd->fd;
		result.m_events = fd->revents;
		if (result.m_events == POLLNVAL)
		{
			result.m_socket = -1;
			result.m_data = false;
			m_poll.erase(fd);
		}
#endif
    }

    return result;
}

bool Fastcgipp::Poll::add(const socket_t socket)
{
#ifdef FASTCGIPP_LINUX
	epoll_event event;
	event.data.fd = socket;
	event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
	return epoll_ctl(m_poll, EPOLL_CTL_ADD, socket, &event) != -1;
#elif defined FASTCGIPP_UNIX
	const auto fd = std::find_if(
		m_poll.begin(),
		m_poll.end(),
		[&socket](const pollfd& x)
		{
			return x.fd == socket;
		});
	if (fd != m_poll.end())
		return false;

	m_poll.emplace_back();
	m_poll.back().fd = socket;
	m_poll.back().events = POLLIN | POLLRDHUP | POLLERR | POLLHUP;
	return true;
#elif defined FASTCGIPP_WINDOWS
	const auto fd = std::find_if(
		m_poll.begin(),
		m_poll.end(),
		[&socket](const pollfd& x)
		{
			return x.fd == socket;
		});
	if (fd != m_poll.end())
		return false;

	m_poll.emplace_back();
	m_poll.back().fd = socket;
	m_poll.back().events = Result::pollIn;
	return true;
#endif
}

bool Fastcgipp::Poll::del(const socket_t socket)
{
#ifdef FASTCGIPP_LINUX
	return epoll_ctl(m_poll, EPOLL_CTL_DEL, socket, nullptr) != -1;
#elif defined FASTCGIPP_UNIX
	const auto fd = std::find_if(
		m_poll.begin(),
		m_poll.end(),
		[&socket](const pollfd& x)
		{
			return x.fd == socket;
		});
	if (fd == m_poll.end())
		return false;

	m_poll.erase(fd);
	return true;
#elif defined FASTCGIPP_WINDOWS
	const auto fd = std::find_if(
		m_poll.begin(),
		m_poll.end(),
		[&socket](const pollfd& x)
		{
			return x.fd == socket;
		});
	if (fd == m_poll.end())
		return false;

	m_poll.erase(fd);
	shutdown(socket);
	closesocket(socket);
	return true;
#endif
}
/*bool Fastcgipp::Poll::add(const socket_t socket)
{
#ifdef FASTCGIPP_LINUX
    epoll_event event;
    event.data.fd = socket;
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    return epoll_ctl(m_poll, EPOLL_CTL_ADD, socket, &event) != -1;
#elif defined FASTCGIPP_UNIX
    const auto fd = std::find_if(
            m_poll.begin(),
            m_poll.end(),
            [&socket] (const pollfd& x)
            {
                return x.fd == socket;
            });
    if(fd != m_poll.end())
        return false;

    m_poll.emplace_back();
    m_poll.back().fd = socket;
    m_poll.back().events = POLLIN | POLLRDHUP | POLLERR | POLLHUP;
    return true;
#elif defined FASTCGIPP_WINDOWS
	const auto fd = std::find_if(
		m_poll.begin(),
		m_poll.end(),
		[&socket](const pollfd& x)
		{
			return x.fd == socket;
		});
	if (fd != m_poll.end())
		return false;

	m_poll.emplace_back();
	m_poll.back().fd = socket;
	m_poll.back().events = Result::pollIn;
	return true;
#endif
}

bool Fastcgipp::Poll::del(const socket_t socket)
{
#ifdef FASTCGIPP_LINUX
    return epoll_ctl(m_poll, EPOLL_CTL_DEL, socket, nullptr) != -1;
#elif defined FASTCGIPP_UNIX
    const auto fd = std::find_if(
            m_poll.begin(),
            m_poll.end(),
            [&socket] (const pollfd& x)
            {
                return x.fd == socket;
            });
    if(fd == m_poll.end())
        return false;

    m_poll.erase(fd);
    return true;
#elif defined FASTCGIPP_WINDOWS
	const auto fd = std::find_if(
		m_poll.begin(),
		m_poll.end(),
		[&socket](const pollfd& x)
		{
			return x.fd == socket;
		});
	if (fd == m_poll.end())
		return false;

	m_poll.erase(fd);
	return true;
#endif
}*/
