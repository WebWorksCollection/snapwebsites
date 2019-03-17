// Snap Websites Server -- all the user content and much of the system content
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


/** \file
 * \brief The implementation of the content plugin attachment_file class.
 *
 * This file contains the attachment_file class implementation.
 */

#include "content.h"

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(content)



/** \brief Create a structure used to setup an attachment file.
 *
 * This contructor is used whenever loading an attachment from the
 * database. In this case the file is setup from the database
 * information.
 *
 * The other constructor is used when creating an attachment from
 * data received in a POST or generated by a backend. It includes
 * the file information.
 *
 * \param[in] snap  A pointer to the snap_child object.
 */
attachment_file::attachment_file(snap_child *snap)
    : f_snap(snap)
    //, f_file()
    //, f_multiple(false) -- auto-init
    //, f_has_cpath(false) -- auto-init
    //, f_parent_cpath("") -- auto-init
    //, f_field_name("") -- auto-init
    //, f_attachment_cpath("") -- auto-init
    //, f_attachment_owner("") -- auto-init
    //, f_attachment_type("") -- auto-init
    //, f_name("") -- auto-init
    //, f_creation_time(0) -- auto-init
    //, f_update_time(0) -- auto-init
    //, f_dependencies() -- auto-init
{
}


/** \brief Create a structure used to setup an attachment file.
 *
 * Create and properly initialize this structure and then you can call
 * the create_attachment() function which takes this structure as a parameter
 * to create a new file in the database.
 *
 * To finish the initialization of this structure you must call the
 * following functions:
 *
 * \li set_parent_cpath()
 * \li set_field_name()
 * \li set_attachment_cpath()
 * \li set_attachment_owner()
 * \li set_attachment_type()
 *
 * By default the attachment file structure is set to work on unique
 * files. Call the set_multiple() function to make sure that the
 * user does not overwrite previous attachments.
 *
 * \warning
 * Each attachment file structure can really only be used once (it is
 * used for throw away objects.) The get_name() function, for example,
 * generates the name internally and it is not possible to change it
 * afterward.
 *
 * \warning
 * Calling the get_name() function fails with a throw if some of
 * the mandatory parameters were not properly set.
 *
 * \param[in] snap  A pointer to the snap_child object.
 * \param[in] file  The file to attach to this page.
 */
attachment_file::attachment_file(snap_child *snap, snap_child::post_file_t const& file)
    : f_snap(snap)
    , f_file(file)
    //, f_multiple(false) -- auto-init
    //, f_parent_cpath("") -- auto-init
    //, f_field_name("") -- auto-init
    //, f_attachment_cpath("") -- auto-init
    //, f_attachment_owner("") -- auto-init
    //, f_attachment_type("") -- auto-init
{
}


/** \brief Whether multiple files can be saved under this one name.
 *
 * This function is used to mark the attachment as unique (false) or
 * not (true). If unique, saving the attachment again with a different
 * files removes the existing file first.
 *
 * When multiple is set to true, saving a new file adds it to the list
 * of existing files. The list may be empty too.
 *
 * Multiple adds a unique number at the end of each field name
 * which gives us a full name such as:
 *
 * \code
 * "content::attachment::<field name>::path::<server_name>_<unique number>"
 * \endcode
 *
 * By default a file is expected to be unique (multiple is set to false).
 *
 * \param[in] multiple  Whether the field represents a multi-file attachment.
 *
 * \sa get_multiple()
 * \sa get_name()
 */
void attachment_file::set_multiple(bool multiple)
{
    f_multiple = multiple;
}


/** \brief Set the path where the attachment is being added.
 *
 * This is the path to the parent page to which this attachment is
 * being added. A path is mandatory so you will have to call this
 * function, although the empty path is allowed (it represents the
 * home page so be careful!)
 *
 * \note
 * The class marks whether you set the path or not. If not, trying
 * to use it (get_cpath() function called) generates an exception
 * because it is definitively a mistake.
 *
 * \param[in] cpath  The path to the page receiving the attachment.
 *
 * \sa get_cpath()
 */
void attachment_file::set_parent_cpath(QString const& cpath)
{
    f_parent_cpath = cpath;
    f_has_cpath = true;
}


