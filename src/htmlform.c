static char const rcsid[] =
        "@(#) $Id: htmlform.c,v 1.33 2005/03/23 01:36:54 danielk1977 Exp $";

/*
** Routines used for processing HTML makeup for forms.
**
** This source code is released into the public domain by the author,
** D. Richard Hipp, on 2002 December 17.  Instead of a license, here
** is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
*/
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "html.h"

static void EmptyInput _ANSI_ARGS_((HtmlElement * pElem));

/*
** Create the window name for a child widget.  Space to hold the name
** is obtained from HtmlAlloc() and must be freed by the calling function.
*/
static char *
MakeWindowName(htmlPtr, pElem)
    HtmlWidget *htmlPtr;               /* The HTML widget */
    HtmlElement *pElem;                /* The input that needs a child widget 
                                        */
{
    int n;
    char *zBuf;

#ifdef _TCLHTML_
    zBuf = HtmlAlloc(20);
    strcpy(zBuf, ".bogus");
#else
    n = strlen(Tk_PathName(htmlPtr->clipwin));
    zBuf = HtmlAlloc(n + 20);
    sprintf(zBuf, "%s.x%d", Tk_PathName(htmlPtr->clipwin), pElem->input.cnt);
#endif
    return zBuf;
}

#ifdef _TCLHTML_
static void
SizeAndLink(HtmlWidget * htmlPtr, char *zWin, HtmlElement * pElem)
{
}
void
HtmlDeleteControls(HtmlWidget * htmlPtr)
{
}
#else

/*
** Unmap any input control that is currently mapped.
*/
void
HtmlUnmapControls(htmlPtr)
    HtmlWidget *htmlPtr;
{
    HtmlElement *p;

    for (p = htmlPtr->firstInput; p; p = p->input.pNext) {
        if (p->input.tkwin != 0 && Tk_IsMapped(p->input.tkwin)) {
            Tk_UnmapWindow(p->input.tkwin);
        }
    }
}

/*
** Map any control that should be visible according to the
** current scroll position.  At the same time, if any controls that
** should not be visible are mapped, unmap them.  After this routine
** finishes, all <INPUT> controls should be in their proper places
** regardless of where they might have been before.
**
** Return the number of controls that are currently visible.
*/
int
HtmlMapControls(htmlPtr)
    HtmlWidget *htmlPtr;
{
    HtmlElement *p;                    /* For looping over all controls */
    int x, y, w, h;                    /* Part of the virtual canvas that is
                                        * visible */
    int cnt = 0;                       /* Number of visible controls */

    x = htmlPtr->xOffset;
    y = htmlPtr->yOffset;
    w = Tk_Width(htmlPtr->clipwin);
    h = Tk_Height(htmlPtr->clipwin);
    for (p = htmlPtr->firstInput; p; p = p->input.pNext) {
        if (p->input.tkwin == 0)
            continue;
        if (p->input.y < y + h
            && p->input.y + p->input.h > y
            && p->input.x < x + w && p->input.x + p->input.w > x) {
            /*
             * The control should be visible.  Make is so if it isn't already 
             */
            Tk_MoveResizeWindow(p->input.tkwin,
                                p->input.x - x, p->input.y - y,
                                p->input.w, p->input.h);
            if (!Tk_IsMapped(p->input.tkwin)) {
                Tk_MapWindow(p->input.tkwin);
            }
            cnt++;
        }
        else {
            /*
             * This control should not be visible.  Unmap it. 
             */
            if (Tk_IsMapped(p->input.tkwin)) {
                Tk_UnmapWindow(p->input.tkwin);
            }
        }
    }
    return cnt;
}

