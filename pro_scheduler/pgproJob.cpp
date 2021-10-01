//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// pgproJob.h - PostgreSQL Agent Job
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "utils/misc.h"
#include "frm/frmMain.h"
#include "schema/pgObject.h"
#include "schema/pgCollection.h"
#include "schema/pgDatabase.h"
#include "pro_scheduler/pgproJob.h"

pgproJob::pgproJob(const wxString &newName)
	: pgDatabaseObject(projobFactory, newName)
{
}

wxString pgproJob::GetTranslatedMessage(int kindOfMessage) const
{
	wxString message = wxEmptyString;

	switch (kindOfMessage)
	{
		case RETRIEVINGDETAILS:
			message = _("Retrieving details on pro_scheduler job");
			break;
		case REFRESHINGDETAILS:
			message = _("Refreshing pro_scheduler job");
			break;
		case PROPERTIESREPORT:
			message = _("pro_scheduler job properties report");
			break;
		case PROPERTIES:
			message = _("pgAgent job properties");
			break;
		case DDLREPORT:
			message = _("pro_scheduler job DDL report");
			break;
		case DEPENDENCIESREPORT:
			message = _("pro_scheduler job dependencies report");
			break;
		case DEPENDENCIES:
			message = _("pro_scheduler job dependencies");
			break;
		case DEPENDENTSREPORT:
			message = _("pro_scheduler job dependents report");
			break;
		case DEPENDENTS:
			message = _("pro_scheduler job dependents");
			break;
		case DROPEXCLUDINGDEPS:
			message = wxString::Format(_("Are you sure you wish to drop job \"%s\"?"),
			                           GetFullIdentifier().c_str());
			break;
		case DROPTITLE:
			message = _("Drop job?");
			break;
	}

	if (!message.IsEmpty() && !(kindOfMessage == DROPEXCLUDINGDEPS || kindOfMessage == DROPTITLE))
		message += wxT(" - ") + GetName();

	return message;
}

int pgproJob::GetIconId()
{
	if (GetEnabled())
		if (GetStatus()==wxT("working")) return projobFactory.GetRunId();
			   else 
			return projobFactory.GetIconId();
	else
		return projobFactory.GetDisabledId();
}


wxMenu *pgproJob::GetNewMenu()
{
	wxMenu *menu = pgObject::GetNewMenu();
	if (1) // check priv.
	{
		//stepFactory.AppendMenu(menu);
		//scheduleFactory.AppendMenu(menu);
	}
	return menu;
}
wxString pgproJob::GetSql(ctlTree *browser)
{
	if (sql.IsNull())
	{
		sql = wxT("-- pro_scheduler job: ") + GetName() + wxT("\n\n");
		sql +=  wxT("-- SELECT schedule.set_job_attributes( ") + NumToStr(GetRecId()) + wxT(", { });\n\n");
		sql +=  wxT("SELECT schedule.create_job(")+GetConnection()->qtDbString(GetRule())+wxT(");");
	}
	return sql;
}


bool pgproJob::DropObject(wxFrame *frame, ctlTree *browser, bool cascaded)
{
	return GetConnection()->ExecuteVoid(wxT("select schedule.drop_job(") + NumToStr(GetRecId())+wxT(")"));
}


