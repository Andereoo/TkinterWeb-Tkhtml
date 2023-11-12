static char const rcsid[] =
        "@(#) $Id: htmlwidget.c,v 1.59 2005/03/23 23:56:27 danielk1977 Exp $";

/*
** The main routine for the HTML widget for Tcl/Tk
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "html.h"
#ifdef USE_TK_STUBS
# include <tkIntXlibDecls.h>
#endif
#include <X11/Xatom.h>

/*
** This global variable is used for tracing the operation of
** the Html formatter.
*/
int HtmlTraceMask = 0;

#ifdef __WIN32__
# define DEF_FRAME_BG_COLOR        "SystemButtonFace"
# define DEF_FRAME_BG_MONO         "White"
# define DEF_FRAME_CURSOR          ""
# define DEF_BUTTON_FG             "SystemButtonText"
# define DEF_BUTTON_HIGHLIGHT_BG   "SystemButtonFace"
# define DEF_BUTTON_HIGHLIGHT      "SystemWindowFrame"
#else
# define DEF_FRAME_BG_COLOR        "#d9d9d9"
# define DEF_FRAME_BG_MONO         "White"
# define DEF_FRAME_CURSOR          ""
# define DEF_BUTTON_FG             "Black"
# define DEF_BUTTON_HIGHLIGHT_BG   "#d9d9d9"
# define DEF_BUTTON_HIGHLIGHT      "Black"
#endif

/*
** Information used for argv parsing.
*/
static Tk_ConfigSpec configSpecs[] = {
#if !defined(_TCLHTML_)
    {TK_CONFIG_BORDER, "-background", "background", "Background",
     DEF_HTML_BG_COLOR, Tk_Offset(HtmlWidget, border),
     TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
     DEF_HTML_BG_MONO, Tk_Offset(HtmlWidget, border),
     TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
     (char *) NULL, 0, 0},
    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
     (char *) NULL, 0, 0},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
     DEF_HTML_BORDER_WIDTH, Tk_Offset(HtmlWidget, borderWidth), 0},
    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
     DEF_HTML_CURSOR, Tk_Offset(HtmlWidget, cursor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_BOOLEAN, "-exportselection", "exportSelection",
     "ExportSelection",
     DEF_HTML_EXPORT_SEL, Tk_Offset(HtmlWidget, exportSelection), 0},
    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
     (char *) NULL, 0, 0},
    {TK_CONFIG_STRING, "-fontcommand", "fontCommand", "FontCommand",
     DEF_HTML_CALLBACK, Tk_Offset(HtmlWidget, zFontCommand), 0},
    {TK_CONFIG_STRING, "-fontfamily", "fontFamily", "FontFamily",
     "times", Tk_Offset(HtmlWidget, FontFamily), 0},
    {TK_CONFIG_INT, "-fontadjust", "fontAdjust", "FontAdjust",
     "2", Tk_Offset(HtmlWidget, FontAdjust), 0},
    {TK_CONFIG_INT, "-formpadding", "formPadding", "FormPadding",
     "4", Tk_Offset(HtmlWidget, formPadding), 0},
    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
     DEF_HTML_FG, Tk_Offset(HtmlWidget, fgColor), 0},
    {TK_CONFIG_STRING, "-imgidxcommand", "imgidxCommand", "HtmlCallback",
     DEF_HTML_CALLBACK, Tk_Offset(HtmlWidget, zImgIdxCommand), 0},
    {TK_CONFIG_PIXELS, "-height", "height", "Hidth",
     DEF_HTML_HEIGHT, Tk_Offset(HtmlWidget, height), 0},
    {TK_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
     "HighlightBackground", DEF_HTML_HIGHLIGHT_BG,
     Tk_Offset(HtmlWidget, highlightBgColorPtr), 0},
    {TK_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
     DEF_HTML_HIGHLIGHT, Tk_Offset(HtmlWidget, highlightColorPtr), 0},
    {TK_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
     "HighlightThickness",
     DEF_HTML_HIGHLIGHT_WIDTH, Tk_Offset(HtmlWidget, highlightWidth), 0},
    {TK_CONFIG_STRING, "-hyperlinkcommand", "hyperlinkCommand", "HtmlCallback",
     DEF_HTML_CALLBACK, Tk_Offset(HtmlWidget, zHyperlinkCommand), 0},
    {TK_CONFIG_STRING, "-imagecommand", "imageCommand", "HtmlCallback",
     DEF_HTML_CALLBACK, Tk_Offset(HtmlWidget, zGetImage), 0},
    {TK_CONFIG_STRING, "-bgimagecommand", "BGimageCommand", "HtmlCallback",
     DEF_HTML_CALLBACK, Tk_Offset(HtmlWidget, zGetBGImage), 0},
    {TK_CONFIG_INT, "-insertofftime", "insertOffTime", "OffTime",
     DEF_HTML_INSERT_OFF_TIME, Tk_Offset(HtmlWidget, insOffTime), 0},
    {TK_CONFIG_INT, "-insertontime", "insertOnTime", "OnTime",
     DEF_HTML_INSERT_ON_TIME, Tk_Offset(HtmlWidget, insOnTime), 0},
    {TK_CONFIG_STRING, "-isvisitedcommand", "isVisitedCommand", "HtmlCallback",
     DEF_HTML_CALLBACK, Tk_Offset(HtmlWidget, zIsVisited), 0},
    {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
     DEF_HTML_PADX, Tk_Offset(HtmlWidget, padx), 0},
    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
     DEF_HTML_PADY, Tk_Offset(HtmlWidget, pady), 0},
    {TK_CONFIG_PIXELS, "-leftmargin", "leftmargin", "Margin",
     DEF_HTML_PADY, Tk_Offset(HtmlWidget, leftmargin), 0},
    {TK_CONFIG_PIXELS, "-topmargin", "topmargin", "Margin",
     DEF_HTML_PADY, Tk_Offset(HtmlWidget, topmargin), 0},
    {TK_CONFIG_PIXELS, "-marginwidth", "marginwidth", "Margin",
     DEF_HTML_PADY, Tk_Offset(HtmlWidget, marginwidth), 0},
    {TK_CONFIG_PIXELS, "-marginheight", "marginheight", "Margin",
     DEF_HTML_PADY, Tk_Offset(HtmlWidget, marginheight), 0},
    {TK_CONFIG_BOOLEAN, "-overridecolors", "overrideColors",
     "OverrideColors", "0", Tk_Offset(HtmlWidget, overrideColors), 0},
    {TK_CONFIG_BOOLEAN, "-overridefonts", "overrideFonts",
     "OverrideFonts", "0", Tk_Offset(HtmlWidget, overrideFonts), 0},
    {TK_CONFIG_RELIEF, "-relief", "relief", "Relief",
     DEF_HTML_RELIEF, Tk_Offset(HtmlWidget, relief), 0},
    {TK_CONFIG_RELIEF, "-rulerelief", "ruleRelief", "RuleRelief",
     "sunken", Tk_Offset(HtmlWidget, ruleRelief), 0},
    {TK_CONFIG_COLOR, "-selectioncolor", "background", "Background",
     DEF_HTML_SELECTION_COLOR, Tk_Offset(HtmlWidget, selectionColor), 0},
    {TK_CONFIG_RELIEF, "-tablerelief", "tableRelief", "TableRelief",
     "raised", Tk_Offset(HtmlWidget, tableRelief), 0},
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
     DEF_HTML_TAKE_FOCUS, Tk_Offset(HtmlWidget, takeFocus),
     TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-unvisitedcolor", "foreground", "Foreground",
     DEF_HTML_UNVISITED, Tk_Offset(HtmlWidget, newLinkColor), 0},
    {TK_CONFIG_BOOLEAN, "-underlinehyperlinks", "underlineHyperlinks",
     "UnderlineHyperlinks", "1", Tk_Offset(HtmlWidget, underlineLinks), 0},
    {TK_CONFIG_COLOR, "-visitedcolor", "foreground", "Foreground",
     DEF_HTML_VISITED, Tk_Offset(HtmlWidget, oldLinkColor), 0},
    {TK_CONFIG_PIXELS, "-width", "width", "Width",
     DEF_HTML_WIDTH, Tk_Offset(HtmlWidget, width), 0},
    {TK_CONFIG_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
     DEF_HTML_SCROLL_COMMAND, Tk_Offset(HtmlWidget, xScrollCmd),
     TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
     DEF_HTML_SCROLL_COMMAND, Tk_Offset(HtmlWidget, yScrollCmd),
     TK_CONFIG_NULL_OK},
#endif
    {TK_CONFIG_STRING, "-appletcommand", "appletCommand", "HtmlCallback",
     DEF_HTML_CALLBACK, Tk_Offset(HtmlWidget, zAppletCommand), 0},
    {TK_CONFIG_STRING, "-base", "base", "Base",
     "", Tk_Offset(HtmlWidget, zBase), 0},
    {TK_CONFIG_STRING, "-scriptcommand", "scriptCommand", "HtmlCallback",
     "", Tk_Offset(HtmlWidget, zScriptCommand), 0},
    {TK_CONFIG_STRING, "-formcommand", "formCommand", "HtmlCallback",
     DEF_HTML_CALLBACK, Tk_Offset(HtmlWidget, zFormCommand), 0},
    {TK_CONFIG_STRING, "-framecommand", "frameCommand", "HtmlCallback",
     DEF_HTML_CALLBACK, Tk_Offset(HtmlWidget, zFrameCommand), 0},
    {TK_CONFIG_INT, "-addendtags", "addendtags", "bool",
     0, Tk_Offset(HtmlWidget, AddEndTags), 0},
    {TK_CONFIG_INT, "-tableborder", "tableborder", "int",
     0, Tk_Offset(HtmlWidget, TableBorderMin), 0},
    {TK_CONFIG_INT, "-hasscript", "hasscript", "bool",
     0, Tk_Offset(HtmlWidget, HasScript), 0},
    {TK_CONFIG_INT, "-hasframes", "hasframes", "bool",
     0, Tk_Offset(HtmlWidget, HasFrames), 0},
    {TK_CONFIG_INT, "-hastktables", "hastktables", "bool",
     0, Tk_Offset(HtmlWidget, HasTktables), 0},
    {TK_CONFIG_INT, "-tclhtml", "tclhtml", "bool",
     0, Tk_Offset(HtmlWidget, TclHtml), 0},
    {TK_CONFIG_STRING, "-resolvercommand", "resolverCommand", "HtmlCallback",
     DEF_HTML_CALLBACK, Tk_Offset(HtmlWidget, zResolverCommand), 0},
    {TK_CONFIG_BOOLEAN, "-sentencepadding", "sentencePadding",
     "SentencePadding", "0", Tk_Offset(HtmlWidget, iSentencePadding), 0},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
     (char *) NULL, 0, 0}
};

