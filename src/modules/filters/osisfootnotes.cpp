/******************************************************************************
 *
 * osisfootnotes -	SWFilter descendant to hide or show footnotes
 *			in an OSIS module.
 */


#include <stdlib.h>
#include <osisfootnotes.h>
#include <swmodule.h>
#include <swbuf.h>
#include <versekey.h>
#include <utilxml.h>
#ifndef __GNUC__
#else
#include <unixstr.h>
#endif

SWORD_NAMESPACE_START

const char oName[] = "Footnotes";
const char oTip[] = "Toggles Footnotes On and Off if they exist";

const SWBuf choices[3] = {"On", "Off", ""};
const StringList oValues(&choices[0], &choices[2]);

OSISFootnotes::OSISFootnotes() : SWOptionFilter(oName, oTip, &oValues) {
	setOptionValue("Off");
}


OSISFootnotes::~OSISFootnotes() {
}


char OSISFootnotes::processText(SWBuf &text, const SWKey *key, const SWModule *module) {
	SWBuf token;
	bool intoken    = false;
	bool hide       = false;
	SWBuf tagText;
	XMLTag startTag;
	SWBuf refs = "";
	int footnoteNum = 1;
	char buf[254];
	VerseKey parser = key->getText();

	SWBuf orig = text;
	const char *from = orig.c_str();
	
	XMLTag tag = "";

	for (text = ""; *from; from++) {

		// remove all newlines temporarily to fix kjv2003 module
		if ((*from == 10) || (*from == 13)) {
			if ((text.length()>1) && (text[text.length()-2] != ' ') && (*(from+1) != ' '))
				text.append(' ');
			continue;
		}

		
		if (*from == '<') {
			intoken = true;
			token = "";
			continue;
		}
		
		
		
		if (*from == '>') {	// process tokens
			intoken = false;			
// 			if (!strcmp(tag.getName(), "note")) {
			if (!strncmp(token, "note", 4) || !strncmp(token.c_str(), "/note", 5)) {
				tag = token;
				
				if (!tag.isEndTag()) {
					if (!strcmp("strongsMarkup", tag.getAttribute("type"))) {  // handle bug in KJV2003 module where some note open tags were <note ... />
						tag.setEmpty(false);
					}
					if (!tag.isEmpty()) {
//					if ((!tag.isEmpty()) || (SWBuf("strongsMarkup") == tag.getAttribute("type"))) {
						refs = "";
						startTag = tag;
						hide = true;
						tagText = "";
						continue;
					}
				}
				if (hide && tag.isEndTag()) {
					if (module->isProcessEntryAttributes()) {
						sprintf(buf, "%i", footnoteNum++);
						StringList attributes = startTag.getAttributeNames();
						for (StringList::const_iterator it = attributes.begin(); it != attributes.end(); it++) {
							module->getEntryAttributes()["Footnote"][buf][it->c_str()] = startTag.getAttribute(it->c_str());
						}
						module->getEntryAttributes()["Footnote"][buf]["body"] = tagText;
						startTag.setAttribute("swordFootnote", buf);
						if ((startTag.getAttribute("type")) && (!strcmp(startTag.getAttribute("type"), "crossReference"))) {
							if (!refs.length())
								refs = parser.ParseVerseList(tagText.c_str(), parser, true).getRangeText();
							module->getEntryAttributes()["Footnote"][buf]["refList"] = refs.c_str();
						}
					}
					hide = false;
					if (option || (startTag.getAttribute("type") && !strcmp(startTag.getAttribute("type"), "crossReference"))) {	// we want the tag in the text; crossReferences are handled by another filter
						text.append(startTag);
						text.append(tagText);
					}
					else	continue;
				}
			}

			// if not a heading token, keep token in text
			//if ((!strcmp(tag.getName(), "reference")) && (!tag.isEndTag())) {
			//	SWBuf osisRef = tag.getAttribute("osisRef");
			if (!strncmp(token, "reference", 9)) {
				if (refs.length()) {
					refs.append("; ");
				}
				
				const char* attr = strstr(token.c_str() + 9, "osisRef=\"");
				const char* end  = attr ? strchr(attr+9, '"') : 0;

				if (attr && end) {
					refs.append(attr+9, end-(attr+9));
				}
			}
			if (!hide) {
				text.append('<');
				text.append(token);
				text.append('>');
			}
			else {
				tagText.append('<');
				tagText.append(token);
				tagText.append('>');
			}
			continue;
		}
		if (intoken) { //copy token
			token.append(*from);
		}
		else if (!hide) { //copy text which is not inside a token
			text.append(*from);
		}
		else tagText.append(*from);
	}
	return 0;
}

SWORD_NAMESPACE_END

