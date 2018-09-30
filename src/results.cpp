/*!
 * @file       results.cpp
 * @brief      Defines SQL results types
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       September 29, 2018
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

#include "fastcgi++/sql/results.hpp"

#include <locale>
#include <codecvt>

Fastcgipp::SQL::Status Fastcgipp::SQL::Results_base::status() const
{
    if(m_res == nullptr)
        return Status::noResult;

    switch(PQresultStatus(m_res))
    {
        case PGRES_EMPTY_QUERY:
            return Status::emptyQuery;
        case PGRES_COMMAND_OK:
            return Status::commandOk;
        case PGRES_TUPLES_OK:
            return Status::rowsOk;
        case PGRES_COPY_OUT:
            return Status::copyOut;
        case PGRES_COPY_IN:
            return Status::copyIn;
        case PGRES_BAD_RESPONSE:
            return Status::badResponse;
        case PGRES_NONFATAL_ERROR:
            return Status::nonfatalError;
        case PGRES_COPY_BOTH:
            return Status::copyBoth;
        case PGRES_SINGLE_TUPLE:
            return Status::singleTuple;
        default:
            return Status::fatalError;
    };
}

template<> std::wstring Fastcgipp::SQL::Results_base::field<std::wstring>(
        int row,
        int column) const
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    const char* const start = PQgetvalue(m_res, row, column);
    const char* const end = start+PQgetlength(m_res, row, column);
    try
    {
        return converter.from_bytes(start, end);
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in code conversion from utf8 in SQL result")
    }
    return std::wstring();
}