/*
** Get a copy of the config specs.
*/
Tk_ConfigSpec *
HtmlConfigSpec()
{
    return configSpecs;
}

int
TclConfigureWidgetObj(interp, htmlPtr, configSpecs, objc, objv, dp, flags)
    Tcl_Interp *interp;
    HtmlWidget *htmlPtr;
    Tk_ConfigSpec *configSpecs;
    int objc;
    Tcl_Obj *const *objv;
    char *dp;
    int flags;
{
    Tk_ConfigSpec *cs;
    int i;
    char *op;
    if (objc == 0) {
        cs = configSpecs;
        while (cs->type != TK_CONFIG_END) {
            switch (cs->type) {
                case TK_CONFIG_STRING:{
                        char **sp;
                        op = dp + cs->offset;
                        sp = (char **) op;
                        Tcl_AppendElement(interp, cs->argvName);
                        Tcl_AppendElement(interp, *sp);
                    }
                    break;
                case TK_CONFIG_INT:{
                        int *sp;
                        char buf[50];
                        op = dp + cs->offset;
                        sp = (int *) op;
                        sprintf(buf, "%d", *sp);
                        Tcl_AppendElement(interp, cs->argvName);
                        Tcl_AppendElement(interp, buf);
                    }
                    break;
                default:
                    assert(0 == "Unknown spec type");
            }
            cs = cs + 1;
        }
        return TCL_OK;
    }
    for (i = 0; (i + 1) <= objc; i++) {
        char *arg = Tcl_GetString(objv[i]);
        cs = configSpecs;
        while (cs->type != TK_CONFIG_END) {
            if (!strcmp(arg, cs->argvName)) {
                switch (cs->type) {
                    case TK_CONFIG_STRING:{
                            char **sp;
                            op = dp + cs->offset;
                            sp = (char **) op;
                            if (++i >= objc) {
                                Tcl_SetResult(interp, *sp, 0);
                                return TCL_OK;
                            }
                            *sp = strdup(arg);
                            goto foundopt;
                        }
                        break;
                    case TK_CONFIG_INT:{
                            int *sp;
                            op = dp + cs->offset;
                            sp = (int *) op;
                            if (++i >= objc) {
                                char buf[50];
                                sprintf(buf, "%d", *sp);
                                Tcl_SetResult(interp, buf, 0);
                                return TCL_OK;
                            }
                            *sp = atoi(arg);
                            goto foundopt;
                        }
                        break;
                    default:
                        assert(0 == "Unknown spec type");
                }
            }
            cs = cs + 1;
        }
        fprintf(stderr, "Unknown option %s\n", arg);
        return TCL_ERROR;
      foundopt:
        continue;
    }
    return TCL_OK;
}

int
TclConfigureWidget(interp, htmlPtr, configSpecs, argc, argv, dp, flags)
    Tcl_Interp *interp;
    HtmlWidget *htmlPtr;
    Tk_ConfigSpec *configSpecs;
    int argc;
    char **argv;
    char *dp;
    int flags;
{
    Tk_ConfigSpec *cs;
    int i;
    char *op;
    if (argc == 0) {
        cs = configSpecs;
        while (cs->type != TK_CONFIG_END) {
            switch (cs->type) {
                case TK_CONFIG_STRING:{
                        char **sp;
                        op = dp + cs->offset;
                        sp = (char **) op;
                        Tcl_AppendElement(interp, cs->argvName);
                        Tcl_AppendElement(interp, *sp);
                    }
                    break;
                case TK_CONFIG_INT:{
                        int *sp;
                        char buf[50];
                        op = dp + cs->offset;
                        sp = (int *) op;
                        sprintf(buf, "%d", *sp);
                        Tcl_AppendElement(interp, cs->argvName);
                        Tcl_AppendElement(interp, buf);
                    }
                    break;
                default:
                    assert(0 == "Unknown spec type");
            }
            cs = cs + 1;
        }
        return TCL_OK;
    }
    for (i = 0; (i + 1) <= argc && argv[i]; i++) {
        cs = configSpecs;
        while (cs->type != TK_CONFIG_END) {
            if (!strcmp(argv[i], cs->argvName)) {
                switch (cs->type) {
                    case TK_CONFIG_STRING:{
                            char **sp;
                            op = dp + cs->offset;
                            sp = (char **) op;
                            if (++i >= argc) {
                                Tcl_SetResult(interp, *sp, 0);
                                return TCL_OK;
                            }
                            *sp = strdup(argv[i]);
                            goto foundopt;
                        }
                        break;
                    case TK_CONFIG_INT:{
                            int *sp;
                            op = dp + cs->offset;
                            sp = (int *) op;
                            if (++i >= argc) {
                                char buf[50];
                                sprintf(buf, "%d", *sp);
                                Tcl_SetResult(interp, buf, 0);
                                return TCL_OK;
                            }
                            *sp = atoi(argv[i]);
                            goto foundopt;
                        }
                        break;
                    default:
                        assert(0 == "Unknown spec type");
                }
            }
            cs = cs + 1;
        }
        fprintf(stderr, "Unknown option %s\n", argv[i]);
        return TCL_ERROR;
      foundopt:
        continue;
    }
    return TCL_OK;
}

#ifdef _TCLHTML_
static void
HtmlCmdDeletedProc(ClientData clientData)
{
}
static void
HtmlEventProc(ClientData clientData, XEvent * eventPtr)
{
}
void
HtmlRedrawText(HtmlWidget * htmlPtr, int y)
{
}
void
HtmlRedrawBlock(HtmlWidget * htmlPtr, HtmlBlock * p)
{
}
int
HtmlGetColorByName(HtmlWidget * htmlPtr, char *zColor, int def)
{
    return 0;
}

void
HtmlScheduleRedraw(HtmlWidget * htmlPtr)
{
}
#else

/*
** Find the width of the usable drawing area in pixels.  If the window isn't
** mapped, use the size requested by the user.
**
** The usable drawing area is the area available for displaying rendered
** HTML.  The usable drawing area does not include the 3D border or the
** padx and pady boundry within the 3D border.  The usable drawing area
** is the size of the clipping window.
*/
int
HtmlUsableWidth(htmlPtr)
    HtmlWidget *htmlPtr;
{
    int w;
    Tk_Window tkwin = htmlPtr->tkwin;
    if (tkwin && Tk_IsMapped(tkwin)) {
        w = Tk_Width(tkwin) - 2 * (htmlPtr->padx + htmlPtr->inset);
    }
    else {
        w = htmlPtr->width;
    }
    return w;
}

/*
** Find the height of the usable drawing area in pixels.  If the window isn't
** mapped, use the size requested by the user.
**
** The usable drawing area is the area available for displaying rendered
** HTML.  The usable drawing area does not include the 3D border or the
** padx and pady boundry within the 3D border.  The usable drawing area
** is the size of the clipping window.
*/
int
HtmlUsableHeight(htmlPtr)
    HtmlWidget *htmlPtr;
{
    int h;
    Tk_Window tkwin = htmlPtr->tkwin;
    if (tkwin && Tk_IsMapped(tkwin)) {
        h = Tk_Height(tkwin) - 2 * (htmlPtr->pady + htmlPtr->inset);
    }
    else {
        h = htmlPtr->height;
    }
    return h;
}

/*
** Compute a pair of floating point numbers that describe the current
** vertical scroll position.  The first number is the fraction of
** the document that is off the top of the visible region and the second 
** number is the fraction that is beyond the end of the visible region.
*/
void
HtmlComputeVerticalPosition(htmlPtr, buf)
    HtmlWidget *htmlPtr;
    char *buf;                         /* Write the two floating point values 
                                        * here */
{
    int actual;                        /* Size of the viewing area */
    double frac1, frac2;

    actual = HtmlUsableHeight(htmlPtr);
    if (htmlPtr->maxY <= 0) {
        frac1 = 0.0;
        frac2 = 1.0;
    }
    else {
        frac1 = (double) htmlPtr->yOffset / (double) htmlPtr->maxY;
        if (frac1 > 1.0) {
            frac1 = 1.0;
        }
        else if (frac1 < 0.0) {
            frac1 = 0.0;
        }
        frac2 = (double) (htmlPtr->yOffset + actual) / (double) htmlPtr->maxY;
        if (frac2 > 1.0) {
            frac2 = 1.0;
        }
        else if (frac2 < 0.0) {
            frac2 = 0.0;
        }
    }
    sprintf(buf, "%g %g", frac1, frac2);
}

/*
** Do the same thing for the horizontal direction
*/
void
HtmlComputeHorizontalPosition(htmlPtr, buf)
    HtmlWidget *htmlPtr;
    char *buf;                         /* Write the two floating point values 
                                        * here */
{
    int actual;                        /* Size of the viewing area */
    double frac1, frac2;

    actual = HtmlUsableWidth(htmlPtr);
    if (htmlPtr->maxX <= 0) {
        frac1 = 0.0;
        frac2 = 1.0;
    }
    else {
        frac1 = (double) htmlPtr->xOffset / (double) htmlPtr->maxX;
        if (frac1 > 1.0) {
            frac1 = 1.0;
        }
        else if (frac1 < 0.0) {
            frac1 = 0.0;
        }
        frac2 = (double) (htmlPtr->xOffset + actual) / (double) htmlPtr->maxX;
        if (frac2 > 1.0) {
            frac2 = 1.0;
        }
        else if (frac2 < 0.0) {
            frac2 = 0.0;
        }
    }
    sprintf(buf, "%g %g", frac1, frac2);
}

static int GcNextToFree = 0;

/*
** Clear the cache of GCs
*/
void
ClearGcCache(htmlPtr)
    HtmlWidget *htmlPtr;
{
    int i;
    for (i = 0; i < N_CACHE_GC; i++) {
        if (htmlPtr->aGcCache[i].index) {
            Tk_FreeGC(htmlPtr->display, htmlPtr->aGcCache[i].gc);
            htmlPtr->aGcCache[i].index = 0;
        }
        else {
        }
    }
    GcNextToFree = 0;
}

/*
** This routine is called when the widget command is deleted.  If the
** widget isn't already in the process of being destroyed, this command
** starts that process rolling.
**
** This routine can be called in two ways.  
**
**   (1) The window is destroyed, which causes the command to be deleted.
**       In this case, we don't have to do anything.
**
**   (2) The command only is deleted (ex: "rename .html {}").  In that
**       case we need to destroy the window.
*/
static void
HtmlCmdDeletedProc(clientData)
    ClientData clientData;
{
    HtmlWidget *htmlPtr = (HtmlWidget *) clientData;
    if (htmlPtr != NULL && htmlPtr->tkwin != NULL) {
        Tk_Window tkwin = htmlPtr->tkwin;
        htmlPtr->tkwin = NULL;
        Tk_DestroyWindow(tkwin);
    }
}

