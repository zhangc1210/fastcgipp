/*!
 * @file       connection.cpp
 * @brief      Defines the Fastcgipp::SQL::SQL::Connection class
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       October 5, 2018
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
    killAll();
    while(!m_terminate && !(m_stop && m_queue.empty()))
    {
        // Get connected
        if(!connected()) connect();

        // Do we have a free connection?
        for(
                auto connection=m_connections.begin();
                connection != m_connections.end();
                ++connection)
        {
            auto& idle = connection->second.idle;
            if(idle)
            {
                auto& conn = connection->second.connection;
                auto& query = connection->second.query;

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if(m_queue.empty())
                        break;
                    query = m_queue.front();
                    m_queue.pop_front();
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
                    kill(connection);
                }
                else
                {
                    PQflush(conn);
                    idle = false;
                }
            }
        }

        // Let's see if any data is waiting for us from the connections
        const auto pollResult = m_poll.poll(connected()?-1:m_retry);
        if(pollResult)
        {
            if(pollResult.socket() == m_wakeSockets[1])
            {
                // Looks like it's time to wake up
                if(pollResult.onlyIn())
                {
                    static char x[256];
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
                auto connection = m_connections.find(pollResult.socket());
                if(connection == m_connections.end())
                {
                    ERROR_LOG("Poll gave fd " << pollResult.socket() \
                            << " which isn't in m_connections.")
                    m_poll.del(pollResult.socket());
                    close(pollResult.socket());
                    continue;
                }

                if(pollResult.in())
                {
                    auto& conn = connection->second.connection;
                    auto& query = connection->second.query;
                    auto& idle = connection->second.idle;
                    if(idle)
                        ERROR_LOG("Recieved input data on SQL connection for "\
                                "which there is no active query")
                    else if(PQconsumeInput(conn) != 1)
                        ERROR_LOG("Error consuming SQL input: " \
                                << PQerrorMessage(conn))
                    else
                    {
                        PQflush(conn);
                        while(PQisBusy(conn) == 0)
                        {
                            PGresult* result = PQgetResult(conn);
                            if(result == nullptr)
                            {
                                // Query is complete
                                idle = true;
                                query.statement = nullptr;
                                query.parameters.reset();
                                query.results.reset();
                                query.callback(m_messageType);
                                break;
                            }

                            if(query.results->m_res == nullptr)
                                query.results->m_res = result;
                            else
                            {
                                WARNING_LOG("Multiple result sets received on"\
                                        " query. Discarding extras.")
                                PQclear(result);
                            }
                        }
                        continue;
                    }
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
                kill(connection);
            }
        }
    }
    killAll();
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
    if(!m_thread.joinable())
    {
        m_stop=false;
        m_terminate=false;
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
    killAll();
}

void Fastcgipp::SQL::Connection::wake()
{
    static const char x=0;
    if(write(m_wakeSockets[0], &x, 1) != 1)
        FAIL_LOG("Unable to write to wakeup socket in SQL::Connection: " \
                << std::strerror(errno))
}

void Fastcgipp::SQL::Connection::init(
        const char* host,
        const char* db,
        const char* username,
        const char* password,
        const unsigned concurrency,
        const unsigned short port,
        int messageType,
        unsigned retryInterval)
{
    if(!m_initialized)
    {
        socketpair(AF_UNIX, SOCK_STREAM, 0, m_wakeSockets);
        m_poll.add(m_wakeSockets[1]);
        m_host = host;
        m_db = db;
        m_username = username;
        m_password = password;
        m_concurrency = concurrency;
        m_port = std::to_string(port);
        m_messageType = messageType;
        m_retry = retryInterval*1000;
        m_initialized = true;
    }
}

bool Fastcgipp::SQL::Connection::query(const Query& query)
{
    if(!m_stop && connected())
    {
        if(query.parameters)
            query.parameters->build();

        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_back(query);
        wake();
        return true;
    }
    return false;
}

void Fastcgipp::SQL::Connection::connect()
{
    while(!connected())
    {
        Conn conn;
        conn.connection = PQsetdbLogin(
                m_host.c_str(),
                m_port.c_str(),
                nullptr,
                nullptr,
                m_db.c_str(),
                m_username.c_str(),
                m_password.c_str());
        if(conn.connection == nullptr)
        {
            ERROR_LOG("Error initiating connection to postgresql server.");
            break;
        }
        if(PQstatus(conn.connection) != CONNECTION_OK)
        {
            ERROR_LOG("Error connecting to postgresql server.");
            break;
        }
        if(PQsetnonblocking(conn.connection, 1) != 0)
        {
            ERROR_LOG("Error setting nonblock on postgresql connection.");
            break;
        }

        conn.idle = true;
        m_connections[PQsocket(conn.connection)] = conn;
    }
}

void Fastcgipp::SQL::Connection::kill(std::map<socket_t, Conn>::iterator& conn)
{
    PQfinish(conn->second.connection);
    m_poll.del(conn->first);
    if(!conn->second.idle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_front(conn->second.query);
    }
    m_connections.erase(conn);
}

void Fastcgipp::SQL::Connection::killAll()
{
    for(auto& connection: m_connections)
    {
        PQfinish(connection.second.connection);
        m_poll.del(connection.first);
    }
    m_connections.clear();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.clear();
}
