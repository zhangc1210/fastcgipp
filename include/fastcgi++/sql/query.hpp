/*!
 * @file       query.hpp
 * @brief      Declares SQL Query
 * @author     Eddie Carle &lt;eddie@isatec.ca&gt;
 * @date       September 29, 2018
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

#ifndef FASTCGIPP_SQL_QUERY_HPP
#define FASTCGIPP_SQL_QUERY_HPP

#include "fastcgi++/sql/parameters.hpp"
#include "fastcgi++/sql/results.hpp"

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
    //! Contains all fastcgi++ SQL facilities
    namespace SQL
    {
        //! De-templated base class for Query
        struct Query
        {
            //! Statement
            const char* statement;
            
            //! Parameters
            std::shared_ptr<Parameters_base> parameters;

            //! Results
            std::shared_ptr<Results_base> results;

            //! Callback function
            std::function<void()> callback;
        };
    }
}

#endif