/** \brief Set the name of the field for the attachment.
 *
 * When saving a file as an attachment, we want to save the reference
 * in the parent as such. This makes it a lot easier to find the
 * attachments attached to a page.
 *
 * Note that to retreive the full name to the field, make sure to
 * call the get_name() function, the get_field_name() will return
 * just and only the \<field name> part, not the whole name.
 *
 * \code
 * // name of the field in the database:
 * "content::attachment::<field name>::path"
 *
 * // or, if multiple is set to true:
 * "content::attachment::<field name>::path::<server_name>_<unique number>"
 * \endcode
 *
 * \param[in] field_name  The name of the field used to save this attachment.
 *
 * \sa get_field_name()
 * \sa get_name()
 */
void attachment_file::set_field_name(QString const& field_name)
{
    f_field_name = field_name;
}


/** \brief Define the path of the attachment page.
 *
 * This function saves the path to the attachment itself in the
 * attachment_file object.
 *
 * Only the create_attachment() function is expected to call this function,
 * although if you replicate your own similar function, then you will have
 * to call this function from your replica.
 *
 * The path is expected to the canonicalized and set only once the full
 * path in the content table is known.
 *
 * \param[in] cpath  The path to the attachment file.
 *
 * \sa get_attachment_cpath()
 */
void attachment_file::set_attachment_cpath(QString const& cpath)
{
    f_attachment_cpath = cpath;
}


/** \brief Set the owner of this attachment.
 *
 * This name represents the plugin owner of the attachment. It must be
 * a valid plugin name as it is saved as the owner of the attachment.
 * This allows the plugin to specially handle the attachment when the
 * client wants to retrieve it.
 *
 * This name is saved as the primary owner of the attachment page.
 *
 * \param[in] owner  The name of the plugin that owns that attachment.
 *
 * \sa get_attachment_owner()
 */
void attachment_file::set_attachment_owner(QString const& owner)
{
    f_attachment_owner = owner;
}


/** \brief Define the type of the attachment page.
 *
 * When adding an attachment to the database, a new page is created as
 * a child of the page where the attachment is added. This allows us
 * to easily do all sorts of things with attachments. This new page being
 * content it needs to have a type and this type represents that type
 * (i.e. "attachment", "page/public", etc.)
 *
 * In most cases the type is set to the parent by default.
 *
 * \param[in] type  The type of the page.
 *
 * \sa get_attachment_type()
 */
void attachment_file::set_attachment_type(QString const& type)
{
    f_attachment_type = type;
}


/** \brief Set the creation time of the attachment.
 *
 * The first time the user POSTs an attachment, it saves the start date of
 * the HTTP request as the creation date. The loader sets the date back in
 * the attachment.
 *
 * \param[in] time  The time when the attachment was added to the database.
 *
 * \sa get_creation_time()
 */
void attachment_file::set_creation_time(int64_t time)
{
    f_creation_time = time;
}


/** \brief Set the modification time of the attachment.
 *
 * Each time the user POSTs an attachment, it saves the start date of the
 * HTTP request as the modification date. The loader sets the date back in
 * the attachment.
 *
 * \param[in] time  The time when the attachment was last modified.
 *
 * \sa get_update_time()
 */
void attachment_file::set_update_time(int64_t time)
{
    f_update_time = time;
}


/** \brief Set the dependencies of this attachment.
 *
 * Attachments can be given dependencies, with versions, and specific
 * browsers. This is particularly useful for JS and CSS files as in
 * this way we can server exactly what is necessary.
 *
 * One dependency looks like a name, one or two versions with an operator
 * (usually \< to define a range), and a browser name. The versions are
 * written between parenthesis and the browser name between square brackets:
 *
 * \code
 * <attachment name> ...
 *    ... (<version>) ...
 *    ... (<op> <version>) ...
 *    ... (<version> <op> <version>) ...
 *    ... (<version>, <version>, ...) ...
 *    ... (<op> <version>, <op> <version>, ...) ...
 *       ... [<browser>]
 *       ... [<browser>, <browser>, ...]
 * \endcode
 *
 * When two versions are used, the operator must be \<. It defines a range
 * and any versions defined between the two versions are considered valid.
 * The supported operators are =, \<, \<=, \>, \>=, !=, and ,. The comma
 * can be used to define a set of versions.
 *
 * Each attachment name must be defined only once.
 *
 * Attachments that are given dependencies are also added to a special
 * list so they can be found instantly. This is important since when a page
 * says to insert a JavaScript file, all its dependencies have to be added
 * too and that can be done automatically using these dependencies.
 *
 * \param[in] dependencies  The dependencies of this attachment.
 */