void pgproJob::ShowTreeDetail(ctlTree *browser, frmMain *form, ctlListView *properties, ctlSQLBox *sqlPane)
{
	if (!expandedKids)
	{
		expandedKids = true;

		browser->RemoveDummyChild(this);

		// Log
		wxLogInfo(wxT("Adding child objects to Job."));

		//browser->AppendCollection(this, scheduleFactory);
		//browser->AppendCollection(this, stepFactory);
	}

	if (properties)
	{
		CreateListColumns(properties);

		properties->AppendItem(_("Name"), GetName());
		properties->AppendItem(_("ID"), GetRecId());
		properties->AppendYesNoItem(_("Enabled"), GetEnabled());
		//properties->AppendItem(_("Host agent"), GetHostAgent());
	//	properties->AppendItem(_("Job class"), GetJobclass());
		properties->AppendItem(_("Started"), GetStarted());
		properties->AppendItem(_("Finished"), GetFinished());
		properties->AppendItem(_("Result"), GetStatus());
		properties->AppendItem(_("Crontab"), GetCrontab());
		//properties->AppendItem(_("Last result"), GetLastresult());
		properties->AppendItem(_("Owner"), GetOwner());
		properties->AppendItem(_("Runas"), GetRunAs());
		properties->AppendItem(_("Message"), GetMessage());
		properties->AppendItem(_("Comment"), firstLineOnly(GetComment()));
		//wxDateTime dt=	GetNextSchedule_At(GetStarted(),-1);
		wxDateTime t = wxDateTime::Now();
		wxDateTime nextdt = GetNextSchedule_At(t, 1);
		properties->AppendItem(_("Next start job"), nextdt);
		
		wxDateTime dt = GetStarted();
		if (!dt.IsValid()) dt = t;
		wxDateTime prev; 
		while (dt<t) {
			prev = dt;
			dt=GetNextSchedule_At(dt, 1);
		}
		//dt = prev;
		wxString str;
		for (int i = 0; i < 100; i++) {

			dt = GetNextSchedule_At(dt, -1);
			if (dt < GetSchedMin()) break;
			if (str.Len() > 0) str += "),(";
			str += "'"+dt.FormatISODate() + wxT(" ") + dt.FormatISOTime()+ "'";
		}
		wxString sql = "with at(dt) as(select to_timestamp(t.dt,'YYY-MM-DD HH24:MI') dt from (values("+str+")) as t(dt))\n";
		sql += "select dt,status from at left join schedule.get_log() b on b.scheduled_at=dt and b.cron="+ NumToStr(GetRecId())+" where b.scheduled_at is null;";
		pgSet* jobs = GetConnection()->ExecuteSet(sql);


		if (jobs)
		{
			wxString str;
			while (!jobs->Eof())
			{

				str+=jobs->GetVal(wxT("dt"))+";";
				jobs->MoveNext();
			}
			delete jobs;
			if (str.Len()>0) properties->AppendItem(_("Previous sched time run skip"), str);
		}
		

	}
}

bool pgproJob::NeedRefresh()
{
	wxDateTime t = wxDateTime::Now();
	if (t >= nextrefresh) {

		wxTimeSpan m(0, 5);

		t.Add(m);
		nextrefresh = t;
		return true;
	}
	return false;
}


pgObject *pgproJob::Refresh(ctlTree *browser, const wxTreeItemId item)
{
	pgObject *job = 0;

	pgObject *obj = browser->GetObject(browser->GetItemParent(item));
	if (obj && obj->IsCollection())
		job = projobFactory.CreateObjects((pgCollection *)obj, 0, wxT("\n   WHERE c.id=") + NumToStr(GetRecId()));
	GetIconId();
	UpdateIcon(browser);
	return job;
}



