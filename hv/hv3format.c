/*
 * This file contains the implementation of the [::tclqjs::format] 
 * command, used to beautify javascript code. 
 *
 * The reason for including this in Hv3 is that a lot of the most interesting
 * javascript floating around the web is distributed with unnecessary
 * white-space removed. This makes it too hard to read. If Hv3 reformats
 * the javascript before processing it, it is much easier to debug.
 */
#define JSTOKEN_OPEN_BRACKET    1
#define JSTOKEN_CLOSE_BRACKET   2
#define JSTOKEN_OPEN_BRACE      3
#define JSTOKEN_CLOSE_BRACE     4
#define JSTOKEN_SEMICOLON       5
#define JSTOKEN_NEWLINE         6
#define JSTOKEN_SPACE           7

#define JSTOKEN_WORD            8
#define JSTOKEN_PUNC            9

#include <ctype.h>
#define iswordchar(x) (isalnum((unsigned char)(x)) || (x) == '_')

typedef struct JsBlob JsBlob;
struct JsBlob {

  const char *zIn;                 /* Input: javascript blob (nul-terminated) */
  const char *zCsr;                /* Current cursor point in zIn[] */
  int iLevel;                      /* Current block level */

  /* This variable points to the start of the previous identifier, if
   * the previous character was part of that identifier. Otherwise it is
   * a copy of JsBlob.zCsr.
   */
  const char *zPrevWord;
  unsigned int nPrevWord;

  Tcl_Obj *pOut;                   /* Tcl list of output lines */
  Tcl_Obj *pLine;                  /* Current output line */
};

static void formatLinefeed(JsBlob *);
static void formatSpace(JsBlob *);
static void formatColon(JsBlob *);
static void formatSemicolon(JsBlob *);

static void formatBracketOpen(JsBlob *);
static void formatBracketClose(JsBlob *);
static void formatSquareOpen(JsBlob *);
static void formatSquareClose(JsBlob *);

static void formatBlockOpen(JsBlob *);
static void formatBlockClose(JsBlob *);

static void formatSlash(JsBlob *);

static void formatDot(JsBlob *);
static void formatComma(JsBlob *);

static void formatAlphanumeric(JsBlob *);
static void formatSymbol(JsBlob *);

static void backupEmptyLine(JsBlob *pBlob){}

static void writeOut(JsBlob *pBlob, const char *z, int n)
{
    if (!pBlob->pLine) {
        pBlob->pLine = Tcl_NewObj();
        Tcl_IncrRefCount(pBlob->pLine);
        for (int i = 0; i < pBlob->iLevel; i++) {
            Tcl_AppendToObj(pBlob->pLine, "    ", 4);
        }
    }
    Tcl_AppendToObj(pBlob->pLine, z, n);
}

static void
writeLine(JsBlob *pBlob)
{
    if (pBlob->pLine) {
        Tcl_ListObjAppendElement(0, pBlob->pOut, pBlob->pLine);
        Tcl_DecrRefCount(pBlob->pLine);
        pBlob->pLine = 0;
    } else {
        Tcl_ListObjAppendElement(0, pBlob->pOut, Tcl_NewStringObj("", 0));
    }
}

static int
prevWasKeyword(JsBlob *pBlob)
{
	static const char *aKeyword[] = {
		"await",        "break",       "case",       "catch",
		"class",        "const",       "continue",   "default",
		"do",           "else",        "enum",       "export",
		"false",        "for",         "function",   "if",
		"import",       "in",          "instanceof", "let",
		"new",          "null",        "return",     "static",
		"super",        "switch",      "this",       "throw",
		"true",         "try",         "typeof",     "var",
		"void",         "while",       "with",       "yield",
		0
	};
    if (pBlob->nPrevWord > 0) {
        for (int i=0; aKeyword[i]; i++) {
            if( 
                strlen(aKeyword[i]) == pBlob->nPrevWord &&
                strncmp(aKeyword[i], pBlob->zPrevWord, pBlob->nPrevWord) == 0
            ) return 1;
        }
    }
    return 0;
}

static void formatLinefeed(JsBlob *pBlob) 
{
    if (pBlob->pLine) writeLine(pBlob);
}

static void formatSpace(JsBlob *pBlob)
{
    const char *zNext = &pBlob->zCsr[1];
    if (
        (prevWasKeyword(pBlob)) || 
        (0 == strncmp("in", zNext, 2)) || 
        (0 == strncmp("new", zNext, 3))
    ) {
        writeOut(pBlob, " ", 1);
        pBlob->zPrevWord = 0;
        pBlob->nPrevWord = 0;
    } 
}

