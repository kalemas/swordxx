/******************************************************************************
 *  swcom.h   - code for base class 'SWCom'.  SWCom is the basis for all
 *		 types of commentary modules
 *
 * $Id: swcom.h,v 1.2 2001/02/08 09:20:48 chrislit Exp $
 *
 * Copyright 1998 CrossWire Bible Society (http://www.crosswire.org)
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

#ifndef SWCOM_H
#define SWCOM_H

#include <swmodule.h>
#include <versekey.h>

#include <defs.h>

class SWDLLEXPORT SWCom : public SWModule {
public:
	SWCom(const char *imodname = 0, const char *imoddesc = 0, SWDisplay *idisp = 0);
	virtual ~SWCom();
	virtual SWKey *CreateKey() { return new VerseKey(); }
};


#endif
