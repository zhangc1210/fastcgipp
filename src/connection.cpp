/*!
 * @file       connection.cpp
 * @brief      Defines the Fastcgipp::SQL::SQL::Connection class
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       September 30, 2018
 * @copyright  Copyright &copy; 2018 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */

/*******************************************************************************
* Copyright (C) 2018 Eddie Carle [eddie@isatec.ca]                             *
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

#include <fastcgi++/sql/connection.hpp>

#include <unistd.h>
#include <cstring>

void Fastcgipp::SQL::Connection::handler()
{
    // DO SERVER CONNECTIONS

    while(!m_terminate && !m_stop)
    {
        // Do we have a free connection?
        for(auto& connection: m_connections)
            if(connection.second.idle)
            {
                auto& conn = connection.second.connection;
                auto& query = connection.second.query;

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if(m_queries.empty())
                        break;
                    query = m_queries.front();
                    m_queries.pop();
                }

                int result;
                if(query.parameters)
                    result = PQsendQuery(
                            conn,
                            query.statement);
                else
                    result = PQsendQueryParams(
                            conn,
                            query.statement,
                            query.parameters->size(),
                            query.parameters->oids(),
                            query.parameters->raws(),
                            query.parameters->sizes(),
                            query.parameters->formats(),
                            1);

                if(result != 1)
                {
                    ERROR_LOG("Unable to dispatch SQL query: " \
                            << PQerrorMessage(conn))
                    // HANDLE THE ERROR. RECONNECT?
                }
                else
                    connection.second.idle = false;
            }

        // Let's see if any data is waiting for us from the connections
        const auto pollResult = m_poll.poll(true);
        if(pollResult)
        {
            if(pollResult.socket() == m_wakeSockets[1])
            {
                // Looks like it's time to wake up
                if(pollResult.onlyIn())
                {
                    char x[256];
                    if(read(m_wakeSockets[1], x, 256)<1)
                        FAIL_LOG("Unable to read out of SQL::Connection wakeup socket: " << \
                                std::strerror(errno))
                    continue;
                }
                else if(pollResult.hup() || pollResult.rdHup())
                    FAIL_LOG("The wakeup socket in SQL::Connection hung up.")
                else if(pollResult.err())
                    FAIL_LOG("Error in the SQL::Connection wakeup socket.")
            }
            else
            {
                auto connId = m_connections.find(pollResult.socket());
                if(connId == m_connections.end())
                {
                    ERROR_LOG("Poll gave fd " << pollResult.socket() \
                            << " which isn't in m_connections.")
                    m_poll.del(pollResult.socket());
                    close(pollResult.socket());
                    continue;
                }

                if(pollResult.in())
                {
                    // HANDLE QUERY DATA RECIEVED
                    continue;
                }

                if(pollResult.rdHup())
                    WARNING_LOG("SQL::Connection socket " \
                            << pollResult.socket() << " remotely hung up. " \
                            << "Reconnecting.")
                else if(pollResult.hup())
                    WARNING_LOG("SQL::Connection socket " \
                            << pollResult.socket() << " hung up. Reconnecting")
                else if(pollResult.err())
                    ERROR_LOG("Error in SQL::Connection socket " \
                            << pollResult.socket() << ". Reconnecting")
                else
                    FAIL_LOG("Got a weird event 0x" << std::hex \
                            << pollResult.events() \
                            << " on SQL::Connection poll." )
                // HANDLE RECONNECTION
            }
        }
    }

    if(!m_terminate)
    {
        // CLOSE SERVER CONNECTIONS
    }
}

void Fastcgipp::SQL::Connection::stop()
{
    m_stop=true;
    wake();
}

void Fastcgipp::SQL::Connection::terminate()
{
    m_terminate=true;
    wake();
}

void Fastcgipp::SQL::Connection::start()
{
    m_stop=false;
    m_terminate=false;
    if(!m_thread.joinable())
    {
        std::thread thread(&Fastcgipp::SQL::Connection::handler, this);
        m_thread.swap(thread);
    }
}

void Fastcgipp::SQL::Connection::join()
{
    if(m_thread.joinable())
        m_thread.join();
}

Fastcgipp::SQL::Connection::~Connection()
{
    terminate();
}

void Fastcgipp::SQL::Connection::wake()
{
    static const char x=0;
    if(write(m_wakeSockets[0], &x, 1) != 1)
        FAIL_LOG("Unable to write to wakeup socket in SQL::Connection: " \
                << std::strerror(errno))
}

void Fastcgipp::SQL::Connection::init(
        const std::string host,
        const std::string db,
        const std::string username,
        const std::string password,
        const unsigned short port,
        const unsigned concurrency,
        int messageType)
{
    if(!m_initialized)
    {
        socketpair(AF_UNIX, SOCK_STREAM, 0, m_wakeSockets);
        m_poll.add(m_wakeSockets[1]);
        m_host = host;
        m_db = db;
        m_username = username;
        m_password = password;
        m_port = port;
        m_concurrency = concurrency;
        m_messageType = messageType;
        m_initialized = true;
    }
}

void Fastcgipp::SQL::Connection::query(const Query& query)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queries.push(query);
    wake();
}
