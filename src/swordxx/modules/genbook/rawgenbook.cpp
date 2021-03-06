/******************************************************************************
 *
 *  rawgenbook.cpp -    code for class 'RawGenBook'- a module that reads raw
 *            text files: ot and nt using indexs ??.bks ??.cps ??.vss
 *
 * $Id$
 *
 * Copyright 2002-2013 CrossWire Bible Society (http://www.crosswire.org)
 *    CrossWire Bible Society
 *    P. O. Box 2528
 *    Tempe, AZ  85280-2528
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

#include "rawgenbook.h"

#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <memory>
#include "../../filemgr.h"
#include "../../keys/treekeyidx.h"
#include "../../keys/versetreekey.h"
#include "../../utilstr.h"
#include "../../sysdata.h"
#include "../common/rawstr.h"


namespace swordxx {

namespace {

SWKey * staticCreateKey(char const * const path, bool const verseKey) {
    auto tKey(std::make_unique<TreeKeyIdx>(path));
    if (verseKey) {
        SWKey * const vtKey = new VerseTreeKey(tKey.get());
        tKey.reset();
        return vtKey;
    }
    return tKey.release();
}

SWKey * constructorCreateKey(char const * const ipath, bool const verseKey) {
    char * path = nullptr;
    stdstr(&path, ipath);
    if ((path[strlen(path) - 1] == '/') || (path[strlen(path) - 1] == '\\'))
        path[strlen(path) - 1] = 0;
    return staticCreateKey(path, verseKey);
}

} // anonymous namespace


/******************************************************************************
 * RawGenBook Constructor - Initializes data for instance of RawGenBook
 *
 * ENT:    iname - Internal name for module
 *    idesc - Name to display to user for module
 */

RawGenBook::RawGenBook(const char *ipath, const char *iname, const char *idesc, SWTextEncoding enc, SWTextDirection dir, SWTextMarkup mark, const char* ilang, const char *keyType)
        : SWGenBook(constructorCreateKey(ipath, !strcmp("VerseKey", keyType)), iname, idesc, enc, dir, mark, ilang) {

    char *buf = new char [ strlen (ipath) + 20 ];

    path = 0;
    stdstr(&path, ipath);
    verseKey = !strcmp("VerseKey", keyType);

    if (verseKey) setType("Biblical Texts");

    if ((path[strlen(path)-1] == '/') || (path[strlen(path)-1] == '\\'))
        path[strlen(path)-1] = 0;

    sprintf(buf, "%s.bdt", path);
    bdtfd = FileMgr::getSystemFileMgr()->open(buf, FileMgr::RDWR, true);

    delete [] buf;

}


/******************************************************************************
 * RawGenBook Destructor - Cleans up instance of RawGenBook
 */

RawGenBook::~RawGenBook() {

    FileMgr::getSystemFileMgr()->close(bdtfd);

    if (path)
        delete [] path;

}


bool RawGenBook::isWritable() const {
    return ((bdtfd->getFd() > 0) && ((bdtfd->mode & FileMgr::RDWR) == FileMgr::RDWR));
}


/******************************************************************************
 * RawGenBook::getRawEntry    - Returns the correct verse when char * cast
 *                    is requested
 *
 * RET: string buffer with verse
 */

std::string &RawGenBook::getRawEntryBuf() const {

    uint32_t offset = 0;
    uint32_t size = 0;

    const TreeKey &key = getTreeKey();

    int dsize;
    key.getUserData(&dsize);
    entryBuf = "";
    if (dsize > 7) {
        memcpy(&offset, key.getUserData(), 4);
        offset = swordtoarch32(offset);

        memcpy(&size, key.getUserData() + 4, 4);
        size = swordtoarch32(size);

        entrySize = size;        // support getEntrySize call

        entryBuf.resize(size, '\0');
        bdtfd->seek(offset, SEEK_SET);
        bdtfd->read(&entryBuf[0u], size);

        rawFilter(entryBuf, 0);    // hack, decipher
        rawFilter(entryBuf, &key);

//           if (!isUnicode())
            SWModule::prepText(entryBuf);
    }

    return entryBuf;
}


void RawGenBook::setEntry(const char *inbuf, long len) {

    uint32_t offset = archtosword32(bdtfd->seek(0, SEEK_END));
    uint32_t size = 0;
    TreeKeyIdx *key = ((TreeKeyIdx *)&(getTreeKey()));

    char userData[8];

    if (len < 0)
        len = strlen(inbuf);

    bdtfd->write(inbuf, len);

    size = archtosword32(len);
    memcpy(userData, &offset, 4);
    memcpy(userData+4, &size, 4);
    key->setUserData(userData, 8);
    key->save();
}


void RawGenBook::linkEntry(const SWKey *inkey) {
    TreeKeyIdx *key = ((TreeKeyIdx *)&(getTreeKey()));
    // see if we have a VerseKey * or decendant
    /// \bug Remove const_cast:
    TreeKeyIdx * srckey =
            const_cast<TreeKeyIdx *>(dynamic_cast<TreeKeyIdx const *>(inkey));
    // if we don't have a VerseKey * decendant, create our own
    if (!srckey) {
        srckey = (TreeKeyIdx *)createKey();
        (*srckey) = *inkey;
    }

    key->setUserData(srckey->getUserData(), 8);
    key->save();

    if (inkey != srckey) // free our key if we created a VerseKey
        delete srckey;
}


/******************************************************************************
 * RawGenBook::deleteEntry    - deletes this entry
 *
 * RET: *this
 */

void RawGenBook::deleteEntry() {
    TreeKeyIdx *key = ((TreeKeyIdx *)&(getTreeKey()));
    key->remove();
}


char RawGenBook::createModule(const char *ipath) {
    char *path = 0;
    char *buf = new char [ strlen (ipath) + 20 ];
    FileDesc *fd;
    signed char retval;

    stdstr(&path, ipath);

    if ((path[strlen(path)-1] == '/') || (path[strlen(path)-1] == '\\'))
        path[strlen(path)-1] = 0;

    sprintf(buf, "%s.bdt", path);
    FileMgr::removeFile(buf);
    fd = FileMgr::getSystemFileMgr()->open(buf, FileMgr::CREAT|FileMgr::WRONLY, FileMgr::IREAD|FileMgr::IWRITE);
    fd->getFd();
    FileMgr::getSystemFileMgr()->close(fd);

    retval = TreeKeyIdx::create(path);
    delete [] path;
    return retval;
}


SWKey * RawGenBook::createKey() const
{ return staticCreateKey(path, verseKey); }

bool RawGenBook::hasEntry(const SWKey *k) const {
    /// \bug remove const_cast:
    TreeKey &key = getTreeKey(const_cast<SWKey *>(k));

    int dsize;
    key.getUserData(&dsize);
    return (dsize > 7) && key.popError() == '\x00';
}

} /* namespace swordxx */
