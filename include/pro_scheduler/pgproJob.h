//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// pgaJob.h - PostgreSQL Agent Job
//
//////////////////////////////////////////////////////////////////////////

#ifndef PGPROJOB_H
#define PGPROJOB_H

#include "schema/pgServer.h"


class pgproJobFactory : public pgDatabaseObjFactory
{
public:
	pgproJobFactory();
	virtual dlgProperty *CreateDialog(frmMain *frame, pgObject *node, pgObject *parent);
	virtual pgObject *CreateObjects(pgCollection *obj, ctlTree *browser, const wxString &restr = wxEmptyString);
	virtual pgCollection *CreateCollection(pgObject *obj);
	int GetDisabledId()
	{
		return disabledId;
	}
	int GetRunId()
	{
		return runId;
	}
	
protected:
	int disabledId,runId;
};
extern pgproJobFactory projobFactory;

class pgproJob : public pgDatabaseObject
{
public:
	pgproJob(const wxString &newName = wxT(""));
	void SetEnabled(ctlTree *browser, const bool b);
	int GetIconId();
	void ShowTreeDetail(ctlTree *browser, frmMain *form = 0, ctlListView *properties = 0, ctlSQLBox *sqlPane = 0);
	void ShowStatistics(frmMain *form, ctlListView *statistics);
	pgObject *Refresh(ctlTree *browser, const wxTreeItemId item);
	bool DropObject(wxFrame *frame, ctlTree *browser, bool cascaded);
	wxString GetTranslatedMessage(int kindOfMessage) const;
	wxString GetSql(ctlTree *browser);
	bool NeedRefresh();

	void iSetSched(int cron, wxString &mi, wxString& h, wxString& d, wxString& wd, wxString& mon)
	{
		for (int i = 0; i < 60; i++) _mi[i] = false;
		for (int i = 0; i < 24; i++) _h[i] = false;
		for (int i = 0; i < 31; i++) _d[i] = false;
		for (int i = 0; i < 7; i++) _wd[i] = false;
		for (int i = 0; i < 12; i++) _mon[i] = false;

		wxStringTokenizer confkey(mi.substr(1,mi.Len()-1),",");
		while (confkey.HasMoreTokens())
		{
			wxString str = confkey.GetNextToken();
			long j = atol(str.ToAscii());
			_mi[j] = true;
		}
		confkey.SetString(h.substr(1, h.Len() - 1),",");
		while (confkey.HasMoreTokens())
		{
			wxString str = confkey.GetNextToken();
			long j = atol(str.ToAscii());
			_h[j] = true;
		}
		confkey.SetString(wd.substr(1, wd.Len() - 1), ",");
		while (confkey.HasMoreTokens())
		{
			wxString str = confkey.GetNextToken();
			long j = atol(str.ToAscii());
			_wd[j] = true;
		}
		confkey.SetString(d.substr(1, d.Len() - 1), ",");
		while (confkey.HasMoreTokens())
		{
			wxString str = confkey.GetNextToken();
			long j = atol(str.ToAscii())-1;
			_d[j] = true;
		}
		confkey.SetString(mon.substr(1, mon.Len() - 1), ",");
		while (confkey.HasMoreTokens())
		{
			wxString str = confkey.GetNextToken();
			long j = atol(str.ToAscii())-1;
			_mon[j] = true;
		}
	}
	// получить индекс следующего элемента или -1 если его уже нет.
	int getnext(bool array[],int len,int pos, int direct) {
		//int len = sizeof(array) / sizeof(array[0]);
		int mi = pos;
		do {
			mi = mi + direct;
			if (mi > -1 && mi < len) {

			}
			else {
				if (direct == 1 && mi == len) mi = 0;
				if (direct == -1 && mi < 0) mi = len - 1;
				mi = -1;
				break;
			}
		} while (!array[mi]);

		// возвращает -1 если нет слудующего элемента.
		return mi;
	}
	// получить индекс первый/последний элемента
	int getfirst(bool array[], int len, int direct) {
		int mi = -1;
		if (direct == -1) mi = len ;
		do {
			mi = mi + direct;
			if (mi > -1 && mi < len) {


			}
			else {
				if (direct == 1 && mi == len) mi = 0;
				if (direct == -1 && mi < 0) mi = len - 1;
				mi = -1;
				break;
			}
		} while (!array[mi]);
		return mi;
	}

