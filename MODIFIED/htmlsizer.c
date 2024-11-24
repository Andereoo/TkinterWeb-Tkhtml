static char const rcsid[] =
        "@(#) $Id: htmlsizer.c,v 1.44 2005/03/23 01:36:54 danielk1977 Exp $";

/*
** Routines used to compute the style and size of individual elements.
**
** This source code is released into the public domain by the author,
** D. Richard Hipp, on 2002 December 17.  Instead of a license, here
** is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
*/
#include <tk.h>
#include <string.h>
#include <stdlib.h>
#include "html.h"

/*
** Get the current rendering style.  In other words, get the style
** that is currently on the top of the style stack.
*/
static HtmlStyle
GetCurrentStyle(htmlPtr)
    HtmlWidget *htmlPtr;
{
    HtmlStyle style;
    if (htmlPtr->styleStack) {
        style = htmlPtr->styleStack->style;
    }
    else {
        style.font = NormalFont(2);
        style.color = COLOR_Normal;
        style.bgcolor = COLOR_Background;
        style.subscript = 0;
        style.align = ALIGN_Left;
        style.flags = 0;
        style.expbg = 0;
    }
    return style;
}

/*
** Push a new rendering style onto the stack.
*/
static void
PushStyleStack(htmlPtr, tag, style)
    HtmlWidget *htmlPtr;               /* Widget on which to push the style */
    int tag;                           /* Tag for this style.  Normally the
                                        * end-tag such as </h3> or </em>. */
    HtmlStyle style;                   /* The style to push */
{
    HtmlStyleStack *p;

    p = HtmlAlloc(sizeof(*p));
    p->pNext = htmlPtr->styleStack;
    p->type = tag;
    p->style = style;
    htmlPtr->styleStack = p;
}

/*
** Pop a rendering style off of the stack.
**
** The top-most style on the stack should have a tag equal to "tag".
** If not, then we have an HTML coding error.  Perhaps something
** like this:  "Some text <em>Enphasized</i> more text".  It is an
** interesting problem to figure out how to respond sanely to this
** kind of error.  Our solution it to keep popping the stack until
** we find the correct tag, or until the stack is empty.
*/
HtmlStyle
HtmlPopStyleStack(htmlPtr, tag)
    HtmlWidget *htmlPtr;
    int tag;
{
    int type;
    HtmlStyleStack *p;
    static Html_u8 priority[Html_TypeCount + 1];

    if (priority[Html_TABLE] == 0) {
        int i;
        for (i = 0; i <= Html_TypeCount; i++)
            priority[i] = 1;
        priority[Html_TD] = 2;
        priority[Html_EndTD] = 2;
        priority[Html_TH] = 2;
        priority[Html_EndTH] = 2;
        priority[Html_TR] = 3;
        priority[Html_EndTR] = 3;
        priority[Html_TABLE] = 4;
        priority[Html_EndTABLE] = 4;
    }
    if (tag <= 0 || tag > Html_TypeCount) {
        CANT_HAPPEN;
        return GetCurrentStyle(htmlPtr);
    }
    while ((p = htmlPtr->styleStack) != 0) {
        type = p->type;
        if (type <= 0 || type > Html_TypeCount) {
            CANT_HAPPEN;
            return GetCurrentStyle(htmlPtr);
        }
        if (type != tag && priority[type] > priority[tag]) {
            return GetCurrentStyle(htmlPtr);
        }
        htmlPtr->styleStack = p->pNext;
        HtmlFree(p);
        if (type == tag) {
            break;
        }
    }
    return GetCurrentStyle(htmlPtr);
}

/*
** Change the font size on the given style by the delta-amount given
*/
static void
ScaleFont(pStyle, delta)
    HtmlStyle *pStyle;
    int delta;
{
    int size = FontSize(pStyle->font) + delta;
    if (size < 0) {
        delta -= size;
    }
    else if (size > 6) {
        delta -= size - 6;
    }
    pStyle->font += delta;
}

/*
** Lookup an argument in the given markup with the name given.
** Return a pointer to its value, or the given default
** value if it doesn't appear.
*/
char *
HtmlMarkupArg(p, tag, zDefault)
    HtmlElement *p;
    const char *tag;
    char *zDefault;
{
    int i;
    if (!HtmlIsMarkup(p)) {
        return 0;
    }
    for (i = 0; i < p->base.count; i += 2) {
        if (strcmp(p->markup.argv[i], tag) == 0) {
            return p->markup.argv[i + 1];
        }
    }
    return zDefault;
}

/*
** Return an alignment or justification flag associated with the
** given markup.  The given default value is returned if no alignment is
** specified.
*/
static int
GetAlignment(p, dflt)
    HtmlElement *p;
    int dflt;
{
    char *z = HtmlMarkupArg(p, "align", 0);
    int rc = dflt;
    if (z) {
        if (stricmp(z, "left") == 0) {
            rc = ALIGN_Left;
        }
        else if (stricmp(z, "right") == 0) {
            rc = ALIGN_Right;
        }
        else if (stricmp(z, "center") == 0) {
            rc = ALIGN_Center;
        }
    }
    return rc;
}

/*
** The "type" argument to the given element might describe the type
** for an ordered list.  Return the corresponding LI_TYPE_* entry
** if this is the case, or the default value if it isn't.
*/
static int
GetOrderedListType(p, dflt)
    HtmlElement *p;
    int dflt;
{
    char *z;

    z = HtmlMarkupArg(p, "type", 0);
    if (z) {
        switch (*z) {
            case 'A':
                dflt = LI_TYPE_Enum_A;
                break;
            case 'a':
                dflt = LI_TYPE_Enum_a;
                break;
            case '1':
                dflt = LI_TYPE_Enum_1;
                break;
            case 'I':
                dflt = LI_TYPE_Enum_I;
                break;
            case 'i':
                dflt = LI_TYPE_Enum_i;
                break;
            default:
                break;
        }
    }
    else {
    }
    return dflt;
}

