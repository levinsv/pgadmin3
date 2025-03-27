//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// explainShape.cpp - Explain Shapes
//
//////////////////////////////////////////////////////////////////////////



// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "ctl/explainCanvas.h"

#include <wx/docview.h>

#include "images/ex_aggregate.pngc"
#include "images/ex_partial_aggregate.pngc"
#include "images/ex_finalize_aggregate.pngc"
#include "images/ex_append.pngc"
#include "images/ex_bmp_and.pngc"
#include "images/ex_bmp_heap.pngc"
#include "images/ex_bmp_index.pngc"
#include "images/ex_bmp_or.pngc"
#include "images/ex_cte_scan.pngc"
#include "images/ex_delete.pngc"
#include "images/ex_foreign_scan.pngc"
#include "images/ex_group.pngc"
#include "images/ex_hash.pngc"
#include "images/ex_hash_anti_join.pngc"
#include "images/ex_hash_semi_join.pngc"
#include "images/ex_hash_setop_except.pngc"
#include "images/ex_hash_setop_except_all.pngc"
#include "images/ex_hash_setop_intersect.pngc"
#include "images/ex_hash_setop_intersect_all.pngc"
#include "images/ex_hash_setop_unknown.pngc"
#include "images/ex_index_only_scan.pngc"
#include "images/ex_index_scan.pngc"
#include "images/ex_insert.pngc"
#include "images/ex_lock_rows.pngc"
#include "images/ex_join.pngc"
#include "images/ex_limit.pngc"
#include "images/ex_materialize.pngc"
#include "images/ex_merge.pngc"
#include "images/ex_merge_anti_join.pngc"
#include "images/ex_merge_append.pngc"
#include "images/ex_merge_semi_join.pngc"
#include "images/ex_gatcher.pngc"
#include "images/ex_nested.pngc"
#include "images/ex_nested_loop_anti_join.pngc"
#include "images/ex_nested_loop_semi_join.pngc"
#include "images/ex_recursive_union.pngc"
#include "images/ex_result.pngc"
#include "images/ex_scan.pngc"
#include "images/ex_pscan.pngc"
#include "images/ex_seek.pngc"
#include "images/ex_setop.pngc"
#include "images/ex_sort.pngc"
#include "images/ex_subplan.pngc"
#include "images/ex_tid_scan.pngc"
#include "images/ex_unique.pngc"
#include "images/ex_unknown.pngc"
#include "images/ex_update.pngc"
#include "images/ex_values_scan.pngc"
#include "images/ex_window_aggregate.pngc"
#include "images/ex_worktable_scan.pngc"

// Greenplum images
#include "images/ex_broadcast_motion.pngc"
#include "images/ex_redistribute_motion.pngc"
#include "images/ex_gather_motion.pngc"

#define BMP_BORDER 3

ExplainShape::ExplainShape(const wxImage &bmp, const wxString &description, long tokenNo, long detailNo)
{
    SetBitmap(wxBitmap(bmp));
    SetLabel(description, tokenNo, detailNo);
    SetDisableLabel(true);
    kidCount = 0;
    totalShapes = 0;
    usedShapes = 0;
    m_rootShape = false;
}


