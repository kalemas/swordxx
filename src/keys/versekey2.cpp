/******************************************************************************
 *  VerseKey.cpp - code for class 'VerseKey'- the new version of a standard Biblical verse key
 */

#include <swmacs.h>
#include <utilfuns.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#ifndef __GNUC__
#include <io.h>
#else
#include <unistd.h>
#endif

#include <utilstr.h>
#include <swkey.h>
#include <swlog.h>
#include <versekey2.h>
#include <localemgr.h>
#include <roman.h>

SWORD_NAMESPACE_START

static const char *classes[] = {"VerseKey", "SWKey", "SWObject", 0};
SWClass VerseKey::classdef(classes);

/******************************************************************************
 *  Initialize static members of VerseKey
 */

#include <osisbook.h>	// Initialize static members of canonical books structure

//struct sbook *VerseKey::builtin_books[2]       = {0,0};
struct sbook *VerseKey::builtin_books       = {0};
//const char    VerseKey::builtin_BMAX[2]        = {39, 27};
const char    VerseKey::builtin_BMAX        = 67;
//long         *VerseKey::offsets[2][2]  = {{VerseKey::otbks, VerseKey::otcps}, {VerseKey::ntbks, VerseKey::ntcps}};
bkref         *VerseKey::offsets[2]  = {VerseKey::kjvbks, VerseKey::kjvcps};
int           VerseKey::instance       = 0;
//VerseKey::LocaleCache   VerseKey::localeCache;
VerseKey::LocaleCache   *VerseKey::localeCache = 0;


/******************************************************************************
 * VerseKey::init - initializes instance of VerseKey
 */

void VerseKey::init() {
	myclass = &classdef;
	if (!instance)
		initstatics();

	instance++;
	autonorm = 1;		// default auto normalization to true
	headings = 0;		// default display headings option is false
	upperBound = 0;
	lowerBound = 0;
	boundSet = false;
	testament = 0;
	book = 0;
	chapter = 0;
	verse = 0;
	locale = 0;
	abbrevsCnt = 192;
	oldindexhack = true;

	setLocale(LocaleMgr::getSystemLocaleMgr()->getDefaultLocaleName());
}

/******************************************************************************
 * VerseKey Constructor - initializes instance of VerseKey
 *
 * ENT:	ikey - base key (will take various forms of 'BOOK CH:VS'.  See
 *		VerseKey::parse for more detailed information)
 */

VerseKey::VerseKey(const SWKey *ikey) : SWKey(*ikey)
{
	init();
	if (ikey)
		parse();
}


/******************************************************************************
 * VerseKey Constructor - initializes instance of VerseKey
 *
 * ENT:	ikey - text key (will take various forms of 'BOOK CH:VS'.  See
 *		VerseKey::parse for more detailed information)
 */

VerseKey::VerseKey(const char *ikey) : SWKey(ikey)
{
	init();
	if (ikey)
		parse();
}


VerseKey::VerseKey(VerseKey const &k) : SWKey(k)
{
	init();
	autonorm = k.autonorm;
	headings = k.headings;
	testament = k.Testament();
	book = k.Book();
	chapter = k.Chapter();
	verse = k.Verse();
	if (k.isBoundSet()) {
		LowerBound(k.LowerBound());
		UpperBound(k.UpperBound());
	}
}


VerseKey::VerseKey(const char *min, const char *max) : SWKey()
{
	init();
	LowerBound(min);
	UpperBound(max);
	setPosition(TOP);
}


SWKey *VerseKey::clone() const
{
	return new VerseKey(*this);
}


/******************************************************************************
 * VerseKey Destructor - cleans up instance of VerseKey
 *
 * ENT:	ikey - text key
 */

VerseKey::~VerseKey() {
	if (upperBound)
		delete upperBound;
	if (lowerBound)
		delete lowerBound;
	if (locale)
		delete [] locale;

	--instance;
	if (!instance) delete localeCache;
}


void VerseKey::setLocale(const char *name) {
	char *lBMAX;
	struct sbook **lbooks;
	bool useCache = false;

	if (localeCache->name)
		useCache = (!strcmp(localeCache->name, name));

	if (!useCache)	{	// if we're setting params for a new locale
		stdstr(&(localeCache->name), name);
		localeCache->abbrevsCnt = 0;
	}

	SWLocale *locale = (useCache) ? localeCache->locale : LocaleMgr::getSystemLocaleMgr()->getLocale(name);
	localeCache->locale = locale;

	if (locale) {
		locale->getBooks(&lBMAX, &lbooks, this);
		setBooks(lBMAX, lbooks);
		setBookAbbrevs(locale->getBookAbbrevs(), localeCache->abbrevsCnt);
		localeCache->abbrevsCnt = abbrevsCnt;
	}
	else {
		setBooks(&builtin_BMAX, &builtin_books);
		setBookAbbrevs(builtin_abbrevs, localeCache->abbrevsCnt);
		localeCache->abbrevsCnt = abbrevsCnt;
	}
	stdstr(&(this->locale), localeCache->name);

	if (lowerBound)
		LowerBound().setLocale(name);
	if (upperBound) 
		UpperBound().setLocale(name);
}

void VerseKey::setBooks(const char *iBMAX, struct sbook **ibooks) {
	BMAX = iBMAX;
	books = ibooks;
}