/*
** Delete all input controls.  This happens when the HTML widget
** is cleared.
**
** When the TCL "exit" command is invoked, the order of operations
** here is very touchy.  
*/
void
HtmlDeleteControls(htmlPtr)
    HtmlWidget *htmlPtr;
{
    HtmlElement *p;                    /* For looping over all controls */
    Tcl_Interp *interp;                /* The interpreter */

    interp = htmlPtr->interp;
    p = htmlPtr->firstInput;
    htmlPtr->firstInput = 0;
    htmlPtr->lastInput = 0;
    htmlPtr->nInput = 0;
    if (p == 0 || htmlPtr->tkwin == 0)
        return;
    HtmlLock(htmlPtr);
    for (; p; p = p->input.pNext) {
        if (p->input.pForm && p->input.pForm->form.hasctl
            && htmlPtr->zFormCommand && htmlPtr->zFormCommand[0]
            && !Tcl_InterpDeleted(interp) && htmlPtr->clipwin) {
            Tcl_DString cmd;
            int result;
            char zBuf[60];
            Tcl_DStringInit(&cmd);
            Tcl_DStringAppend(&cmd, htmlPtr->zFormCommand, -1);
            sprintf(zBuf, " %d flush {}", p->input.pForm->form.id);
            Tcl_DStringAppend(&cmd, zBuf, -1);
            result = Tcl_GlobalEval(htmlPtr->interp, Tcl_DStringValue(&cmd));
            Tcl_DStringFree(&cmd);
            if (!Tcl_InterpDeleted(interp)) {
                if (result != TCL_OK) {
                    Tcl_AddErrorInfo(htmlPtr->interp,
                                     "\n    (-formcommand flush callback executed by html widget)");
                    Tcl_BackgroundError(htmlPtr->interp);
                }
                Tcl_ResetResult(htmlPtr->interp);
            }
            p->input.pForm->form.hasctl = 0;
        }
        if (p->input.tkwin) {
            if (htmlPtr->clipwin != 0)
                Tk_DestroyWindow(p->input.tkwin);
            p->input.tkwin = 0;
        }
        p->input.sized = 0;
    }
    HtmlUnlock(htmlPtr);
}

/*
** This routine is called when one of the child windows for a form
** wants to change its size.
*/
static void
HtmlInputRequestProc(clientData, tkwin)
    ClientData clientData;
    Tk_Window tkwin;
{
    HtmlElement *pElem = (HtmlElement *) clientData;
    if (pElem->base.type != Html_INPUT) {
        CANT_HAPPEN;
        return;
    }
    if (pElem->input.tkwin != tkwin) {
        CANT_HAPPEN;
        return;
    }
    pElem->input.w = Tk_ReqWidth(tkwin);
    pElem->input.h = Tk_ReqHeight(tkwin);
    if (pElem->input.htmlPtr && pElem->input.htmlPtr->tkwin != 0) {
        pElem->input.htmlPtr->flags |= RELAYOUT;
        HtmlScheduleRedraw(pElem->input.htmlPtr);
    }
}

/*
** This routine is called when another entity takes over geometry
** management for a widget corresponding to an input element.
*/
static void
HtmlInputLostSlaveProc(clientData, tkwin)
    ClientData clientData;
    Tk_Window tkwin;
{
    HtmlElement *pElem = (HtmlElement *) clientData;
    if (pElem->base.type != Html_INPUT) {
        CANT_HAPPEN;
        return;
    }
    if (pElem->input.tkwin != tkwin) {
        CANT_HAPPEN;
        return;
    }
    EmptyInput(pElem);
    if (pElem->input.htmlPtr && pElem->input.htmlPtr->tkwin != 0) {
        pElem->input.htmlPtr->flags |= RELAYOUT;
        HtmlScheduleRedraw(pElem->input.htmlPtr);
    }
}

/*
** This routine catches DestroyNotify events on a INPUT window so
** that we will know the window is been deleted.
*/
static void
HtmlInputEventProc(clientData, eventPtr)
    ClientData clientData;
    XEvent *eventPtr;
{
    HtmlElement *pElem = (HtmlElement *) clientData;
    /*
     * if( pElem->base.type!=Html_INPUT ){ CANT_HAPPEN; return; } 
     */
    if (eventPtr->type == DestroyNotify) {
        EmptyInput(pElem);
        if (pElem->input.htmlPtr && pElem->input.htmlPtr->tkwin != 0) {
            pElem->input.htmlPtr->flags |= RELAYOUT;
            HtmlScheduleRedraw(pElem->input.htmlPtr);
        }
    }
}

