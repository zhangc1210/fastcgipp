/*!
 * @file       curl.cpp
 * @brief      Defines types for composing curls
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

#include "fastcgi++/curl.hpp"
#include "fastcgi++/log.hpp"

#include <curl/curl.h>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <iostream>

template<class charT>
Fastcgipp::Curl<charT>::Curl():
    m_streamBuf(*(new StreamBuf<charT>()))
{
    Curl_base::m_streamBuf.reset(&m_streamBuf);
    this->rdbuf(&m_streamBuf);
}

template<class charT>
Fastcgipp::Curl<charT>::Curl(Curl<charT>&& curl):
    m_streamBuf(curl.m_streamBuf)
{
    curl.rdbuf(nullptr);
    this->rdbuf(&m_streamBuf);
    Curl_base::m_streamBuf = curl.Curl_base::m_streamBuf;
}

template<class charT>
Fastcgipp::Curl<charT>::Curl(const Curl<charT>& curl):
    m_streamBuf(curl.m_streamBuf)
{
    Curl_base::m_streamBuf = curl.Curl_base::m_streamBuf;
}

template<class charT>
void Fastcgipp::Curl_base::StreamBuf<charT>::reset()
{
    this->clear();
    m_responseData.reset();
    m_responseDataSize = 0;
    m_responseDataReserve = 0;
    m_responseHeaders.clear();
}

template<class charT>
void Fastcgipp::Curl<charT>::close()
{
    m_streamBuf.emptyBuffer();
    this->setstate(std::ios_base::eofbit);
    this->setstate(std::ios_base::failbit);
    this->setstate(std::ios_base::badbit);
}

template<class charT>
void Fastcgipp::Curl<charT>::open()
{
    m_streamBuf.reset();
    if(this->rdbuf())
        this->clear();
}

void Fastcgipp::Curl_base::close()
{
    FAIL_LOG("Inside Fastcgipp::Curl_base::close() when shouldn't be");
}

void Fastcgipp::Curl_base::open()
{
    FAIL_LOG("Inside Fastcgipp::Curl_base::open() when shouldn't be");
}

void Fastcgipp::Curl_base::setUrl(const char* const url)
{
    CURL* const& handle(reinterpret_cast<CURL* const&>(m_streamBuf->m_handle));
    curl_easy_setopt(handle, CURLOPT_URL, url);
}

unsigned Fastcgipp::Curl_base::responseCode() const
{
    CURL* const& handle(reinterpret_cast<CURL* const&>(m_streamBuf->m_handle));
    long code;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
    return static_cast<unsigned>(code);
}

void Fastcgipp::Curl_base::verifySSL(bool verify)
{
    CURL* const& handle(reinterpret_cast<CURL* const&>(m_streamBuf->m_handle));
    if(verify)
    {
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 1L);
    }
    else
    {
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
    }
}

void Fastcgipp::Curl_base::setUrl(const std::wstring& url)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try
    {
        setUrl(converter.to_bytes(url));
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in code conversion to utf8 in curl url")
    }
}

void Fastcgipp::Curl_base::addHeader(const char* const header)
{
    curl_slist*& headers(reinterpret_cast<curl_slist*&>(m_streamBuf->m_headers));
    headers = curl_slist_append(headers, header);
}

void Fastcgipp::Curl_base::addHeader(const std::wstring& header)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try
    {
        addHeader(converter.to_bytes(header));
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in code conversion to utf8 in curl add header")
    }
}

void Fastcgipp::Curl_base::prepare()
{
    CURL* const& handle(reinterpret_cast<CURL* const&>(m_streamBuf->m_handle));

    long size=0;
    close();
    for(const auto& chunk: m_streamBuf->m_data)
        size += chunk.size;

    if(size > 0)
    {
        m_streamBuf->m_readCounter = 0;
        curl_easy_setopt(handle, CURLOPT_POST, 1L);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, size);
        curl_easy_setopt(handle, CURLOPT_READFUNCTION, readCallback);
        curl_easy_setopt(handle, CURLOPT_READDATA, m_streamBuf.get());
    }
    else
        curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);

    const curl_slist* const& headers(
            reinterpret_cast<const curl_slist* const&>(m_streamBuf->m_headers));
    if(headers)
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, m_streamBuf->m_errorBuffer);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, m_streamBuf.get());
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, m_streamBuf.get());
}

void Fastcgipp::Curl_base::reset()
{
    CURL* const& handle(reinterpret_cast<CURL* const&>(m_streamBuf->m_handle));
    curl_easy_reset(handle);
    open();
}

Fastcgipp::Curl_base::StreamBuf_base::StreamBuf_base(
        std::list<Fastcgipp::ChunkStreamBuf_base::Chunk>& data):
    m_handle(curl_easy_init()),
    m_headers(nullptr),
    m_data(data)
{
    m_errorBuffer[0] = 0;
}

Fastcgipp::Curl_base::StreamBuf_base::~StreamBuf_base()
{
    CURL* const& handle(reinterpret_cast<CURL* const&>(m_handle));
    curl_slist* const& headers(reinterpret_cast<curl_slist* const&>(m_headers));
    if(headers)
        curl_slist_free_all(headers);
    curl_easy_cleanup(handle);
}

size_t Fastcgipp::Curl_base::readCallback(
        char* destination,
        size_t size,
        size_t items,
        void* object)
{
    StreamBuf_base& streamBuf(*reinterpret_cast<StreamBuf_base*>(object));
    unsigned writeSpace = size*items;
    size_t written = 0;

    while(writeSpace > 0 && !streamBuf.m_data.empty())
    {
        const ChunkStreamBuf_base::Chunk& chunk(streamBuf.m_data.front());
        const char* const start = chunk.data.get()+streamBuf.m_readCounter;
        const size_t actualSize = std::min(
                writeSpace,
                chunk.size-streamBuf.m_readCounter);

        std::copy(start, start+actualSize, destination);

        streamBuf.m_readCounter += actualSize;
        if(streamBuf.m_readCounter >= chunk.size)
        {
            streamBuf.m_data.pop_front();
            streamBuf.m_readCounter = 0;
        }

        written += actualSize;
        destination += actualSize;
        writeSpace -= actualSize;
    }

    return written;
}

size_t Fastcgipp::Curl_base::writeCallback(
        const char* data,
        size_t size,
        size_t items,
        void* object)
{
    StreamBuf_base& streamBuf(*reinterpret_cast<StreamBuf_base*>(object));
    return streamBuf.injectResponse(
            data,
            data+size*items);
}

template<>
size_t Fastcgipp::Curl_base::StreamBuf<char>::injectResponse(
        const char* dataStart,
        const char* dataEnd)
{
    const size_t inputDataSize = dataEnd - dataStart;
    const size_t newResponseDataSize = m_responseDataSize+inputDataSize;
    if(newResponseDataSize > m_responseDataReserve)
    {
        m_responseDataReserve = newResponseDataSize;
        std::unique_ptr<char[]> reallocated(new char[m_responseDataReserve]);
        std::copy(
                m_responseData.get(),
                m_responseData.get()+m_responseDataSize,
                reallocated.get());
        m_responseData = std::move(reallocated);
    }

    std::copy(
            dataStart,
            dataEnd,
            m_responseData.get()+m_responseDataSize);
    m_responseDataSize = newResponseDataSize;

    return inputDataSize;
}

template<>
size_t Fastcgipp::Curl_base::StreamBuf<wchar_t>::injectResponse(
        const char* dataStart,
        const char* dataEnd)
{
    const size_t inputDataSize = dataEnd - dataStart;
    {
        const size_t possibleResponseDataSize=m_responseDataSize+inputDataSize;
        if(possibleResponseDataSize > m_responseDataReserve)
        {
            m_responseDataReserve = possibleResponseDataSize;
            std::unique_ptr<wchar_t[]> reallocated(
                    new wchar_t[m_responseDataReserve]);
            std::copy(
                    m_responseData.get(),
                    m_responseData.get()+m_responseDataSize,
                    reallocated.get());
            m_responseData = std::move(reallocated);
        }
    }

    const std::codecvt_utf8<wchar_t> converter;
    std::codecvt_base::result result;
    mbstate_t state = mbstate_t();
    const char* fromNext;
    wchar_t* toNext;
    wchar_t* to = m_responseData.get()+m_responseDataSize;
    wchar_t* const toEnd = m_responseData.get()+m_responseDataReserve;

    result = converter.in(
            state,
            dataStart,
            dataEnd,
            fromNext,
            to,
            toEnd,
            toNext);

    if(result == std::codecvt_base::error
            || result == std::codecvt_base::noconv)
    {
        ERROR_LOG("injectResponse() code conversion failed");
        return inputDataSize;
    }

    m_responseDataSize += toNext - to;
    return fromNext - dataStart;
}

size_t Fastcgipp::Curl_base::headerCallback(
        const char* buffer,
        size_t size,
        size_t items,
        void* object)
{
    size = size*items;
    StreamBuf_base& streamBuf(*reinterpret_cast<StreamBuf_base*>(object));

    const char* bufferEnd = buffer+size;

    const char* colon = std::find(buffer, bufferEnd, ':');
    if(colon == bufferEnd)
        return size;

    const char* const keyStart=buffer;
    const char* const keyEnd=colon++;

    while(colon < bufferEnd && (
            *colon == ' ' ||
            *colon == '\n' ||
            *colon == '\r' ||
            *colon == '\t'))
        ++colon;
    --bufferEnd;
    while(colon <= bufferEnd && (
            *bufferEnd == ' ' ||
            *bufferEnd == '\n' ||
            *bufferEnd == '\r' ||
            *bufferEnd == '\t'))
        --bufferEnd;
    ++bufferEnd;

    const char* const valueStart = colon;
    const char* const valueEnd = bufferEnd;

    streamBuf.injectHeader(keyStart, keyEnd, valueStart, valueEnd);

    return size;
}

template<>
void Fastcgipp::Curl_base::StreamBuf<char>::injectHeader(
        const char* keyStart,
        const char* keyEnd,
        const char* valueStart,
        const char* valueEnd)
{
    std::string key(keyStart, keyEnd);
    std::string value(valueStart, valueEnd);
    if(key == "Content-Length")
    {
        m_responseDataReserve = std::stoi(value);
        m_responseData.reset(new char[m_responseDataReserve]);
    }

    m_responseHeaders.emplace(std::make_pair(
                std::move(key),
                std::move(value)));
}

template<>
void Fastcgipp::Curl_base::StreamBuf<wchar_t>::injectHeader(
        const char* keyStart,
        const char* keyEnd,
        const char* valueStart,
        const char* valueEnd)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    try
    {
        std::wstring key = converter.from_bytes(keyStart, keyEnd);
        std::wstring value = converter.from_bytes(valueStart, valueEnd);
        if(key == L"Content-Length")
        {
            m_responseDataReserve = std::stoi(value);
            m_responseData.reset(new wchar_t[m_responseDataReserve]);
        }

        m_responseHeaders.emplace(std::make_pair(
                    std::move(key),
                    std::move(value)));
    }
    catch(const std::range_error& e)
    {
        WARNING_LOG("Error in code conversion from utf8 in injectHeader")
    }
}

template class Fastcgipp::Curl<wchar_t>;
template class Fastcgipp::Curl<char>;
