#ifndef POPUPHELP_H
#define POPUPHELP_H

#include "wx/popupwin.h"
#include <wx/html/htmlwin.h>
#include "wx/clipbrd.h"
#include "utils/FunctionPGHelper.h"
#include <wx/regex.h>
#include <map>
#include <vector>
#include "wx/display.h"
#include "frm/menuServerStatus.h"

class popuphelp :
    public wxPopupTransientWindow
{
public:
    ~popuphelp() {
        delete closeTimer;
    }
    //popuphelp(wxWindow* parent);
    bool ProcessLeftDown(wxMouseEvent& event)
    {
        return false;
    }
    bool IsValid() {
        return isvalid;
    }
    void SetSizePopup(const wxSize& sz) {
        SetSize(sz);
        Layout();
        Fit();
        sizew = sz;
    }
    wxSize GetSizePopup() { return sizew; }
    popuphelp(wxWindow* parent,wxString keyword, FunctionPGHelper *hhelper,wxPoint &posit,wxSize &newSz) : wxPopupTransientWindow(parent) {
        wxSize top_sz(newSz);
        sizew = top_sz;
        //SetSize(top_sz);
        this->hhelper = hhelper;
        SetBackgroundColour(*wxBLACK);
        extern sysSettings* settings;
        wxFont fnt = settings->GetSQLFont();
        int size = fnt.GetPointSize();
        SetFont(fnt);
        htmlWindow = new wxHtmlWindow(this, -1, wxDefaultPosition, sizew);
        
        wxFont font(size, wxFontFamily::wxFONTFAMILY_MODERN, wxFontStyle::wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        wxString fixf = font.GetFaceName();
        wxString scalf = fnt.GetFaceName();
        int f_sizes[7];
        f_sizes[0] = int(size -4);
        f_sizes[1] = int(size -2);
        f_sizes[2] = size;
        f_sizes[3] = int(size +1);
        f_sizes[4] = int(size +3);
        f_sizes[5] = int(size +5);
        f_sizes[6] = int(size +7);
        htmlWindow->SetFonts(scalf, fixf, f_sizes);

        htmlWindow->SetRelatedStatusBar(0);
        //htmlWindow->SetPage("<html><body><h1>TEST</h1><span fgcolor=\"#332233\">Set Page Works</span></body></hmtl>");
        wxString txt = hhelper->getHelpString(keyword);
        if (txt.IsEmpty()) {
            
            txt = hhelper->getSqlCommandHelp(keyword);
            if (txt.empty()) {
                isvalid = false;
                return;
            }
        }
        SetPage(txt);
        //htmlWindow->SetSize(htmlWindow->GetInternalRepresentation()->GetWidth(), htmlWindow->GetInternalRepresentation()->GetHeight());
        //wxSize sz= htmlWindow->GetSize();
        //sz = htmlWindow->GetBestSize();
        //htmlWindow->SetHTMLBackgroundImage(wxBitmapBundle::FromSVGFile("data/bg.svg", wxSize(65, 45)));
        wxBoxSizer* topsizer; 
        topsizer = new wxBoxSizer(wxVERTICAL);

        //htmlWindow->SetInitialSize(wxSize(htmlWindow->GetInternalRepresentation()->GetWidth(),			htmlWindow->GetInternalRepresentation()->GetHeight()));

        //SetSize(wxSize(300,150));
        topsizer->Add(htmlWindow, 1, wxALL, 1);

        //wxButton* bu1 = new wxButton(this, wxID_OK, _("OK"));
        //bu1->SetDefault();

        //topsizer->Add(bu1, 0, wxALL | wxALIGN_RIGHT, 15);

        SetSizer(topsizer);
        topsizer->Fit(this);
        int xx, yy;
        htmlWindow->GetVirtualSize(&xx, &yy);
        //wxSize sz = GetSize();
        wxPoint posScreen;
        wxSize sizeScreen;
        const int displayNum = wxDisplay::GetFromPoint(posit);
        if (displayNum != wxNOT_FOUND)
        {
            const wxRect rectScreen = wxDisplay(displayNum).GetGeometry();
            posScreen = rectScreen.GetPosition();
            sizeScreen = rectScreen.GetSize();
        }
        else // outside of any display?
        {
            // just use the primary one then
            posScreen = wxPoint(0, 0);
            sizeScreen = wxGetDisplaySize();
        }
        wxSize top_new(top_sz);

        if (xx > sizeScreen.x) xx = sizeScreen.x - 120;
        if (yy > sizeScreen.y) yy = sizeScreen.y - 120;


        if (xx > top_sz.x || yy > top_sz.y) {
            int dx = 0;
            if (htmlWindow->IsScrollbarShown(wxHORIZONTAL)) dx=htmlWindow->GetScrollThumb(wxHORIZONTAL);
            SetSizePopup(wxSize(xx+dx, yy+5));
        }

    //this->Bind(wxEVT_HTML_CELL_CLICKED, [&](wxHtmlCellEvent& event) {
    //	wxHtmlCell* c = event.GetCell();
    //	
    //	wxString ctext=c->ConvertToText(NULL);
    //	ctext=htmlWindow->SelectionToText();
    //	wxString s = wxString::Format("cell = %s",ctext.c_str());
    //	wxMessageBox(s,	"cell", wxOK | wxICON_INFORMATION);

    //    });
    this->Bind(wxEVT_HTML_LINK_CLICKED, [&](wxHtmlLinkEvent& event) {
        wxHtmlLinkInfo i = event.GetLinkInfo();
        wxString name = i.GetHref();
        wxString body=this->hhelper->getHelpString(name);
        if (body.IsEmpty()) {
            body = this->hhelper->getHelpFile(name);
        }
        SetPage(body);
            //ctext=htmlWindow->SelectionToText();
            //wxString s = wxString::Format("cell = %s",ctext.c_str());
        });
    htmlWindow->Bind(wxEVT_RIGHT_UP, [&](wxMouseEvent& event) {
        wxString name;
        wxLongLong e = wxGetLocalTimeMillis();
        if (e - startTimeWin < 150)
            return;
        //wxString body = this->hhelper->getHelpString(name);
        wxString ctext = htmlWindow->SelectionToText();
        if (!ctext.IsEmpty()) {
            wxClipboardLocker clip;
            if (!clip ||
                !wxTheClipboard->AddData(new wxTextDataObject(ctext)))
            {
               
            }
            wxString wname = GetParent()->GetName();
            if (wname == "frmStatus") {
                //CMD_EVENT_FIND_STR
                wxCommandEvent event(wxEVT_MENU, CMD_EVENT_FIND_STR);
                event.SetEventObject(this);
                // Give it some contents
                event.SetString(ctext);
                // Do send it
                GetParent()->ProcessWindowEvent(event);
            }
            //statusBar->SetStatusText(wxString::Format(" TIMER COUNT %d x:%d,y:%d", count, m.x, m.y));
            Hide();
            return;
        }
        this->SetPage("", true);
        //ctext=htmlWindow->SelectionToText();
        //wxString s = wxString::Format("cell = %s",ctext.c_str());
        });
    startTimeWin = wxGetLocalTimeMillis();
    int inter = hhelper->GetTimerClose();
    if (inter != -1) {
        closeTimer = new wxTimer(this);
        Bind(wxEVT_TIMER, [&](wxTimerEvent& event) {
            closeTimer->Stop();
            wxPoint pm = this->GetScreenPosition();
            wxRect rc = this->GetSize();
            wxPoint m = wxGetMousePosition();
            rc.x = pm.x;
            rc.y = pm.y;
            if (!rc.Contains(m)) {
                Hide();
                return;
            }
            });
        closeTimer->StartOnce(inter);
    }
    }
private:
    bool isvalid = true;
    wxHtmlWindow* htmlWindow;
    wxLongLong startTimeWin;
    wxSize sizew;
    FunctionPGHelper* hhelper;
    std::vector<wxString> hist;
    void SetPage(wxString innerbody,bool gethistory=false) {
        wxString h;
        int p = innerbody.Find("<body>");
        if (innerbody.Find("<html>")>=0) h = innerbody;
        else
            if (p > -1) {
                innerbody.Replace("<body>", "<html><body TEXT=\"#000000\" BGCOLOR=\"#FFFFE0\" LINK=\"#0000FF\" VLINK=\"#FF0000\" ALINK=\"#000088\">", false);
                h = "" + innerbody + "";
            }
            else 
                h = "<html><body  TEXT=\"#000000\" BGCOLOR=\"#FFFFE0\" LINK=\"#0000FF\" VLINK=\"#FF0000\" ALINK=\"#000088\">" + innerbody + "</body></hmtl>";

        if (gethistory) {
            if (hist.size() < 2) {
                Hide();
                return;
            }
            hist.pop_back();
            h = hist[hist.size()-1];
        }
        else {
            hist.push_back(h);
        }
        htmlWindow->SetPage(h);
    }
private:
    wxTimer *closeTimer=NULL;
};
#endif
