/***************************************************************************
 *
 *  latin1utf8.h -    Implementation of Latin1UTF8
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

#ifndef LATIN1UTF8_H
#define LATIN1UTF8_H

#include "../swfilter.h"


namespace swordxx {

/** This filter converts Latin-1 encoded text to UTF-8
 */
class SWDLLEXPORT Latin1UTF8 : public SWFilter {
public:
    Latin1UTF8();
    virtual char processText(std::string &text, const SWKey *key = 0, const SWModule *module = 0);
};

} /* namespace swordxx */
#endif
