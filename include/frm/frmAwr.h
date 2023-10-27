#pragma once
#include "dlg/dlgClasses.h"

class frmAwr :
    public pgDialog
{
public:
	frmAwr(frmMain* parent,pgObject *obj);
	~frmAwr();
	void GeterateReport();

private:
	void OnChange(wxCommandEvent& ev);
	void OnOK(wxCommandEvent& ev);
	void OnCancel(wxCommandEvent& ev);
	void OnBrowseFile(wxCommandEvent& ev);
	frmMain* parent;
	pgObject* obj;
	int ctype = 1;
	wxString cServer, cstart1, cstart2, cend1, cend2;

	DECLARE_EVENT_TABLE()

};

class reportAwrFactory : public actionFactory
{
private:
	wxString titleline;
	wxString list_head;
	wxString rowlist;
	wxString list_end;
	wxString tableheader;
	wxString head;
	wxString tableheader2;
	wxString tableshtml;
	int startpathpos, countdiffline;
	short Diff_EditCost;
	float Match_Threshold;
	int Match_Distance;
protected:
	//reportCompareFactory(menuFactoryList *list) : actionFactory(list) {}
	wxWindow* StartDialog(frmMain* form, pgObject* obj);
	frmMain* GetFrmMain()
	{
		return parent;
	};
	frmMain* parent;
public:
	reportAwrFactory(menuFactoryList* list, wxMenu* mnu, ctlMenuToolbar* toolbar);
	bool CheckEnable(pgObject* obj);
};

