/*!
 * @file       fcgistreambuf.cpp
 * @brief      Defines the FcgiStreambuf class
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 6, 2020
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

#include "fastcgi++/fcgistreambuf.hpp"
#include "fastcgi++/log.hpp"

#include <codecvt>
#include <algorithm>

namespace Fastcgipp
{
    template <> bool
    Fastcgipp::FcgiStreambuf<wchar_t, std::char_traits<wchar_t>>::emptyBuffer()
    {
        const std::codecvt_utf8<wchar_t> converter;
        std::codecvt_base::result result;
        Block record;
        size_t count;
        mbstate_t state = mbstate_t();
        char* toNext;
        const wchar_t* from = this->pbase();
        const wchar_t* const fromEnd = this->pptr();

        while((count = fromEnd - from) != 0)
        {
            record.reserve(
                    Protocol::getRecordSize(count*converter.max_length()));

            Protocol::Header& header
                = *reinterpret_cast<Protocol::Header*>(record.begin());

            result = converter.out(
                    state,
                    from,
                    fromEnd,
                    from,
                    record.begin()+sizeof(Protocol::Header),
                    &*record.end(),
                    toNext);

            if(result == std::codecvt_base::error
                    || result == std::codecvt_base::noconv)
            {
                ERR_LOG("FcgiStreambuf code conversion failed")
                pbump(-count);
                return false;
            }
            header.contentLength = std::distance(
                    record.begin()+sizeof(Protocol::Header),
                    toNext);
            record.size(Protocol::getRecordSize(header.contentLength));

            header.version = Protocol::version;
            header.type = m_type;
            header.fcgiId = m_id.m_id;
            header.paddingLength =
                record.size()-header.contentLength-sizeof(Protocol::Header);

            send(m_id.m_socket, std::move(record));
        }

        this->setp(m_buffer, m_buffer+s_buffSize);
        return true;
    }

    template <>
    bool Fastcgipp::FcgiStreambuf<char, std::char_traits<char>>::emptyBuffer()
    {
        Block record;
        size_t count;

        const char* from = this->pbase();
        const char* const fromEnd = this->pptr();

        while((count = fromEnd - from) != 0)
        {
            record.size(Protocol::getRecordSize(count));

            Protocol::Header& header
                = *reinterpret_cast<Protocol::Header*>(record.begin());
            header.contentLength = std::min(
                    count,
                    static_cast<size_t>(0xffffU));

            std::copy(
                    from,
                    from+header.contentLength,
                    record.begin()+sizeof(Protocol::Header));

            from += header.contentLength;

            header.version = Protocol::version;
            header.type = m_type;
            header.fcgiId = m_id.m_id;
            header.paddingLength =
                record.size()-header.contentLength-sizeof(Protocol::Header);

            send(m_id.m_socket, std::move(record));
        }

        this->setp(m_buffer, m_buffer+s_buffSize);
        return true;
    }
}

template <class charT, class traits>
void Fastcgipp::FcgiStreambuf<charT, traits>::dump(
        const char* data,
        size_t size)
{
    emptyBuffer();
    Block record;

    while(size != 0)
    {
        record.size(Protocol::getRecordSize(size));

        Protocol::Header& header
            = *reinterpret_cast<Protocol::Header*>(record.begin());
        header.contentLength = std::min(size, static_cast<size_t>(0xffffU));

        std::copy(
                data,
                data+header.contentLength,
                record.begin()+sizeof(Protocol::Header));

        size -= header.contentLength;
        data += header.contentLength;

        header.version = Protocol::version;
        header.type = m_type;
        header.fcgiId = m_id.m_id;
        header.paddingLength =
            record.size()-header.contentLength-sizeof(Protocol::Header);

        send(m_id.m_socket, std::move(record));
    }
}
template <class charT, class traits>
void Fastcgipp::FcgiStreambuf<charT, traits>::dump2(
        const char* data,
        size_t size)
{
    emptyBuffer();
    Block record;

    while(size != 0)
    {
        record.size(Protocol::getRecordSize(size));

        Protocol::Header& header
            = *reinterpret_cast<Protocol::Header*>(record.begin());
        header.contentLength = std::min(size, static_cast<size_t>(0xffffU));

        std::copy(
                data,
                data+header.contentLength,
                record.begin()+sizeof(Protocol::Header));

        size -= header.contentLength;
        data += header.contentLength;

        header.version = Protocol::version;
        header.type = m_type;
        header.fcgiId = m_id.m_id;
        header.paddingLength =
            record.size()-header.contentLength-sizeof(Protocol::Header);

        send2(m_id.m_socket, std::move(record));
    }
}

template <class charT, class traits>
void Fastcgipp::FcgiStreambuf<charT, traits>::dump(
        std::basic_istream<char>& stream)
{
    const size_t maxContentLength = 0xffffU;
    emptyBuffer();
    Block record;

    while(true)
    {
        record.reserve(Protocol::getRecordSize(maxContentLength));

        Protocol::Header& header
            = *reinterpret_cast<Protocol::Header*>(record.begin());

        stream.read(record.begin()+sizeof(Protocol::Header), maxContentLength);
        header.contentLength = stream.gcount();
        if(header.contentLength == 0)
            break;

        record.size(Protocol::getRecordSize(header.contentLength));

        header.version = Protocol::version;
        header.type = m_type;
        header.fcgiId = m_id.m_id;
        header.paddingLength =
            record.size()-header.contentLength-sizeof(Protocol::Header);

        send(m_id.m_socket, std::move(record));
    }
}

template class Fastcgipp::FcgiStreambuf<wchar_t, std::char_traits<wchar_t>>;
template class Fastcgipp::FcgiStreambuf<char, std::char_traits<char>>;