void attachment_file::set_dependencies(dependency_list_t& dependencies)
{
    f_dependencies = dependencies;
}


/** \brief Define the number of revisions to keep.
 *
 * By default, user files are all kept around forever. However, we use
 * attachments for many things, including automatically generated files
 * which get new versions generated all the time (i.e. if you run
 * the snapbackend every 5 minutes, which is the default, you could
 * very well get a new version of your feeds, sitemap.xml, robots.txt,
 * etc. every 5 minutes.)
 *
 * The limit defined here is the number of revisions to keep, including
 * the new revision. So if you use 1, all revisions except the last
 * one being created with the create_attachment() function, get deleted.
 * In most cases, you want a limit of 2 or 3. This way you keep the
 * previous one which another process may be uploading to a client at
 * the same time (thus, deleting it under the feet of that other process
 * would not be really nice.)
 *
 * In a case where a system offers multiple files, you will have to be
 * careful. If it works as with the sitemap.xml, which can reference
 * the correct revision of each file, you will be in luck. The only
 * revision you will have to worry about is the sitemap.xml file
 * revision. All the others can be moved to the latest revision
 * immediately since the URL to the sub-sitemap.xml files can include
 * the revision as part of the URL as in:
 *
 * \code
 *      ...
 *      <url>http://your.site/sitemap3.xml?revision=3</url>
 *      ...
 * \endcode
 *
 * \warning
 * At this time, the current revision is immediately forced to the newer
 * attachment by the create_attachment() and you do not have a choice...
 * This is a known limitation until we have sufficient UI to allow the
 * user to really choose what to do when saving new content. So once we
 * have that implemented, we will be able to setup the revision properly:
 * leave the current setup as it is until all the new file(s) get saved,
 * then update it appropriately. That way processes loading the files
 * will be sent to previous revisions, which will not be touched. Once
 * all the new revisions are available, then it will be switch to those
 * and the download will work as expected from all of those new files.
 *
 * \param[in] limit  The number of revisions to keep. Use 0 for unlimited.
 *
 * \sa get_revision_limit()
 */
void attachment_file::set_revision_limit(int limit)
{
    if(limit < 0)
    {
        throw content_exception_invalid_parameter("the revision limit cannot be set to a negative value");
    }

    f_revision_limit = limit;
}


/** \brief Set the name of the field the attachment comes from.
 *
 * This function is used by the load_attachment() function to set the
 * name of the file attachment as if it had been sent by a POST.
 *
 * \param[in] name  The name of the field used to upload this attachment.
 */
void attachment_file::set_file_name(QString const& name)
{
    f_file.set_name(name);
}


/** \brief Set the name of the file.
 *
 * This function sets the name of the file as it was sent by the POST
 * sending the attachment.
 *
 * \param[in] filename  The name of the file as sent by the POST message.
 */
void attachment_file::set_file_filename(QString const& filename)
{
    f_file.set_filename(filename);
}


/** \brief Set the mime_type of the file.
 *
 * This function can be used to setup the MIME type of the file when
 * the data if the file is not going to be set in the attachment file.
 * (It is useful NOT to load the data if you are not going to use it
 * anyway!)
 *
 * The original MIME type is the one sent by the browser at
 * the time the attachment was POSTed.
 *
 * \param[in] mime_type  The original MIME time as sent by the client.
 */
void attachment_file::set_file_mime_type(QString const& mime_type)
{
    f_file.set_mime_type(mime_type);
}


/** \brief Set the original mime_type of the file.
 *
 * This function can be used to setup the original MIME type of the
 * file. The original MIME type is the one sent by the browser at
 * the time the attachment was POSTed.
 *
 * \param[in] mime_type  The original MIME time as sent by the client.
 */
void attachment_file::set_file_original_mime_type(QString const& mime_type)
{
    f_file.set_original_mime_type(mime_type);
}


/** \brief Set the creation time.
 *
 * This function sets the creating time of the file.
 *
 * \param[in] ctime  The creation time.
 */