pgObject *pgproJobFactory::CreateObjects(pgCollection *collection, ctlTree *browser, const wxString &restriction)
{
	pgproJob *job = 0;
	wxString sql=    wxT("with last_job as (select cron,max(started) started,min(scheduled_at) scheduled_at_min from schedule.get_log() b group by cron)")
					  wxT(", a as (select cron,scheduled_at,started,finished,status,message from schedule.get_active_jobs()), lastj as (select b.cron,")
					  wxT("coalesce((select scheduled_at from a where a.cron=b.cron),b.scheduled_at) scheduled_at")
					  wxT(",coalesce((select started from a where a.cron=b.cron),b.started) started")
					  wxT(",case when (select status from a where a.cron=b.cron)='working' then null else b.finished end finished")
					  wxT(",coalesce((select status from a where a.cron=b.cron),b.status) status")
					  wxT(",coalesce((select message from a where a.cron=b.cron),b.message) message")
					  wxT(",scheduled_at_min")
					  wxT(" from schedule.get_log() b,last_job where b.cron=last_job.cron and b.started=last_job.started")
					  wxT(") select j.crontab cron,j.days,j.hours,j.wdays,j.months,j.minutes,* from (select c.*,null stop,l.* from schedule.get_cron() c left join lastj l on c.id=l.cron) c")
					  wxT(" join lateral (select crontab,days,hours,wdays,months,minutes from jsonb_to_record(c.rule) as (days int[],hours int[],wdays int[],months int[],crontab text,minutes int[]) ) j on true")
					  wxT("")
					  wxT("")
	                  wxT(" ") + restriction;

	if (!collection->GetConnection()->IsSuperuser()) {
		size_t a=sql.Replace(wxT("get_log()"),wxT("get_user_log()"));
		a=sql.Replace(wxT("get_cron()"),wxT("get_user_cron()"));
	}

	pgSet *jobs = collection->GetConnection()->ExecuteSet(sql);
	

	if (jobs)
	{
		int nCols = jobs->NumCols();
		while (!jobs->Eof())
		{
			wxString status;
			status=jobs->GetVal(wxT("status"));
			if (status.IsNull()) status = _("Not run");

			///wxString name=jobs->GetVal(wxT("name"))+wxT("(")+jobs->GetVal(wxT("id"))+wxT(")");
			
			job = new pgproJob(jobs->GetVal(wxT("name")));
			job->iSetStatus(status);
			job->iSetOwner(jobs->GetVal(wxT("owner")));
			//job->iSetServer(collection->GetServer());
			pgDatabase* db = collection->GetDatabase();
			if (db == NULL) {
				assert(db == NULL);
			}
			job->iSetDatabase(collection->GetDatabase());
			job->iSetRecId(jobs->GetLong(wxT("id")));
			job->iSetComment(jobs->GetVal(wxT("comments")));
			
			job->iSetEnabled(jobs->GetBool(wxT("active")));

			wxString tmp;
			tmp = jobs->GetVal(wxT("commands"));
			if (tmp.find('{',0)==0) { tmp=wxT("[")+tmp.Mid(1,tmp.Len()-2)+wxT("]");  }
			//tmp.Replace(wxT("{"), wxT("["),false);
			//tmp.Replace(wxT("}"), wxT("]"));
			// days,hours,wdays,months,minutes
			job->iSetSched(job->GetRecId(), jobs->GetVal("minutes"), jobs->GetVal("hours"), jobs->GetVal("days"), jobs->GetVal("wdays"), jobs->GetVal("months"));
			job->iSetSchedMin(jobs->GetDateTime(wxT("scheduled_at_min")));
			job->iSetCommands(tmp);
			job->iSetRunAs(jobs->GetVal(wxT("run_as")));
			job->iSetStarted(jobs->GetDateTime(wxT("started")));
			job->iSetSameTransaction(jobs->GetBool(wxT("use_same_transaction")));
			

			job->iSetFinished(jobs->GetDateTime(wxT("finished")));
			job->iSetCrontab(jobs->GetVal(wxT("crontab")));
			//job->iSetNextrun(jobs->GetDateTime(wxT("jobnextrun")));
			
			
			job->iSetMessage(jobs->GetVal(wxT("message")));
			tmp=wxT("");
			int columnCount=0;
					for (int col = 0 ; col < nCols ; col++)
					{
						wxString colname=jobs->ColName(col);
						wxString vl=jobs->GetVal(col);
						if (vl.IsEmpty()) continue;
						if (colname==wxT("id")
							||colname==wxT("node")
							||colname==wxT("rule")
							||colname == wxT("minutes")
							||colname == wxT("days")
							||colname == wxT("wdays")
							||colname == wxT("hours")
							|| colname == wxT("active")
							||colname == wxT("months")
							||colname == wxT("scheduled_at_min")
							||colname==wxT("owner")) continue;
						
						if (colname==wxT("broken")) break;
						if (columnCount)
						{
							tmp += wxT("\n,");
						}
						if (colname==wxT("commands")) vl="["+vl.Mid(1,vl.Length()-2)+"]";
						else vl=wxT("\"")+vl+wxT("\"");
						
						tmp += wxT("\"")+colname+wxT("\"")+wxT(": ")+vl;
						columnCount++;
					}
					job->iSetRule(wxT("{")+tmp+wxT("}"));
			if (browser)
			{
				browser->AppendObject(collection, job);
				jobs->MoveNext();
			}
			else
				break;
		}

		delete jobs;
	}
	return job;
}