void VerseKey::setBookAbbrevs(const struct abbrev *bookAbbrevs, unsigned int size) {
	abbrevs = bookAbbrevs;
	if (!size) {
			/*
		for (abbrevsCnt = 0; *abbrevs[abbrevsCnt].ab; abbrevsCnt++) {
			if (strcmp(abbrevs[abbrevsCnt-1].ab, abbrevs[abbrevsCnt].ab) > 0) {
				fprintf(stderr, "ERROR: book abbreviation (canon.h or locale) misordered at entry: %s\n", abbrevs[abbrevsCnt].ab);
				exit(-1);
			}
		}
			*/
		for (int i = 0; i <= *BMAX; i++) {
			int bn = getBookAbbrev((*books)[i].name);
			if (bn != i) {
				SWLog::getSystemLog()->logError("Book: %s does not have a matching toupper abbrevs entry! book number returned was: %d", 
					(*books)[i].name, bn);
			}
		}
    }
	else abbrevsCnt = size;
}


/******************************************************************************
 * VerseKey::initstatics - initializes statics.  Performed only when first
 *						instance on VerseKey (or descendent) is created.
 */

void VerseKey::initstatics() {
	int l1, l2, chaptmp = 0;

	//builtin_books[0] = otbooks;
	//builtin_books[1] = ntbooks;
	builtin_books = osisbooks;
	
	localeCache = new LocaleCache();
/*
	for (l2 = 0; l2 <= builtin_BMAX; l2++) {
			builtin_books[l2].versemax = &(offsets[1][chaptmp]);
			chaptmp += getMaxChaptersInBook(l2);
		}
*/
	/*
	for (l1 = 0; l1 < 2; l1++) {
		for (l2 = 0; l2 < builtin_BMAX[l1]; l2++) {
			builtin_books[l1][l2].versemax = &vm[chaptmp];
			chaptmp += builtin_books[l1][l2].chapmax;
		}
	}
	*/
}


/******************************************************************************
 * VerseKey::parse - parses keytext into testament|book|chapter|verse
 *
 * RET:	error status
 */

char VerseKey::parse()
{

	
	testament = 2;
	book      = *BMAX;
	chapter   = 1;
	verse     = 1;
	int booklen   = 0;

	int error = 0;

	if (keytext) {
		ListKey tmpListKey = VerseKey::ParseVerseList(keytext);
		if (tmpListKey.Count()) {
			SWKey::setText((const char *)tmpListKey);
			for (int j = 0; j <= *BMAX; j++) {
				int matchlen = strlen((*books)[j].name);
				if (!strncmp(keytext, (*books)[j].name, matchlen)) {
					if (matchlen > booklen) {
						booklen = matchlen;
						//!!!WDG This is a temporary hack
						if (j < OTBOOKS)
							testament = 1;
						book = j;
					}
				}
			}
			//}

			if (booklen) {
				sscanf(&keytext[booklen], "%d:%d", &chapter, &verse);
			}
			else	error = KEYERR_FAILEDPARSE;
		} else error = KEYERR_FAILEDPARSE;
	}
	Normalize(1);
	freshtext();

	return (this->error) ? this->error : (this->error = error);
}


/******************************************************************************
 * VerseKey::freshtext - refreshes keytext based on
 *				testament|book|chapter|verse
 */

void VerseKey::freshtext() const
{
	SWBuf buf = "";
	int realbook = book;

	if (book < 1) {
		if (testament < 1)
			buf = "[ Module Heading ]";
		else buf.appendFormatted("[ Testament %d Heading ]", (int)testament);
	}
	else {
		if (realbook > *BMAX) {
				realbook = *BMAX;
		}
		buf.appendFormatted("%s %d:%d", (*books)[realbook].name, chapter, verse);
	}

	stdstr((char **)&keytext, buf.c_str());
}



/******************************************************************************
 * VerseKey::getBookAbbrev - Attempts to find a book from an abbreviation for a buffer
 *
 * ENT:	abbr - key for which to search;
 * RET:	book number or < 0 = not valid
 */

int VerseKey::getBookAbbrev(const char *iabbr)
{
	int loop, diff, abLen, min, max, target, retVal = -1;

	char *abbr = 0;

	//for (int i = 0; i < 2; i++) {
		stdstr(&abbr, iabbr);
		strstrip(abbr);
		//if (!i)
			toupperstr(abbr);
		abLen = strlen(abbr);

		if (abLen) {
			min = 0;
//			max = abbrevsCnt - 1;
			max 	= abbrevsCnt;
			while(1) {
				target = min + ((max - min) / 2);
				diff = strncmp(abbr, abbrevs[target].ab, abLen);
				if ((!diff)||(target >= max)||(target <= min))
					break;
				if (diff > 0)
					min = target;
				else	max = target;
			}
			for (; target > 0; target--) {
				if (strncmp(abbr, abbrevs[target-1].ab, abLen))
					break;
			}
				
			retVal = (!diff) ? abbrevs[target].book : -1;
		}
		//if (retVal > 0)
			//break;
	//}
	delete [] abbr;
	return retVal;
}

