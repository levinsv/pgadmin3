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

#ifndef PGPUBLICATION_H
#define PGPUBLICATION_H

#include "pgDatabase.h"

class pgCollection;
class pgPublicationFactory : public pgDatabaseObjFactory
{
public:
	pgPublicationFactory();
	virtual dlgProperty *CreateDialog(frmMain *frame, pgObject *node, pgObject *parent);
	virtual pgObject *CreateObjects(pgCollection *obj, ctlTree *browser, const wxString &restr = wxEmptyString);
	virtual pgCollection *CreateCollection(pgObject *obj);
};
extern pgPublicationFactory publicationFactory;

class pgPublication : public pgDatabaseObject
{
public:
	pgPublication(const wxString &newName = wxT(""));

	wxString GetTranslatedMessage(int kindOfMessage) const;
	void ShowTreeDetail(ctlTree *browser, frmMain *form = 0, ctlListView *properties = 0, ctlSQLBox *sqlPane = 0);
	bool CanDropCascaded()
	{
		return true;
	}

	wxString GetTablesStr() const
	{
		return tables;
	}
	void iSetTablesStr(const wxString &s)
	{
		tables = s;
	}
	wxString GetiSchemasList() const
	{
		return schemas;
	}
	void iSetSchemasList(const wxString& s)
	{
		schemas = s;
	}
	wxString GetVersion() const
	{
		return version;
	}
	void iSetVersion(const wxString &s)
	{
		version = s;
	}
	bool GetIsAll() const
	{
		return all;
	}
	void iSetIsAll(const bool b)
	{
		all = b;
	}
	bool GetIsIns() const
	{
		return ins;
	}
	void iSetIsIns(const bool b)
	{
		ins = b;
	}
	bool GetIsUpd() const
	{
		return upd;
	}
	void iSetIsUpd(const bool b)
	{
		upd = b;
	}
	bool GetIsDel() const
	{
		return del;
	}
	bool GetIsViaRoot() const
	{
		return publish_via_partition_root;
	}
	void iSetIsViaRoot(const bool b)
	{
		publish_via_partition_root = b;
	}
	wxString GetTableColumns(const wxString& table) {
		auto it = tablesdetail.find(table);
		wxString v;
		if (it != tablesdetail.end()) {
			v = it->second;
		}
		return v;
	}
	wxString GetTableFilter(const wxString& table) {
		wxString key = " where table filter for " + table;
		return GetTableColumns(key);
	}
	void iAppendTable(const wxString& table, const wxString& listcolumns) {
		auto it = tablesdetail.find(table);
		wxString v;
		if (it != tablesdetail.end()) {
			v = it->second;
			wxASSERT("failure uniq keymap columns");
			wxTrap();
		}
		else {
			tablesdetail.insert({ table,listcolumns });
		}

	}
	void iAppendTableFilter(const wxString& table, const wxString& listcolumns) {
		wxString key = " where table filter for " + table;
		iAppendTable(key,listcolumns);
	}
	wxString GetStrOper() const
	{
		wxString s = wxT("");
		if (GetIsIns()) s += wxT("insert");
		if (GetIsUpd()&&(!s.IsEmpty())) s += wxT(", ");
		if (GetIsUpd()) s += wxT("update");
		if (GetIsDel()&&(!s.IsEmpty())) s += wxT(", ");
		if (GetIsDel()) s += wxT("delete");
		return s;
	}
	void iSetIsDel(const bool b)
	{
		del = b;
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
	wxString tables, schemas, version;
	bool all,ins,upd,del, publish_via_partition_root;
	std::map<wxString, wxString> tablesdetail;
};

class pgPublicationCollection : public pgDatabaseObjCollection
{
public:
	pgPublicationCollection(pgaFactory *factory, pgDatabase *db);
	wxString GetTranslatedMessage(int kindOfMessage) const;
};

#endif
