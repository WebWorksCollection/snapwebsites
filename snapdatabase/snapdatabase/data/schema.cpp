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
 * \brief Database file implementation.
 *
 * Each table uses one or more files. Each file is handled by a dbfile
 * object and a corresponding set of blocks.
 */

// self
//
#include    "snapdatabase/data/schema.h"

#include    "snapdatabase/data/convert.h"
#include    "snapdatabase/data/script.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include    <iostream>
#include    <type_traits>


// boost lib
//
#include    <boost/algorithm/string.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace snapdatabase
{



namespace
{





struct_description_t g_column_description[] =
{
    define_description(
          FieldName("hash")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT128)
    ),
    define_description(
          FieldName("name")
        , FieldType(struct_type_t::STRUCT_TYPE_P8STRING)
    ),
    define_description(
          FieldName("column_id")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT16)
    ),
    define_description(
          FieldName("type")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT16)
    ),
    define_description(
          FieldName("flags=limited/required/blob/system/revision_type:2")
        , FieldType(struct_type_t::STRUCT_TYPE_BITS32)
    ),
    define_description(
          FieldName("encrypt_key_name")
        , FieldType(struct_type_t::STRUCT_TYPE_P16STRING)
    ),
    define_description(
          FieldName("default_value")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
    ),
    define_description(
          FieldName("minimum_value")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
    ),
    define_description(
          FieldName("maximum_value")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
    ),
    define_description(
          FieldName("minimum_length")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("maximum_length")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("validation")
        , FieldType(struct_type_t::STRUCT_TYPE_BUFFER32)
    ),
    end_descriptions()
};


struct_description_t g_table_column_reference[] =
{
    define_description(
          FieldName("column_id")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT16)
    ),
    end_descriptions()
};


struct_description_t g_table_secondary_index[] =
{
    define_description(
          FieldName("name")
        , FieldType(struct_type_t::STRUCT_TYPE_P8STRING)
    ),
    define_description(
          FieldName("flags=distributed")
        , FieldType(struct_type_t::STRUCT_TYPE_BITS32)
        , FieldSubDescription(g_table_column_reference)
    ),
    define_description(
          FieldName("columns")
        , FieldType(struct_type_t::STRUCT_TYPE_ARRAY16)
        , FieldSubDescription(g_table_column_reference)
    ),
    end_descriptions()
};




struct_description_t g_table_description[] =
{
    define_description(
          FieldName("version")
        , FieldType(struct_type_t::STRUCT_TYPE_VERSION)
    ),
    define_description(
          FieldName("name")
        , FieldType(struct_type_t::STRUCT_TYPE_P8STRING)
    ),
    define_description(
          FieldName("flags=temporary/sparse")
        , FieldType(struct_type_t::STRUCT_TYPE_BITS64)
    ),
    define_description(
          FieldName("block_size")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT32)
    ),
    define_description(
          FieldName("model")
        , FieldType(struct_type_t::STRUCT_TYPE_UINT8)
    ),
    define_description(
          FieldName("row_key")
        , FieldType(struct_type_t::STRUCT_TYPE_ARRAY16)
        , FieldSubDescription(g_table_column_reference)
    ),
    define_description(
          FieldName("secondary_indexes")
        , FieldType(struct_type_t::STRUCT_TYPE_ARRAY16)
        , FieldSubDescription(g_table_secondary_index)
    ),
    define_description(
          FieldName("columns")
        , FieldType(struct_type_t::STRUCT_TYPE_ARRAY16)
        , FieldSubDescription(g_column_description)
    ),
    end_descriptions()
};





bool validate_name(std::string const & name, size_t max_length = 255)
{
    if(name.empty())
    {
        return false;
    }
    if(name.length() > max_length)
    {
        return false;
    }

    char c(name[0]);
    if((c < 'a' || c > 'z')
    && (c < 'A' || c > 'Z')
    && c != '_')
    {
        return false;
    }

    auto const max(name.length());
    for(std::remove_const<decltype(max)>::type idx(0); idx < max; ++idx)
    {
        c = name[idx];
        if((c < 'a' || c > 'z')
        && (c < 'A' || c > 'Z')
        && (c < '0' || c > '9')
        && c != '_')
        {
            return false;
        }
    }

    return true;
}


}
// no name namespace