void ExplainShape::SetLabel(const wxString &str, int tokenNo, int detailNo)
{
    if (tokenNo < 0)
    {
        description = str;
        label = str;
    }
    else
    {
        wxStringTokenizer tokens(str, wxT(" "));

        while (tokenNo-- >= 0)
        {
            label = tokens.GetNextToken();

            if (detailNo <= 0)
            {
                if (!description.IsEmpty())
                    description.Append(wxT(" "));

                description.Append(label);
            }
        }
        if (detailNo > 0)
        {
            tokens.SetString(str, wxT(" "));

            while (detailNo--)
            {
                if (!description.IsEmpty())
                    description.Append(wxT(" "));

                description.Append(tokens.GetNextToken());
            }
            detail = tokens.GetString();
        }
    }
}
typedef struct autospacesolver {
    int flags = 0;
    wxSize min{-1,-1};
    wxSize max;
} autospacesolver;
void ExplainShape::OnDraw(wxDC &dc)
{
    wxBitmap &bmp = GetBitmap();
    if (!bmp.Ok())
        return;

    // We do not draw the root shape
    if (m_rootShape)
        return;
    wxRect trg=GetCanvas()->GetClientRect();
    wxMouseEvent ev;
    ev.m_x = 0;
    ev.m_y = 0;
    wxPoint logPos(ev.GetLogicalPosition(dc));
    ev.m_x = trg.x+trg.width;
    ev.m_y = trg.height;
    wxPoint logPos2(ev.GetLogicalPosition(dc));
    wxRect cliprect(logPos, logPos2);
    double width = 0.0, height = 0.0;
    GetBoundingBoxMin(&width,&height);
    wxRect sp((int)(GetX()), (int)(GetY()), (int) (width), (int)(height));
    //int x = (int)(GetX() + (width ));
    //int y = (int)(GetY() + (height));
    if (!cliprect.Intersects(sp)) {
        if (dynamic_cast<ExplainCanvas *>(GetCanvas())->isneedoptimizedraw) return;
        
    }
    // dynamic bitmap
    bool bmpdynamic = false;
    wxBitmap bmpm(bmp.GetWidth(),bmp.GetHeight());
    if (label.StartsWith("Memoize")) {
        wxPen p = dc.GetPen();
        wxMemoryDC dc2;
        dc2.SelectObject(bmpm);
        //dc2.SetBackgroundMode(wxBRUSHSTYLE_SOLID);
        dc2.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
        dc2.Clear();
        int w = bmpm.GetWidth();
        int h = bmpm.GetHeight();
        dc2.DrawRoundedRectangle(wxRect(0, 0, w, h),2);
        //dc2.DrawRectangle(0, 0, w, h);
        wxRegEx r("Hits: (\\d+)  Misses: (\\d+)  Evictions: (\\d+)  Overflows: (\\d+)  Memory Usage: (\\d+\\S+)");
        long hit=0, mis=0, evi=0, ovr=0;
        wxString mem;
        int x = 0, y = 0;
        int tw, th;
        wxString txt = _("Cache");
        wxFont f(GetCanvas()->GetFont());
        f.SetPointSize(8);
        mem = f.GetNativeFontInfoDesc();
        dc2.SetFont(f);
        dc2.SetPen(p);
        //dc2.SetTextForeground(*wxBLACK);
        //dc2.SetTextBackground(*wxWHITE);
        dc2.GetTextExtent(txt, &tw, &th);
        int border = 2;
        y += border;
        x = (w - tw) / 2;
        dc2.DrawText(txt, x, y);
        y += th+ border;
        //dc2.DrawText("TEst", 0, 0);
        mem = wxEmptyString;
        int spaceV = h-y; // label Cache
        int rr = 4;
        int h_line = th / 2;
        if (r.Matches(condition)) {
            hit=StrToLong( r.GetMatch(condition, 1));
            mis = StrToLong(r.GetMatch(condition, 2));
            evi = StrToLong(r.GetMatch(condition, 3));
            ovr = StrToLong(r.GetMatch(condition, 4));
            mem = (r.GetMatch(condition, 5));
            spaceV = spaceV - th;  // memory usage
            if (evi || ovr) {
                spaceV = spaceV - 2 * rr;
                if (spaceV < 8) {
                    border = 1; h_line = 6; rr = 3;
                }
            }
        }
        wxRect rect(border, y, w - 2 * border, h_line);
        //dc2.DrawRectangle(0, 0, w, h);
        if ((mis + hit) > 0) {
            int hitwidth = rect.width * hit * 1.0 / (mis + hit);
            {
                wxDCPenChanger ppen(dc2, *wxTRANSPARENT_PEN);
                //wxColour c(wxString("#00FF00"));
                wxColour c(*wxGREEN);
                wxDCBrushChanger br(dc2, wxBrush(c));
                //dc2.SetBrush(*wxGREEN_BRUSH);
                dc2.DrawRectangle(BMP_BORDER, y, hitwidth, rect.height);
                dc2.SetPen(dc.GetPen());
                dc2.SetBrush(*wxTRANSPARENT_BRUSH);
                dc2.DrawRoundedRectangle(rect,(double)2.0);
                
            }
            y += rect.height + border;
        }
        else {
            wxDCBrushChanger br(dc2, wxBrush(p.GetColour(), wxHATCHSTYLE_BDIAGONAL));
            rect.x = 0; rect.width = w; rect.height = h - y;
            dc2.DrawRectangle(rect);
        }
        if (!mem.IsEmpty()) {
            mem = "" + mem;
            dc2.DrawText(mem, border, y);
            y += th;
        }
        x = border;
        if (evi > 0) {
            wxDCBrushChanger br(dc2, wxBrush(*wxBLUE));
            wxPoint p(x+rr,y+rr);
            dc2.DrawCircle(p, rr);
            //y + 2 * r + 1;
            x += 2 * rr + rr;
        }
        if (ovr > 0) {
            wxDCBrushChanger br(dc2, wxBrush(*wxRED));
            wxPoint p(x + rr, y + rr);
            dc2.DrawCircle(p, rr);
            x += 2 * rr + 3;
        }

        {
            wxPen p = dc.GetPen();
            wxDCPenChanger ppen(dc2, wxPen(p.GetColour(), 2));
            //dc2.DrawLine(wxPoint(0, 0), wxPoint(w, h));
            //dc2.DrawLine(wxPoint(w, 0), wxPoint(0, h));
        }
        dc2.SelectObject(wxNullBitmap);
        bmpdynamic = true;
    }
    int x, y;
    x = WXROUND(m_xpos - bmp.GetWidth() / 2.0);
    y = WXROUND(m_ypos - GetHeight() / 2.0);

    if (bmpdynamic) dc.DrawBitmap(bmpm, x, y, true); else dc.DrawBitmap(bmp, x, y, true);

    int w, h;
    dc.SetFont(GetCanvas()->GetFont());
    dc.GetTextExtent(label.Trim(), &w, &h);

    x = WXROUND(m_xpos - w / 2.0);
    y += bmp.GetHeight() + BMP_BORDER;

    dc.DrawText(label.Trim(), x, y);
}


