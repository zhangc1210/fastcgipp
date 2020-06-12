/*!
 * @file       sqlTraits.hpp
 * @brief      Defines SQL type traits
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       June 11, 2020
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

#ifndef FASTCGIPP_SQL_TRAITS_HPP
#define FASTCGIPP_SQL_TRAITS_HPP

#include <postgres.h>
#include <libpq-fe.h>
#include <catalog/pg_type.h>
#include <utils/inet.h>
#undef WARNING
#undef INFO
#undef ERROR

#include "fastcgi++/http.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ %SQL facilities
    namespace SQL
    {
        template<typename T> struct Traits {};
        template<> struct Traits<int16_t>
        {
            static constexpr unsigned oid = INT2OID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                const auto size = PQfsize(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid && size == sizeof(int16_t);
            }
        };
        template<> struct Traits<int32_t>
        {
            static constexpr unsigned oid = INT4OID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                const auto size = PQfsize(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid && size == sizeof(int32_t);
            }
        };
        template<> struct Traits<int64_t>
        {
            static constexpr unsigned oid = INT8OID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                const auto size = PQfsize(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid && size == sizeof(int64_t);
            }
        };
        template<> struct Traits<float>
        {
            static constexpr unsigned oid = FLOAT4OID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                const auto size = PQfsize(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid && size == sizeof(float);
            }
        };
        template<> struct Traits<double>
        {
            static constexpr unsigned oid = FLOAT8OID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                const auto size = PQfsize(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid && size == sizeof(double);
            }
        };
        template<> struct Traits<std::string>
        {
            static constexpr unsigned oid = TEXTOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
        template<> struct Traits<std::wstring>
        {
            static constexpr unsigned oid = TEXTOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
        template<> struct Traits<std::chrono::time_point<std::chrono::system_clock>>
        {
            static constexpr unsigned oid = TIMESTAMPOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                const auto size = PQfsize(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid && size == 8;
            }
        };
        template<> struct Traits<Fastcgipp::Address>
        {
            static constexpr unsigned oid = INETOID;
            static constexpr char addressFamily = PGSQL_AF_INET6;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
        template<> struct Traits<std::vector<char>>
        {
            static constexpr unsigned oid = BYTEAOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
        template<> struct Traits<std::vector<int16_t>>
        {
            static constexpr unsigned oid = INT2ARRAYOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
        template<> struct Traits<std::vector<int32_t>>
        {
            static constexpr unsigned oid = INT4ARRAYOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
        template<> struct Traits<std::vector<int64_t>>
        {
            static constexpr unsigned oid = INT8ARRAYOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
        template<> struct Traits<std::vector<float>>
        {
            static constexpr unsigned oid = FLOAT4ARRAYOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
        template<> struct Traits<std::vector<double>>
        {
            static constexpr unsigned oid = FLOAT8ARRAYOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
        template<> struct Traits<std::vector<std::string>>
        {
            static constexpr unsigned oid = TEXTARRAYOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
        template<> struct Traits<std::vector<std::wstring>>
        {
            static constexpr unsigned oid = TEXTARRAYOID;
            static bool verifyType(const void* result, int column)
            {
                const Oid type = PQftype(
                        reinterpret_cast<const PGresult*>(result),
                        column);
                return type == oid;
            }
        };
    }
}

#endif