/*
** The geometry manager for the HTML widget
*/
static Tk_GeomMgr htmlGeomType = {
    "html",                     /* Name */
    HtmlInputRequestProc,       /* Called when widget changes size */
    HtmlInputLostSlaveProc,     /* Called when someone else takes over
                                 * management */
};

/*
** zWin is the name of a child widget that is used to implement an
** input element.  Query Tk for information about this widget (such
** as its size) and put that information in the pElem structure
** that represents the input.
*/
static void
SizeAndLink(htmlPtr, zWin, pElem)
    HtmlWidget *htmlPtr;
    char *zWin;
    HtmlElement *pElem;
{
    pElem->input.tkwin =
            Tk_NameToWindow(htmlPtr->interp, zWin, htmlPtr->clipwin);
    if (pElem->input.tkwin == 0) {
        Tcl_ResetResult(htmlPtr->interp);
        EmptyInput(pElem);
    }
    else if (pElem->input.type == INPUT_TYPE_Hidden) {
        pElem->input.w = 0;
        pElem->input.h = 0;
        pElem->base.flags &= !HTML_Visible;
        pElem->base.style.flags |= STY_Invisible;
    }
    else {
        pElem->input.w = Tk_ReqWidth(pElem->input.tkwin);
        pElem->input.h =
                Tk_ReqHeight(pElem->input.tkwin) + htmlPtr->formPadding;
        pElem->base.flags |= HTML_Visible;
        pElem->input.htmlPtr = htmlPtr;
        Tk_ManageGeometry(pElem->input.tkwin, &htmlGeomType, pElem);
        Tk_CreateEventHandler(pElem->input.tkwin, StructureNotifyMask,
                              HtmlInputEventProc, pElem);
    }
    pElem->input.pNext = 0;
    if (htmlPtr->firstInput == 0) {
        htmlPtr->firstInput = pElem;
    }
    else {
        htmlPtr->lastInput->input.pNext = pElem;
    }
    htmlPtr->lastInput = pElem;
    pElem->input.sized = 1;
}

int
HtmlSizeWindow(clientData, interp, argc, argv)
    ClientData clientData;             /* The HTML widget */
    Tcl_Interp *interp;                /* The interpreter */
    int argc;                          /* Number of arguments */
    const char **argv;                 /* List of all arguments */
{
    HtmlWidget *htmlPtr = (HtmlWidget *) clientData;
    CONST char *zWin = argv[2];
    Tk_Window tkwin = Tk_NameToWindow(htmlPtr->interp, zWin, htmlPtr->clipwin);
    Tk_ManageGeometry(tkwin, &htmlGeomType, 0);
}

#endif /* _TCLHTML_ */

/*
** Return an appropriate type value for the given <INPUT> markup.
*/
static int
InputType(p)
    HtmlElement *p;
{
    int type = INPUT_TYPE_Unknown;
    char *z;
    int i;
    static struct {
        char *zName;
        int type;
    } types[] = {
        {
        "checkbox", INPUT_TYPE_Checkbox}, {
        "file", INPUT_TYPE_File}, {
        "hidden", INPUT_TYPE_Hidden}, {
        "image", INPUT_TYPE_Image}, {
        "password", INPUT_TYPE_Password}, {
        "radio", INPUT_TYPE_Radio}, {
        "reset", INPUT_TYPE_Reset}, {
        "submit", INPUT_TYPE_Submit}, {
        "text", INPUT_TYPE_Text}, {
        "name", INPUT_TYPE_Text}, {
        "textfield", INPUT_TYPE_Text}, {
    "button", INPUT_TYPE_Button},};

    switch (p->base.type) {
        case Html_INPUT:
            z = HtmlMarkupArg(p, "type", "text");
            if (z == 0) {
                break;
            }
            for (i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
                if (stricmp(types[i].zName, z) == 0) {
                    type = types[i].type;
                    break;
                }
            }
            break;
        case Html_SELECT:
            type = INPUT_TYPE_Select;
            break;
        case Html_TEXTAREA:
            type = INPUT_TYPE_TextArea;
            break;
        case Html_TABLE:
            type = INPUT_TYPE_Tktable;
            break;
        case Html_APPLET:
        case Html_IFRAME:
        case Html_EMBED:
            type = INPUT_TYPE_Applet;
            break;
        default:
            CANT_HAPPEN;
            break;
    }
    return type;
}

