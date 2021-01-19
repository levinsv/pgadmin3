//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// dlgExtension.cpp - PostgreSQL Extension Property
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "utils/misc.h"
#include "utils/pgDefs.h"

#include "dlg/dlgProJob.h"
#include "pro_scheduler/pgproJob.h"


// pointer to controls
//#define cbName          CTRL_COMBOBOX("cbName")
#define cbRunAs		    CTRL_COMBOBOX2("cbRunAs")
#define txtID           CTRL_TEXT("txtID")
#define txtCron         CTRL_TEXT("txtCron")
#define txtNextrun         CTRL_TEXT("txtNextrun")
#define txtLastrun         CTRL_TEXT("txtLastrun")

#define txtSqlBox       CTRL_SQLBOX("txtSqlBox")
#define chkEnabled		CTRL_CHECKBOX("chkEnabled")
#define chkSameTran		CTRL_CHECKBOX("chkSameTran")




dlgProperty *pgproJobFactory::CreateDialog(frmMain *frame, pgObject *node, pgObject *parent)
{
	return new dlgProJob(this, frame, (pgproJob *)node);
}


BEGIN_EVENT_TABLE(dlgProJob, dlgProperty)
	EVT_STC_MODIFIED(XRCID("txtSqlBox"), dlgProperty::OnChangeStc)
	EVT_TEXT(XRCID("txtCron"),						dlgProperty::OnChange)
	EVT_TEXT(XRCID("txtName"),						dlgProperty::OnChange)
	EVT_TEXT(XRCID("txtComment"),					dlgProperty::OnChange)
	EVT_CHECKBOX(XRCID("chkEnabled"),				dlgProperty::OnChange)
	EVT_CHECKBOX(XRCID("chkSameTran"),				dlgProperty::OnChange)
	EVT_COMBOBOX(XRCID("cbRunAs"),					dlgProperty::OnChange)

END_EVENT_TABLE();


dlgProJob::dlgProJob(pgaFactory *f, frmMain *frame, pgproJob *node)
	: dlgProperty(f, frame, wxT("dlgProJob"))
{
	job = node;
}


pgObject *dlgProJob::GetObject()
{
	return job;
}


int dlgProJob::Go(bool modal)
{
	//txtComment->Disable();

	// add all schemas
	//cbObjectsSchema->Append(wxEmptyString);
	//pgSetIterator schemas(connection,
	//                      wxT("SELECT nspname FROM pg_namespace\n")
	//                      wxT(" ORDER BY nspname"));

	//while (schemas.RowsLeft())
	//	cbObjectsSchema->Append(schemas.GetVal(wxT("nspname")));
	//cbObjectsSchema->SetSelection(0);

	if (job)
	{
		// edit mode
		//txtName->Append(job->GetName());
		//txtName->SetValue(job->GetTryName());
		ctlComboBoxFix* cbowner = (ctlComboBoxFix*)cbOwner;
		AddUsers(cbowner);
		cbowner->SetValue(job->GetOwner());
		ctlComboBoxFix* cbRunas = (ctlComboBoxFix*)cbRunAs;
		AddUsers(cbRunas);
		if (cbRunas)
			cbRunas->SetValue(job->GetRunAs());

		chkEnabled->SetValue(job->GetEnabled());
		txtID->SetValue(NumToStr(job->GetRecId()));
		txtID->Disable();
		wxString t2 = job->GetCrontab();
		txtCron->SetValue(t2);
		chkSameTran->SetValue(job->GetSameTransaction());
		
		int pos = 0;
		pgSetIterator cmds(connection, wxT("select unnest(commands) from schedule.get_cron() c where c.id=")+NumToStr(job->GetRecId()));
		wxString src;
			while (cmds.RowsLeft())
			{
				wxString cmd=cmds.GetVal(0);
				if (!src.IsEmpty()) src += "\n";
				src += cmd;

			}
			
		txtSqlBox->SetEOLMode(2);
		txtSqlBox->SetText(src);
		oldsrc = src;

		wxDateTime lr = job->GetStarted();
		wxDateTime nr = job->GetNextSchedule_At(lr, 1);
		if (lr.IsValid()) {
			wxString str = lr.FormatISODate() + wxT(" ") + lr.FormatISOTime();
			wxTimeSpan sp=nr- wxDateTime::Now();
			wxLongLong d=sp.GetMilliseconds();
			txtLastrun->SetValue(str);
			str = nr.FormatISODate() + wxT(" ") + nr.FormatISOTime()+" [ "+ ElapsedTimeToStr(d) +" ]";
			txtNextrun->SetValue(str);
		}
	}
	else
	{
		// create mode
		txtCron->SetValue("00 * * * *");
	}

	return dlgProperty::Go(modal);
}
void fillarray(wxArrayString &arr,wxString &src)
{
	int l = src.Length();
	wxUniChar ln = '\n';
	int pos = 0;
	int h = pos;
	bool en = false;
	while (pos<l) {
		if (en)
		{
			if (src.GetChar(pos) == ' ') {
			}
			else {
				arr.Add(src.Mid(h, pos-h-1));
				h = pos;
			}
			en = false;
		}
		if (src.GetChar(pos++) != ln) continue;
		en = true;
	}
	if (h!=pos) arr.Add(src.Mid(h));
}

