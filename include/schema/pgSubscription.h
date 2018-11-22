//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// pgExtension.h PostgreSQL Extension
//
//////////////////////////////////////////////////////////////////////////

#ifndef PGSUBSCRIPTION_H
#define PGSUBSCRIPTION_H

#include "pgDatabase.h"

class pgCollection;
class pgSubscriptionFactory : public pgDatabaseObjFactory
{
public:
	pgSubscriptionFactory();
	virtual dlgProperty *CreateDialog(frmMain *frame, pgObject *node, pgObject *parent);
	virtual pgObject *CreateObjects(pgCollection *obj, ctlTree *browser, const wxString &restr = wxEmptyString);
	virtual pgCollection *CreateCollection(pgObject *obj);
};
extern pgSubscriptionFactory subscriptionFactory;

class pgSubscription : public pgDatabaseObject
{
public:
	pgSubscription(const wxString &newName = wxT(""));

	wxString GetTranslatedMessage(int kindOfMessage) const;
	void ShowTreeDetail(ctlTree *browser, frmMain *form = 0, ctlListView *properties = 0, ctlSQLBox *sqlPane = 0);
	bool CanDropCascaded()
	{
		return true;
	}

	wxString GetPubStr() const
	{
		return publications;
	}
	void iSetPubStr(const wxString &s)
	{
		publications = s;
	}
	wxString GetConnInfo() const
	{
		return conninfo;
	}
	void iSetSlotName(const wxString &s) {
		slotname = s;
	}
	wxString GetSlotName() const
	{
		return slotname;
	}
	void iSetConnInfo(const wxString &s)
	{
		conninfo = s;
	}
	bool GetIsEnabled() const
	{
		return enabled;
	}
	void iSetIsEnabled(const bool b)
	{
		enabled = b;
	}
	wxString GetIsSyncCommit() const
	{
		return subsynccommit;
	}
	void iSetIsSyncCommit(const wxString &s)
	{
		subsynccommit = s;
	}
	wxString GetStrOper() const
	{
		wxString s = wxT("");
		s += wxT("enabled = ")+BoolToStr(GetIsEnabled());
		s += wxT(", synchronous_commit = ")+ subsynccommit;
		s += wxT(", slot_name = ");
		if (slotname.IsEmpty()) s += wxT("NONE");
			else s += wxT("'")+slotname+wxT("'");

		//s += wxT(", slot_name = ")+ !slotname.IsEmpty() ? wxT("'")+slotname+wxT("'") : wxT("NONE");
		return s;
	}

	bool DropObject(wxFrame *frame, ctlTree *browser, bool cascaded);
	wxString GetSql(ctlTree *browser);
	pgObject *Refresh(ctlTree *browser, const wxTreeItemId item);

	bool HasStats()
	{
		return false;
	}
	bool HasDepends()
	{
		return false;
	}
	bool HasReferences()
	{
		return false;
	}

private:
	wxString conninfo,publications,slotname,subname,subsynccommit;
	bool enabled;
};

class pgSubscriptionCollection : public pgDatabaseObjCollection
{
public:
	pgSubscriptionCollection(pgaFactory *factory, pgDatabase *db);
	wxString GetTranslatedMessage(int kindOfMessage) const;
};

#endif