	wxDateTime GetNextSchedule_At(wxDateTime prev_at, int dr) {
		int mi = prev_at.GetMinute();
		int h = prev_at.GetHour();
		int d = prev_at.GetDay()-1;
		int wd=prev_at.GetWeekDay();
		int mon = prev_at.GetMonth();
		int y = prev_at.GetYear();
		bool novalid = false;

		int nextp = getnext(_mi, sizeof(_mi) / sizeof(_mi[0]),mi, dr);
		if (nextp == -1 || !_wd[wd]) {
			mi = getfirst(_mi, sizeof(_mi) / sizeof(_mi[0]), dr);
			nextp = getnext(_h, sizeof(_h) / sizeof(_h[0]), h, dr);
			if (nextp == -1 || !_wd[wd]) {
				// hours
				h = getfirst(_h, sizeof(_h) / sizeof(_h[0]), dr);
				do {
					nextp = getnext(_d, sizeof(_d) / sizeof(_d[0]), d, dr);
					if (nextp == -1) {
						// days
						d = getfirst(_d, sizeof(_d) / sizeof(_d[0]), dr);
						nextp = getnext(_mon, sizeof(_mon) / sizeof(_mon[0]), mon, dr);
						if (nextp == -1) {
							// mon
							mon = getfirst(_mon, sizeof(_mon) / sizeof(_mon[0]), dr);
							y = y + dr;
						}
						else mon = nextp;

					}
					else d = nextp;
					// day next 
					// проверим соответствие wdays
					wxDateTime tmp((wxDateTime::wxDateTime_t) d + 1, (wxDateTime::Month) mon, y, (wxDateTime::wxDateTime_t)h, (wxDateTime::wxDateTime_t)mi);
					novalid = !tmp.IsValid();
					wd = tmp.GetWeekDay();
					//if (nextp == -1) nextp=getfirst(_wd, dr);
				} while (!_wd[wd] || novalid);

			}
			else h = nextp;
		}
		else mi = nextp;
		//
		wxDateTime tmp((wxDateTime::wxDateTime_t) d+1, (wxDateTime::Month) mon, y, (wxDateTime::wxDateTime_t)h, (wxDateTime::wxDateTime_t)mi);
		return tmp;
	}
	wxDateTime GetSchedMin() const
	{
		return sched_min;
	}
	void iSetSchedMin(const wxDateTime& d)
	{
		sched_min = d;
	}
	wxString GetCrontab() const
	{
		return crontab;
	}
	void iSetCrontab(const wxString &s)
	{
		crontab = s;
	}
	bool GetEnabled() const
	{
		return enabled;
	}
	void iSetEnabled(const bool b)
	{
		enabled = b;
	}
	wxDateTime GetFinished() const
	{
		return finished;
	}
	void iSetFinished(const wxDateTime &d)
	{
		finished = d;
	}
	wxDateTime GetChanged() const
	{
		return changed;
	}
	void iSetChanged(const wxDateTime &d)
	{
		changed = d;
	}
	wxDateTime GetNextrun() const
	{
		return nextrun;
	}
	void iSetNextrun(const wxDateTime &d)
	{
		nextrun = d;
	}
	wxDateTime GetStarted() const
	{
		return lastrun;
	}
	void iSetStarted(const wxDateTime &d)
	{
		lastrun = d;
	}
	wxString GetMessage() const
	{
		return message;
	}
	void iSetMessage(const wxString &s)
	{
		message = s;
	}
	wxString GetStatus() const
	{
		return status;
	}
	void iSetStatus(const wxString &s)
	{
		status = s;
	}
	wxString GetRunAs() const
	{
		return runas;
	}
	void iSetRunAs(const wxString &s)
	{
		runas = s;
	}
	void iSetSameTransaction(bool b)
	{
		use_same_transaction = b;
	}
	bool GetSameTransaction()
	{
		return use_same_transaction;
	}

	wxString GetTryName() const
	{
		return tryname;
	}
	wxString GetFullName()
	{
		return GetName() + wxT("(") + NumToStr(recId) + wxT(")");
	}

	void iSetTryName(const wxString &s)
	{
		tryname = s;
	}

	wxString GetRule() const
	{
		return rule;
	}
	void iSetRule(const wxString &s)
	{
		rule = s;
	}
	wxString GetCommands() const
	{
		return commands;
	}
	void iSetCommands(const wxString &s)
	{
		commands = s;
	}
	long GetRecId() const
	{
		return recId;
	}
	void iSetRecId(const long l)
	{
		recId = l;
	}
	bool RunNow();

	wxMenu *GetNewMenu();
	bool CanCreate()
	{
		return false;
	}
	bool CanView()
	{
		return true;
	}
	bool CanEdit()
	{
		return true;
	}
	bool CanDrop()
	{
		return true;
	}
	bool WantDummyChild()
	{
		return false;
	}

	wxString GetHelpPage(bool forCreate) const
	{
		return wxT("pgagent-jobs");
	}
	
private:
	bool enabled, use_same_transaction;
	wxDateTime finished, changed, nextrun, lastrun,sched_min;
	wxDateTime nextrefresh;
	wxString message, crontab, runas, commands,status,rule,tryname;
	bool _d[31], _h[24], _mi[60], _wd[7], _mon[12];
	long recId;
};


class pgproJobObject : public pgServerObject
{
public:
	pgproJobObject(pgproJob *job, pgaFactory &factory, const wxString &newName);
	virtual pgproJob *GetJob()
	{
		return job;
	}

	bool CanCreate()
	{
		return job->CanCreate();
	}
	bool CanView()
	{
		return false;
	}
	bool CanEdit()
	{
		return job->CanEdit();
	}
	bool CanDrop()
	{
		return job->CanDrop();
	}

protected:
	pgproJob *job;
};


class pgproJobCollection : public pgDatabaseObjCollection
{
public:
	pgproJobCollection(pgaFactory *factory, pgDatabase *db);
	wxString GetTranslatedMessage(int kindOfMessage) const;
};

class enabledisableJobFactory : public contextActionFactory
{
public:
	enabledisableJobFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar);
	wxWindow *StartDialog(frmMain *form, pgObject *obj);
	bool CheckEnable(pgObject *obj);
	bool CheckChecked(pgObject *obj);
};


class prorunNowFactory : public contextActionFactory
{
public:
	prorunNowFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar);
	wxWindow *StartDialog(frmMain *form, pgObject *obj);
	bool CheckEnable(pgObject *obj);
};

#endif