/*
** Reset the main layout context in the main widget.  This happens
** before we redo the layout, or just before deleting the widget.
*/
static void
ResetLayoutContext(htmlPtr)
    HtmlWidget *htmlPtr;
{
    htmlPtr->layoutContext.headRoom = 0;
    htmlPtr->layoutContext.top = 0;
    htmlPtr->layoutContext.bottom = 0;
    HtmlClearMarginStack(&htmlPtr->layoutContext.leftMargin);
    HtmlClearMarginStack(&htmlPtr->layoutContext.rightMargin);
}

/*
 *---------------------------------------------------------------------------
 *
 * HtmlRedrawCallback --
 *
 *     This routine is invoked in order to redraw all or part of the HTML
 *     widget.  This might happen because the display has changed, or in
 *     response to an expose event.  In all cases, though, this routine is
 *     called by an idle callback.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Layout engine might be run. The window contents might be modified.
 *
 *---------------------------------------------------------------------------
 */
void 
HtmlRedrawCallback(clientData)
    ClientData clientData;          /* The Html widget */
{
    HtmlWidget *htmlPtr = (HtmlWidget *) clientData;
    Tk_Window tkwin = htmlPtr->tkwin;
    Tk_Window clipwin = htmlPtr->clipwin;
    Pixmap pixmap;                     /* The buffer on which to render HTML */
    int x, y, w, h;                    /* Virtual canvas coordinates of area
                                        * to draw */
    int hw;                            /* highlight thickness */
    int insetX, insetY;                /* Total highlight thickness, border
                                        * width and ** padx/y */
    int clipwinH, clipwinW;            /* Width and height of the clipping
                                        * window */
    HtmlBlock *pBlock;                 /* For looping over blocks to be drawn 
                                        */
    int redoSelection = 0;             /* True to recompute the selection */
    int top, bottom, left, right;      /* Coordinates of the clipping window */
    int imageTop;                      /* Top edge of image */
    HtmlElement *pElem;

    /*
     * Don't bother doing anything if the widget is in the process of
     * being destroyed, or we are in the middle of a parse.
     */
    if (tkwin == 0) {
        goto redrawExit;
    }
    if (htmlPtr->inParse) {
        htmlPtr->flags &= ~REDRAW_PENDING;
        goto redrawExit;
    }

    if ((htmlPtr->flags & RESIZE_ELEMENTS) != 0
        && (htmlPtr->flags & STYLER_RUNNING) == 0) {
        HtmlImage *pImage;
        for (pImage = htmlPtr->imageList; pImage; pImage = pImage->pNext) {
            pImage->pList = 0;
        }
        htmlPtr->lastSized = 0;
        htmlPtr->flags &= ~RESIZE_ELEMENTS;
        htmlPtr->flags |= RELAYOUT;
    }

    /*
     * Recompute the layout, if necessary or requested.
     *
     * We used to make a distinction between RELAYOUT and EXTEND_LAYOUT. **
     * RELAYOUT would be used when the widget was resized, but the ** less
     * compute-intensive EXTEND_LAYOUT would be used when new ** text was
     * appended. ** ** Unfortunately, EXTEND_LAYOUT has some problem that
     * arise when ** tables are used.  The quick fix is to make an
     * EXTEND_LAYOUT do ** a complete RELAYOUT.  Someday, we need to fix
     * EXTEND_LAYOUT so ** that it works right... 
     *
     * Calling HtmlLayout() is tricky because HtmlLayout() may invoke one
     * or more callbacks (thru the "-imagecommand" callback, for instance)
     * and these callbacks could, in theory, do nasty things like delete 
     * or unmap this widget.  So we have to take precautions:
     *
     *   *  Don't remove the REDRAW_PENDING flag until after HtmlLayout()
     *      has been called, to prevent a recursive call to 
     *      HtmlRedrawCallback().
     *
     *   *  Call HtmlLock() on the htmlPtr structure to prevent it from
     *      being deleted out from under us.
     */
    if ((htmlPtr->flags & (RELAYOUT | EXTEND_LAYOUT)) != 0
        && (htmlPtr->flags & STYLER_RUNNING) == 0) {
        htmlPtr->nextPlaced = 0;
        /*
         * htmlPtr->nInput = 0; 
         */
        htmlPtr->varId = 0;
        htmlPtr->maxX = 0;
        htmlPtr->maxY = 0;
        ResetLayoutContext(htmlPtr);
        htmlPtr->firstBlock = 0;
        htmlPtr->lastBlock = 0;
        redoSelection = 1;
        htmlPtr->flags &= ~RELAYOUT;
        htmlPtr->flags |= HSCROLL | VSCROLL | REDRAW_TEXT | EXTEND_LAYOUT;
    }
    if ((htmlPtr->flags & EXTEND_LAYOUT) && htmlPtr->pFirst != 0) {
        HtmlLock(htmlPtr);
        HtmlLayout(htmlPtr);
        if (HtmlUnlock(htmlPtr))
            goto redrawExit;
        tkwin = htmlPtr->tkwin;
        htmlPtr->flags &= ~EXTEND_LAYOUT;
        HtmlFormBlocks(htmlPtr);
        HtmlMapControls(htmlPtr);
        if (redoSelection && htmlPtr->selBegin.p && htmlPtr->selEnd.p) {
            HtmlUpdateSelection(htmlPtr, 1);
            HtmlUpdateInsert(htmlPtr);
        }
    }
    htmlPtr->flags &= ~REDRAW_PENDING;

    /*
     * No need to do any actual drawing if we aren't mapped 
     */
    if (!Tk_IsMapped(tkwin)) {
        goto redrawExit;
    }

    /*
     * Redraw the scrollbars.  Take care here, since the scrollbar ** update
     * command could (in theory) delete the html widget, or ** even the whole 
     * interpreter.  Preserve critical data structures, ** and check to see
     * if we are still alive before continuing. 
     */
    if ((htmlPtr->flags & (HSCROLL | VSCROLL)) != 0) {
        Tcl_Interp *interp = htmlPtr->interp;
        int result;
        char buf[200];

        if ((htmlPtr->flags & HSCROLL) != 0) {
            if (htmlPtr->xScrollCmd && htmlPtr->xScrollCmd[0]) {
                HtmlComputeHorizontalPosition(htmlPtr, buf);
                HtmlLock(htmlPtr);
                result = Tcl_VarEval(interp, htmlPtr->xScrollCmd, " ", buf, 0);
                if (HtmlUnlock(htmlPtr))
                    goto redrawExit;
                if (result != TCL_OK) {
                    Tcl_AddErrorInfo(interp,
                                     "\n    (horizontal scrolling command executed by html widget)");
                    Tcl_BackgroundError(interp);
                }
            }
            htmlPtr->flags &= ~HSCROLL;
        }
        if ((htmlPtr->flags & VSCROLL) != 0 && tkwin && Tk_IsMapped(tkwin)) {
            if (htmlPtr->yScrollCmd && htmlPtr->yScrollCmd[0]) {
                Tcl_Interp *interp = htmlPtr->interp;
                int result;
                char buf[200];
                HtmlComputeVerticalPosition(htmlPtr, buf);
                HtmlLock(htmlPtr);
                result = Tcl_VarEval(interp, htmlPtr->yScrollCmd, " ", buf, 0);
                if (HtmlUnlock(htmlPtr))
                    goto redrawExit;
                if (result != TCL_OK) {
                    Tcl_AddErrorInfo(interp,
                                     "\n    (horizontal scrolling command executed by html widget)");
                    Tcl_BackgroundError(interp);
                }
            }
            htmlPtr->flags &= ~VSCROLL;
        }
        tkwin = htmlPtr->tkwin;
        if (tkwin == 0 || !Tk_IsMapped(tkwin)) {
            goto redrawExit;
        }
        if (htmlPtr->flags & REDRAW_PENDING) {
            return;
        }
        clipwin = htmlPtr->clipwin;
        if (clipwin == 0) {
            goto redrawExit;
        }
    }

    /*
     * Redraw the focus highlight, if requested 
     */
    hw = htmlPtr->highlightWidth;
    if (htmlPtr->flags & REDRAW_FOCUS) {
        if (hw > 0) {
            GC gc;
            Tk_Window tkwin = htmlPtr->tkwin;

            if (htmlPtr->flags & GOT_FOCUS) {
                gc = Tk_GCForColor(htmlPtr->highlightColorPtr,
                                   Tk_WindowId(tkwin));
            }
            else {
                gc = Tk_GCForColor(htmlPtr->highlightBgColorPtr,
                                   Tk_WindowId(tkwin));
            }
            Tk_DrawFocusHighlight(tkwin, gc, hw, Tk_WindowId(tkwin));
        }
        htmlPtr->flags &= ~REDRAW_FOCUS;
    }

    /*
     * Draw the borders around the parameter of the window.  This is ** drawn 
     * directly -- it is not double buffered. 
     */
    if (htmlPtr->flags & REDRAW_BORDER) {
        htmlPtr->flags &= ~REDRAW_BORDER;
        Tk_Fill3DRectangle(tkwin, Tk_WindowId(tkwin), htmlPtr->border, hw,      /* x 
                                                                                 */
                           hw,  /* y */
                           Tk_Width(tkwin) - 2 * hw,    /* width */
                           Tk_Height(tkwin) - 2 * hw,   /* height */
                           htmlPtr->borderWidth, htmlPtr->relief);
    }

    /*
     ** If the styler is in a callback, unmap the clipping window and
     ** abort further processing.
     */
    if (htmlPtr->flags & STYLER_RUNNING) {
        if (Tk_IsMapped(clipwin)) {
            Tk_UnmapWindow(clipwin);
        }
        goto earlyOut;
    }

    /*
     ** If we don't have a clipping window, then something is seriously
     ** wrong.  We might as well give up.
     */
    if (clipwin == NULL) {
        goto earlyOut;
    }

    /*
     * Resize, reposition and map the clipping window, if necessary 
     */
    insetX = htmlPtr->padx + htmlPtr->inset;
    insetY = htmlPtr->pady + htmlPtr->inset;
    if (htmlPtr->flags & RESIZE_CLIPWIN) {
        int h, w;
        Tk_MoveResizeWindow(clipwin, insetX, insetY,
                            htmlPtr->realWidth - 2 * insetX,
                            htmlPtr->realHeight - 2 * insetY);
        if (!Tk_IsMapped(clipwin)) {
            Tk_MapWindow(clipwin);
        }
        h = htmlPtr->realHeight - 2 * insetY;
        if (htmlPtr->yOffset + h > htmlPtr->maxY) {
            htmlPtr->yOffset = htmlPtr->maxY - h;
        }
        if (htmlPtr->yOffset < 0) {
            htmlPtr->yOffset = 0;
        }
        w = htmlPtr->realWidth - 2 * insetX;
        if (htmlPtr->xOffset + w > htmlPtr->maxX) {
            htmlPtr->xOffset = htmlPtr->maxX - w;
        }
        if (htmlPtr->xOffset < 0) {
            htmlPtr->xOffset = 0;
        }
        htmlPtr->flags &= ~RESIZE_CLIPWIN;
    }
    HtmlMapControls(htmlPtr);

    /*
     ** Compute the virtual canvas coordinates corresponding to the
     ** dirty region of the clipping window.
     */
    clipwinW = Tk_Width(clipwin);
    clipwinH = Tk_Height(clipwin);
    if (htmlPtr->flags & REDRAW_TEXT) {
        w = clipwinW;
        h = clipwinH;
        x = htmlPtr->xOffset;
        y = htmlPtr->yOffset;
        htmlPtr->dirtyLeft = 0;
        htmlPtr->dirtyTop = 0;
        htmlPtr->flags &= ~REDRAW_TEXT;
    }
    else {
        if (htmlPtr->dirtyLeft < 0) {
            htmlPtr->dirtyLeft = 0;
        }
        if (htmlPtr->dirtyRight > clipwinW) {
            htmlPtr->dirtyRight = clipwinW;
        }
        if (htmlPtr->dirtyTop < 0) {
            htmlPtr->dirtyTop = 0;
        }
        if (htmlPtr->dirtyBottom > clipwinH) {
            htmlPtr->dirtyBottom = clipwinH;
        }
        w = htmlPtr->dirtyRight - htmlPtr->dirtyLeft;
        h = htmlPtr->dirtyBottom - htmlPtr->dirtyTop;
        x = htmlPtr->xOffset + htmlPtr->dirtyLeft;
        y = htmlPtr->yOffset + htmlPtr->dirtyTop;
    }

    top = htmlPtr->yOffset;
    bottom = top + HtmlUsableHeight(htmlPtr);
    left = htmlPtr->xOffset;
    right = left + HtmlUsableWidth(htmlPtr);

    /*
     * Skip the rest of the drawing process if the area to be refreshed is ** 
     * less than zero 
     */
    if (w > 0 && h > 0) {
        Display *display = htmlPtr->display;
        int dead;
        GC gcBg;
        XRectangle xrec;
        /*
         * fprintf(stderr,"Redraw %dx%d at %d,%d: %d,%d: %d,%d\n", w, h, x,
         * y, left, top, htmlPtr->dirtyLeft, htmlPtr->dirtyTop); 
         */

        /*
         * Allocate and clear a pixmap upon which to draw 
         */
        gcBg = HtmlGetGC(htmlPtr, COLOR_Background, FONT_Any);
        pixmap = Tk_GetPixmap(display, Tk_WindowId(clipwin), w, h,
                              Tk_Depth(clipwin));
        xrec.x = 0;
        xrec.y = 0;
        xrec.width = w;
        xrec.height = h;
        XFillRectangles(display, pixmap, gcBg, &xrec, 1);
        if (htmlPtr->bgimage)
            HtmlBGDraw(htmlPtr, left, top, w, h, pixmap, htmlPtr->bgimage);

        /*
         * Render all visible HTML onto the pixmap 
         */
        HtmlLock(htmlPtr);
        for (pBlock = htmlPtr->firstBlock; pBlock; pBlock = pBlock->pNext) {
            if (pBlock->top <= y + h && pBlock->bottom >= y
                && pBlock->left <= x + w && pBlock->right >= x) {
                HtmlBlockDraw(htmlPtr, pBlock, pixmap, x, y, w, h, pixmap);
                if (htmlPtr->tkwin == 0)
                    break;
            }
        }
        dead = HtmlUnlock(htmlPtr);

        /*
         * Finally, copy the pixmap onto the window and delete the pixmap 
         */
        if (!dead) {
            XCopyArea(display, pixmap, Tk_WindowId(clipwin),
                      gcBg, 0, 0, w, h, htmlPtr->dirtyLeft, htmlPtr->dirtyTop);
        }
        Tk_FreePixmap(display, pixmap);
        if (dead)
            goto redrawExit;
        /*
         * XFlush(display); 
         */
    }

    /*
     * Redraw images, if requested 
     */
    if (htmlPtr->flags & REDRAW_IMAGES) {
        HtmlImage *pImage;

        for (pImage = htmlPtr->imageList; pImage; pImage = pImage->pNext) {
            for (pElem = pImage->pList; pElem; pElem = pElem->image.pNext) {
                if (pElem->image.redrawNeeded == 0)
                    continue;
                imageTop = pElem->image.y - pElem->image.ascent;
                if (imageTop > bottom
                    || imageTop + pElem->image.h < top
                    || pElem->image.x > right
                    || pElem->image.x + pElem->image.w < left) {
                    continue;
                }
                HtmlDrawImage(htmlPtr, pElem, Tk_WindowId(htmlPtr->clipwin),
                              left, top, right, bottom);
            }
        }
        htmlPtr->flags &= ~(REDRAW_IMAGES | ANIMATE_IMAGES);
    }

    /*
     * Set the dirty region to the empty set. 
     */
  earlyOut:
    htmlPtr->dirtyTop = LARGE_NUMBER;
    htmlPtr->dirtyLeft = LARGE_NUMBER;
    htmlPtr->dirtyBottom = 0;
    htmlPtr->dirtyRight = 0;
  redrawExit:
    return;
}

