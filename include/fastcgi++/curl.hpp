/*!
 * @file       curl.hpp
 * @brief      Declares types for composing HTTP requests
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       June 22, 2020
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

#ifndef FASTCGIPP_CURL_HPP
#define FASTCGIPP_CURL_HPP

#include <memory>
#include <ostream>
#include <list>
#include <functional>

#include "fastcgi++/chunkstreambuf.hpp"
#include "fastcgi++/message.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! De-templated base class for individual Curl requests
    /*!
     * @date    June 22, 2020
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    class Curl_base
    {
    private:
        friend class Curler;

        //! Callback for libcurl to read POST data
        static size_t readCallback(
                char* destination,
                size_t size,
                size_t items,
                void* object);

        //! Callback for libcurl to write received data
        static size_t writeCallback(
                const char* data,
                size_t size,
                size_t items,
                void* object);

        //! Call for libcurl to write recieved headers
        static size_t headerCallback(
                const char* buffer,
                size_t size,
                size_t items,
                void* object);

        //! Called internally to mark the stream as unusable
        virtual void close();

        //! Called internally to re-open the stream for reuse 
        virtual void open();

        //! Return the associated easy curl handle
        void* const& handle()
        {
            return m_streamBuf->m_handle;
        }

        //! Call the request complete callback function
        void callback(int messageType=80)
        {
            if(m_streamBuf->m_callback)
                m_streamBuf->m_callback(messageType);
        }

        //! Prepare the curl request to be sent out
        void prepare();
    protected:
        //! De-templated base class for StreamBuf
        class StreamBuf_base
        {
        public:
            //! The libcurl easy handle
            void* const m_handle;

            //! The libcurl headers list
            void* m_headers;

            //! A reference to the child class's POST data linked chunk list
            std::list<ChunkStreamBuf_base::Chunk>& m_data;

            //! An internal read counter for pulling data out of m_data
            unsigned m_readCounter;

            //! Call function for when the request is complete
            std::function<void(Message)> m_callback;

            //! Curl Error Buffer
            char m_errorBuffer[256];

            virtual ~StreamBuf_base();

            //! Called by headerCallback() to inject headers into the child
            virtual void injectHeader(
                    const char* keyStart,
                    const char* keyEnd,
                    const char* valueStart,
                    const char* valueEnd) =0;

            //! Called by writeCallback() to inject response data into the child
            virtual size_t injectResponse(
                    const char* dataStart,
                    const char* dataEnd) =0;
        protected:
            //! Only to be called by the child class
            StreamBuf_base(std::list<ChunkStreamBuf_base::Chunk>& data);
        };

        //! Shared pointer to stream buffer base object. It is managed here.
        std::shared_ptr<StreamBuf_base> m_streamBuf;

        //! Templated stream buffer for curl requests
        template<class charT> class StreamBuf:
            public ChunkStreamBuf<charT>,
            public StreamBuf_base
        {
        private:
            void injectHeader(
                    const char* keyStart,
                    const char* keyEnd,
                    const char* valueStart,
                    const char* valueEnd);
            size_t injectResponse(
                    const char* dataStart,
                    const char* dataEnd);
        public:
            StreamBuf():
                StreamBuf_base(ChunkStreamBuf<charT>::m_body),
                m_responseDataReserve(0),
                m_responseDataSize(0)
            {}

            //! Reset the stream buffer a new
            void reset();

            //! Pointer to contiguous received response data
            std::unique_ptr<charT[]> m_responseData;

            //! Total allocated size of response data array
            size_t m_responseDataReserve;

            //! Total used size of response data array
            size_t m_responseDataSize;

            //! Associative array of returned HTTP headers
            std::map<std::basic_string<charT>, std::basic_string<charT>>
                m_responseHeaders;
        };

    public:
        //! Set the function that is to be called when the request is complete
        void setCallback(std::function<void(Message)> callback)
        {
            m_streamBuf->m_callback = callback;
        }

        //! Add a header to the request
        void addHeader(const char* const header);

        //! Add a header to the request
        void addHeader(const std::string& header)
        {
            addHeader(header.c_str());
        }
        //! Add a header to the request
        void addHeader(const std::wstring& header);

        //! Set the URL for the request
        void setUrl(const char* const url);

        //! Set the URL for the request
        void setUrl(const std::string& url)
        {
            setUrl(url.c_str());
        }

        //! Set the URL for the request
        void setUrl(const std::wstring& url);

        //! Should we verify the SSL connection?
        void verifySSL(bool verify);

        //! Reset the curl object to be reused
        void reset();

        //! Get the response code from the last request
        unsigned responseCode() const;

        //! Get the error messsage from the most recent operation
        const char* error() const
        {
            return m_streamBuf->m_errorBuffer;
        }
    };

    //! Object for composing curl messages
    /*!
     * Use this class to compose curl requests. This class inherits from
     * std::basic_ostream so the POST data can be composed using stream
     * insertion operators. The stream buffer inherits from ChunkStreambuf so
     * you can use the Encoding manipulators. Set the various parameters of the
     * request with setUrl(), addHeader() and verifySSL(). Set the callback as
     * for when the request is complete with setCallback(). Queue it into a
     * Curler and once the callback has been called you can access the headers
     * with headers(), the response code with responseCode() and the response
     * data with data() and dataSize().
     *
     * @tparam charT Character type for input and output processing (wchar_t or
     *               char)
     * @date    May 7, 2020
     * @author  Eddie Carle &lt;eddie@isatec.ca&gt;
     */
    template <class charT>
    class Curl: public Curl_base, public std::basic_ostream<charT>
    {
    private:
        //! Our stream buffer object
        StreamBuf<charT>& m_streamBuf;

        //! Empty buffer and close the stream to prevent insertion
        void close();

        //! Re-open the stream to allow insertion
        void open();

    public:
        //! Get the associative container of received headers
        std::map<std::basic_string<charT>, std::basic_string<charT>>& headers()
        {
            return m_streamBuf.m_responseHeaders;
        }

        //! Get the pointer to the start of the contiguous response data
        const charT* data() const
        {
            return m_streamBuf.m_responseData.get();
        }

        //! Get the size of the data pointed to by data()
        size_t dataSize() const
        {
            return m_streamBuf.m_responseDataSize;
        }

        //! Initialize a new fully functional curl request
        Curl();

        //! Construct a fully functional curl from and invalidate the argument
        /*!
         * The result of this will be a constructed curl object that is fully
         * functional. You can write POST data into it, initiate requests and
         * read the response data. The source object, however, becomes
         * semi-invalidated. It can no longer be used to write POST data as the
         * steam component of it loses it's stream buffer.
         */
        Curl(Curl<charT>&& curl);

        //! Construct a read-functional curl
        /*!
         * The result of this will be a constructed curl object that is
         * semi-invalidated. It can not be used to write POST data as the
         * stream component of it has no stream buffer.
         */
        Curl(const Curl<charT>& curl);
    };
}

#endif
