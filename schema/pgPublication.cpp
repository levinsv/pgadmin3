//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// pgPublication.cpp - Publication class
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "utils/misc.h"
#include "schema/pgPublication.h"


pgPublication::pgPublication(const wxString &newName)
	: pgDatabaseObject(publicationFactory, newName)
{
}

wxString pgPublication::GetTranslatedMessage(int kindOfMessage) const
{
	wxString message = wxEmptyString;

	switch (kindOfMessage)
	{
		case RETRIEVINGDETAILS:
			message = _("Retrieving details on publication");
			message += wxT(" ") + GetName();
			break;
		case REFRESHINGDETAILS:
			message = _("Refreshing publication");
			message += wxT(" ") + GetName();
			break;
		case DROPINCLUDINGDEPS:
			message = wxString::Format(_("Are you sure you wish to drop publication \"%s\" including all objects that depend on it?"),
			                           GetFullIdentifier().c_str());
			break;
		case DROPEXCLUDINGDEPS:
			message = wxString::Format(_("Are you sure you wish to drop publication \"%s\"?"),
			                           GetFullIdentifier().c_str());
			break;
		case DROPCASCADETITLE:
			message = _("Drop publication cascaded?");
			break;
		case DROPTITLE:
			message = _("Drop publication?");
			break;
		case PROPERTIESREPORT:
			message = _("Publication properties report");
			message += wxT(" - ") + GetName();
			break;
		case PROPERTIES:
			message = _("Publication properties");
			break;
		case DDLREPORT:
			message = _("Publication DDL report");
			message += wxT(" - ") + GetName();
			break;
		case DDL:
			message = _("Publication DDL");
			break;
		case DEPENDENCIESREPORT:
			message = _("Publication dependencies report");
			message += wxT(" - ") + GetName();
			break;
		case DEPENDENCIES:
			message = _("Publication dependencies");
			break;
		case DEPENDENTSREPORT:
			message = _("Publication dependents report");
			message += wxT(" - ") + GetName();
			break;
		case DEPENDENTS:
			message = _("Publication dependents");
			break;
	}

	return message;
}

bool pgPublication::DropObject(wxFrame *frame, ctlTree *browser, bool cascaded)
{
	wxString sql = wxT("DROP PUBLICATION ") + GetQuotedIdentifier();
	if (cascaded)
		sql += wxT(" CASCADE");
	return GetDatabase()->ExecuteVoid(sql);
}


wxString pgPublication::GetSql(ctlTree *browser)
{
	if (sql.IsNull())
	{
		sql = wxT("-- Publication: ") + GetQuotedIdentifier() + wxT("\n\n")
		      + wxT("-- DROP PUBLICATION ") + GetQuotedIdentifier() + wxT(";")
		      + wxT("\n\n CREATE PUBLICATION ") + GetName();

		if (GetIsAll())
		        {
			sql += wxT("\n  FOR ALL TABLES ");
			} else
			{
                        sql += wxT("\n  FOR TABLE ");
			 if (!GetTablesStr().IsEmpty())
			     sql += wxT("") + GetTablesStr();
			}
	        if (GetIsIns()&&GetIsUpd()&&GetIsDel()&&!GetIsViaRoot()) 
			{

			}
			else
			{
				sql += wxT("\n  WITH (");
				wxString opts = wxEmptyString;
				if (!(GetIsIns() && GetIsUpd() && GetIsDel())) opts =opts+ "publish = '" + GetStrOper() + wxT("'");
				if (GetIsViaRoot()) {
					if (!opts.IsEmpty()) opts += ",";
					opts += "publish_via_partition_root = on";
				}
				if (!opts.IsEmpty()) opts += ")";
				sql += opts;
			}

		sql += wxT(";\n");
	}
	return sql;
}


void pgPublication::ShowTreeDetail(ctlTree *browser, frmMain *form, ctlListView *properties, ctlSQLBox *sqlPane)
{
	if (properties)
	{
		CreateListColumns(properties);

		properties->AppendItem(_("Name"), GetName());
		properties->AppendItem(_("OID"), GetOid());
		properties->AppendItem(_("Owner"), GetOwner());
		properties->AppendItem(_("Tables"), GetTablesStr());
		//properties->AppendYesNoItem(_("Relocatable?"), GetIsRelocatable());
		//properties->AppendItem(_("Version"), GetVersion());
		properties->AppendItem(_("Comment"), firstLineOnly(GetComment()));
	}
}