struct model_and_name_t
{
    model_t const           f_model = model_t::TABLE_MODEL_CONTENT;
    char const * const      f_name = nullptr;
};

#define MODEL_AND_NAME(name)    { model_t::TABLE_MODEL_##name, #name }

model_and_name_t g_model_and_name[] =
{
    MODEL_AND_NAME(CONTENT),
    MODEL_AND_NAME(DATA),
    MODEL_AND_NAME(DEFAULT),
    MODEL_AND_NAME(LOG),
    MODEL_AND_NAME(QUEUE),
    MODEL_AND_NAME(SEQUENCIAL),
    MODEL_AND_NAME(SESSION),
    MODEL_AND_NAME(TREE)
};


model_t name_to_model(std::string const & name)
{
#ifdef _DEBUG
    // verify in debug because if not in order we can't do a binary search
    for(size_t idx(1);
        idx < sizeof(g_model_and_name) / sizeof(g_model_and_name[0]);
        ++idx)
    {
        if(strcmp(g_model_and_name[idx - 1].f_name
                , g_model_and_name[idx].f_name) >= 0)
        {
            throw snapdatabase_logic_error(
                      "names in g_model_and_name are not in alphabetical order: "
                    + std::string(g_model_and_name[idx - 1].f_name)
                    + " >= "
                    + g_model_and_name[idx].f_name
                    + " (position: "
                    + std::to_string(idx)
                    + ").");
        }
    }
#endif

    if(name.empty())
    {
        return model_t::TABLE_MODEL_DEFAULT;
    }

    std::string const uc(boost::algorithm::to_upper_copy(name));

    int i(0);
    int j(sizeof(g_model_and_name) / sizeof(g_model_and_name[0]));
    while(i < j)
    {
        int const p((j - i) / 2 + i);
        int const r(uc.compare(g_model_and_name[p].f_name));
        if(r < 0)
        {
            i = p + 1;
        }
        else if(r > 0)
        {
            j = p;
        }
        else
        {
            return g_model_and_name[p].f_model;
        }
    }

    SNAP_LOG_WARNING
        << "Unknown model name \""
        << name
        << "\" for your table. Please check the spelling. The name is case insensitive."
        << SNAP_LOG_SEND;

    // return the default, this is just a warning
    //
    return model_t::TABLE_MODEL_DEFAULT;
}





// required constructor for copying in the map
schema_complex_type::schema_complex_type()
{
}


/** \brief Initialize a complex type from an XML node.
 *
 * Once in a list of columns, a complex type becomes a
 * `STRUCT_TYPE_STRUCTURE`.
 */
schema_complex_type::schema_complex_type(xml_node::pointer_t x)
{
    if(x->tag_name() != "complex-type")
    {
        throw invalid_xml(
                  "A column schema must be a \"column\" tag. \""
                + x->tag_name()
                + "\" is not acceptable.");
    }

    f_name = x->attribute("name");

    struct_type_t last_type(struct_type_t::STRUCT_TYPE_VOID);
    for(auto child(x->first_child()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "type")
        {
            if(last_type == struct_type_t::STRUCT_TYPE_END)
            {
                throw invalid_xml(
                          "The complex type was already ended with an explicit END. You can have additional types after that. Yet \""
                        + child->text()
                        + "\" was found after the END.");
            }
            field_t ct;
            ct.f_name = child->attribute("name");
            ct.f_type = name_to_struct_type(child->text());
            if(ct.f_type == INVALID_STRUCT_TYPE)
            {
                throw invalid_xml(
                          "Found unknown type \""
                        + child->text()
                        + "\" in your complex type definition.");
            }
            last_type = ct.f_type;

            if(ct.f_type != struct_type_t::STRUCT_TYPE_END)
            {
                f_fields.push_back(ct);
            }
        }
        else
        {
            SNAP_LOG_WARNING
                << "Unknown tag \""
                << child->tag_name()
                << "\" within a <complex-type> tag ignored."
                << SNAP_LOG_SEND;
        }
    }
}


std::string schema_complex_type::name() const
{
    return f_name;
}


size_t schema_complex_type::size() const
{
    return f_fields.size();
}


