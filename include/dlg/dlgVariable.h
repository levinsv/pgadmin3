//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// dlgVariable.h - Variables replace
//
//////////////////////////////////////////////////////////////////////////

#ifndef dlgVariable_H
#define dlgVariable_H
//#include "pgAdmin3.h"
#include "dlg/dlgClasses.h"
#include "ctl/ctlAuiNotebook.h"

class ctlSQLBox;
struct var_query { int position = -1; wxString txtVar; wxString txtReplace; };

// Class declarations
class dlgVariable : public pgDialog
{
public:
	dlgVariable(ctlSQLBox *parent,wxString &query, std::vector<var_query> &var_list);
	~dlgVariable();
	void FindNext();
	wxString GetQuery();
	void OnChangeCombo(wxCommandEvent& ev);

private:

	void OnClose(wxCloseEvent &ev);
	void OnCancel(wxCommandEvent &ev);
	void OnChange(wxCommandEvent &ev);
	void OnOk(wxCommandEvent &ev);
	void OnIgnore(wxCommandEvent &ev);
	void ResetTabOrder();
	void SaveSettings();
	wxString modquery,originalquery;
	std::map<wxString, int> unic_name;

	std::vector<var_query> v_list;
	wxJSONValue opt;
	DECLARE_EVENT_TABLE()
};

#endif