/*
** If any part of the screen needs to be redrawn, Then call this routine
** with the values of a box (in window coordinates) that needs to be 
** redrawn.  This routine will make sure an idle callback is scheduled
** to do the redraw.
**
** The box coordinates are relative to the clipping window (clipwin),
** not the main window (tkwin).  
*/
void
HtmlRedrawArea(htmlPtr, left, top, right, bottom)
    HtmlWidget *htmlPtr;               /* The widget to be redrawn */
    int left;                          /* Top left corner of area to redraw */
    int top;
    int right;                         /* bottom right corner of area to
                                        * redraw */
    int bottom;
{
    if (bottom < 0) {
        return;
    }
    if (top > htmlPtr->realHeight) {
        return;
    }
    if (right < 0) {
        return;
    }
    if (left > htmlPtr->realWidth) {
        return;
    }
    if (htmlPtr->dirtyTop > top) {
        htmlPtr->dirtyTop = top;
    }
    if (htmlPtr->dirtyLeft > left) {
        htmlPtr->dirtyLeft = left;
    }
    if (htmlPtr->dirtyBottom < bottom) {
        htmlPtr->dirtyBottom = bottom;
    }
    if (htmlPtr->dirtyRight < right) {
        htmlPtr->dirtyRight = right;
    }
    HtmlScheduleRedraw(htmlPtr);
}

/* Redraw the HtmlBlock given.
*/
void
HtmlRedrawBlock(htmlPtr, p)
    HtmlWidget *htmlPtr;
    HtmlBlock *p;
{
    if (p) {
        HtmlRedrawArea(htmlPtr,
                       p->left - htmlPtr->xOffset,
                       p->top - htmlPtr->yOffset,
                       p->right - htmlPtr->xOffset + 1,
                       p->bottom - htmlPtr->yOffset);
    }
    else {
    }
}

/*
** Call this routine to force the entire widget to be redrawn.
*/
void
HtmlRedrawEverything(htmlPtr)
    HtmlWidget *htmlPtr;
{
    htmlPtr->flags |= REDRAW_FOCUS | REDRAW_TEXT | REDRAW_BORDER;
    HtmlScheduleRedraw(htmlPtr);
}

/*
** Do the redrawing right now.  Don't wait.
*/
#if 0                           /* NOT_USED */
static void
HtmlRedrawPush(HtmlWidget * htmlPtr)
{
    if (htmlPtr->flags & REDRAW_PENDING) {
        Tcl_CancelIdleCall(HtmlRedrawCallback, (ClientData) htmlPtr);
    }
    else {
    }
    HtmlRedrawCallback((ClientData) htmlPtr);
}
#endif

/*
** Call this routine to cause all of the rendered HTML at the
** virtual canvas coordinate of Y and beyond to be redrawn.
*/
void
HtmlRedrawText(htmlPtr, y)
    HtmlWidget *htmlPtr;
    int y;
{
    int yOffset;                       /* Top-most visible canvas coordinate */
    int clipHeight;                    /* Height of the clipping window */

    yOffset = htmlPtr->yOffset;
    clipHeight = HtmlUsableHeight(htmlPtr);
    y -= yOffset;
    if (y < clipHeight) {
        HtmlRedrawArea(htmlPtr, 0, y, LARGE_NUMBER, clipHeight);
    }
    else {
    }
}

/*
** Recalculate the preferred size of the html widget and pass this
** along to the geometry manager.
*/
static void
HtmlRecomputeGeometry(htmlPtr)
    HtmlWidget *htmlPtr;
{
    int w, h;                          /* Total width and height of the
                                        * widget */

    htmlPtr->inset = htmlPtr->highlightWidth + htmlPtr->borderWidth;
    w = htmlPtr->width + 2 * (htmlPtr->padx + htmlPtr->inset);
    h = htmlPtr->height + 2 * (htmlPtr->pady + htmlPtr->inset);
    Tk_GeometryRequest(htmlPtr->tkwin, w, h);
    Tk_SetInternalBorder(htmlPtr->tkwin, htmlPtr->inset);
}

void
HtmlClearTk(htmlPtr)
    HtmlWidget *htmlPtr;
{
    int i;
    HtmlElement *p, *pNext;
    HtmlImage *Ip;
    htmlPtr->topmargin = htmlPtr->leftmargin = HTML_INDENT / 4;
    htmlPtr->marginwidth = htmlPtr->marginheight = HTML_INDENT / 4;
    for (i = N_PREDEFINED_COLOR; i < N_COLOR; i++) {
        if (htmlPtr->apColor[i] != 0) {
            Tk_FreeColor(htmlPtr->apColor[i]);
            htmlPtr->apColor[i] = 0;
        }
    }
    for (i = 0; i < N_COLOR; i++) {
        htmlPtr->iDark[i] = 0;
        htmlPtr->iLight[i] = 0;
    }
    htmlPtr->colorUsed = 0;
    while ((Ip = htmlPtr->imageList)) {
        htmlPtr->imageList = Ip->pNext;
        Tk_FreeImage(Ip->image);
        while (Ip->anims) {
            HtmlImageAnim *a = Ip->anims;
            Ip->anims = a->next;
            Tk_FreeImage(a->image);
            HtmlFree((char *) a);
        }
        HtmlFree(Ip);
    }
    if (htmlPtr->bgimage) {
        Tk_FreeImage(htmlPtr->bgimage);
        htmlPtr->bgimage = 0;
    }
    ClearGcCache(htmlPtr);
    ResetLayoutContext(htmlPtr);
}