pgObject *pgPublication::Refresh(ctlTree *browser, const wxTreeItemId item)
{
	pgObject *language = 0;
	pgCollection *coll = browser->GetParentCollection(item);
	if (coll)
		language = publicationFactory.CreateObjects(coll, 0, wxT("\n   WHERE x.oid=") + GetOidStr());

	return language;
}



pgObject *pgPublicationFactory::CreateObjects(pgCollection *collection, ctlTree *browser, const wxString &restriction)
{
	wxString sql;
	pgPublication *publication = 0;
	wxString pg13="";
	if (collection->GetDatabase()->BackendMinimumVersion(13, 0)) {
		pg13 = ", pubviaroot";
	}

	sql = wxT("select x.oid,x.pubname, pg_get_userbyid(x.pubowner) AS owner, x.puballtables, x.pubinsert, x.pubupdate, x.pubdelete, obj_description(x.oid,'pg_publication') AS comment, t.t")
		  + pg13 + wxT("\n")
	      wxT("  FROM pg_publication x\n")
	      wxT("  LEFT JOIN LATERAL (select string_agg(schemaname||'.'||tablename,', ') t from pg_publication_tables  where pubname=x.pubname) t on true \n")
	      wxT("  ")
	      + restriction + wxT("\n")
	      wxT(" ORDER BY x.pubname");
	pgSet *publications = collection->GetDatabase()->ExecuteSet(sql);

	if (publications)
	{
		while (!publications->Eof())
		{

			publication = new pgPublication(publications->GetVal(wxT("pubname")));
			publication->iSetDatabase(collection->GetDatabase());
			publication->iSetOid(publications->GetOid(wxT("oid")));
			publication->iSetOwner(publications->GetVal(wxT("owner")));
			publication->iSetTablesStr(publications->GetVal(wxT("t")));
			publication->iSetIsAll(publications->GetBool(wxT("puballtables")));
			publication->iSetIsIns(publications->GetBool(wxT("pubinsert")));
			publication->iSetIsUpd(publications->GetBool(wxT("pubupdate")));
			publication->iSetIsDel(publications->GetBool(wxT("pubdelete")));
			//publication->iSetVersion(publications->GetVal(wxT("extversion")));
			publication->iSetComment(publications->GetVal(wxT("comment")));
			if (!pg13.IsEmpty()) publication->iSetIsViaRoot(publications->GetBool(wxT("pubviaroot")));

			if (browser)
			{
				browser->AppendObject(collection, publication);

				publications->MoveNext();
			}
			else
				break;
		}

		delete publications;
	}
	return publication;
}


/////////////////////////////

pgPublicationCollection::pgPublicationCollection(pgaFactory *factory, pgDatabase *db)
	: pgDatabaseObjCollection(factory, db)
{
}


wxString pgPublicationCollection::GetTranslatedMessage(int kindOfMessage) const
{
	wxString message = wxEmptyString;

	switch (kindOfMessage)
	{
		case RETRIEVINGDETAILS:
			message = _("Retrieving details on publications");
			break;
		case REFRESHINGDETAILS:
			message = _("Refreshing publications");
			break;
		case OBJECTSLISTREPORT:
			message = _("Publications list report");
			break;
	}

	return message;
}

///////////////////////////////////////////////////

#include "images/extension.pngc"
#include "images/extension-sm.pngc"
#include "images/extensions.pngc"

pgPublicationFactory::pgPublicationFactory()
	: pgDatabaseObjFactory(__("Publication"), __("New Publication..."), __("Create a new Publication."), extension_png_img, extension_sm_png_img)
{
}


pgCollection *pgPublicationFactory::CreateCollection(pgObject *obj)
{
	return new pgPublicationCollection(GetCollectionFactory(), (pgDatabase *)obj);
}
dlgProperty *pgPublicationFactory::CreateDialog(frmMain *frame, pgObject *node, pgObject *parent)
{
	return NULL;
}

pgPublicationFactory publicationFactory;
static pgaCollectionFactory cf(&publicationFactory, __("Publications"), extensions_png_img);
