/*!
 * @file       endian.hpp
 * @brief      Defines the BigEndian class
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

#ifndef FASTCGIPP_ENDIAN_HPP
#define FASTCGIPP_ENDIAN_HPP

#include <cstdint>
#include <type_traits>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! De-templated
    class BigEndian_base
    {
    protected:
        //! Get unsigned integral type from size
        template<unsigned Size> struct Unsigned;

        static constexpr void from(
                const unsigned char* arr,
                std::uint16_t& v) noexcept
        {
            v = static_cast<std::uint16_t>(
                std::uint16_t{arr[0]} << 8 |
                std::uint16_t{arr[1]} << 0);
        }
        static constexpr void from(
                const unsigned char* arr,
                std::uint32_t& v) noexcept
        {
            v = static_cast<std::uint32_t>(
                std::uint32_t{arr[0]} << 24 |
                std::uint32_t{arr[1]} << 16 |
                std::uint32_t{arr[2]} << 8 |
                std::uint32_t{arr[3]} << 0);
        }
        static constexpr void from(
                const unsigned char* arr,
                std::uint64_t& v) noexcept
        {
            v = static_cast<std::uint64_t>(
                std::uint64_t{arr[0]} << 56 |
                std::uint64_t{arr[1]} << 48 |
                std::uint64_t{arr[2]} << 40 |
                std::uint64_t{arr[3]} << 32 |
                std::uint64_t{arr[4]} << 24 |
                std::uint64_t{arr[5]} << 16 |
                std::uint64_t{arr[6]} << 8 |
                std::uint64_t{arr[7]} << 0);
        }

        static constexpr void to(unsigned char* arr, const std::uint16_t v)
        {
            arr[0] = static_cast<unsigned char>((v & 0xff00) >> 8);
            arr[1] = static_cast<unsigned char>((v & 0x00ff) >> 0);
        }
        static constexpr void to(unsigned char* arr, const std::uint32_t v)
        {
            arr[0] = static_cast<unsigned char>((v & 0xff000000) >> 24);
            arr[1] = static_cast<unsigned char>((v & 0x00ff0000) >> 16);
            arr[2] = static_cast<unsigned char>((v & 0x0000ff00) >> 8);
            arr[3] = static_cast<unsigned char>((v & 0x000000ff) >> 0);
        }
        static constexpr void to(unsigned char* arr, const std::uint64_t v)
        {
            arr[0] = static_cast<unsigned char>((v & 0xff00000000000000) >> 56);
            arr[1] = static_cast<unsigned char>((v & 0x00ff000000000000) >> 48);
            arr[2] = static_cast<unsigned char>((v & 0x0000ff0000000000) >> 40);
            arr[3] = static_cast<unsigned char>((v & 0x000000ff00000000) >> 32);
            arr[4] = static_cast<unsigned char>((v & 0x00000000ff000000) >> 24);
            arr[5] = static_cast<unsigned char>((v & 0x0000000000ff0000) >> 16);
            arr[6] = static_cast<unsigned char>((v & 0x000000000000ff00) >> 8);
            arr[7] = static_cast<unsigned char>((v & 0x00000000000000ff) >> 0);
        }
    };

    template<> struct BigEndian_base::Unsigned<2>
    {
        typedef std::uint16_t Type;
    };
    template<> struct BigEndian_base::Unsigned<4>
    {
        typedef std::uint32_t Type;
    };
    template<> struct BigEndian_base::Unsigned<8>
    {
        typedef std::uint64_t Type;
    };

    //! Allows raw storage of types in big endian format
    /*!
     * This templated class allows any trivially copyable types of size 2, 4, or
     * 8 to be stored in big endian format but maintain type compatibility.
     *
     * @tparam T Type to emulate. Must be either of size 2, 4 or 8 and be
     *           trivially copyable.
     * @date    October 9, 2018
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    template<typename T> class BigEndian: private BigEndian_base
    {
    private:
        static constexpr unsigned s_size = sizeof(T);
        static_assert(s_size==2||s_size==4||s_size==8, "Fastcgipp::BigEndian"
                " can only work with types of size 2, 4 or 8.");
        static_assert(std::is_trivially_copyable<T>::value, "Fastcgipp::"
                "BigEndian can only work with trivial copyable types.");

        //! Underlying unsigned integral type.
        typedef typename Unsigned<s_size>::Type BaseType;

        //! The raw data of the big endian integer
        unsigned char m_data[s_size];

        //! Set the internal data to the passed parameter.
        constexpr void set(T x) noexcept
        {
            union {
                BaseType base;
                T actual;
            } u = {.actual = x};
            to(m_data, u.base);
        }

    public:
        constexpr BigEndian& operator=(T x) noexcept
        {
            set(x);
            return *this;
        }

        constexpr BigEndian(T x) noexcept
        {
            set(x);
        }

        constexpr BigEndian() noexcept
        {}

        constexpr operator T() const noexcept
        {
            return read(m_data);
        }

        //! Static function for reading the value out of a data array.
        /*!
         * This will read the value out of an unsigned char array in big
         * endian format and cast it into type T.
         *
         * @param [in] source Pointer to start of data. This data should of
         *                    course be at minimum size.
         */
        static constexpr T read(const unsigned char* source) noexcept
        {
            union {
                BaseType base;
                T actual;
            } u = {.base = 0 };
            from(source, u.base);
            return u.actual;
        }

        //! Simply casts char to unsigned char.
        static constexpr T read(const char* source) noexcept
        {
            return read(reinterpret_cast<const unsigned char*>(source));
        }

        //! Pointer to start of big endian integer representation
        constexpr const char* data() const
        {
            return reinterpret_cast<const char*>(&m_data);
        }

        //! Size in bytes of value
        constexpr unsigned size() const
        {
            return s_size;
        }
    };
}

#endif
