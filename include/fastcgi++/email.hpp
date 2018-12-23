/*!
 * @file       email.hpp
 * @brief      Declares types for composing emails
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       December 22, 2018
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

#ifndef FASTCGIPP_EMAIL_HPP
#define FASTCGIPP_EMAIL_HPP

#include <memory>
#include <list>
#include <ostream>

#include "fastcgi++/webstreambuf.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains the stuff for composing and sending emails
    namespace Mail
    {

        //! De-templated base class for Email
        /*!
         * @date    November 27, 2018
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        class Email_base
        {
        public:
            //! A chunk of email body data
            struct Chunk
            {
                //! Total capacity of a chunk
                static constexpr unsigned capacity = 4096;

                //! Pointer to chunk data
                std::unique_ptr<char[]> data;

                //! Size of chunk data
                unsigned size;

                Chunk():
                    data(new char[capacity]),
                    size(0)
                {}
            };

            //! %Email message data
            struct Data
            {
                //! Linked list of body chunks
                std::list<Chunk> body;

                //! Recipient email address
                std::string to;

                //! From email address
                std::string from;
            };

            //! Retrieve email message data. Makes stream EOF.
            Data data()
            {
                close();
                return std::move(m_data);
            }

        protected:
            //! %Email body stream buffer
            template<class charT> class StreamBuffer;

            //! %Email message data
            Data m_data;

            //! Flush the buffer and set the stream to EOF.
            virtual void close() =0;
        };

        //! Specialized email body stream buffer for char
        template<>
        class Email_base::StreamBuffer<char>: public WebStreambuf<char>
        {
        private:
            //! Reference to the body chunk list
            std::list<Email_base::Chunk>& m_body;

            //! Setup the stream buffer pointer
            inline void setBufferPtr();
        public:
            inline StreamBuffer(std::list<Chunk>& body);

            //! Empty/flush the buffer
            bool emptyBuffer();

            ~StreamBuffer()
            {
                emptyBuffer();
            }
        };

        //! Specialized email body stream buffer for wchar_t
        template<>
        class Email_base::StreamBuffer<wchar_t>: public WebStreambuf<wchar_t>
        {
        private:
            //! Buffer for wide character stuff
            wchar_t m_buffer[Chunk::capacity];

            //! Reference to the body chunk list
            std::list<Email_base::Chunk>& m_body;
        public:
            inline StreamBuffer(std::list<Chunk>& body);

            //! Empty/flush the buffer
            bool emptyBuffer();

            ~StreamBuffer()
            {
                emptyBuffer();
            }
        };

        //! Object for composing email messages
        /*!
         * Use this class to compose email messages from the template character
         * type. This class inherits from std::basic_ostream so the body can be
         * composed using stream insertion operators. The stream buffer
         * inherits from WebStreambuf so you can use the Encoding manipulators.
         *
         * Use the to() and from() member functions to set the to/from email
         * addresses.
         *
         * @tparam charT Character type for internal processing (wchar_t or
         *               char)
         * @date    December 22, 2018
         * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
         */
        template <class charT>
        class Email: public std::basic_ostream<charT>, public Email_base
        {
        private:
            //! Our stream buffer object
            StreamBuffer<charT> m_streamBuffer;

            void close();

        public:
            Email();

            //! Set the to address
            void to(const std::basic_string<charT>& address);

            //! Set the to address with move semantics
            void to(std::basic_string<charT>&& address);

            //! Set the from address
            void from(const std::basic_string<charT>& address);

            //! Set the from address with move semantics
            void from(std::basic_string<charT>&& address);
        };

        template<> inline void Email<char>::to(const std::string& address)
        {
            m_data.to = address;
        }

        template<> inline void Email<char>::to(std::string&& address)
        {
            m_data.to = std::move(address);
        }

        template<> inline void Email<char>::from(const std::string& address)
        {
            m_data.from = address;
        }

        template<> inline void Email<char>::from(std::string&& address)
        {
            m_data.from = std::move(address);
        }

        template<> void Email<wchar_t>::to(const std::wstring& address);
        template<> inline void Email<wchar_t>::to(std::wstring&& address)
        {
            to(address);
        }

        template<> void Email<wchar_t>::from(const std::wstring& address);
        template<> inline void Email<wchar_t>::from(std::wstring&& address)
        {
            from(address);
        }
    }
}

#endif