void pgproJob::ShowStatistics(frmMain *form, ctlListView *statistics)
{
	wxString sql =
	    wxT("SELECT jlgid")
	    wxT(", jlgstatus")
	    wxT(", jlgstart")
	    wxT(", jlgduration")
	    wxT(", (jlgstart + jlgduration) AS endtime")
	    wxT("  FROM pgagent.pga_joblog\n")
	    wxT(" WHERE jlgjobid = ") + NumToStr(GetRecId()) +
	    wxT(" ORDER BY jlgstart DESC") +
	    wxT(" LIMIT ") + NumToStr(settings->GetMaxRows());

	if (statistics)
	{
		wxLogInfo(wxT("Displaying statistics for job %s"), GetFullIdentifier().c_str());
		wxString bu=GetConnection()->ExecuteScalar(wxT("select coalesce((select pg_table_is_visible(oid) from pg_class where relname='pg_log'),false)"));
		// Add the statistics view columns
		statistics->ClearAll();
		statistics->AddColumn(_("Log_time"), 95);
		statistics->AddColumn(_("Critical"), 30);
		statistics->AddColumn(_("Message"), 300);
		statistics->AddColumn(_("Stage"), 40);
		if ((bu==wxT("f"))||(!settings->GetASUTPstyle())||(DateToAnsiStr(GetStarted()).IsEmpty())) return ;

		wxString wxDTend = DateToAnsiStr(GetFinished());
		if (wxDTend.IsEmpty()) wxDTend=DateToAnsiStr(wxDateTime::Now());
		sql=wxT("select log_time,detail critical,message,hint from pg_log l where l.log_time>'") + DateToAnsiStr(GetStarted())+
			wxT("'::timestamp - interval '1min' and l.log_time<='")+ wxDTend +
			wxT("'::timestamp + interval '1min' and detail::int>=0");
		//+GetTryName()+wxT("'");
		pgSet *stats = GetConnection()->ExecuteSet(sql);
		wxString critical;
		wxDateTime startTime;
		wxDateTime endTime;

		if (stats)
		{
			while (!stats->Eof())
			{
				startTime.ParseDateTime(stats->GetVal(0));
				//endTime.ParseDateTime(stats->GetVal(4));
				critical=stats->GetVal(1);
				long pos = statistics->AppendItem(DateToStr(startTime), critical, stats->GetVal(2));
				statistics->SetItem(pos, 3, stats->GetVal(3));
				//statistics->SetItem(pos, 4, stats->GetVal(3));

				stats->MoveNext();
			}
			delete stats;
		}
	}
}

bool pgproJob::RunNow()
{
	//if (!GetConnection()->ExecuteVoid(wxT("select schedule.drop_job(") + NumToStr(GetRecId())+wxT(")"))) 	return false;

	return true;
}


pgproJobCollection::pgproJobCollection(pgaFactory *factory, pgDatabase *db)
	: pgDatabaseObjCollection(factory, db)
{
}