static void formatColon(JsBlob *pBlob)
{
    if( 
        pBlob->pLine && 
        Tcl_RegExpMatch(0, Tcl_GetString(pBlob->pLine), "^ *case *")
    ) {
        writeOut(pBlob, ":", 1);
        writeLine(pBlob);
    } else {
        formatSymbol(pBlob);
    }
}

static void formatSemicolon(JsBlob *pBlob)
{
    backupEmptyLine(pBlob);
    writeOut(pBlob, ";", 1);
    if( 
        pBlob->pLine && 
        Tcl_RegExpMatch(0, Tcl_GetString(pBlob->pLine), "^ *for *")
    ) {
        writeOut(pBlob, " ", 1);
    } else {
        writeLine(pBlob);
    }
}

static void formatBracketOpen(JsBlob *pBlob)
{
    char prev = 0;
    if (pBlob->zCsr>pBlob->zIn) {
        prev = pBlob->zCsr[-1];
    }
    if (prevWasKeyword(pBlob) && prev != ' ' && prev != '\t') {
        writeOut(pBlob, " ", 1);
    }
    writeOut(pBlob, "(", 1);
}
static void formatBracketClose(JsBlob *pBlob)
{
    writeOut(pBlob, ")", 1);
}

static void formatSquareOpen(JsBlob *pBlob)
{
    writeOut(pBlob, "[", 1);
}
static void formatSquareClose(JsBlob *pBlob)
{
    writeOut(pBlob, "]", 1);
}

static void formatBlockOpen(JsBlob *pBlob)
{
    if( 
        pBlob->pLine && 
        Tcl_RegExpMatch(0, Tcl_GetString(pBlob->pLine), " $")
    ) {
        writeOut(pBlob, " ", 1);
    }
    writeOut(pBlob, " {", 2);
    writeLine(pBlob);
    pBlob->iLevel++;
}
static void formatBlockClose(JsBlob *pBlob)
{
/*
    if( 
        pBlob->pLine && !Tcl_RegExpMatch(0, Tcl_GetString(pBlob->pLine), ";$")
    ) {
        formatSemicolon(pBlob);
    }
*/
    if (pBlob->pLine) writeLine(pBlob);

    pBlob->iLevel--;
    writeOut(pBlob, "}", 1);
    writeLine(pBlob);
}

static void formatQuotedstring(JsBlob *pBlob)
{
    int isEscaped = 0;
    const char *z = pBlob->zCsr;
    const char c = *z;
    assert(c == '\'' || c == '"');

    do {
        isEscaped = ((!isEscaped && *z == '\\')?1:0);
        z++;
    } while (*z && (isEscaped || *z!=c));

    if (*z) z++;
    writeOut(pBlob, pBlob->zCsr, z - pBlob->zCsr);
    pBlob->zCsr = &z[-1];
}

static void formatSlash(JsBlob *pBlob)
{
    char next = pBlob->zCsr[1];
    char prev = 0;
    if (pBlob->zCsr > pBlob->zIn) {
        prev = pBlob->zCsr[-1];
    }

    if (next == '*') {                          /* C style comment */
        const char *z = &pBlob->zCsr[1];
        while (*z && (*z != '/' || z[-1] != '*')) z++;
        writeOut(pBlob, pBlob->zCsr, 1 + z - pBlob->zCsr);
        pBlob->zCsr = &z[(*z?0:-1)];
    }

    else if (next == '/') {                     /* C++ style comment */
        const char *z = pBlob->zCsr;
        while (*z && *z != '\n') z++;
        writeOut(pBlob, pBlob->zCsr, z - pBlob->zCsr);
        writeLine(pBlob);
        pBlob->zCsr = &z[(*z?0:-1)];
    }

    else if (pBlob->zPrevWord && !prevWasKeyword(pBlob)) {
        writeOut(pBlob, ((next=='=')?" /":" / "), -1); 
    }

    else if (prev == '*') {
        writeOut(pBlob, "/ ", 2);
    }

    else if (prev == ')') {
        writeOut(pBlob, " / ", 3);
    }

    else {                                      /* Regular expression */
        int isEscaped = 0;
        const char *z = ++pBlob->zCsr;
        writeOut(pBlob, ((prevWasKeyword(pBlob) || (prev=='='))?" /":"/"), -1); 
        do {
            isEscaped = ((!isEscaped && *z == '\\')?1:0);
            z++;
        } while (*z && (isEscaped || *z!='/'));

        if (*z) z++;
        writeOut(pBlob, pBlob->zCsr, z - pBlob->zCsr);
        pBlob->zCsr = &z[-1];
    }
}

static void formatDot(JsBlob *pBlob)
{
    writeOut(pBlob, ".", 1);
}

static void formatComma(JsBlob *pBlob)
{
    writeOut(pBlob, ", ", 2);
}

