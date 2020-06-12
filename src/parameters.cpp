/*!
 * @file       parameters.cpp
 * @brief      Defines SQL parameters types
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

#include "sqlTraits.hpp"
#include "fastcgi++/sql/parameters.hpp"
#include "fastcgi++/log.hpp"

#include <locale>
#include <codecvt>

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
    return std::string();
}

const unsigned Fastcgipp::SQL::Parameter<int16_t>::oid = Traits<int16_t>::oid;
const unsigned Fastcgipp::SQL::Parameter<int32_t>::oid = Traits<int32_t>::oid;
const unsigned Fastcgipp::SQL::Parameter<int64_t>::oid = Traits<int64_t>::oid;
const unsigned Fastcgipp::SQL::Parameter<float>::oid = Traits<float>::oid;
const unsigned Fastcgipp::SQL::Parameter<double>::oid = Traits<double>::oid;
const unsigned Fastcgipp::SQL::Parameter<std::string>::oid
    = Traits<std::string>::oid;
const unsigned Fastcgipp::SQL::Parameter<std::wstring>::oid
    = Traits<std::wstring>::oid;
const unsigned Fastcgipp::SQL::Parameter<std::vector<char>>::oid
    = Traits<std::vector<char>>::oid;
const unsigned Fastcgipp::SQL::Parameter<
    std::chrono::time_point<std::chrono::system_clock>>::oid
    = Traits<std::chrono::time_point<std::chrono::system_clock>>::oid;
const unsigned Fastcgipp::SQL::Parameter<Fastcgipp::Address>::oid
    = Traits<Fastcgipp::Address>::oid;
const char Fastcgipp::SQL::Parameter<Fastcgipp::Address>::addressFamily
    = Traits<Fastcgipp::Address>::addressFamily;
template<typename Numeric>
const unsigned Fastcgipp::SQL::Parameter<std::vector<Numeric>>::oid
    = Traits<std::vector<Numeric>>::oid;
const unsigned Fastcgipp::SQL::Parameter<std::vector<std::string>>::oid
    = Traits<std::vector<std::string>>::oid;

template<typename Numeric>
void Fastcgipp::SQL::Parameter<std::vector<Numeric>>::resize(
        const unsigned size)
{
    m_size = sizeof(int32_t)*(5+size) + size*sizeof(Numeric);
    m_data.reset(new char[m_size]);

    BigEndian<int32_t>& ndim(*reinterpret_cast<BigEndian<int32_t>*>(
                m_data.get()+0*sizeof(int32_t)));
    BigEndian<int32_t>& hasNull(*reinterpret_cast<BigEndian<int32_t>*>(
                m_data.get()+1*sizeof(int32_t)));
    BigEndian<int32_t>& elementType(*reinterpret_cast<BigEndian<int32_t>*>(
                m_data.get()+2*sizeof(int32_t)));
    BigEndian<int32_t>& dim(*reinterpret_cast<BigEndian<int32_t>*>(
                m_data.get()+3*sizeof(int32_t)));
    BigEndian<int32_t>& lBound(*reinterpret_cast<BigEndian<int32_t>*>(
                m_data.get()+4*sizeof(int32_t)));

    ndim = 1;
    hasNull = 0;
    elementType = Traits<Numeric>::oid;
    dim = size;
    lBound = 1;
}

template<typename Numeric>
Fastcgipp::SQL::Parameter<std::vector<Numeric>>&
Fastcgipp::SQL::Parameter<std::vector<Numeric>>::operator=(
        const std::vector<Numeric>& x)
{
    resize(x.size());

    for(unsigned i=0; i < x.size(); ++i)
    {
        char* ptr = m_data.get() + 5*sizeof(int32_t)
            + i*(sizeof(int32_t) + sizeof(Numeric));

        BigEndian<int32_t>& length(
                *reinterpret_cast<BigEndian<int32_t>*>(ptr));
        BigEndian<Numeric>& value(
                *reinterpret_cast<BigEndian<Numeric>*>(
                    ptr+sizeof(int32_t)));

        length = sizeof(Numeric);
        value = x[i];
    }

    return *this;
}

template class Fastcgipp::SQL::Parameter<std::vector<int16_t>>;
template class Fastcgipp::SQL::Parameter<std::vector<int32_t>>;
template class Fastcgipp::SQL::Parameter<std::vector<int64_t>>;
template class Fastcgipp::SQL::Parameter<std::vector<float>>;
template class Fastcgipp::SQL::Parameter<std::vector<double>>;

void Fastcgipp::SQL::Parameter<std::vector<std::string>>::assign(
        const std::vector<std::string>& x)
{
    // Allocate the space
    {
        unsigned dataSize = 0;
        for(const auto& string: x)
            dataSize += string.size();
        m_size = sizeof(int32_t)*(5+x.size()) + dataSize;
        m_data.reset(new char[m_size]);
        BigEndian<int32_t>& ndim(*reinterpret_cast<BigEndian<int32_t>*>(
                    m_data.get()+0*sizeof(int32_t)));
        BigEndian<int32_t>& hasNull(*reinterpret_cast<BigEndian<int32_t>*>(
                    m_data.get()+1*sizeof(int32_t)));
        BigEndian<int32_t>& elementType(*reinterpret_cast<BigEndian<int32_t>*>(
                    m_data.get()+2*sizeof(int32_t)));
        BigEndian<int32_t>& dim(*reinterpret_cast<BigEndian<int32_t>*>(
                    m_data.get()+3*sizeof(int32_t)));
        BigEndian<int32_t>& lBound(*reinterpret_cast<BigEndian<int32_t>*>(
                    m_data.get()+4*sizeof(int32_t)));

        ndim = 1;
        hasNull = 0;
        elementType = Traits<std::string>::oid;
        dim = x.size();
        lBound = 1;
    }

    char* ptr = m_data.get() + 5*sizeof(int32_t);
    for(const auto& string: x)
    {
        BigEndian<int32_t>& length(
                *reinterpret_cast<BigEndian<int32_t>*>(ptr));
        length = string.size();
        ptr = std::copy(string.begin(), string.end(), ptr+sizeof(int32_t));
    }
}

std::string Fastcgipp::SQL::Parameter<std::vector<std::string>>::operator[](
        const unsigned x) const
{
    const char* ptr = m_data.get() + 5*sizeof(int32_t);
    unsigned i = 0;
    while(true)
    {
        const int32_t length(*reinterpret_cast<const BigEndian<int32_t>*>(ptr));
        ptr += sizeof(int32_t);
        if(i++ == x)
            return std::string(ptr, length);
        ptr += length;
    }
}

std::vector<std::string>
Fastcgipp::SQL::Parameter<std::vector<std::wstring>>::convert(
        const std::vector<std::wstring>& x)
{
    std::vector<std::string> result;
    result.reserve(x.size());

    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try
    {
        for(const auto& string: x)
            result.emplace_back(std::move(converter.to_bytes(string)));
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in array code conversion to utf8 in SQL parameter")
    }
    return result;
}

std::wstring Fastcgipp::SQL::Parameter<std::vector<std::wstring>>::convert(
        const std::string& x)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try
    {
        return converter.from_bytes(x);
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in array code conversion from utf8 in SQL parameter")
    }
    return std::wstring();
}