wxString pgproJobCollection::GetTranslatedMessage(int kindOfMessage) const
{
	wxString message = wxEmptyString;

	switch (kindOfMessage)
	{
		case RETRIEVINGDETAILS:
			message = _("Retrieving details on pgAgent jobs");
			break;
		case REFRESHINGDETAILS:
			message = _("Refreshing pgAgent jobs");
			break;
		case OBJECTSLISTREPORT:
			message = _("pgAgent jobs list report");
			break;
	}

	return message;
}
void pgproJob::SetEnabled(ctlTree *browser, const bool b)
{
	if (GetRecId() > 0 && ((enabled && !b) || (!enabled && b)))
	{
		wxString sql = wxT("select schedule.");
		if (enabled && !b)
			sql += wxT("deactivate_job(");
		else if (!enabled && b)
			sql += wxT("activate_job(");

		sql += NumToStr(GetRecId())+wxT(")");
		GetConnection()->ExecuteVoid(sql);
	}
	enabled = b;
	UpdateIcon(browser);
}

//dlgProperty *pgproJobFactory::CreateDialog(frmMain *frame, pgObject *node, pgObject *parent)
//{
//	return 0;
//}


/////////////////////////////


#include "images/job.pngc"
#include "images/jobs.pngc"
#include "images/jobdisabled.pngc"
#include "images/jobrun.pngc"

pgproJobFactory::pgproJobFactory()
	: pgDatabaseObjFactory(__("pgpro_Scheduler Job"), __("New Job"), __("Create a new Job."), job_png_img)
{
	metaType = PGM_PROJOB;
	disabledId = addIcon(jobdisabled_png_img);
	runId = addIcon(jobrun_png_img);
	
}


pgCollection *pgproJobFactory::CreateCollection(pgObject *obj)
{
	return new pgproJobCollection(GetCollectionFactory(), (pgDatabase *) obj);

}


pgproJobFactory projobFactory;
static pgaCollectionFactory cf(&projobFactory, __("Jobs"), jobs_png_img);

prorunNowFactory::prorunNowFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar) : contextActionFactory(list)
{
	mnu->Append(id, _("&Run now"), _("Reschedule the job to run now."));
}


wxWindow *prorunNowFactory::StartDialog(frmMain *form, pgObject *obj)
{
	if (!((pgproJob *)(obj))->RunNow())
	{
		wxLogError(_("Failed to reschedule the job."));
	}

	form->Refresh(obj);

	return 0;
}


bool prorunNowFactory::CheckEnable(pgObject *obj)
{
	if (obj)
	{
		if (obj->GetMetaType() == PGM_PROJOB && !obj->IsCollection())
			return true;
	}
	return false;
}
enabledisableJobFactory::enabledisableJobFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar) : contextActionFactory(list)
{
	mnu->Append(id, _("Enabled?"), _("¬ключить или выключить задание."), wxITEM_CHECK);
}

wxWindow *enabledisableJobFactory::StartDialog(frmMain *form, pgObject *obj)
{
	((pgproJob *)obj)->SetEnabled(form->GetBrowser(), !((pgproJob *)obj)->GetEnabled());
	//((pgproJob *)obj)->SetDirty();

	wxTreeItemId item = form->GetBrowser()->GetSelection();
	if (obj == form->GetBrowser()->GetObject(item))
	{
		//form->GetBrowser()->DeleteChildren(item);
		obj->ShowTreeDetail(form->GetBrowser(), 0, form->GetProperties());
		//form->GetSqlPane()->SetReadOnly(false);
		//form->GetSqlPane()->SetText(((pgproJob *)obj)->GetSql(form->GetBrowser()));
		//form->GetSqlPane()->SetReadOnly(true);
	}
	form->GetMenuFactories()->CheckMenu(obj, form->GetMenuBar(), (ctlMenuToolbar *)form->GetToolBar());

	return 0;
}

bool enabledisableJobFactory::CheckEnable(pgObject *obj)
{
	return obj && obj->IsCreatedBy(projobFactory);
}

bool enabledisableJobFactory::CheckChecked(pgObject *obj)
{
	return obj && obj->IsCreatedBy(projobFactory) && ((pgproJob *)obj)->GetEnabled();
}
