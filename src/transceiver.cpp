/*!
 * @file       transceiver.cpp
 * @brief      Defines the Fastcgipp::Transceiver class
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 3, 2017
 * @copyright  Copyright &copy; 2017 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */

/*******************************************************************************
* Copyright (C) 2017 Eddie Carle [eddie@isatec.ca]                             *
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

#include "fastcgi++/transceiver.hpp"

#include "fastcgi++/log.hpp"
#if ! defined(FASTCGIPP_WINDOWS)
#include <unistd.h>
#endif
/*bool Fastcgipp::Transceiver::transmit()
{
    std::unique_ptr<Record> record;
	bool bSleep=false;
	int nCountDDD=0;
    while(!m_sendBuffer.empty())
    {
        {
            std::lock_guard<std::mutex> lock(m_sendBufferMutex);
            record = std::move(m_sendBuffer.front());
            m_sendBuffer.pop_front();
			if(m_sendBuffer.size() == 0)
			{
				bSleep=true;
			}
        }

        const ssize_t sent = record->socket.write(
                record->read,
                record->data.end()-record->read);
        if(sent>=0)
        {
            record->read += sent;
            if(record->read != record->data.end())
            {
                {
                    std::lock_guard<std::mutex> lock(m_sendBufferMutex);
                    m_sendBuffer.push_front(std::move(record));
                }
                return false;
            }
#if FASTCGIPP_LOG_LEVEL > 3
            ++m_recordsSent;
#endif
            if(record->kill)
            {
                record->socket.close();
                m_receiveBuffers.erase(record->socket);
#if FASTCGIPP_LOG_LEVEL > 3
                ++m_connectionKillCount;
#endif
            }
        }
    }
	if(bSleep)
	{
		++nCountDDD;
	}
    return true;
}*/
bool Fastcgipp::Transceiver::transmit()
{
	//bool bSleep=false;
	int nCountDDD=0;
	socket_t lastSocket=-1;
	std::set<Socket> eraseRecvBufferSock;
    while(!m_sendBuffer.empty())
    {
        if(m_sendBufferSize < m_maxSendBufferSize)
        {
            std::unique_ptr<Record> record;
            {
                std::lock_guard<std::mutex> lock(m_sendBufferMutex);
                std::map<socket_t,std::list<std::unique_ptr<Record>>>::const_iterator iter;
                if(lastSocket == -1)
                {
                    iter=m_sendBuffer.begin();
                }
                else
                {
                    iter=m_sendBuffer.lower_bound(lastSocket);
                    if(iter == m_sendBuffer.end())
                    {
                        iter=m_sendBuffer.begin();
                    }
                    else
                    {
                        if(iter->first == lastSocket)//if not equal,direct use nextsocket
                        {
                            ++iter;
                        }
                        if(iter == m_sendBuffer.end())
                        {
                            iter=m_sendBuffer.begin();
                        }
                    }
                }
                lastSocket=iter->first;
                nCountDDD=iter->second.size();
                ++nCountDDD;
                --nCountDDD;
                record = std::move(m_sendBuffer[lastSocket].front());
                m_sendBuffer[lastSocket].pop_front();
                if(m_sendBuffer[lastSocket].empty())
                {
                    m_sendBuffer.erase(lastSocket);
                }
            }
            ssize_t sent =-1;
            int dataSize = record->data.end()-record->read;
            while(record->read != record->data.end())
            {
                sent = record->socket.write(
                    record->read,
                    record->data.end()-record->read);
                if(sent < 0)
                {
                    break;
                }
                if(sent == 0)
                {
                    continue;
                }
                record->read+=sent;
            }
            m_sendBufferSize-=dataSize;
            if(sent < 0)
            {
                record->socket.close();
                std::lock_guard<std::mutex> lock(m_sendBufferMutex);
                std::list<std::unique_ptr<Record>> &listRecord=m_sendBuffer[lastSocket];
                for(auto iterList=listRecord.begin();iterList != listRecord.end();++iterList)
                {
                    m_sendBufferSize-=(*iterList)->data.end()-(*iterList)->read;
                }
                m_sendBuffer.erase(lastSocket);
                eraseRecvBufferSock.insert(record->socket);
                continue;
            }
#if FASTCGIPP_LOG_LEVEL > 3
            ++m_recordsSent;
#endif
            if(record->kill)
            {
                record->socket.close();
                std::lock_guard<std::mutex> lock(m_sendBufferMutex);
                std::list<std::unique_ptr<Record>> &listRecord=m_sendBuffer[lastSocket];
                for(auto iterList=listRecord.begin();iterList != listRecord.end();++iterList)
                {
                    m_sendBufferSize-=(*iterList)->data.end()-(*iterList)->read;
                }
                m_sendBuffer.erase(lastSocket);
                eraseRecvBufferSock.insert(record->socket);
#if FASTCGIPP_LOG_LEVEL > 3
                ++m_connectionKillCount;
#endif
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(m_sendBufferMutex);
            int nn=m_sendBufferSize;
            ++nn;
            --nn;
            lastSocket = -1;
            while(!m_sendBuffer.empty())
            {
                std::unique_ptr<Record> record;
                {
                    std::map<socket_t,std::list<std::unique_ptr<Record>>>::const_iterator iter;
                    if(lastSocket == -1)
                    {
                        iter=m_sendBuffer.begin();
                    }
                    else
                    {
                        iter=m_sendBuffer.lower_bound(lastSocket);
                        if(iter == m_sendBuffer.end())
                        {
                            iter=m_sendBuffer.begin();
                        }
                        else
                        {
                            if(iter->first == lastSocket)//if not equal,direct use nextsocket
                            {
                                ++iter;
                            }
                            if(iter == m_sendBuffer.end())
                            {
                                iter=m_sendBuffer.begin();
                            }
                        }
                    }
                    lastSocket=iter->first;
                    nCountDDD=iter->second.size();
                    ++nCountDDD;
                    --nCountDDD;
                    record = std::move(m_sendBuffer[lastSocket].front());
                    m_sendBuffer[lastSocket].pop_front();
                    if(m_sendBuffer[lastSocket].empty())
                    {
                        m_sendBuffer.erase(lastSocket);
                    }
                }
                ssize_t sent =-1;
                int dataSize= record->data.end()-record->read;
                while(record->read != record->data.end())
                {
                    sent = record->socket.write(
                        record->read,
                        record->data.end()-record->read);
                    if(sent < 0)
                    {
                        break;
                    }
                    if(sent == 0)
                    {
                        continue;
                    }
                    record->read+=sent;
                }
                m_sendBufferSize-=dataSize;
                if(sent < 0)
                {
                    record->socket.close();
                    std::list<std::unique_ptr<Record>> &listRecord=m_sendBuffer[lastSocket];
                    for(auto iterList=listRecord.begin();iterList != listRecord.end();++iterList)
                    {
                        m_sendBufferSize-=(*iterList)->data.end()-(*iterList)->read;
                    }
                    m_sendBuffer.erase(lastSocket);
                    eraseRecvBufferSock.insert(record->socket);
                    continue;
                }
#if FASTCGIPP_LOG_LEVEL > 3
                ++m_recordsSent;
#endif
                if(record->kill)
                {
                    record->socket.close();
                    std::list<std::unique_ptr<Record>> &listRecord=m_sendBuffer[lastSocket];
                    for(auto iterList=listRecord.begin();iterList != listRecord.end();++iterList)
                    {
                        m_sendBufferSize-=(*iterList)->data.end()-(*iterList)->read;
                    }
                    m_sendBuffer.erase(lastSocket);
                    eraseRecvBufferSock.insert(record->socket);
#if FASTCGIPP_LOG_LEVEL > 3
                    ++m_connectionKillCount;
#endif
                }
                if(m_sendBuffer.empty())
                {
                    ++nCountDDD;
                    --nCountDDD;
                    int nn=m_sendBufferSize;
                    ++nn;
                    --nn;
                }
            }
        }
    }
    if(!eraseRecvBufferSock.empty())
    {
        std::lock_guard<std::mutex> lock(m_recvBufferMutex);
        for(auto iter=eraseRecvBufferSock.begin();iter != eraseRecvBufferSock.end();++iter)
        {
            m_receiveBuffers.erase(*iter);
        }
    }
    int nn=m_sendBufferSize;
    ++nn;
    --nn;
    return true;
}

/*void Fastcgipp::Transceiver::handler()
{
    bool flushed=false;
    Socket socket;

    while(!m_terminate && !(m_stop && m_sockets.size()==0))
    {
        socket = m_sockets.poll(flushed);
        receive(socket);
        flushed = transmit();
    }
}*/

void Fastcgipp::Transceiver::handler()
{
    std::unique_lock<std::mutex> lock(m_wakeMutex);
    while(!m_terminate && !(m_stop && m_sockets.size()==0))
    {
        m_wakeSend.wait(lock);
        transmit();
    }
}
void Fastcgipp::Transceiver::recvHandler()
{
    Socket socket;

    while(!m_terminate && !(m_stop && m_sockets.size()==0))
    {
        socket = m_sockets.poll(true);
        receive(socket);
    }
}
void Fastcgipp::Transceiver::stop()
{
    m_stop=true;
    m_sockets.accept(false);
}

void Fastcgipp::Transceiver::terminate()
{
    m_terminate=true;
    m_sockets.wake();
}

void Fastcgipp::Transceiver::start()
{
    m_stop=false;
    m_terminate=false;
    m_sockets.accept(true);
    if(!m_threadRecv.joinable())
    {
        std::thread thread(&Fastcgipp::Transceiver::recvHandler, this);
        m_threadRecv.swap(thread);
    }
    if(!m_thread.joinable())
    {
        std::thread thread(&Fastcgipp::Transceiver::handler, this);
        m_thread.swap(thread);
    }
}

void Fastcgipp::Transceiver::join()
{
    if(m_threadRecv.joinable())
    {
        m_threadRecv.join();
    }
    if(m_thread.joinable())
    {
        m_thread.join();
    }
}

Fastcgipp::Transceiver::Transceiver(
        const std::function<void(Protocol::RequestId, Message&&)> sendMessage):
    m_sendBuffer()
    ,m_sendBufferSize(0)
    ,m_maxSendBufferSize(10*1024*1024)
    ,m_sendMessage(sendMessage)
#if FASTCGIPP_LOG_LEVEL > 3
    ,m_connectionKillCount(0),
    m_connectionRDHupCount(0),
    m_recordsSent(0),
    m_recordsQueued(0),
    m_recordsReceived(0)
#endif
{
    DIAG_LOG("Transceiver::Transciever(): Initialized")
}

void Fastcgipp::Transceiver::receive(Socket& socket)
{
    if(socket.valid())
    {
        std::shared_ptr<Block> pBuffer;
        {
            std::lock_guard<std::mutex> lock(m_recvBufferMutex);
            if(m_receiveBuffers.find(socket) == m_receiveBuffers.end())
            {
                m_receiveBuffers[socket].reset(new Block());
            }
            pBuffer=m_receiveBuffers[socket];
        }
        Block &buffer=*pBuffer;
        // Are we receiving a header?
        if(buffer.size() < sizeof(Protocol::Header))
        {
            buffer.reserve(sizeof(Protocol::Header));

            const ssize_t read = socket.read(
                    buffer.begin()+buffer.size(),
                    buffer.reserve()-buffer.size());
            if(read<0)
            {
                cleanupSocket(socket);
                return;
            }
            buffer.size(buffer.size() + read);
            if(buffer.size() < sizeof(Protocol::Header))
                return;
        }

        if(buffer.size() == sizeof(Protocol::Header))
            buffer.reserve(sizeof(Protocol::Header)
                +reinterpret_cast<Protocol::Header*>(
                    buffer.begin())->contentLength
                +reinterpret_cast<Protocol::Header*>(
                    buffer.begin())->paddingLength);

        Protocol::Header& header
            = *reinterpret_cast<Protocol::Header*>(buffer.begin());

        const ssize_t read = socket.read(
                buffer.begin()+buffer.size(),
                buffer.reserve()-buffer.size());

        if(read<0)
        {
            cleanupSocket(socket);
            return;
        }
        buffer.size(buffer.size() + read);
        if(buffer.size() < buffer.reserve())
            return;

        Message message;
        message.data = std::move(buffer);
        /*pBuffer.reset();
        {
            std::lock_guard<std::mutex> lock(m_recvBufferMutex);
            m_receiveBuffers.erase(socket);
        }*/
        m_sendMessage(
                Protocol::RequestId(header.fcgiId, socket),
                std::move(message));
#if FASTCGIPP_LOG_LEVEL > 3
        ++m_recordsReceived;
#endif
    }
}

void Fastcgipp::Transceiver::cleanupSocket(const Socket& socket)
{
    {
        std::lock_guard<std::mutex> lock(m_recvBufferMutex);
        m_receiveBuffers.erase(socket);
    }
    m_sendMessage(
            Fastcgipp::Protocol::RequestId(Protocol::badFcgiId, socket),
            Message());
    socket.close();
#if FASTCGIPP_LOG_LEVEL > 3
    ++m_connectionRDHupCount;
#endif
}

void Fastcgipp::Transceiver::send(
        const Socket& socket,
        Block&& data,
        bool kill)
{
    std::unique_ptr<Record> record(new Record(
                socket,
                std::move(data),
                kill,false));
	int nCount=0;
	/*old{
		std::lock_guard<std::mutex> lock(m_sendBufferMutex);
		nCount=m_sendBuffer.size();
		m_sendBuffer.push_back(std::move(record));
	}*/
	//new
    int sendDataSize=record->data.size();
	{
		std::lock_guard<std::mutex> lock(m_sendBufferMutex);
		m_sendBuffer[socket.getHandle()].push_back(std::move(record));
        for(auto iterMap=m_sendBuffer.begin();iterMap != m_sendBuffer.end();++iterMap)
        {
            nCount+=iterMap->second.size();
        }
	}
	m_sendBufferSize+=sendDataSize;
	++nCount;
	--nCount;
    m_sockets.wake();
    m_wakeSend.notify_one();
#if FASTCGIPP_LOG_LEVEL > 3
    ++m_recordsQueued;
#endif
}
void Fastcgipp::Transceiver::send2(
        const Socket& socket,
        Block&& data,
        bool kill)
{
    send(socket,std::move(data),kill);
}


Fastcgipp::Transceiver::~Transceiver()
{
    terminate();
    DIAG_LOG("Transceiver::~Transceiver(): Locally closed sockets ==== " \
            << m_connectionKillCount)
    DIAG_LOG("Transceiver::~Transceiver(): Remotely closed sockets === " \
            << m_connectionRDHupCount)
    DIAG_LOG("Transceiver::~Transceiver(): Remaining receive buffers = " \
            << m_receiveBuffers.size())
    DIAG_LOG("Transceiver::~Transceiver(): Records queued === " \
            << m_recordsQueued)
    DIAG_LOG("Transceiver::~Transceiver(): Records sent ===== " \
            << m_recordsSent)
    DIAG_LOG("Transceiver::~Transceiver(): Records received = " \
            << m_recordsReceived)
}
