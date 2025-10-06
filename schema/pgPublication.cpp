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

		if (GetIsAll()) {
						sql += wxT("\n  FOR ALL TABLES");
			}
		else
			{
                        sql += wxT("\n  FOR ");
						wxString acc;;
						wxString listtab= GetTablesStr();
						if (!listtab.IsEmpty())
						{
							acc = "TABLE ";
							wxArrayString atabs=wxSplit(listtab, ',');
							for (int i = 0; i < atabs.Count(); i++) {
								wxString fullname = atabs[i];
								if (i > 0 ) acc += "\n           ,";
								acc += fullname;
								wxString cols = GetTableColumns(fullname);
								if (!cols.IsEmpty()) acc = acc + "(" + cols + ")";
								wxString filter = GetTableFilter(fullname);
								if (!filter.IsEmpty()) acc = acc + " WHERE " + filter + "";
							}
							//acc += "\n            ";
						}
						wxString slist = GetiSchemasList();
						if (!slist.IsEmpty()) {
							if (acc.Length() > 0) acc += "\n      ,";
							acc += "TABLES IN SCHEMA "+slist;

						}
			     sql += wxT("") + acc;
			}
		if (GetIsIns()&&GetIsUpd()&&GetIsDel()&&!GetIsViaRoot())
			{

			}
		else
			{
				sql += wxT("\n  WITH (");
				wxString opts = wxEmptyString;
				if (!(GetIsIns() && GetIsUpd() && GetIsDel())) opts =opts+ "publish = '" + GetStrOper() + wxT("'");
				if (GetIsViaRoot() && (GetDatabase()->BackendMinimumVersion(13, 0))) {
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
		language = publicationFactory.CreateObjects(coll, 0, wxT("\n   WHERE oid=") + GetOidStr());

	return language;
}



pgObject *pgPublicationFactory::CreateObjects(pgCollection *collection, ctlTree *browser, const wxString &restriction)
{
	wxString sql;
	pgPublication *publication = 0;
	if (collection->GetDatabase()->BackendMinimumVersion(15, 0)) {
		sql = R"(with rel as (
SELECT pr.prpubid,quote_ident(n.nspname)|| '.'||quote_ident(c.relname) fulltable, pg_get_expr(pr.prqual, c.oid) rowfilter, (CASE WHEN pr.prattrs IS NOT NULL THEN
     pg_catalog.array_to_string(      ARRAY(SELECT attname
              FROM
                pg_catalog.generate_series(0, pg_catalog.array_upper(pr.prattrs::pg_catalog.int2[], 1)) s,
                pg_catalog.pg_attribute
        WHERE attrelid = c.oid AND attnum = prattrs[s]), ', ')
       ELSE NULL END) cols
FROM pg_catalog.pg_class c JOIN pg_catalog.pg_namespace n ON c.relnamespace = n.oid 
     JOIN pg_catalog.pg_publication_rel pr ON c.oid = pr.prrelid 
), shs as (
select pn.pnpubid,string_agg(pn.pnnspid::regnamespace::text,',') slist from  pg_catalog.pg_publication_namespace pn group by pn.pnpubid
)
select p.oid,p.pubname,pt.fulltable,pg_get_userbyid(p.pubowner) AS owner, p.puballtables, p.pubinsert, p.pubupdate, p.pubdelete, obj_description(p.oid,'pg_publication') AS comment,
p.pubviaroot,pt.rowfilter,pt.cols,pn.slist from pg_publication p 
              LEFT JOIN rel pt ON pt.prpubid = p.oid
              LEFT JOIN shs pn ON p.oid = pn.pnpubid
)";
	}
	else {
		wxString pg13="false pubviaroot";
		if (collection->GetDatabase()->BackendMinimumVersion(13, 0)) {
			pg13="p.pubviaroot";
		}
		sql = R"(with rel as (
SELECT pr.prpubid,quote_ident(n.nspname)|| '.'||quote_ident(c.relname) fulltable, ''::text rowfilter, (CASE WHEN pr.prattrs IS NOT NULL THEN
     pg_catalog.array_to_string(      ARRAY(SELECT attname
              FROM
                pg_catalog.generate_series(0, pg_catalog.array_upper(pr.prattrs::pg_catalog.int2[], 1)) s,
                pg_catalog.pg_attribute
        WHERE attrelid = c.oid AND attnum = prattrs[s]), ', ')
       ELSE NULL END) cols
FROM pg_catalog.pg_class c JOIN pg_catalog.pg_namespace n ON c.relnamespace = n.oid 
     JOIN (select oid, prpubid, prrelid, ''::text prqual, null::int2vector prattrs from  pg_catalog.pg_publication_rel pr ) pr ON c.oid = pr.prrelid 
), shs as (
select 0 pnpubid,''::text slist 
)
select p.oid,p.pubname,pt.fulltable,pg_get_userbyid(p.pubowner) AS owner, p.puballtables, p.pubinsert, p.pubupdate, p.pubdelete, obj_description(p.oid,'pg_publication') AS comment,
)"+pg13+R"(,pt.rowfilter,pt.cols,pn.slist from pg_publication p 
              LEFT JOIN rel pt ON pt.prpubid = p.oid
              LEFT JOIN shs pn ON p.oid = pn.pnpubid
)";
	}
	sql = sql + restriction + wxT("\n")
	      wxT(" ORDER BY p.pubname,pt.fulltable");
	pgSet *publications = collection->GetDatabase()->ExecuteSet(sql);

	if (publications)
	{
		wxString prevpubname;
		wxString currpubname;
		wxString listtables;
		while (!publications->Eof())
		{
			currpubname = publications->GetVal(wxT("pubname"));
			if (currpubname != prevpubname) {
				if (browser && publication && currpubname != prevpubname) browser->AppendObject(collection, publication);
				prevpubname = currpubname;
				if (publication && !listtables.IsEmpty() ) publication->iSetTablesStr(listtables);
				listtables = "";
				publication = NULL;

			}

			if (publication == NULL) {
				// first rows this publuc
				publication = new pgPublication(currpubname);
				//if (prevpubname.IsEmpty()) prevpubname = currpubname;
				publication->iSetDatabase(collection->GetDatabase());
				publication->iSetOid(publications->GetOid(wxT("oid")));
				publication->iSetOwner(publications->GetVal(wxT("owner")));
				publication->iSetIsAll(publications->GetBool(wxT("puballtables")));
				publication->iSetIsIns(publications->GetBool(wxT("pubinsert")));
				publication->iSetIsUpd(publications->GetBool(wxT("pubupdate")));
				publication->iSetIsDel(publications->GetBool(wxT("pubdelete")));
				//publication->iSetVersion(publications->GetVal(wxT("extversion")));
				publication->iSetComment(publications->GetVal(wxT("comment")));
				publication->iSetIsViaRoot(publications->GetBool(wxT("pubviaroot")));

				publication->iSetSchemasList(publications->GetVal(wxT("slist")));
			}
			wxString attnames = publications->GetVal("cols");
			wxString rowfilter = publications->GetVal("rowfilter");
			wxString fulltable = publications->GetVal(wxT("fulltable"));
			if (!fulltable.IsEmpty()) {
				if (!listtables.IsEmpty()) listtables += ",";
				listtables += fulltable;
				publication->iAppendTable(fulltable, attnames);
				publication->iAppendTableFilter(fulltable, rowfilter);
			}
			publications->MoveNext();
			if (publications->Eof()) {
				prevpubname = "";
				publication->iSetTablesStr(listtables);
			}
			if (browser)
			{
				if (currpubname != prevpubname)
					browser->AppendObject(collection, publication);
			}
			else
				if ( currpubname != prevpubname) break;
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
