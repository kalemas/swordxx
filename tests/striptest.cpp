/******************************************************************************
 *
 *  striptest.cpp -
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

#include <iostream>
#include <swordxx/swmgr.h>
#include <swordxx/swmodule.h>


using namespace swordxx;

using std::cout;
using std::endl;

int main(int argc, char **argv) {

    std::string modName = (argc > 1) ? argv[2] : "WLC";
    std::string keyText = (argc > 2) ? argv[3] : "Gen.1.9";
    std::string searchText = (argc > 3) ? argv[4] : "מתחת";
    SWMgr library;
    SWModule *book = library.getModule(modName.c_str());
    StringList filters = library.getGlobalOptions();
    for (StringList::iterator it = filters.begin(); it != filters.end(); ++it) {
        // blindly turn off all filters.  Some filters don't support "Off", but that's ok, we should just silently fail on those.
        library.setGlobalOption(it->c_str(), "Off");
    }
    book->setKey(keyText.c_str());
    std::string entryStripped = book->stripText();
    cout << "Module: " << book->getDescription() << "\t Key: " << book->getKeyText() << "\n";
    cout << "RawEntry:\n" << book->getRawEntry() << "\n";
    cout << "StripText:\n" << entryStripped << "\n";
    cout << "Search Target: " << searchText << "\n";
    cout << "Search Target StripText: " << book->stripText(searchText.c_str()) << "\n";
    cout << "Found: " << ((strstr(entryStripped.c_str(), book->stripText(searchText.c_str()).c_str())) ? "true":"false") << endl;

    return 0;
}