/*
** A Input element is the input.  Mark this element as being
** empty.  It has no widget and doesn't appear on the screen.
**
** This is called for HIDDEN inputs or when the -formcommand
** callback doesn't create the widget.
*/
static void
EmptyInput(pElem)
    HtmlElement *pElem;
{
    pElem->input.tkwin = 0;
    pElem->input.w = 0;
    pElem->input.h = 0;
    pElem->base.flags &= !HTML_Visible;
    pElem->base.style.flags |= STY_Invisible;
    pElem->input.sized = 1;
}

/* Append all text and space tokens between pStart and pEnd to
** the given Tcl_DString.
*/
static void
HtmlAppendText(str, pFirst, pEnd)
    Tcl_DString *str;                  /* Append the text here */
    HtmlElement *pFirst;               /* The first token */
    HtmlElement *pEnd;                 /* The last token */
{
    while (pFirst && pFirst != pEnd) {
        switch (pFirst->base.type) {
            case Html_Text:{
                    Tcl_DStringAppend(str, pFirst->text.zText, -1);
                    break;
                }
            case Html_Space:{
                    if (pFirst->base.flags & HTML_NewLine) {
                        Tcl_DStringAppend(str, "\n", 1);
                    }
                    else {
                        int cnt;
                        static char zSpaces[] = "                             ";
                        cnt = pFirst->base.count;
                        while (cnt > sizeof(zSpaces) - 1) {
                            Tcl_DStringAppend(str, zSpaces,
                                              sizeof(zSpaces) - 1);
                            cnt -= sizeof(zSpaces) - 1;
                        }
                        if (cnt > 0) {
                            Tcl_DStringAppend(str, zSpaces, cnt);
                        }
                    }
                    break;
                }
            default:
                /*
                 * Do nothing 
                 */
                break;
        }
        pFirst = pFirst->pNext;
    }
}

/*
** The "p" argument points to a <select>.  This routine scans all
** subsequent elements (up to the next </select>) looking for
** <option> tags.  For each option tag, it appends three elements
** to the "str" DString:
**
**     *        1 or 0 to indicated whether or not the element is
**              selected.
**
**     *        The value returned if this element is selected.
**
**     *        The text displayed for this element.
*/
static void
AddSelectOptions(str, p, pEnd)
    Tcl_DString *str;                  /* Add text here */
    HtmlElement *p;                    /* The <SELECT> markup */
    HtmlElement *pEnd;                 /* The </SELECT> markup */
{
    HtmlElement *pSave;
    while (p && p != pEnd && p->base.type != Html_EndSELECT) {
        if (p->base.type == Html_OPTION) {
            char *zValue;
            Tcl_DStringStartSublist(str);
            if (HtmlMarkupArg(p, "selected", 0) == 0) {
                Tcl_DStringAppend(str, "0 ", 2);
            }
            else {
                Tcl_DStringAppend(str, "1 ", 2);
            }
            zValue = HtmlMarkupArg(p, "value", 0);
            if (zValue) {
                Tcl_DStringAppendElement(str, zValue);
                pSave = 0;
            }
            else
                pSave = p;
          SelectDo:
            Tcl_DStringStartSublist(str);
            p = p->pNext;
            while (p && p != pEnd && p->base.type != Html_EndOPTION
                   && p->base.type != Html_OPTION
                   && p->base.type != Html_EndSELECT) {
                if (p->base.type == Html_Text) {
                    Tcl_DStringAppend(str, p->text.zText, -1);
                }
                else if (p->base.type == Html_Space) {
                    Tcl_DStringAppend(str, " ", 1);
                }
                p = p->pNext;
            }
            Tcl_DStringEndSublist(str);
            if (pSave) {
                p = pSave;
                pSave = 0;
                goto SelectDo;
            }
            Tcl_DStringEndSublist(str);
        }
        else {
            p = p->pNext;
        }
    }
}

