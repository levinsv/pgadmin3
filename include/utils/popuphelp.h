#ifndef POPUPHELP_H
#define POPUPHELP_H

#include "wx/popupwin.h"
#include <wx/html/htmlwin.h>
#include "wx/clipbrd.h"
#include "utils/FunctionPGHelper.h"
#include <wx/regex.h>
#include <map>
#include <vector>

class popuphelp :
	public wxPopupTransientWindow
{
public:
	//popuphelp(wxWindow* parent);
	bool ProcessLeftDown(wxMouseEvent& event)
	{
		return false;
	}
	bool IsValid() {
		return isvalid;
	}
	popuphelp(wxWindow* parent,wxString keyword, FunctionPGHelper *hhelper) : wxPopupTransientWindow(parent) {
		SetSize(450,370);
		this->hhelper = hhelper;
		SetBackgroundColour(*wxBLACK);
		htmlWindow = new wxHtmlWindow(this, -1, wxDefaultPosition,GetSize());
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
        //wxString body = this->hhelper->getHelpString(name);
        wxString ctext = htmlWindow->SelectionToText();
        if (!ctext.IsEmpty()) {
            wxClipboardLocker clip;
            if (!clip ||
                !wxTheClipboard->AddData(new wxTextDataObject(ctext)))
            {
               
            }
            Hide();
            return;
        }
        this->SetPage("", true);
        //ctext=htmlWindow->SelectionToText();
        //wxString s = wxString::Format("cell = %s",ctext.c_str());
        });

	}
private:
	bool isvalid = true;
	wxHtmlWindow* htmlWindow;
	FunctionPGHelper* hhelper;
    std::vector<wxString> hist;
	void SetPage(wxString innerbody,bool gethistory=false) {
        wxString h;
        int p = innerbody.Find("<body>");
        if (p > -1) {
            innerbody.Replace("<body>", "<html><body TEXT=\"#000000\" BGCOLOR=\"#FFFFA0\" LINK=\"#0000FF\" VLINK=\"#FF0000\" ALINK=\"#000088\">", false);
            h = "" + innerbody + "";
        } else 
            h = "<html><body  TEXT=\"#000000\" BGCOLOR=\"#FFFFA0\" LINK=\"#0000FF\" VLINK=\"#FF0000\" ALINK=\"#000088\">" + innerbody + "</body></hmtl>";

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
};
#endif
