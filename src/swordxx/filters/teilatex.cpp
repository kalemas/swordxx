/***************************************************************************
 *
 *  teilatex.cpp -    TEI to LATEX filter
 *
 * $Id$
 *
 * Copyright 2012-2014 CrossWire Bible Society (http://www.crosswire.org)
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

#include "teilatex.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include "../swmodule.h"
#include "../url.h"
#include "../utilxml.h"


namespace swordxx {


TEILaTeX::MyUserData::MyUserData(const SWModule *module, const SWKey *key) : BasicFilterUserData(module, key) {
    BiblicalText = false;
    if (module) {
        version = module->getName();
        BiblicalText = (!strcmp(module->getType(), "Biblical Texts"));
    }
}


TEILaTeX::TEILaTeX() {
    setTokenStart("<");
    setTokenEnd(">");

    setEscapeStart("&");
    setEscapeEnd(";");

    setEscapeStringCaseSensitive(true);

    addAllowedEscapeString("quot");
    addAllowedEscapeString("apos");
    addAllowedEscapeString("amp");
    addAllowedEscapeString("lt");
    addAllowedEscapeString("gt");

    setTokenCaseSensitive(true);

    renderNoteNumbers = false;
}

bool TEILaTeX::handleToken(std::string &buf, const char *token, BasicFilterUserData *userData) {
  // manually process if it wasn't a simple substitution
    if (!substituteToken(buf, token)) {
        MyUserData *u = (MyUserData *)userData;
        XMLTag tag(token);

        if (!strcmp(tag.getName(), "p")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {    // non-empty start tag
                buf += "";
            }
            else if (tag.isEndTag()) {    // end tag
                buf += "//\n";
                //userData->supressAdjacentWhitespace = true;
            }
            else {                    // empty paragraph break marker
                buf += "//\n";
                //userData->supressAdjacentWhitespace = true;
            }
        }

        // <hi>
        else if (!strcmp(tag.getName(), "hi")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                std::string rend = tag.getAttribute("rend");

                u->lastHi = rend;
                if (rend == "italic" || rend == "ital")
                    buf += "\\it{";
                else if (rend == "bold")
                    buf += "\\bd{";
                else if (rend == "super" || rend == "sup")
                    buf += "^{";
                else if (rend == "sub")
                    buf += "_{";
                else if (rend == "overline")
                    buf += "\\overline{";

            }
            else if (tag.isEndTag()) {
                buf += "}";
            }
        }

        // <entryFree>
        else if (!strcmp(tag.getName(), "entryFree")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                std::string n = tag.getAttribute("n");
                if (n != "") {
                    buf += "\\teiEntryFree{";
                    buf += n;
                    buf += "}";
                }
            }
        }

        // <sense>
        else if (!strcmp(tag.getName(), "sense")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                std::string n = tag.getAttribute("n");
                if (n != "") {
                    buf += "\n\\teiSense{";
                    buf += n;
                    buf += "}";
                }
            }
        }

        // <div>
        else if (!strcmp(tag.getName(), "div")) {

            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                buf += "";
            }
            else if (tag.isEndTag()) {
            }
        }

        // <lb.../>
        else if (!strcmp(tag.getName(), "lb")) {
            buf += "//\n";
        }

        // <pos>, <gen>, <case>, <gram>, <number>, <mood>, <pron>, <def>
        else if (!strcmp(tag.getName(), "pos") ||
                 !strcmp(tag.getName(), "gen") ||
                 !strcmp(tag.getName(), "case") ||
                 !strcmp(tag.getName(), "gram") ||
                 !strcmp(tag.getName(), "number") ||
                 !strcmp(tag.getName(), "pron") ||
                 !strcmp(tag.getName(), "tr") ||
                 !strcmp(tag.getName(), "orth") ||
                 !strcmp(tag.getName(), "etym") ||
                 !strcmp(tag.getName(), "usg") ||


                 !strcmp(tag.getName(), "def")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                buf += "\\tei";
                buf += tag.getName();
                buf += "{";
            }
            else if (tag.isEndTag()) {
                buf += "}";
            }
        }

        else if (!strcmp(tag.getName(), "ref")) {
            if (!tag.isEndTag()) {
                u->suspendTextPassThru = true;
                std::string target;
                std::string work;
                std::string ref;

                int was_osisref = false;
                if(!tag.getAttribute("osisRef").empty())
                {
                    target += tag.getAttribute("osisRef");
                    was_osisref=true;
                }
                else if(!tag.getAttribute("target").empty())
                    target += tag.getAttribute("target");

                if(target.size())
                {
                    const char* the_ref = std::strchr(target.c_str(), ':');

                    if(!the_ref) {
                        // No work
                        ref = target;
                    }
                    else {
                        // Compensate for starting :
                        ref = the_ref + 1;

                        int size = target.size() - ref.size() - 1;
                        work.resize(size, '\0');
                        strncpy(&work[0u], target.c_str(), size);
                    }

                    if(was_osisref)
                    {
                        buf += formatted("\\swordref{%s}{%s}{",
                            ref.c_str(),
                            work.c_str());
                    }
                    else
                    {
                        // Dictionary link, or something
                        buf += formatted("\\sworddictref{%s}{%s}{",
                            (!work.empty()) ? work.c_str() : u->version.c_str(),
                            ref.c_str()
                            );
                    }
                }
                else
                {
                    //std::cout << "TARGET WASN'T\n";
                }

            }
            else {
                buf += u->lastTextNode.c_str();
                buf += "}";

                u->suspendTextPassThru = false;
            }
        }

           // <note> tag
        else if (!strcmp(tag.getName(), "note")) {
            if (!tag.isEndTag()) {
                if (!tag.isEmpty()) {
                    u->suspendTextPassThru = true;
                }
            }
            if (tag.isEndTag()) {
                std::string footnoteNumber = tag.getAttribute("swordFootnote");
                std::string noteName = tag.getAttribute("n");
                std::string footnoteBody = "";
                if (u->module){
                    footnoteBody += u->module->getEntryAttributes()["Footnote"][footnoteNumber]["body"];
                }

                buf += formatted("\\swordfootnote{%s}{%s}{%s}{%s}{",
                    footnoteNumber.c_str(),
                    u->version.c_str(),
                    u->key->getText(),
                    renderNoteNumbers ? noteName.c_str() : "");
                    if (u->module) {
                        buf += u->module->renderText(footnoteBody.c_str()).c_str();
                    }
                u->suspendTextPassThru = false;
            }
        }

        // <graphic> image tag
        else if (!strcmp(tag.getName(), "graphic")) {
            auto url(tag.getAttribute("url"));
            if (!url.empty()) {        // assert we have a url attribute
                std::string filepath;
                if (userData->module) {
                    filepath = userData->module->getConfigEntry("AbsoluteDataPath");
                    if ((filepath.size()) && (filepath[filepath.size()-1] != '/') && (url[0] != '/'))
                        filepath += '/';
                }
                filepath += url;

                buf += formatted("\\figure{\\includegraphics{%s}}",
                            filepath.c_str());
                u->suspendTextPassThru = false;

            }
        }

        // <table> <row> <cell>
        else if (!strcmp(tag.getName(), "table")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                buf += "\n\\begin{tabular}";
            }
            else if (tag.isEndTag()) {
                buf += "\n\\end{tabular}";
                ++u->consecutiveNewlines;
                u->supressAdjacentWhitespace = true;
            }

        }
        else if (!strcmp(tag.getName(), "row")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                buf += "\n";
                u->firstCell = true;
            }
            else if (tag.isEndTag()) {
                buf += "//";
                u->firstCell = false;
            }

        }
        else if (!strcmp(tag.getName(), "cell")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                if (u->firstCell == false) {
                    buf += " & ";
                }
                else {
                    u->firstCell = false;
                }
            }
            else if (tag.isEndTag()) {
                buf += "";
            }
        }


        else {
            return false;  // we still didn't handle token
        }

    }
    return true;
}


} /* namespace swordxx */