/******************************************************************************
 * VerseKey::ParseVerseList - Attempts to parse a buffer into separate
 *				verse entries returned in a ListKey
 *
 * ENT:	buf		- buffer to parse;
 *	defaultKey	- if verse, chap, book, or testament is left off,
 *				pull info from this key (ie. Gen 2:3; 4:5;
 *				Gen would be used when parsing the 4:5 section)
 *	expandRange	- whether or not to expand eg. John 1:10-12 or just
 *				save John 1:10
 *
 * RET:	ListKey reference filled with verse entries contained in buf
 *
 * COMMENT: This code works but wreaks.  Rewrite to make more maintainable.
 */

ListKey VerseKey::ParseVerseList(const char *buf, const char *defaultKey, bool expandRange) {
	SWKey textkey;

	char book[2048];
	char number[2048];
	int tobook = 0;
	int tonumber = 0;
	int chap = -1, verse = -1;
	int bookno = 0;
	VerseKey curkey, lBound;
	curkey.setLocale(getLocale());
	lBound.setLocale(getLocale());
	int loop;
	char comma = 0;
	char dash = 0;
	const char *orig = buf;
	int q;
	ListKey tmpListKey;
	ListKey internalListKey;
	SWKey tmpDefaultKey = defaultKey;
	char lastPartial = 0;
	bool inTerm = true;
	int notAllDigits;

	curkey.AutoNormalize(0);
	tmpListKey << tmpDefaultKey;
	tmpListKey.GetElement()->userData = (void *)buf;
	
	while (*buf) {
		switch (*buf) {
		case ' ':
			inTerm = true;
			while (true) {
				if ((!*number) || (chap < 0))
					break;
				for (q = 1; ((buf[q]) && (buf[q] != ' ')); q++);
				if (buf[q] == ':')
					break;
				inTerm = false;
				break;
			}
			if (inTerm) {
				book[tobook++] = ' ';
				break;
			}
		case ':':
			if (buf[1] != ' ') {		// for silly Mat 1:1: this verse....
				number[tonumber] = 0;
				tonumber = 0;
				if (*number)
					chap = atoi(number);
				*number = 0;
				break;
			}
			// otherwise drop down to next case

		case '-': 
		case ',': // on number new verse
		case ';': // on number new chapter
			number[tonumber] = 0;
			tonumber = 0;
			if (*number) {
				if (chap >= 0)
					verse = atoi(number);
				else	chap = atoi(number);
			}
			*number = 0;
			book[tobook] = 0;
			tobook = 0;
			bookno = -1;
			if (*book) {
				for (loop = strlen(book) - 1; loop+1; loop--) {
					if ((isdigit(book[loop])) || (book[loop] == ' ')) {
						book[loop] = 0;
						continue;
					}
					else {
						if ((SW_toupper(book[loop])=='F')&&(loop)) {
							if ((isdigit(book[loop-1])) || (book[loop-1] == ' ') || (SW_toupper(book[loop-1]) == 'F')) {
								book[loop] = 0;
								continue;
							}
						}
					}
					break;
				}

				for (loop = strlen(book) - 1; loop+1; loop--) {
					if (book[loop] == ' ') {
						if (isroman(&book[loop+1])) {
							if (verse == -1) {
								verse = chap;
								chap = from_rom(&book[loop+1]);
								book[loop] = 0;
							}
						}
	        				break;
					}
				}

				if ((!stricmp(book, "V")) || (!stricmp(book, "VER"))) {	// Verse abbrev
					if (verse == -1) {
						verse = chap;
						chap = VerseKey(tmpListKey).Chapter();
						*book = 0;
					}
				}
				if ((!stricmp(book, "ch")) || (!stricmp(book, "chap"))) {	// Verse abbrev
					strcpy(book, VerseKey(tmpListKey).getBookName());
				}
				bookno = getBookAbbrev(book);
			}
			if (((bookno > -1) || (!*book)) && ((*book) || (chap >= 0) || (verse >= 0))) {
				char partial = 0;
				curkey.Verse(1);
				curkey.Chapter(1);
				curkey.Book(1);

				if (bookno < 0) {
					curkey.Testament(VerseKey(tmpListKey).Testament());
					curkey.Book(VerseKey(tmpListKey).Book());
				}
				else {
					curkey.Testament(1);
					curkey.Book(bookno);
				}

				if (((comma)||((verse < 0)&&(bookno < 0)))&&(!lastPartial)) {
//				if (comma) {
					curkey.Chapter(VerseKey(tmpListKey).Chapter());
					curkey.Verse(chap);  // chap because this is the first number captured
				}
				else {
					if (chap >= 0) {
						curkey.Chapter(chap);
					}
					else {
						partial++;
						curkey.Chapter(1);
					}
					if (verse >= 0) {
						curkey.Verse(verse);
					}
					else {
						partial++;
						curkey.Verse(1);
					}
				}

				if ((*buf == '-') && (expandRange)) {	// if this is a dash save lowerBound and wait for upper
					VerseKey newElement;
					newElement.LowerBound(curkey);
					newElement.setPosition(TOP);
					tmpListKey << newElement;
					tmpListKey.GetElement()->userData = (void *)buf;
				}
				else {
					if (!dash) { 	// if last separator was not a dash just add
						if (expandRange && partial) {
							VerseKey newElement;
							newElement.LowerBound(curkey);
							if (partial > 1)
								curkey.setPosition(MAXCHAPTER);
							if (partial > 0)
								curkey = MAXVERSE;
							newElement.UpperBound(curkey);
							newElement = TOP;
							tmpListKey << newElement;
							tmpListKey.GetElement()->userData = (void *)buf;
						}
						else {
							tmpListKey << (const SWKey &)(const SWKey)(const char *)curkey;
							tmpListKey.GetElement()->userData = (void *)buf;
						}
					}
					else	if (expandRange) {
						VerseKey *newElement = SWDYNAMIC_CAST(VerseKey, tmpListKey.GetElement());
						if (newElement) {
							if (partial > 1)
								curkey = MAXCHAPTER;
							if (partial > 0)
								curkey = MAXVERSE;
							newElement->UpperBound(curkey);
							*newElement = TOP;
							tmpListKey.GetElement()->userData = (void *)buf;
						}
					}
				}
				lastPartial = partial;
			}
			*book = 0;
			chap = -1;
			verse = -1;
			if (*buf == ',')
				comma = 1;
			else	comma = 0;
			if (*buf == '-')
				dash = 1;
			else	dash = 0;
			break;
		case 10:	// ignore these
		case 13: 
		case '[': 
		case ']': 
		case '(': 
		case ')': 
		case '{': 
		case '}': 
			break;
		case '.':
			if (buf > orig)			// ignore (break) if preceeding char is not a digit
				for (notAllDigits = tobook; notAllDigits; notAllDigits--) {
					if ((!isdigit(book[notAllDigits-1])) && (!strchr(" .", book[notAllDigits-1])))
						break;
				}
				if (!notAllDigits)
					break;

			number[tonumber] = 0;
			tonumber = 0;
			if (*number)
				chap = atoi(number);
			*number = 0;
			break;
			
		default:
			if (isdigit(*buf)) {
				number[tonumber++] = *buf;
			}
			else {
				switch (*buf) {
				case ' ':    // ignore these and don't reset number
				case 'f':
				case 'F':
					break;
				default:
					number[tonumber] = 0;
					tonumber = 0;
					break;
				}
			}
			if (chap == -1)
				book[tobook++] = *buf;
		}
		buf++;
	}
	number[tonumber] = 0;
	tonumber = 0;
	if (*number) {
		if (chap >= 0)
			verse = atoi(number);
		else	chap = atoi(number);
	}
	*number = 0;
	book[tobook] = 0;
	tobook = 0;
	if (*book) {
		for (loop = strlen(book) - 1; loop+1; loop--) {
			if ((isdigit(book[loop])) || (book[loop] == ' ')) {
				book[loop] = 0;
				continue;
			}
			else {
				if ((SW_toupper(book[loop])=='F')&&(loop)) {
					if ((isdigit(book[loop-1])) || (book[loop-1] == ' ') || (SW_toupper(book[loop-1]) == 'F')) {
						book[loop] = 0;
						continue;
					}
				}
			}
			break;
		}

		for (loop = strlen(book) - 1; loop+1; loop--) {
			if (book[loop] == ' ') {
				if (isroman(&book[loop+1])) {
					if (verse == -1) {
						verse = chap;
						chap = from_rom(&book[loop+1]);
						book[loop] = 0;
					}
				}
				break;
			}
          }
               
		if ((!stricmp(book, "V")) || (!stricmp(book, "VER"))) {	// Verse abbrev.
			if (verse == -1) {
				verse = chap;
				chap = VerseKey(tmpListKey).Chapter();
				*book = 0;
			}
		}
			
		if ((!stricmp(book, "ch")) || (!stricmp(book, "chap"))) {	// Verse abbrev
			strcpy(book, VerseKey(tmpListKey).getBookName());
		}
		bookno = getBookAbbrev(book);
	}
	if (((bookno > -1) || (!*book)) && ((*book) || (chap >= 0) || (verse >= 0))) {
		char partial = 0;
		curkey.Verse(1);
		curkey.Chapter(1);
		curkey.Book(1);

		if (bookno < 0) {
			curkey.Testament(VerseKey(tmpListKey).Testament());
			curkey.Book(VerseKey(tmpListKey).Book());
		}
		else {
			curkey.Testament(1);
			curkey.Book(bookno);
		}

		if (((comma)||((verse < 0)&&(bookno < 0)))&&(!lastPartial)) {
//		if (comma) {
			curkey.Chapter(VerseKey(tmpListKey).Chapter());
			curkey.Verse(chap);  // chap because this is the first number captured
		}
		else {
			if (chap >= 0) {
				curkey.Chapter(chap);
			}
			else {
				partial++;
				curkey.Chapter(1);
			}
			if (verse >= 0) {
				curkey.Verse(verse);
			}
			else {
				partial++;
				curkey.Verse(1);
			}
		}

		if ((*buf == '-') && (expandRange)) {	// if this is a dash save lowerBound and wait for upper
			VerseKey newElement;
			newElement.LowerBound(curkey);
			newElement = TOP;
			tmpListKey << newElement;
			tmpListKey.GetElement()->userData = (void *)buf;
		}
		else {
			if (!dash) { 	// if last separator was not a dash just add
				if (expandRange && partial) {
					VerseKey newElement;
					newElement.LowerBound(curkey);
					if (partial > 1)
						curkey = MAXCHAPTER;
					if (partial > 0)
						curkey = MAXVERSE;
					newElement.UpperBound(curkey);
					newElement = TOP;
					tmpListKey << newElement;
					tmpListKey.GetElement()->userData = (void *)buf;
				}
				else {
					tmpListKey << (const SWKey &)(const SWKey)(const char *)curkey;
					tmpListKey.GetElement()->userData = (void *)buf;
				}
			}
			else if (expandRange) {
				VerseKey *newElement = SWDYNAMIC_CAST(VerseKey, tmpListKey.GetElement());
				if (newElement) {
					if (partial > 1)
						curkey = MAXCHAPTER;
					if (partial > 0)
						curkey = MAXVERSE;
					newElement->UpperBound(curkey);
					*newElement = TOP;
					tmpListKey.GetElement()->userData = (void *)buf;
				}
			}
		}
	}
	*book = 0;
	tmpListKey = TOP;
	tmpListKey.Remove();	// remove defaultKey
	internalListKey = tmpListKey;
	internalListKey = TOP;	// Align internalListKey to first element before passing back;

	return internalListKey;
}