std::string schema_complex_type::type_name(int idx) const
{
    if(static_cast<std::size_t>(idx) >= f_fields.size())
    {
        throw snapdatabase_out_of_range(
                "index ("
                + std::to_string(idx)
                + ") is too large for this complex type list of fields (max: "
                + std::to_string(f_fields.size())
                + ").");
    }

    return f_fields[idx].f_name;
}


struct_type_t schema_complex_type::type(int idx) const
{
    if(static_cast<std::size_t>(idx) >= f_fields.size())
    {
        throw snapdatabase_out_of_range(
                "index ("
                + std::to_string(idx)
                + ") is too large for this complex type list of fields (max: "
                + std::to_string(f_fields.size())
                + ").");
    }

    return f_fields[idx].f_type;
}






schema_column::schema_column(schema_table::pointer_t table, xml_node::pointer_t x)
    : f_schema_table(table)
{
    if(x->tag_name() != "column")
    {
        throw invalid_xml(
                  "A column schema must be a \"column\" tag. \""
                + x->tag_name()
                + "\" is not acceptable.");
    }

    f_name = x->attribute("name");
    if(!validate_name(f_name))
    {
        throw invalid_xml(
                  "\""
                + f_name
                + "\" is not a valid column name.");
    }

    f_type = name_to_struct_type(x->attribute("type"));
    if(f_type == INVALID_STRUCT_TYPE)
    {
        // TODO: search for complex type first
        //
        throw invalid_xml(
                  "Found unknown type \""
                + x->attribute("type")
                + "\" in your column definition.");
    }

    f_flags = 0;
    if(x->attribute("limited") == "limited")
    {
        f_flags |= COLUMN_FLAG_LIMITED;
    }
    if(x->attribute("required") == "required")
    {
        f_flags |= COLUMN_FLAG_REQUIRED;
    }
    if(x->attribute("blob") == "blob")
    {
        f_flags |= COLUMN_FLAG_BLOB;
    }

    f_encrypt_key_name = x->attribute("encrypt");

    for(auto child(x->first_child()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "description")
        {
            f_description = child->text();
        }
        else if(child->tag_name() == "default")
        {
            f_default_value = string_to_typed_buffer(f_type, child->text());
        }
        else if(child->tag_name() == "external")
        {
            f_internal_size_limit = convert_to_int(child->text(), 32, unit_t::UNIT_SIZE);
        }
        else if(child->tag_name() == "min-value")
        {
            f_minimum_value = string_to_typed_buffer(f_type, child->text());
        }
        else if(child->tag_name() == "max-value")
        {
            f_maximum_value = string_to_typed_buffer(f_type, child->text());
        }
        else if(child->tag_name() == "min-length")
        {
            f_minimum_length = convert_to_uint(child->text(), 32);
        }
        else if(child->tag_name() == "max-length")
        {
            f_maximum_length = convert_to_uint(child->text(), 32);
        }
        else if(child->tag_name() == "validation")
        {
            f_validation = compile_script(child->text());
        }
        else
        {
            // generate an error for unknown tags or ignore?
            //
            SNAP_LOG_WARNING
                << "Unknown tag \""
                << child->tag_name()
                << "\" within a <column> tag ignored."
                << SNAP_LOG_SEND;
        }
    }
}


schema_column::schema_column(schema_table::pointer_t table, structure::pointer_t s)
    : f_schema_table(table)
{
    from_structure(s);
}


schema_column::schema_column(
              schema_table_pointer_t table
            , std::string name
            , struct_type_t type
            , flag32_t flags)
    : f_name(name)
    , f_type(type)
    , f_flags(flags)
    , f_schema_table(table)
{
}


void schema_column::from_structure(structure::pointer_t s)
{
    auto const large_uint(s->get_large_uinteger("hash"));
    f_hash[0] = large_uint.f_value[0];
    f_hash[1] = large_uint.f_value[1];
    f_name = s->get_string("name");
    f_column_id = s->get_uinteger("column_id");
    f_type = static_cast<struct_type_t>(s->get_uinteger("type"));
    f_flags = s->get_uinteger("flags");
    f_encrypt_key_name = s->get_string("encrypt_key_name");
    f_default_value = s->get_buffer("default_value");
    f_minimum_value = s->get_buffer("minimum_value");
    f_maximum_value = s->get_buffer("maximum_value");
    f_minimum_length = s->get_uinteger("minimum_length");
    f_maximum_length = s->get_uinteger("maximum_length");
    f_validation = s->get_buffer("validation");
}