void
DestroyHtmlWidgetTk(htmlPtr)
    HtmlWidget *htmlPtr;
{
    int i;

    Tk_FreeOptions(configSpecs, (char *) htmlPtr, htmlPtr->display, 0);
    for (i = 0; i < N_FONT; i++) {
        if (htmlPtr->aFont[i] != 0) {
            Tk_FreeFont(htmlPtr->aFont[i]);
            htmlPtr->aFont[i] = 0;
        }
    }
    HtmlFree(htmlPtr->zClipwin);
}

/*
** Flash the insertion cursor.
*/
void
HtmlFlashCursor(clientData)
    ClientData clientData;
{
    HtmlWidget *htmlPtr = (HtmlWidget *) clientData;
    if (htmlPtr->pInsBlock == 0 || htmlPtr->insOnTime <= 0
        || htmlPtr->insOffTime <= 0) {
        htmlPtr->insTimer = 0;
        return;
    }
    HtmlRedrawBlock(htmlPtr, htmlPtr->pInsBlock);
    if ((htmlPtr->flags & GOT_FOCUS) == 0) {
        htmlPtr->insStatus = 0;
        htmlPtr->insTimer = 0;
    }
    else if (htmlPtr->insStatus) {
        htmlPtr->insTimer = Tcl_CreateTimerHandler(htmlPtr->insOffTime,
                                                   HtmlFlashCursor, clientData);
        htmlPtr->insStatus = 0;
    }
    else {
        htmlPtr->insTimer = Tcl_CreateTimerHandler(htmlPtr->insOnTime,
                                                   HtmlFlashCursor, clientData);
        htmlPtr->insStatus = 1;
    }
}

/*
** Return a GC from the cache.  As many as N_CACHE_GCs are kept valid
** at any one time.  They are replaced using an LRU algorithm.
**
** A value of FONT_Any (-1) for the font means "don't care".
*/
GC
HtmlGetGC(htmlPtr, color, font)
    HtmlWidget *htmlPtr;
    int color;
    int font;
{
    int i, j;
    GcCache *p = htmlPtr->aGcCache;
    XGCValues gcValues;
    int mask;
    Tk_Font tkfont;

    /*
     ** Check for an existing GC.
     */
    if (color < 0 || color >= N_COLOR) {
        color = 0;
    }
    if (font < FONT_Any || font >= N_FONT) {
        font = FONT_Default;
    }
    for (i = 0; i < N_CACHE_GC; i++, p++) {
        if (p->index == 0) {
            continue;
        }
        if ((font < 0 || p->font == font) && p->color == color) {
            if (p->index > 1) {
                for (j = 0; j < N_CACHE_GC; j++) {
                    if (htmlPtr->aGcCache[j].index
                        && htmlPtr->aGcCache[j].index < p->index) {
                        htmlPtr->aGcCache[j].index++;
                    }
                }
                p->index = 1;
            }
            return htmlPtr->aGcCache[i].gc;
        }
    }

    /*
     ** No GC matches.  Find a place to allocate a new GC.
     */
    p = htmlPtr->aGcCache;
    for (i = 0; i < N_CACHE_GC; i++, p++) {
        if (p->index == 0 || p->index == N_CACHE_GC) {
            break;
        }
    }
    if (i >= N_CACHE_GC) {      /* No slot, so free one: round-robin */
        p = htmlPtr->aGcCache;
        for (i = 0; i < N_CACHE_GC && i < GcNextToFree; i++, p++);
        GcNextToFree = (GcNextToFree + 1) % N_CACHE_GC;
        Tk_FreeGC(htmlPtr->display, p->gc);
        /*
         * fprintf(stderr,"Tk_FreeGC: %d)\n", p->gc); 
         */
    }
    gcValues.foreground = htmlPtr->apColor[color]->pixel;
    gcValues.graphics_exposures = True;
    mask = GCForeground | GCGraphicsExposures;
    if (font < 0) {
        font = FONT_Default;
    }
    tkfont = HtmlGetFont(htmlPtr, font);
    if (tkfont) {
        gcValues.font = Tk_FontId(tkfont);
        mask |= GCFont;
    }
    p->gc = Tk_GetGC(htmlPtr->tkwin, mask, &gcValues);
    /*
     * fprintf(stderr,"Tk_GetGC: %d\n", p->gc); 
     */
    if (p->index == 0) {
        p->index = N_CACHE_GC + 1;
    }
    for (j = 0; j < N_CACHE_GC; j++) {
        if (htmlPtr->aGcCache[j].index && htmlPtr->aGcCache[j].index < p->index) {
            htmlPtr->aGcCache[j].index++;
        }
    }
    p->index = 1;
    p->font = font;
    p->color = color;
    return p->gc;
}

/*
** Retrieve any valid GC.  The font and color don't matter since the
** GC will only be used for copying.
*/
GC
HtmlGetAnyGC(htmlPtr)
    HtmlWidget *htmlPtr;
{
    int i;
    GcCache *p = htmlPtr->aGcCache;

    for (i = 0; i < N_CACHE_GC; i++, p++) {
        if (p->index) {
            return p->gc;
        }
    }
    return HtmlGetGC(htmlPtr, COLOR_Normal, FONT_Default);
}

/*
** All window events (for both tkwin and clipwin) are
** sent to this routine.
*/
static void
HtmlEventProc(clientData, eventPtr)
    ClientData clientData;
    XEvent *eventPtr;
{
    HtmlWidget *htmlPtr = (HtmlWidget *) clientData;
    int redraw_needed = 0;
    XConfigureRequestEvent *p;

    switch (eventPtr->type) {
        case GraphicsExpose:
        case Expose:
            if (htmlPtr->tkwin == 0) {
                /*
                 * The widget is being deleted.  Do nothing 
                 */
            }
            else if (eventPtr->xexpose.window != Tk_WindowId(htmlPtr->tkwin)) {
                /*
                 * Exposure in the clipping window 
                 */
                htmlPtr->flags |= ANIMATE_IMAGES;
                HtmlRedrawArea(htmlPtr, eventPtr->xexpose.x - 1,
                               eventPtr->xexpose.y - 1,
                               eventPtr->xexpose.x + eventPtr->xexpose.width +
                               1,
                               eventPtr->xexpose.y + eventPtr->xexpose.height +
                               1);
            }
            else {
                /*
                 * Exposure in the main window 
                 */
                htmlPtr->flags |= (REDRAW_BORDER | ANIMATE_IMAGES);
                HtmlScheduleRedraw(htmlPtr);
            }
            break;
        case DestroyNotify:
            if ((htmlPtr->flags & REDRAW_PENDING)) {
                Tcl_CancelIdleCall(HtmlRedrawCallback, (ClientData) htmlPtr);
                htmlPtr->flags &= ~REDRAW_PENDING;
            }
            if (htmlPtr->tkwin != 0) {
                if (eventPtr->xany.window != Tk_WindowId(htmlPtr->tkwin)) {
                    Tk_DestroyWindow(htmlPtr->tkwin);
                    htmlPtr->clipwin = 0;
                    break;
                }
                htmlPtr->tkwin = 0;
                Tcl_DeleteCommand(htmlPtr->interp, htmlPtr->zCmdName);
                Tcl_DeleteCommand(htmlPtr->interp, htmlPtr->zClipwin);
            }
            HtmlUnlock(htmlPtr);
            break;
        case ConfigureNotify:
            if (htmlPtr->tkwin != 0
                && eventPtr->xconfigure.window == Tk_WindowId(htmlPtr->tkwin)
                    ) {
                p = (XConfigureRequestEvent *) eventPtr;
                if (p->width != htmlPtr->realWidth) {
                    redraw_needed = 1;
                    htmlPtr->realWidth = p->width;
                }
                if (p->height != htmlPtr->realHeight) {
                    redraw_needed = 1;
                    htmlPtr->realHeight = p->height;
                }
                else {
                }
                if (redraw_needed) {
                    htmlPtr->flags |=
                            RELAYOUT | VSCROLL | HSCROLL | RESIZE_CLIPWIN;
                    htmlPtr->flags |= ANIMATE_IMAGES;
                    HtmlRedrawEverything(htmlPtr);
                }
            }
            break;
        case FocusIn:
            if (htmlPtr->tkwin != 0
                && eventPtr->xfocus.window == Tk_WindowId(htmlPtr->tkwin)
                && eventPtr->xfocus.detail != NotifyInferior) {
                htmlPtr->flags |= GOT_FOCUS | REDRAW_FOCUS | ANIMATE_IMAGES;
                HtmlScheduleRedraw(htmlPtr);
                HtmlUpdateInsert(htmlPtr);
            }
            break;
        case FocusOut:
            if (htmlPtr->tkwin != 0
                && eventPtr->xfocus.window == Tk_WindowId(htmlPtr->tkwin)
                && eventPtr->xfocus.detail != NotifyInferior) {
                htmlPtr->flags &= ~GOT_FOCUS;
                htmlPtr->flags |= REDRAW_FOCUS;
                HtmlScheduleRedraw(htmlPtr);
            }
            break;
    }
}