pgObject *dlgProJob::CreateObject(pgCollection *collection)
{
	wxString name = txtName->GetValue();

	pgObject *obj = projobFactory.CreateObjects(collection, 0, wxT("\n   AND extname ILIKE ") + qtDbString(name));
	return obj;
}

void dlgProJob::OnSelChangeCmds(wxListEvent& ev) {
}
void dlgProJob::OnAdd(wxCommandEvent& ev) {

}
void dlgProJob::OnChange(wxCommandEvent& ev) {

}
void dlgProJob::OnRemove(wxCommandEvent& ev) {

}

void dlgProJob::OnChangeName(wxCommandEvent &ev)
{
	bool relocatable;

	// add all versions
	//cbVersion->Clear();
	//cbVersion->Append(wxEmptyString);
	//pgSetIterator versions(connection,
	//                       wxT("SELECT version, relocatable FROM pg_available_extension_versions\n")
	//                       wxT(" WHERE name=") + qtDbString(cbName->GetValue()) + wxT(" ")
	//                       wxT(" ORDER BY version"));

	//while (versions.RowsLeft())
	//{
	//	relocatable = versions.GetBool(wxT("relocatable"));
	//	cbVersion->Append(versions.GetVal(wxT("version")));
	//}


	OnChange(ev);
}


void dlgProJob::CheckChange()
{
	bool didChange = true;
	if (job && cbRunAs->GetCount()>0)
	{
		didChange = chkEnabled->GetValue() != job->GetEnabled()
		            || txtCron->GetValue() != job->GetCrontab()
					|| chkSameTran->GetValue() != job->GetSameTransaction()
					|| txtName->GetValue() != job->GetName()
					|| txtComment->GetValue() != job->GetComment()
					|| cbRunAs->GetValue() != job->GetRunAs()
					|| txtSqlBox->GetText()!=oldsrc
			;
		EnableOK(didChange);
	}
	else
	{
		bool enable = true;

		CheckValid(enable, !txtName->GetValue().IsEmpty(), _("Please specify name."));
		EnableOK(enable);
	}
}

wxString dlgProJob::StrAttribute(const wxString &name,wxString &value)
{
	wxString tmp = value;
	//tmp.Replace("\'", "\'\'");
	return "\"" + name + "\": \"" + tmp+"\"";
}
void dlgProJob::append(wxString& str, const wxString& delimiter, const wxString& what)
{
	if (!what.IsNull())
		if(str.IsNull()) str +=  what;
			else
				str += delimiter + what;
}
wxString dlgProJob::GetSql()
{
	wxString sql;

	if (job)
	{
		// edit mode
		if (chkEnabled->GetValue() != job->GetEnabled())
			if (chkEnabled->GetValue()) sql = wxT("select schedule.activate_job(") + NumToStr(job->GetRecId()) + wxT(");\n");
			else
				sql = wxT("select schedule.deactivate_job(") + NumToStr(job->GetRecId()) + wxT(");\n");

		
		wxString att;
		if (txtCron->GetValue() != job->GetCrontab()) att= StrAttribute(wxT("cron"),txtCron->GetValue());
		if (txtName->GetValue() != job->GetName()) append(att, ",\n", StrAttribute(wxT("name"), txtName->GetValue()));
		if (txtComment->GetValue() != job->GetComment()) append(att, ",\n", StrAttribute(wxT("comments"), txtComment->GetValue()));
		if (cbRunAs->GetValue() != job->GetRunAs()) append(att, ",\n", StrAttribute(wxT("run_as"), cbRunAs->GetValue()));
		if (chkSameTran->GetValue() != job->GetSameTransaction()) append(att, ",\n", StrAttribute(wxT("use_same_transaction"), BoolToStr(chkSameTran->GetValue())));


		if (txtSqlBox->GetText() != oldsrc) {
			wxArrayString a;
			wxString tmp;
			fillarray(a, txtSqlBox->GetText());
			if (a.GetCount() > 0) {
				for (int i = 0; i < a.GetCount(); i++) append(tmp, ",\n", "\"" + a[i] + "\"");
				tmp = "[" + tmp + "]";
			}
			else tmp = "";
			tmp = "\"commands\": " + tmp;
			if (!att.IsEmpty()) att += ",\n";
			att += tmp;
		}
		if (!att.IsEmpty()) {
			sql += wxT("select schedule.set_job_attributes(") + NumToStr(job->GetRecId()) + "\n";
			att.Replace("\'", "\'\'");
			sql += wxT(",'{") + att + wxT("}');\n");
		}
	}
	else
	{
		//sql = wxT("CREATE EXTENSION ") + qtIdent(cbName->GetValue());
		//AppendIfFilled(sql, wxT("\n   SCHEMA "), qtIdent(cbObjectsSchema->GetValue()));
		//AppendIfFilled(sql, wxT("\n   VERSION "), qtIdent(cbVersion->GetValue()));
	}

	return sql;
}