/* BROKEN Return the idx'th elments of type n in form. */
HtmlElement *
HtmlFormIdx(htmlPtr, p, tag, idx, radio)
    HtmlWidget *htmlPtr;
    HtmlElement *p;
    int tag;
    int idx;
    int radio;
{
    int n;
    HtmlElement *q;
    if (p->base.type = Html_FORM) {
        switch (n) {
            case Html_FORM:
            case Html_SELECT:
            case Html_TEXTAREA:
            case Html_INPUT:
            case Html_OPTION:
                break;
        }
    }
    return 0;
}

/* Return the number of elments of type p in form. */
int
HtmlFormCount(htmlPtr, p, radio)
    HtmlWidget *htmlPtr;
    HtmlElement *p;
    int radio;
{
    HtmlElement *q = p;
    switch (p->base.type) {
        case Html_SELECT:
            return p->input.subid;
        case Html_TEXTAREA:
        case Html_INPUT:
            if (radio && p->input.type == INPUT_TYPE_Radio)
                return p->input.subid;
            return p->input.pForm->form.els;
        case Html_OPTION:
            while ((q = q->base.pPrev))
                if (q->base.type == Html_SELECT)
                    return q->input.subid;
    }
    return -1;
}

/* Add the DOM control information for form elements. */
int
HtmlAddFormInfo(htmlPtr, p)
    HtmlWidget *htmlPtr;
    HtmlElement *p;
{
    HtmlElement *q, *f;
    char *name, *z;
    int t;
    switch (p->base.type) {
        case Html_SELECT:
        case Html_TEXTAREA:
        case Html_INPUT:
            if (!(f = htmlPtr->formStart))
                return;
            p->input.pForm = htmlPtr->formStart;
            if (!f->form.pFirst)
                f->form.pFirst = p;
            if (htmlPtr->formElemLast)
                htmlPtr->formElemLast->input.pNext = p;
            htmlPtr->formElemLast = p;
            p->input.id = htmlPtr->inputIdx++;
            t = p->input.type = InputType(p);
            if (t == INPUT_TYPE_Radio) {
                if ((name = HtmlMarkupArg(p, "name", 0))) {
                    for (q = f->form.pFirst; q; q = q->input.pNext)
                        if ((z = HtmlMarkupArg(q, "name", 0))
                            && !strcmp(z, name)) {
                            p->input.subid = htmlPtr->radioIdx++;
                            break;
                        }
                    if (!q)
                        p->input.subid = (htmlPtr->radioIdx = 0);
                }
            }
            break;
        case Html_FORM:
            htmlPtr->formStart = p;
            p->form.id = htmlPtr->nForm++;
            break;
        case Html_EndTEXTAREA:
        case Html_EndSELECT:
        case Html_EndFORM:
            htmlPtr->formStart = 0;
            htmlPtr->inputIdx = 0;
            htmlPtr->radioIdx = 0;
            htmlPtr->formElemLast = 0;
            break;
        case Html_OPTION:
            if (htmlPtr->formElemLast && htmlPtr->formElemLast->base.type ==
                Html_SELECT)
                htmlPtr->formElemLast->input.subid++;
            break;
        default:
            break;
    }
}

