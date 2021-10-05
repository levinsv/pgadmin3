//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// frmMaintenance.cpp - Maintenance options selection dialogue
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>
#include <wx/settings.h>
#include <wx/xrc/xmlres.h>


// App headers
#include "pgAdmin3.h"
#include "ctl/ctlMenuToolbar.h"
#include "frm/frmHint.h"
#include "frm/frmMaintenance.h"
#include "frm/frmMain.h"
#include "utils/sysLogger.h"
#include "schema/pgIndex.h"

// Icons
#include "images/vacuum.pngc"


BEGIN_EVENT_TABLE(frmMaintenance, ExecutionDialog)
	EVT_RADIOBOX(XRCID("rbxAction"),    frmMaintenance::OnAction)
END_EVENT_TABLE()

#define nbNotebook              CTRL_NOTEBOOK("nbNotebook")
#define rbxAction               CTRL_RADIOBOX("rbxAction")
#define chkFull                 CTRL_CHECKBOX("chkFull")
#define chkFreeze               CTRL_CHECKBOX("chkFreeze")
#define chkAnalyze              CTRL_CHECKBOX("chkAnalyze")
#define chkVerbose              CTRL_CHECKBOX("chkVerbose")
#define chkDISABLE_PAGE_SKIPPING  CTRL_CHECKBOX("chkDISABLE_PAGE_SKIPPING")
#define chkCONCURRENTLY         CTRL_CHECKBOX("chkCONCURRENTLY")


#define stBitmap                CTRL("stBitmap", wxStaticBitmap)



frmMaintenance::frmMaintenance(frmMain *form, pgObject *obj) : ExecutionDialog(form, obj)
{
	SetFont(settings->GetSystemFont());
	LoadResource(form, wxT("frmMaintenance"));
	RestorePosition();

	SetTitle(object->GetTranslatedMessage(MAINTENANCEDIALOGTITLE));

	txtMessages = CTRL_TEXT("txtMessages");

	// Icon
	SetIcon(*vacuum_png_ico);

	// Note that under GTK+, SetMaxLength() function may only be used with single line text controls.
	// (see http://docs.wxwidgets.org/2.8/wx_wxtextctrl.html#wxtextctrlsetmaxlength)
#ifndef __WXGTK__
	txtMessages->SetMaxLength(0L);
#endif
	bool iscomprss = false;
	wxString cmd;
	if (object->GetMetaType() == PGM_INDEX || object->GetMetaType() == PGM_PRIMARYKEY || object->GetMetaType() == PGM_UNIQUE)
	{
		rbxAction->SetSelection(2);
		rbxAction->Enable(0, false);
		rbxAction->Enable(1, false);
		if (object->GetDatabase()->connection()->GetIsPgProEnt()) {
			wxString ratio = object->GetConnection()->ExecuteScalar("select left((cfs_fragmentation("+object->GetOidStr()+")*100)::text,5)::text");
			iscomprss = !(ratio == "NaN");
		}
		wxString sql = "SELECT \
			case when(SELECT max(pronargs) FROM pg_proc WHERE proname = 'bt_index_check') = 3 then\
			'bt_index_check(' || c.oid || ', true,' || i.indisunique || ')'\
			when(SELECT max(pronargs) FROM pg_proc WHERE proname = 'bt_index_check') = 2 then\
			'bt_index_check(' || c.oid || ', true)'\
		else\
			''\
			end\
			FROM pg_index i\
			JOIN pg_opclass op ON i.indclass[0] = op.oid\
			JOIN pg_am am ON op.opcmethod = am.oid\
			JOIN pg_class c ON i.indexrelid = c.oid\
			JOIN pg_namespace n ON c.relnamespace = n.oid\
			WHERE am.amname = 'btree'\
			AND c.relpersistence != 't'\
			AND c.relkind = 'i' AND i.indisready AND i.indisvalid and c.oid = "+object->GetOidStr();
			cmd = object->GetConnection()->ExecuteScalar(sql);
			cmdcheck = cmd;

	}
	if (object->GetMetaType() == PGM_TABLE) {
		pgTable* t = (pgTable *)object;
		iscomprss = !(t->GetRatio() == "");

	}
	rbxAction->Enable(4, iscomprss);
	rbxAction->Enable(5, !cmd.IsEmpty());
	wxCommandEvent ev;
	OnAction(ev);
}


frmMaintenance::~frmMaintenance()
{
	SavePosition();
	Abort();
}


wxString frmMaintenance::GetHelpPage() const
{
	wxString page;
	switch ((XRCCTRL(*(frmMaintenance *)this, "rbxAction", wxRadioBox))->GetSelection())
	{
		case 0:
			page = wxT("pg/sql-vacuum");
			break;
		case 1:
			page = wxT("pg/sql-analyze");
			break;
		case 2:
			page = wxT("pg/sql-reindex");
			break;
		case 3:
			page = wxT("pg/sql-cluster");
			break;
	}
	return page;
}



void frmMaintenance::OnAction(wxCommandEvent &ev)
{
	bool isVacuum = (rbxAction->GetSelection() == 0);
	chkFull->Enable(isVacuum);
	chkFreeze->Enable(isVacuum);
	chkAnalyze->Enable(isVacuum);
	chkDISABLE_PAGE_SKIPPING->Enable(isVacuum && conn->BackendMinimumVersion(10, 0));

	bool isReindex = (rbxAction->GetSelection() == 2);
	chkCONCURRENTLY->Enable(isReindex && conn->BackendMinimumVersion(12, 0));

	bool isCluster = (rbxAction->GetSelection() == 3);
	if ((isCluster && !conn->BackendMinimumVersion(8, 4)))
	{
		chkVerbose->SetValue(false);
		chkVerbose->Enable(false);
	}
	else
	{
		chkVerbose->SetValue(true);
		chkVerbose->Enable(true);
	}
	bool isAmcheck = (rbxAction->GetSelection() == 5);
	if (isAmcheck) {

	}

}



