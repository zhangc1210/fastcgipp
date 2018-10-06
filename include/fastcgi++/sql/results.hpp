/*!
 * @file       results.hpp
 * @brief      Declares SQL results types
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       October 6, 2018
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

#ifndef FASTCGIPP_SQL_RESULTS_HPP
#define FASTCGIPP_SQL_RESULTS_HPP

#include <tuple>
#include <string>
#include <vector>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ SQL facilities
    namespace SQL
    {
        enum class Status
        {
            noResult,
            emptyQuery,
            commandOk,
            rowsOk,
            copyOut,
            copyIn,
            badResponse,
            nonfatalError,
            fatalError,
            copyBoth,
            singleTuple
        };

        const char* statusString(const Status status);

        class Results_base
        {
        public:
            void* m_res;

            virtual ~Results_base();

            Status status() const;

            const char* errorMessage() const;
            
            unsigned rows() const;

            unsigned affectedRows() const;

            bool null(int row, int column) const;

        protected:
            Results_base():
                m_res(nullptr)
            {}

            int columns() const;

            template<typename T> bool verifyColumn(int column) const;
            template<typename T> T field(int row, int column) const;
        };

        template<>
        bool Results_base::verifyColumn<int16_t>(int column) const;
        template<>
        int16_t Results_base::field<int16_t>(int row, int column) const;
        template<>
        bool Results_base::verifyColumn<int32_t>(int column) const;
        template<>
        int32_t Results_base::field<int32_t>(int row, int column) const;
        template<>
        bool Results_base::verifyColumn<int64_t>(int column) const;
        template<>
        int64_t Results_base::field<int64_t>(int row, int column) const;
        template<>
        bool Results_base::verifyColumn<float>(int column) const;
        template<>
        float Results_base::field<float>(int row, int column) const;
        template<>
        bool Results_base::verifyColumn<double>(int column) const;
        template<>
        double Results_base::field<double>(int row, int column) const;
        template<>
        bool Results_base::verifyColumn<std::string>(int column) const;
        template<>
        std::string Results_base::field<std::string>(int row, int column) const;
        template<>
        bool Results_base::verifyColumn<std::wstring>(int column) const;
        template<>
        std::wstring Results_base::field<std::wstring>(
                int row,
                int column) const;
        template<>
        bool Results_base::verifyColumn<std::vector<char>>(int column) const;
        template<>
        std::vector<char> Results_base::field<std::vector<char>>(
                int row,
                int column) const;

        template<typename... Types>
        class Results: public Results_base
        {
        public:
            typedef std::tuple<Types...> Row;

        private:
            static const int size = sizeof...(Types);

            template<int column, int... columns>
            int verify_impl(
                    std::integer_sequence<int, column, columns...>) const
            {
                if(verifyColumn<
                        typename std::tuple_element<column, Row>::type>(column))
                    return verify_impl(
                            std::integer_sequence<int, columns...>{});
                else
                    return column+1;
            }

            template<int column>
            int verify_impl(std::integer_sequence<int, column>) const
            {
                if(verifyColumn<
                        typename std::tuple_element<column, Row>::type>(column))
                    return 0;
                else
                    return column+1;
            }

            template<int... columns>
            Row row_impl(
                    int index, std::integer_sequence<int, columns...>) const
            {
                return Row(field<
                        typename std::tuple_element<columns, Row>::type>(
                            index,
                            columns)...);
            }

        public:
            int verify() const
            {
                if(columns() != size)
                    return -1;
                else
                    return verify_impl(
                            std::make_integer_sequence<int, size>{});
            }

            Row row(int index) const
            {
                return row_impl(
                        index,
                        std::make_integer_sequence<int, size>{});
            }
        };

        template<>
        class Results<>: public Results_base
        {
        public:
            int verify() const
            {
                if(columns() != 0)
                    return -1;
                else
                    return 0;
            }
        };
    }
}

#endif
