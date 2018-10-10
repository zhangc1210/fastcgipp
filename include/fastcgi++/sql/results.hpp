/*!
 * @file       results.hpp
 * @brief      Declares %SQL Results types
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       October 8, 2018
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
#include <chrono>

#include "fastcgi++/address.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ %SQL facilities
    namespace SQL
    {
        //! Response type for %SQL query results statuses
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

        //! Returns a text description of the specified %SQL query result status
        const char* statusString(const Status status);

        class Connection;

        //! De-templated base class for %SQL query result sets
        class Results_base
        {
        public:
            //! Get status of %SQL query result
            /*!
             * This function should be called first thing after an %SQL query is
             * complete to ensure the result of the query is what you expect it
             * to be.
             */
            Status status() const;

            //! Get error message associated with %SQL query result
            const char* errorMessage() const;
            
            //! How many rows were returned with the %SQL query
            unsigned rows() const;

            //! How many rows were affected from the %SQL query
            unsigned affectedRows() const;

            //! Check for nullness of a specific row/column
            bool null(int row, int column) const;

            virtual ~Results_base();

        protected:
            friend class Connection;

            //! Pointer to underlying %SQL results object
            void* m_res;

            Results_base():
                m_res(nullptr)
            {}

            //! How many columns are associated with the underlying results
            int columns() const;

            //! Verify type and size consistency for the specified column
            /*!
             * This function has no default definition and must be specialized
             * for each and every type that is to be used.
             */
            template<typename T> bool verifyColumn(int column) const;

            //! Extract typed value from specific row and column
            /*!
             * This function has no default definition and must be specialized
             * for each and every type that is to be used.
             */
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
        template<>
        bool Results_base::verifyColumn<std::chrono::time_point<
            std::chrono::system_clock>>(int column) const;
        template<>
        std::chrono::time_point<std::chrono::system_clock>
        Results_base::field<std::chrono::time_point<std::chrono::system_clock>>(
                int row,
                int column) const;
        template<>
        bool Results_base::verifyColumn<Address>(int column) const;
        template<>
        Address Results_base::field<Address>(int row, int column) const;

        //! Holds %SQL query result sets
        /*!
         * This class provides a strongly typed method for getting result sets
         * from an %SQL query. Auxiliary data is available through accessor
         * functions while row data is retrieved as tuples with types defined by
         * the template parameters.
         *
         * Once a query is complete:
         *  1. Call status() to ensure no errors.
         *  2. Call verify() to ensure the %SQL data types and the tuple are
         *     consistent.
         *  3. If there is row data, call row() to retrieve a row.
         *
         * @tparam Types Pack of types to contain in the row tuple.
         * @date    October 7, 2018
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template<typename... Types>
        class Results: public Results_base
        {
        public:
            //! Type returned for row retrieval requests
            typedef std::tuple<Types...> Row;

        private:
            //! How many columns in the row?
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
            //! Verify consistency between the tuple and the %SQL data
            /*!
             * This function should be called when an %SQL query is complete
             * after ensuring status() is what you're expecting it to be. It
             * will first ensure that the tuple size matches the column count
             * and subsequently ensure every column type in the underlying %SQL
             * data set matches that of the row tuple. If this function does not
             * return 0 it is not safe to call row().
             *
             * @return 0 if everything is good. -1 if the column count doesn't
             *         match the tuple size. A positive non-zero integer
             *         indicates a type mismatch at that column.
             */
            int verify() const
            {
                if(columns() != size)
                    return -1;
                else
                    return verify_impl(
                            std::make_integer_sequence<int, size>{});
            }

            //! Retrieve row data in tuple form
            /*!
             * Retrieve a row in tuple form from the underlying %SQL result data.
             * Do not call this function is the result set validity has not been
             * verified first by calling verify().
             *
             * @param [in] index Zero indexed row number. If this value is
             *             greater than or equal to rows(), the result is
             *             undefined and will likely segfault.
             * @return Row data in tuple form
             */
            Row row(int index) const
            {
                return row_impl(
                        index,
                        std::make_integer_sequence<int, size>{});
            }
        };

        //! Specialization of Results for zero sized result sets
        template<>
        class Results<>: public Results_base
        {
        public:
            //! Verify that the result set is zero sized.
            int verify() const
            {
                if(columns() == 0 && rows() ==0)
                    return 0;
                else
                    return -1;
            }
        };
    }
}

#endif