/******************************************************************************
 * VerseKey::LowerBound	- sets / gets the lower boundary for this key
 */

VerseKey &VerseKey::LowerBound(const char *lb)
{
	if (!lowerBound) 
		initBounds();

	(*lowerBound) = lb;
	lowerBound->Normalize();
	lowerBound->setLocale( this->getLocale() );
	boundSet = true;
	return (*lowerBound);
}


/******************************************************************************
 * VerseKey::UpperBound	- sets / gets the upper boundary for this key
 */

VerseKey &VerseKey::UpperBound(const char *ub)
{
	if (!upperBound) 
		initBounds();

// need to set upperbound parsing to resolve to max verse/chap if not specified
	   (*upperBound) = ub;
	if (*upperBound < *lowerBound)
		*upperBound = *lowerBound;
	upperBound->Normalize();
	upperBound->setLocale( this->getLocale() );

// until we have a proper method to resolve max verse/chap use this kludge
	int len = strlen(ub);
	bool alpha = false;
	bool versespec = false;
	bool chapspec = false;
	for (int i = 0; i < len; i++) {
		if (isalpha(ub[i]))
			alpha = true;
		if (ub[i] == ':')	// if we have a : we assume verse spec
			versespec = true;
		if ((isdigit(ub[i])) && (alpha))	// if digit after alpha assume chap spec
			chapspec = true;
	}
	if (!chapspec)
		*upperBound = MAXCHAPTER;
	if (!versespec)
		*upperBound = MAXVERSE;
	

// -- end kludge
	boundSet = true;
	return (*upperBound);
}


