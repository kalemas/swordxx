/******************************************************************************
 *
 *  thmlvariants.h -    Implementation of ThMLVariants
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

#ifndef THMLVARIANTS_H
#define THMLVARIANTS_H

#include "../swoptfilter.h"


namespace swordxx {

/** This Filter shows/hides textual variants
 */
class SWDLLEXPORT ThMLVariants : public SWOptionFilter {
public:
    ThMLVariants();
    virtual ~ThMLVariants();
    virtual char processText(std::string &text, const SWKey *key = 0, const SWModule *module = 0);
};

} /* namespace swordxx */
#endif