/*
** The "type" argument to the given element might describe a type
** for an unordered list.  Return the corresponding LI_TYPE entry
** if this is the case, or the default value if it isn't.
*/
static int
GetUnorderedListType(p, dflt)
    HtmlElement *p;
    int dflt;
{
    char *z;

    z = HtmlMarkupArg(p, "type", 0);
    if (z) {
        if (stricmp(z, "disc") == 0) {
            dflt = LI_TYPE_Bullet1;
        }
        else if (stricmp(z, "circle") == 0) {
            dflt = LI_TYPE_Bullet2;
        }
        else if (stricmp(z, "square") == 0) {
            dflt = LI_TYPE_Bullet3;
        }
    }
    return dflt;
}

/*
** Add the STY_Invisible style to every token between pFirst and pLast.
*/
static void
MakeInvisible(pFirst, pLast)
    HtmlElement *pFirst;
    HtmlElement *pLast;
{
    if (pFirst == 0)
        return;
    pFirst = pFirst->pNext;
    while (pFirst && pFirst != pLast) {
        pFirst->base.style.flags |= STY_Invisible;
        pFirst = pFirst->pNext;
    }
}

/*
** For the markup <a href=XXX>, find out if the URL has been visited
** before or not.  Return COLOR_Visited or COLOR_Unvisited, as 
** appropriate.
**
** This routine may invoke a callback procedure which could delete
** the HTML widget.  The calling function should call HtmlLock()
** if it needs the widget structure to be preserved.
*/
static int
GetLinkColor(htmlPtr, zURL)
    HtmlWidget *htmlPtr;
    char *zURL;
{
    char *zCmd;
    int result;
    int isVisited;

    if (htmlPtr->tkwin == 0) {
        return COLOR_Normal;
    }
    if (htmlPtr->zIsVisited == 0 || htmlPtr->zIsVisited[0] == 0) {
        return COLOR_Unvisited;
    }
    zCmd = HtmlAlloc(strlen(htmlPtr->zIsVisited) + strlen(zURL) + 10);
    if (zCmd == 0) {
        return COLOR_Unvisited;
    }
    sprintf(zCmd, "%s {%s}", htmlPtr->zIsVisited, zURL);
    HtmlLock(htmlPtr);
    result = Tcl_GlobalEval(htmlPtr->interp, zCmd);
    HtmlFree(zCmd);
    if (HtmlUnlock(htmlPtr)) {
        return COLOR_Unvisited;
    }
    if (result != TCL_OK) {
        goto errorOut;
    }
    result = Tcl_GetBoolean(htmlPtr->interp, htmlPtr->interp->result,
                            &isVisited);
    if (result != TCL_OK) {
        goto errorOut;
    }
    return isVisited ? COLOR_Visited : COLOR_Unvisited;

  errorOut:
    Tcl_AddErrorInfo(htmlPtr->interp,
                     "\n    (\"-isvisitedcommand\" command executed by html widget)");
    Tcl_BackgroundError(htmlPtr->interp);
    return COLOR_Unvisited;
}

static int *
GetCoords(str, nptr)
    char *str;
    int *nptr;
{
    char *cp = str, *ncp;
    int i, n = 0, sz = 4, *cr;
    cr = (int *) HtmlAlloc(sz * sizeof(int));
    while (cp) {
        while (*cp && (!isdigit(*cp)))
            cp++;
        if ((!*cp) || (!isdigit(*cp)))
            break;
        cr[n] = (int) strtol(cp, &ncp, 10);
        if (cp == ncp)
            break;
        cp = ncp;
        n++;
        if (n >= sz) {
            sz += 4;
            cr = (int *) HtmlRealloc((char *) cr, sz * sizeof(int));
        }
    }
    *nptr = n;
    return cr;
}

