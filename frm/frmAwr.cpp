#include "pgAdmin3.h"
#include <wx/file.h>
#include "frm/frmAwr.h"
#include "schema/pgObject.h"
#include <wx/stdpaths.h>
#include "utils/pgfeatures.h"

#define btnOK           CTRL_BUTTON("wxID_OK")
#define rbTypeReport    CTRL_RADIOBOX("rbTypeReport")
#define cbServer        CTRL_COMBOBOX("cbServer")
#define cbStartId1      CTRL_COMBOBOX("cbStartId1")
#define cbStartId2      CTRL_COMBOBOX("cbStartId2")
#define cbEndId1        CTRL_COMBOBOX("cbEndId1")
#define cbEndId2        CTRL_COMBOBOX("cbEndId2")

BEGIN_EVENT_TABLE(frmAwr, pgDialog)

EVT_RADIOBOX(XRCID("rbTypeReport"), frmAwr::OnChange)
EVT_COMBOBOX(XRCID("cbStartId1"), frmAwr::OnChange)
EVT_COMBOBOX(XRCID("cbStartId2"), frmAwr::OnChange)
EVT_COMBOBOX(XRCID("cbEndId1"), frmAwr::OnChange)
EVT_COMBOBOX(XRCID("cbEndId2"), frmAwr::OnChange)
EVT_BUTTON(wxID_OK, frmAwr::OnOK)
EVT_BUTTON(wxID_CANCEL, frmAwr::OnCancel)
END_EVENT_TABLE()


reportAwrFactory::reportAwrFactory(menuFactoryList* list, wxMenu* mnu, ctlMenuToolbar* toolbar) : actionFactory(list) {
	mnu->Append(id, _("Report AWR"), _("Generate AWR report"));
};
bool reportAwrFactory::CheckEnable(pgObject* obj)
{
	if (obj)
	{
		if ((obj->GetMetaType() == PGM_DATABASE ) && !obj->IsCollection() && obj->GetConnection()->HasFeature(FEATURE_PGPRO_PWR))
			return true;
		else
			return false;
	}
	return false;

}

wxWindow* reportAwrFactory::StartDialog(frmMain* form, pgObject* obj) {
	frmAwr* awr= new frmAwr(form,obj);
	if (awr->ShowModal() == wxID_OK)
	{
		awr->GeterateReport();
	}
	delete awr;
	return 0;
}

frmAwr::frmAwr(frmMain* parent, pgObject* obj)
{
	this->parent = parent;
	this->obj = obj;
	SetFont(settings->GetSystemFont());
	LoadResource((wxWindow *)parent, wxT("frmAwr"));
	RestorePosition();
	pgSet* dataSet1 = obj->GetConnection()->ExecuteSet("select server_name from profile.show_servers()");
	if (dataSet1)
	{
		wxString n;
		while (!dataSet1->Eof())
		{
			n = dataSet1->GetVal("server_name");
			cbServer->Append(n);
			dataSet1->MoveNext();
		}
		delete dataSet1;
		if (!n.IsEmpty()) {
			cbServer->SetSelection(0);
		}
		
		wxString val;
		settings->Read(wxT("Awr/Lastserver"), &val, wxEmptyString);
		if (!val.IsEmpty())cbServer->SetValue(val);
		cServer = "";
	}
	int valint;
	settings->Read(wxT("Awr/Type"), &valint, 1);
	rbTypeReport->SetSelection(valint);
	wxCommandEvent ev;
	OnChange(ev);

}

