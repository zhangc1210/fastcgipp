/*!
 * @file       http.hpp
 * @brief      Declares the Address class
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       October 9, 2018
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

#ifndef FASTCGIPP_ADDRESS_HPP
#define FASTCGIPP_ADDRESS_HPP

#include <array>
#include <algorithm>
#include <cstring>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Efficiently stores IPv6 addresses
    /*!
     * This class stores IPv6 addresses as a 128 bit array. It does this as
     * opposed to storing the string itself to facilitate efficient logging and
     * processing of the address. The class possesses full IO and comparison
     * capabilities as well as allowing bitwise AND operations for netmask
     * calculation. It detects when an IPv4 address is stored outputs it
     * accordingly.
     *
     * @date    October 9, 2018
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    struct Address
    {
        //! This is the data length of the IPv6 address
        static constexpr size_t size=16;

        //! Data representation of the IPv6 address
        std::array<unsigned char, size> m_data;

        //! Assign the IPv6 address from a data array
        /*!
         * @param[in] data Pointer to a 16 byte array
         */
        Address operator=(const unsigned char* data)
        {
            std::copy(data, data+m_data.size(), m_data.begin());
            return *this;
        }

        Address operator=(const Address& address)
        {
            std::copy(
                    address.m_data.begin(),
                    address.m_data.end(),
                    m_data.begin());
            return *this;
        }

        Address(const Address& address)
        {
            std::copy(
                    address.m_data.begin(),
                    address.m_data.end(),
                    m_data.begin());
        }

        //! Initializes an all zero address
        Address()
        {
            zero();
        }

        //! Construct the IPv6 address from a data array
        /*!
         * @param[in] data Pointer to a 16 byte array
         */
        explicit Address(const unsigned char* data)
        {
            std::copy(data, data+m_data.size(), m_data.begin());
        }

#if ! defined(FASTCGIPP_WINDOWS)
        //! Assign the IP address from a string of characters
        /*!
         * In order for this to work the string must represent either an IPv4
         * address in standard textual decimal form (127.0.0.1) or an IPv6 in
         * standard form.
         *
         * @param[in] start First character of the string
         * @param[in] end Last character of the string + 1
         * @tparam charT Character type.
         */
        template<class charT> void assign(
                const charT* start,
                const charT* end);
        Address(const char* string)
        {
            assign(string, string+std::strlen(string));
        }

#endif
        bool operator==(const Address& x) const
        {
            return std::equal(
                    m_data.cbegin(),
                    m_data.cend(),
                    x.m_data.cbegin());
        }

        bool operator<(const Address& x) const
        {
            return std::memcmp(m_data.data(), x.m_data.data(), size)<0;
        }

        //! Returns false if the ip address is zeroed. True otherwise
        operator bool() const;

        Address& operator&=(const Address& x);

        Address operator&(const Address& x) const
        {
            Address address(*this);
            address &= x;

            return address;
        }

        //! Set all bits to zero in IP address
        void zero()
        {
            m_data.fill(0);
        }
    };

    //! Address stream insertion operation
    /*!
     * This stream inserter obeys all stream manipulators regarding alignment,
     * field width and numerical base.
     */
    template<class charT, class Traits>
    std::basic_ostream<charT, Traits>& operator<<(
            std::basic_ostream<charT, Traits>& os,
            const Address& address);

    //! Address stream extractor operation
    /*!
     * In order for this to work the string must represent either an IPv4
     * address in standard textual decimal form (127.0.0.1) or an IPv6 in
     * standard form.
     */
    template<class charT, class Traits>
    std::basic_istream<charT, Traits>& operator>>(
            std::basic_istream<charT, Traits>& is,
            Address& address);
}

#endif