/******************************************************************************
 * VerseKey::LowerBound	- sets / gets the lower boundary for this key
 */

VerseKey &VerseKey::LowerBound() const
{
	if (!lowerBound) 
		initBounds();

	return (*lowerBound);
}


/******************************************************************************
 * VerseKey::UpperBound	- sets / gets the upper boundary for this key
 */

VerseKey &VerseKey::UpperBound() const
{
	if (!upperBound) 
		initBounds();

	return (*upperBound);
}


/******************************************************************************
 * VerseKey::ClearBounds	- clears bounds for this VerseKey
 */

void VerseKey::ClearBounds()
{
	initBounds();
}


void VerseKey::initBounds() const
{
	if (!upperBound) {
		upperBound = new VerseKey();
		upperBound->AutoNormalize(0);
		upperBound->Headings(1);
	}
	if (!lowerBound) {
		lowerBound = new VerseKey();
		lowerBound->AutoNormalize(0);
		lowerBound->Headings(1);
	}

	lowerBound->Testament(0);
	lowerBound->Book(0);
	lowerBound->Chapter(0);
	lowerBound->Verse(0);

	upperBound->Testament(2);
	upperBound->Book(*BMAX);
	upperBound->Chapter(getMaxChaptersInBook(upperBound->Book()));
	upperBound->Verse(getMaxVerseInChapter(upperBound->Book(), upperBound->Chapter()));
	boundSet = false;
}


/******************************************************************************
 * VerseKey::copyFrom - Equates this VerseKey to another VerseKey
 */

void VerseKey::copyFrom(const VerseKey &ikey) {
	SWKey::copyFrom(ikey);

	parse();
}


/******************************************************************************
 * VerseKey::copyFrom - Equates this VerseKey to another SWKey
 */

void VerseKey::copyFrom(const SWKey &ikey) {
	SWKey::copyFrom(ikey);

	parse();
}


/******************************************************************************
 * VerseKey::getText - refreshes keytext before returning if cast to
 *				a (char *) is requested
 */

const char *VerseKey::getText() const {
	freshtext();
	return keytext;
}


const char *VerseKey::getShortText() const {
	static SWBuf buf = "";
	freshtext();
	/*
	if (book < 1) {
		if (testament < 1)
			sprintf(buf, "[ Module Heading ]");
		else sprintf(buf, "[ Testament %d Heading ]", (int)testament);
	}
	else {
		*/
	if (book < 0)
		buf = "[ Module Heading ]";
	else if (getMaxChaptersInBook(book) == TESTAMENT_HEADING) // !!!WDG needs testament support
		buf.appendFormatted("[ Testament %d Heading ]", (int)testament);
	else
		buf.appendFormatted("%s %d:%d", (*books)[book].prefAbbrev, chapter, verse);
	return buf.c_str();
}


const char *VerseKey::getBookName() const {
	return (*books)[book].name;
}


const char *VerseKey::getBookAbbrev() const {
	return (*books)[book].prefAbbrev;
}
/******************************************************************************
 * VerseKey::setPosition(SW_POSITION)	- Positions this key
 *
 * ENT:	p	- position
 *
 * RET:	*this
 */

