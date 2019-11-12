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
 * \brief Block representing a blob of data.
 *
 * The _blob part_ of a row is that part that does not get indexed or
 * queried in any way other than the whole at once. It is used whenever
 * it is too large to fit in a `DATA` block.
 */

// self
//
#include    "snapdatabase/structure.h"



namespace snapdatabase
{


class block_blob
    : public block
{
public:
    typedef std::shared_ptr<block_blob>       pointer_t;

                                block_blob(dbfile::pointer_t f, reference_t offset);

    uint32_t                    get_size();
    void                        set_size(uint32_t size);
    reference_t                 get_next_blob();
    void                        set_next_blob(reference_t offset);

private:
};




} // namespace snapdatabase
// vim: ts=4 sw=4 et