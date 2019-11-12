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


/** \file
 * \brief Base block implementation.
 *
 * The block base class handles the loading of the block in memory using
 * mmap() and gives information such as its type and location.
 */

// self
//
#include    "snapdatabase/block.h"

#include    "snapdatabase/exception.h"
#include    "snapdatabase/structure.h"
#include    "snapdatabase/table.h"

// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



block::block(dbfile::pointer_t f, reference_t offset)
    : f_file(f)
    , f_offset(offset)
{
}


table_pointer_t block::get_table() const
{
    if(f_table == nullptr)
    {
        throw invalid_xml("block::get_table() called before the table was defined.");
    }

    return f_table;
}


void block::set_table(table_pointer_t table)
{
    if(f_table != nullptr)
    {
        throw invalid_xml("block::set_table() called twice.");
    }

    f_table = table;
}


structure::pointer_t block::get_structure() const
{
    return f_structure;
}


dbtype_t block::get_dbtype() const
{
    return f_dbtype;
}


void block::set_dbtype(dbtype_t type)
{
    // TODO: add verifications (i.e. go from FREE to any or any to FREE
    //       and maybe a few others)
    //
    *reinterpret_cast<dbtype_t *>(data(0)) = type;
}


reference_t block::get_offset() const
{
    return f_offset;
}


data_t block::data(reference_t offset)
{
    if(f_data == nullptr)
    {
        f_data = get_table()->get_dbfile()->data(f_offset);
    }

    return f_data + (offset % get_table()->get_page_size());
}


const_data_t block::data(reference_t offset) const
{
    if(f_data == nullptr)
    {
        f_data = get_table()->get_dbfile()->data(f_offset);
    }

    return f_data + (offset % get_table()->get_page_size());
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et