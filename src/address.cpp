/*!
 * @file       http.cpp
 * @brief      Defines elements of the HTTP protocol
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

#include <utility>
#include <sstream>
#include <iomanip>

#include "fastcgi++/address.hpp"
#include "fastcgi++/log.hpp"
#include "fastcgi++/http.hpp"

Fastcgipp::Address& Fastcgipp::Address::operator&=(
        const Address& x)
{
    *reinterpret_cast<uint64_t*>(m_data.data())
        &= *reinterpret_cast<const uint64_t*>(x.m_data.data());
    *reinterpret_cast<uint64_t*>(m_data.data()+8)
        &= *reinterpret_cast<const uint64_t*>(x.m_data.data()+8);

    return *this;
}

template void Fastcgipp::Address::assign<char>(
        const char* start,
        const char* end);
template void Fastcgipp::Address::assign<wchar_t>(
        const wchar_t* start,
        const wchar_t* end);
template<class charT> void Fastcgipp::Address::assign(
        const charT* start,
        const charT* end)
{
    const charT* read=start-1;
    auto write=m_data.begin();
    auto pad=m_data.end();
    unsigned char offset;
    uint16_t chunk=0;
    bool error=false;

    while(1)
    {
        ++read;
        if(read >= end || *read == ':')
        {
            if(read == start || *(read-1) == ':')
            {
                if(pad!=m_data.end() && pad!=write)
                {
                    error=true;
                    break;
                }
                else
                    pad = write;
            }
            else
            {
                *write = (chunk&0xff00)>>8;
                *(write+1) = chunk&0x00ff;
                chunk = 0;
                write += 2;
                if(write>=m_data.end() || read>=end)
                {
                    if(read>=end && write<m_data.end() && pad==m_data.end())
                        error=true;
                    break;
                }
            }
            continue;
        }
        else if('0' <= *read && *read <= '9')
            offset = '0';
        else if('A' <= *read && *read <= 'F')
            offset = 'A'-10;
        else if('a' <= *read && *read <= 'f')
            offset = 'a'-10;
        else if(*read == '.')
        {
            if(write == m_data.begin())
            {
                // We must be getting a pure ipv4 formatted address. Not an
                // ::ffff:xxx.xxx.xxx.xxx style ipv4 address.
                *reinterpret_cast<uint16_t*>(write) = 0xffff;
                pad = m_data.begin();
                write += 2;
            }
            else if(write - m_data.begin() > 12)
            {
                // We don't have enought space for an ipv4 address
                error=true;
                break;
            }

            // First convert the value stored in chunk to the first part of the
            // ipv4 address
            *write = 0;
            for(int i=0; i<3; ++i)
            {
                *write = *write * 10 + ((chunk&0x0f00)>>8);
                chunk <<= 4;
            }
            ++write;

            // Now we'll get the remaining pieces
            for(int i=0; i<3 && read<end; ++i)
            {
                const charT* point=std::find(read, end, '.');
                if(point<end-1)
                    read=point;
                else
                {
                    error=true;
                    break;
                }
                *write++ = Http::atoi(++read, end);
            }
            break;
        }
        else
        {
            error=true;
            break;
        }
        chunk <<= 4;
        chunk |= *read-offset;
    }

    if(error)
    {
        m_data.fill(0);
        WARNING_LOG("Error converting IPv6 address " \
                << std::wstring(start, end))
    }
    else if(pad != m_data.end())
    {
        if(pad==write)
            std::fill(write, m_data.end(), 0);
        else
        {
            auto padEnd=pad+(m_data.end()-write);
            std::copy(pad, write, padEnd);
            std::fill(pad, padEnd, 0);
        }
    }
}

template std::basic_ostream<char, std::char_traits<char>>&
Fastcgipp::operator<< <char, std::char_traits<char>>(
        std::basic_ostream<char, std::char_traits<char>>& os,
        const Address& address);
template std::basic_ostream<wchar_t, std::char_traits<wchar_t>>&
Fastcgipp::operator<< <wchar_t, std::char_traits<wchar_t>>(
        std::basic_ostream<wchar_t, std::char_traits<wchar_t>>& os,
        const Address& address);
template<class charT, class Traits> std::basic_ostream<charT, Traits>&
Fastcgipp::operator<<(
        std::basic_ostream<charT, Traits>& os,
        const Address& address)
{
    using namespace std;
    if(!os.good()) return os;

    try
    {
        typename basic_ostream<charT, Traits>::sentry opfx(os);
        if(opfx)
        {
            streamsize fieldWidth=os.width(0);
            charT buffer[40];
            charT* bufPtr=buffer;
            locale loc(os.getloc(), new num_put<charT, charT*>);

            const uint16_t* subStart=0;
            const uint16_t* subEnd=0;
            {
                const uint16_t* subStartCandidate;
                const uint16_t* subEndCandidate;
                bool inZero = false;

                for(
                        const uint16_t* it = reinterpret_cast<const uint16_t*>(
                            address.m_data.data());
                        it < reinterpret_cast<const uint16_t*>(
                            address.m_data.data()+Address::size);
                        ++it)
                {
                    if(*it == 0)
                    {
                        if(!inZero)
                        {
                            subStartCandidate = it;
                            subEndCandidate = it;
                            inZero=true;
                        }
                        ++subEndCandidate;
                    }
                    else if(inZero)
                    {
                        if(subEndCandidate-subStartCandidate > subEnd-subStart)
                        {
                            subStart=subStartCandidate;
                            subEnd=subEndCandidate-1;
                        }
                        inZero=false;
                    }
                }
                if(inZero)
                {
                    if(subEndCandidate-subStartCandidate > subEnd-subStart)
                    {
                        subStart=subStartCandidate;
                        subEnd=subEndCandidate-1;
                    }
                    inZero=false;
                }
            }

            ios_base::fmtflags oldFlags = os.flags();
            os.setf(ios::hex, ios::basefield);

            if(
                    subStart==reinterpret_cast<const uint16_t*>(
                        address.m_data.data())
                    && subEnd==reinterpret_cast<const uint16_t*>(
                        address.m_data.data())+4
                    && *(reinterpret_cast<const uint16_t*>(
                            address.m_data.data())+5) == 0xffff)
            {
                // It is an ipv4 address
                *bufPtr++=os.widen(':');
                *bufPtr++=os.widen(':');
                bufPtr=use_facet<num_put<charT, charT*> >(loc).put(
                        bufPtr,
                        os,
                        os.fill(),
                        static_cast<unsigned long int>(0xffff));
                *bufPtr++=os.widen(':');
                os.setf(ios::dec, ios::basefield);

                for(
                        const unsigned char* it = address.m_data.data()+12;
                        it < address.m_data.data()+Address::size;
                        ++it)
                {
                    bufPtr=use_facet<num_put<charT, charT*> >(loc).put(
                            bufPtr,
                            os,
                            os.fill(),
                            static_cast<unsigned long int>(*it));
                    *bufPtr++=os.widen('.');
                }
                --bufPtr;
            }
            else
            {
                // It is an ipv6 address
                for(const uint16_t* it= reinterpret_cast<const uint16_t*>(
                            address.m_data.data());
                        it < reinterpret_cast<const uint16_t*>(
                            address.m_data.data()+Address::size);
                        ++it)
                {
                    if(subStart <= it && it <= subEnd)
                    {
                        if(
                                it == subStart
                                && it == reinterpret_cast<const uint16_t*>(
                                    address.m_data.data()))
                            *bufPtr++=os.widen(':');
                        if(it == subEnd)
                            *bufPtr++=os.widen(':');
                    }
                    else
                    {
                        bufPtr=use_facet<num_put<charT, charT*> >(loc).put(
                                bufPtr,
                                os,
                                os.fill(),
                                static_cast<unsigned long int>(
                                    *reinterpret_cast<
                                    const BigEndian<uint16_t>*>(it)));

                        if(it < reinterpret_cast<const uint16_t*>(
                                    address.m_data.data()+Address::size)-1)
                            *bufPtr++=os.widen(':');
                    }
                }
            }

            os.flags(oldFlags);

            charT* ptr=buffer;
            ostreambuf_iterator<charT,Traits> sink(os);
            if(os.flags() & ios_base::left)
                for(int i=max(fieldWidth, bufPtr-buffer); i>0; i--)
                {
                    if(ptr!=bufPtr) *sink++=*ptr++;
                    else *sink++=os.fill();
                }
            else
                for(int i=fieldWidth-(bufPtr-buffer); ptr!=bufPtr;)
                {
                    if(i>0) { *sink++=os.fill(); --i; }
                    else *sink++=*ptr++;
                }

            if(sink.failed()) os.setstate(ios_base::failbit);
        }
    }
    catch(bad_alloc&)
    {
        ios_base::iostate exception_mask = os.exceptions();
        os.exceptions(ios_base::goodbit);
        os.setstate(ios_base::badbit);
        os.exceptions(exception_mask);
        if(exception_mask & ios_base::badbit) throw;
    }
    catch(...)
    {
        ios_base::iostate exception_mask = os.exceptions();
        os.exceptions(ios_base::goodbit);
        os.setstate(ios_base::failbit);
        os.exceptions(exception_mask);
        if(exception_mask & ios_base::failbit) throw;
    }
    return os;
}

template std::basic_istream<char, std::char_traits<char>>&
Fastcgipp::operator>> <char, std::char_traits<char>>(
        std::basic_istream<char, std::char_traits<char>>& is,
        Address& address);
template std::basic_istream<wchar_t, std::char_traits<wchar_t>>&
Fastcgipp::operator>> <wchar_t, std::char_traits<wchar_t>>(
        std::basic_istream<wchar_t, std::char_traits<wchar_t>>& is,
        Address& address);
template<class charT, class Traits> std::basic_istream<charT, Traits>&
Fastcgipp::operator>>(
        std::basic_istream<charT, Traits>& is,
        Address& address)
{
    using namespace std;
    if(!is.good()) return is;

    ios_base::iostate err = ios::goodbit;
    try
    {
        typename basic_istream<charT, Traits>::sentry ipfx(is);
        if(ipfx)
        {
            istreambuf_iterator<charT, Traits> read(is);
            unsigned char buffer[Address::size];
            unsigned char* write=buffer;
            unsigned char* pad=0;
            unsigned char offset;
            unsigned char count=0;
            uint16_t chunk=0;
            charT lastChar=0;

            for(;;++read)
            {
                if(++count>40)
                {
                    err = ios::failbit;
                    break;
                }
                else if('0' <= *read && *read <= '9')
                    offset = '0';
                else if('A' <= *read && *read <= 'F')
                    offset = 'A'-10;
                else if('a' <= *read && *read <= 'f')
                    offset = 'a'-10;
                else if(*read == '.')
                {
                    if(write == buffer)
                    {
                        // We must be getting a pure ipv4 formatted address. Not an ::ffff:xxx.xxx.xxx.xxx style ipv4 address.
                        *reinterpret_cast<uint16_t*>(write) = 0xffff;
                        pad = buffer;
                        write+=2;
                    }
                    else if(write - buffer > 12)
                    {
                        // We don't have enought space for an ipv4 address
                        err = ios::failbit;
                        break;
                    }

                    // First convert the value stored in chunk to the first part of the ipv4 address
                    *write = 0;
                    for(int i=0; i<3; ++i)
                    {
                        *write = *write * 10 + ((chunk&0x0f00)>>8);
                        chunk <<= 4;
                    }
                    ++write;

                    // Now we'll get the remaining pieces
                    for(int i=0; i<3; ++i)
                    {
                        if(*read != is.widen('.'))
                        {
                            err = ios::failbit;
                            break;
                        }
                        unsigned int value;
                        use_facet<num_get<charT, istreambuf_iterator<charT, Traits> > >(is.getloc()).get(++read, istreambuf_iterator<charT, Traits>(), is, err, value);
                        *write++ = value;
                    }
                    break;
                }
                else
                {
                    if(*read == ':' && (!lastChar || lastChar == ':'))
                    {
                        if(pad && pad != write)
                        {
                            err = ios::failbit;
                            break;
                        }
                        else
                            pad = write;
                    }
                    else
                    {
                        *write = (chunk&0xff00)>>8;
                        *(write+1) = chunk&0x00ff;
                        chunk = 0;
                        write += 2;
                        if(write>=buffer+Address::size)
                            break;
                        if(*read!=':')
                        {
                            if(!pad)
                                err = ios::failbit;
                            break;
                        }
                    }
                    lastChar=':';
                    continue;
                }
                chunk <<= 4;
                chunk |= *read-offset;
                lastChar=*read;

            }

            if(err == ios::goodbit)
            {
                if(pad)
                {
                    if(pad==write)
                        std::memset(write, 0, Address::size-(write-buffer));
                    else
                    {
                        const size_t padSize=buffer+Address::size-write;
                        std::memmove(pad+padSize, pad, write-pad);
                        std::memset(pad, 0, padSize);
                    }
                }
                address=buffer;
            }
            else
                is.setstate(err);
        }
    }
    catch(bad_alloc&)
    {
        ios_base::iostate exception_mask = is.exceptions();
        is.exceptions(ios_base::goodbit);
        is.setstate(ios_base::badbit);
        is.exceptions(exception_mask);
        if(exception_mask & ios_base::badbit) throw;
    }
    catch(...)
    {
        ios_base::iostate exception_mask = is.exceptions();
        is.exceptions(ios_base::goodbit);
        is.setstate(ios_base::failbit);
        is.exceptions(exception_mask);
        if(exception_mask & ios_base::failbit) throw;
    }

    return is;
}

Fastcgipp::Address::operator bool() const
{
    static const std::array<const unsigned char, 16> nullString =
        {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};
    if(std::equal(m_data.begin(), m_data.end(), nullString.begin()))
        return false;
    return true;
}