schema_table::pointer_t schema_column::table() const
{
    return f_schema_table.lock();
}


column_id_t schema_column::column_id() const
{
    return f_column_id;
}


void schema_column::set_column_id(column_id_t id)
{
    if(f_column_id != COLUMN_NULL)
    {
        throw id_already_assigned(
                  "This column already has an identifier ("
                + std::to_string(static_cast<int>(f_column_id))
                + "). You can't assigned it another one.");
    }

    f_column_id = id;
}


void schema_column::hash(uint64_t & h0, uint64_t & h1) const
{
    h0 = f_hash[0];
    h1 = f_hash[1];
}


std::string schema_column::name() const
{
    return f_name;
}


struct_type_t schema_column::type() const
{
    return f_type;
}


flag32_t schema_column::flags() const
{
    return f_flags;
}


std::string schema_column::encrypt_key_name() const
{
    return f_encrypt_key_name;
}


buffer_t schema_column::default_value() const
{
    return f_default_value;
}


buffer_t schema_column::minimum_value() const
{
    return f_minimum_value;
}


buffer_t schema_column::maximum_value() const
{
    return f_maximum_value;
}


std::uint32_t schema_column::minimum_length() const
{
    return f_minimum_length;
}


std::uint32_t schema_column::maximum_length() const
{
    return f_maximum_length;
}


buffer_t schema_column::validation() const
{
    return f_validation;
}













std::string schema_secondary_index::get_index_name() const
{
    return f_index_name;
}


void schema_secondary_index::set_index_name(std::string const & index_name)
{
    f_index_name = index_name;
}


bool schema_secondary_index::get_distributed_index() const
{
    return (f_flags & SECONDARY_INDEX_FLAG_DISTRIBUTED) != 0;
}


void schema_secondary_index::set_distributed_index(bool distributed)
{
    if(distributed)
    {
        f_flags |= SECONDARY_INDEX_FLAG_DISTRIBUTED;
    }
    else
    {
        f_flags &= ~SECONDARY_INDEX_FLAG_DISTRIBUTED;
    }
}


size_t schema_secondary_index::get_column_count()
{
    return f_column_ids.size();
}


column_id_t schema_secondary_index::get_column_id(int idx)
{
    if(static_cast<size_t>(idx) >= f_column_ids.size())
    {
        throw snapdatabase_out_of_range(
                  "Index ("
                + std::to_string(idx)
                + ") is too large to pick a column identifier.");
    }

    return f_column_ids[idx];
}


void schema_secondary_index::add_column_id(column_id_t id)
{
    f_column_ids.push_back(id);
}