void VerseKey::setPosition(SW_POSITION p) {
	switch (p) {
	case POS_TOP:
		testament = LowerBound().Testament();
		book      = LowerBound().Book();
		chapter   = LowerBound().Chapter();
		verse     = LowerBound().Verse();
		break;
	case POS_BOTTOM:
		testament = UpperBound().Testament();
		book      = UpperBound().Book();
		chapter   = UpperBound().Chapter();
		verse     = UpperBound().Verse();
		break;
	case POS_MAXVERSE:
		Normalize();
		verse     = getMaxVerseInChapter(book, chapter);
		break;
	case POS_MAXCHAPTER:
		verse     = 1;
		Normalize();
		chapter   = getMaxChaptersInBook(book);
		break;
	} 
	Normalize(1);
	Error();	// clear error from normalize
}


/******************************************************************************
 * VerseKey::increment	- Increments key a number of verses
 *
 * ENT:	step	- Number of verses to jump forward
 *
 * RET: *this
 */

void VerseKey::increment(int step) {
	char ierror = 0;
	Index(Index() + step);
	while ((!verse) && (!headings) && (!ierror)) {
		Index(Index() + 1);
		ierror = Error();
	}

	error = (ierror) ? ierror : error;
}


/******************************************************************************
 * VerseKey::decrement	- Decrements key a number of verses
 *
 * ENT:	step	- Number of verses to jump backward
 *
 * RET: *this
 */

void VerseKey::decrement(int step) {
	char ierror = 0;

	Index(Index() - step);
	while ((!verse) && (!headings) && (!ierror)) {
		Index(Index() - 1);
		ierror = Error();
	}
	if ((ierror) && (!headings))
		(*this)++;

	error = (ierror) ? ierror : error;
}


/******************************************************************************
 * VerseKey::Normalize	- checks limits and normalizes if necessary (e.g.
 *				Matthew 29:47 = Mark 2:2).  If last verse is
 *				exceeded, key is set to last Book CH:VS
 * RET: *this
 */

void VerseKey::Normalize(char autocheck)
{
	error = 0;
	bool valid = false;

	if ((autocheck) && (!autonorm))	// only normalize if we were explicitely called or if autonorm is turned on
		return;

	if ((headings) && (!verse))		// this is cheeze and temporary until deciding what actions should be taken.
		return;					// so headings should only be turned on when positioning with Index() or incrementors

	printf("normalizing %d:%d:%d\n", book, chapter, verse);fflush(NULL);
	while (!valid) {
		
		if (book <= 0 || book > *BMAX) break;
		
		while (chapter > getMaxChaptersInBook(book) && book <= *BMAX) {
			chapter -= getMaxChaptersInBook(book);
			book++;
		}

		while (chapter < !headings && (--book > 0)) {
			chapter += getMaxChaptersInBook(book);
		}

		while (verse > getMaxVerseInChapter(book, chapter)) {
			verse -= getMaxVerseInChapter(book, chapter);
			chapter++;
		}

		while (verse < !headings && (--chapter > 0)) {
			verse += getMaxVerseInChapter(book, chapter);
		}
		
		if (book > 0 && book <= *BMAX && 
			chapter >= !headings && chapter <= getMaxChaptersInBook(book) && 
			verse >= !headings && verse <= getMaxVerseInChapter(book, chapter))
				valid = true;
		
	}
	
	if (book > *BMAX) {
		book      = *BMAX;
		chapter   = getMaxChaptersInBook(book);
		verse     = getMaxVerseInChapter(book, chapter);
		error     = KEYERR_OUTOFBOUNDS;
	}

	if (book <= 0) {
		error     = ((!headings) || (book < 0)) ? KEYERR_OUTOFBOUNDS : 0;
		testament = ((headings) ? 0 : 1);
		book      = ((headings) ? 0 : 1);
		chapter   = ((headings) ? 0 : 1);
		verse     = ((headings) ? 0 : 1);
	}
	if (_compare(UpperBound()) > 0) {
		*this = UpperBound();
		error = KEYERR_OUTOFBOUNDS;
	}
	if (_compare(LowerBound()) < 0) {
		*this = LowerBound();
		error = KEYERR_OUTOFBOUNDS;
	}
}

//!!!WDG once it is working and becomes core we need to change these to get/set
/******************************************************************************
 * VerseKey::Testament - Gets testament
 *
 * RET:	value of testament
 */

char VerseKey::Testament() const
{
	return testament;
}


/******************************************************************************
 * VerseKey::Book - Gets book
 *
 * RET:	value of book
 */

char VerseKey::Book() const
{
	return book;
}


/******************************************************************************
 * VerseKey::Chapter - Gets chapter
 *
 * RET:	value of chapter
 */

int VerseKey::Chapter() const
{
	return chapter;
}


/******************************************************************************
 * VerseKey::Verse - Gets verse
 *
 * RET:	value of verse
 */

int VerseKey::Verse() const
{
	return verse;
}


/******************************************************************************
 * VerseKey::Testament - Sets/gets testament
 *
 * ENT:	itestament - value which to set testament
 *		[MAXPOS(char)] - only get
 *
 * RET:	if unchanged ->          value of testament
 *	if   changed -> previous value of testament
 */