/*
** This routine adds information to the input texts that doesn't change
** when the display is resized or when new fonts are selected, etc.
** Mostly this means adding style attributes.  But other constant
** information (such as numbering on <li> and images used for <IMG>)
** is also obtained.  The key is that this routine is only called
** once, where the sizer and layout routines can be called many times.
**
** This routine is called whenever the list of elements grows.  The
** style stack is stored as part of the HTML widget so that we can
** always continue where we left off the last time.
**
** In addition to adding style, this routine will invoke callbacks
** needed to acquire information about a markup.  The htmlPtr->zIsVisitied
** callback is called for each <a> and the htmlPtr->zGetImage is called
** for each <IMG> or for each <LI> that has a SRC= field.
**
** This routine may invoke a callback procedure which could delete
** the HTML widget.
**
** When a markup is inserted or deleted from the token list, the
** style routine must be completely rerun from the beginning.  So
** what we said above, that this routine is only run once, is not
** strictly true.
*/
void
HtmlAddStyle(htmlPtr, p)
    HtmlWidget *htmlPtr;
    HtmlElement *p;
{
    HtmlStyle style;                   /* Current style */
    int size;                          /* A new font size */
    int i;                             /* Loop counter */
    int paraAlign;                     /* Current paragraph alignment */
    int rowAlign;                      /* Current table row alignment */
    int anchorFlags;                   /* Flags associated with <a> tag */
    int inDt;                          /* True if within <dt>..</dt> */
    HtmlStyle nextStyle;               /* Style for next token if
                                        * useNextStyle==1 */
    int useNextStyle = 0;              /* True if nextStyle is valid */
    char *z;                           /* A tag parameter's value */

    /*
     * The size of header fonts relative to the current font size 
     */
    static int header_sizes[] = { +2, +1, 1, 1, -1, -1 };

    /*
     * Don't allow recursion 
     */
    if (htmlPtr->flags & STYLER_RUNNING) {
        return;
    }
    htmlPtr->flags |= STYLER_RUNNING;

    /*
     * Load the style state out of the htmlPtr structure and into local **
     * variables.  This is purely a matter of convenience... 
     */
    style = GetCurrentStyle(htmlPtr);
    paraAlign = htmlPtr->paraAlignment;
    rowAlign = htmlPtr->rowAlignment;
    anchorFlags = htmlPtr->anchorFlags;
    inDt = htmlPtr->inDt;

    /*
     * Loop over tokens 
     */
    while (htmlPtr->pFirst && p) {
        switch (p->base.type) {
            case Html_A:
                if (htmlPtr->anchorStart) {
                    style = HtmlPopStyleStack(htmlPtr, Html_EndA);
                    htmlPtr->anchorStart = 0;
                    anchorFlags = 0;
                }
                z = HtmlMarkupArg(p, "href", 0);
                if (z) {
                    HtmlLock(htmlPtr);
                    style.color = GetLinkColor(htmlPtr, z);
                    if (htmlPtr->underlineLinks) {
                        style.flags |= STY_Underline;
                    }
                    if (HtmlUnlock(htmlPtr))
                        return;
                    anchorFlags |= STY_Anchor;
                    PushStyleStack(htmlPtr, Html_EndA, style);
                    htmlPtr->anchorStart = p;
                }
                break;
            case Html_EndA:
                if (htmlPtr->anchorStart) {
                    p->ref.pOther = htmlPtr->anchorStart;
                    style = HtmlPopStyleStack(htmlPtr, Html_EndA);
                    htmlPtr->anchorStart = 0;
                    anchorFlags = 0;
                }
                break;
            case Html_MAP:
                break;
            case Html_EndMAP:
                break;
            case Html_AREA:
                z = HtmlMarkupArg(p, "shape", 0);
                p->area.type = MAP_RECT;
                if (z) {
                    if (!strcasecmp(z, "circle"))
                        p->area.type = MAP_CIRCLE;
                    else if (!strcasecmp(z, "poly"))
                        p->area.type = MAP_POLY;
                }
                z = HtmlMarkupArg(p, "coords", 0);
                if (z) {
                    p->area.coords = GetCoords(z, &p->area.num);
                }
                break;
            case Html_ADDRESS:
            case Html_EndADDRESS:
            case Html_BLOCKQUOTE:
            case Html_EndBLOCKQUOTE:
                paraAlign = ALIGN_None;
                break;
            case Html_APPLET:
                if (htmlPtr->zAppletCommand && *htmlPtr->zAppletCommand) {
                    nextStyle = style;
                    nextStyle.flags |= STY_Invisible;
                    PushStyleStack(htmlPtr, Html_EndAPPLET, nextStyle);
                    useNextStyle = 1;
                }
                else {
                    PushStyleStack(htmlPtr, Html_EndAPPLET, style);
                }
                break;
            case Html_B:
                style.font = BoldFont(FontSize(style.font));
                PushStyleStack(htmlPtr, Html_EndB, style);
                break;
            case Html_EndAPPLET:
            case Html_EndB:
            case Html_EndBIG:
            case Html_EndCENTER:
            case Html_EndCITE:
            case Html_EndCODE:
            case Html_EndCOMMENT:
            case Html_EndEM:
            case Html_EndFONT:
            case Html_EndI:
            case Html_EndKBD:
            case Html_EndMARQUEE:
            case Html_EndNOBR:
            case Html_EndNOFRAMES:
            case Html_EndNOSCRIPT:
            case Html_EndNOEMBED:
            case Html_EndS:
            case Html_EndSAMP:
            case Html_EndSMALL:
            case Html_EndSTRIKE:
            case Html_EndSTRONG:
            case Html_EndSUB:
            case Html_EndSUP:
            case Html_EndTITLE:
            case Html_EndTT:
            case Html_EndU:
            case Html_EndVAR:
                style = HtmlPopStyleStack(htmlPtr, p->base.type);
                break;
            case Html_BASE:
                z = HtmlMarkupArg(p, "href", 0);
                if (z) {
                    HtmlLock(htmlPtr);
                    z = HtmlResolveUri(htmlPtr, z);
                    if (HtmlUnlock(htmlPtr)) {
                        if (z)
                            HtmlFree(z);
                        return;
                    }
                    if (z != 0) {
                        if (htmlPtr->zBaseHref) {
                            HtmlFree(htmlPtr->zBaseHref);
                        }
                        htmlPtr->zBaseHref = z;
                    }
                }
                break;
            case Html_EndDIV:
                paraAlign = ALIGN_None;
                style = HtmlPopStyleStack(htmlPtr, p->base.type);
                break;
            case Html_EndBASEFONT:
                style = HtmlPopStyleStack(htmlPtr, Html_EndBASEFONT);
                style.font = FontFamily(style.font) + 2;
                break;
            case Html_BIG:
                ScaleFont(&style, 1);
                PushStyleStack(htmlPtr, Html_EndBIG, style);
                break;
            case Html_CAPTION:
                paraAlign = GetAlignment(p, paraAlign);
                break;
            case Html_EndCAPTION:
                paraAlign = ALIGN_None;
                break;
            case Html_CENTER:
                paraAlign = ALIGN_None;
                style.align = ALIGN_Center;
                PushStyleStack(htmlPtr, Html_EndCENTER, style);
                break;
            case Html_CITE:
                PushStyleStack(htmlPtr, Html_EndCITE, style);
                break;
            case Html_CODE:
                style.font = CWFont(FontSize(style.font));
                PushStyleStack(htmlPtr, Html_EndCODE, style);
                break;
            case Html_COMMENT:
                style.flags |= STY_Invisible;
                PushStyleStack(htmlPtr, Html_EndCOMMENT, style);
                break;
            case Html_DD:
                if (htmlPtr->innerList
                    && htmlPtr->innerList->base.type == Html_DL) {
                    p->ref.pOther = htmlPtr->innerList;
                }
                else {
                    p->ref.pOther = 0;
                }
                inDt = 0;
                break;
            case Html_DIR:
            case Html_MENU:
            case Html_UL:
                p->list.pPrev = htmlPtr->innerList;
                p->list.cnt = 0;
                htmlPtr->innerList = p;
                if (p->list.pPrev == 0) {
                    p->list.type = LI_TYPE_Bullet1;
                    p->list.compact = HtmlMarkupArg(p, "compact", 0) != 0;
                }
                else if (p->list.pPrev->list.pPrev == 0) {
                    p->list.type = LI_TYPE_Bullet2;
                    p->list.compact = 1;
                }
                else {
                    p->list.type = LI_TYPE_Bullet3;
                    p->list.compact = 1;
                }
                p->list.type = GetUnorderedListType(p, p->list.type);
                break;
            case Html_EndDL:
                inDt = 0;
                /*
                 * Fall thru into the next case 
                 */
            case Html_EndDIR:
            case Html_EndMENU:
            case Html_EndOL:
            case Html_EndUL:
                p->ref.pOther = htmlPtr->innerList;
                if (htmlPtr->innerList) {
                    htmlPtr->innerList = htmlPtr->innerList->list.pPrev;
                }
                else {
                }
                break;
            case Html_DIV:
                paraAlign = ALIGN_None;
                style.align = GetAlignment(p, style.align);
                PushStyleStack(htmlPtr, Html_EndDIV, style);
                break;
            case Html_DT:
                if (htmlPtr->innerList
                    && htmlPtr->innerList->base.type == Html_DL) {
                    p->ref.pOther = htmlPtr->innerList;
                }
                else {
                    p->ref.pOther = 0;
                }
                inDt = STY_DT;
                break;
            case Html_EndDD:
            case Html_EndDT:
                inDt = 0;
                break;
            case Html_DL:
                p->list.pPrev = htmlPtr->innerList;
                p->list.cnt = 0;
                htmlPtr->innerList = p;
                p->list.compact = HtmlMarkupArg(p, "compact", 0) != 0;
                inDt = 0;
                break;
            case Html_EM:
                style.font = ItalicFont(FontSize(style.font));
                PushStyleStack(htmlPtr, Html_EndEM, style);
                break;
            case Html_EMBED:
                break;
            case Html_BASEFONT:
            case Html_FONT:
                z = HtmlMarkupArg(p, "size", 0);
                if (z && (!htmlPtr->overrideFonts)) {
                    if (*z == '-') {
                        size = FontSize(style.font) - atoi(&z[1]) + 1;
                    }
                    else if (*z == '+') {
                        size = FontSize(style.font) + atoi(&z[1]);
                    }
                    else {
                        size = atoi(z);
                    }
                    if (size <= 0) {
                        size = 1;
                    }
                    if (size >= N_FONT_SIZE) {
                        size = N_FONT_SIZE - 1;
                    }
                    style.font = FontFamily(style.font) + size - 1;
                }
                z = HtmlMarkupArg(p, "color", 0);
                if (z && z[0] && (!htmlPtr->overrideColors)) {
                    style.color = HtmlGetColorByName(htmlPtr, z, style.color);
                }
                PushStyleStack(htmlPtr,
                               p->base.type ==
                               Html_FONT ? Html_EndFONT : Html_EndBASEFONT,
                               style);
                break;
            case Html_FORM:{
                    char *zUrl;
                    char *zMethod;
                    Tcl_DString cmd;   /* -formcommand callback */
                    int result;
                    char zToken[50];

                    htmlPtr->formStart = 0;

/*        p->form.id = 0; */
                    if (htmlPtr->zFormCommand == 0
                        || htmlPtr->zFormCommand[0] == 0) {
                        TestPoint(0);
                        break;
                    }
                    zUrl = HtmlMarkupArg(p, "action", 0);
                    if (zUrl == 0) {
                        TestPoint(0);
                        /*
                         * break; 
                         */
                        zUrl = htmlPtr->zBase;
                    }
                    HtmlLock(htmlPtr);
                    zUrl = HtmlResolveUri(htmlPtr, zUrl);
                    if (HtmlUnlock(htmlPtr)) {
                        if (zUrl)
                            HtmlFree(zUrl);
                        return;
                    }
                    if (zUrl == 0)
                        zUrl = strdup("");
                    zMethod = HtmlMarkupArg(p, "method", "GET");
                    sprintf(zToken, " %d form {} ", p->form.id);
                    Tcl_DStringInit(&cmd);
                    Tcl_DStringAppend(&cmd, htmlPtr->zFormCommand, -1);
                    Tcl_DStringAppend(&cmd, zToken, -1);
                    Tcl_DStringAppendElement(&cmd, zUrl);
                    HtmlFree(zUrl);
                    Tcl_DStringAppendElement(&cmd, zMethod);
                    Tcl_DStringStartSublist(&cmd);
                    HtmlAppendArglist(&cmd, p);
                    Tcl_DStringEndSublist(&cmd);
                    HtmlLock(htmlPtr);
                    htmlPtr->inParse++;
                    result = Tcl_GlobalEval(htmlPtr->interp,
                                            Tcl_DStringValue(&cmd));
                    htmlPtr->inParse--;
                    Tcl_DStringFree(&cmd);
                    if (HtmlUnlock(htmlPtr))
                        return;
                    if (result == TCL_OK) {
                        htmlPtr->formStart = p;
                    }
                    else {
                        Tcl_AddErrorInfo(htmlPtr->interp,
                                         "\n (\"-formcommand\" command executed by html widget)");
                        Tcl_BackgroundError(htmlPtr->interp);
                    }
                    Tcl_ResetResult(htmlPtr->interp);
                    break;
                }
            case Html_EndFORM:
                p->ref.pOther = htmlPtr->formStart;
                if (htmlPtr->formStart)
                    htmlPtr->formStart->form.pEnd = p;
                htmlPtr->formStart = 0;
                break;
            case Html_H1:
            case Html_H2:
            case Html_H3:
            case Html_H4:
            case Html_H5:
            case Html_H6:
                paraAlign = ALIGN_None;
                i = (p->base.type - Html_H1) / 2 + 1;
                if (i >= 1 && i <= 6) {
                    ScaleFont(&style, header_sizes[i - 1]);
                }
                style.font = BoldFont(FontSize(style.font));
                style.align = GetAlignment(p, style.align);
                PushStyleStack(htmlPtr, Html_EndH1, style);
                break;
            case Html_EndH1:
            case Html_EndH2:
            case Html_EndH3:
            case Html_EndH4:
            case Html_EndH5:
            case Html_EndH6:
                paraAlign = ALIGN_None;
                style = HtmlPopStyleStack(htmlPtr, Html_EndH1);
                break;
            case Html_HR:
                nextStyle = style;
                style.align = GetAlignment(p, ALIGN_None);
                useNextStyle = 1;
                break;
            case Html_I:
                style.font = ItalicFont(FontSize(style.font));
                PushStyleStack(htmlPtr, Html_EndI, style);
                break;
            case Html_IMG:
                if (style.flags & STY_Invisible)
                    break;
                HtmlLock(htmlPtr);
                p->image.pImage = HtmlGetImage(htmlPtr, p);
                if (HtmlUnlock(htmlPtr))
                    return;
                break;
            case Html_OPTION:
                break;
            case Html_INPUT:
                p->input.pForm = htmlPtr->formStart;
                if (htmlPtr->TclHtml)
                    HtmlControlSize(htmlPtr, p);
                break;
            case Html_KBD:
                style.font = CWFont(FontSize(style.font));
                PushStyleStack(htmlPtr, Html_EndKBD, style);
                break;
            case Html_LI:
                if (htmlPtr->innerList) {
                    p->li.type = htmlPtr->innerList->list.type;
                    if (htmlPtr->innerList->base.type == Html_OL) {
                        z = HtmlMarkupArg(p, "value", 0);
                        if (z) {
                            int n = atoi(z);
                            if (n > 0) {
                                p->li.cnt = n;
                                htmlPtr->innerList->list.cnt = n + 1;
                            }
                            else {
                            }
                        }
                        else {
                            p->li.cnt = htmlPtr->innerList->list.cnt++;
                        }
                        p->li.type = GetOrderedListType(p, p->li.type);
                    }
                    else {
                        p->li.type = GetUnorderedListType(p, p->li.type);
                    }
                }
                else {
                    p->base.flags &= ~HTML_Visible;
                }
                break;
            case Html_MARQUEE:
                style.flags |= STY_Invisible;
                PushStyleStack(htmlPtr, Html_EndMARQUEE, style);
                break;
            case Html_NOBR:
                style.flags |= STY_NoBreak;
                PushStyleStack(htmlPtr, Html_EndNOBR, style);
                break;
            case Html_NOFRAMES:
                if (htmlPtr->zFrameCommand && *htmlPtr->zFrameCommand) {
                    nextStyle = style;
                    nextStyle.flags |= STY_Invisible;
                    PushStyleStack(htmlPtr, Html_EndNOFRAMES, nextStyle);
                    useNextStyle = 1;
                }
                else {
                    PushStyleStack(htmlPtr, Html_EndNOFRAMES, style);
                }
                break;
            case Html_NOEMBED:
                if (htmlPtr->zScriptCommand && *htmlPtr->zScriptCommand
                    && htmlPtr->HasScript) {
                    nextStyle = style;
                    nextStyle.flags |= STY_Invisible;
                    PushStyleStack(htmlPtr, Html_EndNOEMBED, nextStyle);
                    useNextStyle = 1;
                }
                else {
                    PushStyleStack(htmlPtr, Html_EndNOEMBED, style);
                }
                break;
            case Html_NOSCRIPT:
                if (htmlPtr->zScriptCommand && *htmlPtr->zScriptCommand
                    && htmlPtr->HasScript) {
                    nextStyle = style;
                    nextStyle.flags |= STY_Invisible;
                    PushStyleStack(htmlPtr, Html_EndNOSCRIPT, nextStyle);
                    useNextStyle = 1;
                }
                else {
                    PushStyleStack(htmlPtr, Html_EndNOSCRIPT, style);
                }
                break;
            case Html_OL:
                p->list.pPrev = htmlPtr->innerList;
                p->list.type = GetOrderedListType(p, LI_TYPE_Enum_1);
                p->list.cnt = 1;
                z = HtmlMarkupArg(p, "start", 0);
                if (z) {
                    int n = atoi(z);
                    if (n > 0) {
                        p->list.cnt = n;
                    }
                    else {
                    }
                }
                else {
                }
                p->list.compact = htmlPtr->innerList != 0 ||
                        HtmlMarkupArg(p, "compact", 0) != 0;
                htmlPtr->innerList = p;
                break;
            case Html_P:
                paraAlign = GetAlignment(p, ALIGN_None);
                break;
            case Html_EndP:
                paraAlign = ALIGN_None;
                break;
            case Html_PRE:
            case Html_LISTING:
            case Html_XMP:
            case Html_PLAINTEXT:
                paraAlign = ALIGN_None;
                style.font = CWFont(FontSize(style.font));
                style.flags |= STY_Preformatted;
                PushStyleStack(htmlPtr, Html_EndPRE, style);
                break;
            case Html_EndPRE:
            case Html_EndLISTING:
            case Html_EndXMP:
                style = HtmlPopStyleStack(htmlPtr, Html_EndPRE);
                break;
            case Html_S:
                style.flags |= STY_StrikeThru;
                PushStyleStack(htmlPtr, Html_EndS, style);
                break;
            case Html_SCRIPT:
#if 0
                if (htmlPtr->zScriptCommand && *htmlPtr->zScriptCommand) {
                    Tcl_DString cmd;
                    char *resstr;
                    int result, reslen;
                    Tcl_DStringInit(&cmd);
                    Tcl_DStringAppend(&cmd, htmlPtr->zScriptCommand, -1);
                    Tcl_DStringStartSublist(&cmd);
                    HtmlAppendArglist(&cmd, p);
                    Tcl_DStringEndSublist(&cmd);
                    Tcl_DStringStartSublist(&cmd);
                    Tcl_DStringAppend(&cmd, p->script.zScript,
                                      p->script.nScript);
                    Tcl_DStringEndSublist(&cmd);
                    HtmlLock(htmlPtr);
                    htmlPtr->inParse++;
                    result = Tcl_GlobalEval(htmlPtr->interp,
                                            Tcl_DStringValue(&cmd));
                    htmlPtr->inParse--;
                    Tcl_DStringFree(&cmd);
                    if (HtmlUnlock(htmlPtr))
                        return;
                    resstr = Tcl_GetByteArrayObj(Tcl_GetObjResult
                                                 (htmlPtr->interp->result),
                                                 &reslen);
                    if (result == 0 && resstr && reslen) {
                        HtmlElement *b2 = p->pNext, *b3, *ps, *e1 = p, *e2 =
                                b2, *e3;
                        if (e2)
                            while (e2->pNext)
                                e2 = e2->pNext;
                        HtmlTokenizerAppend(htmlPtr, resstr, reslen);
                        if (e2 && e2 != p && ((e3 = b3 = e2->pNext))) {
                            while (e3->pNext)
                                e3 = e3->pNext;
                            e1->pNext = b3;
                            e2->pNext = 0;
                            b2->base.pPrev = e3;
                            e3->pNext = b2;
                            b3->base.pPrev = e1;
                        }
                    }
                    Tcl_ResetResult(htmlPtr->interp);
                }
#endif
                nextStyle = style;
                style.flags |= STY_Invisible;
                useNextStyle = 1;
                break;
            case Html_SELECT:
                p->input.pForm = htmlPtr->formStart;
                nextStyle.flags |= STY_Invisible;
                useNextStyle = 1;
                PushStyleStack(htmlPtr, Html_EndSELECT, style);
                htmlPtr->formElemStart = p;
                break;
            case Html_EndSELECT:
                style = HtmlPopStyleStack(htmlPtr, Html_EndSELECT);
                if (htmlPtr->formElemStart
                    && htmlPtr->formElemStart->base.type == Html_SELECT) {
                    p->ref.pOther = htmlPtr->formElemStart;
                    MakeInvisible(p->ref.pOther, p);
                }
                else {
                    p->ref.pOther = 0;
                }
                htmlPtr->formElemStart = 0;
                break;
            case Html_STRIKE:
                style.flags |= STY_StrikeThru;
                PushStyleStack(htmlPtr, Html_EndSTRIKE, style);
                break;
            case Html_STYLE:
                /*
                 * Ignore style sheets 
                 */
                break;
            case Html_SAMP:
                style.font = CWFont(FontSize(style.font));
                PushStyleStack(htmlPtr, Html_EndSAMP, style);
                break;
            case Html_SMALL:
                ScaleFont(&style, -1);
                PushStyleStack(htmlPtr, Html_EndSMALL, style);
                break;
            case Html_STRONG:
                style.font = BoldFont(FontSize(style.font));
                PushStyleStack(htmlPtr, Html_EndSTRONG, style);
                break;
            case Html_SUB:
                ScaleFont(&style, -1);
                if (style.subscript > -6) {
                    style.subscript--;
                }
                else {
                }
                PushStyleStack(htmlPtr, Html_EndSUB, style);
                break;
            case Html_SUP:
                ScaleFont(&style, -1);
                if (style.subscript < 6) {
                    style.subscript++;
                }
                else {
                }
                PushStyleStack(htmlPtr, Html_EndSUP, style);
                break;
            case Html_TABLE:
                if (p->table.tktable) {
                    if (p->table.pEnd =
                        HtmlFindEndNest(htmlPtr, p, Html_EndTABLE, 0))
                        MakeInvisible(p, p->table.pEnd);
                    if (p->table.pEnd) {
                        p = p->table.pEnd;
                        break;
                    }
                }
                paraAlign = ALIGN_None;
                nextStyle = style;
                if (style.flags & STY_Preformatted) {
                    nextStyle.flags &= ~STY_Preformatted;
                    style.flags |= STY_Preformatted;
                }
                nextStyle.align = ALIGN_Left;
                z = HtmlMarkupArg(p, "bgcolor", 0);
                if (z && z[0] && (!htmlPtr->overrideColors)) {
                    style.bgcolor = nextStyle.bgcolor =
                            HtmlGetColorByName(htmlPtr, z, style.bgcolor);
                    style.expbg = 1;

/*        }else{
          nextStyle.bgcolor = COLOR_Background; */
                }
                HtmlTableBgImage(htmlPtr, p);
                PushStyleStack(htmlPtr, Html_EndTABLE, nextStyle);
                useNextStyle = 1;
                htmlPtr->inTd = 0;
                htmlPtr->inTr = 0;
                break;
            case Html_EndTABLE:
                paraAlign = ALIGN_None;
                if (htmlPtr->inTd) {
                    style = HtmlPopStyleStack(htmlPtr, Html_EndTD);
                    htmlPtr->inTd = 0;
                }
                if (htmlPtr->inTr) {
                    style = HtmlPopStyleStack(htmlPtr, Html_EndTR);
                    htmlPtr->inTr = 0;
                }
                style = HtmlPopStyleStack(htmlPtr, p->base.type);
                break;
            case Html_TD:
                if (htmlPtr->inTd) {
                    style = HtmlPopStyleStack(htmlPtr, Html_EndTD);
                }
                htmlPtr->inTd = 1;
                paraAlign = GetAlignment(p, rowAlign);
                if ((z = HtmlMarkupArg(p, "bgcolor", 0)) != 0 && z[0]
                    && (!htmlPtr->overrideColors)) {
                    style.bgcolor =
                            HtmlGetColorByName(htmlPtr, z, style.bgcolor);
                    style.expbg = 1;
                }
                HtmlTableBgImage(htmlPtr, p);
                PushStyleStack(htmlPtr, Html_EndTD, style);
                break;
            case Html_TEXTAREA:
                p->input.pForm = htmlPtr->formStart;
                nextStyle = style;
                nextStyle.flags |= STY_Invisible;
                PushStyleStack(htmlPtr, Html_EndTEXTAREA, nextStyle);
                htmlPtr->formElemStart = p;
                useNextStyle = 1;
                break;
            case Html_EndTEXTAREA:
                style = HtmlPopStyleStack(htmlPtr, Html_EndTEXTAREA);
                if (htmlPtr->formElemStart
                    && htmlPtr->formElemStart->base.type == Html_TEXTAREA) {
                    p->ref.pOther = htmlPtr->formElemStart;
                }
                else {
                    p->ref.pOther = 0;
                }
                htmlPtr->formElemStart = 0;
                break;
            case Html_TH:
                /*
                 * paraAlign = GetAlignment(p, rowAlign); 
                 */
                if (htmlPtr->inTd) {
                    style = HtmlPopStyleStack(htmlPtr, Html_EndTD);
                }
                paraAlign = GetAlignment(p, ALIGN_Center);
                style.font = BoldFont(FontSize(style.font));
                if ((z = HtmlMarkupArg(p, "bgcolor", 0)) != 0 && z[0]) {
                    style.bgcolor =
                            HtmlGetColorByName(htmlPtr, z, style.bgcolor);
                    style.expbg = 1;
                }
                HtmlTableBgImage(htmlPtr, p);
                PushStyleStack(htmlPtr, Html_EndTD, style);
                htmlPtr->inTd = 1;
                break;
            case Html_TR:
                if (htmlPtr->inTd) {
                    style = HtmlPopStyleStack(htmlPtr, Html_EndTD);
                    htmlPtr->inTd = 0;
                }
                if (htmlPtr->inTr) {
                    style = HtmlPopStyleStack(htmlPtr, Html_EndTR);
                }
                rowAlign = GetAlignment(p, ALIGN_None);
                if ((z = HtmlMarkupArg(p, "bgcolor", 0)) != 0 && z[0]
                    && (!htmlPtr->overrideColors)) {
                    style.bgcolor =
                            HtmlGetColorByName(htmlPtr, z, style.bgcolor);
                    style.expbg = 1;
                }
                HtmlTableBgImage(htmlPtr, p);
                PushStyleStack(htmlPtr, Html_EndTR, style);
                htmlPtr->inTr = 1;
                break;
            case Html_EndTR:
                if (htmlPtr->inTd) {
                    style = HtmlPopStyleStack(htmlPtr, Html_EndTD);
                    htmlPtr->inTd = 0;
                }
                style = HtmlPopStyleStack(htmlPtr, Html_EndTR);
                htmlPtr->inTr = 0;
                paraAlign = ALIGN_None;
                rowAlign = ALIGN_None;
                break;
            case Html_EndTD:
            case Html_EndTH:
                style = HtmlPopStyleStack(htmlPtr, Html_EndTD);
                htmlPtr->inTd = 0;
                paraAlign = ALIGN_None;
                rowAlign = ALIGN_None;
                break;
            case Html_TITLE:
                style.flags |= STY_Invisible;
                PushStyleStack(htmlPtr, Html_EndTITLE, style);
                break;
            case Html_TT:
                style.font = CWFont(FontSize(style.font));
                PushStyleStack(htmlPtr, Html_EndTT, style);
                break;
            case Html_U:
                style.flags |= STY_Underline;
                PushStyleStack(htmlPtr, Html_EndU, style);
                break;
            case Html_VAR:
                style.font = ItalicFont(FontSize(style.font));
                PushStyleStack(htmlPtr, Html_EndVAR, style);
                break;
            default:
                break;
        }
        p->base.style = style;
        p->base.style.flags |= anchorFlags | inDt;
        if (paraAlign != ALIGN_None) {
            p->base.style.align = paraAlign;
        }
        if (useNextStyle) {
            style = nextStyle;
            style.expbg = 0;
            useNextStyle = 0;
        }
        TRACE(HtmlTrace_Style,
              ("Style of 0x%08x font=%02d color=%02d bg=%02d "
               "align=%d flags=0x%04x token=%s\n",
               (int) p, p->base.style.font, p->base.style.color,
               p->base.style.bgcolor, p->base.style.align, p->base.style.flags,
               HtmlTokenName(htmlPtr, p)));
        p = p->pNext;
    }

    /*
     * Copy state information back into the htmlPtr structure for ** safe
     * keeping. 
     */
    htmlPtr->paraAlignment = paraAlign;
    htmlPtr->rowAlignment = rowAlign;
    htmlPtr->anchorFlags = anchorFlags;
    htmlPtr->inDt = inDt;
    htmlPtr->flags &= ~STYLER_RUNNING;
}