void frmAwr::OnChange(wxCommandEvent& ev)
{
//	cbQuoteChar->Enable(rbQuoteStrings->GetValue() || rbQuoteAll->GetValue());
	int type=rbTypeReport->GetSelection();

	wxString server=cbServer->GetValue();
	if (server != cServer) {
		pgSet* dataSet1 = obj->GetConnection()->ExecuteSet("select sample,sample_time from profile.show_samples('"+ server +"') order by 1 desc");
		if (dataSet1)
		{
			cbStartId1->Clear();
			cbStartId2->Clear();
			cbEndId1->Clear();
			cbEndId2->Clear();
			wxArrayString ar;
			while (!dataSet1->Eof())
			{
				int s = dataSet1->GetLong(wxT("sample"));
				wxString dt= dataSet1->GetVal(wxT("sample_time"));
				ar.Add(dt);
				dataSet1->MoveNext();
			}
			delete dataSet1;
			cbStartId1->Insert(ar,0);
			cbStartId2->Insert(ar, 0);
			cbEndId2->Insert(ar, 0);
			cbEndId1->Insert(ar, 0);

		}
		cServer = server;
	}
	cbStartId2->Enable(type > 0);
	cbEndId2->Enable(type > 0);
	int s1 = cbStartId1->GetSelection();
	int e1 = cbEndId1->GetSelection();
	if (cbStartId1->GetValue() != cstart1 || cbEndId1->GetValue() != cend1 ) {
		if ((s1 - 1) >= 0 && ((e1 >= s1)|| (e1<0))) cbEndId1->SetSelection(s1 - 1);
		cstart1 = cbStartId1->GetValue();
		cend1 = cbEndId1->GetValue();
	}
	
	if (cbStartId2->IsEnabled()) {
		int s1 = cbStartId2->GetSelection();
		int e1 = cbEndId2->GetSelection();
		if (cbStartId2->GetValue() != cstart2 || cbEndId2->GetValue() != cend2) {
			if ((s1 - 1) >= 0 && ((e1 >= s1) || (e1 < 0))) cbEndId2->SetSelection(s1 - 1);
			cstart2 = cbStartId2->GetValue();
			cend2 = cbEndId2->GetValue();
		}
		
	}

	btnOK->Enable(!cbEndId1->GetValue().IsEmpty() );
	if (type == 1) {
		btnOK->Enable(!cbEndId2->GetValue().IsEmpty());
	}
}

void frmAwr::GeterateReport() {
	wxFileName fn("");

	int type = rbTypeReport->GetSelection();
	wxString sql;
	if (type == 0) {
		sql = "select rep from profile.get_report('" + cbServer->GetValue() + "',tstzrange('" + cbStartId1->GetValue() + "','" + cbEndId1->GetValue() + "')) rep";
	}
	else {
		sql = "select rep from profile.get_diffreport('" + cbServer->GetValue() + "',tstzrange('" + cbStartId1->GetValue() + "','" + cbEndId1->GetValue() + "'),tstzrange('" + cbStartId2->GetValue() + "','" + cbEndId2->GetValue() + "')) rep";
	}

	//fn="D:\\PostgreSQL\\cmp.html";
	pgSet* dataSet1 = obj->GetConnection()->ExecuteSet(sql);
	if (dataSet1)
	{
		wxString rep;
		while (!dataSet1->Eof())
		{
			rep = dataSet1->GetVal("rep");
			dataSet1->MoveNext();
		}
		delete dataSet1;
		fn = wxStandardPaths::Get().GetTempDir() + wxT("\\awr_report.html");
		fn.MakeAbsolute();
		wxFile file4(fn.GetFullPath(), wxFile::write);
		if (!file4.IsOpened())
		{
			wxLogError(_("Failed to open file %s."), fn.GetFullPath().c_str());
			return;
		}
		file4.Write(rep, wxConvUTF8);
		file4.Close();

#ifdef __WXMSW__
		wxLaunchDefaultBrowser(fn.GetFullPath());
#else
		wxLaunchDefaultBrowser(wxT("file://") + fn.GetFullPath());
#endif

	}

}
void frmAwr::OnOK(wxCommandEvent& ev)
{
	settings->WriteInt(wxT("Awr/Type"), rbTypeReport->GetSelection());



	if (IsModal())
		EndModal(wxID_OK);
	else
		Destroy();
}
frmAwr::~frmAwr()
{
	SavePosition();
}
void frmAwr::OnCancel(wxCommandEvent& ev)
{
	if (IsModal())
		EndModal(wxID_CANCEL);
	else
		Destroy();
}