void schema_table::from_xml(xml_node::pointer_t x)
{
    if(x->tag_name() != "table")
    {
        throw invalid_xml(
                  "A table schema must be a \"keyspaces\" or \"context\". \""
                + x->tag_name()
                + "\" is not acceptable.");
    }

    // start at version 1.0
    //
    f_version.set_major(1);

    f_name = x->attribute("name");
    if(!validate_name(f_name))
    {
        throw invalid_xml(
                  "\""
                + f_name
                + "\" is not a valid table name.");
    }

    bool drop(x->attribute("drop") == "drop");
    if(drop)
    {
        // do not ever save a table when the DROP flag is set (actually
        // we want to delete the entire folder if it still exists!)
        //
        f_flags |= TABLE_FLAG_DROP;
        return;
    }

    if(x->attribute("temporary") == "temporary")
    {
        f_flags |= TABLE_FLAG_TEMPORARY;
    }

    if(x->attribute("sparse") == "sparse")
    {
        f_flags |= TABLE_FLAG_SPARSE;
    }

    if(x->attribute("secure") == "secure")
    {
        f_flags |= TABLE_FLAG_SECURE;
    }

    xml_node::deque_t schemata;
    xml_node::deque_t secondary_indexes;

    f_model = name_to_model(x->attribute("model"));

    // 1. fully parse the complex types on the first iteration
    //
std::cerr << "schema: parse complex types\n";
    for(auto child(x->first_child()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "block-size")
        {
            f_block_size = convert_to_uint(child->text(), 32);

            // TBD--we adjust the size in dbfile
            //size_t const page_size(dbfile::get_system_page_size());
            //if((f_block_size % page_size) != 0)
            //{
            //    throw invalid_xml(
            //              "Table \""
            //            + f_name
            //            + "\" is not compatible, block size "
            //            + std::to_string(f_block_size)
            //            + " is not supported because it is not an exact multiple of "
            //            + std::to_string(page_size)
            //            + ".");
            //}
        }
        else if(child->tag_name() == "description")
        {
            if(!f_description.empty())
            {
                throw invalid_xml(
                          "Table \""
                        + f_name
                        + "\" has two <description> tags, only one is allowed.");
            }
            f_description = child->text();
        }
        else if(child->tag_name() == "schema")
        {
            schemata.push_back(child);
        }
        else if(child->tag_name() == "secondary-index")
        {
            secondary_indexes.push_back(child);
        }
        else if(child->tag_name() == "complex-type")
        {
            schema_complex_type ct(child);
            f_complex_types[ct.name()] = ct;
        }
        else
        {
            // generate an error for unknown tags or ignore?
            //
            SNAP_LOG_WARNING
                << "Unknown tag \""
                << child->tag_name()
                << "\" within <table name=\""
                << f_name
                << "\"> tag ignored."
                << SNAP_LOG_SEND;
        }
    }

    // 2. add system columns and parse user defined columns
    //
    //column_id_t col_id(1); -- TBD
std::cerr << "schema: add system columns\n";

    // object identifier -- to place the rows in our indirect index
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_oid"
                    , struct_type_t::STRUCT_TYPE_UINT64
                    , COLUMN_FLAG_REQUIRED | COLUMN_FLAG_SYSTEM));

        //f_columns_by_id[c->column_id()] = c;
        f_columns_by_name[c->name()] = c;
    }

    // date when the row was created
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_created_on"
                    , struct_type_t::STRUCT_TYPE_USTIME
                    , COLUMN_FLAG_SYSTEM));

        //f_columns_by_id[c->column_id()] = c;
        f_columns_by_name[c->name()] = c;
    }

    // when the row was last updated
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_last_updated"
                    , struct_type_t::STRUCT_TYPE_USTIME
                    , COLUMN_FLAG_REQUIRED | COLUMN_FLAG_SYSTEM));

        //f_columns_by_id[c->column_id()] = c;
        f_columns_by_name[c->name()] = c;
    }

    // the date when it gets deleted automatically
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_delete_on"
                    , struct_type_t::STRUCT_TYPE_USTIME
                    , COLUMN_FLAG_SYSTEM));

        //f_columns_by_id[c->column_id()] = c;
        f_columns_by_name[c->name()] = c;
    }

    // ID of user who created this row
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_created_by"
                    , struct_type_t::STRUCT_TYPE_UINT64
                    , COLUMN_FLAG_SYSTEM));

        //f_columns_by_id[c->column_id()] = c;
        f_columns_by_name[c->name()] = c;
    }

    // ID of user who last updated this row
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_updated_by"
                    , struct_type_t::STRUCT_TYPE_UINT64
                    , COLUMN_FLAG_SYSTEM));

        //f_columns_by_id[c->column_id()] = c;
        f_columns_by_name[c->name()] = c;
    }

    // ID of user who deleted this row
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_deleted_by"
                    , struct_type_t::STRUCT_TYPE_UINT64
                    , COLUMN_FLAG_SYSTEM));

        //f_columns_by_id[c->column_id()] = c;
        f_columns_by_name[c->name()] = c;
    }

    // version of this row (TBD TBD TBD)
    //
    // how this will be implemented is not clear at this point--it will
    // only be for the `content` table; the version itself would not be
    // saved as a column per se, instead it would be a form of sub-index
    // where the version is ignored for fields that are marked `global`,
    // only the `major` part is used for fields marked as `branch`, and
    // both, `major` and `minor` are used for fields marked as
    // `revision`... as far as the client is concerned, though, it look
    // like we have a full version column.
    {
        auto c(std::make_shared<schema_column>(
                      shared_from_this()
                    , "_version"
                    , struct_type_t::STRUCT_TYPE_VERSION
                    , COLUMN_FLAG_SYSTEM));

        //f_columns_by_id[c->column_id()] = c;
        f_columns_by_name[c->name()] = c;
    }