wxString frmMaintenance::GetSql()
{
	wxString sql;

	switch (rbxAction->GetSelection())
	{
		case 0:
		{
			/* Warn about VACUUM FULL on < 9.0 */
			if (chkFull->GetValue() &&
			        !conn->BackendMinimumVersion(9, 0))
			{
				if (frmHint::ShowHint(this, HINT_VACUUM_FULL) == wxID_CANCEL)
					return wxEmptyString;
			}
			sql = wxT("VACUUM ");
			wxString opt = "";
			if (chkFull->GetValue())
				AppendIfFilled(opt,",",wxT("FULL"));
			if (chkFreeze->GetValue())
				AppendIfFilled(opt,",",wxT("FREEZE"));
			if (chkVerbose->GetValue())
				AppendIfFilled(opt,",",wxT("VERBOSE"));
			if (chkAnalyze->GetValue())
				AppendIfFilled(opt,",",wxT("ANALYZE"));
			if (chkDISABLE_PAGE_SKIPPING->GetValue())
				AppendIfFilled(opt,",",wxT("DISABLE_PAGE_SKIPPING"));
			sql += opt.IsNull() ? "" : "("+opt.Mid(1)+")";
			if (object->GetMetaType() != PGM_DATABASE)
				sql += object->GetQuotedFullIdentifier();

			break;
		}
		case 1:
		{
			sql = wxT("ANALYZE ");
			if (chkVerbose->GetValue())
				sql += wxT("VERBOSE ");

			if (object->GetMetaType() != PGM_DATABASE)
				sql += object->GetQuotedFullIdentifier();

			break;
		}
		case 2:
		{
			sql = wxT("REINDEX ");
			if (chkVerbose->GetValue())
				sql += wxT("(VERBOSE) ");

			if (object->GetMetaType() == PGM_UNIQUE || object->GetMetaType() == PGM_PRIMARYKEY)
			{
				sql += wxT("INDEX ");
				sql += chkCONCURRENTLY->GetValue() ? "CONCURRENTLY ": "";
				sql	+= object->GetQuotedFullIdentifier();
			}
			else // Database, Tables, and Index (but not Constraintes ones)
			{
				sql += object->GetTypeName().Upper();
				sql += chkCONCURRENTLY->GetValue() ? " CONCURRENTLY ": " ";
				sql += object->GetQuotedFullIdentifier();
			}
			break;
		}
		case 3:
		{
			sql = wxT("CLUSTER ");

			if (chkVerbose->GetValue())
				sql += wxT("VERBOSE ");
			if (object->GetMetaType() == PGM_TABLE)
				sql += object->GetQuotedFullIdentifier();
			if (object->GetMetaType() == PGM_INDEX || object->GetMetaType() == PGM_UNIQUE
			        || object->GetMetaType() == PGM_PRIMARYKEY)
			{
				sql += object->GetSchema()->GetQuotedFullIdentifier();
				if (conn->BackendMinimumVersion(8, 4))
				{
					sql += wxT(" USING ") + object->GetQuotedIdentifier();
				}
				else
				{
					sql += wxT(" ON ") + object->GetQuotedIdentifier();
				}
			}
			break;
		}
		case 4:
		{
			sql = wxT("set  cfs_gc_threshold = 0;select ");
			sql += "cfs_gc_relation(" + object->GetOidStr() + ")";
			if (object->GetMetaType() == PGM_TABLE) {
				//sql += object->GetQuotedFullIdentifier();
				wxString strindex= object->GetConnection()->ExecuteScalar("select string_agg('cfs_gc_relation('||indexrelid::text||')',',') from pg_index i where indrelid=" + object->GetOidStr() + " and cfs_fragmentation(indexrelid)  between 0.01 and 1;");
				if (strindex.Len() > 0) sql += ","+strindex;
			}
			break;
		}
		case 5: // amchek
		{
			sql = wxT("SET client_min_messages = DEBUG1;select ");
			sql += cmdcheck;
				//wxString strindex = object->GetConnection()->ExecuteScalar("select string_agg('cfs_gc_relation('||indexrelid::text||')',',') from pg_index i where indrelid=" + object->GetOidStr() + " and cfs_fragmentation(indexrelid)  between 0.01 and 1;");
			break;
		}
	}

	return sql;
}



void frmMaintenance::Go()
{
	chkFull->SetFocus();
	Show(true);
}



maintenanceFactory::maintenanceFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar) : contextActionFactory(list)
{
	mnu->Append(id, _("&Maintenance..."), _("Maintain the current database or table."));
	toolbar->AddTool(id, wxEmptyString, *vacuum_png_bmp, _("Maintain the current database or table."), wxITEM_NORMAL);
}


wxWindow *maintenanceFactory::StartDialog(frmMain *form, pgObject *obj)
{
	frmMaintenance *frm = new frmMaintenance(form, obj);
	frm->Go();
	return 0;
}


bool maintenanceFactory::CheckEnable(pgObject *obj)
{
	return obj && obj->CanMaintenance();
}