void attachment_file::set_file_creation_time(time_t ctime)
{
    f_file.set_creation_time(ctime);
}


/** \brief Set the modification time.
 *
 * This function sets the creation time of the file.
 *
 * \param[in] mtime  The last modification time.
 */
void attachment_file::set_file_modification_time(time_t mtime)
{
    f_file.set_modification_time(mtime);
}


/** \brief Set the data of the file.
 *
 * This function sets the data of the file. This is the actual file
 * content.
 *
 * \param[in] data  The last modification time.
 */
void attachment_file::set_file_data(QByteArray const& data)
{
    f_file.set_data(data);
}


/** \brief Set the size of the file.
 *
 * This function sets the size of the file. This is particularly
 * useful if you do not want to load the data but still want to
 * get the size for display purposes.
 *
 * \param[in] size  Set the size of the file.
 */
void attachment_file::set_file_size(int size)
{
    f_file.set_size(size);
}


/** \brief Set the image width.
 *
 * This function is used to set the width of the image.
 *
 * \param[in] width  The image width.
 */
void attachment_file::set_file_image_width(int width)
{
    f_file.set_image_width(width);
}


/** \brief Set the image height.
 *
 * This function is used to set the height of the image.
 *
 * \param[in] height  The image height.
 */
void attachment_file::set_file_image_height(int height)
{
    f_file.set_image_height(height);
}


/** \brief Set the index of the field.
 *
 * This function is used to set the field index within the form.
 *
 * \param[in] index  The index of the field.
 */
void attachment_file::set_file_index(int index)
{
    f_file.set_index(index);
}


/** \brief Return whether the attachment is unique or not.
 *
 * This function returns the flag as set by the set_multiple().
 * If true it means that as many attachments as necessary can
 * be added under the same field name. Otherwise only one
 * attachment can be added.
 *
 * \return Whether multiple attachments can be saved.
 *
 * \sa set_multiple()
 * \sa get_name()
 */
bool attachment_file::get_multiple() const
{
    return f_multiple;
}


/** \brief Return the file structure.
 *
 * When receiving a file, in most cases it is via an upload so we
 * use that structure directly to avoid copying all that data all
 * the time.
 *
 * This function returns a reference so you can directly use a
 * reference instead of a copy.
 *
 * \note
 * The only way to setup the file is via the constructor.
 *
 * \return A reference to the file representing this attachment.
 *
 * \sa attachment_file()
 */
snap_child::post_file_t const& attachment_file::get_file() const
{
    return f_file;
}


/** \brief Path to the parent of the file.
 *
 * This path represents the parent receiving this attachment.
 *
 * \return The path to the parent of the attachment.
 *
 * \sa set_parent_cpath()
 */
QString const& attachment_file::get_parent_cpath() const
{
    if(!f_has_cpath)
    {
        throw content_exception_invalid_name("the cpath parameter of a attachment_file object was never set");
    }
    return f_parent_cpath;
}


/** \brief Retrieve the name of the field.
 *
 * This function retrieves the raw name of the field. For the complete
 * name, make sure to use the get_name() function instead.
 *
 * \exception content_exception_invalid_name
 * This function generates the invalid name exception if the owner was
 * not defined an the parameter is still empty at the time it is to be
 * used.
 *
 * \return The field name to use for this attachment file.
 *
 * \sa set_field_name()
 */
QString const& attachment_file::get_field_name() const
{
    if(f_field_name.isEmpty())
    {
        throw content_exception_invalid_name("the field name of a attachment_file object cannot be empty");
    }
    return f_field_name;
}


/** \brief Retrieve the path of the attachment page.
 *
 * This function returns the path that the create_attachment() function
 * creates to save the attachment. This can be used to later access the
 * attachment.
 *
 * The path is expected to the canonicalized.
 *
 * The function may return an empty string if the create_attachment()
 * function was not called or it failed. It is considered a bug to
 * set this path outside of the create_attachment() function.
 *
 * \return The path to the attachment file.
 *
 * \sa set_attachment_cpath()
 */
QString const & attachment_file::get_attachment_cpath() const
{
    return f_attachment_cpath;
}