void
HtmlTableBgImage(htmlPtr, p)
    HtmlWidget *htmlPtr;
    HtmlElement *p;
{
#ifndef _TCLHTML_
    Tcl_DString cmd;
    int result;
    char *z, buf[30];
    if (htmlPtr->TclHtml)
        return;
    if ((!htmlPtr->zGetBGImage) || (!*htmlPtr->zGetBGImage))
        return;
    if (!(z = HtmlMarkupArg(p, "background", 0)))
        return;
    Tcl_DStringInit(&cmd);
    Tcl_DStringAppend(&cmd, htmlPtr->zGetBGImage, -1);
    Tcl_DStringAppend(&cmd, " ", 1);
    Tcl_DStringAppend(&cmd, z, -1);
    sprintf(buf, " %d", p->base.id);
    Tcl_DStringAppend(&cmd, buf, -1);
    HtmlLock(htmlPtr);
    htmlPtr->inParse++;
    result = Tcl_GlobalEval(htmlPtr->interp, Tcl_DStringValue(&cmd));
    htmlPtr->inParse--;
    Tcl_DStringFree(&cmd);
    if (HtmlUnlock(htmlPtr))
        return;
    if (result == TCL_OK)
        HtmlSetImageBg(htmlPtr, htmlPtr->interp, htmlPtr->interp->result, p);
    Tcl_ResetResult(htmlPtr->interp);
#endif
}

