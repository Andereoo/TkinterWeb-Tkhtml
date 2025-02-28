 /* 
 * So blatently ripped off from canvas, most comments are unchanged.
 * Peter MacDonald.  http://browsex.com        NOTE: This link was pasted 23 years ago.
 *
 * tkCanvPs.c --
 *
 *    This module provides Postscript output support for canvases,
 *    including the "postscript" widget command plus a few utility
 *    procedures used for generating Postscript.
 *
 * Copyright © 1991-1994 The Regents of the University of California.
 * Copyright © 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: htmlPs.c,v 1.8 2005/03/23 01:36:54 danielk1977 Exp $
 */

#include <ctype.h>
#include <math.h>
#include <time.h>
#include "html.h"
#define UCHAR(c) ((unsigned char) (c))
#if !defined(__cplusplus) && !defined(c_plusplus)
# define c_class class
#endif
#define DRAWBOX_NOBORDER     0x00000001
#define DRAWBOX_NOBACKGROUND 0x00000002

/*
 * See tkCanvas.h for key data structures used to implement canvases.
 */

/*
 * The following definition is used in generating postscript for images and
 * windows.
 */

typedef struct TkColormapData {    /* Hold color information for a window */
    int separated;        /* Whether to use separate color bands */
    int color;            /* Whether window is color or black/white */
    int ncolors;        /* Number of color values stored */
    XColor *colors;        /* Pixel value -> RGB mappings */
    int red_mask, green_mask, blue_mask;    /* Masks and shifts for each */
    int red_shift, green_shift, blue_shift;    /* color band */
} TkColormapData;

typedef struct PsPageSize{
    unsigned int width;
    unsigned int height;
} PsPageSize;

/*
 * One of the following structures is created to keep track of Postscript
 * output being generated. It consists mostly of information provided on the
 * widget command line.
 */

typedef struct TkPostscriptInfo {
    int x, y, width, height;    /* Area to print, in canvas pixel coordinates. */
    int x2, y2;            /* x+width and y+height. */
    char *pageXString;        /* String value of "-pagex" option or NULL. */
    char *pageYString;        /* String value of "-pagey" option or NULL. */
    double pageX, pageY;    /* Postscript coordinates (in points)
                             * corresponding to pageXString and
                             * pageYString. Don't forget that y-values
                             * grow upwards for Postscript! */
    char *pageWidthString;    /* Printed width of output. */
    char *pageHeightString;    /* Printed height of output. */
    double scale;            /* Scale factor for conversion: each pixel
                             * maps into this many points. */
    Tk_Anchor pageAnchor;    /* How to anchor bbox on Postscript page. */
    int rotate;                /* Non-zero means output should be rotated on
                             * page (landscape mode). */
    char *fontVar;            /* If non-NULL, gives name of global variable
                             * containing font mapping information.
                             * Malloc'ed. */
    char *colorVar;            /* If non-NULL, give name of global variable
                             * containing color mapping information.
                             * Malloc'ed. */
    char *colorMode;        /* Mode for handling colors: "monochrome",
                             * "gray", or "color".  Malloc'ed. */
    int colorLevel;            /* Numeric value corresponding to colorMode: 0
                             * for mono, 1 for gray, 2 for color. */
    char *fileName;            /* Name of file in which to write Postscript;
                            * NULL means return Postscript info as
                             * result. Malloc'ed. */
    char *channelName;        /* If -channel is specified, the name of the
                             * channel to use. */
    Tcl_Channel chan;        /* Open channel corresponding to fileName. */
    Tcl_HashTable fontTable;/* Hash table containing names of all font
                             * families used in output. The hash table
                             * values are not used. */
    int prepass;            /* Non-zero means that we're currently in the
                             * pre-pass that collects font information, so
                             * the Postscript generated isn't relevant. */
    int prolog;                /* Non-zero means output should contain the
                             * standard prolog in the header. Generated in
                             * library/mkpsenc.tcl, stored in the variable
                             * ::tk::ps_preamable [sic]. */
    Tk_Window tkwin;        /* Window to get font pixel/point transform from. */
    PsPageSize pageSize;
    int nobackground;        /* Non-zero means CSS "background-color" property is skipped */
    int noimages;            /* Non-zero means images are left blank */
    char *pageMode;
} TkPostscriptInfo;

/* The table below provides a template that's used to process arguments to the
 * canvas "postscript" command and fill in TkPostscriptInfo structures. */

static const Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_STRING, "-colormap",   NULL, NULL, "", Tk_Offset(TkPostscriptInfo, colorVar), 0, NULL},
    {TK_CONFIG_STRING, "-colormode",  NULL, NULL, "", Tk_Offset(TkPostscriptInfo, colorMode), 0, NULL},
    {TK_CONFIG_STRING, "-file",       NULL, NULL, "", Tk_Offset(TkPostscriptInfo, fileName), 0, NULL},
    {TK_CONFIG_STRING, "-channel",    NULL, NULL, "", Tk_Offset(TkPostscriptInfo, channelName), 0, NULL},
    {TK_CONFIG_STRING, "-fontmap",    NULL, NULL, "", Tk_Offset(TkPostscriptInfo, fontVar), 0, NULL},
    {TK_CONFIG_PIXELS, "-height",     NULL, NULL, "", Tk_Offset(TkPostscriptInfo, height), 0, NULL},
    {TK_CONFIG_ANCHOR, "-pageanchor", NULL, NULL, "", Tk_Offset(TkPostscriptInfo, pageAnchor), 0, NULL},
    {TK_CONFIG_STRING, "-pageheight", NULL, NULL, "", Tk_Offset(TkPostscriptInfo, pageHeightString), 0, NULL},
    {TK_CONFIG_STRING, "-pagesize",   NULL, NULL, "", Tk_Offset(TkPostscriptInfo, pageMode), 0, NULL},
    {TK_CONFIG_STRING, "-pagewidth",  NULL, NULL, "", Tk_Offset(TkPostscriptInfo, pageWidthString), 0, NULL},
    {TK_CONFIG_STRING, "-pagex",      NULL, NULL, "", Tk_Offset(TkPostscriptInfo, pageXString), 0, NULL},
    {TK_CONFIG_STRING, "-pagey",      NULL, NULL, "", Tk_Offset(TkPostscriptInfo, pageYString), 0, NULL},
    {TK_CONFIG_BOOLEAN, "-prolog",    NULL, NULL, "", Tk_Offset(TkPostscriptInfo, prolog), 0, NULL},
    {TK_CONFIG_BOOLEAN, "-nobg",      NULL, NULL, "", Tk_Offset(TkPostscriptInfo, nobackground), 0},
    {TK_CONFIG_BOOLEAN, "-noimages",  NULL, NULL, "", Tk_Offset(TkPostscriptInfo, noimages), 0},
    {TK_CONFIG_BOOLEAN, "-rotate",    NULL, NULL, "", Tk_Offset(TkPostscriptInfo, rotate), 0, NULL},
    {TK_CONFIG_PIXELS, "-width",      NULL, NULL, "", Tk_Offset(TkPostscriptInfo, width), 0, NULL},
    {TK_CONFIG_PIXELS, "-x",          NULL, NULL, "", Tk_Offset(TkPostscriptInfo, x), 0, NULL},
    {TK_CONFIG_PIXELS, "-y",          NULL, NULL, "", Tk_Offset(TkPostscriptInfo, y), 0, NULL},
    {TK_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0, NULL}
};
/* Forward declarations for functions defined later in this file: */