std::cerr << "schema: parse columns\n";
    for(auto const & child : schemata)
    {
        for(auto column(child->first_child());
            column != nullptr;
            column = column->next())
        {
            auto c(std::make_shared<schema_column>(shared_from_this(), column)); // TBD: + col_id?
            if(f_columns_by_name.find(c->name()) != f_columns_by_name.end())
            {
                throw invalid_xml(
                          "Column \""
                        + f_name
                        + "."
                        + c->name()
                        + "\" defined twice.");
            }

            //f_columns_by_id[c->column_id()] = c;
            f_columns_by_name[c->name()] = c;
            //++col_id; -- TBD
        }
    }

    // 3. the row-key is transformed in an array of column identifiers
    //
    // the parameter in the XML is a string of column names separated
    // by commas
    //
    std::string row_key_name(x->attribute("row-key"));

    advgetopt::string_list_t row_key_names;
    advgetopt::split_string(row_key_name, row_key_names, {","});

std::cerr << "schema: parse key names\n";
    for(auto const & n : row_key_names)
    {
        schema_column::pointer_t c(column(n));
        if(c == nullptr)
        {
            throw invalid_xml(
                      "A column referenced in the row-key attribute of table \""
                    + f_name
                    + "\" must exist. We could not find \""
                    + f_name
                    + "."
                    + n
                    + "\".");
        }
        f_row_key.push_back(c->column_id());
    }

    // 4. the secondary indexes are transformed to array of columns
    //
std::cerr << "schema: secondary indexes\n";
    for(auto const & si : secondary_indexes)
    {
        schema_secondary_index::pointer_t index(std::make_shared<schema_secondary_index>());
        index->set_index_name(si->attribute("name"));

        std::string const distributed(si->attribute("distributed"));
        if(distributed.empty() || distributed == "distributed")
        {
            index->set_distributed_index(true);
        }
        else if(distributed == "one-instance")
        {
            index->set_distributed_index(false);
        }
        else
        {
            SNAP_LOG_WARNING
                << "Unknown distributed attribute value \""
                << distributed
                << "\" within a <secondary-index> tag ignored."
                << SNAP_LOG_SEND;
        }

        std::string const columns(si->text());
        advgetopt::string_list_t column_names;
        advgetopt::split_string(
                  columns
                , column_names
                , {","});

        for(auto const & n : column_names)
        {
            schema_column::pointer_t c(column(n));
            if(c == nullptr)
            {
                throw invalid_xml(
                          "A column referenced in the secondary-index of table \""
                        + f_name
                        + "\" must exist. We could not find \""
                        + f_name
                        + "."
                        + n
                        + "\".");
            }
            index->add_column_id(c->column_id());
        }

        f_secondary_indexes.push_back(index);
    }
}


void schema_table::load_extension(xml_node::pointer_t e)
{
    // determine the largest column identifier, but really this is not
    // the right way of assigning the ids
    //
    column_id_t col_id(0);
    for(auto const & c : f_columns_by_id)
    {
        if(c.second->column_id() > col_id)
        {
            col_id = c.second->column_id();
        }
    }
    ++col_id;

    for(auto child(e->first_child()); child != nullptr; child = child->next())
    {
        if(child->tag_name() == "schema")
        {
            // TODO: move to sub-function & make sure we do not get duplicates
            for(auto column(child->first_child());
                column != nullptr;
                column = column->next())
            {
                auto c(std::make_shared<schema_column>(shared_from_this(), column));
                f_columns_by_id[c->column_id()] = c;
                f_columns_by_name[c->name()] = c;
                ++col_id;
            }
        }
        // TODO: once we have a better handle on column identifiers?
        //else if(child->tag_name() == "secondary-index")
        //{
        //    secondary_index_t si;
        //    si.f_name = child->attribute("name");
        //    si.f_columns = child->attribute("columns");
        //    secondary_indexes.push_back(si);
        //}
        else
        {
            // generate an error for unknown tags or ignore?
            //
            SNAP_LOG_WARNING
                << "Unknown tag \""
                << child->tag_name()
                << "\" within a <table-extension> tag ignored."
                << SNAP_LOG_SEND;
        }
    }
}


