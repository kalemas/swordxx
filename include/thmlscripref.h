/******************************************************************************
 *
 *  thmlscripref.h -	Implementation of ThMLScripRef
 *
 * $Id$
 *
 * Copyright 2001-2013 CrossWire Bible Society (http://www.crosswire.org)
 *	CrossWire Bible Society
 *	P. O. Box 2528
 *	Tempe, AZ  85280-2528
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

#ifndef THMLSCRIPREF_H
#define THMLSCRIPREF_H

#include <swoptfilter.h>

namespace swordxx {

/** This Filter shows/hides scripture references in a ThML text
 */
class SWDLLEXPORT ThMLScripref : public SWOptionFilter {
public:
	ThMLScripref();
	virtual ~ThMLScripref();
	virtual char processText(SWBuf &text, const SWKey *key = 0, const SWModule *module = 0);
};

} /* namespace swordxx */
#endif
