/*!
 * @file       email.cpp
 * @brief      Defines types for composing emails
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 4, 2020
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

#include "fastcgi++/email.hpp"
#include "fastcgi++/log.hpp"

#include <codecvt>
#include <locale>
#include <algorithm>

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
    Email_base(m_streamBuffer.m_body)
{}

template class Fastcgipp::Mail::Email<wchar_t>;
template class Fastcgipp::Mail::Email<char>;