/** \brief Retrieve the owner of the attachment page.
 *
 * This function returns the name of the plugin that becomes the attachment
 * owner in the content table. The owner has rights over the content to
 * display it, allow the client to download it, etc.
 *
 * \exception content_exception_invalid_name
 * This function generates the invalid name exception if the owner was
 * not defined an the parameter is still empty at the time it is to be
 * used.
 *
 * \return The name of the plugin that owns this attachment file.
 *
 * \sa set_attachment_owner()
 */
QString const& attachment_file::get_attachment_owner() const
{
    if(f_attachment_owner.isEmpty())
    {
        throw content_exception_invalid_name("the attachment owner of a attachment_file object cannot be empty");
    }
    return f_attachment_owner;
}


/** \brief Retrieve the type of the attachment page.
 *
 * This function returns the type to use for the page we are to create for
 * this attachment. This is one of the .../content-types/\<name> types.
 *
 * \exception content_exception_invalid_name
 * This function generates the invalid name exception if the type was
 * not defined and the parameter is still empty at the time it is to be
 * used.
 *
 * \return The name of the type to use for the attachment page.
 *
 * \sa set_attachment_type()
 */
QString const& attachment_file::get_attachment_type() const
{
    if(f_attachment_type.isEmpty())
    {
        throw content_exception_invalid_name("the attachment type of a attachment_file object cannot be empty");
    }
    return f_attachment_type;
}


/** \brief Set the creation time of the attachment.
 *
 * The first time the user POSTs an attachment, it saves the start date of
 * the HTTP request as the creation date. The loader sets the date back in
 * the attachment.
 *
 * \return The time when the attachment was added to the database.
 *
 * \sa set_creation_time()
 */
int64_t attachment_file::get_creation_time() const
{
    return f_creation_time;
}


/** \brief Get the modification time of the attachment.
 *
 * Each time the user POSTs an attachment, it saves the start date of the
 * HTTP request as the modification date. The loader sets the date back in
 * the attachment.
 *
 * \return The time when the attachment was last updated.
 *
 * \sa set_update_time()
 */
int64_t attachment_file::get_update_time() const
{
    return f_update_time;
}


/** \brief Retrieve the list of dependencies of an attachment.
 *
 * The list of dependencies on an attachment are set with the
 * set_dependencies() function. These are used to determine which files are
 * required in a completely automated way.
 *
 * \return The list of dependency of this attachment.
 */
dependency_list_t const & attachment_file::get_dependencies() const
{
    return f_dependencies;
}


/** \brief Get the number of revisions to keep.
 *
 * This function returns the number of revisions to be kept in the database
 * for that attachment. This is used by backends that automatically generate
 * new files all the time.
 *
 * By default, attachments are kept forever (the limit is zero.)
 *
 * For additional information about the revision limit, see the
 * set_revision_limit() function.
 *
 * \return The limit number of revisions.
 *
 * \sa set_revision_limit()
 */
int attachment_file::get_revision_limit() const
{
    return f_revision_limit;
}


/** \brief Generate the full field name.
 *
 * The name of the field in the parent page in the content is defined
 * as follow:
 *
 * \code
 * // name of the field in the database:
 * "content::attachment::<field name>::path"
 *
 * // or, if multiple is set to true:
 * "content::attachment::<field name>::path::<server name>_<unique number>"
 * \endcode
 *
 * To make sure that everyone always uses the same name each time, we
 * created this function and you'll automatically get the right name
 * every time.
 *
 * \warning
 * After the first call this function always returns exactly the same
 * name. This is because we cache the name so it can be called any
 * number of time and it will quickly return with the name.
 *
 * \return The full name of the field.
 *
 * \sa set_multiple()
 * \sa set_field_name()
 * \sa set_attachment_owner()
 */
QString const& attachment_file::get_name() const
{
    // this name appears in the PARENT of the attachment
    if(f_name.isEmpty())
    {
        if(f_multiple)
        {
            f_name = QString("%1::%2::%3::%4")
                    .arg(snap::content::get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT))
                    .arg(get_field_name())
                    .arg(snap::content::get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT_PATH_END))
                    .arg(f_snap->get_unique_number())
                    ;
        }
        else
        {
            f_name = QString("%1::%2::%3")
                    .arg(snap::content::get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT))
                    .arg(get_field_name())
                    .arg(snap::content::get_name(name_t::SNAP_NAME_CONTENT_ATTACHMENT_PATH_END))
                    ;
        }
    }
    return f_name;
}



SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