/*
** Compute the size of all elements in the widget.  Assume that a
** style has already been assigned to all elements.
**
** Some of the elements might have already been sized.  Refer to the
** htmlPtr->lastSized and only compute sizes for elements that follow
** this one.  If htmlPtr->lastSized==0, then size everything.
**
** This routine only computes the sizes of individual elements.  The
** size of aggregate elements (like tables) are computed separately.
**
** The HTML_Visible flag is also set on every element that results 
** in ink on the page.
**
** This routine may invoke a callback procedure which could delete
** the HTML widget. 
*/
void
HtmlSizer(htmlPtr)
    HtmlWidget *htmlPtr;
{
    HtmlElement *p;
    int iFont = -1;
    Tk_Font font = 0;
    int spaceWidth = 0;
    Tk_FontMetrics fontMetrics;
    char *z;
    int stop = 0;

    if (htmlPtr->pFirst == 0) {
        return;
    }
    if (htmlPtr->lastSized == 0) {
        p = htmlPtr->pFirst;
    }
    else {
        p = htmlPtr->lastSized->pNext;
    }
    for (; !stop && p; p = p ? p->pNext : 0) {
        if (p->base.style.flags & STY_Invisible) {
            p->base.flags &= ~HTML_Visible;
            continue;
        }
        if (iFont != p->base.style.font) {
            iFont = p->base.style.font;
            HtmlLock(htmlPtr);
#ifndef _TCLHTML_
            font = HtmlGetFont(htmlPtr, iFont);
            if (HtmlUnlock(htmlPtr))
                break;
            Tk_GetFontMetrics(font, &fontMetrics);
#else
            fontMetrics.descent = 1;
            fontMetrics.ascent = 9;
#endif
            spaceWidth = 0;
        }
        switch (p->base.type) {
            case Html_Text:
#ifndef _TCLHTML_
                p->text.w = Tk_TextWidth(font, p->text.zText, p->base.count);
                p->base.flags |= HTML_Visible;
                p->text.descent = fontMetrics.descent;
                p->text.ascent = fontMetrics.ascent;
                if (spaceWidth == 0) {
                    spaceWidth = Tk_TextWidth(font, " ", 1);
                }
                else {
                }
                p->text.spaceWidth = spaceWidth;
#else
                p->text.w = 10;
                p->base.flags |= HTML_Visible;
                p->text.descent = 1;
                p->text.ascent = 9;
                if (spaceWidth == 0) {
                    spaceWidth = 10;
                    TestPoint(0);
                }
                else {
                    TestPoint(0);
                }
                p->text.spaceWidth = spaceWidth;
#endif
                break;
            case Html_Space:
                if (spaceWidth == 0) {
#ifndef _TCLHTML_
                    spaceWidth = Tk_TextWidth(font, " ", 1);
#else
                    spaceWidth = 10;
#endif
                }
                p->space.w = spaceWidth;
                p->space.descent = fontMetrics.descent;
                p->space.ascent = fontMetrics.ascent;
                p->base.flags &= ~HTML_Visible;
                break;
            case Html_TD:
            case Html_TH:
                z = HtmlMarkupArg(p, "rowspan", "1");
                p->cell.rowspan = atoi(z);
                z = HtmlMarkupArg(p, "colspan", "1");
                p->cell.colspan = atoi(z);
                p->base.flags |= HTML_Visible;
                break;
            case Html_LI:
                p->li.descent = fontMetrics.descent;
                p->li.ascent = fontMetrics.ascent;
                p->base.flags |= HTML_Visible;
                break;
            case Html_IMG:
#ifndef _TCLHTML_
                z = HtmlMarkupArg(p, "usemap", 0);
                if (z && *z == '#') {
                    p->image.pMap = HtmlGetMap(htmlPtr, z + 1);
                }
                else
                    p->image.pMap = 0;
                p->base.flags |= HTML_Visible;
                p->image.redrawNeeded = 0;
                p->image.textAscent = fontMetrics.ascent;
                p->image.textDescent = fontMetrics.descent;
                p->image.align = HtmlGetImageAlignment(p);
                if (p->image.pImage == 0) {
                    p->image.ascent = fontMetrics.ascent;
                    p->image.descent = fontMetrics.descent;
                    p->image.zAlt = HtmlMarkupArg(p, "alt", "<image>");
                    p->image.w =
                            Tk_TextWidth(font, p->image.zAlt,
                                         strlen(p->image.zAlt));
                }
                else {
                    int w, h;
                    p->image.pNext = p->image.pImage->pList;
                    p->image.pImage->pList = p;
                    Tk_SizeOfImage(p->image.pImage->image, &w, &h);
                    p->image.h = h;
                    p->image.w = w;
                    p->image.ascent = h / 2;
                    p->image.descent = h - p->image.ascent;
                }
                if ((z = HtmlMarkupArg(p, "width", 0)) != 0) {
                    int w = atoi(z);
                    if (w > 0)
                        p->image.w = w;
                }
                if ((z = HtmlMarkupArg(p, "height", 0)) != 0) {
                    int h = atoi(z);
                    if (h > 0)
                        p->image.h = h;
                }
#endif /* _TCLHTML_ */
                break;
            case Html_TABLE:
                if (p->table.tktable) {
                    p = p->table.pEnd;
                    break;
                }
            case Html_HR:
                p->base.flags |= HTML_Visible;
                break;
            case Html_APPLET:
            case Html_EMBED:
            case Html_INPUT:
                p->input.textAscent = fontMetrics.ascent;
                p->input.textDescent = fontMetrics.descent;
                stop = HtmlControlSize(htmlPtr, p);
                break;
            case Html_SELECT:
            case Html_TEXTAREA:
                p->input.textAscent = fontMetrics.ascent;
                p->input.textDescent = fontMetrics.descent;
                break;
            case Html_EndSELECT:
            case Html_EndTEXTAREA:
                if (p->ref.pOther) {
                    p->ref.pOther->input.pEnd = p;
                    stop = HtmlControlSize(htmlPtr, p->ref.pOther);
                }
                break;
            default:
                p->base.flags &= ~HTML_Visible;
                break;
        }
    }
    if (p) {
        htmlPtr->lastSized = p;
    }
    else {
        htmlPtr->lastSized = htmlPtr->pLast;
    }
}
