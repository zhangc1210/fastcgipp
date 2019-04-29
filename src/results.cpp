/*!
 * @file       results.cpp
 * @brief      Defines SQL results types
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

#include "fastcgi++/sql/results.hpp"
#include "fastcgi++/endian.hpp"
#include "fastcgi++/log.hpp"

#include <locale>
#include <codecvt>
#include <cstdio>

#include <postgres.h>
#include <libpq-fe.h>
#include <catalog/pg_type.h>
#undef WARNING
// I sure would like to know who thought it clever to define the macro WARNING
// in these postgresql header files

template<>
bool Fastcgipp::SQL::Results_base::verifyColumn<int16_t>(int column) const
{
    return PQftype(reinterpret_cast<const PGresult*>(m_res), column) == INT2OID
        && PQfsize(reinterpret_cast<const PGresult*>(m_res), column) == 2;
}
template<> void Fastcgipp::SQL::Results_base::field<int16_t>(
        int row,
        int column,
        int16_t& value) const
{
    value = BigEndian<int16_t>::read(
            PQgetvalue(reinterpret_cast<const PGresult*>(m_res), row, column));
}

template<>
bool Fastcgipp::SQL::Results_base::verifyColumn<int32_t>(int column) const
{
    return PQftype(reinterpret_cast<const PGresult*>(m_res), column) == INT4OID
        && PQfsize(reinterpret_cast<const PGresult*>(m_res), column) == 4;
}
template<> void Fastcgipp::SQL::Results_base::field<int32_t>(
        int row,
        int column,
        int32_t& value) const
{
    value = BigEndian<int32_t>::read(
            PQgetvalue(reinterpret_cast<const PGresult*>(m_res), row, column));
}

template<>
bool Fastcgipp::SQL::Results_base::verifyColumn<int64_t>(int column) const
{
    return PQftype(reinterpret_cast<const PGresult*>(m_res), column) == INT8OID
        && PQfsize(reinterpret_cast<const PGresult*>(m_res), column) == 8;
}
template<> void Fastcgipp::SQL::Results_base::field<int64_t>(
        int row,
        int column,
        int64_t& value) const
{
    value = BigEndian<int64_t>::read(
            PQgetvalue(reinterpret_cast<const PGresult*>(m_res), row, column));
}

template<>
bool Fastcgipp::SQL::Results_base::verifyColumn<float>(int column) const
{
    return PQftype(reinterpret_cast<const PGresult*>(m_res), column)==FLOAT4OID
        && PQfsize(reinterpret_cast<const PGresult*>(m_res), column) == 4;
}
template<> void Fastcgipp::SQL::Results_base::field<float>(
        int row,
        int column,
        float& value) const
{
    value = BigEndian<float>::read(
            PQgetvalue(reinterpret_cast<const PGresult*>(m_res), row, column));
}

template<>
bool Fastcgipp::SQL::Results_base::verifyColumn<double>(int column) const
{
    return PQftype(reinterpret_cast<const PGresult*>(m_res), column)==FLOAT8OID
        && PQfsize(reinterpret_cast<const PGresult*>(m_res), column) == 8;
}
template<> void Fastcgipp::SQL::Results_base::field<double>(
        int row,
        int column,
        double& value) const
{
    value = BigEndian<double>::read(
            PQgetvalue(reinterpret_cast<const PGresult*>(m_res), row, column));
}

template<>
bool Fastcgipp::SQL::Results_base::verifyColumn<std::string>(int column) const
{
    const Oid type = PQftype(reinterpret_cast<const PGresult*>(m_res), column);
    return type == TEXTOID || type == VARCHAROID;
}
template<> void Fastcgipp::SQL::Results_base::field<std::string>(
        int row,
        int column,
        std::string& value) const
{
    value.assign(
            PQgetvalue(reinterpret_cast<const PGresult*>(m_res), row, column),
            PQgetlength(reinterpret_cast<const PGresult*>(m_res), row, column));
}

template<>
bool Fastcgipp::SQL::Results_base::verifyColumn<std::wstring>(int column) const
{
    const Oid type = PQftype(reinterpret_cast<const PGresult*>(m_res), column);
    return type == TEXTOID || type == VARCHAROID;
}
template<> void Fastcgipp::SQL::Results_base::field<std::wstring>(
        int row,
        int column,
        std::wstring& value) const
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    const char* const start = PQgetvalue(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);
    const char* const end = start+PQgetlength(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);
    try
    {
        value = converter.from_bytes(start, end);
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in code conversion from utf8 in SQL result")
    }
}

template<>
bool Fastcgipp::SQL::Results_base::verifyColumn<std::vector<char>>(
        int column) const
{
    const Oid type = PQftype(reinterpret_cast<const PGresult*>(m_res), column);
    return type == BYTEAOID;
}
template<> void Fastcgipp::SQL::Results_base::field<std::vector<char>>(
        int row,
        int column,
        std::vector<char>& value) const
{
    const unsigned size = PQgetlength(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);
    const char* const start = PQgetvalue(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);
    const char* const end = start+size;

    value.reserve(size);
    value.assign(start, end);
}

template<>
bool Fastcgipp::SQL::Results_base::verifyColumn<
    std::chrono::time_point<std::chrono::system_clock>>(int column) const
{
    const Oid type = PQftype(reinterpret_cast<const PGresult*>(m_res), column);
    return type == TIMESTAMPOID
        && PQfsize(reinterpret_cast<const PGresult*>(m_res), column) == 8;
}
template<> void Fastcgipp::SQL::Results_base::field<
  std::chrono::time_point<std::chrono::system_clock>>(
          int row,
          int column,
          std::chrono::time_point<std::chrono::system_clock>& value) const
{
    const int64_t count = BigEndian<int64_t>::read(
            PQgetvalue(reinterpret_cast<const PGresult*>(m_res), row, column));

    const std::chrono::duration<int64_t, std::micro> duration(count);

    value = std::chrono::time_point<std::chrono::system_clock>(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                duration)+std::chrono::seconds(946684800));
}

template<> bool Fastcgipp::SQL::Results_base::verifyColumn<Fastcgipp::Address>(
        int column) const
{
    return PQftype(reinterpret_cast<const PGresult*>(m_res), column) == INETOID;
}
template<> void Fastcgipp::SQL::Results_base::field<Fastcgipp::Address>(
        int row,
        int column,
        Address& value) const
{
    char* address_p = reinterpret_cast<char*>(&value);
    const char* const data = PQgetvalue(
            reinterpret_cast<const PGresult*>(m_res),
            row,
            column);

    switch(PQgetlength(reinterpret_cast<const PGresult*>(m_res), row, column))
    {
        case 8:
        {
            address_p = std::fill_n(address_p, 10, char(0));
            address_p = std::fill_n(address_p, 2, char(-1));
            std::copy_n(
                    data+4,
                    4,
                    address_p);
            break;
        }
        case 20:
        {
            std::copy_n(data+4, Address::size, address_p);
            break;
        }
    }
}

Fastcgipp::SQL::Status Fastcgipp::SQL::Results_base::status() const
{
    if(reinterpret_cast<const PGresult*>(m_res) == nullptr)
        return Status::noResult;

    switch(PQresultStatus(reinterpret_cast<const PGresult*>(m_res)))
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

unsigned Fastcgipp::SQL::Results_base::affectedRows() const
{
    return std::atoi(PQcmdTuples(reinterpret_cast<PGresult*>(m_res)));
}

Fastcgipp::SQL::Results_base::~Results_base()
{
    if(m_res != nullptr)
        PQclear(reinterpret_cast<PGresult*>(m_res));
}

const char* Fastcgipp::SQL::Results_base::errorMessage() const
{
    return PQresultErrorMessage(reinterpret_cast<const PGresult*>(m_res));
}

unsigned Fastcgipp::SQL::Results_base::rows() const
{
    return PQntuples(reinterpret_cast<const PGresult*>(m_res));
}

bool Fastcgipp::SQL::Results_base::null(int row, int column) const
{
    return static_cast<bool>(PQgetisnull(
                reinterpret_cast<const PGresult*>(m_res),
                row,
                column));
}

int Fastcgipp::SQL::Results_base::columns() const
{
    return PQnfields(reinterpret_cast<const PGresult*>(m_res));
}

const char* Fastcgipp::SQL::statusString(const Status status)
{
    switch(status)
    {
        case Status::noResult:
            return "No Result";
        case Status::emptyQuery:
            return "Empty Query";
        case Status::commandOk:
            return "Command OK";
        case Status::rowsOk:
            return "Rows OK";
        case Status::copyOut:
            return "Copy Out";
        case Status::copyIn:
            return "Copy In";
        case Status::badResponse:
            return "Bad Response";
        case Status::nonfatalError:
            return "Non-fatal Error";
        case Status::copyBoth:
            return "Copy Both";
        case Status::singleTuple:
            return "Single Tuple";
        case Status::fatalError:
        default:
            return "Fatal Error";
    }
}
