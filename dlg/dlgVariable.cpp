//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// dlgVariable.cpp - Variable replace
//
//////////////////////////////////////////////////////////////////////////



// App headers
#include "pgAdmin3.h"

#include "dlg/dlgVariable.h"
#include "ctl/ctlSQLBox.h"
#include <wx/mstream.h>
#include <wx/xml/xml.h>

BEGIN_EVENT_TABLE(dlgVariable, pgDialog)
EVT_BUTTON(wxID_CANCEL, dlgVariable::OnCancel)
EVT_BUTTON(wxID_OK,     dlgVariable::OnOk)
EVT_BUTTON(wxID_IGNORE, dlgVariable::OnIgnore)
EVT_CLOSE(dlgVariable::OnClose)
END_EVENT_TABLE()


#define btnOk			     CTRL_BUTTON("wxID_OK")
#define btnCancel			 CTRL_BUTTON("wxID_CANCEL")


const char* const xrc_data_head = R"(<?xml version="1.0" encoding="ISO-8859-1"?>
<resource>
  <object class="wxDialog" name="dlgVariable">
    <title>Variables</title>
    <size>300,166d</size>
    <style>wxDEFAULT_DIALOG_STYLE|wxCAPTION|wxSYSTEM_MENU|wxRESIZE_BORDER</style>
    <object class="wxFlexGridSizer">
      <cols>1</cols>
      <growablecols>0</growablecols>
      <growablerows>0</growablerows>
      <object class="sizeritem">
        <object class="wxFlexGridSizer">
          <cols>2</cols>
          <growablecols>1</growablecols>

)";
const char* const xrc_data_tail = R"(
        </object>
        <flag>wxEXPAND|wxTOP|wxLEFT|wxRIGHT</flag>
        <border>4</border>
      </object>
      <object class="sizeritem">
        <object class="wxFlexGridSizer">
          <cols>4</cols>
          <growablecols>0</growablecols>
          <object class="spacer">
            <size>0,0d</size>
          </object>
          <object class="sizeritem">
            <object class="wxButton" name="wxID_OK">
              <label>&amp;OK</label>
              <default>1</default>
            </object>
            <flag>wxEXPAND|wxALL</flag>
            <border>3</border>
          </object>
          <object class="sizeritem">
            <object class="wxButton" name="wxID_IGNORE">
              <label>Ignore</label>
              <tooltip>Ignore variable replace</tooltip>
            </object>
            <flag>wxALIGN_CENTRE_VERTICAL|wxALL</flag>
            <border>4</border>
          </object>
          <object class="sizeritem">
            <object class="wxButton" name="wxID_CANCEL">
              <label>&amp;Close</label>
              <tooltip>Close the dialog</tooltip>
            </object>
            <flag>wxALIGN_CENTRE_VERTICAL|wxALL</flag>
            <border>4</border>
          </object>
        </object>
        <flag>wxEXPAND</flag>
      </object>
    </object>
  </object>
