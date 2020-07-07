//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// pgPartition.h Greenplum Partitioned Table Partition
//
//////////////////////////////////////////////////////////////////////////

#ifndef pgPartition_H
#define pgPartition_H

#include "pgSchema.h"
#include "pgTable.h"



class pgPartitionFactory : public pgTableObjFactory
{
public:
	pgPartitionFactory();
	virtual dlgProperty *CreateDialog(frmMain *frame, pgObject *node, pgObject *parent) ;
	virtual pgObject *CreateObjects(pgCollection *obj, ctlTree *browser, const wxString &restr = wxEmptyString);
	virtual pgCollection *CreateCollection(pgObject *obj);
	virtual void AppendMenu(wxMenu *menu);
};

extern pgPartitionFactory pg_partitionFactory;

class pgPartition : public pgTable
{
public:
	pgPartition(pgSchema *newSchema, const wxString &newName = wxT(""));
	~pgPartition();
	bool CanCreate();
	wxMenu *GetNewMenu();
	wxString GetSql(ctlTree *browser);
	pgObject *Refresh(ctlTree *browser, const wxTreeItemId item);
	wxString GetPartitionName()
	{
		return partitionname;
	}
	void iSetPartitionName(const wxString &pn)
	{
		partitionname = pn;
	}

private:
	wxString partitionname;
};


class pgPartitionObject : public pgTableObject
{
public:
	pgPartitionObject(pgPartition *newTable, pgaFactory &factory, const wxString &newName = wxT(""))
		: pgTableObject(newTable, factory, newName) { };
	virtual pgPartition *GetTable() const
	{
		return dynamic_cast<pgPartition *>(table);
	}
	OID GetTableOid() const
	{
		return table->GetOid();
	}
	wxString GetTableOidStr() const
	{
		return NumToStr(table->GetOid()) + wxT("::oid");
	}
};


class pgPartitionCollection : public pgTableCollection
{
public:
	pgPartitionCollection(pgaFactory *factory, pgPartition *_table);
	void ShowStatistics(frmMain *form, ctlListView *statistics);

	virtual bool CanCreate()
	{
		return false;
	};
};


class pgPartitionObjCollection : public pgTableObjCollection
{
public:
	pgPartitionObjCollection(pgaFactory *factory, pgPartition *_table)
		: pgTableObjCollection(factory, (pgTable *)_table ) { };
	virtual pgTable *GetTable() const
	{
		return table;
	}
	virtual bool CanCreate();
};



#endif