static void formatAlphanumeric(JsBlob *pBlob)
{
    if (!pBlob->zPrevWord) {
        pBlob->zPrevWord = pBlob->zCsr;
        pBlob->nPrevWord = 0;
    } 
    pBlob->nPrevWord++;
    writeOut(pBlob, pBlob->zCsr, 1);
}

static void formatSymbol(JsBlob *pBlob)
{
    char zSpecial[] = "-+*%<=>?:&|/!";

    char c = pBlob->zCsr[0];
    char next = pBlob->zCsr[1];
    char prev = 0;
    if (pBlob->zCsr > pBlob->zIn){
        prev = pBlob->zCsr[-1];
    }

    if (c == '!' && next != '='){
        writeOut(pBlob, "!", 1);
    }

    else if (c == '~' || c == '^') {
        writeOut(pBlob, pBlob->zCsr, 1);
    } 

    else if (c == next && (c == '+' || c == '-')) {
        writeOut(pBlob, pBlob->zCsr, 1);
    }

    else {
        if (0 == strchr(zSpecial, (unsigned char)prev)) {
            writeOut(pBlob, " ", 1);
        }

        writeOut(pBlob, pBlob->zCsr, 1);

        if (0 == strchr(zSpecial, (unsigned char)next)) {
            writeOut(pBlob, " ", 1);
        }
    }
}

static void formatLessthan(JsBlob *pBlob)
{
    if (0==strncmp("<!--", pBlob->zCsr, 4)) {
        const char *z = pBlob->zCsr;
        while (*z && *z != '\n') z++;
        writeOut(pBlob, pBlob->zCsr, z - pBlob->zCsr);
        writeLine(pBlob);
        pBlob->zCsr = &z[(*z?0:-1)];
    } else {
        formatSymbol(pBlob);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * tclSeeFormat --
 *
 *         ::qjs::format JAVASCRIPT-CODE
 *
 *     Used to beautify javascript code. The theory is that this will
 *     make it easier to debug scripts running in Hv3.
 *
 * Results:
 *     Standard Tcl result.
 *
 * Side effects:
 *     None
 *
 *---------------------------------------------------------------------------
 */
static int tclSeeFormat(
    ClientData clientData,             /* Not used */
    Tcl_Interp *interp,                /* Current interpreter. */
    int objc,                          /* Number of arguments. */
    Tcl_Obj *CONST objv[]              /* Argument strings. */
) {
    JsBlob blob;
    Tcl_Obj *pScript;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "JAVASCRIPT-CODE");
        return TCL_ERROR;
    }

    /* Initialise variable "blob" */
    memset(&blob, 0, sizeof(JsBlob));
    blob.pOut = Tcl_NewObj();
    Tcl_IncrRefCount(blob.pOut);
    blob.zIn = Tcl_GetString(objv[1]);
    blob.zCsr = blob.zIn;
    blob.zPrevWord = blob.zIn;

    while (*blob.zCsr) {
        char c = *blob.zCsr;
        switch (c) {
            case '\n': formatLinefeed(&blob);     break;

            case ' ':  formatSpace(&blob);        break;
            case '\t': formatSpace(&blob);        break;
            case ':':  formatColon(&blob);        break;
            case ';':  formatSemicolon(&blob);    break;

            case '(':  formatBracketOpen(&blob);  break;
            case ')':  formatBracketClose(&blob); break;
            case '[':  formatSquareOpen(&blob);   break;
            case ']':  formatSquareClose(&blob);  break;
            case '{':  formatBlockOpen(&blob);    break;
            case '}':  formatBlockClose(&blob);   break;

            case '"':  formatQuotedstring(&blob);  break;
            case '\'': formatQuotedstring(&blob);  break;

            case '/':  formatSlash(&blob);        break;
            case '.':  formatDot(&blob);          break;
            case ',':  formatComma(&blob);        break;

            case '<':  formatLessthan(&blob);     break;

            case '~': case '^': case '-': case '+':
            case '*': case '%': case '>': case '=':
            case '?': case '&': case '|': case '!':
                formatSymbol(&blob);
                break;

            default:
                formatAlphanumeric(&blob);
                break;
        }

        blob.zCsr++;
        if (!iswordchar(c) && c != ' ' && c != '\t') {
            blob.zPrevWord = 0;
            blob.nPrevWord = 0;
        }
    }

    /* Hand the result to the interpreter. */
    pScript = Tcl_NewObj();
    Tcl_ListObjAppendElement(0, pScript, Tcl_NewStringObj("join", 4));
    Tcl_ListObjAppendElement(0, pScript, blob.pOut);
    Tcl_ListObjAppendElement(0, pScript, Tcl_NewStringObj("\n", 1));
    Tcl_DecrRefCount(blob.pOut);
    return Tcl_EvalObjEx(interp, pScript, TCL_GLOBAL_ONLY);
}