/*
** The rendering and layout routines should call this routine in order to get
** a font structure.  The iFont parameter specifies which of the N_FONT
** fonts should be obtained.  The font is allocated if necessary.
**
** Because the -fontcommand callback can be invoked, this function can
** (in theory) cause the HTML widget to be changed arbitrarily or even
** deleted.  Callers of this function much be prepared to be called
** recursively and/or to have the HTML widget deleted out from under
** them.  This routine will return NULL if the HTML widget is deleted.
*/
Tk_Font
HtmlGetFont(htmlPtr, iFont)
    HtmlWidget *htmlPtr;               /* The HTML widget to which the font
                                        * applies */
    int iFont;                         /* Which font to obtain */
{
    Tk_Font toFree = 0;

    if (iFont < 0) {
        iFont = 0;
    }
    if (iFont >= N_FONT) {
        iFont = N_FONT - 1;
        CANT_HAPPEN;
    }

    /*
     ** If the font has previously been allocated, but the "fontValid" bitmap
     ** shows it is no longer valid, then mark it for freeing later.  We use
     ** a policy of allocate-before-free because Tk's font cache operates
     ** much more efficiently that way.
     */
    if (!FontIsValid(htmlPtr, iFont) && htmlPtr->aFont[iFont] != 0) {
        toFree = htmlPtr->aFont[iFont];
        htmlPtr->aFont[iFont] = 0;
    }

    /*
     ** If we need to allocate a font, first construct the font name then
     ** allocate it.
     */
    if (htmlPtr->aFont[iFont] == 0) {
        char name[200];                /* Name of the font */

        name[0] = 0;

        /*
         * Run the -fontcommand if it is specified 
         */
        if (htmlPtr->zFontCommand && htmlPtr->zFontCommand[0]) {
            int iFam;                  /* The font family index.  Value
                                        * between 0 and 7 */
            Tcl_DString str;           /* The command we'll execute to get
                                        * the font name */
            char *zSep = "";           /* Separator between font attributes */
            int rc;                    /* Return code from the font command */
            char zBuf[100];            /* Temporary buffer */

            Tcl_DStringInit(&str);
            Tcl_DStringAppend(&str, htmlPtr->zFontCommand, -1);
            sprintf(zBuf, " %d {", FontSize(iFont) + 1);
            Tcl_DStringAppend(&str, zBuf, -1);
            iFam = iFont / N_FONT_SIZE;
            if (iFam & 1) {
                Tcl_DStringAppend(&str, "bold", -1);
                zSep = " ";
            }
            if (iFam & 2) {
                Tcl_DStringAppend(&str, zSep, -1);
                Tcl_DStringAppend(&str, "italic", -1);
                zSep = " ";
            }
            if (iFam & 4) {
                Tcl_DStringAppend(&str, zSep, -1);
                Tcl_DStringAppend(&str, "fixed", -1);
            }
            Tcl_DStringAppend(&str, "}", -1);
            HtmlLock(htmlPtr);
            rc = Tcl_GlobalEval(htmlPtr->interp, Tcl_DStringValue(&str));
            Tcl_DStringFree(&str);
            if (HtmlUnlock(htmlPtr)) {
                return NULL;
            }
            if (rc != TCL_OK) {
                Tcl_AddErrorInfo(htmlPtr->interp,
                                 "\n    (-fontcommand callback of HTML widget)");
                Tcl_BackgroundError(htmlPtr->interp);
            }
            else {
                sprintf(name, "%.100s", htmlPtr->interp->result);
            }
            Tcl_ResetResult(htmlPtr->interp);
        }

        /*
         ** If the -fontcommand failed or returned an empty string, or if
         ** there is no -fontcommand, then get the default font name.
         */
        if (name[0] == 0) {
            char *familyStr = "";
            int iFamily;
            int iSize;
            int size, finc = htmlPtr->FontAdjust;

            iFamily = iFont / N_FONT_SIZE;
            iSize = iFont % N_FONT_SIZE + 1;
            switch (iFamily) {
                case 0:
                    familyStr = "%s -%d";
                    break;
                case 1:
                    familyStr = "%s -%d bold";
                    break;
                case 2:
                    familyStr = "%s -%d italic";
                    break;
                case 3:
                    familyStr = "%s -%d bold italic";
                    break;
                case 4:
                    familyStr = "%s -%d";
                    break;
                case 5:
                    familyStr = "%s -%d bold";
                    break;
                case 6:
                    familyStr = "%s -%d italic";
                    break;
                case 7:
                    familyStr = "%s -%d bold italic";
                    break;
                default:
                    familyStr = "%s -14";
                    CANT_HAPPEN;
            }
            switch (iSize) {
                case 1:
                    size = 6 + finc;
                    break;
                case 2:
                    size = 10 + finc;
                    break;
                case 3:
                    size = 12 + finc;
                    break;
                case 4:
                    size = 14 + finc;
                    break;
                case 5:
                    size = 20 + finc;
                    break;
                case 6:
                    size = 24 + finc;
                    break;
                case 7:
                    size = 30 + finc;
                    break;
                default:
                    size = 14 + finc;
                    CANT_HAPPEN;
            }
            sprintf(name, familyStr, htmlPtr->FontFamily, size);
        }

        /*
         * Get the named font 
         */
        htmlPtr->aFont[iFont] =
                Tk_GetFont(htmlPtr->interp, htmlPtr->tkwin, name);
        if (htmlPtr->aFont[iFont] == 0) {
            Tcl_AddErrorInfo(htmlPtr->interp,
                             "\n    (trying to create a font named \"");
            Tcl_AddErrorInfo(htmlPtr->interp, name);
            Tcl_AddErrorInfo(htmlPtr->interp, "\" in the HTML widget)");
            Tcl_BackgroundError(htmlPtr->interp);
            htmlPtr->aFont[iFont] =
                    Tk_GetFont(htmlPtr->interp, htmlPtr->tkwin, "fixed");
        }
        if (htmlPtr->aFont[iFont] == 0) {
            Tcl_AddErrorInfo(htmlPtr->interp,
                             "\n    (trying to create font \"fixed\" in the HTML widget)");
            Tcl_BackgroundError(htmlPtr->interp);
            htmlPtr->aFont[iFont] =
                    Tk_GetFont(htmlPtr->interp, htmlPtr->tkwin,
                               "helvetica -12");
        }
        FontSetValid(htmlPtr, iFont);
    }

    /*
     ** Free the expired font, if any.
     */
    if (toFree != 0) {
        Tk_FreeFont(toFree);
    }
    return htmlPtr->aFont[iFont];
}

/*
** Compute the squared distance between two colors
*/
static float
colorDistance(pA, pB)
    XColor *pA;
    XColor *pB;
{
    float x, y, z;

    x = 0.30 * (pA->red - pB->red);
    y = 0.61 * (pA->green - pB->green);
    z = 0.11 * (pA->blue - pB->blue);
    return x * x + y * y + z * z;
}

/*
** This routine returns an index between 0 and N_COLOR-1 which indicates
** which XColor structure in the apColor[] array of htmlPtr should be
** used to describe the color specified by the given name.
*/
int
HtmlGetColorByName(htmlPtr, zColor, def)
    HtmlWidget *htmlPtr;
    char *zColor;
    int def;
{
    XColor *pNew;
    int iColor;
    Tk_Uid name;
    int i, n;
    char zAltColor[16];

    /*
     * Netscape accepts color names that are just HEX values, without ** the
     * # up front.  This isn't valid HTML, but we support it for **
     * compatibility. 
     */
    n = strlen(zColor);
    if (n == 6 || n == 3 || n == 9 || n == 12) {
        for (i = 0; i < n; i++) {
            if (!isxdigit(zColor[i]))
                break;
        }
        if (i == n) {
            sprintf(zAltColor, "#%s", zColor);
        }
        else {
            strcpy(zAltColor, zColor);
        }
        name = Tk_GetUid(zAltColor);
    }
    else {
        name = Tk_GetUid(zColor);
    }
    pNew = Tk_GetColor(htmlPtr->interp, htmlPtr->clipwin, name);
    if (pNew == 0) {
        return def;
    }

    iColor = GetColorByValue(htmlPtr, pNew);
    Tk_FreeColor(pNew);
    if (iColor < N_COLOR)
        return iColor;
    return def;
}

/*
** Macros used in the computation of appropriate shadow colors.
*/
#define MAX_COLOR 65535
#define MAX(A,B)     ((A)<(B)?(B):(A))
#define MIN(A,B)     ((A)<(B)?(A):(B))

/*
** Check to see if the given color is too dark to be easily distinguished
** from black.
*/
static int
isDarkColor(p)
    XColor *p;
{
    float x, y, z;

    x = 0.50 * p->red;
    y = 1.00 * p->green;
    z = 0.28 * p->blue;
    return (x * x + y * y + z * z) < 0.05 * MAX_COLOR * MAX_COLOR;
}

/*
** Given that the background color is iBgColor, figure out an
** appropriate color for the dark part of a 3D shadow.
*/
int
HtmlGetDarkShadowColor(htmlPtr, iBgColor)
    HtmlWidget *htmlPtr;
    int iBgColor;
{
    if (htmlPtr->iDark[iBgColor] == 0) {
        XColor *pRef, val;
        pRef = htmlPtr->apColor[iBgColor];
        if (isDarkColor(pRef)) {
            int t1, t2;
            t1 = MIN(MAX_COLOR, pRef->red * 1.2);
            t2 = (pRef->red * 3 + MAX_COLOR) / 4;
            val.red = MAX(t1, t2);
            t1 = MIN(MAX_COLOR, pRef->green * 1.2);
            t2 = (pRef->green * 3 + MAX_COLOR) / 4;
            val.green = MAX(t1, t2);
            t1 = MIN(MAX_COLOR, pRef->blue * 1.2);
            t2 = (pRef->blue * 3 + MAX_COLOR) / 4;
            val.blue = MAX(t1, t2);
        }
        else {
            val.red = pRef->red * 0.6;
            val.green = pRef->green * 0.6;
            val.blue = pRef->blue * 0.6;
        }
        htmlPtr->iDark[iBgColor] = GetColorByValue(htmlPtr, &val) + 1;
    }
    return htmlPtr->iDark[iBgColor] - 1;
}

/*
** Check to see if the given color is too light to be easily distinguished
** from white.
*/
static int
isLightColor(p)
    XColor *p;
{
    return p->green >= 0.85 * MAX_COLOR;
}

/*
** Given that the background color is iBgColor, figure out an
** appropriate color for the bright part of the 3D shadow.
*/
int
HtmlGetLightShadowColor(htmlPtr, iBgColor)
    HtmlWidget *htmlPtr;
    int iBgColor;
{
    if (htmlPtr->iLight[iBgColor] == 0) {
        XColor *pRef, val;
        pRef = htmlPtr->apColor[iBgColor];
        if (isLightColor(pRef)) {
            val.red = pRef->red * 0.9;
            val.green = pRef->green * 0.9;
            val.blue = pRef->blue * 0.9;
        }
        else {
            int t1, t2;
            t1 = MIN(MAX_COLOR, pRef->green * 1.4);
            t2 = (pRef->green + MAX_COLOR) / 2;
            val.green = MAX(t1, t2);
            t1 = MIN(MAX_COLOR, pRef->red * 1.4);
            t2 = (pRef->red + MAX_COLOR) / 2;
            val.red = MAX(t1, t2);
            t1 = MIN(MAX_COLOR, pRef->blue * 1.4);
            t2 = (pRef->blue + MAX_COLOR) / 2;
            val.blue = MAX(t1, t2);
        }
        htmlPtr->iLight[iBgColor] = GetColorByValue(htmlPtr, &val) + 1;
    }
    return htmlPtr->iLight[iBgColor] - 1;
}

# define COLOR_MASK  0xf800

/* Eliminate remapped duplicate colors. */
int
CheckDupColor(htmlPtr, slot)
    HtmlWidget *htmlPtr;
    int slot;
{
    int i;
    int r, g, b;
    XColor *pRef = htmlPtr->apColor[slot];
    r = pRef->red &= COLOR_MASK;
    g = pRef->green &= COLOR_MASK;
    b = pRef->blue &= COLOR_MASK;
    for (i = 0; i < N_COLOR; i++) {
        XColor *p = htmlPtr->apColor[i];
        if (i == slot)
            continue;
        if (p && (p->red & COLOR_MASK) == r && (p->green & COLOR_MASK) == g
            && (p->blue & COLOR_MASK) == b) {
            htmlPtr->colorUsed &= ~(1LL << slot);
            htmlPtr->apColor[slot] = 0;
            return i;
        }
    }
    return slot;
}