</resource>
)";
dlgVariable::dlgVariable(ctlSQLBox* parent, wxString& query, std::vector<var_query>& var_list) :
	pgDialog()
{
	SetFont(settings->GetSystemFont());
    wxString varname = "var";
    wxString xmlbody;
    wxString l = R"(          <object class="sizeritem">
            <object class="wxStaticText" name="lbl%s">
              <label>%s</label>
            </object>
            <flag>wxALIGN_CENTRE_VERTICAL|wxTOP|wxLEFT|wxRIGHT</flag>
            <border>4</border>
          </object>
          <object class="sizeritem">
            <object class="ctlComboBox" name="cb%s">
                    <style>wxCB_DROPDOWN</style>
                    %s
              <tooltip>Enter the variable value</tooltip>
            </object>
            <flag>wxEXPAND|wxTOP|wxLEFT|wxRIGHT</flag>
            <border>4</border>
          </object>
)";
    wxJSONValue def(wxJSONType::wxJSONTYPE_OBJECT);
    opt.SetType(wxJSONType::wxJSONTYPE_OBJECT);
    dlgName = wxT("dlgVariable");
    settings->ReloadJsonFileIfNeed();
    settings->ReadJsonObect(dlgName, opt, def);
    
    int idx = 1;
    for (int i = 0; i < var_list.size(); i++)
    {
        wxString name = var_list[i].txtVar;
        if (name.length() > 1) name = name.substr(1);
        
        if (unic_name.count(name) > 0) {
            idx = unic_name[name];
            continue;
        }
        else
            unic_name[name] = i;
        wxString content="<content/>";
        varname = wxString::Format("var%d", idx);
        xmlbody += wxString::Format(l, varname, name, name,content);
        idx++;
    }
    wxString xrc_data = xrc_data_head + xmlbody + xrc_data_tail;
    
    wxMemoryInputStream mis(xrc_data, strlen(xrc_data));
    wxScopedPtr<wxXmlDocument> xmlDoc(new wxXmlDocument(mis, "UTF-8"));
    if (!xmlDoc->IsOk())
    {
            return;
    }

    if (!wxXmlResource::Get()->LoadDocument(xmlDoc.release(), dlgName))
    {
            return;
    }

	LoadResource(parent, dlgName);
	RestorePosition();
	//wxStaticText statusBar = new wxStatusBar(this, -1, flags);
	int flags = 0;

	// Icon
	appearanceFactory->SetIcons(this);
	//if (startsqlbox->GetQueryBook()==NULL) chkOptionsAllFind->Disable();
	// Load up the defaults
	wxString val;
	bool bVal;
    v_list = var_list;
    originalquery = query;
    
    for (int i = 0; i < v_list.size(); i++)
    {
        wxString name = v_list[i].txtVar;
        if (name.length() > 1) name = name.substr(1);
        ctlComboBox* cb = CTRL_COMBOBOX2("cb" + name);
        if (cb) {
            if (opt.HasMember(name) && cb->GetCount()==0) {
                
                cb->Bind(wxEVT_COMMAND_TEXT_UPDATED, &dlgVariable::OnChangeCombo,this);
                wxJSONValue ar(wxJSONType::wxJSONTYPE_ARRAY);
                int ar_size = opt[name].Size();
                for (int i = 0; i < opt[name].Size(); i++) {
                    wxString val = opt[name][i].AsString();
                    cb->AppendString(val);
                }
                if (ar_size > 0) cb->SetSelection(0);
            }
        }
    }

	wxCommandEvent ev;
	OnChange(ev);
	ResetTabOrder();
}
void dlgVariable::OnChangeCombo(wxCommandEvent& ev) {
    ctlComboBox *c= (ctlComboBox *) ev.GetEventObject();
    c->GuessSelection(ev);
}
wxString dlgVariable::GetQuery() {
    return modquery;
}
dlgVariable::~dlgVariable()
{
	SavePosition();
    wxXmlResource::Get()->Unload(dlgName);
}

void dlgVariable::SaveSettings()
{
    
}

void dlgVariable::OnClose(wxCloseEvent &ev)
{
	SaveSettings();
	pgDialog::OnClose(ev);
}

void dlgVariable::OnCancel(wxCommandEvent &ev)
{
	//SaveSettings();
    wxCloseEvent e;
    pgDialog::OnClose(e);
}

void dlgVariable::OnChange(wxCommandEvent &ev)
{
}

void dlgVariable::OnIgnore(wxCommandEvent &ev)
{
	//if (txtFind->GetValue().IsEmpty()) 		return;
    if (IsModal())
        EndModal(wxID_IGNORE);

}

void dlgVariable::OnOk(wxCommandEvent &ev)
{
	//if (txtFind->GetValue().IsEmpty()) 		return;
    
    int idx = 0;
    int lastpos = 0;
    std::map<wxString, int> unic_n;
    for (int i = 0; i < v_list.size(); i++)
    {
        wxString name = v_list[i].txtVar;
        if (name.length() > 1) name = name.substr(1);
        ctlComboBox* cb = CTRL_COMBOBOX2("cb"+name);
        wxString varVal=cb->GetValue();
        v_list[i].txtReplace = varVal;
        // save json
        if (unic_n.count(name) > 0) {
            
        }
        else {
            unic_n[name] = i;
            int sel = cb->GetGuessedSelection();
            wxJSONValue ar(wxJSONType::wxJSONTYPE_ARRAY);
            int limit = 20;
            if (opt.HasMember(name)) {
                int ar_size = opt[name].Size();
                for (int i = 0; i < ar_size; i++) {
                    wxString val = opt[name][i].AsString();
                    if (i == 0) ar.Append(varVal);
                    if (i == sel) continue;
                    ar.Append(val);
                    if (i == limit) break;
                }
            }
            else {
                // first
                ar.Append(varVal);
            }
            opt[name] = ar;
        }
        // var replace
        int startv = v_list[i].position;
        modquery += originalquery.substr(lastpos, startv - lastpos);
        lastpos += startv - lastpos;
        modquery += varVal;
        lastpos += v_list[i].txtVar.length();
    }
    if (lastpos< originalquery.length()) modquery += originalquery.substr(lastpos);

    settings->WriteJsonObect(dlgName, opt);
    SaveSettings();
    if (IsModal())
        EndModal(wxID_OK);

}

void dlgVariable::FindNext()
{
}


void dlgVariable::ResetTabOrder()
{
//	btnOk->MoveAfterInTabOrder(chkOptionsUseRegexps);
//	btnCancel->MoveAfterInTabOrder(btnReplaceAll);
}

