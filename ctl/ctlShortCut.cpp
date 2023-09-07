#include "pgAdmin3.h"
#include "ctl/ctlShortCut.h"
#include "frm/frmMain.h"

ctlShortCut::ctlShortCut(frmMain* main,wxWindow* parent, wxWindowID id, const wxPoint& pos , const wxSize& size )
{
    wxSize sz(400, -1);
    int style = wxCB_SIMPLE| wxTE_PROCESS_ENTER;
    
    frm = main;
    imageList = main->GetImageList();
    //wxComboCtrl* comboCustom = new wxComboCtrl();
    //wxCheckBox* cbox = new wxCheckBox();
    txt = new wxTextCtrl();
    SetMainControl(txt);
    Create(parent, id, wxEmptyString, pos, sz, style);
    //comboCustom->Create(panel, wxID_ANY, wxEmptyString);
    txt->Create(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, style);
        txt->SetFont(settings->GetSystemFont());
    //txt->SetBackgroundColour(*wxWHITE);
    pop = new wxVListBoxComboPopup();
    ///UseAltPopupWindow(false);
    SetPopupControl(pop);


//    wxArrayString ar;
//    ar.Add("100000");
//    ar.Add("200000");
//    ar.Add("300000");
//    this->Append(ar);
    txt->Bind(wxEVT_TEXT_ENTER, &ctlShortCut::OnTEXT_ENTER, this);
    
    txt->Bind(wxEVT_TEXT, &ctlShortCut::OnTEXT, this);
    txt->Bind(wxEVT_CHAR, &ctlShortCut::OnCharT, this);
    pop->Bind(wxEVT_CHAR, &ctlShortCut::OnChar, this);
    Bind(wxEVT_COMBOBOX, &ctlShortCut::OnCombo, this);
    
   // this->CentreOnParent();
    ;
}
wxTextCtrl* ctlShortCut::GetTextControl() {
    return txt;
}
void ctlShortCut::SetText(wxString &str,bool settext) {
    if (settext) {
        txt->SetValue(str);
    }
    this->Clear();
    fnd = str;
    this->Append(BuildList(str, false));
}
wxString ctlShortCut::viewText(wxString& s) {
    wxString src = s;
    src = src.AfterFirst('/').AfterFirst('/');
    src.Replace(_("Schemas"), "");
    src.Replace(_("Tables"), "");
    src.Replace(_("Views"), "");
    src.Replace(_("Functions"), "");
    src.Replace(_("Procedures"), "");
    src.Replace(_("Databases"), "");

    src.Replace(_("Trigger Functions"), "");
    src.Replace(_("Sequences"), "");
    src.Replace(_("Foreign Tables"), "");
    src.Replace(_("Partitions"), "");
    src.Replace(_("Types"), "");
    src.Replace(_("Triggers"), "");
    src.Replace("//", "/");
    return src;
}
wxArrayString ctlShortCut::BuildList(wxString& find, bool full) {
    wxArrayString rez;
    if (find.Length()==0) return frm->shortcut;
    for (int i = 0; i<frm->shortcut.Count(); i++) {
        wxString t = viewText(frm->shortcut[i]);
        if (t.Find(find) != wxNOT_FOUND) {
            if (full)
            {
                if (find.Length() == t.length()) rez.Add(frm->shortcut[i]);
            }
           else
                rez.Add(frm->shortcut[i]);

        }
    }
    if (rez.Count() > 0|| full) return rez;

    return frm->shortcut;
}
ctlShortCut::~ctlShortCut() {
    //delete txt;
    //delete pop;
}

void ctlShortCut::OnCombo(wxCommandEvent& event) {
    // go link
    wxString s = GetValue();
    if (s.Length() > 0) frm->select_shortcut = s;
    GetParent()->Close(true);

}
void ctlShortCut::OnTEXT_ENTER(wxCommandEvent& event ) {

    wxString s = txt->GetValue();
    //wxMessageBox("Enter press\n Add link "+s, "Ontext_Enter");
    wxArrayString a=BuildList(s, true);
    if (a.Count() > 0) {
        // go link
    }
    else
    {
        frm->select_shortcut=s;
    }
    if (s.Length()>0) frm->select_shortcut = '@'+s;
    GetParent()->Close(true);
}
void ctlShortCut::OnTEXT(wxCommandEvent& event) {
    if (!IsPopupShown() && ( true) ) {
        wxString s = txt->GetValue();
        SetText(s,false);
        if (pop->GetCount() > 0) {
            Popup();
        }
        //int e = txt->SetSelection(0, 1);
        //
    }
}
void ctlShortCut::OnCharT(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_ESCAPE) {
        GetParent()->Close(true);
    }
    event.Skip();
}
void ctlShortCut::OnChar(wxKeyEvent& event) {
    if (IsPopupShown()) {
        wxString s = txt->GetValue();
        wxChar charcode = event.GetUnicodeKey();
        if (event.GetKeyCode() == WXK_BACK) {
            txt->EmulateKeyPress(event);
            //txt->SetValue(s.Left(s.Length()-1));
        }
        if (wxIsprint(charcode))
        {
            //txt->EmulateKeyPress(event);
            s += charcode;
            SetText(s, false);
            txt->SetValue(s);
            pop->Refresh();
            return;
        }
        else event.Skip();

        
    }

event.Skip();
}
dlgShortCut::dlgShortCut(frmMain* parent, wxWindowID id, wxPoint pos, const wxSize size)
    :wxDialog(reinterpret_cast<wxWindow*> (parent), id, wxEmptyString, pos, size, wxSUNKEN_BORDER | wxSTAY_ON_TOP)
{
   // SetExtraStyle(GetExtraStyle() & ~wxWS_EX_BLOCK_EVENTS);
    //this->SetSize(wxSize(243, 165));
    wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);

    ctlShortCut *cb=new ctlShortCut(parent,this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    frm = parent;
    wxTreeItemId item = frm->GetBrowser()->GetSelection();
    wxString curparh=frm->GetNodePath(item);
    curparh = "";
    cb->SetText(curparh, true);
    wxTextCtrl *c=cb->GetTextControl();
    
    cb->Bind(wxEVT_KEY_DOWN, &dlgShortCut::OnKEY_DOWN, this);
   // Bind(wxEVT_CHAR_HOOK, &dlgShortCut::OnCharHook, this);
    
    vSizer->Add(cb, 0, wxEXPAND | wxALL, 3);
    SetSizerAndFit(vSizer);
    //SetSizer(vSizer);
}
dlgShortCut::~dlgShortCut() {
    //delete cb;
}
void dlgShortCut::OnCharHook(wxKeyEvent& event) {

    event.Skip();
}
void dlgShortCut::OnKEY_DOWN(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_ESCAPE) {
        Close(true);
    }
    HWND f=GetFocus();
    if (!cb->HasFocus()) {

    }
    event.Skip();
}
