// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapdatabase
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once


/** \file
 * \brief Convert file header.
 *
 * The convert code is used to transform data from text to binary and vice
 * versa.
 */

// self
//
#include    "snapdatabase/structure.h"



namespace snapdatabase
{



int64_t convert_to_int(std::string const & value, size_t max_size);
uint64_t convert_to_uint(std::string const & value, size_t max_size);

buffer_t string_to_typed_buffer(struct_type_t type, std::string value);
std::string typed_buffer_to_string(struct_type_t type, buffer_t value);



} // namespace snapdatabase
// vim: ts=4 sw=4 et
