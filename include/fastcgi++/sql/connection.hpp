/*!
 * @file       connection.hpp
 * @brief      Declares the Fastcgipp::SQL::Connection class
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

#ifndef FASTCGIPP_CONNECTION_HPP
#define FASTCGIPP_CONNECTION_HPP

#include <queue>
#include <map>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>

#include <fastcgi++/sockets.hpp>
#include <fastcgi++/sql/query.hpp>

#include <postgres.h>
#undef ERROR
// I sure would like to know who thought it clever to define the macro ERROR in
// these postgresql header files

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ SQL facilities
    namespace SQL
    {
        //! Handles low level communication with "the other side"
        /*!
         * This class handles the sending/receiving/buffering of data through the OS
         * level sockets and also the creation/destruction of the sockets
         * themselves.
         *
         * @date    May 4, 2018
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        class Connection
        {
        public:
            //! General connection handler
            void handler();

            //! Call from any thread to stop the handler() thread
            /*!
             * Calling this thread will signal the handler() thread to
             * gracefully stop itself. This means it waits until all
             * connections are properly closed.
             *
             * @sa join()
             */
            void stop();

            //! Call from any thread to terminate the handler() thread
            /*!
             * Calling this thread will signal the handler() thread to
             * immediately terminate itself. This means it doesn't wait until
             * all connections are closed.
             *
             * @sa join()
             */
            void terminate();

            //! Call from any thread to start the handler() thread
            /*!
             * If the thread is already running this will do nothing.
             */
            void start();

            //! Block until a stop() or terminate() is completed
            void join();

            //! Queue up a query
            void query(const Query& query);

            //! Initialize the connection
            void init(
                    const std::string host,
                    const std::string db,
                    const std::string username,
                    const std::string password,
                    const unsigned short port=5432,
                    const unsigned concurrency=1,
                    int messageType=2);

            ~Connection();

            Connection():
                m_initialized(false)
            {}

        private:
            //! Call this to wakeup the thread if it's sleeping
            void wake();

            //! True if we're initialized
            bool m_initialized;

            struct Conn
            {
                bool idle;
                PGconn* connection;
                Query query;
            };

            //! Container associating sockets with their receive buffers
            std::map<socket_t, Conn> m_connections;

            //! %Buffer for transmitting data
            std::queue<Query> m_queries;

            //! True when handler() should be terminating
            std::atomic_bool m_terminate;

            //! True when handler() should be stopping
            std::atomic_bool m_stop;

            //! Thread our handler is running in
            std::thread m_thread;

            //! Always practice safe threading
            std::mutex m_mutex;

            //! A pair of sockets for wakeup purposes
            socket_t m_wakeSockets[2];

            //! Server host identifier
            std::string m_host;

            //! Database name
            std::string m_db;

            //! Server username
            std::string m_username;

            //! Server password
            std::string m_password;

            //! Server port
            unsigned short m_port;

            //! How many concurrent queries shall we allow?
            unsigned m_concurrency;

            //! Callback message type ID
            unsigned m_messageType;

            //! The poll group
            Poll m_poll;
        };
    }
}

#endif