int HtmlGetPostscript(HtmlTree *, HtmlNode *, int , int , int , int , Tcl_Interp *, Tcl_Obj *, HtmlComputedValues *);
static int fill_quadPs(Tcl_Interp*, Tk_PostscriptInfo, Tcl_Obj*, XColor*, int, double, int, int, int, int, int, int);
static int fill_rectanglePs(Tcl_Interp*, Tk_PostscriptInfo, Tcl_Obj*, XColor*, int, double, int, int);
static int scaledHeight(TkPostscriptInfo *);
static void getLowerCorners(TkPostscriptInfo *);
static void getPageCentre(TkPostscriptInfo *);
static int GetPostscriptPoints(Tcl_Interp *, char *, double *);
static void    PostscriptBitmap(Tk_Window , Pixmap , int , int , int , int , Tcl_Obj *);
static inline Tcl_Obj *    GetPostscriptBuffer(Tcl_Interp *);

/*
 *--------------------------------------------------------------
 *
 * HtmlPostscript --
 *
 *    This function is invoked to process the "postscript" options of the
 *    widget command for canvas widgets. See the user documentation for
 *    details on what it does.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    See the user documentation.
 *
 *--------------------------------------------------------------
 */

    /* ARGSUSED */
int HtmlPostscript(
    ClientData clientData,             /* The HTML widget data structure */
    Tcl_Interp *interp,                /* Current interpreter. */
    int argc,                          /* Number of arguments. */
    Tcl_Obj *const argv[]              /* Argument strings. */
    )
{
    TkPostscriptInfo psInfo, *pPsInfo = &psInfo;
    Tk_PostscriptInfo oldInfoPtr;
    int result;
    const char *p;
    time_t now;
    HtmlTree *pTree = (HtmlTree *)clientData;
    HtmlCanvas *pCanvas = &pTree->canvas;
    Tk_Window tkwin = pTree->tkwin;
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    Tcl_DString buffer;
    Tcl_Obj *psObj, *preambleObj;
    int deltaX = 0, deltaY = 0;    /* Offset of lower-left corner of area to be
                 * marked up, measured in canvas units from the positioning point on the page (reflects
                 * anchor position). Initial values needed only to stop compiler warnings. */
    int nographics;
    double pagestotal;

    /*
     * Get the generic preamble. We only ever bother with the ASCII encoding;
     * the others just make life too complicated and never actually worked as
     * such.
     */

    result = Tcl_EvalEx(interp, "::tk::ensure_psenc_is_loaded", -1, TCL_EVAL_GLOBAL);
    if (result != TCL_OK) {
    return result;
    }
    preambleObj = Tcl_GetVar2Ex(interp, "::tk::ps_preamble", NULL, TCL_LEAVE_ERR_MSG);
    if (preambleObj == NULL) {
    return TCL_ERROR;
    }
    Tcl_IncrRefCount(preambleObj);
    Tcl_ResetResult(interp);
    psObj = Tcl_NewObj();
    
    Tk_MakeWindowExist(pTree->tkwin);

    /*
     * Initialize the data structure describing Postscript generation, then
     * process all the arguments to fill the data structure in.
     */

    oldInfoPtr = pTree->psInfo;
    pTree->psInfo = (Tk_PostscriptInfo) pPsInfo;
    psInfo.x = pCanvas->left;
    psInfo.y = pCanvas->top;
    psInfo.width = -1;
    psInfo.height = -1;
    psInfo.pageXString = NULL;
    psInfo.pageYString = NULL;
    psInfo.pageX = 72*4.25;
    psInfo.pageY = 72*5.5;
    psInfo.pageWidthString = NULL;
    psInfo.pageHeightString = NULL;
    psInfo.scale = 1.0;
    psInfo.pageAnchor = TK_ANCHOR_CENTER;
    psInfo.rotate = 0;
    psInfo.fontVar = NULL;
    psInfo.colorVar = NULL;
    psInfo.colorMode = NULL;
    psInfo.colorLevel = 0;
    psInfo.fileName = NULL;
    psInfo.channelName = NULL;
    psInfo.chan = NULL;
    psInfo.prepass = 0;
    psInfo.prolog = 1;
    psInfo.tkwin = tkwin;
    psInfo.pageMode = NULL;
    psInfo.noimages = 0;
    psInfo.nobackground = 0;
    Tcl_InitHashTable(&psInfo.fontTable, TCL_STRING_KEYS);
    
    result = Tk_ConfigureWidget(interp, tkwin, configSpecs, argc-2, (const char**)argv+2, (char*)&psInfo, TK_CONFIG_ARGV_ONLY|TK_CONFIG_OBJS);
    if (result != TCL_OK) {
        goto cleanup;
    }

    if (psInfo.width == -1) psInfo.width = pCanvas->right;
    if (psInfo.height == -1) psInfo.height = pCanvas->bottom;
    getLowerCorners(pPsInfo);

    if (psInfo.pageXString != NULL) {
        if (GetPostscriptPoints(interp, psInfo.pageXString, &psInfo.pageX) != TCL_OK) {
            goto cleanup;
        }
    }
    if (psInfo.pageYString != NULL) {
        if (GetPostscriptPoints(interp, psInfo.pageYString, &psInfo.pageY) != TCL_OK) {
            goto cleanup;
        }
    }
    if (psInfo.pageWidthString != NULL) {
        if (GetPostscriptPoints(interp, psInfo.pageWidthString, &psInfo.scale) != TCL_OK) {
            goto cleanup;
        }
        psInfo.scale /= psInfo.width;
    } else if (psInfo.pageHeightString != NULL) {
        if (GetPostscriptPoints(interp, psInfo.pageHeightString, &psInfo.scale) != TCL_OK) {
            goto cleanup;
        }
        psInfo.scale /= psInfo.height;
    } else {
        psInfo.scale = (72.0/25.4)*WidthMMOfScreen(Tk_Screen(tkwin));
        psInfo.scale /= WidthOfScreen(Tk_Screen(tkwin));
    }
    
    if (psInfo.pageMode != NULL) {
        const unsigned int pageLen = strlen(psInfo.pageMode);
        char wStr[pageLen/2-1], hStr[pageLen/2-1], buffer[pageLen]; // Sufficient size to hold
        double tempWidth, tempHeight;
        
        if (!strnicmp(psInfo.pageMode, "window", 6)) {
            psInfo.x = pTree->iScrollX; psInfo.y = pTree->iScrollY;
            psInfo.width = MIN(pTree->iCanvasWidth, psInfo.width);
            psInfo.height = MIN(pTree->iCanvasHeight, psInfo.height);
            psInfo.pageSize.width = psInfo.width * psInfo.scale;
            psInfo.pageSize.height = psInfo.height * psInfo.scale;
            getLowerCorners(pPsInfo);
            getPageCentre(pPsInfo);
            goto finish;
        }
        
        strcpy(buffer, psInfo.pageMode); // Copy the input string to a mutable array
        
        char *token = strtok(buffer, "x"); // Use strtok to split the string
        if (token != NULL) {
            strcpy(wStr, token);  // First token is the width
            token = strtok(NULL, "x");  // Get the next token
            if (token != NULL) strcpy(hStr, token);  // Second token is the height
        }
        char end = hStr[strlen(hStr)-1];
        if (!isdigit(end) && isdigit(wStr[strlen(wStr)-1]))
        {
            int len = strlen(wStr);
            wStr[len] = end;
            wStr[len + 1] = '\0';
        }
        if (GetPostscriptPoints(interp, wStr, &tempWidth) != TCL_OK) goto cleanup;
        if (GetPostscriptPoints(interp, hStr, &tempHeight) != TCL_OK) goto cleanup;
        
        psInfo.pageSize.width = (int)round(tempWidth);
        psInfo.pageSize.height = (int)round(tempHeight);
        assert(psInfo.pageSize.width > 0 && psInfo.pageSize.height > 0);
        getPageCentre(pPsInfo);
        
        pTree->options.pagination = scaledHeight(pPsInfo);
        pTree->options.forcewidth = 1; /* If a page size has been set, make sure layout width is set to it. */
        pTree->options.width = ceil(psInfo.pageSize.width / psInfo.scale);
        HtmlCallbackLayout(pTree, pTree->pRoot);
        pTree->isPrintedMedia = 1;
        HtmlCallbackRestyle(pTree, pTree->pRoot);
        HtmlCallbackForce(pTree);

        psInfo.width = pCanvas->right;
        psInfo.height = pCanvas->bottom;
        getLowerCorners(pPsInfo);
        
        pagestotal = Tk_PostscriptY(pPsInfo->y, (Tk_PostscriptInfo)pPsInfo)/psInfo.pageSize.height*pPsInfo->scale;
    } else {
        finish:
            pTree->isPrintedMedia = 1;
            HtmlCallbackRestyle(pTree, pTree->pRoot);
            HtmlCallbackForce(pTree); /* Force any pending style and/or layout operations to run. */
            pagestotal = 1;
    }
    
    switch (psInfo.pageAnchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_W:
        case TK_ANCHOR_SW:
            deltaX = 0;
            break;
        case TK_ANCHOR_N:
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_S:
            deltaX = -psInfo.width/2;
            break;
        case TK_ANCHOR_NE:
        case TK_ANCHOR_E:
        case TK_ANCHOR_SE:
            deltaX = -psInfo.width;
            break;
        }
        switch (psInfo.pageAnchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_N:
        case TK_ANCHOR_NE:
            deltaY = -psInfo.height;
            break;
        case TK_ANCHOR_W:
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_E:
            deltaY = -psInfo.height/2;
            break;
        case TK_ANCHOR_SW:
        case TK_ANCHOR_S:
        case TK_ANCHOR_SE:
            deltaY = 0;
            break;
    }

    if (psInfo.colorMode == NULL) {
        psInfo.colorLevel = 2;  // Default to color
    } else {
        if (strncmp(psInfo.colorMode, "m", 10) == 0)
            psInfo.colorLevel = 0;  // Monochrome
        else if (strncmp(psInfo.colorMode, "g", 4) == 0)
            psInfo.colorLevel = 1;  // Gray / grey
        else if (strncmp(psInfo.colorMode, "c", 5) == 0)
            psInfo.colorLevel = 2;  // Color / colour
        else {
            Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "bad color mode \"%s\": must be m (monochrome), g (gray), or c (color)",
                psInfo.colorMode));
            Tcl_SetErrorCode(interp, "TK", "HTML", "PS", "COLORMODE", NULL);
            result = TCL_ERROR;
            goto cleanup;
        }
    }

    if (psInfo.fileName != NULL) {
    /*
     * Check that -file and -channel are not both specified.
     */

    if (psInfo.channelName != NULL) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "can't specify both -file and -channel", -1));
        Tcl_SetErrorCode(interp, "TK", "HTML", "PS", "USAGE", NULL);
        result = TCL_ERROR;
        goto cleanup;
    }

    /*
     * Check that we are not in a safe interpreter. If we are, disallow
     * the -file specification.
     */

    if (Tcl_IsSafe(interp)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
            "can't specify -file in a safe interpreter", -1));
        Tcl_SetErrorCode(interp, "TK", "SAFE", "PS_FILE", NULL);
        result = TCL_ERROR;
        goto cleanup;
    }

    p = Tcl_TranslateFileName(interp, psInfo.fileName, &buffer);
    if (p == NULL) goto cleanup;
    psInfo.chan = Tcl_OpenFileChannel(interp, p, "w", 0666);
    Tcl_DStringFree(&buffer);
    if (psInfo.chan == NULL) goto cleanup;
    }

    if (psInfo.channelName != NULL) {
        int mode;

        /*
         * Check that the channel is found in this interpreter and that it is
         * open for writing.
         */

        psInfo.chan = Tcl_GetChannel(interp, psInfo.channelName, &mode);
        if (psInfo.chan == NULL) {
            result = TCL_ERROR;
            goto cleanup;
        }
        if (!(mode & TCL_WRITABLE)) {
            Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "channel \"%s\" wasn't opened for writing",
                psInfo.channelName));
            Tcl_SetErrorCode(interp, "TK", "HTML", "PS", "UNWRITABLE",NULL);
            result = TCL_ERROR;
            goto cleanup;
        }
    }
    nographics = (psInfo.noimages << 1) | psInfo.nobackground;

    /*
     * Make a pre-pass over all of the items, generating Postscript and then
     * throwing it away. The purpose of this pass is just to collect
     * information about all the fonts in use, so that we can output font
     * information in the proper form required by the Document Structuring
     * Conventions.
     */

    psInfo.prepass = 1;
    
    result = HtmlGetPostscript(pTree, 0, -1, -1, psInfo.prepass, nographics, interp, psObj, 0);
    
    /*
    * If an error just occurred. Just skip out of that loop. There's no
    * need to report the error now; it can be reported later (errors
    * can happen later that don't happen now, so we still have to
    * check for errors later anyway).
    */
    
    psInfo.prepass = 0;

    /*
     * Generate the header and prolog for the Postscript.
     */

    if (psInfo.prolog) {
        Tcl_AppendPrintfToObj(psObj,
            "%%!PS-Adobe-3.0 EPSF-3.0\n"
            "%%%%Creator: %s %s Widget\n", HTML_PKGNAME, HTML_PKGVERSION);

    #ifdef HAVE_PW_GECOS
        if (!Tcl_IsSafe(interp)) {
            struct passwd *pwPtr = getpwuid(getuid());    /* INTL: Native. */

            Tcl_AppendPrintfToObj(psObj,
                "%%%%For: %s\n", (pwPtr ? pwPtr->pw_gecos : "Unknown"));
            endpwent();
        }
    #endif /* HAVE_PW_GECOS */
        Tcl_AppendPrintfToObj(psObj,
            "%%%%Title: Window %s\n", Tk_PathName(tkwin));
        time(&now);
        Tcl_AppendPrintfToObj(psObj, "%%%%CreationDate: %s", ctime(&now));    /* INTL: Native. */
        
        if (!psInfo.rotate) {
            Tcl_AppendPrintfToObj(psObj,
                "%%%%BoundingBox: %d %d %d %d\n",
                (int) (psInfo.pageX + psInfo.scale*deltaX),
                (int) (psInfo.pageY + psInfo.scale*deltaY),
                (int) (psInfo.pageX + psInfo.scale*(deltaX + psInfo.width) + 1.0),
                (int) (psInfo.pageY + psInfo.scale*(deltaY + psInfo.height) + 1.0));
        } else {
            Tcl_AppendPrintfToObj(psObj,
                "%%%%BoundingBox: %d %d %d %d\n",
                (int) (psInfo.pageX - psInfo.scale*(deltaY+psInfo.height)),
                (int) (psInfo.pageY + psInfo.scale*deltaX),
                (int) (psInfo.pageX - psInfo.scale*deltaY + 1.0),
                (int) (psInfo.pageY + psInfo.scale*(deltaX + psInfo.width) + 1.0));
        }
        Tcl_AppendPrintfToObj(psObj,
            "%%%%Pages: %d\n"
            "%%%%DocumentData: Clean7Bit\n"
            "%%%%Orientation: %s\n",
            (int)ceil(pagestotal),
            psInfo.rotate ? "Landscape" : "Portrait");
            
        p = "%%%%DocumentNeededResources: font %s\n";
        for (hPtr = Tcl_FirstHashEntry(&psInfo.fontTable, &search);
            hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
            Tcl_AppendPrintfToObj(psObj, p,
                Tcl_GetHashKey(&psInfo.fontTable, hPtr));
            p = "%%%%+ font %s\n";
        }
        Tcl_AppendToObj(psObj, "%%EndComments\n\n", -1);

        /*
         * Insert the prolog
         */

        Tcl_AppendObjToObj(psObj, preambleObj);

        if (psInfo.chan != NULL) {
            if (Tcl_WriteObj(psInfo.chan, psObj) == -1) {
            channelWriteFailed:
            Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                "problem writing postscript data to channel: %s",
                Tcl_PosixError(interp)));
            result = TCL_ERROR;
            goto cleanup;
            }
            Tcl_DecrRefCount(psObj);
            psObj = Tcl_NewObj();
        }

        /*
         * Document setup:  set the color level and include fonts.
         */

        Tcl_AppendPrintfToObj(psObj, "%%%%BeginSetup\n/CL %d def\n", psInfo.colorLevel);
        
        for (hPtr = Tcl_FirstHashEntry(&psInfo.fontTable, &search);
            hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
            Tcl_AppendPrintfToObj(psObj,
                "%%%%IncludeResource: font %s\n",
                (char *) Tcl_GetHashKey(&psInfo.fontTable, hPtr));
        }
        Tcl_AppendToObj(psObj, "%%EndSetup\n\n", -1);

        /*
         * Page setup: move to page positioning point, rotate if needed, set
         * scale factor, offset for proper anchor position, and set clip
         * region, set height and width of page if needed.
         */
        if(psInfo.pageMode)
            Tcl_AppendPrintfToObj(psObj, 
                "<< /PageSize [%d %d] >> setpagedevice\n\n", psInfo.pageSize.width, psInfo.pageSize.height);
        
        if (psInfo.chan != NULL) {
            if (Tcl_WriteObj(psInfo.chan, psObj) == -1) {
            goto channelWriteFailed;
            }
            Tcl_DecrRefCount(psObj);
            psObj = Tcl_NewObj();
        }
    }
    
    HtmlNode *pBgRoot = pTree->pRoot;
    HtmlComputedValues *pBgRootV;
    
    if (pBgRoot) {
        pBgRootV = HtmlNodeComputedValues(pBgRoot);
        if (!pBgRootV->cBackgroundColor->xcolor && !pBgRootV->imZoomedBackgroundImage) {
            pBgRoot = HtmlNodeChild(pBgRoot, 1);
        }
        pBgRootV = HtmlNodeComputedValues(pBgRoot);
        if (!pBgRootV->cBackgroundColor->xcolor && !pBgRootV->imZoomedBackgroundImage) {
            pBgRoot = 0;
        }
    }
    
    int pagenum, page_h, pageYmin, pageYmax;
    if (psInfo.pageMode) page_h = scaledHeight(pPsInfo);

    /*
     * Iterate through all the items, having each relevant one draw itself.
     * Quit if any of the items returns an error.
     */
    
    result = TCL_OK;
    for (int i = 0; i < pagestotal; i++) {
        
        pagenum = 1 + i;
        
        /*
         * Page setup: move to page positioning point, rotate if needed, set
         * scale factor, offset for proper anchor position, and set clip
         * region.
         */
        
        if (psInfo.prolog) {
            Tcl_AppendPrintfToObj(psObj, "%%%%Page: %d %d\nsave\n", pagenum, pagenum);
            Tcl_AppendPrintfToObj(psObj, "%.1f %.1f translate\n", psInfo.pageX, psInfo.pageY);
            if (psInfo.pageMode)
                Tcl_AppendPrintfToObj(psObj, "0 %g translate ", 
                psInfo.pageSize.height * -(pagestotal - pagenum - pagestotal / 2 + 0.5));
            if (psInfo.rotate) Tcl_AppendToObj(psObj, "90 rotate\n", -1);
            Tcl_AppendPrintfToObj(psObj, "%.4g %.4g scale\n", psInfo.scale, psInfo.scale);
            Tcl_AppendPrintfToObj(psObj, "%d %d translate\n", deltaX - psInfo.x, deltaY);
            Tcl_AppendPrintfToObj(psObj,
                "%d %.15g moveto %d %.15g lineto %d %.15g lineto %d %.15g "
                "lineto closepath clip newpath\n",
                psInfo.x, Tk_PostscriptY(psInfo.y, (Tk_PostscriptInfo)pPsInfo),
                psInfo.x2, Tk_PostscriptY(psInfo.y, (Tk_PostscriptInfo)pPsInfo),
                psInfo.x2, Tk_PostscriptY(psInfo.y2, (Tk_PostscriptInfo)pPsInfo),
                psInfo.x, Tk_PostscriptY(psInfo.y2, (Tk_PostscriptInfo)pPsInfo));
        }
        if (psInfo.pageMode && pagestotal <= 1) {
            pageYmin = pTree->iScrollY; pageYmax = pageYmin+psInfo.height;
        } else if (psInfo.pageMode) {
            pageYmin = i*page_h; pageYmax = pagenum*page_h;
        } else
            pageYmin = pageYmax = -1;
        
        result = HtmlGetPostscript(
            pTree, pBgRoot, pageYmin, pageYmax, psInfo.prepass, nographics, interp, psObj, pBgRootV
        );
        
        if (result != TCL_OK) goto cleanup;
        // Output page-end information, such as commands to print the page
        else if (psInfo.prolog) Tcl_AppendToObj(psObj, "restore showpage\n\n", -1);
    }

    if (psInfo.chan != NULL) {
        if (Tcl_WriteObj(psInfo.chan, psObj) == -1) goto channelWriteFailed;
        Tcl_DecrRefCount(psObj);
        psObj = Tcl_NewObj();
    }

    /*
     * Output page-end information, such as document trailer stuff.
     */

    if (psInfo.prolog) {
        Tcl_AppendToObj(psObj, "%%Trailer\nend\n%%EOF\n", -1);

        if (psInfo.chan != NULL) {
            if (Tcl_WriteObj(psInfo.chan, psObj) == -1) {
                goto channelWriteFailed;
            }
        }
    }

    if (psInfo.chan == NULL) {
        Tcl_SetObjResult(interp, psObj);
        psObj = Tcl_NewObj();
    }

    /*
     * Clean up psInfo to release malloc'ed stuff.
     */

    cleanup:
        if (psInfo.pageXString != NULL) {
            ckfree(psInfo.pageXString);
        }
        if (psInfo.pageYString != NULL) {
            ckfree(psInfo.pageYString);
        }
        if (psInfo.pageWidthString != NULL) {
            ckfree(psInfo.pageWidthString);
        }
        if (psInfo.pageHeightString != NULL) {
            ckfree(psInfo.pageHeightString);
        }
        if (psInfo.fontVar != NULL) {
            ckfree(psInfo.fontVar);
        }
        if (psInfo.colorVar != NULL) {
            ckfree(psInfo.colorVar);
        }
        if (psInfo.colorMode != NULL) {
            ckfree(psInfo.colorMode);
        }
        if (psInfo.fileName != NULL) {
            ckfree(psInfo.fileName);
        }
        if ((psInfo.chan != NULL) && (psInfo.channelName == NULL)) {
            Tcl_Close(interp, psInfo.chan);
        }
        if (psInfo.channelName != NULL) {
            ckfree(psInfo.channelName);
        }
        if (psInfo.pageMode != NULL) {
            ckfree(psInfo.pageMode);
            pTree->options.forcewidth = pTree->options.pagination = 0;
            HtmlCallbackLayout(pTree, pTree->pRoot);
        }
        Tcl_DeleteHashTable(&psInfo.fontTable);
        pTree->psInfo = (Tk_PostscriptInfo) oldInfoPtr;
        pTree->isPrintedMedia = 0;
        HtmlCallbackRestyle(pTree, pTree->pRoot);
        Tcl_DecrRefCount(preambleObj);
        Tcl_DecrRefCount(psObj);
        return result;
}

