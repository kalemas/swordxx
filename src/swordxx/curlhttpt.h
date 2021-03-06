/******************************************************************************
 *
 *  curlhttpt.h  -    code for CURL impl of HTTP Transport
 *
 * $Id$
 *
 * Copyright 2009-2013 CrossWire Bible Society (http://www.crosswire.org)
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

#ifndef CURLHTTPT_H
#define CURLHTTPT_H

#include "remotetrans.h"

#include "defs.h"


namespace swordxx {

class CURL;


class SWDLLEXPORT CURLHTTPTransport : public RemoteTransport {
    CURL *session;

public:
    CURLHTTPTransport(const char *host, StatusReporter *statusReporter = 0);
    ~CURLHTTPTransport();

    virtual std::vector<struct DirEntry> getDirList(const char *dirURL);
    virtual char getURL(const char *destPath, const char *sourceURL, std::string *destBuf = 0);
};


} /* namespace swordxx */

#endif