void schema_table::from_binary(virtual_buffer::pointer_t b)
{
    structure::pointer_t s(std::make_shared<structure>(g_table_description));
    
    s->set_virtual_buffer(b, 0);

    f_version = s->get_uinteger("version");
    f_name = s->get_string("name");
    f_flags = s->get_uinteger("flags");
    f_model = static_cast<model_t>(s->get_uinteger("model"));

    {
        auto const field(s->get_field("row_key"));
        auto const max(field->size());
        for(std::remove_const<decltype(max)>::type idx(0); idx < max; ++idx)
        {
            f_row_key.push_back((*field)[idx]->get_uinteger("column_id"));
        }
    }

    {
        auto const field(s->get_field("secondary_indexes"));
        auto const max(field->size());
        for(std::remove_const<decltype(max)>::type idx(0); idx < max; ++idx)
        {
            schema_secondary_index::pointer_t secondary_index(std::make_shared<schema_secondary_index>());

            secondary_index->set_index_name((*field)[idx]->get_string("name"));

            auto const columns_field((*field)[idx]->get_field("columns"));
            auto const columns_max(columns_field->size());
            for(std::remove_const<decltype(columns_max)>::type j(0); j < columns_max; ++j)
            {
                secondary_index->add_column_id((*field)[idx]->get_uinteger("column_id"));
            }

            f_secondary_indexes.push_back(secondary_index);
        }
    }

    {
        auto field(s->get_field("columns"));
        auto const max(field->size());
        for(std::remove_const<decltype(max)>::type idx(0); idx < max; ++idx)
        {
            schema_column::pointer_t column(std::make_shared<schema_column>(shared_from_this(), (*field)[idx]));
            if(column->column_id() != 0)
            {
                throw id_missing(
                      "loaded column \""
                    + column->name()
                    + "\" from the database and its column identifier is 0.");
            }

            f_columns_by_name[column->name()] = column;
            f_columns_by_id[column->column_id()] = column;
        }
    }
}


