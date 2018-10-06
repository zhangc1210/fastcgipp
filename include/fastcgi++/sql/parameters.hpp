/*!
 * @file       parameters.hpp
 * @brief      Declares SQL parameters types
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

#ifndef FASTCGIPP_SQL_PARAMETERS_HPP
#define FASTCGIPP_SQL_PARAMETERS_HPP

#include "fastcgi++/protocol.hpp"
#include "fastcgi++/log.hpp"

#include <tuple>
#include <utility>
#include <string>
#include <vector>
#include <type_traits>

#include <postgres.h>
#include <libpq-fe.h>
#include <catalog/pg_type.h>
#undef ERROR
#undef WARNING
// I sure would like to know who thought it clever to define the macros ERROR
// and WARNING in these postgresql header files

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ SQL facilities
    namespace SQL
    {
        //! A single parameter in an SQL query
        /*!
         * All these types are assignable from an object of the template
         * parameter type while providing some additional interfacing so they
         * fit in nicely into a Parameters tuple.
         */
        template<typename T> class Parameter;

        //! Parameter specialization for 16 bit signed integers
        template<>
        struct Parameter<int16_t>: public Protocol::BigEndian<int16_t>
        {
            using Protocol::BigEndian<int16_t>::BigEndian;
            using Protocol::BigEndian<int16_t>::operator=;
            static const Oid oid = INT2OID;
        };

        //! Parameter specialization for 32 bit signed integers
        template<>
        struct Parameter<int32_t>: public Protocol::BigEndian<int32_t>
        {
            using Protocol::BigEndian<int32_t>::BigEndian;
            using Protocol::BigEndian<int32_t>::operator=;
            static const Oid oid = INT4OID;
        };

        //! Parameter specialization for 64 bit signed integers
        template<>
        struct Parameter<int64_t>: public Protocol::BigEndian<int64_t>
        {
            using Protocol::BigEndian<int64_t>::BigEndian;
            using Protocol::BigEndian<int64_t>::operator=;
            static const Oid oid = INT8OID;
        };


        //! Parameter specialization for 32 bit floating point values
        template<>
        struct Parameter<float>: public Protocol::BigEndian<float>
        {
            using Protocol::BigEndian<float>::BigEndian;
            using Protocol::BigEndian<float>::operator=;
            static const Oid oid = FLOAT4OID;
        };

        //! Parameter specialization for 64 bit floating point values
        template<>
        struct Parameter<double>: public Protocol::BigEndian<double>
        {
            using Protocol::BigEndian<double>::BigEndian;
            using Protocol::BigEndian<double>::operator=;
            static const Oid oid = FLOAT8OID;
        };

        //! Parameter specialization for character text strings
        template<>
        struct Parameter<std::string>: public std::string
        {
            Parameter(const std::string& x):
                std::string(x)
            {}
            using std::string::string;
            using std::string::operator=;
            static const Oid oid = TEXTOID;
        };

        //! Parameter specialization for data arrays
        template<>
        struct Parameter<std::vector<char>>: public std::vector<char>
        {
            Parameter(const std::vector<char>& x):
                std::vector<char>(x)
            {}
            using std::vector<char>::vector;
            using std::vector<char>::operator=;
            static const Oid oid = BYTEAOID;
        };

        //! Parameter specialization for wide character text strings
        /*!
         * This specialization should be used for both wide and narrow character
         * strings.
         */
        template<>
        struct Parameter<std::wstring>: public std::string
        {
            using std::string::operator=;
            using std::string::string;
            Parameter& operator=(const std::wstring& x);

            Parameter(const std::wstring& x)
            {
                *this = x;
            }

            //! Associated PostgreSQL Oid
            static const Oid oid = TEXTOID;
        };

        //! De-templated base class for Parameters
        class Parameters_base
        {
        protected:
            //! Array of Oids for each parameter
            /*!
             * This gets initialized by calling build().
             */
            const std::vector<Oid>* m_oids;

            //! Array of raw data pointers for each parameter
            /*!
             * This gets initialized by calling build().
             */
            std::vector<const char*> m_raws;

            //! Array of sizes for each parameter
            /*!
             * This gets initialized by calling build().
             */
            std::vector<int> m_sizes;

            //! Array of formats for each parameter
            /*!
             * This gets initialized by calling build(). It is really just an
             * array of 1s.
             */
            const std::vector<int>* m_formats;

            //! Template side virtual to populate the above arrays
            /*!
             * This just gets called by build().
             */
            virtual void build_impl() =0;

        public:
            //! Initialize the arrays needed by PostgreSQL
            void build();

            //! Constant pointer to array of all parameter Oids
            /*!
             * This is not valid until build() is called
             */
            const Oid* oids() const
            {
                return m_oids->data();
            }

            //! Constant pointer to pointer array of all raw parameter data
            /*!
             * This is not valid until build() is called
             */
            const char* const* raws() const
            {
                return m_raws.data();
            }

            //! Constant pointer to array of all parameter sizes
            const int* sizes() const
            {
                return m_sizes.data();
            }

            //! Constant pointer to array of all formats
            const int* formats() const
            {
                return m_formats->data();
            }

            //! How many parameters in this tuple?
            virtual int size() const =0;

            virtual ~Parameters_base() {}
        };

        //! A tuple of parameters to tie to a PostgreSQL query
        /*!
         * This class allows you to pass separate parameters to a PostgreSQL
         * query. From the interface perspective this should behave exactly like
         * an std::tuple<Types...>. The differences from std::tuple lie in it's
         * ability to format the tuple data in a way PostgreSQL wants to see it.
         *
         * @tparam Types Pack of types to contain in the tuple.
         * @date    September 29, 2018
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template<typename... Types>
        class Parameters:
            public std::tuple<Parameter<Types>...>,
            public Parameters_base
        {
        private:
            static const std::vector<Oid> s_oids;
            static const std::vector<int> s_formats;

            //! How many items in the tuple?
            constexpr int size() const
            {
                return sizeof...(Types);
            }

            //! Recursive template PostgreSQL array building function
            template<size_t column, size_t... columns>
            inline void build_impl(std::index_sequence<column, columns...>)
            {
                m_raws.push_back(std::get<column>(*this).data());
                m_sizes.push_back(std::get<column>(*this).size());
                build_impl(std::index_sequence<columns...>{});
            }

            //! Terminating template PostgreSQL array building function
            template<size_t column>
            inline void build_impl(std::index_sequence<column>)
            {
                m_raws.push_back(std::get<column>(*this).data());
                m_sizes.push_back(std::get<column>(*this).size());
            }

            void build_impl();

        public:
            using std::tuple<Parameter<Types>...>::tuple;
            using std::tuple<Parameter<Types>...>::operator=;
        };

        template<typename... Types>
        std::shared_ptr<Parameters<Types...>> make_Parameters(const Types&... args)
        {
            return std::shared_ptr<Parameters<Types...>>(
                    new Parameters<Types...>(args...));
        }

        template<typename... Types>
        std::shared_ptr<Parameters<Types...>> make_Parameters(const std::tuple<Types...>& tuple)
        {
            return std::shared_ptr<Parameters<Types...>>(
                    new Parameters<Types...>(tuple));
        }
    }
}

template<typename... Types>
void Fastcgipp::SQL::Parameters<Types...>::build_impl()
{
    m_oids = &s_oids;
    m_formats = &s_formats;
    build_impl(std::index_sequence_for<Types...>{});
}

template<typename... Types>
const std::vector<Oid> Fastcgipp::SQL::Parameters<Types...>::s_oids
{
    Fastcgipp::SQL::Parameter<Types>::oid...
};

template<typename... Types>
const std::vector<int> Fastcgipp::SQL::Parameters<Types...>::s_formats(
        sizeof...(Types),
        1);

#endif