void ExplainShape::OnLeftClick(double x, double y, int keys, int attachment)
{
    ((ExplainCanvas *)GetCanvas())->ShowPopup(this);
}


#define ARROWMARGIN 5
wxRealPoint ExplainShape::GetStartPoint()
{
    wxRealPoint rp(GetX() + GetBitmap().GetWidth() / 2.0 + ARROWMARGIN, GetY() - (GetHeight() - GetBitmap().GetHeight()) / 2.);
    return rp;
}


wxRealPoint ExplainShape::GetEndPoint(int kidNo)
{
    wxRealPoint rp(GetX() - GetBitmap().GetWidth() / 2.0 - ARROWMARGIN, GetY() - (GetHeight() - GetBitmap().GetHeight()) / 2. + (kidCount > 1 ? GetBitmap().GetHeight() * 2. / 3. * kidNo / (2 * kidCount - 2) : 0 ));
    return rp;
}



ExplainShape *ExplainShape::Create(long level, ExplainShape *last, const wxString &str)
{
    ExplainShape *s = 0;

    int costPos = str.Find(wxT("(cost="));
    int actPos = str.Find(wxT("(actual"));
    bool parallel=false;
    int inc=0;
    wxStringTokenizer tokens(str, wxT(" "));
    wxString token = tokens.GetNextToken();
    if (token == wxT("Parallel")) 
        {
            token = tokens.GetNextToken();
            parallel=true;
            inc=1;
        }
    
    wxString token2 = tokens.GetNextToken();
    wxString token3 = tokens.GetNextToken();
    wxString token4;
    if (tokens.HasMoreTokens())
        token4 = tokens.GetNextToken();
    wxString descr;
    if (costPos > 0)
        descr = str.Left(costPos);
    else if (actPos > 0)
        descr = str.Left(actPos);
    else
        descr = str;

    // Requested an empty shape, which can be treated as a root shape
    if (level == 0)
    {
        s = new ExplainShape(*ex_unknown_png_img, wxEmptyString);
        s->SetDraggable(false);
        s->m_rootShape = true;
        s->level = level;
        int w = 50, h = 20;

        wxBitmap &bmp = s->GetBitmap();
        if (w < bmp.GetWidth())
            w = bmp.GetWidth();

        s->SetHeight(bmp.GetHeight() + BMP_BORDER + h);
        s->SetWidth(w);

        s->upperShape = NULL;
        s->kidNo = 0;

        return s;
    }


    // possible keywords can be found in postgresql/src/backend/commands/explain.c

    if (token == wxT("Total"))              return 0;
    else if (token == wxT("Trigger"))       return 0;
    else if (token == wxT("Settings:"))		return 0;		/* Greenplum */
    else if (token == wxT("Slice"))			return 0;		/* Greenplum */
    else if (token.Mid(0, 6) == wxT("(slice"))	return 0;	/* Greenplum */
    else if (token == wxT("Result"))        s = new ExplainShape(*ex_result_png_img, descr);
    else if (token == wxT("Append"))        s = new ExplainShape(*ex_append_png_img, descr);
    else if (token == wxT("Gather"))	    s = new ExplainShape(*ex_gatcher_png_img, descr);
    else if (token == wxT("Partial") && token2 == wxT("GroupAggregate"))  s = new ExplainShape(*ex_partial_aggregate_png_img, descr);
    else if (token == wxT("Finalize") && token2 == wxT("GroupAggregate"))  s = new ExplainShape(*ex_finalize_aggregate_png_img, descr);
    else if (token == wxT("Nested"))
    {
        if (token2 == wxT("Loop") && token4 == wxT("Join"))
        {
            // Nested Loop Anti Join
            if (token3 == wxT("Anti"))
            {
                s = new ExplainShape(*ex_nested_loop_anti_join_png_img, descr);
            }
            // Nested Loop Semi Join
            else
            {
                s = new ExplainShape(*ex_nested_loop_semi_join_png_img, descr);
            }
        }
        if (!s)
            s = new ExplainShape(*ex_nested_png_img, descr);
    }
    else if (token == wxT("Merge"))
    {
        if (token3 == wxT("Join"))
        {
            // Merge Anti Join
            if (token2 == wxT("Anti"))
            {
                s = new ExplainShape(*ex_merge_anti_join_png_img, descr);
            }
            // Merge Semi Join
            else
            {
                s = new ExplainShape(*ex_merge_semi_join_png_img, descr);
            }
        }
        // Merge Append
        else if (token2 == wxT("Append"))
        {
            s = new ExplainShape(*ex_merge_append_png_img, descr);
        }
        else
        {
            s = new ExplainShape(*ex_merge_png_img, descr);
        }
    }
    else if (token == wxT("Hash"))
    {
        if (token2 == wxT("Join"))
        {
            s = new ExplainShape(*ex_join_png_img, descr);
        }
        else
        {
            if (token3 == wxT("Join"))
            {
                // Hash Anti Join
                if (token2 == wxT("Anti"))
                {
                    s = new ExplainShape(*ex_hash_anti_join_png_img, descr);
                }
                // Hash Semi Join
                else if (token2 == wxT("Semi"))
                {
                    s = new ExplainShape(*ex_hash_semi_join_png_img, descr);
                }
                else
                {
                    s = new ExplainShape(*ex_hash_png_img, descr);
                }
            }
            else
            {
                s = new ExplainShape(*ex_hash_png_img, descr);
            }
        }
    }
    else if (token == wxT("HashSetOp"))
    {
        if (token2 == wxT("Except"))
        {
            // HashSetOp Except ALL
            if (token3 == wxT("ALL"))
            {
                s = new ExplainShape(*ex_hash_setop_except_all_png_img, descr);
            }
            // HashSetOp Except
            else
            {
                s = new ExplainShape(*ex_hash_setop_except_png_img, descr);
            }
        }
        else if (token2 == wxT("Intersect"))
        {
            // HashSetOp Intersect ALL
            if (token3 == wxT("ALL"))
            {
                s = new ExplainShape(*ex_hash_setop_intersect_all_png_img, descr);
            }
            // HashSetOp Intersect
            else
            {
                s = new ExplainShape(*ex_hash_setop_intersect_png_img, descr);
            }
        }
        else
        {
            // HashSetOp ???
            s = new ExplainShape(*ex_hash_setop_unknown_png_img, descr);
        }
    }
    else if (token == wxT("Subquery"))      s = new ExplainShape(*ex_subplan_png_img, descr, 0, 2);
    else if (token == wxT("Function"))      s = new ExplainShape(*ex_result_png_img, descr, 0, 2);
    else if (token == wxT("Materialize"))   s = new ExplainShape(*ex_materialize_png_img, descr);
    else if (token == wxT("Sort"))          s = new ExplainShape(*ex_sort_png_img, descr);
    else if (token == wxT("Group"))         s = new ExplainShape(*ex_group_png_img, descr);
    else if (token == wxT("Aggregate") || token == wxT("GroupAggregate") || token == wxT("HashAggregate"))
        s = new ExplainShape(*ex_aggregate_png_img, descr);
    else if (token == wxT("Unique"))        s = new ExplainShape(*ex_unique_png_img, descr);
    else if (token == wxT("SetOp"))         s = new ExplainShape(*ex_setop_png_img, descr);
    else if (token == wxT("Limit"))         s = new ExplainShape(*ex_limit_png_img, descr);
    else if (token == wxT("LockRows"))      s = new ExplainShape(*ex_lock_rows_png_img, descr);
    else if (token == wxT("Bitmap"))
    {
        if (token2 == wxT("Index"))         s = new ExplainShape(*ex_bmp_index_png_img, descr, 4+inc, 3+inc);
        else                                s = new ExplainShape(*ex_bmp_heap_png_img, descr, 4+inc, 3+inc);
    }
    else if (token == wxT("BitmapAnd"))     s = new ExplainShape(*ex_bmp_and_png_img, descr);
    else if (token == wxT("BitmapOr"))      s = new ExplainShape(*ex_bmp_or_png_img, descr);
    else if (token2 == wxT("Scan"))
    {
        if (token == wxT("Index"))
            // Scan Index Backward
            if (token3 == wxT("Backward"))
                s = new ExplainShape(*ex_index_scan_png_img, descr, 4+inc, 3+inc);
            else
                s = new ExplainShape(*ex_index_scan_png_img, descr, 3+inc, 2+inc);
        // Tid Scan
        else if (token == wxT("Tid"))
            s = new ExplainShape(*ex_tid_scan_png_img, descr, 3+inc, 2+inc);
        // WorkTable Scan
        else if (token == wxT("WorkTable"))
            s = new ExplainShape(*ex_worktable_scan_png_img, descr, 3+inc, 2+inc);
        // CTE Scan
        else if (token == wxT("CTE"))
            s = new ExplainShape(*ex_cte_scan_png_img, descr, 3, 2);
        // Foreign Scan
        else if (token == wxT("Foreign"))
            s = new ExplainShape(*ex_foreign_scan_png_img, descr, 3, 2);
        // Values Scan
        else if (token == wxT("Values"))
            s = new ExplainShape(*ex_values_scan_png_img, descr, 3, 2);
        else if (parallel) s = new ExplainShape(*ex_pscan_png_img, descr, 4, 3);
        else
            s = new ExplainShape(*ex_scan_png_img, descr, 3+inc, 2+inc);
    }
    else if (token == wxT("Index"))
    {
        // Index Only Scan
        if (token2 == wxT("Only") && token3 == wxT("Scan"))
        {
            s = new ExplainShape(*ex_index_only_scan_png_img, descr, 4+inc, 3+inc);
        }
    }
    else if (token2 == wxT("Seek"))         s = new ExplainShape(*ex_seek_png_img, descr, 3, 2);
    // Recursive Union
    else if (token == wxT("Recursive") && token2 == wxT("Union"))
        s = new ExplainShape(*ex_recursive_union_png_img, descr);
    else if (token == wxT("WindowAgg"))
        s = new ExplainShape(*ex_window_aggregate_png_img, descr);

    // DML
    else if (token == wxT("Insert"))
        s = new ExplainShape(*ex_insert_png_img, descr, 2, 1);
    else if (token == wxT("Update"))
        s = new ExplainShape(*ex_update_png_img, descr, 2, 1);
    else if (token == wxT("Delete"))
        s = new ExplainShape(*ex_delete_png_img, descr, 2, 1);

    // Greenplum additions
    else if (token == wxT("Gather") && token2 == wxT("Motion"))
        s = new ExplainShape(*ex_gather_motion_png_img, descr);
    else if (token == wxT("Broadcast") && token2 == wxT("Motion"))
        s = new ExplainShape(*ex_broadcast_motion_png_img, descr);
    else if (token == wxT("Redistribute") && token2 == wxT("Motion"))
        s = new ExplainShape(*ex_redistribute_motion_png_img, descr);

    if (!s)
        s = new ExplainShape(*ex_unknown_png_img, descr);

    s->SetDraggable(false);

    s->level = level;

    if (costPos > 0)
    {
        if (actPos > 0)
        {
            s->actual = str.Mid(actPos);
            s->cost = str.Mid(costPos, actPos - costPos);
        }
        else
            s->cost = str.Mid(costPos);
    }
    else if (actPos > 0)
        s->actual = str.Mid(actPos);

    int w = 50, h = 20;

    wxBitmap &bmp = s->GetBitmap();
    if (w < bmp.GetWidth())
        w = bmp.GetWidth();

    s->SetHeight(bmp.GetHeight() + BMP_BORDER + h);
    s->SetWidth(w);

    s->upperShape = last;
    if (last)
    {
        s->kidNo = last->kidCount;
        last->kidCount++;
    }
    else
        s->kidNo = 0;

    if (costPos > 0)
    {
        wxChar *cl = const_cast<wxChar *>((const wxChar *)str + costPos + 6);
        wxChar *ch = wxStrstr(cl, wxT(".."));
        if (ch)
        {
            *ch = 0;
            ch += 2;
        }
        s->costLow = StrToDouble(cl);
        if (ch)
        {
            wxChar *r = wxStrstr(ch, wxT(" rows="));
            if (r)
            {
                *r = 0;
                r += 6;
            }
            s->costHigh = StrToDouble(ch);
            if (r)
            {
                wxChar *w = wxStrstr(r, wxT(" width="));
                if (w)
                {
                    *w = 0;
                    w += 7;
                }
                s->rows = StrToLong(r);
                if (w)
                    s->width = StrToLong(w);
            }
        }
    }
    return s;
}


