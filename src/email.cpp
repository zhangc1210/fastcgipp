/*!
 * @file       email.cpp
 * @brief      Defines types for composing emails
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       December 22, 2018
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

#include "fastcgi++/email.hpp"
#include "fastcgi++/log.hpp"

#include <codecvt>
#include <locale>
#include <algorithm>

Fastcgipp::Mail::Email_base::StreamBuffer<char>::StreamBuffer(
        std::list<Chunk>& body):
    m_body(body)
{
    setBufferPtr();
}

void Fastcgipp::Mail::Email_base::StreamBuffer<char>::setBufferPtr()
{
    m_body.emplace_back();
    this->setp(
            m_body.back().data.get(), 
            m_body.back().data.get()+Chunk::capacity);
}

bool Fastcgipp::Mail::Email_base::StreamBuffer<char>::emptyBuffer()
{
    m_body.back().size = this->pptr() - this->pbase();
    if(m_body.back().size == Chunk::capacity)
        setBufferPtr();
    return true;
}

Fastcgipp::Mail::Email_base::StreamBuffer<wchar_t>::StreamBuffer(
        std::list<Chunk>& body):
    m_body(body)
{
    this->setp(m_buffer, m_buffer+Chunk::capacity);
}

bool Fastcgipp::Mail::Email_base::StreamBuffer<wchar_t>::emptyBuffer()
{
    size_t count = this->pptr() - this->pbase();
    if(count == 0)
        return true;

    const std::codecvt_utf8<wchar_t> converter;
    std::codecvt_base::result result;
    mbstate_t state = mbstate_t();
    char* toNext;
    const wchar_t* fromNext;

    if(m_body.empty() || m_body.back().size == Chunk::capacity)
        m_body.emplace_back();

    while(true)
    {
        result = converter.out(
                state,
                this->pbase(),
                this->pptr(),
                fromNext,
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
        pbump(-(fromNext - this->pbase()));
        m_body.back().size = std::distance(
                m_body.back().data.get(),
                toNext);
        count = this->pptr() - this->pbase();

        if(count == 0)
            break;
        else
            m_body.emplace_back();
    }

    return true;
}

template<class charT>
void Fastcgipp::Mail::Email<charT>::close()
{
    m_streamBuffer.emptyBuffer();
    this->setstate(std::ios_base::eofbit);
    this->setstate(std::ios_base::failbit);
    this->setstate(std::ios_base::badbit);
}

template<class charT>
Fastcgipp::Mail::Email<charT>::Email():
    std::basic_ostream<charT>(&m_streamBuffer),
    m_streamBuffer(m_data.body)
{}

template <>
void Fastcgipp::Mail::Email<wchar_t>::to(const std::wstring& address)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try
    {
        m_data.to = converter.to_bytes(address);
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in code conversion to utf8 in email to address")
    }
}

template <>
void Fastcgipp::Mail::Email<wchar_t>::from(const std::wstring& address)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try
    {
        m_data.from = converter.to_bytes(address);
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in code conversion to utf8 in email from address")
    }
}

template class Fastcgipp::Mail::Email<wchar_t>;
template class Fastcgipp::Mail::Email<char>;