static inline Tcl_Obj *GetPostscriptBuffer(Tcl_Interp *interp)
{
    Tcl_Obj *psObj = Tcl_GetObjResult(interp);

    if (Tcl_IsShared(psObj)) {
    psObj = Tcl_DuplicateObj(psObj);
    Tcl_SetObjResult(interp, psObj);
    }
    return psObj;
}

static int fill_quadPs(
    Tcl_Interp *interp,
    Tk_PostscriptInfo psInfo,
    Tcl_Obj *psObj,
    XColor *color,
    int x1, double y1,
    int x2, int y2,
    int x3, int y3,
    int x4, int y4)
{
    Tcl_AppendPrintfToObj(psObj,
        "%d %.15g moveto %d %d rlineto %d %d rlineto %d %d rlineto closepath\n",
        x1, y1, x2, y2, x3, y3, x4, y4
    );
    Tcl_ResetResult(interp);
    if (Tk_PostscriptColor(interp, psInfo, color) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
    Tcl_AppendToObj(psObj, "fill\n", -1);
    return TCL_OK;
}

static int fill_rectanglePs(
    Tcl_Interp *interp,
    Tk_PostscriptInfo psInfo,
    Tcl_Obj *psObj,
    XColor *color,
    int x, double y, int h, int w)
{
    fill_quadPs(interp, psInfo, psObj, color, x, y, w, 0, 0, h, -w, 0);
}

Tk_Window HtmlTreeTkwin(HtmlTree *pTree) /* Token for tree on whose behalf Postscript is being generated. */
{
    TkPostscriptInfo *p = (TkPostscriptInfo *) pTree->psInfo;
    return p->tkwin;
}

/*
 *--------------------------------------------------------------
 *
 * GetPostscriptPoints --
 *
 *    Given a string, returns the number of Postscript points corresponding
 *    to that string.
 *
 * Results:
 *    The return value is a standard Tcl return result. If TCL_OK is
 *    returned, then everything went well and the screen distance is stored
 *    at *doublePtr; otherwise TCL_ERROR is returned and an error message is
 *    left in the interp's result.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */

static int
GetPostscriptPoints(
    Tcl_Interp *interp,        /* Use this for error reporting. */
    char *string,        /* String describing a screen distance. */
    double *doublePtr)        /* Place to store converted result. */
{
    char *end;
    double d;

    d = strtod(string, &end);
    if (end == string) {
        goto error;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
        end++;
    }
    switch (*end) {
        case 'c':
            d *= 72.0/2.54;
            end++;
            break;
        case 'i':
            d *= 72.0;
            end++;
            break;
        case 'm':
            d *= 72.0/25.4;
            end++;
            break;
        case 0:
            break;
        case 'p':
            end++;
            break;
        default:
            goto error;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
        end++;
    }
    if (*end != 0) {
        goto error;
    }
    *doublePtr = d;
    return TCL_OK;

  error:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad distance \"%s\"", string));
    Tcl_SetErrorCode(interp, "TK", "HTML", "PS", "POINTS", NULL);
    return TCL_ERROR;
}