ExplainLine::ExplainLine(ExplainShape *from, ExplainShape *to, double weight)
{
    SetCanvas(from->GetCanvas());
    from->AddLine(this, to);
    MakeLineControlPoints(4);

    width = (int) log(from->GetAverageCost());
    if (width > 10)
        width = 10;

    // If we got an average cost of 0, width is probably negative
    // which will look pretty darn ugly as an arrow width!
    // This may happen on Greenplum.
    if (width < 1)
        width = 1;

    wxNode *first = GetLineControlPoints()->GetFirst();
    wxNode *last  = GetLineControlPoints()->GetLast();
    *(wxRealPoint *)first->GetData() = from->GetStartPoint();
    *(wxRealPoint *)last->GetData() = to->GetEndPoint(from->GetKidno());

    wxRealPoint *p1 = (wxRealPoint *)first->GetNext()->GetData();
    wxRealPoint *p2 = (wxRealPoint *)last->GetPrevious()->GetData();
    *p1 = from->GetStartPoint();
    *p2 = to->GetEndPoint(from->GetKidno());
    p1->x -= (p1->x - p2->x) / 3. + 8;
    p2->x += (p1->x - p2->x) / 3. - 8;

    Initialise();
}


#define ARROWWIDTH  4


void ExplainLine::OnDraw(wxDC &dc)
{
    if (m_lineControlPoints)
    {
        dc.SetPen(*wxThePenList->FindOrCreatePen(*wxBLACK, 1, wxPENSTYLE_SOLID));
        dc.SetBrush(*wxTheBrushList->FindOrCreateBrush(*wxLIGHT_GREY, wxBRUSHSTYLE_SOLID));

        wxPoint *points = new wxPoint[11];
        wxRealPoint *point0 = (wxRealPoint *) m_lineControlPoints->Item(0)->GetData();
        wxRealPoint *point1 = (wxRealPoint *) m_lineControlPoints->Item(1)->GetData();
        wxRealPoint *point2 = (wxRealPoint *) m_lineControlPoints->Item(2)->GetData();
        wxRealPoint *point3 = (wxRealPoint *) m_lineControlPoints->Item(3)->GetData();

        double phi  = atan2(point2->y - point1->y, point2->x - point1->x);
        double offs = width * tan(phi / 2);

        points[0].x  = WXROUND(point0->x);
        points[0].y  = WXROUND(point0->y) - width;
        points[10].x = WXROUND(point0->x);
        points[10].y = WXROUND(point0->y) + width;

        points[1].x  = WXROUND(point1->x + offs);
        points[1].y  = WXROUND(point1->y) - width;
        points[9].x  = WXROUND(point1->x - offs);
        points[9].y  = WXROUND(point1->y) + width;

        points[2].x  = WXROUND(point2->x + offs);
        points[2].y  = WXROUND(point2->y) - width;
        points[8].x  = WXROUND(point2->x - offs);
        points[8].y  = WXROUND(point2->y) + width;

        points[3].x  = WXROUND(point3->x) - width - ARROWWIDTH;
        points[3].y  = WXROUND(point3->y) - width;
        points[7].x  = WXROUND(point3->x) - width - ARROWWIDTH;
        points[7].y  = WXROUND(point3->y) + width;

        points[4].x  = points[3].x;
        points[4].y  = points[3].y - ARROWWIDTH;
        points[6].x  = points[7].x;
        points[6].y  = points[7].y + ARROWWIDTH;

        points[5].x  = WXROUND(point3->x);
        points[5].y  = WXROUND(point3->y);

        dc.DrawPolygon(11, points, 0, 0);
    }
}