char VerseKey::Testament(char itestament)
{
	char retval = testament;

	if (itestament != MAXPOS(char)) {
		testament = itestament;
		Normalize(1);
	}
	return retval;
}


/******************************************************************************
 * VerseKey::Book - Sets/gets book
 *
 * ENT:	ibook - value which to set book
 *		[MAXPOS(char)] - only get
 *
 * RET:	if unchanged ->          value of book
 *	if   changed -> previous value of book
 */

char VerseKey::Book(char ibook)
{
	char retval = book;

	Chapter(1);
	book = ibook;
	Normalize(1);

	return retval;
}


/******************************************************************************
 * VerseKey::Chapter - Sets/gets chapter
 *
 * ENT:	ichapter - value which to set chapter
 *		[MAXPOS(int)] - only get
 *
 * RET:	if unchanged ->          value of chapter
 *	if   changed -> previous value of chapter
 */

int VerseKey::Chapter(int ichapter)
{
	int retval = chapter;

	Verse(1);
	chapter = ichapter;
	Normalize(1);

	return retval;
}


/******************************************************************************
 * VerseKey::Verse - Sets/gets verse
 *
 * ENT:	iverse - value which to set verse
 *		[MAXPOS(int)] - only get
 *
 * RET:	if unchanged ->          value of verse
 *	if   changed -> previous value of verse
 */

int VerseKey::Verse(int iverse)
{
	int retval = verse;

	verse = iverse;
	Normalize(1);

	return retval;
}


/******************************************************************************
 * VerseKey::AutoNormalize - Sets/gets flag that tells VerseKey to auto-
 *				matically normalize itself when modified
 *
 * ENT:	iautonorm - value which to set autonorm
 *		[MAXPOS(char)] - only get
 *
 * RET:	if unchanged ->          value of autonorm
 *		if   changed -> previous value of autonorm
 */

char VerseKey::AutoNormalize(char iautonorm)
{
	char retval = autonorm;

	if (iautonorm != MAXPOS(char)) {
		autonorm = iautonorm;
		Normalize(1);
	}
	return retval;
}


/******************************************************************************
 * VerseKey::Headings - Sets/gets flag that tells VerseKey to include
 *					chap/book/testmnt/module headings
 *
 * ENT:	iheadings - value which to set headings
 *		[MAXPOS(char)] - only get
 *
 * RET:	if unchanged ->          value of headings
 *		if   changed -> previous value of headings
 */

char VerseKey::Headings(char iheadings)
{
	char retval = headings;

	if (iheadings != MAXPOS(char)) {
		headings = iheadings;
		Normalize(1);
	}
	return retval;
}


/******************************************************************************
 * VerseKey::findindex - binary search to find the index closest, but less
 *						than the given value.
 *
 * ENT:	array	- long * to array to search
 *		size		- number of elements in the array
 *		value	- value to find
 *
 * RET:	the index into the array that is less than but closest to value
 */

int VerseKey::findindex(bkref *array, int size, long value)
{
	int lbound, ubound, tval;

	lbound = 0;
	ubound = size - 1;
	while ((ubound - lbound) > 1) {
		tval = lbound + (ubound-lbound)/2;
		if (array[tval].offset <= value)
			lbound = tval;
		else ubound = tval;
	}
	return (array[ubound].offset <= value) ? ubound : lbound;
}


/******************************************************************************
 * VerseKey::Index - Gets index based upon current verse
 *
 * RET:	offset
 */

long VerseKey::Index() const
{
	long  loffset;

	if (!testament) { // if we want module heading
		loffset = 0;
		verse  = 0;
	}
	else {
		if (!book)
			chapter = 0;
		if (!chapter)
			verse   = 0;

		loffset = offsets[0][book].offset;
		loffset = offsets[1][(int)loffset + chapter].offset;
		if (offsets[0][book].maxnext == TESTAMENT_HEADING) // if we want testament heading
		{
			chapter = 0;
			verse = 0;
		}
	}
	if (oldindexhack && testament==2)
	{
		loffset -= 24115; //24115 is offset to start of NT
	}
	return (loffset + verse);
}


/******************************************************************************
 * VerseKey::Index - Gets index based upon current verse
 *
 * RET:	offset
 */

long VerseKey::NewIndex() const
{
	//static long otMaxIndex = 32300 - 8245;  // total positions - new testament positions
//	static long otMaxIndex = offsets[0][1][(int)offsets[0][0][BMAX[0]] + books[0][BMAX[0]].chapmax];
	//return ((testament-1) * otMaxIndex) + Index();
	return Index();
}


/******************************************************************************
 * VerseKey::Index - Sets index based upon current verse
 *
 * ENT:	iindex - value to set index to
 *
 * RET:	offset
 */

