#pragma once
#include "pgAdmin3.h"
#include <wx/clipbrd.h>
#include "ctl/ctlStyledText.h"
class dlgTransformText :
    public pgDialog
{
public:
	dlgTransformText(ctlSQLBox* form,const wxString source);
	~dlgTransformText();
	
	void SetSource(const wxString& src);
private:
	void OnCheckUI(wxCommandEvent& ev);
	void OnOk(wxCommandEvent& ev);
	void OnClose(wxCloseEvent& ev);
	void OnCancel(wxCommandEvent& ev);
	void OnComboSelect(wxCommandEvent& ev);
	void OnChange(wxCommandEvent& ev);
	void OnChangeLimit(wxCommandEvent& ev);
	void OnChangeOnline(wxCommandEvent& ev);
	void OnSave(wxCommandEvent& ev);
	void OnLoad(wxCommandEvent& ev);
	
	void OnChangeRegEx2(wxStyledTextEvent& ev);
	void TransformText(const wxRegEx &regfld);
	void OnIdle(wxIdleEvent &ev);
	struct  replace_opt {
		bool isGroup = false;
		int nGroup = -1;
		wxString text;
	};
	wxJSONValue LoadConfig(const wxString confname);
	void CheckLimits();
	wxJSONValue FillConfig();
	void LoadOptions();
	void SetDecoration(ctlStyledText* s);
	void SetStyled(ctlStyledText* s);
	void showNumber(ctlStyledText* text, bool visible);
	void AppendTextControl(ctlStyledText* ctrl, const wxString appendtext, bool isnewline = false);
	std::vector<replace_opt> BuildString(const wxString repstr);

	wxString src;
	
	wxString srcRowSep;
	wxJSONValue opt,lastconf;
	int countGroupColor = 0;
	int limitLine;
	int limitChar;
	wxString strResult;
	// UI
	bool inizialize;
	bool isChange = false;
	bool isNeedTransform = false;
	bool isOnline = false;
	wxString strReg,strRep;
	wxColour bgerror;
	DECLARE_EVENT_TABLE()

};

