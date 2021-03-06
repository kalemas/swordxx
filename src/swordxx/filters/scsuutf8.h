/******************************************************************************
 *
 *  scsuutf8.h - SWFilter descendant to convert a SCSU character to UTF-8
 *
 * $Id$
 *
 * Copyright 2001-2013 CrossWire Bible Society (http://www.crosswire.org)
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

#ifndef SCSUUTF8_H
#define SCSUUTF8_H

#include "../swfilter.h"

#if SWORDXX_HAS_ICU
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include <unicode/uchar.h>
#endif


namespace swordxx {

/** This filter converts SCSU compressed (encoded) text to UTF-8
 */
class SWDLLEXPORT SCSUUTF8 : public SWFilter {
private:
#if SWORDXX_HAS_ICU
    UConverter* scsuConv;
    UConverter* utf8Conv;
    UErrorCode err = U_ZERO_ERROR;
#else
    // without ICU, we'll attempt to use Roman Czyborra's SCSU decoder code
    unsigned char active;
    bool mode;
    unsigned long c, d;

    static unsigned short start[8];
    static unsigned short slide[8];
    static unsigned short win[256];

    int UTF8Output(unsigned long, std::string* utf8Buf);
#endif

public:
    SCSUUTF8();
    ~SCSUUTF8();
    virtual char processText(std::string &text, const SWKey *key = 0, const SWModule *module = 0);
};

} /* namespace swordxx */
#endif