long VerseKey::Index(long iindex)
{
	long  offset;

// This is the dirty stuff --------------------------------------------

	//if (!testament)
	//	testament = 1;
/*
	if (iindex < 1) {				// if (-) or module heading
		if (testament < 2) {
			if (iindex < 0) {
				testament = 0;  // previously we changed 0 -> 1
				error     = KEYERR_OUTOFBOUNDS;
			}
			else testament = 0;		// we want module heading
		}
		else {
			testament--;
			iindex = (offsets[testament-1][1][offsize[testament-1][1]-1] + 
				books[testament-1][BMAX[testament-1]-1].versemax[books[testament-1][BMAX[testament-1]-1].chapmax-1]) + 
				iindex; // What a doozy! ((offset of last chapter + number of verses in the last chapter) + iindex)
		}
	}
*/
	if (iindex < 1) {				// if (-) or module heading
		if (iindex < 0) {
			testament = 0;  // previously we changed 0 -> 1
			error     = KEYERR_OUTOFBOUNDS;
		}
		else testament = 0;		// we want module heading
	}

	// --------------------------------------------------------------------

/*
	if (testament) {
		if ((!error) && (iindex)) {
			offset  = findindex(offsets[testament-1][1], offsize[testament-1][1], iindex);
			verse   = iindex - offsets[testament-1][1][offset];
			book    = findindex(offsets[testament-1][0], offsize[testament-1][0], offset);
			chapter = offset - offsets[testament-1][0][VerseKey::book];
			verse   = (chapter) ? verse : 0;  
				// funny check. if we are index=1 (testmt header) all gets set to 0 exept verse.  
				//Don't know why.  Fix if you figure out.  Think its in the offsets table.
			if (verse) {		// only check if -1 won't give negative
				if (verse > books[testament-1][book-1].versemax[chapter-1]) {
					if (testament > 1) {
						verse = books[testament-1][book-1].versemax[chapter-1];
						error = KEYERR_OUTOFBOUNDS;
					}
					else {
						testament++;
						Index(verse - books[testament-2][book-1].versemax[chapter-1]);
					}
				}
			}
		}
	}
*/
	if (testament) {
		if ((!error) && (iindex)) {
			offset  = findindex(offsets[1], offsize[1], iindex);
			verse   = iindex - offsets[1][offset].offset;
			book    = findindex(offsets[0], offsize[0], offset);
			chapter = offset - offsets[0][VerseKey::book].offset;
			verse   = (chapter) ? verse : 0;  
				// funny check. if we are index=1 (testmt header) all gets set to 0 exept verse.  
				//Don't know why.  Fix if you figure out.  Think its in the offsets table. !!!WDG fix
		}
	}

	if (_compare(UpperBound()) > 0) {
		*this = UpperBound();
		error = KEYERR_OUTOFBOUNDS;
	}
	if (_compare(LowerBound()) < 0) {
		*this = LowerBound();
		error = KEYERR_OUTOFBOUNDS;
	}
	return Index();
}


/******************************************************************************
 * VerseKey::compare	- Compares another SWKey object
 *
 * ENT:	ikey - key to compare with this one
 *
 * RET:	>0 if this VerseKey is greater than compare VerseKey
 *	<0 <
 *	 0 =
 */

int VerseKey::compare(const SWKey &ikey)
{
	VerseKey ivkey = (const char *)ikey;
	return _compare(ivkey);
}


/******************************************************************************
 * VerseKey::_compare	- Compares another VerseKey object
 *
 * ENT:	ikey - key to compare with this one
 *
 * RET:	>0 if this VerseKey is greater than compare VerseKey
 *	<0 <
 *	 0 =
 */

int VerseKey::_compare(const VerseKey &ivkey)
{
	long keyval1 = 0;
	long keyval2 = 0;

	//keyval1 += Testament() * 1000000000;
	//keyval2 += ivkey.Testament() * 1000000000;
	keyval1 += Book() * 1000000;
	keyval2 += ivkey.Book() * 1000000;
	keyval1 += Chapter() * 1000;
	keyval2 += ivkey.Chapter() * 1000;
	keyval1 += Verse();
	keyval2 += ivkey.Verse();
	keyval1 -= keyval2;
	keyval1 = (keyval1) ? ((keyval1 > 0) ? 1 : -1) /*keyval1/labs(keyval1)*/:0; // -1 | 0 | 1
	return keyval1;
}


const char *VerseKey::getOSISRef() const {
	static SWBuf buf = "";
	freshtext();
	if (Verse())
		buf.appendFormatted("%s.%d.%d", osisbooks[book].prefAbbrev, chapter, verse);
	else if (Chapter())
		buf.appendFormatted("%s.%d", osisbooks[book].prefAbbrev, chapter);
	else if (Book())
		buf.appendFormatted("%s", osisbooks[book].prefAbbrev);
	else	buf = "";
	
	return buf.c_str();
}

const char VerseKey::getMaxBooks() const {
	return *BMAX;
}

const char *VerseKey::getNameOfBook(char book) const {
	return osisbooks[book].name;
}

const char *VerseKey::getPrefAbbrev(char book) const {
	return osisbooks[book].prefAbbrev;
}

const int VerseKey::getMaxChaptersInBook(char book) const {
	return (offsets[0][book].maxnext == TESTAMENT_HEADING ? 0 : offsets[0][book].maxnext);
}

const int VerseKey::getMaxVerseInChapter(char book, int chapter) const {
	return offsets[1][(offsets[0][book].offset)+chapter].maxnext;
}



/******************************************************************************
 * VerseKey::getRangeText - returns parsable range text for this key
 */

const char *VerseKey::getRangeText() const {
	if (isBoundSet()) {
		SWBuf buf;
		buf.appendFormatted("%s-%s", (const char *)LowerBound(), (const char *)UpperBound());
		stdstr(&rangeText, buf.c_str());
	}
	else stdstr(&rangeText, getText());
	return rangeText;
}

SWORD_NAMESPACE_END