/*
** Find a color integer for the color whose color components
** are given by pRef.
*/
int
GetColorByValue(htmlPtr, pRef)
    HtmlWidget *htmlPtr;
    XColor *pRef;
{
    int i;
    float dist;
    float closestDist;
    int closest;
    int r, g, b;

    /*
     * Search for an exact match 
     */
    r = pRef->red &= COLOR_MASK;
    g = pRef->green &= COLOR_MASK;
    b = pRef->blue &= COLOR_MASK;
    for (i = 0; i < N_COLOR; i++) {
        XColor *p = htmlPtr->apColor[i];
        if (p && (p->red & COLOR_MASK) == r && (p->green & COLOR_MASK) == g
            && (p->blue & COLOR_MASK) == b) {
            htmlPtr->colorUsed |= (1LL << i);
            return i;
        }
    }

    /*
     * No exact matches.  Look for a completely unused slot 
     */
    for (i = N_PREDEFINED_COLOR; i < N_COLOR; i++) {
        if (htmlPtr->apColor[i] == 0) {
            htmlPtr->apColor[i] = Tk_GetColorByValue(htmlPtr->clipwin, pRef);
            /*
             * Check if colow was remapped to an existing slot 
             */
            htmlPtr->colorUsed |= (1LL << i);
            return CheckDupColor(htmlPtr, i);
        }
    }

    /*
     * No empty slots.  Look for a slot that contains a color that ** isn't
     * currently in use. 
     */
    for (i = N_PREDEFINED_COLOR; i < N_COLOR; i++) {
        if (((htmlPtr->colorUsed >> i) & 1LL) == 0) {
            Tk_FreeColor(htmlPtr->apColor[i]);
            htmlPtr->apColor[i] = Tk_GetColorByValue(htmlPtr->clipwin, pRef);
            htmlPtr->colorUsed |= (1LL << i);
            return CheckDupColor(htmlPtr, i);
        }
    }

    /*
     * Ok, find the existing color that is closest to the color requested **
     * and use it. 
     */
    closest = 0;
    closestDist = colorDistance(pRef, htmlPtr->apColor[0]);
    for (i = 1; i < N_COLOR; i++) {
        dist = colorDistance(pRef, htmlPtr->apColor[i]);
        if (dist < closestDist) {
            closestDist = dist;
            closest = i;
        }
    }
    return i;
}

/* Only support rect for now */
int
HtmlInArea(p, left, top, x, y)
    HtmlElement *p;
    int left;
    int top;
    int x;
    int y;
{
    int *ip = p->area.coords;
    return (ip && (left + ip[0]) <= x && (top + ip[1]) <= y
            && (left + ip[2]) >= x && (top + ip[3]) >= y);
}

/*
** This routine searchs for a hyperlink beneath the coordinates x,y
** and returns a pointer to the HREF for that hyperlink.  The text
** is held one of the markup.argv[] fields of the <a> markup.
*/
char *
HtmlGetHref(htmlPtr, x, y, target)
    HtmlWidget *htmlPtr;
    int x;
    int y;
    char **target;
{
    HtmlBlock *pBlock;
    HtmlElement *pElem;
    char *z;

    for (pBlock = htmlPtr->firstBlock; pBlock; pBlock = pBlock->pNext) {
        if (pBlock->top > y || pBlock->bottom < y
            || pBlock->left > x || pBlock->right < x) {
            continue;
        }
        pElem = pBlock->base.pNext;
        if (pElem->base.type == Html_IMG && pElem->image.pMap) {
            pElem = pElem->image.pMap->pNext;
            while (pElem && pElem->base.type != Html_EndMAP) {
                if (pElem->base.type == Html_AREA)
                    if (HtmlInArea(pElem, pBlock->left, pBlock->top, x, y)) {
                        *target = HtmlMarkupArg(pElem, "target", 0);
                        return HtmlMarkupArg(pElem, "href", 0);
                    }
                pElem = pElem->pNext;
            }
            continue;
        }
        if ((pElem->base.style.flags & STY_Anchor) == 0) {
            continue;
        }
        switch (pElem->base.type) {
            case Html_Text:
            case Html_Space:
            case Html_IMG:
                while (pElem && pElem->base.type != Html_A) {
                    pElem = pElem->base.pPrev;
                }
                if (pElem == 0 || pElem->base.type != Html_A) {
                    break;
                }
                *target = HtmlMarkupArg(pElem, "target", 0);
                return HtmlMarkupArg(pElem, "href", 0);
            default:
                break;
        }
    }
    return 0;
}

/* Return coordinates of item. */
int
HtmlElementCoords(interp, htmlPtr, p, i, pct, coords)
    Tcl_Interp *interp;
    HtmlWidget *htmlPtr;
    HtmlElement *p;
    int i;
    int pct;
    int *coords;
{
    HtmlBlock *pBlock;

    while (p && p->base.type != Html_Block) {
        p = p->base.pPrev;
    }
    if (!p)
        return 1;
    pBlock = &p->block;
    if (pct) {
        HtmlElement *pEnd = htmlPtr->pLast;
        HtmlBlock *pb2;
        while (pEnd && pEnd->base.type != Html_Block) {
            pEnd = pEnd->base.pPrev;
        }
        pb2 = &pEnd->block;
#define HGCo(dir) pb2->dir?pBlock->dir*100/pb2->dir:0
        coords[0] = HGCo(left);
        coords[1] = HGCo(top);
        coords[2] = HGCo(right);
        coords[3] = HGCo(bottom);
    }
    else {
        coords[0] = pBlock->left;
        coords[1] = pBlock->top;
        coords[2] = pBlock->right;
        coords[3] = pBlock->bottom;
    }
    return 0;
}

/* Return coordinates of item. */
void
HtmlGetCoords(interp, htmlPtr, p, i, pct)
    Tcl_Interp *interp;
    HtmlWidget *htmlPtr;
    HtmlElement *p;
    int i;
    int pct;
{
    Tcl_DString str;
    char *z, zLine[100];
    int coords[4];

    if (HtmlElementCoords(interp, htmlPtr, p, i, pct, coords))
        return;
    Tcl_DStringInit(&str);
    sprintf(zLine, "%d %d %d %d", coords[0], coords[1], coords[2], coords[3]);
    Tcl_DStringAppend(&str, zLine, -1);
    Tcl_DStringResult(interp, &str);
}

/*
** Change the "yOffset" field from its current value to the value given.
** This has the effect of scrolling the widget vertically.
*/
void
HtmlVerticalScroll(htmlPtr, yOffset)
    HtmlWidget *htmlPtr;
    int yOffset;
{
    int inset;                         /* The 3D border plus the pady */
    int h;                             /* Height of the clipping window */
    int diff;                          /* Difference between old and new
                                        * offset */
    GC gc;                             /* Graphics context used for copying */
    int w;                             /* Width of text area */

    if (yOffset == htmlPtr->yOffset) {
        return;
    }
    inset = htmlPtr->pady + htmlPtr->inset;
    h = htmlPtr->realHeight - 2 * inset;
    if ((htmlPtr->flags & REDRAW_TEXT) != 0
        || (htmlPtr->dirtyTop < h && htmlPtr->dirtyBottom > 0)
        || htmlPtr->yOffset > yOffset + (h - 30)
        || htmlPtr->yOffset < yOffset - (h - 30)
            ) {
        htmlPtr->yOffset = yOffset;
        htmlPtr->flags |= VSCROLL | REDRAW_TEXT;
        HtmlScheduleRedraw(htmlPtr);
        return;
    }
    diff = htmlPtr->yOffset - yOffset;
    gc = HtmlGetAnyGC(htmlPtr);
    w = htmlPtr->realWidth - 2 * (htmlPtr->inset + htmlPtr->padx);
    htmlPtr->flags |= VSCROLL;
    htmlPtr->yOffset = yOffset;
    if (diff < 0) {
        XCopyArea(htmlPtr->display, Tk_WindowId(htmlPtr->clipwin),      /* source 
                                                                         */
                  Tk_WindowId(htmlPtr->clipwin),        /* destination */
                  gc, 0, -diff, /* source X, Y */
                  w, h + diff,  /* Width and height */
                  0, 0);        /* Destination X, Y */
        HtmlRedrawArea(htmlPtr, 0, h + diff, w, h);
    }
    else {
        XCopyArea(htmlPtr->display, Tk_WindowId(htmlPtr->clipwin),      /* source 
                                                                         */
                  Tk_WindowId(htmlPtr->clipwin),        /* destination */
                  gc, 0, 0,     /* source X, Y */
                  w, h - diff,  /* Width and height */
                  0, diff);     /* Destination X, Y */
        HtmlRedrawArea(htmlPtr, 0, 0, w, diff);
    }
    /*
     * HtmlMapControls(htmlPtr);
     */
}

/*
** Change the "xOffset" field from its current value to the value given.
** This has the effect of scrolling the widget horizontally.
*/
void
HtmlHorizontalScroll(htmlPtr, xOffset)
    HtmlWidget *htmlPtr;
    int xOffset;
{
    if (xOffset == htmlPtr->xOffset) {
        return;
    }
    htmlPtr->xOffset = xOffset;
    HtmlMapControls(htmlPtr);
    htmlPtr->flags |= HSCROLL | REDRAW_TEXT;
    HtmlScheduleRedraw(htmlPtr);
}

/*
** Make sure that a call to the HtmlRedrawCallback() routine has been
** queued.
*/
void
HtmlScheduleRedraw(htmlPtr)
    HtmlWidget *htmlPtr;
{
    if ((htmlPtr->flags & REDRAW_PENDING) == 0
        && htmlPtr->tkwin != 0 && Tk_IsMapped(htmlPtr->tkwin)
            ) {
        Tcl_DoWhenIdle(HtmlRedrawCallback, (ClientData) htmlPtr);
        htmlPtr->flags |= REDRAW_PENDING;
    }
}

#endif /* _TCLHTML_ */

