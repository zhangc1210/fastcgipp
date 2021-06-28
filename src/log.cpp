/*!
 * @file       log.cpp
 * @brief      Defines the Fastcgipp debugging/logging facilities
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       May 18, 2016
 * @copyright  Copyright &copy; 2016 Eddie Carle. This project is released under
 *             the GNU Lesser General Public License Version 3.
 */

 /*******************************************************************************
 * Copyright (C) 2016 Eddie Carle [eddie@isatec.ca]                             *
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

#include "fastcgi++/log.hpp"

#include <iomanip>
#include <iostream>
#include <ctime>
#include <codecvt>
#include <cstring>
#include <array>
#include <sstream>

#ifndef FASTCGIPP_WINDOWS
#include <unistd.h>
#else
#include <WinSock2.h>
#include "fastcgi++/sockets.hpp"
#endif
#include <limits.h>
#include <sys/types.h>

 //! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	//! Contains the Fastcgipp debugging/logging mechanism
	namespace Logging
	{
		std::wstring getHostname()
		{
#if defined(FASTCGIPP_WINDOWS)
			const int cnBufferLength = 256 + 2;
#else
			const int cnBufferLength = HOST_NAME_MAX + 2;
#endif
			char buffer[cnBufferLength];
			if (0 != gethostname(buffer, sizeof(buffer)))
			{
#if defined(FASTCGIPP_WINDOWS)
				Socket::Startup();
				gethostname(buffer, sizeof(buffer));
#endif
			}
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			try
			{
				return(converter.from_bytes(
					buffer,
					buffer + std::strlen(buffer)));
			}
			catch (const std::range_error& /*e*/)
			{
				WARNING_LOG("Error in hostname code conversion from utf8")
					return std::wstring(L"localhost");
			}

		}

		std::wstring getProgram()
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
			std::wostringstream ss;
			try
			{
#if defined(FASTCGIPP_WINDOWS)
				ss << "FastCGI";
#else
				ss << converter.from_bytes(
					program_invocation_name,
					program_invocation_name
					+ std::strlen(program_invocation_name));
#endif
			}
			catch (const std::range_error& /*e*/)
			{
				WARNING_LOG("Error in program name code conversion from utf8")
					ss << "unknown";
			}

#if defined(FASTCGIPP_WINDOWS)
			ss << '[' << _getpid() << ']';
#else
			ss << '[' << getpid() << ']';
#endif
			return ss.str();
		}

		std::array<std::wstring, 6> levels
		{ {
			L"[info]: ",
			L"[fail]: ",
			L"[error]: ",
			L"[warning]: ",
			L"[debug]: ",
			L"[diagnostic]: "
		} };
	}
}

std::wostream* Fastcgipp::Logging::logstream(&std::wcerr);
std::mutex Fastcgipp::Logging::mutex;
bool Fastcgipp::Logging::suppress(false);
std::wstring Fastcgipp::Logging::hostname(Fastcgipp::Logging::getHostname());
std::wstring Fastcgipp::Logging::program(Fastcgipp::Logging::getProgram());

void Fastcgipp::Logging::header(Level level)
{
	const std::time_t now = std::time(nullptr);
	*logstream
		<< std::put_time(std::localtime(&now), L"%b %d %H:%M:%S ")
		<< hostname << ' ' << program << ' ' << levels[level];
}
