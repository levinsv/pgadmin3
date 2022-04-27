//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// dlgExtension.h - Extension property
//
//////////////////////////////////////////////////////////////////////////


#ifndef __DLG_EXTENSIONPROP
#define __DLG_EXTENSIONPROP

#include "dlg/dlgProperty.h"

class pgproJob;

class dlgProJob : public dlgProperty
{
public:
	dlgProJob(pgaFactory *factory, frmMain *frame, pgproJob *job);
	int Go(bool modal);

	void CheckChange();
	wxString GetSql();
	pgObject *CreateObject(pgCollection *collection);
	pgObject *GetObject();

private:
	pgproJob *job;
	wxString oldsrc;
	ctlComboBox* cbRunas;
	wxString StrAttribute(const wxString& name, const wxString& value);
	void append(wxString& str, const wxString& delimiter, const wxString& what);
	void OnChangeName(wxCommandEvent &ev);
	void OnSelChangeCmds(wxListEvent& ev);
	void OnAdd(wxCommandEvent& ev);
	void OnChange(wxCommandEvent& ev);
	void OnRemove(wxCommandEvent& ev);
#ifdef __WXMAC__
	void OnChangeSize(wxSizeEvent &ev);
#endif

	DECLARE_EVENT_TABLE()
};


#endif