virtual_buffer::pointer_t schema_table::to_binary() const
{
std::cerr << "creating a binary schema from a schema_table and its children\n";
    structure::pointer_t s(std::make_shared<structure>(g_table_description));
    s->init_buffer();
    s->set_uinteger("version", f_version.to_binary());
    s->set_string("name", f_name);
    s->set_uinteger("flags", f_flags);
    s->set_uinteger("model", static_cast<uint8_t>(f_model));

{
std::cerr << "-------------------------------------------- HEADER?\n";
reference_t offset;
std::cerr << *s->get_virtual_buffer(offset);
std::cerr << "--------------------------------------------\n";
}

std::cerr << "   + row_key " << f_row_key.size() << "\n";
    {
        structure::vector_t v;
        auto const max(f_row_key.size());
        for(std::remove_const<decltype(max)>::type i(0); i < max; ++i)
        {
            structure::pointer_t column_id_structure(std::make_shared<structure>(g_table_column_reference));
            column_id_structure->init_buffer();
            column_id_structure->set_uinteger("column_id", f_row_key[i]);
            v.push_back(column_id_structure);
        }
        s->set_array("row_key", v);
    }

std::cerr << "   + secondary indexes " << f_secondary_indexes.size() << "\n";
    {
        structure::vector_t v;
        auto const max(f_secondary_indexes.size());
        for(std::remove_const<decltype(max)>::type i(0); i < max; ++i)
        {
            structure::pointer_t secondary_index_structure(std::make_shared<structure>(g_table_secondary_index));
            secondary_index_structure->init_buffer();
            secondary_index_structure->set_string("name", f_secondary_indexes[i]->get_index_name());

            structure::vector_t c;
            auto const jmax(f_secondary_indexes[i]->get_column_count());
            for(std::remove_const<decltype(max)>::type j(0); j < jmax; ++j)
            {
                structure::pointer_t column_id_structure(std::make_shared<structure>(g_table_column_reference));
                column_id_structure->init_buffer();
                column_id_structure->set_uinteger("column_id", f_secondary_indexes[i]->get_column_id(j));
                c.push_back(column_id_structure);
            }

            secondary_index_structure->set_array("columns", c);

            v.push_back(secondary_index_structure);
        }

        s->set_array("secondary_indexes", v);
    }

std::cerr << "   + columns " << f_columns_by_name.size() << "/" << f_columns_by_id.size() << "\n";
    {
        for(auto const & col : f_columns_by_id)
        {
std::cerr << "----------------------------\n"
          << "   -> get one column pointer\n";
            structure::pointer_t column_description(s->new_array_item("columns"));
            //column_description->init_buffer();
std::cerr << "   -> save one column first [" << col.second->name() << "]\n";
            column_description->set_string("name", col.second->name());
            uint512_t hash;
std::cerr << "   -> column hash\n";
            col.second->hash(hash.f_value[0], hash.f_value[1]);
            column_description->set_large_uinteger("hash", hash);
std::cerr << "   -> column column_id\n";
            column_description->set_uinteger("column_id", col.second->column_id());
std::cerr << "   -> column type\n";
            column_description->set_uinteger("type", static_cast<uint16_t>(col.second->type()));
std::cerr << "   -> column flags\n";
            column_description->set_uinteger("flags", col.second->flags());
std::cerr << "   -> column encrypt\n";
            column_description->set_string("encrypt_key_name", col.second->encrypt_key_name());
std::cerr << "   -> column default\n";
            column_description->set_buffer("default_value", col.second->default_value());
std::cerr << "   -> column min\n";
            column_description->set_buffer("minimum_value", col.second->minimum_value());
std::cerr << "   -> column max\n";
            column_description->set_buffer("maximum_value", col.second->maximum_value());
std::cerr << "   -> column min len: " << col.second->minimum_length() << "\n";
            column_description->set_uinteger("minimum_length", col.second->minimum_length());
std::cerr << "   -> column max len: " << col.second->maximum_length() << "\n";
            column_description->set_uinteger("maximum_length", col.second->maximum_length());
std::cerr << "   -> column valid\n";
            column_description->set_buffer("validation", col.second->validation());
        }
    }

    // we know it is zero so we ignore that one anyay
    //
std::cerr << "   + convert structure to virtual buffer\n";
    uint64_t start_offset(0);
    return s->get_virtual_buffer(start_offset);
}


version_t schema_table::version() const
{
    return f_version;
}


std::string schema_table::name() const
{
    return f_name;
}


model_t schema_table::model() const
{
    return f_model;
}


bool schema_table::is_sparse() const
{
    return (f_flags & TABLE_FLAG_SPARSE) != 0;
}


bool schema_table::is_secure() const
{
    return (f_flags & TABLE_FLAG_SECURE) != 0;
}


column_ids_t schema_table::row_key() const
{
    return f_row_key;
}


void schema_table::assign_column_ids()
{
    if(!f_columns_by_id.empty())
    {
        return;
    }

    column_id_t id(1);
    for(auto c : f_columns_by_name)
    {
        c.second->set_column_id(id);
        f_columns_by_id[id] = c.second;
        ++id;
    }
}


schema_column::pointer_t schema_table::column(std::string const & name) const
{
    auto it(f_columns_by_name.find(name));
    if(it == f_columns_by_name.end())
    {
        return schema_column::pointer_t();
    }
    return it->second;
}


schema_column::pointer_t schema_table::column(column_id_t id) const
{
    auto it(f_columns_by_id.find(id));
    if(it == f_columns_by_id.end())
    {
        return schema_column::pointer_t();
    }
    return it->second;
}


schema_column::map_by_name_t schema_table::columns_by_name() const
{
    return f_columns_by_name;
}


schema_column::map_by_id_t schema_table::columns_by_id() const
{
    return f_columns_by_id;
}


std::string schema_table::description() const
{
    return f_description;
}


std::uint32_t schema_table::block_size() const
{
    return f_block_size;
}



} // namespace snapdatabase
// vim: ts=4 sw=4 et
