/***************************************************************************
                     osisplain.cpp  -  OSIS to Plaintext filter
                             -------------------
    begin                : 2003-02-15
    copyright            : 2003 by CrossWire Bible Society
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <osisplain.h>

SWORD_NAMESPACE_START

OSISPlain::OSISPlain()
{
  setTokenStart("<");
  setTokenEnd(">");
  
  setEscapeStart("&");
  setEscapeEnd(";");
  
  setEscapeStringCaseSensitive(true);
  
  addEscapeStringSubstitute("amp", "&");
  addEscapeStringSubstitute("apos", "'");
  addEscapeStringSubstitute("lt", "<");
  addEscapeStringSubstitute("gt", ">");
  addEscapeStringSubstitute("quot", "\"");
  
  setTokenCaseSensitive(true);  
}

bool OSISPlain::handleToken(char **buf, const char *token, DualStringMap &userData) {
  // manually process if it wasn't a simple substitution
  if (!substituteToken(buf, token)) {
    //w
    if (!strncmp(token, "w", 1)) {
      userData["w"] == token;
    }
    else if (!strncmp(token, "/w", 2)) {
      pos1 = userData["w"].find("xlit=\"", 0);
      if (pos1 != string::npos) {
	pos1 = userData["w"].find(":", pos1) + 1;
	pos2 = userData["w"].find("\"", pos1) - 1;
	tagData = userData["w"].substr(pos1, pos2-pos1);
	pushString(buf, " <%s>", tagData.c_str());
      }
      pos1 = userData["w"].find("gloss=\"", 0);
      if (pos1 != string::npos) {
	pos1 = userData["w"].find(":", pos1) + 1;
	pos2 = userData["w"].find("\"", pos1) - 1;
	tagData = userData["w"].substr(pos1, pos2-pos1);
	pushString(buf, " <%s>", tagData.c_str());
      }
      pos1 = userData["w"].find("lemma=\"", 0);
      if (pos1 != string::npos) {
	pos1 = userData["w"].find(":", pos1) + 1;
	pos2 = userData["w"].find("\"", pos1) - 1;
	tagData = userData["w"].substr(pos1, pos2-pos1);
	pushString(buf, " <%s>", tagData.c_str());
      }
      pos1 = userData["w"].find("morph=\"", 0);
      if (pos1 != string::npos) {
	pos1 = userData["w"].find(":", pos1) + 1;
	pos2 = userData["w"].find("\"", pos1) - 1;
	tagData = userData["w"].substr(pos1, pos2-pos1);
	pushString(buf, " <%s>", tagData.c_str());
      }
      pos1 = userData["w"].find("POS=\"", 0);
      if (pos1 != string::npos) {
	pos1 = userData["w"].find(":", pos1) + 1;
	pos2 = userData["w"].find("\"", pos1) - 1;
	tagData = userData["w"].substr(pos1, pos2-pos1);
	pushString(buf, " <%s>", tagData.c_str());
      }      
    }
    
    //p
    else if (!strncmp(token, "p", 1)) {
      pushString(buf, "\n\n");
    }

    //line
    else if (!strncmp(token, "line", 4)) {
      pushString(buf, "\n");
    }

    //note
    else if (!strncmp(token, "note", 4)) {
      pushString(buf, " (");
    }
    else if (!strncmp(token, "/note", 5)) {
      pushString(buf, ")");
    }
    
    //title
    else if (!strncmp(token, "/title", 6)) {
      pushString(buf, "\n");
    }

    else {
      return false;  // we still didn't handle token
		}
  }
  return true;
}


SWORD_NAMESPACE_END