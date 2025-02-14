/*
 * htmltable.c ---
 *
 *     This file contains code for layout of tables.
 *
 *--------------------------------------------------------------------------
 * Copyright (c) 2005 Eolas Technologies Inc.
 * All rights reserved.
 *
 * This Open Source project was made possible through the financial support
 * of Eolas Technologies Inc.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <ORGANIZATION> nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
static const char rcsid[] = "$Id: htmltable.c,v 1.124 2007/11/03 11:23:16 danielk1977 Exp $";


#include "htmllayout.h"

#define LOG if (pLayout->pTree->options.logcmd && !pLayout->minmaxTest)

struct TableCell {
    BoxContext box;
    int startrow;             /* Index of row cell starts at */
    int finrow;               /* Index of row cell ends at */
    int colspan;              /* Number of columns spanned by cell (often 1) */
    HtmlNode *pNode;          /* Node with "display:table-cell" */
};
typedef struct TableCell TableCell;

typedef struct CellReqWidth CellReqWidth;
struct CellReqWidth {
    int eType;
    union {
        int i;          /* For CELL_WIDTH_PIXELS */
        float f;        /* For CELL_WIDTH_PERCENT */
    } val;
};
#define CELL_WIDTH_AUTO    0
#define CELL_WIDTH_PIXELS  1
#define CELL_WIDTH_PERCENT 2

/*
 * Structure used whilst laying out tables. See HtmlTableLayout().
 */
struct TableData {
    HtmlNode *pNode;         /* <table> node */
    LayoutContext *pLayout;
    int border_spacing;      /* Pixel value of 'border-spacing' property */
    int availablewidth;      /* Width available between margins for table */

    /* 
     * Determined by:
     *
     *     tableCountCells()
     */
    int nCol;                /* Total number of columns in table */
    int nRow;                /* Total number of rows in table */

    /*
     * The following four arrays are populated by the two-pass algorithm
     * implemented by functions:
     *
     *     tableColWidthSingleSpan()
     *     tableColWidthMultiSpan()
     */
    int *aMaxWidth;          /* Maximum content width of each column */
    int *aMinWidth;          /* Minimum content width of each column */
    CellReqWidth *aReqWidth;       /* Widths requested via CSS */
    CellReqWidth *aSingleReqWidth; /* Widths requested by single span cells */
    
    int *aMaxHeight;
    int *aMinHeight;
    CellReqWidth *aReqHeight;
    CellReqWidth *aSingleReqHeight;

    /* 
     * Determined by:
     *
     *     tableCalculateCellWidths()
     */
    int *aWidth;             /* Actual widths of each column (calculated) */
    int *aHeight;

    int *aY;                 /* Top y-coord for each row+1, wrt table box */
    TableCell *aCell;

    int row;                 /* Current row */
    int y;                   /* y-coord to draw at */
    int x;                   /* x-coord to draw at */
    BoxContext *pBox;        /* Box to draw into */
    HtmlComputedValues *pDefaultProperties;
};
typedef struct TableData TableData;

/* The two types of callbacks made by tableIterate(). */
typedef int (CellCallback)(HtmlNode *, int, int, int, int, void *);
typedef int (RowCallback)(HtmlNode *, int, void *);

/* Iterate through each cell in each row of the table. */
static void tableIterate(HtmlTree*,HtmlNode*, CellCallback, RowCallback, void*);

/* Count the number of rows/columns in the table */
static CellCallback tableCountCells;

/* Populate the aMinWidth, aMaxWidth, aReqWidth and aSingleReqWidth array
 * members of the TableData structure.
 */
static CellCallback tableColWidthSingleSpan;
static CellCallback tableColWidthMultiSpan;

/* Figure out the actual column widths (TableData.aWidth[]). */
static void tableCalculateCellWidths(TableData *, int, int, int);

/* A row and cell callback (used together in a single iteration) to draw
 * the table content. All the actual drawing is done here. Everything
 * else is just about figuring out column widths.
 */
static CellCallback tableDrawCells;
static RowCallback tableDrawRow;

