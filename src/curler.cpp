/*!
 * @file       curler.cpp
 * @brief      Defines types for sending ecurls
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 7, 2020
 * @copyright  Copyright &copy; 2020 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
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

#include "fastcgi++/curler.hpp"
#include "fastcgi++/log.hpp"

#include <curl/curl.h>
#include <unistd.h>
#include <cstring>

void Fastcgipp::Curler::handler()
{
    CURLM* const& multiHandle(reinterpret_cast<CURL* const&>(m_multiHandle));
    std::unique_lock<std::mutex> lock(m_mutex);
    int handles = 0;
    long timeout = -1;

    while(!m_terminate && !(m_stop && m_queue.empty() && m_handles.empty()))
    {
        if(!m_queue.empty() && m_handles.size() < m_concurrency)
        {
            while(!m_queue.empty() && m_handles.size() < m_concurrency)
            {
                CURL* const& handle(reinterpret_cast<CURL* const&>(
                            m_queue.front().handle()));
                m_handles.emplace(handle, m_queue.front());
                m_queue.pop();
                if(curl_multi_add_handle(multiHandle, handle))
                    FAIL_LOG("Unable to add curl handle to multi");
            }
            curl_multi_socket_action(
                    multiHandle,
                    CURL_SOCKET_TIMEOUT,
                    0,
                    &handles);
        }
        lock.unlock();

        if(handles)
            curl_multi_timeout(multiHandle, &timeout);
        else
            timeout = -1;
        const auto pollResult = m_poll.poll(timeout);
        if(pollResult.socket() == m_wakeSockets[1])
        {
            if(pollResult.onlyIn())
            {
                lock.lock();
                char x[256];
                if(read(m_wakeSockets[1], x, 256)<1)
                    FAIL_LOG("Unable to read out of Curler wakeup socket: " << \
                            std::strerror(errno))
                m_waking=false;
                continue;
            }
            else if(pollResult.hup() || pollResult.rdHup())
                FAIL_LOG("The Curler wakeup socket hung up.")
            else if(pollResult.err())
                FAIL_LOG("Error in the Curler wakeup socket.")
        }

        curl_multi_socket_action(
                multiHandle,
                pollResult.socket(),
                0,
                &handles);
        int messages = 1;
        while(messages)
        {
            const auto message = curl_multi_info_read(multiHandle, &messages);
            if(message && message->msg == CURLMSG_DONE)
            {
                const auto curlIt = m_handles.find(message->easy_handle);
                if(curlIt == m_handles.end())
                    FAIL_LOG("Curler returned an easy handle that doesn't exist");
                Curl_base curl = curlIt->second;
                m_handles.erase(curlIt);
                curl_multi_remove_handle(multiHandle, curl.handle());
                curl.callback();
            }
        }

        lock.lock();
    }
}
int Fastcgipp::Curler::socketCallback(
        void* handle,
        int socket,
        int action,
        void *userp,
        void *socketp)
{
    Curler& curler(*reinterpret_cast<Curler*>(userp));
    if(action == CURL_POLL_REMOVE)
        curler.m_poll.del(socket);
    else
        curler.m_poll.add(socket);
    return 0;
}

Fastcgipp::Curler::Curler(unsigned concurrency):
    m_concurrency(concurrency),
    m_multiHandle(curl_multi_init())
{
    socketpair(AF_UNIX, SOCK_STREAM, 0, m_wakeSockets);
    m_poll.add(m_wakeSockets[1]);

    CURLM* const& multiHandle(reinterpret_cast<CURL* const&>(m_multiHandle));
    curl_multi_setopt(
            multiHandle,
            CURLMOPT_SOCKETFUNCTION,
            Fastcgipp::Curler::socketCallback);
    curl_multi_setopt(
            multiHandle,
            CURLMOPT_SOCKETDATA,
            this);
}

Fastcgipp::Curler::~Curler()
{
    close(m_wakeSockets[0]);
    close(m_wakeSockets[1]);

    CURLM* const& multiHandle(reinterpret_cast<CURL* const&>(m_multiHandle));
    curl_multi_cleanup(multiHandle);
}

void Fastcgipp::Curler::stop()
{
    m_stop=true;
    wake();
}

void Fastcgipp::Curler::terminate()
{
    m_terminate=true;
    wake();
}

void Fastcgipp::Curler::start()
{
    if(!m_thread.joinable())
    {
        m_stop=false;
        m_terminate=false;
        m_waking=false;
        std::thread thread(&Fastcgipp::Curler::handler, this);
        m_thread.swap(thread);
    }
}

void Fastcgipp::Curler::join()
{
    if(m_thread.joinable())
        m_thread.join();
}

void Fastcgipp::Curler::queue(Curl_base& curl)
{
    curl.prepare();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(curl);
    wake();
}

void Fastcgipp::Curler::wake()
{
    if(!m_waking)
    {
        m_waking=true;
        static const char x=0;
        if(write(m_wakeSockets[0], &x, 1) != 1)
            FAIL_LOG("Unable to write to wakeup socket in Curler: " \
                    << std::strerror(errno))
    }
}
