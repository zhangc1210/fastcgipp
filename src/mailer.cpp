/*!
 * @file       mailer.cpp
 * @brief      Defines types for sending emails
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 12, 2019
 * @copyright  Copyright &copy; 2019 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */
/*******************************************************************************
* Copyright (C) 2019 Eddie Carle [eddie@isatec.ca]                             *
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

#include "fastcgi++/mailer.hpp"
#include "fastcgi++/log.hpp"

#include <chrono>

void Fastcgipp::Mail::Mailer::handler()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while(!m_terminate && !(m_stop && m_queue.empty() && !inEmail()))
    {
        if(m_state == ERROR)
        {
            using namespace std::chrono_literals;
            m_wake.wait_for(lock, m_retry*1s);
            m_state = DISCONNECTED;

            if(m_terminate || (m_stop && m_queue.empty() && !inEmail()))
                break;
        }

        if(!inEmail() && !m_queue.empty())
        {
            m_email = std::move(m_queue.front());
            m_queue.pop();
        }

        lock.unlock();

        if(inEmail() && !m_socket.valid())
        {
            m_line.clear();
            m_socket = m_socketGroup.connect(m_host.c_str(), m_port.c_str());
            if(!m_socket.valid())
            {
                ERROR_LOG("Error connecting to SMTP server.")
                m_state = ERROR;
                m_socket.close();
                lock.lock();
                continue;
            }
            else
                m_state = CONNECTED;
        }

        if(m_socketGroup.poll(true) == m_socket)
        {
            char buffer;
            while(m_socket.read(&buffer, 1) == 1)
            {
                if(buffer == '\n')
                    break;
                m_line.push_back(buffer);
            }

            if(buffer == '\n')
            {
                if(m_line.back() == '\r')
                    m_line.pop_back();

                switch(m_state)
                {
                    case CONNECTED:
                    {
                        if(m_line.size() >= 4 && m_line.substr(0,4) == "220 ")
                        {
                            m_line = "EHLO ";
                            m_line += m_origin + '\n';
                            if(m_socket.write(m_line.data(), m_line.size())
                                    != static_cast<unsigned>(m_line.size()))
                            {
                                ERROR_LOG("Error sending EHLO command to SMTP "\
                                        "server.")
                            }
                            else
                            {
                                m_state = EHLO;
                                break;
                            }
                        }
                        else
                        {
                            ERROR_LOG("Bad reply from SMTP server after "\
                                    "connecting: " << m_line.c_str())
                        }
                        m_socket.close();
                        m_state = ERROR;
                        break;
                    }

                    case EHLO:
                    {
                        if(m_line == "250-8BITMIME")
                        {
                            m_state = EIGHTBIT;
                            break;
                        }
                        else if(m_line.size()>=4 && m_line.substr(0,3)=="250")
                        {
                            if(m_line[3] == ' ')
                            {
                                ERROR_LOG("SMTP server does not support 8BITMIME.")
                            }
                            else if(m_line[3] == '-')
                                break;
                            else
                                ERROR_LOG("Bad reply from SMTP server after "\
                                        "EHLO: " << m_line.c_str())
                        }
                        else
                        {
                            ERROR_LOG("Bad reply from SMTP server after EHLO: "\
                                    << m_line.c_str())
                        }
                        m_socket.close();
                        m_state = ERROR;
                        break;
                    }

                    case EIGHTBIT:
                    {
                        if(m_line.size()>=4 && m_line.substr(0,3)=="250")
                        {
                            if(m_line[3] == ' ')
                            {
                                m_line = "MAIL FROM: <";
                                m_line += m_email.from + ">\n";
                                if(m_socket.write(m_line.data(), m_line.size())
                                        != static_cast<unsigned>(m_line.size()))
                                {
                                    ERROR_LOG("Error sending MAIL command to "\
                                            "SMTP server.")
                                }
                                else
                                {
                                    m_state = MAIL;
                                    break;
                                }
                            }
                            else if(m_line[3] == '-')
                                break;
                        }
                        else
                        {
                            ERROR_LOG("Bad reply from SMTP server after EHLO: "\
                                    << m_line.c_str())
                        }
                        m_socket.close();
                        m_state = ERROR;
                        break;
                    }

                    case MAIL:
                    {
                        if(m_line.size() >= 4 && m_line.substr(0,4) == "250 ")
                        {
                            m_line = "RCPT TO: <";
                            m_line += m_email.to + ">\n";
                            if(m_socket.write(m_line.data(), m_line.size())
                                    != static_cast<unsigned>(m_line.size()))
                            {
                                ERROR_LOG("Error sending RCPT command to SMTP "\
                                        "server.")
                            }
                            else
                            {
                                m_state = RCPT;
                                break;
                            }
                        }
                        else
                        {
                            ERROR_LOG("Bad reply from SMTP server after MAIL: "\
                                    << m_line.c_str())
                        }
                        m_socket.close();
                        m_state = ERROR;
                        break;
                    }

                    case RCPT:
                    {
                        if(m_line.size() >= 4 && m_line.substr(0,4) == "250 ")
                        {
                            if(m_socket.write("DATA\n", 5) != 5)
                            {
                                ERROR_LOG("Error sending DATA command to SMTP "\
                                        "server.")
                            }
                            else
                            {
                                m_state = DATA;
                                break;
                            }
                        }
                        else
                        {
                            ERROR_LOG("Bad reply from SMTP server after RCPT: "\
                                    << m_line.c_str())
                        }
                        m_socket.close();
                        m_state = ERROR;
                        break;
                    }

                    case DATA:
                    {
                        if(m_line.size() >= 4 && m_line.substr(0,4) == "354 ")
                        {
                            bool success=true;
                            for(const auto& chunk: m_email.body)
                                if(m_socket.write(
                                            chunk.data.get(),
                                            chunk.size) != chunk.size)
                                {
                                    ERROR_LOG("Error sending data chunk "\
                                            "to SMTP server.")
                                    success=false;
                                    break;
                                }
                            if(success)
                            {
                                if(m_socket.write("\r\n.\r\n", 5) != 5)
                                {
                                    ERROR_LOG("Error sending CRLF.CRLF to "\
                                            "SMTP server.")
                                }
                                else
                                {
                                    m_state = DUMP;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            ERROR_LOG("Bad reply from SMTP server after RCPT: "\
                                    << m_line.c_str())
                        }
                        m_socket.close();
                        m_state = ERROR;
                        break;
                    }

                    case DUMP:
                    {
                        if(m_line.size() >= 4 && m_line.substr(0,4) == "250 ")
                        {
                            purgeEmail();
                            if(m_socket.write("QUIT\n", 5) != 5)
                            {
                                ERROR_LOG("Error sending QUIT command to SMTP "\
                                        "server.")
                            }
                            else
                            {
                                m_state = QUIT;
                                break;
                            }
                        }
                        else
                        {
                            ERROR_LOG("Bad reply from SMTP server after data "\
                                    "insertion: " << m_line.c_str())
                        }
                        m_socket.close();
                        m_state = ERROR;
                        break;
                    }

                    case QUIT:
                    {
                        if(!(m_line.size() >= 4 && m_line.substr(0,4) == "221 "))
                        {
                            ERROR_LOG("Bad reply from SMTP server after QUIT: "\
                                    << m_line.c_str())
                            m_state = ERROR;
                        }
                        else
                            m_state = DISCONNECTED;
                        m_socket.close();
                        break;
                    }

                    case ERROR:
                    case DISCONNECTED:
                        ;// DO NOTHING
                }
                m_line.clear();
            }
        }

        lock.lock();
    }
}

void Fastcgipp::Mail::Mailer::stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stop=true;
    m_socketGroup.wake();
    m_wake.notify_all();
}

void Fastcgipp::Mail::Mailer::terminate()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_terminate=true;
    m_socketGroup.wake();
    m_wake.notify_all();
}

void Fastcgipp::Mail::Mailer::start()
{
    if(!m_thread.joinable())
    {
        m_stop=false;
        m_terminate=false;
        std::thread thread(&Fastcgipp::Mail::Mailer::handler, this);
        m_thread.swap(thread);
    }
}

void Fastcgipp::Mail::Mailer::join()
{
    if(m_thread.joinable())
        m_thread.join();
}

Fastcgipp::Mail::Mailer::~Mailer()
{
}

void Fastcgipp::Mail::Mailer::init(
        const char* host,
        const char* origin,
        const unsigned short port,
        unsigned retryInterval)
{
    if(!m_initialized)
    {
        m_host = host;
        m_origin = origin;
        m_port = std::to_string(port);
        m_retry = retryInterval;
        m_initialized = true;
    }
}

void Fastcgipp::Mail::Mailer::queue(Email_base& email)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(email.data());
    m_socketGroup.wake();
}
