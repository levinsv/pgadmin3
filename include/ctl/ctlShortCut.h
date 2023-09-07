#ifndef CTLSHORTCUT_H
#define CTLSHORTCUT_H

#include "pgAdmin3.h"

#include <wx/odcombo.h>
class ctlShortCut : public wxOwnerDrawnComboBox
{
public:
    ctlShortCut(frmMain* main,wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    ~ctlShortCut();
    void OnTEXT_ENTER(wxCommandEvent& event);
    void OnCombo(wxCommandEvent& event);
    void OnTEXT(wxCommandEvent& event);
    void OnChar(wxKeyEvent& event);
    void OnCharT(wxKeyEvent& event);
    void SetText(wxString& str, bool settext);
    wxTextCtrl* GetTextControl();
    static wxString viewText(wxString& src);
    wxArrayString BuildList(wxString& find,bool full);
    wxTextCtrl* txt;
    wxString fnd;
    wxVListBoxComboPopup* pop;
    frmMain* frm;
    wxImageList* imageList;
    virtual void OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const
    {
        if (item == wxNOT_FOUND) return;
        wxCoord w1, h1, w2, h2;
        wxString ss = GetString(item);
        wxString src = viewText(ss);
        int pp = src.Find(fnd);
        wxRect r;
        
        int idx=0;
        wxString si = ss.substr(0, 4).Trim();
        if (si.ToInt(&idx)) {
            dc.DrawBitmap(imageList->GetBitmap(idx),wxPoint(rect.x,rect.y));
        }
        ;
        r.x = rect.x+24;
        r.y = rect.y + (rect.height / 2) - (dc.GetCharHeight() / 2);
        if (pp >= 0) {
            dc.SetBrush(*wxYELLOW_BRUSH);
                const wxString& line = src;
                pp = line.Find(fnd);

                int lineWidth, lineWidthP, lineHeight, start = 0;
                wxString pref;
                pref = line.substr(start, pp);

                dc.GetTextExtent(pref, &lineWidthP, &lineHeight);
                r.x = r.x + lineWidthP+3;
                pref = line.substr(pp, fnd.Len());
                dc.GetTextExtent(pref, &lineWidth, &lineHeight);
                r.width = lineWidth;
                r.height = lineHeight;
                dc.DrawRoundedRectangle(r, 3);
        }
        dc.GetTextExtent(src, &w1, &h1);
        dc.DrawText(src,
            rect.x + 3+24,
            (rect.y + 0) + (rect.height / 2) - (dc.GetCharHeight() / 2)
        );
        //dc.SetFont(m_fontList[item]);
    }

    virtual void OnDrawBackground(wxDC& dc, const wxRect& rect, int item, int flags) const
    {
        // If item is selected or even, or we are painting the
        // combo control itself, use the default rendering.
        if ((flags & (wxODCB_PAINTING_CONTROL | wxODCB_PAINTING_SELECTED)) ||
            (item & 1) == 0)
        {
            wxOwnerDrawnComboBox::OnDrawBackground(dc, rect, item, flags);
            return;
        }
        // Otherwise, draw every other background with different colour.
        wxColour bgCol(245, 245, 255);
        dc.SetBrush(wxBrush(bgCol));
        dc.SetPen(wxPen(bgCol));
        dc.DrawRectangle(rect);
    }

    virtual wxCoord OnMeasureItem(size_t item) const
    {
        return 20;
    }

    virtual wxCoord OnMeasureItemWidth(size_t item) const
    {
        return 400;
    }
};
class dlgShortCut : public wxDialog
{
public:
    dlgShortCut(frmMain* parent, wxWindowID id, wxPoint pos, const wxSize size);
    ~dlgShortCut();
    void OnKEY_DOWN(wxKeyEvent& event);
    void OnCharHook(wxKeyEvent& event);
    //void OnPopUpOKClick(wxCommandEvent& event);
    //void focus();

private:
    ctlShortCut *cb;
    frmMain* frm;
};

#endif