static void 
fixNodeProperties (TableData *pData, HtmlNode *pNode)
{
    HtmlElementNode *pElem = (HtmlElementNode *)pNode;
    if (!pElem->pPropertyValues) {
        if (!pData->pDefaultProperties) {
            HtmlTree *pTree = pData->pLayout->pTree;
            HtmlComputedValuesCreator sCreator;
            HtmlComputedValuesInit(pTree, pNode, 0, &sCreator);
            pData->pDefaultProperties = HtmlComputedValuesFinish(&sCreator);
        }
        pElem->pPropertyValues = pData->pDefaultProperties;
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * tableColWidthSingleSpan --
 *
 *     A tableIterate() callback to determine the following for each
 *     column in the table:
 * 
 *         * The minimum content width
 *         * The maximum content width
 *         * The requested width (may be in pixels, a percentage or "auto")
 *
 *     This function only considers cells that span a single column (either 
 *     colspan="1" cells, or cells with no explicit colspan value). A second
 *     tableIterate() loop, with tableColWidthMultiSpan() as the callback
 *     analyses the cells that span multiple columns.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Populates the following arrays:
 *
 *         TableData.aMinWidth[]
 *         TableData.aMaxWidth[]
 *         TableData.aSingleReqWidth[]
 *
 *---------------------------------------------------------------------------
 */
static int 
tableColWidthSingleSpan (HtmlNode *pNode, int col, int colspan, int row, int rowspan, void *pContext)
{
    TableData *pData = (TableData *)pContext;
    int *aMinWidth       = pData->aMinWidth;
    int *aMaxWidth       = pData->aMaxWidth;

    /* Because a cell originates in this column, it's min and max width
     * must be at least 1 pixel. It doesn't matter if the cell spans
     * multiple columns or not (gleaned from alternative CSS engine
     * implementation).
     */
    aMaxWidth[col] = MAX(aMaxWidth[col], 1);
    aMinWidth[col] = MAX(aMinWidth[col], 1);

    if (colspan == 1) {
        HtmlComputedValues *pV;
        BoxProperties box;
        int max;
        int min;

        /* Note: aReq is an alias for aSingleReqWidth, NOT aReqWidth.
         * aReqWidth is populated by the second analysis parse of
         * the table - the one that uses tableColWidthMultiSpan(). 
         */
        CellReqWidth *aReq = pData->aSingleReqWidth;

        /* Figure out the minimum and maximum widths of the content */
        fixNodeProperties(pData, pNode);
        pV = HtmlNodeComputedValues(pNode);
        blockMinMaxWidth(pData->pLayout, pNode, &min, &max);
        nodeGetBoxProperties(pData->pLayout, pNode, 0, &box);

        aMinWidth[col] = MAX(aMinWidth[col], min + box.iLeft + box.iRight);
        aMaxWidth[col] = MAX(aMaxWidth[col], max + box.iLeft + box.iRight);
        assert(aMinWidth[col] <= aMaxWidth[col]);
        
        if (pV->mask & PROP_MASK_WIDTH) {

            /* The computed value of the 'width' property is a percentage */
            float val = ((float)pV->iWidth) / 100.0; 
            switch (aReq[col].eType) {
                case CELL_WIDTH_AUTO:
                case CELL_WIDTH_PIXELS:
                    aReq[col].eType = CELL_WIDTH_PERCENT;
                    aReq[col].val.f = val;
                    break;
                case CELL_WIDTH_PERCENT:
                    aReq[col].val.f = MAX(aReq[col].val.f, val);
                    break;
            }
        } else if (pV->iWidth >= 0) {

            /* There is a pixel value for the 'width' property */
            int val = pV->iWidth + box.iLeft + box.iRight;
            switch (aReq[col].eType) {
                case CELL_WIDTH_AUTO:
                case CELL_WIDTH_PIXELS:
                    aReq[col].eType = CELL_WIDTH_PIXELS;
                    aReq[col].val.i = MAX(aReq[col].val.i, val);
                    aMaxWidth[col] = MAX(val, aMaxWidth[col]);
                    break;
                case CELL_WIDTH_PERCENT:
                    break;
            }

        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * logWidthsToTable --
 *
 *     This function is only used by LOG{...} blocks (i.e. to debug the
 *     widget internals). It appends a formatted HTML table to the 
 *     current value of pObj summarizing the values in the following arrays:
 *
 *         pData->aReqWidth
 *         pData->aMinWidth
 *         pData->aMaxWidth
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Appends to pObj.
 *
 *---------------------------------------------------------------------------
 */
static void
logWidthsToTable(TableData *pData, Tcl_Obj *pObj)
{
    int *aMinWidth = pData->aMinWidth;
    int *aMaxWidth = pData->aMaxWidth;
    CellReqWidth *aReqWidth = pData->aReqWidth;
    int i;

    Tcl_AppendToObj(pObj, 
        "<table><tr>"
        "  <th>Col Number"
        "  <th>Min Content Width"
        "  <th>Max Content Width"
        "  <th>Explicit Width"
        "  <th>Percentage Width", -1);

    for (i = 0; i < pData->nCol; i++) {
        int j;
        char zPercent[32];

        Tcl_AppendToObj(pObj, "<tr><td>", -1);
        Tcl_AppendObjToObj(pObj, Tcl_NewIntObj(i));

        for (j = 0; j < 3; j++) {
            int val = PIXELVAL_AUTO;
            switch (j) {
                case 0: val = aMinWidth[i]; break;
                case 1: val = aMaxWidth[i]; break;
                case 2:
                    if (aReqWidth[i].eType == CELL_WIDTH_PIXELS) {
                        val = aReqWidth[i].val.i;
                    } else {
                        val = PIXELVAL_AUTO;
                    }
                    break;
                default:
                    assert(0);
            }
            Tcl_AppendToObj(pObj, "<td>", -1);
            if (val != PIXELVAL_AUTO) {
                Tcl_AppendObjToObj(pObj, Tcl_NewIntObj(val));
                Tcl_AppendToObj(pObj, "px", -1);
            } else {
                Tcl_AppendToObj(pObj, "N/A", -1);
            }
        }

        Tcl_AppendToObj(pObj, "<td>", -1);
        if (aReqWidth[i].eType == CELL_WIDTH_PERCENT) {
            sprintf(zPercent, "%.2f%%", aReqWidth[i].val.f);
        } else {
            sprintf(zPercent, "N/A");
        }
        Tcl_AppendToObj(pObj, zPercent, -1);
    }
    Tcl_AppendToObj(pObj, "</table>", -1);
}

static void 
getReqWidth (HtmlNode *pNode, CellReqWidth *pReq)
{
    HtmlComputedValues *pV = HtmlNodeComputedValues(pNode);
    if (pV->mask & PROP_MASK_WIDTH) {
        /* The computed value of the 'width' property is a percentage */
        pReq->eType = CELL_WIDTH_PERCENT;
        pReq->val.f = ((float)pV->iWidth) / 100.0; 
    } else if (pV->iWidth > 0) {
        pReq->eType = CELL_WIDTH_PIXELS;
        pReq->val.i = pV->iWidth;
    } else {
        pReq->eType = CELL_WIDTH_AUTO;
    }
}

static void 
logMinMaxWidths (LayoutContext *pLayout, HtmlNode *pNode, int col, int colspan, int *aMinWidth, int *aMaxWidth)
{
    LOG {
        int i;
        HtmlTree *pTree = pLayout->pTree;
        Tcl_Obj *pMinWidths = Tcl_NewObj();
        Tcl_IncrRefCount(pMinWidths);
        Tcl_AppendToObj(pMinWidths, "<tr><th> aMinWidth", -1);
        for (i = col; i < (col + colspan); i++) {
            Tcl_AppendToObj(pMinWidths, "<td>", 4);
            Tcl_AppendObjToObj(pMinWidths, Tcl_NewIntObj(i));
            Tcl_AppendToObj(pMinWidths, ":", 1);
            Tcl_AppendObjToObj(
                pMinWidths, Tcl_NewIntObj(aMinWidth[i])
            );
        }
        Tcl_AppendToObj(pMinWidths, "<tr><th> aMaxWidths", -1);
        for (i = col; i < (col + colspan); i++) {
            Tcl_AppendToObj(pMinWidths, "<td>", 4);
            Tcl_AppendObjToObj(pMinWidths, Tcl_NewIntObj(i));
            Tcl_AppendToObj(pMinWidths, ":", 1);
            Tcl_AppendObjToObj(
                pMinWidths, Tcl_NewIntObj(aMaxWidth[i])
            );
        }
        HtmlLog(pTree, "LAYOUTENGINE", 
            "%s tableColWidthMultiSpan() aMinWidth before:"
            "<table> %s </table>",
            Tcl_GetString(HtmlNodeCommand(pTree, pNode)),
            Tcl_GetString(pMinWidths)
        );
        Tcl_DecrRefCount(pMinWidths);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * tableColWidthMultiSpan --
 *
 *     A tableIterate() callback to analyse the following for each
 *     cell in the table that spans more than one column:
 * 
 *         * The minimum content width
 *         * The maximum content width
 *         * The requested width (may be in pixels, a percentage or "auto")
 *
 *     This function updates values set by the tableColWidthSingleSpan()
 *     loop.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Populates the following arrays:
 *
 *         TableData.aMinWidth[]
 *         TableData.aMaxWidth[]
 *         TableData.aReqWidth[]
 *
 *---------------------------------------------------------------------------
 */
static int 
tableColWidthMultiSpan (HtmlNode *pNode, int col, int colspan, int row, int rowspan, void *pContext)
{
    TableData *pData = (TableData *)pContext;

    int   *aMinWidth = pData->aMinWidth;
    int   *aMaxWidth = pData->aMaxWidth;
    CellReqWidth *aReq      = pData->aSingleReqWidth;
    CellReqWidth *aReqOut   = pData->aReqWidth;

    /* Because a cell originates in column $col, it's min and max content
     * width must be at least 1 pixel. tableColWidthSingleSpan() should
     * have taken care of this. 
     */
    assert(aMaxWidth[col] > 0);
    assert(aMinWidth[col] > 0);

    if (colspan > 1) {

        double fTotalPercent = 0.0;  /* Total of spanned percentage widths */
        int iTotalMin = 0;           /* Total min-width of all spanned cols */
        int iTotalMax = 0;           /* Total max-width of all spanned cols */
        int iTotalPixel = 0;         /* Total pixel width of all spanned cols */

        int nPixelWidth = 0;    /* Number of spanned pixel width cols */
        int nPercentWidth = 0;  /* Number of spanned percent width cols */
        int nAutoWidth = 0;     /* Number of spanned auto width cols */

        int i;

        /* Minimum, maximum and requested width of the multi-span cell */
        int min;
        int max;
        CellReqWidth req;
        BoxProperties box;

        /* Retrieve the min, max and requested width of the multi-span cell.
         * Adjust min and max so that they take into account the
         * 'border-spacing' regions that this cell spans and the borders and
         * padding on the cell itself.
         */
        getReqWidth(pNode, &req);
        blockMinMaxWidth(pData->pLayout, pNode, &min, &max);
        min = min - pData->border_spacing * (colspan - 1);
        max = max - pData->border_spacing * (colspan - 1);
        nodeGetBoxProperties(pData->pLayout, pNode, 0, &box);
        min = min + box.iLeft + box.iRight;
        max = max + box.iLeft + box.iRight;

        for (i = col; i < (col + colspan); i++) {
            switch (aReq[i].eType) {
                case CELL_WIDTH_AUTO:
                    nAutoWidth++;
                    break;
                case CELL_WIDTH_PIXELS:
                    iTotalPixel += aReq[i].val.i;
                    nPixelWidth++;
                    break;
                case CELL_WIDTH_PERCENT:
                    nPercentWidth++;
                    fTotalPercent += aReq[i].val.f;
                    break;
            }
            iTotalMin += aMinWidth[i];
            iTotalMax += aMaxWidth[i];
        }

        if (
            req.eType == CELL_WIDTH_PERCENT && 
            (colspan == nPercentWidth || fTotalPercent > req.val.f)
        ) {
            /* We have no means to satisfy this condition, so simply discard
             * the percentage width request.
             */
            req.eType = CELL_WIDTH_AUTO;
        }

        if (req.eType == CELL_WIDTH_PERCENT) {
            /* Any columns in the spanned set that do not already have
             * percentage values are given them, so that the percentages
             * add up to that requested by the spanning cell.
             *
             * If there is more than one column to add a percentage width
             * to, the percentages are allocated in proportion to the 
             * maximum content widths of the columns.
             */
             int iMaxNonPercent = 0;
             float fRem = req.val.f - fTotalPercent;
             for (i = col; i < (col + colspan); i++) {
                 if (aReq[i].eType != CELL_WIDTH_PERCENT) {
                     iMaxNonPercent += aMaxWidth[i];
                 }
             }
             for (i = col; i < (col + colspan) && iMaxNonPercent > 0; i++) {
                 if (aReq[i].eType != CELL_WIDTH_PERCENT) {
                     aReqOut[i].eType = CELL_WIDTH_PERCENT;
                     aReqOut[i].val.f = fRem * aMaxWidth[i] / iMaxNonPercent;
                     iMaxNonPercent -= aMaxWidth[i];
                 }
             }
             assert(iMaxNonPercent == 0);
        }

        if (min > iTotalMin) {
            /* The minimum required width for the spanning cell is greater
             * than that of the columns it spans.
             */
            int iRem = min;
            int iTPW = iTotalPixel;

            if (nPixelWidth == colspan) {
                /* All spanned columns have explicit pixel widths. In this
                 * case try to divide up the minimum width of the spanning
                 * cell according to the ratio between the pixel widths.
                 * Respect each cells min-width while doing this. 
                 */
                for (i = col; i < (col + colspan) && iTPW > 0; i++) {
                    int w = MAX(aMinWidth[i], iRem * aReq[i].val.i / iTPW);
                    iRem -= w;
                    aMinWidth[i] = w;
                    iTPW -= aReq[i].val.i;
                }
                assert(iTPW == 0);
            } else {
                LayoutContext *pLayout = pData->pLayout;

                int iMin = iTotalMin;
                int iMax = iTotalMax;

                LOG {
                    HtmlTree *pTree = pLayout->pTree;
                    HtmlLog(pTree, "LAYOUTENGINE", 
                        "%s tableColWidthMultiSpan() Distributing %d pixels."
                        " iMax=%d iMin=%d.",
                        Tcl_GetString(HtmlNodeCommand(pTree, pNode)), 
                        iRem, iMin, iMax
                    );
                }

                logMinMaxWidths(
                    pLayout, pNode, col, colspan, aMinWidth, aMaxWidth
                );

                for (i = col; iMax >= 0 && i < (col + colspan); i++) {
                    int isFixed = (aReq[i].eType == CELL_WIDTH_PIXELS);
                    if (isFixed && nAutoWidth > 0 && iTPW <= iRem) {
                        int w = MAX(aMinWidth[i], aReq[i].val.i);
                        iRem -= w;
                        iTPW -= aReq[i].val.i;
                        iMax -= aMaxWidth[i];
                        iMin -= aMinWidth[i];
                        aMinWidth[i] = w;
                    }
                }

                i = col;
                for (; iMax >= 0 && iMin < iRem && i < (col + colspan); i++){
                    int isFixed = (aReq[i].eType == CELL_WIDTH_PIXELS);
                    if (!isFixed || nAutoWidth == 0) {
                        int w = aMinWidth[i];
                        if (iMax) {
                            assert(aMaxWidth[i] <= iMax);
                            w = MAX(w, iRem * aMaxWidth[i] / iMax);
                        } else {
                            w = MAX(w, iRem);
                        }
                        assert(w <= iRem);

                        iMax -= aMaxWidth[i];
                        iMin -= aMinWidth[i];
                        iRem -= w;
                        aMinWidth[i] = w;
                    }
                }

                logMinMaxWidths(
                    pLayout, pNode, col, colspan, aMinWidth, aMaxWidth
                );
            }
        }

        if (iTotalMax < max) {
            int iM = iTotalMax;        /* Current sum of aMaxWidth[] */
            int iRem = max;            /* Required sum of aMaxWidth[] */
            for (i = col; iM > 0 && iRem > 0 && i < (col + colspan); i++){
                int w = MAX(aMaxWidth[i], iRem * aMaxWidth[i] / iM);
                iM -= aMaxWidth[i];
                iRem -= w;
                aMaxWidth[i] = w;
            }
        }

        for (i = col; i < (col + colspan); i++){
            aMaxWidth[i] = MAX(aMaxWidth[i], aMinWidth[i]);
        }
    }

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * tableCountCells --
 *
 *     A callback invoked by the tableIterate() function to figure out
 *     how many columns are in the table.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
static int 
tableCountCells (HtmlNode *pNode, int col, int colspan, int row, int rowspan, void *pContext)
{
    TableData *pData = (TableData *)pContext;
 
    /* A colspan of 0 is legal (apparently), but Tkhtml just handles it as 1 */
    if (colspan==0) {
        colspan = 1;
    }

    if (pData->nCol<(col+colspan)) {
        pData->nCol = col+colspan;
    }
    return TCL_OK;
}

static int 
tableCountRows (HtmlNode *pNode, int row, void *pContext)
{
    TableData *pData = (TableData *)pContext;
    pData->nRow = row + 1;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * tableDrawRow --
 *
 *     This is a tableIterate() 'row callback' used while actually drawing
 *     table data to canvas. See comments above tableDrawCells() for a
 *     description.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
static int 
tableDrawRow (HtmlNode *pNode, int row, void *pContext)
{
    TableData *pData = (TableData *)pContext;
    LayoutContext *pLayout = pData->pLayout;
    int nextrow = row+1;
    int x = 0;                             /* X coordinate to draw content */
    int i;                                 /* Column iterator */
    const int mmt = pLayout->minmaxTest;

    HtmlElementNode *pElem = (HtmlElementNode *)pNode;
    assert(!pElem || !HtmlNodeIsText(pNode));

    assert(row < pData->nRow);

    /* Add the background and border for the table-row, if a node exists. A
     * node may not exist if the row is entirely populated by overflow from
     * above. For example in the following document, there is no node for the
     * second row of the table.
     *
     *     <table><tr><td rowspan=2></table>
     */

    CHECK_INTEGER_PLAUSIBILITY(pData->pBox->vc.bottom);
    if (pElem && pElem->node.index >= 0 && pElem->pPropertyValues) {
        int iHeight;

        int x1, y1, w1, h1;           /* Border coordinates */
        x1 = pData->border_spacing;
        y1 = pData->aY[row];
        h1 = pData->aY[nextrow] - pData->aY[row] - pData->border_spacing;

    /* If we have a non-auto 'height' property on the table-row, then 
         * use it as a minimum height. Such a 'height' does not include
         * the border-spacing.
         */
        iHeight = PIXELVAL(pElem->pPropertyValues, HEIGHT, 0);
        if (iHeight > h1) {
            pData->aY[nextrow] += (iHeight - h1);
            h1 = iHeight;
        }

        w1 = 0;
        for (i = 0; i < pData->nCol; i++) w1 += pData->aWidth[i];
        w1 += ((pData->nCol - 1) * pData->border_spacing);
        HtmlLayoutDrawBox(pData->pLayout->pTree, &pData->pBox->vc, x1, y1, w1, h1, pNode, 0, mmt);
    }
    CHECK_INTEGER_PLAUSIBILITY(pData->pBox->vc.bottom);
    CHECK_INTEGER_PLAUSIBILITY(pData->pBox->vc.right);

    for (i = 0; i < pData->nCol; i++) {
        TableCell *pCell = &pData->aCell[i];

        /* At this point variable x holds the horizontal canvas offset of
         * the outside edge of the cell pCell's left border.
         */
        x += pData->border_spacing;
        if (pCell->finrow == nextrow) {
            BoxProperties box;
            int x1, y1, w1, h1;           /* Border coordinates */
            int y;
            int k;

            HtmlCanvas *pCanvas = &pData->pBox->vc;

            x1 = x;
            y1 = pData->aY[pCell->startrow];
            w1 = 0;
            for (k = i; k < (i+pCell->colspan); k++) w1 += pData->aWidth[k];
            w1 += ((pCell->colspan-1) * pData->border_spacing);
            h1 = pData->aY[pCell->finrow] - pData->border_spacing - y1;
            if (pCell->pNode->index >= 0) {
                HtmlLayoutDrawBox(pData->pLayout->pTree, pCanvas, x1, y1, w1, h1, pCell->pNode, 0, mmt);
            }
            nodeGetBoxProperties(pLayout, pCell->pNode, 0, &box);

            /* Todo: The formulas for the various vertical alignments below
             *       only work if the top and bottom borders of the cell
             *       are of the same thickness. Same goes for the padding.
             */
            switch (HtmlNodeComputedValues(pCell->pNode)->eVerticalAlign) {
                case CSS_CONST_TOP:
                case CSS_CONST_BASELINE:
                    y = pData->aY[pCell->startrow] + box.iTop;
                    break;
                case CSS_CONST_BOTTOM:
                    y = pData->aY[pCell->finrow] - pCell->box.height - box.iBottom - pData->border_spacing;
                    break;
                default:
                    y = pData->aY[pCell->startrow];
                    y += (h1 - box.iTop - box.iBottom - pCell->box.height) / 2;
                    y += box.iTop;
                    break;
            }
            CHECK_INTEGER_PLAUSIBILITY(pCanvas->bottom);
            DRAW_CANVAS(pCanvas, &pCell->box.vc, x+box.iLeft, y, pCell->pNode);
            CHECK_INTEGER_PLAUSIBILITY(pCanvas->bottom);
            memset(pCell, 0, sizeof(TableCell));
        }
        x += pData->aWidth[i];
    }

    CHECK_INTEGER_PLAUSIBILITY(pData->pBox->vc.bottom);
    CHECK_INTEGER_PLAUSIBILITY(pData->pBox->vc.right);

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * tableDrawCells --
 *
 *     tableIterate() callback to actually draw cells. Drawing uses two
 *     callbacks. This function is called for each cell in the table
 *     and the tableDrawRow() function above is called after each row has
 *     been completed.
 *
 *     This function draws the cell into the BoxContext at location
 *     aCell[col-number].box  in the TableData struct. The border and
 *     background are not drawn at this stage.
 *
 *     When the tableDrawRow() function is called, it is possible to
 *     determine the height of the row. This is needed before cell contents
 *     can be copied into the table canvas, so that the cell can be
 *     vertically aligned correctly, and so that the cell border and
 *     background match the height of the row they are in.
 * 
 *     Plus a few complications for cells that span multiple rows.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
static int 
tableDrawCells (HtmlNode *pNode, int col, int colspan, int row, int rowspan, void *pContext)
{
    TableData *pData = (TableData *)pContext;
    BoxContext *pBox;
    BoxProperties box;
    int i;
    int x = 0, y = 0;
    int belowY;
    LayoutContext *pLayout = pData->pLayout;
    int iHeight;
    HtmlComputedValues *pV;

    fixNodeProperties(pData, pNode);
    pV = HtmlNodeComputedValues(pNode);

    /* A rowspan of 0 means the cell spans the remainder of the table
     * vertically.  Similarly, a colspan of 0 means the cell spans the
     * remainder of the table horizontally. 
     */
    if (rowspan <= 0) rowspan = pData->nRow-row;
    if (colspan <= 0) colspan = pData->nCol-col;

    y = pData->aY[row];
    if (y == 0) {
        y = pData->border_spacing * (row+1);
        pData->aY[row] = y;
    }

    for (i = 0; i < col; i++) x += pData->aWidth[i];
    x += ((col+1) * pData->border_spacing);

    pBox = &pData->aCell[col].box;
    assert(pData->aCell[col].finrow == 0);
    pData->aCell[col].finrow = row+rowspan;
    pData->aCell[col].startrow = row;
    pData->aCell[col].pNode = pNode;
    pData->aCell[col].colspan = colspan;

    nodeGetBoxProperties(pData->pLayout, pNode, 0, &box);
    pBox->iContainingW = pData->aWidth[col] - box.iLeft - box.iRight;

    for (i = col+1; i < col+colspan; i++) {
        pBox->iContainingW += (pData->aWidth[i] + pData->border_spacing);
    }

    paginationPageYOrigin(y, pLayout);
    HtmlLayoutNodeContent(pData->pLayout, pBox, pNode);
    paginationPageYOrigin(-y, pLayout);

    /* Handle the 'height' property on the table-cell node. The 'height'
     * really specifies a minimum height for the row, not the height of the
     * table-cell box.
     *
     * Later: I have now learned that the 'height' property actually refers
     * to the minimum height of the cell box. The cell box includes the 
     * border edge of the box generated by the table cell. See CSS 2.1
     * sections 17.5 and 17.5.3.
     */
    iHeight = pBox->height + box.iTop + box.iBottom;
    iHeight = MAX(PIXELVAL(pV, HEIGHT, 0), iHeight);
    iHeight = MAX(pData->aHeight[row], iHeight);
    belowY = y + iHeight + pData->border_spacing;
    
    LOG {
        HtmlTree *pTree = pLayout->pTree;
        Tcl_Obj *pCmd = HtmlNodeCommand(pTree, pNode);
        if (pCmd) {
            HtmlLog(pTree, "LAYOUTENGINE", "%s tableDrawCells() "
                "containing=%d actual=%d",
                Tcl_GetString(pCmd),
                pBox->iContainingW, pBox->width
            );
        }
    }

    assert(row+rowspan < pData->nRow+1);
    pData->aY[row+rowspan] = MAX(pData->aY[row+rowspan], belowY);
    for (i = row+rowspan+1; i <= pData->nRow; i++) {
        pData->aY[i] = MAX(pData->aY[row+rowspan], pData->aY[i]);
    }

    CHECK_INTEGER_PLAUSIBILITY(pData->aY[row+rowspan]);
    CHECK_INTEGER_PLAUSIBILITY(pBox->vc.bottom);
    CHECK_INTEGER_PLAUSIBILITY(pBox->vc.right);

    return TCL_OK;
}

/*
 * Context object used by the tableIterate() iteration procedure. i.e.
 * the functions:
 *
 *     tableIterate()
 *     rowIterate()
 *     cellIterate()
 */
struct RowIterateContext {
    /* The cell and row callbacks */
    int (*xRowCallback)(HtmlNode *, int, void *);
    int (*xCallback)(HtmlNode *, int, int, int, int, void *);
    ClientData clientData;        /* Client data for the callbacks */

    /* The following two variables are used to keep track of cells that
     * span multiple rows. The array aRowSpan is dynamically allocated as
     * needed and freed before tableIterate() returns. The allocated size
     * of aRowSpan is stored in nRowSpan.
     * 
     * When iterating through the columns in a row (i.e. <th> or <td> tags
     * that are children of a <tr>) if a table cell with a rowspan greater
     * than 1 is encountered, then aRowSpan[<col-number>] is set to
     * rowspan. */
    int nRowSpan;
    int *aRowSpan;

    int iMaxRow;        /* Index of the final row of table */

    int iRow;           /* The current row number (first row is 0) */
    int iCol;           /* The current col number (first row is 0) */
};
typedef struct RowIterateContext RowIterateContext;

static void 
cellIterate (HtmlTree *pTree, HtmlNode *pNode, RowIterateContext *p)
{
    int nSpan = 1;
    int nRSpan = 1;
    int col_ok = 0;
    char const *zSpan = 0;

    HtmlElementNode *pElem = (HtmlElementNode *)pNode;

    /* Either this is a synthetic node, or it's 'display' property
     * is set to "table-cell" (in HTML <TD> or <TH>).
     */
    assert(
        0 == HtmlNodeParent(pNode) ||
        CSS_CONST_TABLE_CELL == DISPLAY(HtmlNodeComputedValues(pNode))
    );
    
    if (pElem->pPropertyValues) {
        /* Set nSpan to the number of columns this cell spans */
        zSpan = HtmlNodeAttr(pNode, "colspan");
        nSpan = zSpan?atoi(zSpan):1;
        if (nSpan <= 0) {
            nSpan = 1;
        }
        
        /* Set nRowSpan to the number of rows this cell spans */
        zSpan = HtmlNodeAttr(pNode, "rowspan");
        nRSpan = zSpan?atoi(zSpan):1;
        if (nRSpan <= 0) {
            nRSpan = 1;
        }
    }

    /* Now figure out what column this cell falls in. The
     * value of the 'col' variable is where we would like
     * to place this cell (i.e. just to the right of the
     * previous cell), but that might change based on cells
     * from a previous row with a rowspan greater than 1.
     * If this is true, we shift the cell one column to the
     * right until the above condition is false.
     */
    do {
        int k;
        for (k = p->iCol; k < (p->iCol + nSpan); k++) {
            if (k < p->nRowSpan && p->aRowSpan[k]) break;
        }
        if (k == (p->iCol + nSpan)) {
            col_ok = 1;
        } else {
            p->iCol++;
        }
    } while (!col_ok);
    
    /* Update the p->aRowSpan array. It grows here if required. */
    if (nRSpan!=1) {
        int k;
        if (p->nRowSpan<(p->iCol+nSpan)) {
            int n = p->iCol+nSpan;
            p->aRowSpan = (int *)HtmlRealloc(0, (char *)p->aRowSpan, 
                    sizeof(int)*n);
            for (k=p->nRowSpan; k<n; k++) {
                p->aRowSpan[k] = 0;
            }
            p->nRowSpan = n;
        }
        for (k=p->iCol; k<p->iCol+nSpan; k++) {
            assert(k < p->nRowSpan);
            p->aRowSpan[k] = (nRSpan>1?nRSpan:-1);
        }
    }
    
    if (p->xCallback) {
        p->xCallback(pNode, p->iCol, nSpan, p->iRow, nRSpan, p->clientData);
    }
    p->iCol += nSpan;
    p->iMaxRow = MAX(p->iMaxRow, p->iRow + nRSpan - 1);
}

static int 
rowIterate (HtmlTree *pTree, HtmlNode *pNode, RowIterateContext *p)
{
    int k;
    int i;

    /* Either this is a synthetic node, or it's 'display' property
     * is set to "table-row".
     */
    assert(
        0 == HtmlNodeParent(pNode) ||
        CSS_CONST_TABLE_ROW == DISPLAY(HtmlNodeComputedValues(pNode))
    );

    if (HtmlNodeIsText(pNode)) return 0;
    p->iCol = 0;

    for (i = 0; i < HtmlNodeNumChildren(pNode); i++) {
        HtmlNode *pCell = HtmlNodeChild(pNode, i);
        HtmlComputedValues *pV = HtmlNodeComputedValues(pCell);

        /* Throw away white-space children of the row node. */
        if (HtmlNodeIsWhitespace(pCell)) continue;

        if (DISPLAY(pV) == CSS_CONST_TABLE_CELL) {
            /* Child has "display:table-cell". Good. */
            cellIterate(pTree, pCell, p);
        } else {
            /* Have to create a fake <td> node. Bad. */
            int j;
            HtmlElementNode sCell;
            memset(&sCell, 0, sizeof(HtmlElementNode));
            for (j = i + 1; j < HtmlNodeNumChildren(pNode); j++) {
                HtmlNode *pNextRow = HtmlNodeChild(pNode, j);
                HtmlComputedValues *pV2 = HtmlNodeComputedValues(pNextRow);
                if (DISPLAY(pV2) == CSS_CONST_TABLE_CELL) break;
            }
            sCell.node.index = -1;
            sCell.nChild = j - i;
            sCell.apChildren = &((HtmlElementNode *)pNode)->apChildren[i];
            cellIterate(pTree, (HtmlNode *)&sCell, p);
            HtmlLayoutInvalidateCache(pTree, (HtmlNode *)&sCell);
            i = j - 1;
        }
    }

    if (p->xRowCallback) {
        p->xRowCallback(pNode, p->iRow, p->clientData);
    }
    p->iRow++;
    for (k=0; k < p->nRowSpan; k++) {
        if (p->aRowSpan[k]) p->aRowSpan[k]--;
    }

    return 0;
}

static void 
rowGroupIterate (HtmlTree *pTree, HtmlNode *pNode, RowIterateContext *p)
{
    int i;

    if (!pNode) return;

    /* Either this is a synthetic node, or it's 'display' property
     * is set to one of "table-row-group", "table-footer-group" or
     * "table-header-group".
     */
    assert(
        0 == HtmlNodeParent(pNode)                                           ||
        CSS_CONST_TABLE_ROW_GROUP==DISPLAY(HtmlNodeComputedValues(pNode))    ||
        CSS_CONST_TABLE_FOOTER_GROUP==DISPLAY(HtmlNodeComputedValues(pNode)) ||
        CSS_CONST_TABLE_HEADER_GROUP==DISPLAY(HtmlNodeComputedValues(pNode))
    );

    for (i = 0; i < HtmlNodeNumChildren(pNode); i++) {
        HtmlNode *pRow = HtmlNodeChild(pNode, i);
        HtmlComputedValues *pV = HtmlNodeComputedValues(pRow);

        /* Throw away white-space node children of the <TBODY> node. */
        if (HtmlNodeIsWhitespace(pRow)) continue;

        if (DISPLAY(pV) == CSS_CONST_TABLE_ROW) {
            /* Child has "display:table-row". Good. */
            rowIterate(pTree, pRow, p);
        } else {
            /* Have to create a fake <tr> node. Bad. */
            int j;
            HtmlElementNode sRow;
            memset(&sRow, 0, sizeof(HtmlElementNode));
            for (j = i + 1; j < HtmlNodeNumChildren(pNode); j++) {
                HtmlNode *pNextRow = HtmlNodeChild(pNode, j);
                HtmlComputedValues *pV2 = HtmlNodeComputedValues(pNextRow);
                if (DISPLAY(pV2) == CSS_CONST_TABLE_ROW) break;
            }
            sRow.node.index = -1;
            sRow.nChild = j - i;
            sRow.apChildren = &((HtmlElementNode *)pNode)->apChildren[i];
            rowIterate(pTree, (HtmlNode*)&sRow, p);
            assert(!sRow.pLayoutCache);
            i = j - 1;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * tableIterate --
 *
 *     Helper function for HtmlTableLayout, used to iterate through cells
 *     of the table. For the table below, the iteration order is W, X,
 *     Y, Z.
 *
 *     /-------\
 *     | W | X |       row number = 0
 *     |-------|
 *     | Y | Z |       row number = 1
 *     \-------/
 *
 *     For each cell, the function passed as the second argument is 
 *     invoked. The arguments are a pointer to the <td> or <th> node
 *     that identifies the cell, the column number, the colspan, the row
 *     number, the rowspan, and a copy of the pContext argument passed to
 *     iterateTable().
 *
 *     After xCallback has been invoked for each cell in a row, the
 *     row-callback (xRowCallback) is invoked for the row. The arguments
 *     to xRowCallback are the <tr> node object, the row number and a
 *     copy of the pContext argument passed to tableIterate().
 *
 *   TRANSIENT NODES:
 *
 *     Sometimes, the nodes passed to the xCallback or xRowCallback 
 *     callback functions may be allocated on the stack, rather than
 *     actually part of the document tree. This happens when implicit
 *     nodes are inserted.
 *
 *     Transient (stack) nodes can be identified by the following test:
 *
 *         if (pNode->pParent == 0) { // Is a transient node }
 *
 *     Because the node structure is allocated on the stack, it should
 *     not be passed to HtmlDrawBox() etc.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     Whatever xCallback does.
 *
 *---------------------------------------------------------------------------
 */
static void 
tableIterate (
    HtmlTree *pTree,
    HtmlNode *pNode,                               /* The <table> node */
    int (*xCallback)(HtmlNode *, int, int, int, int, void *),  /* Callback */
    int (*xRowCallback)(HtmlNode *, int, void *),  /* Row Callback */
    void *pContext                                /* pContext of callbacks */
)
{
    int i;
  
    HtmlNode *pHeader = 0;     /* Table header (i.e. <THEAD>) */
    HtmlNode *pFooter = 0;     /* Table footer (i.e. <TFOOT>) */

    RowIterateContext sRowContext;
    memset(&sRowContext, 0, sizeof(RowIterateContext));
    sRowContext.xRowCallback = xRowCallback;
    sRowContext.xCallback  = xCallback;
    sRowContext.clientData = (ClientData)pContext;

    /* Search for the table header and footer blocks. */
    for (i = 0; i < HtmlNodeNumChildren(pNode); i++) {
        HtmlNode *pChild = HtmlNodeChild(pNode, i);
        switch (DISPLAY(HtmlNodeComputedValues(pChild))) {
            case CSS_CONST_TABLE_FOOTER_GROUP:
                pFooter = (pFooter ? pFooter : pChild);
                break;
            case CSS_CONST_TABLE_HEADER_GROUP:
                pHeader = (pHeader ? pHeader : pChild);
                break;
        }
    }

    rowGroupIterate(pTree, pHeader, &sRowContext);

    for (i = 0; i < HtmlNodeNumChildren(pNode); i++) {
        HtmlNode *pChild = HtmlNodeChild(pNode, i);
        int eDisplay;

        if (pChild == pFooter || pChild == pHeader) continue;

    /* Throw away white-space node children of the table node. 
         * Todo: Is this correct?  */
        if (HtmlNodeIsWhitespace(pChild)) continue;

        eDisplay = DISPLAY(HtmlNodeComputedValues(pChild));
        if (
            eDisplay == CSS_CONST_TABLE_ROW_GROUP ||
            eDisplay == CSS_CONST_TABLE_FOOTER_GROUP ||
            eDisplay == CSS_CONST_TABLE_HEADER_GROUP
        ) {
            rowGroupIterate(pTree, pChild, &sRowContext);
        } else {
            /* Create a transient <TBODY> node */
            int j;
            HtmlElementNode sRowGroup;

            for (j = i + 1; j < HtmlNodeNumChildren(pNode); j++) {
                HtmlNode *pSibling = HtmlNodeChild(pNode, j);
                eDisplay = DISPLAY(HtmlNodeComputedValues(pSibling));
                if (
                    eDisplay == CSS_CONST_TABLE_ROW_GROUP ||
                    eDisplay == CSS_CONST_TABLE_FOOTER_GROUP ||
                    eDisplay == CSS_CONST_TABLE_HEADER_GROUP
                ) break;
            }

            memset(&sRowGroup, 0, sizeof(HtmlElementNode));
            sRowGroup.node.index = -1;
            sRowGroup.nChild = j - i;
            sRowGroup.apChildren = &((HtmlElementNode *)pNode)->apChildren[i];
            rowGroupIterate(pTree, (HtmlNode*)&sRowGroup, &sRowContext);
            assert(!sRowGroup.pLayoutCache);
            i = j - 1;
        }
    }

    rowGroupIterate(pTree, pFooter, &sRowContext);

    while (sRowContext.iRow <= sRowContext.iMaxRow && xRowCallback) {
        xRowCallback(0, sRowContext.iRow, pContext);
        sRowContext.iRow++;
    }
    HtmlFree(sRowContext.aRowSpan);
}


static void
logWidthStage(
    int nStage,
    Tcl_Obj *pStageLog,
    int nWidth,
    int *aWidth
    )
{
    int i;
    if (!pStageLog) return;
    Tcl_AppendToObj(pStageLog, "<tr><td>Stage ", -1);
    Tcl_AppendObjToObj(pStageLog, Tcl_NewIntObj(nStage));
    for (i = 0; i < nWidth; i++) {
        Tcl_AppendToObj(pStageLog, "<td>", -1);
        Tcl_AppendObjToObj(pStageLog, Tcl_NewIntObj(aWidth[i]));
    }
}


static void 
tableCalculateCellWidths (
    TableData *pData,
    int availablewidth,    /* Total width available for cells */
    int isAuto,           /* True if the 'width' of the <table> was "auto" */
    int isHeight
) {
    /* The values of the following variables are set in the "analysis loop"
     * (the first loop below) and thereafter left unchanged.
     */ 
    int nPercentCol = 0;         /* Number of percentage width columns */
    double fTotalPercent = 0.0;  /* Total of percentage widths */
    int nExplicitCol = 0;  /* Number of explicit pixel width columns */
    int iMaxExplicit = 0;  /* Total of max-content-width for explicit cols */
    int nAutoCol = 0;      /* Number of 'auto' width columns */
    int iMaxAuto = 0;      /* Total of max-content-width for all 'auto' cols */
    int iMinAuto = 0;      /* Total of min-content-width for all 'auto' cols */
    int i, j;
    int iRemaining = availablewidth;
    const int n = isHeight ? pData->nRow : pData->nCol;
    const int LRGT = 1 + MAX(MAX(CELL_WIDTH_AUTO, CELL_WIDTH_PIXELS), CELL_WIDTH_PERCENT);

    /* Local handles for the input arrays */
    int *aMinWidth = isHeight ? pData->aMinHeight : pData->aMinWidth;
    int *aMaxWidth = isHeight ? pData->aMaxHeight : pData->aMaxWidth;
    CellReqWidth *aReqWidth = isHeight ? pData->aReqHeight : pData->aReqWidth;
    
    int *aWidth = isHeight ? pData->aHeight : pData->aWidth; /* Local handle for the output array */

    /* Log the inputs to this function. */
    LayoutContext *pLayout = pData->pLayout;
    Tcl_Obj *pStageLog = 0;
    LOG {
        HtmlTree *pTree = pLayout->pTree;
        Tcl_Obj *pCmd = HtmlNodeCommand(pTree, pData->pNode);
        if (pCmd) {
            Tcl_Obj *pLog = Tcl_NewObj();
            Tcl_IncrRefCount(pLog);
    
            Tcl_AppendToObj(pLog, "Inputs to column width algorithm: ", -1);
            Tcl_AppendToObj(pLog, "<p>Available width is ", -1);
            Tcl_AppendObjToObj(pLog, Tcl_NewIntObj(availablewidth));
            Tcl_AppendToObj(pLog, "  (width property was <b>", -1);
            Tcl_AppendToObj(pLog, isAuto ? "auto</b>" : "not</b> auto", -1);
            Tcl_AppendToObj(pLog, ")</p>", -1);
    
            logWidthsToTable(pData, pLog);
    
            HtmlLog(pTree, "LAYOUTENGINE", "%s tableCalculateCellWidths() %s",
                Tcl_GetString(pCmd), Tcl_GetString(pLog)
            );
    
            Tcl_DecrRefCount(pLog);
            pStageLog = Tcl_NewObj();
            Tcl_IncrRefCount(pStageLog);
        }
    }
    /* This loop serves two purposes:
     *
     *     1. Allocate each column it's minimum content width.
     *     2. It is the "analysis loop" refered to above that populates
     *        local variables used by later stages of the algorithm.
     */
    for (i = 0; i < n; i++) {
        aWidth[i] = aMinWidth[i];
        iRemaining -= aMinWidth[i];

        switch (aReqWidth[i].eType) {
            case CELL_WIDTH_AUTO:
                iMaxAuto += aMaxWidth[i];
                iMinAuto += aMinWidth[i];
                nAutoCol++;
                break;
            case CELL_WIDTH_PIXELS:
                iMaxExplicit += aMaxWidth[i];
                nExplicitCol++;
                break;
            case CELL_WIDTH_PERCENT:
                nPercentCol++;
                fTotalPercent += aReqWidth[i].val.f;
                break;
        }
    }
    logWidthStage(1, pStageLog, n, aWidth);
    if (iRemaining > 0) { /* Allocate pixels to percentage width columns */
        for (i = 0; i < n; i++) {
            if (aReqWidth[i].eType == CELL_WIDTH_PERCENT) {
                int iReq = (50 + (aReqWidth[i].val.f * availablewidth)) / 100;
                iReq = MAX(0, iReq - aWidth[i]);
                aWidth[i] += iReq;
                iRemaining -= iReq;
            }
        }
        if (fTotalPercent > 100.0) {
            int iRemove = (50 + ((fTotalPercent-100.0) * availablewidth)) / 100;
            for (i = n - 1; i >= 0; i--) {
                if (aReqWidth[i].eType == CELL_WIDTH_PERCENT) {
                    /* Apparently this is for Gecko compatibility. */
                    int rem = MIN(aWidth[i], iRemove);
                    iRemove -= rem;
                    rem = MIN(aWidth[i] - aMinWidth[i], rem);
                    iRemaining += rem;
                    aWidth[i] -= rem;
                }
            }
        }
    }
    logWidthStage(2, pStageLog, n, aWidth);
    if (iRemaining > 0) { /* Allocate pixels to explicit width columns */
        for (i = 0; i < n; i++) {
            if (aReqWidth[i].eType == CELL_WIDTH_PIXELS) {
                int iReq = MAX(0, aReqWidth[i].val.i - aWidth[i]);
                aWidth[i] += iReq;
                iRemaining -= iReq;
            }
        }
    }
    logWidthStage(3, pStageLog, n, aWidth);
    if (iRemaining > 0) { /* Allocate pixels to auto width columns */
        int iMA = iMaxAuto;
        iRemaining += iMinAuto;
        for (i = 0; iMA > 0 && i < n; i++) {
            if (aReqWidth[i].eType == CELL_WIDTH_AUTO) {
                int w = MAX(aMinWidth[i], iRemaining*aMaxWidth[i]/iMA);
                aWidth[i] = w;
                iRemaining -= w;
                iMA -= aMaxWidth[i];
            }
        }
    }
    logWidthStage(4, pStageLog, n, aWidth);
    if (iRemaining > 0) { /* Force pixels into fixed columns (subject to max-width) */
        int iME = iMaxExplicit;
        for (i = 0; i < n; i++) {
            if (aReqWidth[i].eType == CELL_WIDTH_PIXELS) {
                int w = iRemaining * aMaxWidth[i] / iME;
                iME -= aMaxWidth[i];
                iRemaining -= w;
                aWidth[i] += w;
            }
        }
    }
    logWidthStage(5, pStageLog, n, aWidth);
    if (iRemaining > 0 && fTotalPercent < 100.0) { /* Force pixels into percent columns (not subject to max-width!) */
        float fTP = fTotalPercent;
        for (i = 0; i < n; i++) {
            if (aReqWidth[i].eType == CELL_WIDTH_PERCENT) {
                int w = iRemaining * aReqWidth[i].val.f / fTP;
                fTP -= aReqWidth[i].val.f;
                iRemaining -= w;
                aWidth[i] += w;
            }
        }
    }
    logWidthStage(6, pStageLog, n, aWidth);
    if (iRemaining > 0) { /* Force pixels into any columns (not subject to max-width!) */
        for (i = 0; i < n; i++) {
            int w = iRemaining / (n - i);
            iRemaining -= w;
            aWidth[i] += w;
        }
    }
    logWidthStage(7, pStageLog, n, aWidth);
    /* If too many pixels have been allocated, take some back from
     * the columns. By preference we take pixels from "auto" columns,
     * followed by "pixel width" columns and finally "percent width"
     * columns.
     *
     * In pseudo-tcl the outer loop would read:
     *
     *     foreach j {auto pixels percent} { 
     *         reduce_pixels_in_cols_of_type $j
     *     }
     */
    for (j = 0; iRemaining < 0 && j < LRGT; j++) {
        /* Total allocated, less the total min-content-width, for the cols */
        int iAllocLessMin = 0;
        for (i = 0; i < n; i++) {
            if (aReqWidth[i].eType == j) {
                iAllocLessMin += (aWidth[i] - aMinWidth[i]);
            }
        } for (i = 0; iAllocLessMin > 0 && i < n; i++){
            if (aReqWidth[i].eType == j) {
                int iDiff = aWidth[i] - aMinWidth[i];
                int iReduce = -1 * (iRemaining * iDiff) / iAllocLessMin;
                iRemaining += iReduce;
                iAllocLessMin -= iDiff;
                aWidth[i] -= iReduce;
            }
        }
        logWidthStage(j+8, pStageLog, n, aWidth);
    }
    LOG {
        HtmlTree *pTree = pLayout->pTree;
        Tcl_Obj *pCmd = HtmlNodeCommand(pTree, pData->pNode);
        if (pCmd) {
            Tcl_Obj *pLog = Tcl_NewObj();
            Tcl_IncrRefCount(pLog);
    
            Tcl_AppendToObj(pLog, "<p>Summary of algorithm:</p>", -1);
            Tcl_AppendToObj(pLog, 
                "<ol>"
                "  <li>Minimum content width allocation."
                "  <li>Percent width allocation."
                "  <li>Explicit pixel width allocation."
                "  <li>Auto width allocation."
                "  <li>Force pixels into explicit pixel width cols."
                "  <li>Force pixels into percent width cols."
                "  <li>Force pixels into auto width cols."
                "  <li>Reduce auto width cols. (optional)"
                "  <li>Reduce explicit pixel width cols. (optional)"
                "  <li>Reduce percent width cols. (optional)"
                "</ol>", -1
            );
    
            Tcl_AppendToObj(pLog, "<p>Results of column width algorithm:</p>", -1);
            Tcl_AppendToObj(pLog, "<table><tr><th></th>", -1);
            for (i = 0; i < n; i++) {
                Tcl_AppendToObj(pLog, "<th>Col ", -1);
                Tcl_AppendObjToObj(pLog, Tcl_NewIntObj(i));
            }
            Tcl_AppendToObj(pLog, "</tr>", -1);
            Tcl_AppendObjToObj(pLog, pStageLog);
            Tcl_AppendToObj(pLog, "</table>", -1);
    
            HtmlLog(pTree, "LAYOUTENGINE", "%s tableCalculateCellWidths() %s",
                Tcl_GetString(pCmd), Tcl_GetString(pLog)
            );
    
            Tcl_DecrRefCount(pLog);
        }
    }
}


static int 
tableCalculateMaxWidth (TableData *pData)
{
    int   *aMaxWidth        = pData->aMaxWidth;
    int   *aMinWidth        = pData->aMinWidth;
    CellReqWidth *aReqWidth = pData->aReqWidth;
    int i;
    int ret = 0;

    float fTotalPercent = 0.0;
    int iMaxNonPercent = 0;
    int iPercent = 0;

    int bConsiderPercent = 0;

    HtmlComputedValues *pV = HtmlNodeComputedValues(pData->pNode);

    for (i = 0; i < pData->nCol; i++) {
        if (aReqWidth[i].eType == CELL_WIDTH_PIXELS) {
            ret += MAX(aMinWidth[i], aReqWidth[i].val.i);
        } else {
            assert(aMaxWidth[i] >= aMinWidth[i]);
            ret += aMaxWidth[i];
        }

        if (aReqWidth[i].eType == CELL_WIDTH_PERCENT) {
            float percent = MIN(aReqWidth[i].val.f, 100.0 - fTotalPercent);
            int w = (aMaxWidth[i] * 100.0) / MAX(percent, 1.0);
            iPercent = MAX(iPercent, w);
            fTotalPercent += percent;
            bConsiderPercent = 1;
        } else {
            iMaxNonPercent += aMaxWidth[i];
        }
    }

#if 0
    /* TODO: Including this block breaks the google-groups message page. */
    for (p = HtmlNodeParent(pData->pNode); p; p = HtmlNodeParent(p)) {
        HtmlComputedValues *pComputed = HtmlNodeComputedValues(p);
        if (
            PIXELVAL(pComputed, WIDTH, 0) != PIXELVAL_AUTO ||
            pComputed->ePosition != CSS_CONST_STATIC
        ) {
            break;
        }

        if (
            pComputed->eDisplay == CSS_CONST_TABLE || 
            pComputed->eDisplay == CSS_CONST_TABLE_CELL || 
            pComputed->eDisplay == CSS_CONST_TABLE_ROW
        ) {
            bConsiderPercent = 0;
            break;
        }
    }
#endif

    if (bConsiderPercent) {
        if (fTotalPercent <= 99.0) {
            iMaxNonPercent = iMaxNonPercent * 100.0 / (100.0 - fTotalPercent);
        } else if (iMaxNonPercent > 0) {
            /* If control flows to here, there exists the following:
             *
             *     + There are one or columns with percentage widths, and
             *       the percentage widths sum to more than 100%.
             *     + There is at least one other column.
             *
             * Return something really large for the maximum width in this
             * case, as the correct rendering is to make the table consume
             * the full width of the containing block.
             */
            iMaxNonPercent = 10000;
        }
        ret = MAX(iMaxNonPercent, ret);
        ret = MAX(iPercent, ret);
    }

    ret = MAX(ret, PIXELVAL(pV, WIDTH, PIXELVAL_AUTO));

    return ret;
}

/*
 *---------------------------------------------------------------------------
 *
 * HtmlTableLayout --
 *
 *     Lay out a table node.
 *
 *     Todo: Update this comment.
 *
 *     This is an incomplete implementation of HTML tables - it does not
 *     support the <col>, <colspan>, <thead>, <tfoot> or <tbody> elements.
 *     Since the parser just ignores tags that we don't know about, this
 *     means that all children of the <table> node should have tag-type
 *     <tr>. Omitting <thead>, <tfoot> and <tbody> is not such a big deal
 *     since it is optional to format these elements differently anyway,
 *     but <col> and <colspan> are fairly important.
 *
 *     The table layout algorithm used is described in section 17.5.2.2 of 
 *     the CSS 2.1 spec.
 *
 *     When this function is called, pBox->iContainingW contains the width
 *     available to the table content - not including any margin, border or
 *     padding on the table itself. Any pixels allocated between the edge of
 *     the table and the leftmost or rightmost cell due to 'border-spacing' is
 *     included in pBox->iContainingW. If the table element has a computed value
 *     for width other than 'auto', then pBox->iContainingW is the calculated
 *     'width' value. Otherwise it is the width available according to the
 *     width of the containing block.
 * 
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------------
 */
int HtmlTableLayout (
    LayoutContext *pLayout,
    BoxContext *pBox,
    HtmlNode *pNode          /* The node to layout */
)
{
    HtmlTree *pTree = pLayout->pTree;
    HtmlComputedValues *pV = HtmlNodeComputedValues(pNode);
    int nCol = 0;             /* Number of columns in this table */
    int i;
    int availwidth, availheight;    /* Total width available for cells */

    int *aMinWidth = 0, *aMinHeight = 0; /* Minimum width for each column */
    int *aMaxWidth = 0, *aMaxHeight = 0; /* Minimum width for each column */
    int *aWidth = 0, *aHeight = 0;       /* Actual width for each column */
    int *aY = 0;              /* Top y-coord for each row */
    TableCell *aCell = 0;     /* Array of nCol cells used during drawing */
    TableData data;
    int offset = pBox->height;

    CellReqWidth *aReqWidth = 0, *aReqHeight = 0;
    CellReqWidth *aSingleReqWidth = 0, *aSingleReqHeight = 0;

    memset(&data, 0, sizeof(struct TableData));
    data.pLayout = pLayout;
    data.pNode = pNode;

    pBox->iContainingW = MAX(pBox->iContainingW, 0);  /* ??? */
    assert(pBox->iContainingW>=0);
    
    pBox->iContainingH = MAX(pBox->iContainingH, 0);  /* ??? */
    assert(pBox->iContainingH>=0);

    assert(pV->eDisplay==CSS_CONST_TABLE);

    /* Read the value of the 'border-spacing' property. 'border-spacing' may
     * not take a percentage value, so there is no need to use PIXELVAL().
     */
    data.border_spacing = pV->iBorderSpacing;

    /* First step is to figure out how many columns this table has.
     * There are two ways to do this - by looking at COL or COLGROUP
     * children of the table, or by counting the cells in each rows.
     * Technically, we should use the first method if one or more COL or
     * COLGROUP elements exist. For now though, always use the second 
     * method.
     */
    tableIterate(pTree, pNode, tableCountCells, tableCountRows, &data);
    nCol = data.nCol;

    LOG {
        Tcl_Obj *pCmd = HtmlNodeCommand(pTree, pNode);
        if (pCmd) {
            HtmlTree *pTree = pLayout->pTree;
            HtmlLog(pTree, "LAYOUTENGINE", "%s HtmlTableLayout() "
                "Dimensions are %dx%d", Tcl_GetString(pCmd), 
                data.nCol, data.nRow
            );
        }
    }

    /* Allocate arrays for the minimum and maximum widths of each column */
    aMinWidth = (int *)HtmlClearAlloc(0, nCol*sizeof(int));
    aMaxWidth = (int *)HtmlClearAlloc(0, nCol*sizeof(int));
    aWidth    = (int *)HtmlClearAlloc(0, nCol*sizeof(int));

    aReqWidth = (CellReqWidth *)HtmlClearAlloc(0, nCol*sizeof(CellReqWidth));
    aSingleReqWidth = (CellReqWidth *)HtmlClearAlloc(0, nCol*sizeof(CellReqWidth));

    aY = (int *)HtmlClearAlloc(0, (data.nRow+1)*sizeof(int));
    aCell = (TableCell *)HtmlClearAlloc(0, nCol*sizeof(TableCell));

    data.aMaxWidth = aMaxWidth;
    data.aMinWidth = aMinWidth;
    data.aWidth = aWidth;
    data.aReqWidth = aReqWidth;
    data.aSingleReqWidth = aSingleReqWidth;
    
    aMinHeight = (int *)HtmlClearAlloc(0, data.nRow*sizeof(int));
    aMaxHeight = (int *)HtmlClearAlloc(0, data.nRow*sizeof(int));
    aHeight    = (int *)HtmlClearAlloc(0, data.nRow*sizeof(int));
    
    aReqHeight = (CellReqWidth *)HtmlClearAlloc(0, data.nRow*sizeof(CellReqWidth));
    aSingleReqHeight = (CellReqWidth *)HtmlClearAlloc(0, data.nRow*sizeof(CellReqWidth));
    
    data.aMaxHeight = aMaxHeight;
    data.aMinHeight = aMinHeight;
    data.aHeight = aHeight;
    data.aReqHeight = aReqHeight;
    data.aSingleReqHeight = aSingleReqHeight;

    /* Calculate the minimum, maximum, and requested percentage widths of
     * each column.  The first pass only considers cells that span a single
     * column.  In this case the min/max width of each column is the maximum of
     * the min/max widths for all cells in the column.
     * 
     * If the table contains one or more cells that span more than one
     * column, we make a second pass. The min/max widths are increased,
     * if necessary, to account for the multi-column cell. In this case,
     * the width of each column that the cell spans is increased by 
     * the same amount (plus or minus a pixel to account for integer
     * rounding).
     */
    tableIterate(pTree, pNode, tableColWidthSingleSpan, 0, &data);
    memcpy(aReqWidth, aSingleReqWidth, nCol*sizeof(CellReqWidth));
    tableIterate(pTree, pNode, tableColWidthMultiSpan, 0, &data);
    
    paginationPageYOrigin(offset, pLayout);

    pBox->height = pBox->width = 0;
    availwidth = (pBox->iContainingW - (nCol+1) * data.border_spacing);
    availheight = (pBox->iContainingH - (data.nRow+1) * data.border_spacing);
    switch (pLayout->minmaxTest) {
        case 0:
            tableCalculateCellWidths(&data, availwidth, 0, 0);
            tableCalculateCellWidths(&data, availheight, 0, 1);
            for (i = 0; i < nCol; i++) pBox->width += aWidth[i];
            data.aY = aY;
            data.aCell = aCell;
            data.pBox = pBox;
            tableIterate(pTree, pNode, tableDrawCells, tableDrawRow, &data);
            pBox->height = data.aY[data.nRow];
            break;

        case MINMAX_TEST_MIN:
            for (i = 0; i < nCol; i++) pBox->width += aMinWidth[i];
            for (i = 0; i < data.nRow; i++) pBox->height += aMinHeight[i];
            break;

        case MINMAX_TEST_MAX: {
            int minwidth = 0;
            pBox->width = tableCalculateMaxWidth(&data);
            pBox->width = MIN(pBox->width, availwidth);
            for (i = 0; i < nCol; i++) {
                minwidth += aMinWidth[i];
            }
            pBox->width = MAX(pBox->width, minwidth);
            break;
        }
        default:
            assert(!"Bad value for LayoutContext.minmaxTest");
    }
    paginationPageYOrigin(-offset, pLayout);
    pBox->width += (data.border_spacing * (nCol+1));

    HtmlFree(aMinWidth);
    HtmlFree(aMaxWidth);
    HtmlFree(aWidth);
    HtmlFree(aMinHeight);
    HtmlFree(aMaxHeight);
    HtmlFree(aHeight);
    HtmlFree(aY);
    HtmlFree(aCell);
    HtmlFree(aReqWidth);
    HtmlFree(aSingleReqWidth);
    HtmlFree(aReqHeight);
    HtmlFree(aSingleReqHeight);

    HtmlComputedValuesRelease(pTree, data.pDefaultProperties);

    CHECK_INTEGER_PLAUSIBILITY(pBox->width);
    CHECK_INTEGER_PLAUSIBILITY(pBox->height);
    CHECK_INTEGER_PLAUSIBILITY(pBox->vc.bottom);
    CHECK_INTEGER_PLAUSIBILITY(pBox->vc.right);
    LOG {
        Tcl_Obj *pCmd = HtmlNodeCommand(pTree, pNode);
        if (pCmd) {
            HtmlTree *pTree = pLayout->pTree;
            HtmlLog(pTree, "LAYOUTENGINE", "%s HtmlTableLayout() "
                "Content size is %dx%d", Tcl_GetString(pCmd), pBox->width, pBox->height);
        }
    }
    return TCL_OK;
}