#ifdef _WIN32
    #include <windows.h>
#else /* !_WIN32 */
    #define GetRValue(rgb)    ((rgb & cdata->red_mask) >> cdata->red_shift)
    #define GetGValue(rgb)    ((rgb & cdata->green_mask) >> cdata->green_shift)
    #define GetBValue(rgb)    ((rgb & cdata->blue_mask) >> cdata->blue_shift)
#endif /* _WIN32 */

#if defined(_WIN32) || defined(MAC_OSX_TK)
static void
TkImageGetColor(
    TkColormapData *cdata,    /* Colormap data */
    unsigned long pixel,    /* Pixel value to look up */
    double *red, double *green, double *blue)
                /* Color data to return */
{
    *red   = (double) GetRValue(pixel) / 255.0;
    *green = (double) GetGValue(pixel) / 255.0;
    *blue  = (double) GetBValue(pixel) / 255.0;
}
#else /* ! (_WIN32 || MAC_OSX_TK) */
static void
TkImageGetColor(
    TkColormapData *cdata,    /* Colormap data */
    unsigned long pixel,    /* Pixel value to look up */
    double *red, double *green, double *blue)
                /* Color data to return */
{
    if (cdata->separated) {
    int r = GetRValue(pixel);
    int g = GetGValue(pixel);
    int b = GetBValue(pixel);

    *red   = cdata->colors[r].red / 65535.0;
    *green = cdata->colors[g].green / 65535.0;
    *blue  = cdata->colors[b].blue / 65535.0;
    } else {
    *red   = cdata->colors[pixel].red / 65535.0;
    *green = cdata->colors[pixel].green / 65535.0;
    *blue  = cdata->colors[pixel].blue / 65535.0;
    }
}
#endif /* _WIN32 || MAC_OSX_TK */
/*
 *--------------------------------------------------------------
 *
 * TkPostscriptImage --
 *
 *    This function is called to output the contents of an image in
 *    Postscript, using a format appropriate for the current color mode
 *    (i.e. one bit per pixel in monochrome, one byte per pixel in gray, and
 *    three bytes per pixel in color).
 *
 * Results:
 *    Returns a standard Tcl return value. If an error occurs then an error
 *    message will be left in interp->result. If no error occurs, then
 *    additional Postscript will be appended to interp->result.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */

int
TkPostscriptImage(
    Tcl_Interp *interp,
    Tk_Window tkwin,
    Tk_PostscriptInfo psInfo,    /* postscript info */
    XImage *ximage,        /* Image to draw */
    int x, int y,        /* First pixel to output */
    int width, int height)    /* Width and height of area */
{
    TkPostscriptInfo *pPsInfo = (TkPostscriptInfo *) psInfo;
    int xx, yy, band, maxRows;
    double red, green, blue;
    int bytesPerLine = 0, maxWidth = 0;
    int level = pPsInfo->colorLevel;
    Colormap cmap;
    int i, ncolors;
    Visual *visual;
    TkColormapData cdata;
    Tcl_Obj *psObj;

    if (pPsInfo->prepass) {
    return TCL_OK;
    }

    cmap = Tk_Colormap(tkwin);
    visual = Tk_Visual(tkwin);

    /*
     * Obtain information about the colormap, ie the mapping between pixel
     * values and RGB values. The code below should work for all Visual types.
     */

    ncolors = visual->map_entries;
    cdata.colors = ckalloc(sizeof(XColor) * ncolors);
    cdata.ncolors = ncolors;

    if (visual->c_class == DirectColor || visual->c_class == TrueColor) {
    cdata.separated = 1;
    cdata.red_mask = visual->red_mask;
    cdata.green_mask = visual->green_mask;
    cdata.blue_mask = visual->blue_mask;
    cdata.red_shift = 0;
    cdata.green_shift = 0;
    cdata.blue_shift = 0;

    while ((0x0001 & (cdata.red_mask >> cdata.red_shift)) == 0) {
        cdata.red_shift ++;
    }
    while ((0x0001 & (cdata.green_mask >> cdata.green_shift)) == 0) {
        cdata.green_shift ++;
    }
    while ((0x0001 & (cdata.blue_mask >> cdata.blue_shift)) == 0) {
        cdata.blue_shift ++;
    }

    for (i = 0; i < ncolors; i ++) {
        cdata.colors[i].pixel =
            ((i << cdata.red_shift) & cdata.red_mask) |
            ((i << cdata.green_shift) & cdata.green_mask) |
            ((i << cdata.blue_shift) & cdata.blue_mask);
    }
    } else {
    cdata.separated=0;
    for (i = 0; i < ncolors; i ++) {
        cdata.colors[i].pixel = i;
    }
    }

    if (visual->c_class == StaticGray || visual->c_class == GrayScale) {
    cdata.color = 0;
    } else {
    cdata.color = 1;
    }

    XQueryColors(Tk_Display(tkwin), cmap, cdata.colors, ncolors);

    /*
     * Figure out which color level to use (possibly lower than the one
     * specified by the user). For example, if the user specifies color with
     * monochrome screen, use gray or monochrome mode instead.
     */

    if (!cdata.color && level >= 2) {
    level = 1;
    }

    if (!cdata.color && cdata.ncolors == 2) {
    level = 0;
    }

    /*
     * Check that at least one row of the image can be represented with a
     * string less than 64 KB long (this is a limit in the Postscript
     * interpreter).
     */

    switch (level) {
    case 0: bytesPerLine = (width + 7) / 8;  maxWidth = 240000; break;
    case 1: bytesPerLine = width;         maxWidth = 60000;  break;
    default: bytesPerLine = 3 * width;         maxWidth = 20000;  break;
    }

    if (bytesPerLine > 60000) {
    Tcl_ResetResult(interp);
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
        "can't generate Postscript for images more than %d pixels wide",
        maxWidth));
    Tcl_SetErrorCode(interp, "TK", "HTML", "PS", "MEMLIMIT", NULL);
    ckfree(cdata.colors);
    return TCL_ERROR;
    }

    maxRows = 60000 / bytesPerLine;
    psObj = GetPostscriptBuffer(interp);

    for (band = height-1; band >= 0; band -= maxRows) {
    int rows = (band >= maxRows) ? maxRows : band + 1;
    int lineLen = 0;

    switch (level) {
    case 0:
        Tcl_AppendPrintfToObj(psObj, "%d %d 1 matrix {\n<", width, rows);
        break;
    case 1:
        Tcl_AppendPrintfToObj(psObj, "%d %d 8 matrix {\n<", width, rows);
        break;
    default:
        Tcl_AppendPrintfToObj(psObj, "%d %d 8 matrix {\n<", width, rows);
        break;
    }
    for (yy = band; yy > band - rows; yy--) {
        switch (level) {
        case 0: {
        /*
         * Generate data for image in monochrome mode. No attempt at
         * dithering is made--instead, just set a threshold.
         */

        unsigned char mask = 0x80;
        unsigned char data = 0x00;

        for (xx = x; xx< x+width; xx++) {
            TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
                &red, &green, &blue);
            if (0.30 * red + 0.59 * green + 0.11 * blue > 0.5) {
            data |= mask;
            }
            mask >>= 1;
            if (mask == 0) {
            Tcl_AppendPrintfToObj(psObj, "%02X", data);
            lineLen += 2;
            if (lineLen > 60) {
                lineLen = 0;
                Tcl_AppendToObj(psObj, "\n", -1);
            }
            mask = 0x80;
            data = 0x00;
            }
        }
        if ((width % 8) != 0) {
            Tcl_AppendPrintfToObj(psObj, "%02X", data);
            mask = 0x80;
            data = 0x00;
        }
        break;
        }
        case 1:
        /* Generate data in gray mode; in this case, take a weighted sum of the red, green, and blue values. */

        for (xx = x; xx < x+width; xx ++) {
            TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy), &red, &green, &blue);
            Tcl_AppendPrintfToObj(psObj, "%02X", (int) floor(0.5 + 255.0 * (0.30 * red + 0.59 * green + 0.11 * blue)));
            lineLen += 2;
            if (lineLen > 60) {
                lineLen = 0;
                Tcl_AppendToObj(psObj, "\n", -1);
            }
        }
        break;
        default:
        /* Finally, color mode. Here, just output the red, green, and blue values directly. */

        for (xx = x; xx < x+width; xx++) {
            TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
                &red, &green, &blue);
            Tcl_AppendPrintfToObj(psObj, "%02X%02X%02X",
                (int) floor(0.5 + 255.0 * red),
                (int) floor(0.5 + 255.0 * green),
                (int) floor(0.5 + 255.0 * blue));
            lineLen += 6;
            if (lineLen > 60) {
            lineLen = 0;
            Tcl_AppendToObj(psObj, "\n", -1);
            }
        }
        break;
        }
    }
    switch (level) {
    case 0: case 1:
        Tcl_AppendToObj(psObj, ">\n} image\n", -1); break;
    default:
        Tcl_AppendToObj(psObj, ">\n} false 3 colorimage\n", -1); break;
    }
    Tcl_AppendPrintfToObj(psObj, "0 %d translate\n", rows);
    }
    ckfree(cdata.colors);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TextToPostscript --
 *
 *    This function is called to generate Postscript for text items.
 *
 * Results:
 *    The return value is a standard Tcl result. If an error occurs in
 *    generating Postscript then an error message is left in the interp's
 *    result, replacing whatever used to be there. If no error occurs, then
 *    Postscript for the item is appended to the result.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */
int TextToPostscript(Tk_PostscriptInfo psInfo, const char *z, int n, int x, int y, int prepass, HtmlNode *pNode, Tcl_Interp *interp)
{
    float anchor;
    const char *justify;
    Tcl_Obj *psObj;
    Tcl_InterpState interpState;
    int w, h;
    Tk_TextLayout tl;

    HtmlComputedValues *pV = HtmlNodeComputedValues(pNode);

    // Make our working space.
    psObj = Tcl_NewObj();
    interpState = Tcl_SaveInterpState(interp, TCL_OK);

    // Generate postscript.
    Tcl_ResetResult(interp);
    if (Tk_PostscriptFont(interp, psInfo, pV->fFont->tkfont) != TCL_OK) goto error;
    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));

    if (prepass) goto done;

    Tcl_ResetResult(interp);
    if (Tk_PostscriptColor(interp, psInfo, pV->cColor->xcolor) != TCL_OK) goto error;
    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
    
    switch (pV->eTextAlign) {
        case CSS_CONST_CENTER: anchor = 0.15; justify = "0.5"; break;
        case CSS_CONST_RIGHT:  anchor = 0.30; justify = "1";   break;
        default:               anchor = 0;    justify = "0";   break;
    }
    Tk_FontMetrics fm = pV->fFont->metrics;

    // Angle, horizontal and vertical positions to render at
    Tcl_AppendPrintfToObj(psObj, "0 %d %.15g [\n", x, Tk_PostscriptY(y, psInfo));
    Tcl_ResetResult(interp);
    tl = Tk_ComputeTextLayout(pV->fFont->tkfont, z, n, 0, 0, 0, &w, &h);
    Tk_TextLayoutToPostscript(interp, tl);
    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp)); // How far apart two lines of text in the same font
    Tcl_AppendPrintfToObj(psObj, "] %d %g 1 %s false DrawText\n", fm.linespace, anchor, justify);

    // Plug the accumulated postscript back into the result.
    done:
        (void) Tcl_RestoreInterpState(interp, interpState);
        Tcl_AppendObjToObj(Tcl_GetObjResult(interp), psObj);
        Tcl_DecrRefCount(psObj);
        return TCL_OK;

    error:
        Tcl_DiscardInterpState(interpState);
        Tcl_DecrRefCount(psObj);
        return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * ImageToPostscript --
 *
 *    This function is called to generate Postscript for image items.
 *
 * Results:
 *    The return value is a standard Tcl result. If an error occurs in
 *    generating Postscript then an error message is left in interp->result,
 *    replacing whatever used to be there. If no error occurs, then
 *    Postscript for the item is appended to the result.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */
int ImageToPostscript(HtmlTree *pTree, HtmlImage2 *pImage, int x, int y, int prepass, HtmlNode *pNode, Tcl_Interp *interp)
{
    Tk_Window win = HtmlTreeTkwin(pTree);
    Tk_PostscriptInfo psInfo = pTree->psInfo;
    int w, h;

    if (pImage == NULL) { /* Image item without actual image specified. */
        return TCL_OK;
    }
    HtmlImageSize(pImage, &w, &h);

    if (!prepass) {
        Tcl_Obj *psObj = Tcl_GetObjResult(interp);

        if (Tcl_IsShared(psObj)) {
            psObj = Tcl_DuplicateObj(psObj);
            Tcl_SetObjResult(interp, psObj);
        }

        Tcl_AppendPrintfToObj(psObj, "%d %.15g translate\n", x, Tk_PostscriptY(y, psInfo)-h);
    }

    return Tk_PostscriptImage(HtmlImageImage(pImage), interp, win, psInfo, 0, 0, w, h, prepass);
}

/*
 *--------------------------------------------------------------
 *
 * BoxToPostscript --
 *
 *    This function is called to generate Postscript for rectangle and oval
 *    items.
 *
 * Results:
 *    The return value is a standard Tcl result. If an error occurs in
 *    generating Postscript then an error message is left in the interp's
 *    result, replacing whatever used to be there. If no error occurs, then
 *    Postscript for the rectangle is appended to the result.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */
