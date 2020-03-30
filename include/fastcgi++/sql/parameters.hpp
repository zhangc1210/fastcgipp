/*!
 * @file       parameters.hpp
 * @brief      Declares %SQL parameters types
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       March 30, 2020
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

#ifndef FASTCGIPP_SQL_PARAMETERS_HPP
#define FASTCGIPP_SQL_PARAMETERS_HPP

#include "fastcgi++/endian.hpp"
#include "fastcgi++/address.hpp"

#include <tuple>
#include <string>
#include <vector>
#include <chrono>
#include <memory>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ %SQL facilities
    namespace SQL
    {
        //! A single parameter in an %SQL query
        /*!
         * All these types are assignable from an object of the template
         * parameter type while providing some additional interfacing so they
         * fit in nicely into a Parameters tuple. Note that no generic
         * definition exists for this class so only specializations are valid.
         */
        template<typename T> class Parameter;

        template<>
        struct Parameter<int16_t>: public BigEndian<int16_t>
        {
            using BigEndian<int16_t>::BigEndian;
            using BigEndian<int16_t>::operator=;
            static const unsigned oid;
        };

        template<>
        class Parameter<std::vector<int16_t>>:
            public std::vector<BigEndian<int16_t>>
        {
        private:
            typedef int16_t INT_TYPE;
            unsigned m_size;
            std::unique_ptr<char[]> m_data;
        public:
            void resize(const unsigned size);

            Parameter& operator=(const std::vector<INT_TYPE>& x);

            Parameter(const std::vector<INT_TYPE>& x)
            {
                *this = x;
            }

            Parameter(const unsigned size)
            {
                resize(size);
            }

            BigEndian<INT_TYPE>& operator[](const unsigned i)
            {
                return *reinterpret_cast<BigEndian<INT_TYPE>*>(
                        m_data.get() + 6*sizeof(int32_t)
                        + i*(sizeof(int32_t) + sizeof(INT_TYPE)));
            }

            unsigned size() const
            {
                return m_size;
            }

            const char* data() const
            {
                return m_data.get();
            }

            static const unsigned oid;
        };

        template<>
        struct Parameter<int32_t>: public BigEndian<int32_t>
        {
            using BigEndian<int32_t>::BigEndian;
            using BigEndian<int32_t>::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<int64_t>: public BigEndian<int64_t>
        {
            using BigEndian<int64_t>::BigEndian;
            using BigEndian<int64_t>::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<float>: public BigEndian<float>
        {
            using BigEndian<float>::BigEndian;
            using BigEndian<float>::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<double>: public BigEndian<double>
        {
            using BigEndian<double>::BigEndian;
            using BigEndian<double>::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<std::string>: public std::string
        {
            Parameter(const std::string& x):
                std::string(x)
            {}
            using std::string::string;
            using std::string::operator=;
            static const unsigned oid;
        };

        template<>
        struct Parameter<std::vector<char>>: public std::vector<char>
        {
            Parameter(const std::vector<char>& x):
                std::vector<char>(x)
            {}
            using std::vector<char>::vector;
            using std::vector<char>::operator=;
            static const unsigned oid;
        };

        template<>
        class Parameter<std::wstring>: public std::string
        {
        private:
            static std::string convert(const std::wstring& x);

        public:
            Parameter& operator=(const std::wstring& x)
            {
                 assign(convert(x));
                 return *this;
            }

            Parameter(const std::wstring& x):
                std::string(convert(x))
            {}

            static const unsigned oid;
        };

        template<>
        class Parameter<std::chrono::time_point<std::chrono::system_clock>>:
            public BigEndian<int64_t>
        {
        private:
            using BigEndian<int64_t>::operator=;
            static int64_t convert(
                    const std::chrono::time_point<std::chrono::system_clock>& x)
            {
                return std::chrono::duration_cast<
                    std::chrono::duration<int64_t, std::micro>>(
                        x.time_since_epoch()-std::chrono::seconds(946684800)).count();
            }

        public:
            Parameter& operator=(
                    const std::chrono::time_point<std::chrono::system_clock>& x)
            {
                *this = convert(x);
                return *this;
            }

            Parameter(
                    const std::chrono::time_point<std::chrono::system_clock>& x):
                BigEndian<int64_t>(convert(x))
            {}

            static const unsigned oid;
        };

        template<>
        struct Parameter<Address>: public std::array<char, 20>
        {
            Parameter& operator=(const Address& x)
            {
                auto next = begin();
                *next++ = addressFamily;
                *next++ = 128;
                *next++ = 0;
                *next++ = 16;
                next = std::copy_n(
                        reinterpret_cast<const char*>(&x),
                        Address::size,
                        next);
                return *this;
            }
            Parameter(const Address& x)
            {
                *this = x;
            }
            static const unsigned oid;
            static const char addressFamily;
        };

        //! De-templated base class for Parameters
        class Parameters_base
        {
        protected:
            //! Array of oids for each parameter
            /*!
             * This gets initialized by calling build().
             */
            const std::vector<unsigned>* m_oids;

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
            //! Initialize the arrays needed by %SQL
            void build();

            //! Constant pointer to array of all parameter oids
            /*!
             * This is not valid until build() is called
             */
            const unsigned* oids() const
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

        //! A tuple of parameters to tie to a %SQL query
        /*!
         * This class allows you to pass separate parameters to a %SQL
         * query. From the interface perspective this should behave exactly like
         * an std::tuple<Types...>. The differences from std::tuple lie in it's
         * ability to format the tuple data in a way %SQL wants to see it.
         *
         * @tparam Types Pack of types to contain in the tuple.
         * @date    October 7, 2018
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template<typename... Types>
        class Parameters:
            public std::tuple<Parameter<Types>...>,
            public Parameters_base
        {
        private:
            static const std::vector<unsigned> s_oids;
            static const std::vector<int> s_formats;

            //! How many items in the tuple?
            constexpr int size() const
            {
                return sizeof...(Types);
            }

            //! Recursive template %SQL array building function
            template<size_t column, size_t... columns>
            inline void build_impl(std::index_sequence<column, columns...>)
            {
                m_raws.push_back(std::get<column>(*this).data());
                m_sizes.push_back(std::get<column>(*this).size());
                build_impl(std::index_sequence<columns...>{});
            }

            //! Terminating template %SQL array building function
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
        std::shared_ptr<Parameters<Types...>> make_Parameters(
                const Types&... args)
        {
            return std::shared_ptr<Parameters<Types...>>(
                    new Parameters<Types...>(args...));
        }

        template<typename... Types>
        std::shared_ptr<Parameters<Types...>> make_Parameters(
                const std::tuple<Types...>& tuple)
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
const std::vector<unsigned> Fastcgipp::SQL::Parameters<Types...>::s_oids
{
    Fastcgipp::SQL::Parameter<Types>::oid...
};

template<typename... Types>
const std::vector<int> Fastcgipp::SQL::Parameters<Types...>::s_formats(
        sizeof...(Types),
        1);

#endif
