/*!
 * @file       chunkstreambuf.cpp
 * @brief      Defines the ChunkStreamBuf stuff
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

#include "fastcgi++/chunkstreambuf.hpp"
#include "fastcgi++/log.hpp"

#include <codecvt>
#include <locale>
#include <algorithm>

Fastcgipp::ChunkStreamBuf<char>::ChunkStreamBuf()
{
    setBufferPtr();
}

void Fastcgipp::ChunkStreamBuf<char>::clear()
{
    m_body.clear();
    setBufferPtr();
}

void Fastcgipp::ChunkStreamBuf<char>::setBufferPtr()
{
    m_body.emplace_back();
    this->setp(
            m_body.back().data.get(), 
            m_body.back().data.get()+Chunk::capacity);
}

bool Fastcgipp::ChunkStreamBuf<char>::emptyBuffer()
{
    m_body.back().size = this->pptr() - this->pbase();
    if(m_body.back().size == Chunk::capacity)
        setBufferPtr();
    return true;
}

Fastcgipp::ChunkStreamBuf<wchar_t>::ChunkStreamBuf()
{
    this->setp(m_buffer, m_buffer+Chunk::capacity);
}

void Fastcgipp::ChunkStreamBuf<wchar_t>::clear()
{
    m_body.clear();
    this->setp(m_buffer, m_buffer+Chunk::capacity);
}

bool Fastcgipp::ChunkStreamBuf<wchar_t>::emptyBuffer()
{
    size_t count = this->pptr() - this->pbase();
    if(count == 0)
        return true;

    const std::codecvt_utf8<wchar_t> converter;
    std::codecvt_base::result result;
    mbstate_t state = mbstate_t();
    char* toNext;
    const wchar_t* from=this->pbase();
    const wchar_t* const fromEnd = this->pptr();

    if(m_body.empty() || m_body.back().size == Chunk::capacity)
        m_body.emplace_back();

    while(true)
    {
        result = converter.out(
                state,
                from,
                fromEnd,
                from,
                m_body.back().data.get()+m_body.back().size,
                m_body.back().data.get()+Chunk::capacity,
                toNext);

        if(result == std::codecvt_base::error
                || result == std::codecvt_base::noconv)
        {
            ERROR_LOG("Email::Streambuf code conversion failed")
            pbump(-count);
            return false;
        }
        m_body.back().size = std::distance(
                m_body.back().data.get(),
                toNext);
        count = fromEnd - from;
        if(count == 0)
            break;
        else
            m_body.emplace_back();
    }

    this->setp(m_buffer, m_buffer+Chunk::capacity);
    return true;
}