int BoxToPostscript(HtmlTree *pTree, int x, int y, int w, int h, int prepass, HtmlNode *pNode, int f, Tcl_Interp *interp, HtmlComputedValues *pV)
{
    Tcl_Obj *psObj;
    double y1, y2;
    Tcl_InterpState interpState;
    Tk_PostscriptInfo psInfo = pTree->psInfo;

    y1 = Tk_PostscriptY(y, psInfo);
    y2 = Tk_PostscriptY(y + h, psInfo);
    
    // Make our working space.
    psObj = Tcl_NewObj();
    interpState = Tcl_SaveInterpState(interp, TCL_OK);
    
    // Generate a string that creates a path for the solid background, if required.
    if (0 == (f & DRAWBOX_NOBACKGROUND) && pV->cBackgroundColor->xcolor) {
        // First draw the filled area of the rectangle.
        if (fill_rectanglePs(interp, psInfo, psObj, pV->cBackgroundColor->xcolor,
            x, y1, y2 - y1, w
        ) != TCL_OK) goto error;
    }
    /* Figure out the widths of the top, bottom, right and left borders */
    int tw = ((pV->eBorderTopStyle != CSS_CONST_NONE) ? pV->border.iTop :0);
    int bw = ((pV->eBorderBottomStyle != CSS_CONST_NONE) ? pV->border.iBottom :0);
    int rw = ((pV->eBorderRightStyle != CSS_CONST_NONE) ? pV->border.iRight :0);
    int lw = ((pV->eBorderLeftStyle != CSS_CONST_NONE) ? pV->border.iLeft :0);
    int ow = ((pV->eOutlineStyle != CSS_CONST_NONE) ? pV->iOutlineWidth :0);
    
    int bg_x = x + lw;    /* Drawable x coord for background */
    int bg_y = y + tw;    /* Drawable y coord for background */
    int bg_w = w - lw - rw;    /* Width of background rectangle */
    int bg_h = h - tw - bw;    /* Height of background rectangle */

    /* Figure out the colors of the top, bottom, right and left borders */
    XColor *tc = pV->cBorderTopColor->xcolor;
    XColor *rc = pV->cBorderRightColor->xcolor;
    XColor *bc = pV->cBorderBottomColor->xcolor;
    XColor *lc = pV->cBorderLeftColor->xcolor;
    XColor *oc = pV->cOutlineColor->xcolor;
    
    if (0 == (f & DRAWBOX_NOBORDER)) {
        if (tw > 0 && tc) {  /* Top border */
            if (fill_quadPs(interp, psInfo, psObj, tc,
                x, y1, rw, -tw, w-lw-rw, 0, lw, tw
            ) != TCL_OK) goto error;
        }
        if (bw > 0 && bc) {  /* Bottom border, if required */
            if (fill_quadPs(interp, psInfo, psObj, bc,
                x, y2, lw, bw, w-lw-rw, 0, rw, -bw
            ) != TCL_OK) goto error;
        }
        if (lw > 0 && lc) {  /* Left border, if required */
            if (fill_quadPs(interp, psInfo, psObj, lc,
                x, y2, lw, tw, 0, h-tw-bw, -lw, bw
            ) != TCL_OK) goto error;
        }
        if (rw > 0 && rc) {  /* Right border, if required */
            if (fill_quadPs(interp, psInfo, psObj, rc,
                x+w, y2, -rw, tw, 0, h-tw-bw, rw, bw
            ) != TCL_OK) goto error;
        }
    }
    if (0 == (f & DRAWBOX_NOBACKGROUND) && pV->imZoomedBackgroundImage) { /* Image background, if required. */
        Tk_Window win = HtmlTreeTkwin(pTree);
        int iWidth, iHeight, eR = pV->eBackgroundRepeat;
        HtmlImageSize(pV->imZoomedBackgroundImage, &iWidth, &iHeight);

        if (iWidth > 0 && iHeight > 0) {
            int iPosX, iPosY;
            iPosX = pV->iBackgroundPositionX;
            iPosY = pV->iBackgroundPositionY;
            if (eR != CSS_CONST_REPEAT && eR != CSS_CONST_REPEAT_X) {
                int draw_x1 = MAX(bg_x, iPosX);
                int draw_x2 = MIN(bg_x + bg_w, iPosX + iWidth);
                bg_x = draw_x1;
                bg_w = draw_x2 - draw_x1;
            } 
            if (eR != CSS_CONST_REPEAT && eR != CSS_CONST_REPEAT_Y) {
                int draw_y1 = MAX(bg_y, iPosY);
                int draw_y2 = MIN(bg_y + bg_h, iPosY + iHeight);
                bg_y = draw_y1;
                bg_h = draw_y2 - draw_y1;
            }
            Tcl_AppendPrintfToObj(psObj, "%d %.15g translate\n", bg_x, Tk_PostscriptY(bg_y, psInfo)-bg_h);
            if (Tk_PostscriptImage(HtmlImageImage(pV->imZoomedBackgroundImage), interp, 
                win, psInfo, iPosX, iPosY, bg_w, bg_h, prepass
            ) != TCL_OK) goto error;
            Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
        }
    }
    // Plug the accumulated postscript back into the result.
    done:
        (void) Tcl_RestoreInterpState(interp, interpState);
        Tcl_AppendObjToObj(Tcl_GetObjResult(interp), psObj);
        Tcl_DecrRefCount(psObj);
        return TCL_OK;

    error:
        Tcl_DiscardInterpState(interpState);
        Tcl_DecrRefCount(psObj);
        return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * LineToPostscript --
 *
 *     This function is used to draw a CANVAS_LINE primitive to the 
 *     drawable.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
int LineToPostscript(Tk_PostscriptInfo psInfo, int x, int y, int w, int y_linethrough, int y_underline, int prepass, HtmlNode *pNode, Tcl_Interp *interp)
{
    Tcl_InterpState interpState;
    XColor *xcolor;
    Tcl_Obj *psObj;
    int yrel;

    switch (HtmlNodeComputedValues(pNode)->eTextDecoration) {
        case CSS_CONST_LINE_THROUGH:
            yrel = y + y_linethrough; 
            break;
        case CSS_CONST_UNDERLINE:
            yrel = y + y_underline; 
            break;
        case CSS_CONST_OVERLINE:
            yrel = y; 
            break;
        default: goto done;
    }
    // Make our working space.
    psObj = Tcl_NewObj();
    interpState = Tcl_SaveInterpState(interp, TCL_OK);
    Tcl_AppendPrintfToObj(psObj, "%d %.15g moveto %d 0 rlineto closepath ",
        x, Tk_PostscriptY(yrel, psInfo), w
    );
    Tcl_ResetResult(interp);
    xcolor = HtmlNodeComputedValues(pNode)->cColor->xcolor;
    if (Tk_PostscriptColor(interp, psInfo, xcolor) != TCL_OK) goto error;
    Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
    Tcl_AppendToObj(psObj, "stroke\n", -1);
    
    // Plug the accumulated postscript back into the result.

    done:
        (void) Tcl_RestoreInterpState(interp, interpState);
        Tcl_AppendObjToObj(Tcl_GetObjResult(interp), psObj);
        Tcl_DecrRefCount(psObj);
        return TCL_OK;

    error:
        Tcl_DiscardInterpState(interpState);
        Tcl_DecrRefCount(psObj);
        return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * WinItemToPostscript --
 *
 *    This function is called to generate Postscript for window items.
 *
 * Results:
 *    The return value is a standard Tcl result. If an error occurs in
 *    generating Postscript then an error message is left in interp->result,
 *    replacing whatever used to be there. If no error occurs, then
 *    Postscript for the item is appended to the result.
 *
 * Side effects:
 *    None.
 *
 *--------------------------------------------------------------
 */
int WinItemToPostscript(HtmlTree *pTree, int x, int y, Tk_Window win, int prepass, Tcl_Interp *interp)
{
    int w, h, result;
    XImage *ximage;
    #ifdef X_GetImage
    Tk_ErrorHandler handle;
    #endif
    Tcl_Obj *cmdObj, *psObj;

    if (prepass || win == NULL) { return TCL_OK; }

    w = Tk_Width(win);
    h = Tk_Height(win);
    Tcl_InterpState interpState = Tcl_SaveInterpState(interp, TCL_OK);

    /* Locate the subwindow within the wider window. */
    psObj = Tcl_ObjPrintf(
        "\n%%%% %s item (%s, %d x %d)\n"    /* Comment */
        "%d %.15g translate\n",        /* Position */
        Tk_Class(win), Tk_PathName(win), w, h, x, Tk_PostscriptY(y, pTree->psInfo)-h);

    /* First try if the widget has its own "postscript" command. If it exists, this will produce much better postscript than when a pixmap is used. */
    Tcl_ResetResult(interp);
    cmdObj = Tcl_ObjPrintf("%s postscript -prolog 0", Tk_PathName(win));
    Tcl_IncrRefCount(cmdObj);
    result = Tcl_EvalObjEx(interp, cmdObj, 0);
    Tcl_DecrRefCount(cmdObj);

    if (result == TCL_OK) {
        Tcl_AppendPrintfToObj(psObj,
            "50 dict begin\nsave\ngsave\n0 %d moveto %d 0 rlineto 0 -%d rlineto -%d 0 rlineto closepath\n"
            "1.000 1.000 1.000 setrgbcolor AdjustColor\nfill\ngrestore\n", h, w, h, w);
        Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
        Tcl_AppendToObj(psObj, "\nrestore\nend\n\n\n", -1);
        goto done;
    }
    /* If the window is off the screen it will generate a BadMatch/XError. We catch any BadMatch errors here */
    #ifdef X_GetImage
    handle = Tk_CreateErrorHandler(Tk_Display(win), BadMatch, X_GetImage, -1, xerrorhandler, win);
    #endif

    /* Generate an XImage from the window. We can then read pixel values out of the XImage. */
    ximage = XGetImage(Tk_Display(win), Tk_WindowId(win), 0, 0, (unsigned)w, (unsigned)h, AllPlanes, ZPixmap);

    #ifdef X_GetImage
    Tk_DeleteErrorHandler(handle);
    #endif

    if (ximage == NULL) { result = TCL_OK;
    } else {
        Tcl_ResetResult(interp);
        result = TkPostscriptImage(interp, win, pTree->psInfo, ximage, 0, 0, w, h);
        Tcl_AppendObjToObj(psObj, Tcl_GetObjResult(interp));
        XDestroyImage(ximage);
    }

    /* Plug the accumulated postscript back into the result. */
    done:
        if (result == TCL_OK) {
            (void) Tcl_RestoreInterpState(interp, interpState);
            Tcl_AppendObjToObj(Tcl_GetObjResult(interp), psObj);
        } else {
            Tcl_DiscardInterpState(interpState);
        }
        Tcl_DecrRefCount(psObj);
        return result;
}

static int scaledHeight(TkPostscriptInfo *psInfo) {
    return ceil(psInfo->pageSize.height / psInfo->scale);
}
static void getLowerCorners(TkPostscriptInfo *psInfo) {
    psInfo->x2 = psInfo->x + psInfo->width;
    psInfo->y2 = psInfo->y + psInfo->height;
}
static void getPageCentre(TkPostscriptInfo *psInfo) {
    if (!psInfo->pageXString) psInfo->pageX = psInfo->pageSize.width / 2;
    if (!psInfo->pageYString) psInfo->pageY = psInfo->pageSize.height / 2;
}
