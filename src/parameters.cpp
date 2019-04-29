/*!
 * @file       parameters.cpp
 * @brief      Defines SQL parameters types
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       April 29, 2019
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

#include "fastcgi++/sql/parameters.hpp"
#include "fastcgi++/log.hpp"

#include <locale>
#include <codecvt>

#include <postgres.h>
#include <libpq-fe.h>
#include <catalog/pg_type.h>
#include <utils/inet.h>
#undef WARNING
// I sure would like to know who thought it clever to define the macro WARNING
// in these postgresql header files

void Fastcgipp::SQL::Parameters_base::build()
{
    const int columns = size();
    m_raws.clear();
    m_raws.reserve(columns);
    m_sizes.clear();
    m_sizes.reserve(columns);
    build_impl();
}

std::string
Fastcgipp::SQL::Parameter<std::wstring>::convert(const std::wstring& x)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try
    {
        return converter.to_bytes(x);
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in code conversion to utf8 in SQL parameter")
    }
    return std::string("");
}

const unsigned Fastcgipp::SQL::Parameter<int16_t>::oid = INT2OID;
const unsigned Fastcgipp::SQL::Parameter<int32_t>::oid = INT4OID;
const unsigned Fastcgipp::SQL::Parameter<int64_t>::oid = INT8OID;
const unsigned Fastcgipp::SQL::Parameter<float>::oid = FLOAT4OID;
const unsigned Fastcgipp::SQL::Parameter<double>::oid = FLOAT8OID;
const unsigned Fastcgipp::SQL::Parameter<std::string>::oid = TEXTOID;
const unsigned Fastcgipp::SQL::Parameter<std::wstring>::oid = TEXTOID;
const unsigned Fastcgipp::SQL::Parameter<std::vector<char>>::oid = BYTEAOID;
const unsigned Fastcgipp::SQL::Parameter<
    std::chrono::time_point<std::chrono::system_clock>>::oid = TIMESTAMPOID;
const unsigned Fastcgipp::SQL::Parameter<Fastcgipp::Address>::oid = INETOID;
const char Fastcgipp::SQL::Parameter<Fastcgipp::Address>::addressFamily
    = PGSQL_AF_INET6;