void
HtmlAppendStyle(htmlPtr, cmd, pf)
    HtmlWidget *htmlPtr;
    Tcl_DString *cmd;
    HtmlElement *pf;
{
#ifndef _TCLHTML_
    char buf[BUFSIZ];
    CONST char *c1;
    CONST char *c2;
    int bg = pf->base.style.bgcolor;
    int fg = pf->base.style.color;
    XColor *cbg = htmlPtr->apColor[bg];
    XColor *cfg = htmlPtr->apColor[fg];
    c1 = Tk_NameOfColor(cfg);
    c2 = Tk_NameOfColor(cbg);
    Tcl_DStringAppend(cmd, "color ", -1);
    Tcl_DStringAppend(cmd, Clr2Name(c1), -1);
    Tcl_DStringAppend(cmd, " bgcolor ", -1);
    Tcl_DStringAppend(cmd, Clr2Name(c2), -1);
    Tcl_DStringAppend(cmd, " font {", -1);
    Tcl_DStringAppend(cmd, Tk_NameOfFont(HtmlGetFont(htmlPtr,
                                                     pf->base.style.font)), -1);
    Tcl_DStringAppend(cmd, "}", -1);
#endif
}

/*
** This routine implements the Sizer() function for <INPUT>,
** <SELECT> and <TEXTAREA> markup.
**
** A side effect of sizing these markups is that widgets are
** created to represent the corresponding input controls.
**
** The function normally returns 0.  But if it is dealing with
** a <SELECT> or <TEXTAREA> that is incomplete, 1 is returned.
** In that case, the sizer will be called again at some point in
** the future when more information is available.
*/
int
HtmlControlSize(htmlPtr, pElem)
    HtmlWidget *htmlPtr;
    HtmlElement *pElem;
{
    char *zWin;                        /* Name of child widget that
                                        * implements this input */
    int incomplete = 0;                /* True if data is incomplete */
    Tcl_DString cmd;                   /* The complete -formcommand callback */

    if (pElem->input.sized)
        return 0;
    if (pElem->input.type == INPUT_TYPE_Unknown)
        pElem->input.type = InputType(pElem);
    switch (pElem->input.type) {
        case INPUT_TYPE_Checkbox:
        case INPUT_TYPE_Hidden:
        case INPUT_TYPE_Image:
        case INPUT_TYPE_Radio:
        case INPUT_TYPE_Reset:
        case INPUT_TYPE_Submit:
        case INPUT_TYPE_Button:
        case INPUT_TYPE_Text:
        case INPUT_TYPE_Password:
        case INPUT_TYPE_File:{
                int result;
                char zToken[50];

                if (pElem->input.pForm == 0 || htmlPtr->zFormCommand == 0
                    || htmlPtr->zFormCommand[0] == 0) {
                    EmptyInput(pElem);
                    break;
                }
                Tcl_DStringInit(&cmd);
                Tcl_DStringAppend(&cmd, htmlPtr->zFormCommand, -1);
                sprintf(zToken, " %d input ", pElem->input.pForm->form.id);
                Tcl_DStringAppend(&cmd, zToken, -1);

/*
      Tcl_DStringStartSublist(&cmd);
      HtmlAppendStyle(htmlPtr,&cmd, pElem);
      Tcl_DStringEndSublist(&cmd);
*/
                Tcl_DStringAppend(&cmd, " ", -1);
                pElem->input.cnt = ++htmlPtr->nInput;
                zWin = MakeWindowName(htmlPtr, pElem);
                Tcl_DStringAppend(&cmd, zWin, -1);
                Tcl_DStringStartSublist(&cmd);
                HtmlAppendArglist(&cmd, pElem);
                Tcl_DStringEndSublist(&cmd);
                HtmlLock(htmlPtr);
                htmlPtr->inParse++;
                result = Tcl_GlobalEval(htmlPtr->interp,
                                        Tcl_DStringValue(&cmd));
                htmlPtr->inParse--;
                Tcl_DStringFree(&cmd);
                if (!HtmlUnlock(htmlPtr)) {
                    pElem->form.hasctl = 1;
                    if (result != TCL_OK) {
                        Tcl_AddErrorInfo(htmlPtr->interp,
                                         "\n    (-formcommand input callback executed by html widget)");
                        Tcl_BackgroundError(htmlPtr->interp);
                    }
                    SizeAndLink(htmlPtr, zWin, pElem);
                }
                HtmlFree(zWin);
                break;
            }
        case INPUT_TYPE_Select:{
                int result;
                char zToken[50];

                if (pElem->input.pForm == 0 || htmlPtr->zFormCommand == 0
                    || htmlPtr->zFormCommand[0] == 0) {
                    EmptyInput(pElem);
                    break;
                }
                Tcl_DStringInit(&cmd);
                Tcl_DStringAppend(&cmd, htmlPtr->zFormCommand, -1);
                sprintf(zToken, " %d select ", pElem->input.pForm->form.id);
                Tcl_DStringAppend(&cmd, zToken, -1);
                Tcl_DStringStartSublist(&cmd);
                HtmlAppendStyle(htmlPtr, &cmd, pElem);
                Tcl_DStringEndSublist(&cmd);
                Tcl_DStringAppend(&cmd, " ", -1);
                pElem->input.cnt = ++htmlPtr->nInput;
                zWin = MakeWindowName(htmlPtr, pElem);
                Tcl_DStringAppend(&cmd, zWin, -1);
                Tcl_DStringStartSublist(&cmd);
                HtmlAppendArglist(&cmd, pElem);
                Tcl_DStringEndSublist(&cmd);
                Tcl_DStringStartSublist(&cmd);
                AddSelectOptions(&cmd, pElem, pElem->input.pEnd);
                Tcl_DStringEndSublist(&cmd);
                HtmlLock(htmlPtr);
                htmlPtr->inParse++;
                result = Tcl_GlobalEval(htmlPtr->interp,
                                        Tcl_DStringValue(&cmd));
                htmlPtr->inParse--;
                Tcl_DStringFree(&cmd);
                if (!HtmlUnlock(htmlPtr)) {
                    SizeAndLink(htmlPtr, zWin, pElem);
                    if (result != TCL_OK) {
                        Tcl_AddErrorInfo(htmlPtr->interp,
                                         "\n    (-formcommand select callback executed by html widget)");
                        Tcl_BackgroundError(htmlPtr->interp);
                    }
                }
                HtmlFree(zWin);
                break;
            }
        case INPUT_TYPE_TextArea:{
                int result;
                char zToken[50];

                if (pElem->input.pForm == 0 || htmlPtr->zFormCommand == 0
                    || htmlPtr->zFormCommand[0] == 0) {
                    EmptyInput(pElem);
                    break;
                }
                Tcl_DStringInit(&cmd);
                Tcl_DStringAppend(&cmd, htmlPtr->zFormCommand, -1);
                sprintf(zToken, " %d textarea ", pElem->input.pForm->form.id);
                Tcl_DStringAppend(&cmd, zToken, -1);
                Tcl_DStringStartSublist(&cmd);
                HtmlAppendStyle(htmlPtr, &cmd, pElem);
                Tcl_DStringEndSublist(&cmd);
                Tcl_DStringAppend(&cmd, " ", -1);
                pElem->input.cnt = ++htmlPtr->nInput;
                zWin = MakeWindowName(htmlPtr, pElem);
                Tcl_DStringAppend(&cmd, zWin, -1);
                Tcl_DStringStartSublist(&cmd);
                HtmlAppendArglist(&cmd, pElem);
                Tcl_DStringEndSublist(&cmd);
                Tcl_DStringStartSublist(&cmd);
                HtmlAppendText(&cmd, pElem, pElem->input.pEnd);
                Tcl_DStringEndSublist(&cmd);
                HtmlLock(htmlPtr);
                htmlPtr->inParse++;
                result = Tcl_GlobalEval(htmlPtr->interp,
                                        Tcl_DStringValue(&cmd));
                htmlPtr->inParse--;
                Tcl_DStringFree(&cmd);
                if (!HtmlUnlock(htmlPtr)) {
                    SizeAndLink(htmlPtr, zWin, pElem);
                    if (result != TCL_OK) {
                        Tcl_AddErrorInfo(htmlPtr->interp,
                                         "\n    (-formcommand textarea callback executed by html widget)");
                        Tcl_BackgroundError(htmlPtr->interp);
                    }
                }
                HtmlFree(zWin);
                break;
            }
        case INPUT_TYPE_Tktable:
        case INPUT_TYPE_Applet:{
                int result;

                if (htmlPtr->zAppletCommand == 0
                    || htmlPtr->zAppletCommand[0] == 0) {
                    EmptyInput(pElem);
                    break;
                }
                Tcl_DStringInit(&cmd);
                Tcl_DStringAppend(&cmd, htmlPtr->zAppletCommand, -1);
                Tcl_DStringAppend(&cmd, " ", 1);
                pElem->input.cnt = ++htmlPtr->nInput;
                zWin = MakeWindowName(htmlPtr, pElem);
                Tcl_DStringAppend(&cmd, zWin, -1);
                Tcl_DStringStartSublist(&cmd);
                if (pElem->input.type != INPUT_TYPE_Tktable) {
                    HtmlAppendArglist(&cmd, pElem);
                    Tcl_DStringEndSublist(&cmd);
                }
                else {
                    if (pElem->pNext && pElem->pNext->base.type == Html_TABLE) {
                        char buf[30];
                        HtmlAppendArglist(&cmd, pElem->pNext);
                        Tcl_DStringEndSublist(&cmd);
                        Tcl_DStringAppend(&cmd, " ", 1);
                        sprintf(buf, "%d", pElem->pNext->base.id);
                        Tcl_DStringAppend(&cmd, buf, -1);
                    }
                }
                HtmlLock(htmlPtr);
                htmlPtr->inParse++;
                result = Tcl_GlobalEval(htmlPtr->interp,
                                        Tcl_DStringValue(&cmd));
                htmlPtr->inParse--;
                Tcl_DStringFree(&cmd);
                if (!HtmlUnlock(htmlPtr)) {
                    SizeAndLink(htmlPtr, zWin, pElem);
                    if (result != TCL_OK) {
                        Tcl_AddErrorInfo(htmlPtr->interp,
                                         "\n    (-appletcommand callback executed by html widget)");
                        Tcl_BackgroundError(htmlPtr->interp);
                    }
                }
                HtmlFree(zWin);
                break;
            }
        default:{
                CANT_HAPPEN;
                pElem->base.flags &= ~HTML_Visible;
                pElem->base.style.flags |= STY_Invisible;
                pElem->input.tkwin = 0;
                break;
            }
    }
    return incomplete;
}

#if 0

/*
** The following array determines which characters can be put directly
** in a query string and which must be escaped.
*/
static char needEscape[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1,
};

#define NeedToEscape(C) ((C)>0 && (C)<127 && needEscape[(int)(C)])

/*
** Append to the given DString, an encoded version of the given
** text.
*/
static void
EncodeText(Tcl_DString * str, char *z)
{
    int i;
    while (*z) {
        for (i = 0; z[i] && !NeedToEscape(z[i]); i++) {
        }
        if (i > 0) {
            Tcl_DStringAppend(str, z, i);
        }
        z += i;
        while (*z && NeedToEscape(*z)) {
            if (*z == ' ') {
                Tcl_DStringAppend(str, "+", 1);
            }
            else if (*z == '\n') {
                Tcl_DStringAppend(str, "%0D%0A", 6);
            }
            else if (*z == '\r') {
                /*
                 * Ignore it... 
                 */
            }
            else {
                char zBuf[5];
                sprintf(zBuf, "%%%02X", 0xff & *z);
                Tcl_DStringAppend(str, zBuf, 3);
            }
            z++;
        }
    }
}
#endif