/*
** This routine is called in order to process a "configure" subcommand
** on the given html widget.
*/
int
ConfigureHtmlWidgetObj(interp, htmlPtr, objc, objv, flags, realign)
    Tcl_Interp *interp;                /* Write error message to this
                                        * interpreter */
    HtmlWidget *htmlPtr;               /* The Html widget to be configured */
    int objc;                          /* Number of configuration arguments */
    Tcl_Obj *CONST objv[];             /* Text of configuration arguments */
    int flags;                         /* Configuration flags */
    int realign;                       /* Always do a redraw if set */
{
    int rc;
    int i;
    int redraw = realign;              /* True if a redraw is required. */
    char *arg;

    /*
     * Scan thru the configuration options to see if we need to redraw ** the 
     * widget. 
     */
    for (i = 0; redraw == 0 && i < objc; i += 2) {
        int c;
        int n;
        arg = Tcl_GetStringFromObj(objv[i], &n);
        if (arg[0] != '-') {
            redraw = 1;
            break;
        }
        c = arg[1];
        if (c == 'c' && n > 4 && strncmp(arg, "-cursor", n) == 0) {
            /*
             * do nothing 
             */
        }
        else
            /*
             * The default case 
             */
        {
            redraw = 1;
        }
    }
#ifdef _TCLHTML_
    rc = TclConfigureWidgetObj(interp, htmlPtr, configSpecs, objc, objv,
                               (char *) htmlPtr, flags);
    if (rc != TCL_OK || redraw == 0) {
        return rc;
    }
#else
    {
        CONST char *sargv[20];
        CONST char **argv;
        if (objc >= 19) {
            argv = calloc(sizeof(char *), objc + 1);
            for (i = 0; i < objc; i++)
                argv[i] = Tcl_GetString(objv[i]);
            argv[i] = 0;
            rc = Tk_ConfigureWidget(interp, htmlPtr->tkwin, configSpecs, objc,
                                    argv, (char *) htmlPtr, flags);
            HtmlFree(argv);
        }
        else {
            for (i = 0; i < objc; i++)
                sargv[i] = Tcl_GetString(objv[i]);
            sargv[i] = 0;
            rc = Tk_ConfigureWidget(interp, htmlPtr->tkwin, configSpecs, objc,
                                    sargv, (char *) htmlPtr, flags);
        }
    }
    if (rc != TCL_OK || redraw == 0) {
        return rc;
    }
    memset(htmlPtr->fontValid, 0, sizeof(htmlPtr->fontValid));
    htmlPtr->apColor[COLOR_Normal] = htmlPtr->fgColor;
    htmlPtr->apColor[COLOR_Visited] = htmlPtr->oldLinkColor;
    htmlPtr->apColor[COLOR_Unvisited] = htmlPtr->newLinkColor;
    htmlPtr->apColor[COLOR_Selection] = htmlPtr->selectionColor;
    htmlPtr->apColor[COLOR_Background] = Tk_3DBorderColor(htmlPtr->border);
    Tk_SetBackgroundFromBorder(htmlPtr->tkwin, htmlPtr->border);
    if (htmlPtr->highlightWidth < 0) {
        htmlPtr->highlightWidth = 0;
    }
    if (htmlPtr->padx < 0) {
        htmlPtr->padx = 0;
    }
    if (htmlPtr->pady < 0) {
        htmlPtr->pady = 0;
    }
    if (htmlPtr->width < 100) {
        htmlPtr->width = 100;
    }
    if (htmlPtr->height < 100) {
        htmlPtr->height = 100;
    }
    if (htmlPtr->borderWidth < 0) {
        htmlPtr->borderWidth = 0;
    }
    htmlPtr->flags |=
            RESIZE_ELEMENTS | RELAYOUT | REDRAW_BORDER | RESIZE_CLIPWIN;
    HtmlRecomputeGeometry(htmlPtr);
    HtmlRedrawEverything(htmlPtr);
    ClearGcCache(htmlPtr);
#endif
    return rc;
}

int
HtmlNewWidget(clientData, interp, objc, objv)
    ClientData clientData;             /* Main window */
    Tcl_Interp *interp;                /* Current interpreter. */
    int objc;                          /* Number of arguments. */
    Tcl_Obj *CONST objv[];             /* Argument strings. */
{

    HtmlWidget *htmlPtr;
    Tk_Window new;
    Tk_Window clipwin;
    char *zClipwin;
    Tk_Window tkwin = (Tk_Window) clientData;
    static int varId = 1;              /* Used to construct unique names */
    int n;
    char *arg1 = Tcl_GetStringFromObj(objv[1], &n);

#ifndef _TCLHTML_
    new = Tk_CreateWindowFromPath(interp, tkwin, arg1, (char *) NULL);
    if (new == NULL) {
        return TCL_ERROR;
    }
    zClipwin = HtmlAlloc(n + 3);
    if (zClipwin == 0) {
        Tk_DestroyWindow(new);
        return TCL_ERROR;
    }
    sprintf(zClipwin, "%s.x", arg1);
    clipwin = Tk_CreateWindowFromPath(interp, new, zClipwin, 0);
    if (clipwin == 0) {
        Tk_DestroyWindow(new);
        HtmlFree(zClipwin);
        return TCL_ERROR;
    }
#endif

    dbghtmlPtr = htmlPtr = HtmlAlloc(sizeof(HtmlWidget) + n + 1);
    memset(htmlPtr, 0, sizeof(HtmlWidget));
#ifdef _TCLHTML_
    htmlPtr->tkwin = 1;
#else
    htmlPtr->tkwin = new;
    htmlPtr->clipwin = clipwin;
    htmlPtr->zClipwin = zClipwin;
    htmlPtr->display = Tk_Display(new);
#endif
    htmlPtr->interp = interp;
    htmlPtr->zCmdName = (char *) &htmlPtr[1];
    strcpy(htmlPtr->zCmdName, arg1);
    htmlPtr->relief = TK_RELIEF_FLAT;
    htmlPtr->dirtyLeft = LARGE_NUMBER;
    htmlPtr->dirtyTop = LARGE_NUMBER;
    htmlPtr->flags = RESIZE_CLIPWIN;
    htmlPtr->varId = varId++;
    Tcl_CreateObjCommand(interp, htmlPtr->zCmdName,
                         HtmlWidgetObjCommand, (ClientData) htmlPtr,
                         HtmlCmdDeletedProc);
#ifndef _TCLHTML_
    Tcl_CreateObjCommand(interp, htmlPtr->zClipwin,
                         HtmlWidgetObjCommand, (ClientData) htmlPtr,
                         HtmlCmdDeletedProc);

    Tk_SetClass(new, "Html");
    Tk_SetClass(clipwin, "HtmlClip");
    Tk_CreateEventHandler(htmlPtr->tkwin,
                          ExposureMask | StructureNotifyMask | FocusChangeMask,
                          HtmlEventProc, (ClientData) htmlPtr);
    Tk_CreateEventHandler(htmlPtr->clipwin,
                          ExposureMask | StructureNotifyMask,
                          HtmlEventProc, (ClientData) htmlPtr);
    if (HtmlFetchSelectionPtr) {
        Tk_CreateSelHandler(htmlPtr->tkwin, XA_PRIMARY, XA_STRING,
                            HtmlFetchSelectionPtr, (ClientData) htmlPtr,
                            XA_STRING);
        Tk_CreateSelHandler(htmlPtr->clipwin, XA_PRIMARY, XA_STRING,
                            HtmlFetchSelectionPtr, (ClientData) htmlPtr,
                            XA_STRING);
    }
#endif /* _TCLHTML_ */

    if (ConfigureHtmlWidgetObj(interp, htmlPtr, objc - 2, objv + 2, 0, 1) !=
        TCL_OK) {
        goto error;
    }

    Tcl_InitHashTable(&htmlPtr->tokenHash, TCL_STRING_KEYS);
    htmlPtr->tokenCnt = Html_TypeCount;
#ifdef _TCLHTML_
    interp->result = arg1;
#else
    interp->result = Tk_PathName(htmlPtr->tkwin);
#endif
    return TCL_OK;

  error:
#ifndef _TCLHTML_
    Tk_DestroyWindow(htmlPtr->tkwin);
#endif
    return TCL_ERROR;
}

/*
** The following routine implements the Tcl "html" command.  This command
** is used to create new HTML widgets only.  After the widget has been
** created, it is manipulated using the widget command defined above.
*/
int
HtmlObjCommand(clientData, interp, objc, objv)
    ClientData clientData;             /* Main window */
    Tcl_Interp *interp;                /* Current interpreter. */
    int objc;                          /* Number of arguments. */
    Tcl_Obj *CONST objv[];             /* Argument strings. */
{
    int n;
    char *arg1, *zn, zs, *cmd;

    cmd = Tcl_GetString(objv[0]);

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                         cmd, " pathName ?options?\"", (char *) NULL);
        return TCL_ERROR;
    }
    arg1 = Tcl_GetStringFromObj(objv[1], &n);

    /*
     * If the first argument begins with ".", then it must be the ** name of
     * a new window the user wants to create. 
     */
    if (*arg1 == '.')
        return HtmlNewWidget(clientData, interp, objc, objv);

    return HtmlCommandObj(clientData, interp, objc, objv);
}

/*
** The following mess is used to define DLL_EXPORT.  DLL_EXPORT is
** blank except when we are building a Windows95/NT DLL from this
** library.  Some special trickery is necessary to make this wall
** work together with makeheaders.
*/
#if INTERFACE
#define DLL_EXPORT
#endif
#if defined(USE_TCL_STUBS) && defined(__WIN32__)
# undef DLL_EXPORT
# define DLL_EXPORT __declspec(dllexport)
#endif

#ifndef DLL_EXPORT
#define DLL_EXPORT
#endif

/*
** This routine is used to register the "html" command with the
** Tcl interpreter.  This is the only routine in this file with
** external linkage.
*/
extern Tcl_Command htmlcmdhandle;

#ifndef _TCLHTML_
int
HtmlXErrorHandler(dsp, ev)
    Display *dsp;
    XErrorEvent *ev;
{
    char buf[300];

/* #if ! defined(__WIN32__) */
#if 0
    XGetErrorText(dsp, ev->error_code, buf, 300);
    fprintf(stderr, "X-Error: %s\n", buf);
#endif

/*  if (dsp) abort();
  if (ev) abort();
  abort(); */
}

DLL_EXPORT int
Tkhtml_SafeInit(interp)
    Tcl_Interp *interp;
{
    return Tkhtml_Init(interp);
}

DLL_EXPORT int
Tkhtml_Init(interp)
    Tcl_Interp *interp;
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, "8.3", 0) == 0) {
        return TCL_ERROR;
    }
    if (Tk_InitStubs(interp, "8.3", 0) == 0) {
        return TCL_ERROR;
    }
#endif
    htmlcmdhandle = Tcl_CreateObjCommand(interp, "html", HtmlObjCommand,
                                         Tk_MainWindow(interp), 0);
    /*
     * Tcl_GlobalEval(interp,HtmlLib); 
     */
#ifdef DEBUG
    Tcl_LinkVar(interp, "HtmlTraceMask", (char *) &HtmlTraceMask, TCL_LINK_INT);
#endif
    Tcl_StaticPackage(interp, "Tkhtml", Tkhtml_Init, Tkhtml_SafeInit);
    Tcl_PkgProvide(interp, HTML_PKGNAME, HTML_PKGVERSION);
    XSetErrorHandler(HtmlXErrorHandler);
    return Htmlexts_Init(interp);
    return TCL_OK;
}
#endif
