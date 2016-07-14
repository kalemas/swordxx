/***************************************************************************
 *
 *  teixhtml.cpp -    TEI to XHTML filter
 *
 * $Id$
 *
 * Copyright 2012-2013 CrossWire Bible Society (http://www.crosswire.org)
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

#include "teixhtml.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include "../../utilfuns/url.h"
#include "../../utilfuns/utilxml.h"
#include "../swmodule.h"


namespace swordxx {


const char *TEIXHTML::getHeader() const {
        // <pos>, <gen>, <case>, <gram>, <number>, <mood>, <pron>, <def> <tr> <orth> <etym> <usg>
    const static char *header = "\n\
        .entryFree, .form, .etym, .def, .usg, .quote {display:block;}\n\
        .pron, .pos, .oVar, .ref, {display:inline}\n\
        [type=headword] {font-weight:bold; font-variant:small-caps; text-decoration:underline;}\n\
        [type=derivative] {font-weight:bold; font-variant:small-caps;}\n\
        [rend=italic] {font-style:italic;}\n\
        [rend=bold] {font-weight:bold;}\n\
        [rend=small-caps] {font-variant:small-caps}\n\
        .pos:before {content: \"Pos.: \"; font-weight:bold;}\n\
        .pron:before {content:\" \\\\ \";}\n\
        .pron:after {content:\" \\\\ \";}\n\
        .etym:before {content:\"Etym.:\"; display:block; font-weight:bold;}\n\
        .usg:before {content:\"Usg.:\"; display:block; font-weight:bold;}\n\
        .def:before {content:\"Def.:\" display:block; font-weight:bold;}\n\
        .quote {background-color:#cfcfdf; padding:0.3em; margin:0.5em; border-width:1px; border-style:solid;}\n\
        .cit:before {content:\"quote:\" ; display:block; margin-top:0.5em; font-size:small;}\n\
        .cit {align:center;}\n\
        .persName:before {content:\" (\"; font-size:small;}\n\
        .persName:after {content:\") \"; font-size:small;}\n\
        .persName {font-size:small;}\n\
        .number {font-style:bold;}\n\
        .def {font-style:bold;}\n\
        ";
    return header;
}



TEIXHTML::MyUserData::MyUserData(const SWModule *module, const SWKey *key) : BasicFilterUserData(module, key) {
    BiblicalText = false;
    if (module) {
        version = module->getName();
        BiblicalText = (!strcmp(module->getType(), "Biblical Texts"));
    }
}


TEIXHTML::TEIXHTML() {
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

bool TEIXHTML::handleToken(std::string &buf, const char *token, BasicFilterUserData *userData) {
  // manually process if it wasn't a simple substitution
    if (!substituteToken(buf, token)) {
        MyUserData *u = (MyUserData *)userData;
        XMLTag tag(token);

        if (!strcmp(tag.getName(), "p")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {    // non-empty start tag
                buf += "<!P><br />";
            }
            else if (tag.isEndTag()) {    // end tag
                buf += "<!/P><br />";
                //userData->supressAdjacentWhitespace = true;
            }
            else {                    // empty paragraph break marker
                buf += "<!P><br />";
                //userData->supressAdjacentWhitespace = true;
            }
        }

        // <hi>
        else if (!strcmp(tag.getName(), "hi")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                std::string rend = tag.getAttribute("rend");

                u->lastHi = rend;
                if (rend == "italic" || rend == "ital")
                    buf += "<i>";
                else if (rend == "bold")
                    buf += "<b>";
                else if (rend == "super" || rend == "sup")
                    buf += "<sup>";
                else if (rend == "sub")
                    buf += "<sub>";
                else if (rend == "overline")
                    buf += "<span style=\"text-decoration:overline\">";

            }
            else if (tag.isEndTag()) {
                std::string rend = u->lastHi;
                if (rend == "italic" || rend == "ital")
                    buf += "</i>";
                else if (rend == "bold")
                    buf += "</b>";
                else if (rend == "super" || rend == "sup")
                    buf += "</sup>";
                else if (rend == "sub")
                    buf += "</sub>";
                else if (rend == "overline")
                    buf += "</span>";
            }
        }

        // <entryFree>
        else if (!strcmp(tag.getName(), "entryFree")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                std::string n = tag.getAttribute("n");
                if (n != "") {
                    buf += "<span class=\"entryFree\">";
                    buf += n;
                    buf += "</span>";
                }
            }
        }

        // <sense>
        else if (!strcmp(tag.getName(), "sense")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                std::string n = tag.getAttribute("n");
                buf += "<br/><span class=\"sense";
                if (n != "") {
                    buf += "\" n=\"";
                    buf += n;


                }
                buf += "\">";
            }
            else if (tag.isEndTag()) {
                buf += "</span>";
            }
        }

        // <div>
        else if (!strcmp(tag.getName(), "div")) {

            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                buf += "<!P>";
            }
            else if (tag.isEndTag()) {
            }
        }

        // <lb.../>
        else if (!strcmp(tag.getName(), "lb")) {
            buf += "<br />";
        }

        // <pos>, <gen>, <case>, <gram>, <number>, <mood>, <pron>, <def> <tr> <orth> <etym> <usg>
        else if (!strcmp(tag.getName(), "pos") ||
                 !strcmp(tag.getName(), "gen") ||
                 !strcmp(tag.getName(), "case") ||
                 !strcmp(tag.getName(), "gram") ||
                 !strcmp(tag.getName(), "number") ||
                 !strcmp(tag.getName(), "pron") ||
                 !strcmp(tag.getName(), "def") ||
                 !strcmp(tag.getName(), "tr") ||
                 !strcmp(tag.getName(), "orth") ||
                 !strcmp(tag.getName(), "etym") ||
                 !strcmp(tag.getName(), "usg") ||
                 !strcmp(tag.getName(), "quote")||
                 !strcmp(tag.getName(), "cit")||
                 !strcmp(tag.getName(), "persName")||
                 !strcmp(tag.getName(), "oVar"))
                 {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                buf += "<span class=\"";
                buf += tag.getName();
                if (!tag.getAttribute("type").empty()) {
                    buf += "\" type =\"";
                    buf += tag.getAttribute("type");
                }
                if (!tag.getAttribute("rend").empty()) {
                    buf += "\" rend =\"";
                    buf += tag.getAttribute("rend");
                }
                buf += "\">";
            }
            else if (tag.isEndTag()) {
                buf += "</span>";
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
                        std::strncpy(&work[0u], target.c_str(), size);
                    }

                    if(was_osisref)
                    {
                        buf += formatted("<a href=\"passagestudy.jsp?action=showRef&type=scripRef&value=%s&module=%s\">",
                            (ref.c_str()) ? URL::encode(ref.c_str()).c_str() : "",
                            (work.size()) ? URL::encode(work.c_str()).c_str() : "");
                    }
                    else
                    {
                        // Dictionary link, or something
                        buf += formatted("<a href=\"sword://%s/%s\">",
                            (work.size()) ? URL::encode(work.c_str()).c_str() : u->version.c_str(),
                            (ref.c_str()) ? URL::encode(ref.c_str()).c_str() : ""
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
                buf += "</a>";

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

                buf += formatted("<a href=\"passagestudy.jsp?action=showNote&type=n&value=%s&module=%s&passage=%s\"><small><sup class=\"n\">*n%s</sup></small></a>",
                    URL::encode(footnoteNumber.c_str()).c_str(),
                    URL::encode(u->version.c_str()).c_str(),
                    URL::encode(u->key->getText()).c_str(),
                    (renderNoteNumbers ? URL::encode(noteName.c_str()).c_str() : ""));

                u->suspendTextPassThru = false;
            }
        }
        // <graphic> image tag
        else if (!strcmp(tag.getName(), "graphic")) {
            auto const url(tag.getAttribute("url"));
            if (!url.empty()) {        // assert we have a url attribute
                std::string filepath;
                if (userData->module) {
                    filepath = userData->module->getConfigEntry("AbsoluteDataPath");
                    if ((filepath.size()) && (filepath[filepath.size()-1] != '/') && (url[0] != '/'))
                        filepath += '/';
                }
                filepath += url;
                buf += formatted("<a href=\"passagestudy.jsp?action=showImage&value=%s&module=%s\"><img src=\"file:%s\" border=\"0\" /></a>",
                            URL::encode(filepath.c_str()).c_str(),
                            URL::encode(u->version.c_str()).c_str(),
                            filepath.c_str());
                u->suspendTextPassThru = false;
            }
        }
        // <table> <row> <cell>
        else if (!strcmp(tag.getName(), "table")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                buf += "<table><tbody>\n";
            }
            else if (tag.isEndTag()) {
                buf += "</tbody></table>\n";
                u->supressAdjacentWhitespace = true;
            }
        }
        else if (!strcmp(tag.getName(), "row")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                buf += "\t<tr>";
            }
            else if (tag.isEndTag()) {
                buf += "</tr>\n";
            }
        }
        else if (!strcmp(tag.getName(), "cell")) {
            if ((!tag.isEndTag()) && (!tag.isEmpty())) {
                buf += "<td>";
            }
            else if (tag.isEndTag()) {
                buf += "</td>";
            }
        }
        else {
            return false;  // we still didn't handle token
        }

    }
    return true;
}


} /* namespace swordxx */
