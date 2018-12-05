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


class pgproJobFactory : public pgServerObjFactory
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

class pgproJob : public pgServerObject
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
	
	wxString GetTryName() const
	{
		return tryname;
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
		return false;
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
	bool enabled;
	wxDateTime finished, changed, nextrun, lastrun;
	wxString message, crontab, runas, commands,status,rule,tryname;
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


class pgproJobCollection : public pgServerObjCollection
{
public:
	pgproJobCollection(pgaFactory *factory, pgServer *sv);
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
