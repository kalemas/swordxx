/******************************************************************************
 *
 *  remotetrans.h -    code for Remote Transport
 *
 * $Id$
 *
 * Copyright 2004-2013 CrossWire Bible Society (http://www.crosswire.org)
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

#ifndef REMOTETRANS_H
#define REMOTETRANS_H

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>
#include "defs.h"


namespace swordxx {

/** Class for reporting status
*/
class SWDLLEXPORT StatusReporter {

public: /* Methods: */

    virtual ~StatusReporter() noexcept {};

    /** Messages before stages of a batch download */
    virtual void preStatus(std::size_t totalBytes,
                           std::size_t completedBytes,
                           const char * message) noexcept
    {
        (void) totalBytes;
        (void) completedBytes;
        (void) message;
    };

    /** Frequently called throughout a download, to report status */
    virtual void update(std::size_t totalBytes, std::size_t completedBytes)
            noexcept
    {
        (void) totalBytes;
        (void) completedBytes;
    };

};


/**
* A base class to be used for reimplementation of network services.
*/
class SWDLLEXPORT RemoteTransport {

protected:
    StatusReporter *statusReporter;
    bool passive;
    std::atomic<bool> term;
    std::string host;
    std::string u;
    std::string p;

public:
    RemoteTransport(const char *host, StatusReporter *statusReporter = 0);
    virtual ~RemoteTransport();

    /***********
     * override this method in your real impl
     *
     * if destBuf then write to buffer instead of file
     */
    virtual char getURL(const char *destPath, const char *sourceURL, std::string *destBuf = 0);


    int copyDirectory(const char *urlPrefix, const char *dir, const char *dest, const char *suffix);

    virtual std::vector<struct DirEntry> getDirList(const char *dirURL);
    void setPassive(bool passive) { this->passive = passive; }
    void setUser(const char *user) { u = user; }
    void setPasswd(const char *passwd) { p = passwd; }
    void terminate() { term.store(true, std::memory_order_release); }
};


} /* namespace swordxx */

#endif
