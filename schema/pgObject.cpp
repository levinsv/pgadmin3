//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// pgObject.cpp - PostgreSQL object base class
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "utils/misc.h"
#include "schema/pgObject.h"
#include "schema/pgServer.h"
#include "frm/frmMain.h"
#include "frm/frmReport.h"
#include "schema/pgDomain.h"
#include "schema/pgAggregate.h"
#include "schema/pgSequence.h"
#include "schema/pgFunction.h"
#include "schema/pgType.h"
#include "schema/pgDatabase.h"
#include "schema/pgTable.h"
#include "schema/pgColumn.h"
#include "schema/pgView.h"
#include "schema/pgType.h"
#include "schema/pgOperator.h"
#include "schema/pgLanguage.h"
#include "schema/pgConversion.h"
#include "schema/pgTablespace.h"
#include "schema/pgGroup.h"
#include "schema/pgUser.h"
#include "schema/pgUserMapping.h"
#include "schema/pgIndex.h"
#include "schema/pgTrigger.h"
#include "schema/pgCheck.h"
#include "schema/pgIndexConstraint.h"
#include "schema/pgForeignKey.h"
#include "schema/pgForeignDataWrapper.h"
#include "schema/pgForeignServer.h"
#include "schema/pgForeignTable.h"
#include "schema/pgRule.h"
#include "schema/pgRole.h"
#include "schema/pgCast.h"
#include "schema/pgCatalogObject.h"
#include "schema/pgTextSearchConfiguration.h"
#include "schema/pgTextSearchDictionary.h"
#include "schema/pgTextSearchParser.h"
#include "schema/pgTextSearchTemplate.h"
#include "schema/pgOperatorClass.h"
#include "schema/pgOperatorFamily.h"
#include "schema/pgSchema.h"
#include "schema/pgIndexConstraint.h"
#include "schema/pgExtension.h"
#include "schema/edbPackage.h"
#include "schema/edbSynonym.h"
#include "schema/pgCollation.h"
#include "utils/pgDefs.h"
#include "schema/gpExtTable.h"
#include "schema/gpResQueue.h"
#include "agent/pgaJob.h"
#include "agent/pgaSchedule.h"
#include "agent/pgaStep.h"
#include "schema/pgPartition.h"
#include "utils/pgfeatures.h"


int pgObject::GetType() const
{
	if (factory)
		return factory->GetId();
	return type;
}


int pgObject::GetMetaType() const
{
	if (factory)
		return factory->GetMetaType();
	return PGM_UNKNOWN;
}


wxString pgObject::GetTypeName() const
{
	return factory->GetTypeName();
}


wxString pgObject::GetTranslatedTypeName() const
{
	return wxString(wxGetTranslation(GetTypeName()));
}


wxString pgObject::GetTranslatedMessage(int kindOfMessage) const
{
	wxString message = wxEmptyString;

	switch (kindOfMessage)
	{
		case RETRIEVINGDETAILS:
			message = _("Retrieving details on unknown object of type");
			break;
		case REFRESHINGDETAILS:
			message = _("Refreshing unknown object of type");
			break;
		case BACKUPGLOBALS:
			message = _("Backup globals of unknown object of type");
			break;
		case BACKUPSERVERTITLE:
			message = _("Backup unknown object of type");
			break;
		case DROPEXCLUDINGDEPS:
			message = wxString::Format(_("Are you sure you wish to drop object \"%s\"?"),
			                           GetFullIdentifier().c_str());
			break;
		case DROPTITLE:
			message = _("Drop object?");
			break;
		case BACKUPTITLE:
			message = wxString::Format(_("Backup \"%s\""),
			                           GetFullIdentifier().c_str());
			break;
		case RESTORETITLE:
			message = wxString::Format(_("Restore \"%s\""),
			                           GetFullIdentifier().c_str());
			break;
	}
	//message += wxT(" ") + factory->GetTypeName();

	return message;
}


int pgObject::GetIconId()
{
	int id = -1;
	if (factory)
		id = factory->GetIconId();

	wxASSERT(id != -1);
	return id;
}


int pgObject::GetTypeId(const wxString &typname)
{
	pgaFactory *factory = pgaFactory::GetFactory(typname);
	if (factory)
		return factory->GetId();

	return -1;
}


pgObject::pgObject(pgaFactory &_factory, const wxString &newName)
	: wxTreeItemData(), oid(0)
{
	factory = &_factory;

	if (factory->IsCollection() && newName.IsEmpty())
		name = factory->GetTypeName();
	else
		name = newName;

	type = factory->GetId();
	expandedKids = false;
	needReread = false;
	hintShown = false;
	dlg = NULL;
}


pgObject::pgObject(int newType, const wxString &newName)
	: wxTreeItemData(), oid(0)
{
	factory = pgaFactory::GetFactory(newType);

	// Set the typename and type
	type = newType;

	if (newName.IsEmpty())
		name = factory->GetTypeName();
	else
		name = newName;
	expandedKids = false;
	needReread = false;
	hintShown = false;
	dlg = NULL;
}


void pgObject::AppendMenu(wxMenu *menu, int type)
{
	if (menu)
		factory->AppendMenu(menu);
}


wxString pgObject::GetHelpPage(bool forCreate) const
{
	wxString page;

	if (!IsCollection())
		page = wxT("pg/sql-create") + GetTypeName().Lower();

	return page;
}


wxMenu *pgObject::GetNewMenu()
{
	wxMenu *menu = new wxMenu();
	if (CanCreate())
		AppendMenu(menu);
	return menu;
}

void pgObject::ShowStatistics(frmMain *form, ctlListView *statistics)
{
}

void pgObject::ShowStatisticsTables(frmMain* form, ctlListView* statistics, pgObject *obj)
{
	wxLogInfo(wxT("Displaying statistics for tables on %s"), obj->GetSchema()->GetIdentifier().c_str());
	bool tabcoll = false;
	bool partcoll = false;
	bool onetable = false;
	if (IsCollection()) {
		wxString t = obj->GetFactory()->GetTypeName();
		if (t == ("Tables")
			//obj->IsCreatedBy(tableFactory)
			) tabcoll = true;
		if ( t == ("Partitions")) partcoll = true;
	}
	else if (obj->GetConnection()->BackendMinimumVersion(12, 0)) onetable = true;
	bool hasSize = obj->GetConnection()->HasFeature(FEATURE_SIZE);

	// Add the statistics view columns
	statistics->ClearAll();
	statistics->AddColumn(_("Table Name"));
	if (hasSize)
		statistics->AddColumn(_("Size"), wxLIST_AUTOSIZE);
	if (obj->GetConnection()->GetIsPgProEnt()) statistics->AddColumn(_("CFS %"), wxLIST_AUTOSIZE);
	statistics->AddColumn(_("Live tuples"));
	statistics->AddColumn(_("Tuples inserted"));
	statistics->AddColumn(_("Tuples updated"));
	statistics->AddColumn(_("Tuples deleted"));
	if (obj->GetConnection()->BackendMinimumVersion(8, 3))
	{
		statistics->AddColumn(_("Tuples HOT updated"));
		statistics->AddColumn(_("Dead tuples"));
	}
	if (obj->GetConnection()->BackendMinimumVersion(8, 2))
	{
		statistics->AddColumn(_("Last vacuum"));
		statistics->AddColumn(_("Last autovacuum"));
		statistics->AddColumn(_("Last analyze"));
		statistics->AddColumn(_("Last autoanalyze"));
	}
	if (obj->GetConnection()->BackendMinimumVersion(9, 1))
	{
		statistics->AddColumn(_("Vacuum counter"));
		statistics->AddColumn(_("Autovacuum counter"));
		statistics->AddColumn(_("Analyze counter"));
		statistics->AddColumn(_("Autoanalyze counter"));
	}

	wxString sql = wxT("SELECT st.relname, n_tup_ins, n_tup_upd, n_tup_del");
	if (obj->GetConnection()->BackendMinimumVersion(8, 3))
		sql += wxT(", n_tup_hot_upd, n_live_tup, n_dead_tup");
	if (obj->GetConnection()->BackendMinimumVersion(8, 2))
		sql += wxT(", last_vacuum, last_autovacuum, last_analyze, last_autoanalyze");
	if (obj->GetConnection()->BackendMinimumVersion(9, 1))
		sql += wxT(", vacuum_count, autovacuum_count, analyze_count, autoanalyze_count");
	if (hasSize)
		sql += wxT(", pg_size_pretty(pg_relation_size(st.relid)")
		wxT(" + CASE WHEN cl.reltoastrelid = 0 THEN 0 ELSE pg_relation_size(cl.reltoastrelid) + COALESCE((SELECT SUM(pg_relation_size(indexrelid)) FROM pg_index WHERE indrelid=cl.reltoastrelid)::int8, 0) END")
		wxT(" + COALESCE((SELECT SUM(pg_relation_size(indexrelid)) FROM pg_index WHERE indrelid=st.relid)::int8, 0)) AS size");
	if (obj->GetConnection()->GetIsPgProEnt()) sql += wxT(",left((cfs_fragmentation(cl.oid)*100)::text,5)::text AS cfs_ratio");
	sql += wxT("\n  FROM pg_stat_all_tables st")
		wxT("  JOIN pg_class cl on cl.oid=st.relid\n");
	if (partcoll) sql += wxT("  JOIN pg_inherits i ON (cl.oid = i.inhrelid) WHERE ");
	if (tabcoll) sql += wxT("  WHERE schemaname = ")+obj->qtDbString(obj->GetSchema()->GetName());
	if (partcoll) sql += wxT(" i.inhparent = ") + obj->GetOidStr();
		//+ obj->qtDbString(obj->GetSchema()->GetName()) + wxT(" AND i.inhparent = ") + obj->GetOidStr()
	if (onetable) sql += wxT("join (select t.relid::oid oid from pg_partition_tree(")+ (obj->GetOidStr())+wxT("::regclass) t where t.isleaf = 't') t on t.oid=cl.oid");

	sql += wxT("\n ORDER BY relname");

	pgSet* stats = obj->GetDatabase()->ExecuteSet(sql);

	if (stats)
	{
		long pos = 0;
		int i;
		while (!stats->Eof())
		{
			i = 1;
			statistics->InsertItem(pos, stats->GetVal(wxT("relname")), PGICON_STATISTICS);
			if (hasSize)
				statistics->SetItem(pos, i++, stats->GetVal(wxT("size")));
			if (obj->GetConnection()->GetIsPgProEnt()) statistics->SetItem(pos, i++, stats->GetVal(wxT("cfs_ratio")));
			statistics->SetItem(pos, i++, stats->GetVal(wxT("n_live_tup")));
			statistics->SetItem(pos, i++, stats->GetVal(wxT("n_tup_ins")));
			statistics->SetItem(pos, i++, stats->GetVal(wxT("n_tup_upd")));
			statistics->SetItem(pos, i++, stats->GetVal(wxT("n_tup_del")));
			if (obj->GetConnection()->BackendMinimumVersion(8, 3))
			{
				statistics->SetItem(pos, i++, stats->GetVal(wxT("n_tup_hot_upd")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("n_dead_tup")));
			}
			if (obj->GetConnection()->BackendMinimumVersion(8, 2))
			{
				statistics->SetItem(pos, i++, stats->GetVal(wxT("last_vacuum")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("last_autovacuum")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("last_analyze")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("last_autoanalyze")));
				if (stats->GetVal(wxT("last_analyze")).IsEmpty() && stats->GetVal(wxT("last_autoanalyze")).IsEmpty())
					statistics->SetItemBackgroundColour(pos, wxColour(wxT("#EEAAAA")));
			}
			if (obj->GetConnection()->BackendMinimumVersion(9, 1))
			{
				statistics->SetItem(pos, i++, stats->GetVal(wxT("vacuum_count")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("autovacuum_count")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("analyze_count")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("autoanalyze_count")));
			}
			stats->MoveNext();
			pos++;
		}

		delete stats;
	}
	statistics->SetColumnWidth(0, wxLIST_AUTOSIZE);

}


bool pgObject::UpdateIcon(ctlTree *browser)
{
	int icon = GetIconId();
	if (GetId() && browser->GetItemImage(GetId(), wxTreeItemIcon_Normal) != icon)
	{
		browser->SetItemImage(GetId(), GetIconId(), wxTreeItemIcon_Normal);
		browser->SetItemImage(GetId(), GetIconId(), wxTreeItemIcon_Selected);
		return true;
	}
	return false;
}


void pgObject::ShowDependency(pgDatabase *db, ctlListView *list, const wxString &query, const wxString &clsorder)
{
	list->ClearAll();
	list->AddColumn(_("Type"), 60);
	list->AddColumn(_("Name"), 100);
	list->AddColumn(_("Restriction"), 50);

	pgConn *conn = GetConnection();
	if (conn)
	{
		pgSet *set;
		// currently missing:
		// - pg_cast
		// - pg_operator
		// - pg_opclass

		// not being implemented:
		// - pg_index (done by pg_class)
		wxString q=query + wxT("\n")
		                       wxT("   AND ") + clsorder + wxT(" IN (\n")
		                       wxT("   SELECT oid FROM pg_class\n")
		                       wxT("    WHERE relname IN ('pg_class', 'pg_constraint', 'pg_conversion', 'pg_language', 'pg_proc', 'pg_extension', \n")
		                       wxT("                      'pg_rewrite', 'pg_namespace', 'pg_trigger', 'pg_type', 'pg_attrdef', 'pg_event_trigger','pg_publication_rel','pg_subscription_rel'))\n")
		                       wxT(" ORDER BY ") + clsorder + wxT(", cl.relkind");
		set = conn->ExecuteSet(q);

		if (set)
		{
			while (!set->Eof())
			{
				wxString refname;
				wxString _refname = set->GetVal(wxT("refname"));

				if (db)
					refname = db->GetQuotedSchemaPrefix(set->GetVal(wxT("nspname")));
				else
				{
					refname = qtIdent(set->GetVal(wxT("nspname")));
					if (!refname.IsEmpty())
						refname += wxT(".");
				}

				wxString typestr = set->GetVal(wxT("type"));
				pgaFactory *depFactory = 0;
				int icon=-1;
				switch ((wxChar)typestr.c_str()[0])
				{
					case 'c':
					case 's':   // we don't know these; internally handled
					case 't':
						set->MoveNext();
						continue;

					case 'r':
					{
						if (StrToLong(typestr.Mid(1)) > 0)
							depFactory = &columnFactory;
						else
							depFactory = &tableFactory;
						break;
					}
					case 'i':
						depFactory = &indexFactory;
						break;
					case 'E':
						depFactory = &extensionFactory;
						break;
						
					case 'S':
						depFactory = &sequenceFactory;
						break;
					case 'v':
						depFactory = &viewFactory;
						break;
					case 'm':
						depFactory = &viewFactory;
						icon=viewFactory.GetMaterializedIconId();
						break;
					case 'x':
						depFactory = &extTableFactory;
						break;
					case 'p':
						depFactory = &functionFactory;
						break;
					case 'n':
						depFactory = &schemaFactory;
						break;
					case 'y':
						depFactory = &typeFactory;
						break;
					case 'T':
						depFactory = &triggerFactory;
						break;
					case 'f':
						depFactory = &foreignTableFactory;
						break;
						
					case 'l':
						depFactory = &languageFactory;
						break;
					case 'R':
					{
						refname = _refname + wxT(" ON ") + refname + set->GetVal(wxT("ownertable"));
						_refname = wxEmptyString;
						depFactory = &ruleFactory;
						break;
					}
					case 'C':
					{
						switch ((wxChar)typestr.c_str()[1])
						{
							case 'c':
								depFactory = &checkFactory;
								break;
							case 'f':
								refname += set->GetVal(wxT("ownertable")) + wxT(".");
								depFactory = &foreignKeyFactory;
								break;
							case 'p':
								depFactory = &primaryKeyFactory;
								break;
							case 'u':
								depFactory = &uniqueFactory;
								break;
							case 'x':
								depFactory = &excludeFactory;
								break;
							default:
								break;
						}
						break;
					}
					case 'A':
					{
						// Include only functions
						if (set->GetVal(wxT("adbin")).StartsWith(wxT("{FUNCEXPR")))
						{
							depFactory = &functionFactory;
							refname = set->GetVal(wxT("adsrc"));
							break;
						}
						else
						{
							set->MoveNext();
							continue;
						}
					}
					default:
						break;
				}

				refname += _refname;

				wxString typname;
				if (depFactory)
				{
					typname = depFactory->GetTypeName();
					if (icon==-1) icon = depFactory->GetIconId();
				}
				else
				{
					typname = _("Unknown");
					icon = -1;
				}

				wxString deptype;

				switch ( (wxChar) set->GetVal(wxT("deptype")).c_str()[0])
				{
					case 'n':
						deptype = wxT("normal");
						break;
					case 'a':
						deptype = wxT("auto");
						break;
					case 'i':
					{
						if (settings->GetShowSystemObjects())
							deptype = wxT("internal");
						else
						{
							set->MoveNext();
							continue;
						}
						break;
					}
					case 'p':
						deptype = wxT("pin");
						typname = wxEmptyString;
						break;
					default:
						break;
				}

				list->AppendItem(icon, typname, refname, deptype);
				set->MoveNext();
			}
			delete set;
		}
	}
}

void pgObject::CreateList3Columns(ctlListView *list, const wxString &left, const wxString &middle, const wxString &right)
{
	list->ClearAll();
	list->AddColumn(left, 80);
	list->AddColumn(middle, 80);
	list->AddColumn(right, list->GetSize().GetWidth() - 170);
}


void pgObject::CreateListColumns(ctlListView *list, const wxString &left, const wxString &right)
{
	list->ClearAll();
	list->AddColumn(left, 130);
	list->AddColumn(right, list->GetSize().GetWidth() - 140);
}

wxString GetClassTableName(int metatype) {
	switch (metatype)
	{
	case PGM_CONSTRAINT:
		return "pg_constraint";
	case PGM_FOREIGNKEY:
		return "pg_constraint";
	case PGM_PRIMARYKEY:
		return "pg_constraint";
	case PGM_UNIQUE:
		return "pg_constraint";
	case PGM_EXCLUDE:
		return "pg_constraint";
	case PGM_OPCLASS:
		return "pg_amop";
	case PGM_COLUMN:
		return "pg_attribute";
	case PGM_FUNCTION:
		return "pg_proc";
	case PGM_INDEX:
		return "pg_class";
	case PGM_TABLE:
		return "pg_class";
	case PGM_VIEW:
		return "pg_class";
	case PG_PARTITION:
		return "pg_class";
	case PGM_LANGUAGE:
		return "pg_language";
	case PGM_ROLE:
		return "pg_authid";
	case PGM_SCHEMA:
		return "pg_namespace";
	case PGM_SEQUENCE:
		return "pg_class";
	case PGM_TABLESPACE:
		return "pg_tablespace";
	case PGM_TRIGGER:
		return "pg_trigger";
	case PGM_OPFAMILY:
		return "pg_opfamily";
	default:
		break;
	}
	return "";
}

void pgObject::ShowDependencies(frmMain *form, ctlListView *Dependencies, const wxString &wh)
{
	if (this->IsCollection())
		return;

	// Bail out if this is a pgAgent object, as they use the OID for IDs
	if (GetMetaType() == PGM_JOB || GetMetaType() == PGM_SCHEDULE || GetMetaType() == PGM_STEP || GetMetaType() == PGM_PROJOB)
		return;

	wxString where;
	if (wh.IsEmpty())
	{
		if (!GetOidStr().IsSameAs(wxT("0")))
		{
			wxString ts = GetClassTableName(GetMetaType());
			where = wxT(" WHERE dep.objid=") + GetOidStr();
			if (!ts.IsEmpty()) where += " and dep.classid IN(select oid from pg_class where oid=dep.classid and relname='" + ts + "')";
		}
		else
			return;
	}
	else
		where = wh;
	/*
	* Behavior of concatinating operator (||) is different for EnterpriseDB.
	* For the following query:
	*     SELECT a || null;
	* When selected postgresql compatible mode, it will return null.
	* And when selected oracle compatible mode, it will return 'a'.
	*
	* Hence, the following query may or may not work for EnterpriseDB depending on the
	* compatiblity mode:
	*     COALESCE(cl.relname || '.' || att.attname, cl.relname, co.conname, pr.proname,
	*              tg.tgname, ty.typname, la.lanname, rw.rulename, ns.nspname)
	*
	* Instead of that, we will use the following query to run it correctly everytime:
	*     CASE WHEN cl.relname IS NOT NULL AND att.attname IS NOT NULL
	*               THEN cl.relname || '.' || att.attname
	*          ELSE COALESCE(cl.relname, co.conname, pr.proname, tg.tgname, ty.typname,
	*                        la.lanname, rw.rulename, ns.nspname)
	*     END
	*/
	ShowDependency(GetDatabase(), Dependencies,
	               wxT("SELECT DISTINCT dep.deptype, dep.refclassid, cl.relkind, ad.adbin, pg_get_expr(ad.adbin,ad.adrelid) adsrc, \n")
	               wxT("       CASE WHEN cl.relkind IS NOT NULL THEN cl.relkind::text || COALESCE(dep.refobjsubid::text, '')\n")
	               wxT("            WHEN tg.oid IS NOT NULL THEN 'T'::text\n")
	               wxT("            WHEN ty.oid IS NOT NULL THEN 'y'::text\n")
	               wxT("            WHEN ns.oid IS NOT NULL THEN 'n'::text\n")
	               wxT("            WHEN pr.oid IS NOT NULL THEN 'p'::text\n")
	               wxT("            WHEN la.oid IS NOT NULL THEN 'l'::text\n")
	               wxT("            WHEN rw.oid IS NOT NULL THEN 'R'::text\n")
	               wxT("            WHEN co.oid IS NOT NULL THEN 'C'::text || contype::text\n")
	               wxT("            WHEN ad.oid IS NOT NULL THEN 'A'::text\n")
				   wxT("            WHEN ext.oid IS NOT NULL THEN 'E'::text\n")
				   wxT("            WHEN pub.oid IS NOT NULL THEN 'r'::text\n")
	               wxT("            ELSE '' END AS type,\n")
	               wxT("       COALESCE(coc.relname, clrw.relname) AS ownertable,\n")
	               wxT("       CASE WHEN cl.relname IS NOT NULL AND att.attname IS NOT NULL THEN cl.relname || '.' || att.attname\n")
	               wxT("            ELSE COALESCE(ext.extname,cl.relname, co.conname, pr.proname, tg.tgname, ty.typname, la.lanname, rw.rulename, ns.nspname,pub.prrelid::regclass::text)\n")
	               wxT("       END AS refname,\n")
	               wxT("       COALESCE(nsc.nspname, nso.nspname, nsp.nspname, nst.nspname, nsrw.nspname) AS nspname\n")
	               wxT("  FROM pg_depend dep join pg_class nc on nc.oid=dep.refclassid\n")
	               wxT("  LEFT JOIN pg_class cl ON dep.refobjid=cl.oid and nc.relname='pg_class'\n")
	               wxT("  LEFT JOIN pg_attribute att ON dep.refobjid=att.attrelid AND dep.refobjsubid=att.attnum and nc.relname='pg_attribute'\n")
	               wxT("  LEFT JOIN pg_namespace nsc ON cl.relnamespace=nsc.oid\n")
	               wxT("  LEFT JOIN pg_proc pr ON dep.refobjid=pr.oid and nc.relname='pg_proc'\n")
	               wxT("  LEFT JOIN pg_namespace nsp ON pr.pronamespace=nsp.oid\n")
	               wxT("  LEFT JOIN pg_trigger tg ON dep.refobjid=tg.oid and nc.relname='pg_trigger'\n")
	               wxT("  LEFT JOIN pg_type ty ON dep.refobjid=ty.oid and nc.relname='pg_type'\n")
	               wxT("  LEFT JOIN pg_namespace nst ON ty.typnamespace=nst.oid\n")
	               wxT("  LEFT JOIN pg_constraint co ON dep.refobjid=co.oid and nc.relname='pg_constraint'\n")
	               wxT("  LEFT JOIN pg_class coc ON co.conrelid=coc.oid\n")
	               wxT("  LEFT JOIN pg_namespace nso ON co.connamespace=nso.oid\n")
	               wxT("  LEFT JOIN pg_rewrite rw ON dep.refobjid=rw.oid and nc.relname='pg_rewrite'\n")
	               wxT("  LEFT JOIN pg_class clrw ON clrw.oid=rw.ev_class\n")
	               wxT("  LEFT JOIN pg_namespace nsrw ON clrw.relnamespace=nsrw.oid\n")
	               wxT("  LEFT JOIN pg_language la ON dep.refobjid=la.oid and nc.relname='pg_language'\n")
	               wxT("  LEFT JOIN pg_namespace ns ON dep.refobjid=ns.oid and nc.relname='pg_namespace'\n")
	               wxT("  LEFT JOIN pg_attrdef ad ON ad.adrelid=att.attrelid AND ad.adnum=att.attnum\n")
				   wxT("  LEFT JOIN pg_publication_rel pub ON dep.objid=pub.oid AND pub.prpubid=dep.refobjid and nc.relname='pg_publication_rel'\n")
				   wxT("  LEFT JOIN pg_extension ext ON ext.oid=dep.refobjid\n")
	               + where, wxT("refclassid"));

	pgConn *conn = GetConnection();
	if (conn)
	{
		if (where.Find(wxT("subid")) < 0 && conn->BackendMinimumVersion(8, 1))
		{
			int iconId = groupRoleFactory.GetCollectionFactory()->GetIconId();
			pgSetIterator set(conn,
			                  wxT("SELECT rolname AS refname, refclassid, deptype\n")
			                  wxT("  FROM pg_shdepend dep\n")
			                  wxT("  LEFT JOIN pg_roles r ON refclassid=1260 AND refobjid=r.oid\n")
			                  + where + wxT("\n")
			                  wxT(" ORDER BY 1"));

			while (set.RowsLeft())
			{
				wxString refname = set.GetVal(wxT("refname"));
				wxString deptype = set.GetVal(wxT("deptype"));
				if (deptype == wxT("a"))
					deptype = wxT("ACL");
				else if (deptype == wxT("o"))
					deptype = _("Owner");

				if (set.GetOid(wxT("refclassid")) == PGOID_CLASS_PG_AUTHID)
					Dependencies->AppendItem(iconId, wxT("Role"), refname, deptype);
			}
		}
		/*
		*
		* A Corner case, reported by Guillaume Lelarge, could be found at:
		* http://archives.postgresql.org/pgadmin-hackers/2009-03/msg00026.php
		*
		* SQL:
		* CREATE TABLE t1 (id serial);
		* CREATE TABLE t2 (LIKE t1 INCLUDING DEFAULTS);
		*
		* When we try to drop the table t1, it gives the following notice:
		* "NOTICE:  default for table t2 column id depends on sequence t1_id_seq"
		*
		* This suggests that the column 't2.id' should be shown in the "Dependency" list
		* of the sequence 't1_seq_id'
		*
		* As we could not find any direct relationship between 't1_seq_id' and 't2'
		* table, we come up with this solution.
		*
		*/
		if (GetMetaType() == PGM_SEQUENCE)
		{
			int iconId = columnFactory.GetIconId();
			/*
			* Behavior of concatinating operator (||) is different for EnterpriseDB.
			* For the following query:
			*     SELECT a || null;
			* When selected postgresql compatible mode, it will return null.
			* And when selected oracle compatible mode, it will return 'a'.
			*
			* Hence, the following query may or may not work for EnterpriseDB depending on the
			* compatiblity mode:
			*     COALESCE(ref.relname || '.' || att.attname, ref.relname)
			*
			* Instead of that, we will use the following query to run it correctly everytime:
			*     CASE WHEN ref.relname IS NOT NULL AND att.attname IS NOT NULL
			*               THEN ref.relname || '.' || att.attname
			*          ELSE ref.relname
			*     END
			*/
			pgSetIterator set(conn,
			                  wxT("SELECT \n")
			                  wxT("  CASE WHEN att.attname IS NOT NULL AND ref.relname IS NOT NULL THEN ref.relname || '.' || att.attname\n")
			                  wxT("       ELSE ref.relname \n")
			                  wxT("  END AS refname, \n")
			                  wxT("  d2.refclassid, d1.deptype AS deptype\n")
			                  wxT("FROM pg_depend d1\n")
			                  wxT("  LEFT JOIN pg_depend d2 ON d1.objid=d2.objid AND d1.refobjid != d2.refobjid\n")
			                  wxT("  LEFT JOIN pg_class ref ON ref.oid = d2.refobjid\n")
			                  wxT("  LEFT JOIN pg_attribute att ON d2.refobjid=att.attrelid AND d2.refobjsubid=att.attnum\n")
			                  wxT("WHERE d1.classid=(SELECT oid FROM pg_class WHERE relname='pg_attrdef')\n")
			                  wxT("  AND d2.refobjid NOT IN (SELECT d3.refobjid FROM pg_depend d3 WHERE d3.objid=d1.refobjid)\n")
			                  wxT("  AND d1.refobjid=") + GetOidStr());
			while (set.RowsLeft())
			{
				wxString refname = set.GetVal(wxT("refname"));
				wxString deptype = set.GetVal(wxT("deptype"));
				if (deptype == wxT("n"))
					deptype = wxT("normal");
				else if (deptype == wxT("i"))
					deptype = _("internal");
				else if (deptype == wxT("a"))
					deptype = _("auto");

				Dependencies->AppendItem(iconId, wxT("Column"), refname, deptype);
			}
		}
	}

}
void pgObject::ShowDependents(frmMain* form, ctlListView* referencedBy, const wxString& wh)
{
	if (this->IsCollection())
		return;

	// Bail out if this is a pgAgent object, as they use the OID for IDs
	if (GetMetaType() == PGM_JOB || GetMetaType() == PGM_SCHEDULE || GetMetaType() == PGM_STEP || GetMetaType() == PGM_PROJOB)
		return;

	wxString where;
	if (wh.IsEmpty())
	{
		wxString ts = GetClassTableName(GetMetaType());
		where = wxT(" WHERE dep.refobjid=") + GetOidStr();
		if (!ts.IsEmpty()) where += " and dep.refclassid IN(select oid from pg_class where oid=dep.refclassid and relname='" + ts + "')";
	}
	else
		where = wh;
	/*
	* Behavior of concatinating operator (||) is different for EnterpriseDB.
	* For the following query:
	*     SELECT a || null;
	* When selected postgresql compatible mode, it will return null.
	* And when selected oracle compatible mode, it will return 'a'.
	*
	* Hence, the following query may or may not work for EnterpriseDB depending on the
	* compatiblity mode:
	*     COALESCE(cl.relname || '.' || att.attname, cl.relname, co.conname, pr.proname,
	*              tg.tgname, ty.typname, la.lanname, rw.rulename, ns.nspname)
	*
	* Instead of that, we will use the following query to run it correctly everytime:
	*     CASE WHEN cl.relname IS NOT NULL AND att.attname IS NOT NULL
	*               THEN cl.relname || '.' || att.attname
	*          ELSE COALESCE(cl.relname, co.conname, pr.proname, tg.tgname, ty.typname,
	*                        la.lanname, rw.rulename, ns.nspname)
	*     END
	*/
	ShowDependency(GetDatabase(), referencedBy,
	               wxT("SELECT DISTINCT dep.deptype, dep.classid, cl.relkind, ad.adbin, pg_get_expr(ad.adbin,ad.adrelid) adsrc, \n")
	               wxT("       CASE WHEN cl.relkind IS NOT NULL THEN cl.relkind::text || COALESCE(dep.objsubid::text, '')\n")
	               wxT("            WHEN tg.oid IS NOT NULL THEN 'T'::text\n")
	               wxT("            WHEN ty.oid IS NOT NULL THEN 'y'::text\n")
	               wxT("            WHEN ns.oid IS NOT NULL THEN 'n'::text\n")
	               wxT("            WHEN pr.oid IS NOT NULL THEN 'p'::text\n")
	               wxT("            WHEN la.oid IS NOT NULL THEN 'l'::text\n")
	               wxT("            WHEN rw.oid IS NOT NULL THEN 'R'::text\n")
	               wxT("            WHEN co.oid IS NOT NULL THEN 'C'::text || contype::text\n")
	               wxT("            WHEN ad.oid IS NOT NULL THEN 'A'::text\n")
				   wxT("            WHEN pub.oid IS NOT NULL THEN 'r'::text\n")
				   wxT("            WHEN ext.oid IS NOT NULL THEN 'E'::text\n")
	               wxT("            ELSE '' END AS type,\n")
	               wxT("       COALESCE(coc.relname, clrw.relname) AS ownertable,\n")
	               wxT("       CASE WHEN cl.relname IS NOT NULL AND att.attname IS NOT NULL THEN cl.relname || '.' || att.attname \n")
	               wxT("            ELSE COALESCE(ext.extname,cl.relname, co.conname, pr.proname, tg.tgname, ty.typname, la.lanname, rw.rulename, ns.nspname,pub.prrelid::regclass::text) \n")
	               wxT("       END AS refname,\n")
	               wxT("       COALESCE(nsc.nspname, nso.nspname, nsp.nspname, nst.nspname, nsrw.nspname) AS nspname\n")
	               wxT("  FROM pg_depend dep join pg_class nc on nc.oid=dep.classid\n")
	               wxT("  LEFT JOIN pg_class cl ON dep.objid=cl.oid and nc.relname='pg_class'\n")
	               wxT("  LEFT JOIN pg_attribute att ON dep.objid=att.attrelid AND dep.objsubid=att.attnum and nc.relname='pg_attribute'\n")
	               wxT("  LEFT JOIN pg_namespace nsc ON cl.relnamespace=nsc.oid\n")
	               wxT("  LEFT JOIN pg_proc pr ON dep.objid=pr.oid and nc.relname='pg_proc'\n")
	               wxT("  LEFT JOIN pg_namespace nsp ON pr.pronamespace=nsp.oid\n")
	               wxT("  LEFT JOIN pg_trigger tg ON dep.objid=tg.oid and nc.relname='pg_trigger'\n")
	               wxT("  LEFT JOIN pg_type ty ON dep.objid=ty.oid and nc.relname='pg_type'\n")
	               wxT("  LEFT JOIN pg_namespace nst ON ty.typnamespace=nst.oid\n")
	               wxT("  LEFT JOIN pg_constraint co ON dep.objid=co.oid and nc.relname='pg_constraint'\n")
	               wxT("  LEFT JOIN pg_class coc ON co.conrelid=coc.oid\n")
	               wxT("  LEFT JOIN pg_namespace nso ON co.connamespace=nso.oid\n")
	               wxT("  LEFT JOIN pg_rewrite rw ON dep.objid=rw.oid and nc.relname='pg_rewrite'\n")
	               wxT("  LEFT JOIN pg_class clrw ON clrw.oid=rw.ev_class\n")
	               wxT("  LEFT JOIN pg_namespace nsrw ON clrw.relnamespace=nsrw.oid\n")
	               wxT("  LEFT JOIN pg_language la ON dep.objid=la.oid and nc.relname='pg_language'\n")
	               wxT("  LEFT JOIN pg_namespace ns ON dep.objid=ns.oid and nc.relname='pg_namespace'\n")
	               wxT("  LEFT JOIN pg_attrdef ad ON ad.oid=dep.objid and nc.relname='pg_attrdef'\n")
				   wxT("  LEFT JOIN pg_extension ext ON ext.oid=dep.objid and nc.relname='pg_extension'\n")
				   wxT("  LEFT JOIN pg_publication_rel pub ON dep.objid=pub.oid AND pub.prpubid=dep.refobjid and nc.relname='pg_publication_rel'\n")
	               + where, wxT("classid"));
	
	/*
	*
	* A Corner case, reported by Guillaume Lelarge, could be found at:
	* http://archives.postgresql.org/pgadmin-hackers/2009-03/msg00026.php
	*
	* SQL:
	* CREATE TABLE t1 (id serial);
	* CREATE TABLE t2 (LIKE t1 INCLUDING DEFAULTS);
	*
	* When we try to drop the table t1, it gives the following notice:
	* "NOTICE:  default for table t2 column id depends on sequence t1_id_seq"
	*
	* This suggests that the sequence 't1_seq_id' should be shown in the
	* "Dependents" list of the table 't2' and column 't2.id'
	*
	* As we could not find any direct relationship between 't1_seq_id' and 't2'
	* table, we come up with this solution.
	*
	*/
	pgConn *conn = GetConnection();
	if (conn && (GetMetaType() == PGM_TABLE || GetMetaType() == PGM_COLUMN))
	{
		int iconId = sequenceFactory.GetIconId();
		wxString strQuery =
		    wxT("SELECT ref.relname AS refname, d2.refclassid, dep.deptype AS deptype\n")
		    wxT("  FROM pg_depend dep\n")
		    wxT("  LEFT JOIN pg_depend d2 ON dep.objid=d2.objid AND dep.refobjid != d2.refobjid\n")
		    wxT("  LEFT JOIN pg_class ref ON ref.oid=d2.refobjid\n")
		    wxT("  LEFT JOIN pg_attribute att ON d2.refclassid=att.attrelid AND d2.refobjsubid=att.attnum\n")
		    + where +
		    wxT("    AND dep.classid=(SELECT oid FROM pg_class WHERE relname='pg_attrdef')\n")
		    wxT("    AND dep.refobjid NOT IN (SELECT d3.refobjid FROM pg_depend d3 WHERE d3.objid=d2.refobjid)");


		pgSetIterator set(conn, strQuery);

		while (set.RowsLeft())
		{
			wxString refname = set.GetVal(wxT("refname"));
			if (refname.IsEmpty())
				continue;

			wxString deptype = set.GetVal(wxT("deptype"));
			if (deptype == wxT("a"))
				deptype = _("auto");
			else if (deptype == wxT("n"))
				deptype = _("normal");
			else if (deptype == wxT("i"))
				deptype = _("internal");

			referencedBy->AppendItem(iconId, wxT("Sequence"), refname, deptype);
		}
	}
}


void pgObject::ShowTree(frmMain *form, ctlTree *browser, ctlListView *properties, ctlSQLBox *sqlPane)
{
	pgConn *conn = GetConnection();
	if (conn)
	{
		int status = conn->GetStatus();
		if (status == PGCONN_BROKEN || status == PGCONN_BAD)
		{
			form->SetStatusText(_(" Connection broken."));
			return;
		}
	}

	wxLogInfo(wxT("Displaying properties for %s %s"), GetTypeName().c_str(), GetIdentifier().c_str());

	if (form)
	{
		form->StartMsg(GetTranslatedMessage(RETRIEVINGDETAILS));

		SetContextInfo(form);

		form->ShowObjStatistics(this);
	}

	ShowTreeDetail(browser, form, properties, sqlPane);
	if (form)
		form->EndMsg(!GetConnection() || GetConnection()->GetStatus() == PGCONN_OK);
}


wxTreeItemId pgObject::AppendBrowserItem(ctlTree *browser, pgObject *object)
{
	return browser->AppendObject(this, object);
}


wxString pgObject::GetCommentSql()
{
	wxString cmt;
	if (!comment.IsNull())
	{
		cmt = wxT("COMMENT ON ") + GetTypeName().Upper() + wxT(" ") + GetQuotedFullIdentifier()
		      + wxT("\n  IS ") + qtDbString(comment) + wxT(";\n");
	}
	return cmt;
}


wxString pgObject::GetOwnerSql(int major, int minor, wxString objname, wxString objtype)
{
	wxString sql;
	if (GetConnection()->BackendMinimumVersion(major, minor))
	{
//      if (GetConnection()->GetUser() != owner)       // optional?
		{
			if (objtype.IsEmpty())
				objtype = GetTypeName().Upper();

			if (objname.IsEmpty())
				objname = objtype + wxT(" ") + GetQuotedFullIdentifier();

			sql = wxT("ALTER ") + objname + wxT("\n  OWNER TO ") + qtIdent(owner) + wxT(";\n");
		}
	}
	return sql;
}


void pgObject::AppendRight(wxString &rights, const wxString &acl, wxChar c, const wxChar *rightName, const wxString &column)
{
	if (acl.Find(c) >= 0)
	{
		if (!rights.IsNull())
			rights.Append(wxT(", "));
		rights.Append(rightName);

		if (!column.IsEmpty())
			rights.Append(wxT("(") + column + wxT(")"));
	}
}


wxString pgObject::GetPrivilegeGrant(const wxString &allPattern, const wxString &acl, const wxString &grantOnObject, const wxString &user, const wxString &column)
{
	wxString rights;

	if (allPattern.Length() > 1 && acl == allPattern)
	{
		rights = wxT("ALL");
		if (!column.IsEmpty())
			rights += wxT("(") + column + wxT(")");
	}
	else
	{
		AppendRight(rights, acl, 'r', wxT("SELECT"), column);
		AppendRight(rights, acl, 'w', wxT("UPDATE"), column);
		AppendRight(rights, acl, 'a', wxT("INSERT"), column);
		AppendRight(rights, acl, 'D', wxT("TRUNCATE"), column);
		AppendRight(rights, acl, 'c', wxT("CONNECT"), column);
		AppendRight(rights, acl, 'd', wxT("DELETE"), column);
		AppendRight(rights, acl, 'R', wxT("RULE"), column);
		AppendRight(rights, acl, 'x', wxT("REFERENCES"), column);
		AppendRight(rights, acl, 't', wxT("TRIGGER"), column);
		AppendRight(rights, acl, 'm', wxT("MAINTAIN"), column);
		AppendRight(rights, acl, 'X', wxT("EXECUTE"), column);
		AppendRight(rights, acl, 'U', wxT("USAGE"), column);
		AppendRight(rights, acl, 'C', wxT("CREATE"), column);
		AppendRight(rights, acl, 'T', wxT("TEMPORARY"), column);
	}
	wxString grant;
	if (rights.IsNull())
	{
		grant += wxT("REVOKE ALL");
		if (!column.IsEmpty())
			grant += wxT("(") + column + wxT(")");
	}
	else
		grant += wxT("GRANT ") + rights;

	grant += wxT(" ON ") + grantOnObject;

	if (rights.IsNull())    grant += wxT(" FROM ");
	else                    grant += wxT(" TO ");

	grant += user;

	return grant;
}


wxArrayString pgObject::GetProviderLabelArray()
{
	wxArrayString providersArray, labelsArray, seclabelsArray;
	wxString currentChar;
	wxString tmp;
	bool wrappedInQuotes, antislash;

	if (labels.IsEmpty())
		return seclabelsArray;

	// parse the labels string
	// we start at 1 and stop at length-1 to get rid of the { and } of the array
	for (unsigned int index = 1 ; index < labels.Length() - 1 ; index++)
	{
		// get current char
		currentChar = labels.Mid(index, 1);

		// if there is a double quote at the beginning of a label,
		// the whole label will be wrapped in quotes
		if (currentChar == wxT("\"") && tmp.IsEmpty())
			wrappedInQuotes = true;
		else if (currentChar == wxT("\\") && wrappedInQuotes)
			antislash = true;
		else
		{
			if ((currentChar == wxT(",") && !wrappedInQuotes && !tmp.IsEmpty())
			        || (currentChar == wxT("\"") && wrappedInQuotes && !antislash && !tmp.IsEmpty()))
			{
				// put new label in the array
				labelsArray.Add(tmp);

				// reinit tmp
				tmp = wxEmptyString;
				wrappedInQuotes = false;
			}
			else
				tmp += currentChar;
			antislash = false;
		}
	}

	// last label
	if (!tmp.IsEmpty())
	{
		// put last label in the array
		labelsArray.Add(tmp);
	}

	// reinit tmp
	tmp = wxEmptyString;
	wrappedInQuotes = false;

	// parse the providers string
	// we start at 1 and stop at length-1 to get rid of the { and } of the array
	for (unsigned int index = 1 ; index < providers.Length() - 1 ; index++)
	{
		// get current char
		currentChar = providers.Mid(index, 1);

		// if there is a double quote at the beginning of a provider,
		// the whole provider will be wrapped in quotes
		if (currentChar == wxT("\"") && tmp.IsEmpty())
			wrappedInQuotes = true;
		else if (currentChar == wxT("\\") && wrappedInQuotes)
			antislash = true;
		else
		{
			if ((currentChar == wxT(",") && !wrappedInQuotes && !tmp.IsEmpty())
			        || (currentChar == wxT("\"") && wrappedInQuotes && !antislash && !tmp.IsEmpty()))
			{
				// put new provider in the array
				providersArray.Add(tmp);

				// reinit tmp
				tmp = wxEmptyString;
				wrappedInQuotes = false;
			}
			else
				tmp += currentChar;
			antislash = false;
		}
	}

	// last provider
	if (!tmp.IsEmpty())
	{
		// put last provider in the array
		providersArray.Add(tmp);
	}

	// now, build one wxArrayString from these two
	for (unsigned int index = 0 ; index < providersArray.GetCount() ; index++)
	{
		seclabelsArray.Add(providersArray.Item(index));
		seclabelsArray.Add(labelsArray.Item(index));
	}

	// return the final one
	return seclabelsArray;
}


wxString pgObject::GetSeqLabelsSql()
{
	wxString sql = wxEmptyString;
	wxArrayString seclabels = GetProviderLabelArray();
	if (seclabels.GetCount() > 0)
	{
		for (unsigned int index = 0 ; index < seclabels.GetCount() - 1 ; index += 2)
		{
			sql += wxT("SECURITY LABEL FOR ") + seclabels.Item(index)
			       + wxT("\n  ON ") + GetTypeName().Upper() + wxT(" ") + GetQuotedFullIdentifier()
			       + wxT("\n  IS ") + qtDbString(seclabels.Item(index + 1)) + wxT(";\n");
		}
	}

	return sql;
}


wxString pgObject::GetPrivileges(const wxString &allPattern, const wxString &str, const wxString &grantOnObject, const wxString &user, const wxString &column)
{
	wxString aclWithGrant, aclWithoutGrant;

	const wxChar *p = str.c_str();
	while (*p)
	{
		if (allPattern.Find(*p) >= 0)
		{
			if (p[1] == (wxChar)'*')
				aclWithGrant += *p;
			else
				aclWithoutGrant += *p;
		}
		p++;
		if (*p == (wxChar)'*')
			p++;
	}

	wxString grant;
	if (!aclWithoutGrant.IsEmpty() || aclWithGrant.IsEmpty())
		grant += GetPrivilegeGrant(allPattern, aclWithoutGrant, grantOnObject, user, column) + wxT(";\n");
	if (!aclWithGrant.IsEmpty())
		grant += GetPrivilegeGrant(allPattern, aclWithGrant, grantOnObject, user, column) + wxT(" WITH GRANT OPTION;\n");

	return grant;
}


wxString pgObject::GetGrant(const wxString &allPattern, const wxString &_grantFor, const wxString &_column)
{
	wxString grant, str, user, grantFor, tmpUser;

	if (_grantFor.IsNull())
	{
		grantFor = GetTypeName();
		grantFor.MakeUpper();
		grantFor += wxT(" ") + GetQuotedFullIdentifier();
	}
	else
		grantFor = _grantFor;

	if (!acl.IsNull())
	{
		if (acl == wxT("{}"))
		{
			grant += GetPrivileges(allPattern, str, grantFor, wxT("public"), qtIdent(_column));
			grant += GetPrivileges(allPattern, str, grantFor, qtIdent(owner), qtIdent(_column));
		}
		else
		{
			// checks if certain privilege is granted to public
			bool grantedToPublic = false;
			// checks if certain privilege is granted to owner
			bool grantedToOwner = false;

			//queryTokenizer acls(acl.Mid(1, acl.Length() - 2), ',');
			wxSortedArrayString acls(wxSplit(acl.Mid(1, acl.Length() - 2),','));

			//while (acls.HasMoreTokens())
			for (int j=0;j<acls.Count();j++)
			{
				//str = acls.GetNextToken();
				str = acls[j];
				if (str.Left(1) == '"')
					str = str.Mid(1, str.Length() - 2);
				user = str.BeforeFirst('=');
				str = str.AfterFirst('=').BeforeFirst('/');
				if (user == wxT(""))
				{
					user = wxT("public");
					grantedToPublic = true;
				}
				else
				{
					if (user.Left(6) == wxT("group "))
					{
						tmpUser = user.Mid(6);
						if (user.Mid(6).StartsWith(wxT("\\\"")) && user.Mid(6).EndsWith(wxT("\\\"")))
							user = wxT("GROUP ") + qtIdent(user.Mid(8, user.Length() - 10));
						else
							user = wxT("GROUP ") + qtIdent(user.Mid(6));
					}
					else
					{
						tmpUser = user;
						if (user.StartsWith(wxT("\\\"")) && user.EndsWith(wxT("\\\"")))
							user = qtIdent(user.Mid(2, user.Length() - 4));
						else
							user = qtIdent(user);
					}

					if (tmpUser.Contains(owner))
						grantedToOwner = true;
				}

				grant += GetPrivileges(allPattern, str, grantFor, user, qtIdent(_column));
			}

			str = wxEmptyString;
			int metaType = GetMetaType();

			// We check here that whether the user has revoked prvileges granted to databases, functions
			// and languages. If so then this must be part of reverse engineered sql statement
			if (!grantedToPublic && (metaType == PGM_LANGUAGE || metaType == PGM_FUNCTION || metaType == PGM_DATABASE))
				grant += GetPrivileges(allPattern, str, grantFor, wxT("public"), qtIdent(_column));
			// We check here that whether the owner has revoked prvileges on himself to this postgres
			// object. If so then this must be part of reverse engineered sql statement
			if (!grantedToOwner)
				grant += GetPrivileges(allPattern, str, grantFor, qtIdent(owner), qtIdent(_column));
		}
	}
	return grant;
}



pgConn *pgObject::GetConnection() const
{
	pgDatabase *db = GetDatabase();
	if (db)
		return db->connection();

	pgServer *server;

	if (IsCreatedBy(serverFactory))
		server = (pgServer *)this;
	else
		server = GetServer();

	if (server)
		return server->connection();
	return 0;
}


bool pgObject::CheckOpenDialogs(ctlTree *browser, wxTreeItemId node)
{
	pgObject *obj = browser->GetObject(node);
	if (obj && obj->GetWindowPtr())
		return true;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = browser->GetFirstChild(node, cookie);

	while (child.IsOk())
	{
		obj = browser->GetObject(child);
		if (obj && obj->GetWindowPtr())
			return true;

		wxTreeItemIdValue subCookie;
		wxTreeItemId subChildItem = browser->GetFirstChild(child, subCookie);
		if (browser->IsExpanded(child))
		{
			if (CheckOpenDialogs(browser, child))
				return true;
		}
		// It may be the case the user might have expanded the node opened the
		// dialog and then collapsed the node again. This case is handled in the
		// check below
		else if (subChildItem && browser->GetItemData(subChildItem))
		{
			if (CheckOpenDialogs(browser, child))
				return true;
		}

		child = browser->GetNextChild(node, cookie);
	}

	return false;
}

//////////////////////////////////////////////////////////////

bool pgServerObject::CanDrop()
{
	if (GetMetaType() == PGM_DATABASE)
		return (server->GetCreatePrivilege() || server->GetSuperUser());
	else
	{
		if (server->GetConnection()->BackendMinimumVersion(8, 1) && GetMetaType() == PGM_ROLE)
			return (server->GetCreateRole() || server->GetSuperUser());
		else
			return server->GetSuperUser();
	}
}


bool pgServerObject::CanCreate()
{
	if (GetMetaType() == PGM_DATABASE)
		return (server->GetCreatePrivilege() || server->GetSuperUser());
	else
	{
		if (server->GetConnection()->BackendMinimumVersion(8, 1) && GetMetaType() == PGM_ROLE)
			return (server->GetCreateRole() || server->GetSuperUser());
		else
			return server->GetSuperUser();
	}
}


void pgServerObject::FillOwned(ctlTree *browser, ctlListView *referencedBy, const wxArrayString &dblist, const wxString &query)
{
	pgCollection *databases = 0;

	wxCookieType cookie;
	wxTreeItemId item = browser->GetFirstChild(GetServer()->GetId(), cookie);
	while (item)
	{
		databases = (pgCollection *)browser->GetObject(item);
		if (databases && databases->GetMetaType() == PGM_DATABASE)
			break;
		else
			databases = 0;

		item = browser->GetNextChild(GetServer()->GetId(), cookie);
	}

	size_t i;
	for (i = 0 ; i < dblist.GetCount() ; i++)
	{
		wxString dbname = dblist.Item(i);
		pgConn *conn = 0;
		pgConn *tmpConn = 0;

		if (GetServer()->GetDatabaseName() == dbname)
			conn = GetServer()->GetConnection();
		else
		{
			item = browser->GetFirstChild(databases->GetId(), cookie);
			while (item)
			{
				pgDatabase *db = (pgDatabase *)browser->GetObject(item);
				if (db && db->GetMetaType() == PGM_DATABASE && db->GetName() == dbname)
				{
					if (db->GetConnected())
						conn = db->GetConnection();
					break;
				}
				item = browser->GetNextChild(databases->GetId(), cookie);
			}
		}
		if (conn && conn->GetStatus() != PGCONN_OK)
			conn = 0;

		if (!conn)
		{
			tmpConn = GetServer()->CreateConn(dbname);
			conn = tmpConn;
		}

		if (conn)
		{
			pgSet *set = conn->ExecuteSet(query);

			if (set)
			{
				while (!set->Eof())
				{
					wxString relname = qtIdent(set->GetVal(wxT("nspname")));
					if (!relname.IsEmpty())
						relname += wxT(".");
					relname += qtIdent(set->GetVal(wxT("relname")));
					pgaFactory *ownerFactory = 0;

					switch ( (wxChar)set->GetVal(wxT("relkind")).c_str()[0])
					{
						case 'r':
							ownerFactory = &tableFactory;
							break;
						case 'i':
							ownerFactory = &indexFactory;
							relname = qtIdent(set->GetVal(wxT("indname"))) + wxT(" ON ") + relname;
							break;
						case 'S':
							ownerFactory = &sequenceFactory;
							break;
						case 'v':
							ownerFactory = &viewFactory;
							break;
						case 'x':
							ownerFactory = &extTableFactory;
							break;
						case 'c':   // composite type handled in PG_TYPE
						case 's':   // special
						case 't':   // toast
							break;
						case 'n':
							ownerFactory = &schemaFactory;
							break;
						case 'y':
							ownerFactory = &typeFactory;
							break;
						case 'd':
							ownerFactory = &domainFactory;
							break;
						case 'C':
							ownerFactory = &conversionFactory;
							break;
						case 'p':
							ownerFactory = &functionFactory;
							break;
						case 'T':
							ownerFactory = &triggerFunctionFactory;
							break;
						case 'o':
							ownerFactory = &operatorFactory;
							relname = set->GetVal(wxT("relname"));  // unquoted
							break;
					}

					if (ownerFactory)
					{
						wxString typname;
						int icon;
						typname = ownerFactory->GetTypeName();
						icon = ownerFactory->GetIconId();
						referencedBy->AppendItem(icon, typname, dbname, relname);
					}

					set->MoveNext();
				}
				delete set;
			}
		}

		if (tmpConn)
			delete tmpConn;
	}
}

//////////////////////////////////////////////////////////////

pgServer *pgDatabaseObject::GetServer() const
{
	return database->GetServer();
}


bool pgDatabaseObject::CanDrop()
{
	return (database->GetCreatePrivilege() && (GetMetaType() != PGM_CATALOG));
}


bool pgDatabaseObject::CanCreate()
{
	return database->GetCreatePrivilege();
}


wxString pgDatabaseObject::GetSchemaPrefix(const wxString &schemaname) const
{
	return database->GetSchemaPrefix(schemaname);
}


wxString pgDatabaseObject::GetQuotedSchemaPrefix(const wxString &schemaname) const
{
	return database->GetQuotedSchemaPrefix(schemaname);
}


void pgDatabaseObject::DisplayStatistics(ctlListView *statistics, const wxString &query)
{
	if (statistics)
	{
		wxLogInfo(wxT("Displaying statistics for %s %s"), GetTypeName().c_str(), GetFullIdentifier().c_str());

		// Add the statistics view columns
		CreateListColumns(statistics, _("Statistic"), _("Value"));

		pgSet *stats = database->ExecuteSet(query);

		if (stats)
		{
			bool pretty = settings->GetNumberPretty();
			int col;
			wxArrayInt a;
			int vmax = -1;
			int lt = -1;
			for (col = 0 ; col < stats->NumCols() ; col++)
			{
				if (!stats->ColName(col).IsEmpty()) {
					wxString name = stats->ColName(col);
					wxString vl = stats->GetVal(col);
					if (vl.IsNumber() && vl.Length()>0) {
						int l = vl.Length();
						if (l > vmax) vmax = l;
						a.Add(l);
						if (_("Live Tuples") == name) lt = a.GetCount() - 1;
					} else
						a.Add(-1);
					statistics->AppendItem(name, vl);
				}
			}
			if (vmax > 0 && pretty) {
				for (int i = 0; i < a.Count(); i++) {
					if (a[i] >= 0) {
						wxString str = statistics->GetItemText(i, 1);
						wxLongLong l = StrToLongLong(str);
						wxString h = NumToStrHuman(l);
						if (h.IsEmpty()) continue;
						str += generate_spaces(vmax - a[i] + 5) + h;
						if (lt == i) str[vmax + 2] = 'R';
						statistics->SetItem(i, 1, str);
					}

				}
			}
			delete stats;
		}
	}
}


///////////////////////////////////////////////////////////////

void pgSchemaObject::SetSchema(pgSchema *newSchema)
{
	schema = newSchema;
	database = schema->GetDatabase();
}


void pgSchemaObject::UpdateSchema(ctlTree *browser, OID schemaOid)
{
	// used e.g. for triggers that use trigger functions from other namespaces
	if (!browser)
		return;

	if (schema->GetOid() != schemaOid)
	{
		pgObject *schemas = browser->GetObject(browser->GetItemParent(schema->GetId()));

		wxASSERT(schemas);
		treeObjectIterator schIt(browser, schemas);
		pgSchema *sch;

		while ((sch = (pgSchema *)schIt.GetNextObject()) != 0)
		{
			if (sch->GetOid() == schemaOid)
			{
				SetSchema(sch);
				return;
			}
		}

		// If we get this far, it's possible the schema is actually a catalog
		// in this case. We need to find the catalogs node and then search that.
		pgObject *db = browser->GetObject(browser->GetItemParent(schemas->GetId()));

		wxASSERT(db);
		treeObjectIterator catsIt(browser, db);
		pgObject *catalogs;

		while ((catalogs = (pgObject *)catsIt.GetNextObject()) != 0)
		{
			if (catalogs->GetMetaType() == PGM_CATALOG)
				break;
		}

		// Assuming we got the catalogs node, now get the catalog
		if (catalogs)
		{
			treeObjectIterator catIt(browser, catalogs);
			pgCatalog *cat;

			while ((cat = (pgCatalog *)catIt.GetNextObject()) != 0)
			{
				if (cat->GetOid() == schemaOid)
				{
					SetSchema((pgSchema *)cat);
					return;
				}
			}
		}

		wxMessageBox(_("The schema oid can't be located, please refresh all schemas!"),
		             _("Missing information"), wxICON_EXCLAMATION | wxOK, browser);
	}
}


bool pgSchemaObject::GetSystemObject() const
{
	if (!schema)
		return false;
	return GetOid() < GetConnection()->GetLastSystemOID();
}


bool pgSchemaObject::CanDrop()
{
	return schema->GetCreatePrivilege() && schema->GetMetaType() != PGM_CATALOG;
}


bool pgSchemaObject::CanCreate()
{
	return schema->GetCreatePrivilege() && schema->GetMetaType() != PGM_CATALOG;
}


void pgSchemaObject::SetContextInfo(frmMain *form)
{
//    form->SetDatabase(schema->GetDatabase());
}

pgSet *pgSchemaObject::ExecuteSet(const wxString &sql)
{
	return schema->GetDatabase()->ExecuteSet(sql);
}

wxString pgSchemaObject::ExecuteScalar(const wxString &sql)
{
	return schema->GetDatabase()->ExecuteScalar(sql);
}

bool pgSchemaObject::ExecuteVoid(const wxString &sql)
{
	return schema->GetDatabase()->ExecuteVoid(sql);
}


wxString pgSchemaObject::GetFullIdentifier() const
{
	return schema->GetPrefix() + GetName();
}


wxString pgSchemaObject::GetQuotedFullIdentifier() const
{
	if (schema->GetTypeName() == wxT("Table"))
		return schema->GetSchema()->GetQuotedPrefix() + GetQuotedIdentifier();
	else
		return schema->GetQuotedPrefix() + GetQuotedIdentifier();
}




enum tokentype
{
	SQLTK_NORMAL = 0,
	SQLTK_JOINMOD,
	SQLTK_JOIN,
	SQLTK_ON,
	SQLTK_UNION

};

typedef struct __tokenaction
{
	const wxChar *keyword, *replaceKeyword;
	int actionBefore, actionAfter;
	tokentype special;
	bool doBreak;
} tokenAction;


tokenAction sqlTokens[] =
{
	{ wxT("WHERE")},     // initializing fails, so we're doing it in the code
	{ wxT("SELECT"), wxT(" SELECT"),   0, 8,      SQLTK_NORMAL,   true},
	{ wxT("FROM"),   wxT("   FROM"),  -8, 8,      SQLTK_NORMAL,   true},
	{ wxT("LEFT"),   wxT("   LEFT"),  -8, 13,     SQLTK_JOINMOD,  true},
	{ wxT("RIGHT"),  wxT("   RIGHT"), -8, 13,     SQLTK_JOINMOD,  true},
	{ wxT("NATURAL"), wxT("   NATURAL"), -8, 13,   SQLTK_JOINMOD,  true},
	{ wxT("FULL"),   wxT("   FULL"),  -8, 13,     SQLTK_JOINMOD,  true},
	{ wxT("CROSS"),  wxT("   CROSS"), -8, 13,     SQLTK_JOINMOD,  true},
	{ wxT("UNION"),  wxT("   UNION"), -8, 13,     SQLTK_UNION,    true},
	{ wxT("JOIN"),   wxT("   JOIN"),  -8, 13,     SQLTK_JOIN,     true},
	{ wxT("ON"),     wxT("ON"),        0, -5,     SQLTK_ON,       false},
	{ wxT("ORDER"),  wxT("  ORDER"),  -8, 8,      SQLTK_NORMAL,   true},
	{ wxT("GROUP"),  wxT("  GROUP"),  -8, 8,      SQLTK_NORMAL,   true},
	{ wxT("HAVING"), wxT(" HAVING"),  -8, 8,      SQLTK_NORMAL,   true},
	{ wxT("LIMIT"),  wxT("  LIMIT"),  -8, 8,      SQLTK_NORMAL,   true},
	{ wxT("CASE"),   wxT("CASE"),      0, 4,      SQLTK_NORMAL,   true},
	{ wxT("WHEN"),   wxT("WHEN"),      0, 0,      SQLTK_NORMAL,   true},
	{ wxT("ELSE"),   wxT("ELSE"),      0, 0,      SQLTK_NORMAL,   true},
	{ wxT("END"),    wxT("END "),     -4, 0,      SQLTK_NORMAL,   true},
	{0, 0, 0, 0, SQLTK_NORMAL, false}
};

tokenAction secondOnToken =
{ wxT("ON"),     wxT("ON"),       -5, 0,      SQLTK_ON,       true};



wxString pgRuleObject::GetFormattedDefinition()
{
	// pgsql 7.4 does formatting itself
	if (!GetDatabase()->GetPrettyOption().IsEmpty())
		return GetDefinition();

	////////////////////////////////
	// ok, this code looks weird. It's necessary, because somebody (NOT the running code)
	// will screw up that entry. It's broken in pgAdmin3::OnInit() already.
	// maybe your compiler does better (VC6SP5, but an older c2xx to avoid other bugs)

	sqlTokens[0].replaceKeyword = wxT("  WHERE");
	sqlTokens[0].actionBefore = -8;
	sqlTokens[0].actionAfter = 8;
	sqlTokens[0].special = SQLTK_NORMAL;
	sqlTokens[0].doBreak = true;

	wxString fc, token;
	queryTokenizer tokenizer(GetDefinition());
	int indent = 0;
	int position = 0; // col position. updated, but not used at the moment.
	bool wasOn = false;

	while (tokenizer.HasMoreTokens())
	{
		token = tokenizer.GetNextToken();

gotToken:
		wxString trailingChars;

		// token may contain brackets
		int bracketPos;
		bracketPos = token.Find('(', true);
		while (bracketPos >= 0)
		{
			fc += token.Left(bracketPos + 1);
			token = token.Mid(bracketPos + 1);
			bracketPos = token.Find('(', true);
		}

		bracketPos = token.Find(')', true);
		while (bracketPos >= 0)
		{
			trailingChars = token.Mid(bracketPos) + trailingChars;
			token = token.Left(bracketPos);
			bracketPos = token.Find(')', true);
		}
		// identify token
		tokenAction *tp = sqlTokens;
		while (tp->keyword)
		{
			if (!token.CmpNoCase(tp->keyword))
			{
				if (tp->special == SQLTK_ON && wasOn)
					tp = &secondOnToken;
				else
					wasOn = (tp->special == SQLTK_ON);
				break;
			}
			tp++;
		}

		if (tp && tp->keyword)
		{
			// we found a keyword.
			if (tp->special == SQLTK_UNION || tp->special == SQLTK_JOINMOD)
			{
				token = tokenizer.GetNextToken();
				if (tp->special == SQLTK_UNION && token.CmpNoCase(wxT("JOIN")))
				{
					fc += wxT("\nUNION\n");
					indent = 0;
					goto gotToken;
				}
				else
				{
					trailingChars = token + wxT(" ") + trailingChars;
					indent += tp->actionBefore;
					if (indent < 0)   indent = 0;
				}
			}
			else
			{
				indent += tp->actionBefore;
				if (indent < 0)   indent = 0;
			}
			if (tp->doBreak)
			{
				fc += wxT("\n") + wxString(' ', (size_t)indent);
				position = indent;
			}
			else
			{
				fc += wxT(" ");
				position += 1;
			}
			fc += tp->replaceKeyword;
			position += wxString(tp->replaceKeyword).Length();

			indent += tp->actionAfter;
			if (indent < 0)   indent = 0;
		}
		else
		{
			fc += token;
			position += token.Length();
		}
		fc += wxT(" ");
		position++;
		if (!trailingChars.IsNull())
		{
			fc += trailingChars + wxT(" ");;
			position += trailingChars.Length() + 1;
		}
	}
	return fc;
}

wxString pgObject::qtDbString(const wxString &str)
{
	// Use the server aware version if possible
	if (GetDatabase() && GetDatabase()->GetConnection())
		return GetDatabase()->GetConnection()->qtDbString(str);
	else
	{
		wxString ret = str;
		ret.Replace(wxT("\\"), wxT("\\\\"));
		ret.Replace(wxT("'"), wxT("''"));
		ret.Append(wxT("'"));
		ret.Prepend(wxT("'"));
		return ret;
	}
}

wxString pgObject::GetDefaultPrivileges(const wxString &strType, const wxString &strSupportedPrivs,
                                        const wxString &strSchema, const wxString &strOrigDefPrivs,
                                        const wxString &strNewDefPrivs, const wxString &strRole)
{
	wxString strDefPrivs, strGrant, strRevoke, strGrantOption, strRevokeGrantOption;
	int privilegeCount = strSupportedPrivs.Length();

	for (int index = 0; index < privilegeCount; index++)
	{
		bool inOrigPriv = false, inNewPriv = false, grantOptInOrigPriv = false, grantOptInNewPriv = false;
		wxChar privChar = strSupportedPrivs.GetChar(index);
		int privAt = strOrigDefPrivs.Find(privChar);
		if (privAt != wxNOT_FOUND)
		{
			inOrigPriv =  true;
			if ((unsigned int)privAt < strOrigDefPrivs.Length() - 1 &&
			        strOrigDefPrivs.GetChar(privAt + 1) == wxT('*'))
				grantOptInOrigPriv = true;
		}

		privAt = strNewDefPrivs.Find(privChar);
		if (privAt != wxNOT_FOUND)
		{
			inNewPriv =  true;
			if ((unsigned int)privAt < strNewDefPrivs.Length() - 1 &&
			        strNewDefPrivs.GetChar(privAt + 1) == wxT('*'))
				grantOptInNewPriv = true;
		}
		if (inOrigPriv || inNewPriv || grantOptInOrigPriv || grantOptInNewPriv)
		{
			wxString strPrivilege = GetPrivilegeName(privChar);
			if (!inOrigPriv && inNewPriv)
			{
				// GRANT PRIVILEGES
				if (!grantOptInNewPriv)
					strGrant             += strPrivilege + wxT(", ");
				// GRANT PRVILEGES WITH GRANT OPTION
				else
					strGrantOption       += strPrivilege + wxT(", ");
			}
			// REVOKE PRIVILEGES
			else if (inOrigPriv && !inNewPriv)
				strRevoke                += strPrivilege + wxT(", ");
			else if (inOrigPriv && inNewPriv)
			{
				// REVOKE ONLY 'WITH GRANT OPTION'
				if(grantOptInOrigPriv && !grantOptInNewPriv)
					strRevokeGrantOption += strPrivilege + wxT(", ");
				// GRANT PRVILEGES WITH GRANT OPTION
				else if (!grantOptInOrigPriv && grantOptInNewPriv)
					strGrantOption       += strPrivilege + wxT(", ");
			}
		}
	}

	bool isModified = false;
	wxString strAltDefPriv;

	if (!strSchema.IsEmpty())
		strAltDefPriv = wxT("ALTER DEFAULT PRIVILEGES IN SCHEMA ") + strSchema;
	else
		strAltDefPriv = wxT("ALTER DEFAULT PRIVILEGES ");

	if (!strRevoke.IsEmpty())
	{
		isModified = true;
		strRevoke = strRevoke.SubString(0, strRevoke.Length() - 3);
		strDefPrivs += strAltDefPriv +
		               wxT("\n    REVOKE ") + strRevoke + wxT(" ON ") + strType +
		               wxT("\n    FROM ") + strRole + wxT(";\n");
	}
	if (!strRevokeGrantOption.IsEmpty())
	{
		isModified = true;
		strRevokeGrantOption = strRevokeGrantOption.SubString(0, strRevokeGrantOption.Length() - 3);
		strDefPrivs += strAltDefPriv +
		               wxT("\n    REVOKE GRANT OPTION FOR ") + strRevokeGrantOption + wxT(" ON ") + strType +
		               wxT("\n    FROM ") + strRole + wxT(";\n");
	}
	if (!strGrant.IsEmpty())
	{
		isModified = true;

		strGrant = strGrant.SubString(0, strGrant.Length() - 3);
		strDefPrivs += strAltDefPriv +
		               wxT("\n    GRANT ") + strGrant + wxT(" ON ") + strType +
		               wxT("\n    TO ") + strRole + wxT(";\n");
	}
	if (!strGrantOption.IsEmpty())
	{
		isModified = true;
		strGrantOption = strGrantOption.SubString(0, strGrantOption.Length() - 3);
		strDefPrivs += strAltDefPriv +
		               wxT("\n    GRANT ") + strGrantOption + wxT(" ON ") + strType +
		               wxT("\n    TO ") + strRole + wxT(" WITH GRANT OPTION;\n");
	}
	if (isModified)
		return strDefPrivs + wxT("\n");

	return wxT("");
}

// Find the user-privileges pair from ACLs
// i.e. {=wDx/user1,postgres=adDxt/user1}
// Remove starting and ending curly braces, before supplying as the input
bool pgObject::findUserPrivs(wxString &strDefPrivs, wxString &strUser, wxString &strPriv)
{
	strUser = strPriv = wxT("");
	if (strDefPrivs.IsEmpty()) return false;

	bool startsWithQuote = false;
	if (strDefPrivs.StartsWith(wxT("\""))) startsWithQuote = true;

	if (strDefPrivs.StartsWith(wxT("\"\"")))
	{
		wxChar currChar;
		int    quoteCount = 0;
		int    index = 0;

		currChar = strDefPrivs.GetChar(index);

		while (true)
		{
			if (currChar == wxT('=') && quoteCount % 2 == 0)
				break;
			strUser += currChar;
			currChar = strDefPrivs.GetChar(++index);
			if (currChar == wxT('"')) quoteCount++;
		}
		strUser = strUser.SubString(2, strUser.Length() - 2);
		strUser.Replace(wxT("\"\""), wxT("\""), true);
		strDefPrivs = strDefPrivs.SubString(index + 1, strDefPrivs.Length());
	}
	else
	{
		/* Remove first quote */
		if (startsWithQuote) strDefPrivs = strDefPrivs.SubString(1, strDefPrivs.Length());

		int equalCharAt = strDefPrivs.Find(wxT('='));

		if (equalCharAt != 0)
			strUser = strDefPrivs.SubString(0, equalCharAt - 1);
		else
			strUser = wxT("public");
		strDefPrivs = strDefPrivs.SubString(equalCharAt + 1, strDefPrivs.Length());
	}

	int slashCharAt = strDefPrivs.Find(wxT('/'));
	strPriv = strDefPrivs.SubString(0, slashCharAt - 1);

	strDefPrivs = strDefPrivs.SubString(strPriv.Length() + 2, strDefPrivs.Length());

	if (!strDefPrivs.StartsWith(wxT("\"")))
	{
		int commaCharAt = strDefPrivs.Find(wxT(','));
		if (commaCharAt == wxNOT_FOUND) strDefPrivs = wxT("");
		else strDefPrivs = strDefPrivs.SubString(commaCharAt + 1, strDefPrivs.Length());
	}
	else
	{
		wxChar currChar;
		int    quoteCount = 0;
		int    index = 0;

		currChar = strDefPrivs.GetChar(index);
		while (true)
		{
			if (currChar == wxT(',') && quoteCount % 2 == 0)
				break;
			currChar = strDefPrivs.GetChar(++index);
			if (currChar == wxT('"')) quoteCount++;
		}
		strDefPrivs = strDefPrivs.SubString(index, strDefPrivs.Length());
	}

	return true;
}

wxString pgObject::GetPrivilegeName(wxChar privilege)
{
	switch(privilege)
	{
		case 'a':
			return wxT("INSERT");
		case 'r':
			return wxT("SELECT");
		case 'w':
			return wxT("UPDATE");
		case 'd':
			return wxT("DELETE");
		case 'D':
			return wxT("TRUNCATE");
		case 'x':
			return wxT("REFERENCES");
		case 't':
			return wxT("TRIGGER");
		case 'm':
			return wxT("MAINTAIN");
		case 'U':
			return wxT("USAGE");
		case 'X':
			return wxT("EXECUTE");
		default:
			return wxT("UNKNOWN");
	}
}

void pgObject::SetWindowPtr(dlgProperty *dlgprop)
{
	dlg = dlgprop;
}
wxString pgObject::GetSqlReCreate(frmMain *form, pgObject *obj)
{
	pgConn *conn = GetConnection();
	if (conn)
	{
		int status = conn->GetStatus();
		if (status == PGCONN_BROKEN || status == PGCONN_BAD)
		{
			form->SetStatusText(_(" Connection broken."));
			return wxT("");;
		}
	}
	wxLogInfo(wxT("Recreate cascade object for %s %s"), GetTypeName().c_str(), GetIdentifier().c_str());

    wxString sql;
        wxString line;
		ctlTree *browser=form->GetBrowser();
		wxString databasePath = form->GetNodePath(obj->GetDatabase()->GetId());
		// получение правила и от него уже зависимости будем раскручивать
        int colcount = 0;
        pgSetIterator set(GetConnection(),
                wxT("WITH RECURSIVE t(lvl,classid,objid,type_child) AS (\n")
                wxT(" select 1,c_classid::regclass,c_objid,type_child from (\n")
                wxT("   select dd.*\n")
                wxT("   ,(pg_identify_object_as_address(dd.classid, dd.objid, dd.objsubid)).type type_child\n")
                wxT("   ,rule_to_view.classid c_classid\n")
                wxT("   ,rule_to_view.objid c_objid\n")
                wxT("    from pg_depend dd, lateral \n")
                wxT("         pg_get_object_address(case when (pg_identify_object_as_address(classid, objid, objsubid)).type='rule' then 'view' \n")
                wxT("         else (pg_identify_object_as_address(classid, objid, objsubid)).type end , \n")
                wxT("                              case when (pg_identify_object_as_address(classid, objid, objsubid)).type='rule' then \n")
                wxT("                                        (pg_identify_object_as_address(classid, objid, objsubid)).object_names[1:2]\n")
                wxT("                                   else (pg_identify_object_as_address(classid, objid, objsubid)).object_names\n")
                wxT("                                   end\n")
                wxT("                                      ,(pg_identify_object_as_address(classid, objid, objsubid)).object_args)\n")
                wxT("     rule_to_view\n")
                wxT("   where dd.deptype='n' and  dd.refobjid=") + GetOidStr() + wxT("\n")
                wxT("   ) d\n")
                wxT("   where d.refobjid<>d.c_objid\n")
                wxT("   group by c_classid,c_objid,type_child\n")
                wxT("UNION ALL\n")
                wxT("select lvl+1,c_classid::regclass classid,c_objid,type_child from (\n")
                wxT("   select dd.*\n")
                wxT("   ,(pg_identify_object_as_address(dd.classid, dd.objid, dd.objsubid)).type type_child\n")
                wxT("   ,rule_to_view.classid c_classid\n")
                wxT("   ,rule_to_view.objid c_objid\n")
                wxT("   ,t.lvl\n")
                wxT("    from pg_depend dd,t, lateral \n")
                wxT("         pg_get_object_address(case when (pg_identify_object_as_address(dd.classid, dd.objid, dd.objsubid)).type='rule' then 'view' \n")
                wxT("         else (pg_identify_object_as_address(dd.classid, dd.objid, dd.objsubid)).type end , \n")
                wxT("                              case when (pg_identify_object_as_address(dd.classid, dd.objid, dd.objsubid)).type='rule' then \n")
                wxT("                                        (pg_identify_object_as_address(dd.classid, dd.objid, dd.objsubid)).object_names[1:2]\n")
                wxT("                                   else (pg_identify_object_as_address(dd.classid, dd.objid, dd.objsubid)).object_names\n")
                wxT("                                   end\n")
                wxT("                                      ,(pg_identify_object_as_address(dd.classid, dd.objid, dd.objsubid)).object_args)\n")
                wxT("     rule_to_view\n")
                wxT("   where dd.deptype='n' and  dd.refobjid=t.objid\n")
                wxT("   ) d\n")
                wxT("   where d.refobjid<>d.c_objid\n")
                wxT("   group by c_classid,c_objid,type_child,lvl\n")
                wxT(") select t.*,coalesce(cc.conname,vv.relname,tt.tgname) objname,coalesce(cc.connamespace::regnamespace,vv.relnamespace::regnamespace)::text sch\n")
                wxT(",coalesce(cc.conrelid::regclass) parent,coalesce(vv.relkind) kind from t\n")
                wxT("left join pg_constraint cc on t.classid='pg_constraint'::regclass and t.objid=cc.oid\n")
                wxT("left join pg_class vv on (t.classid='pg_class'::regclass and t.objid=vv.oid) or (cc.conrelid=vv.oid)\n")
                wxT("left join pg_trigger tt on t.classid='pg_trigger'::regclass and t.objid=tt.oid\n")
                wxT("")
                wxT("")
                wxT("order by 1 desc,2 asc;")
				);
 
        // lvl;classid;objid;child_type;objname;sch;parent
 sql=wxT("-- ");
 pgCollection *database = 0;
 wxTreeItemId dbitem = obj->GetDatabase()->GetId();
			//pgCollection *collect = 0;
			pgCollection *collectSchEl = 0;
			wxCookieType cookie;
			pgCollection *collect= browser->FindCollection(schemaFactory,dbitem);

			wxTreeItemId item = browser->GetFirstChild(dbitem, cookie);
if (1==0) {
			while (item)
			{
				collect = (pgCollection *)browser->GetObject(item);
				if (collect && collect->GetMetaType() == PGM_SCHEMA)
					break;
				else
					collect = 0;
				item = browser->GetNextChild(dbitem, cookie);
			}
}
	wxString findobj=wxT(",");
	wxString dropblock=wxT("-- \n");
	wxString createblock=wxEmptyString;
        while (set.RowsLeft())
        {
			wxString refname = set.GetVal(wxT("objname"));
			wxString parent = set.GetVal(wxT("parent"));
			wxString type = set.GetVal(wxT("type_child"));
			wxString kind = set.GetVal(wxT("kind"));
			wxString sxema = parent.BeforeFirst('.');
			wxString table = parent.AfterFirst('.');
			if (sxema.IsEmpty()) {
				sxema=set.GetVal(wxT("sch"));
				table=refname;
			}
			pgaFactory *depFactory = 0;			
			if (refname.IsEmpty())
				continue;
			if (findobj.Contains(wxT(",")+table+wxT(".")+refname+wxT(","))) continue;
			//pgaCollectionFactory

			item = browser->GetFirstChild(collect->GetId(), cookie);
			while (item)
			{
				collectSchEl = (pgCollection *)browser->GetObject(item);
				if (collectSchEl && collectSchEl->GetMetaType() == PGM_SCHEMA && collectSchEl->GetName()==sxema)
				{
					    pgCollection *coll= 0;
						wxString whatfindt;
						wxString tmpsql;
						wxString fn=collectSchEl->GetFullName();
						if (type.Contains(wxT("rule"))||(type.Contains(wxT("view")))) whatfindt=wxT("...bb");
						if (type.Contains(wxT("table constraint"))) {
							//coll=browser->FindCollection(tableFactory,item);
							whatfindt=wxT("Tables");
						}
							// look for the item starting with the given prefix after it
						
						
						wxTreeItemId id=item;
							while ( id.IsOk() &&
									(  browser->GetItemText(id) == wxT("Dummy") && !browser->GetItemData(id) 
									  || true ))
							{
								pgObject *oo= ((pgObject *) browser->GetItemData(id));
								if (oo) {
									pgaFactory *ff=oo->GetFactory();
									fn=ff->GetTypeName();
								fn=oo->GetName();
								if (fn==wxT("debug**info_history2")) {
									fn=oo->GetFullName();
									pgaFactory *ff=oo->GetFactory();
									fn=ff->GetTypeName();
									if (ff==&pg_partitionFactory) {
										fn=oo->GetFullName();
									};
									//oo->ShowTreeDetail(browser);
								}
								if ((oo->GetFactory()==&viewFactory || oo->GetFactory()==&tableFactory
									|| oo->GetFactory()==&pg_partitionFactory )
									&& fn==table) {
									break;
								}
								}
								wxCookieType cookie;
								if ( browser->HasChildren(id) &&((oo->GetFactory()==&viewFactory || oo->GetFactory()==&tableFactory
									|| oo->GetFactory()==&pg_partitionFactory 
									|| oo->GetFactory()==&schemaFactory 
									||oo->GetFactory()->GetTypeName()==whatfindt
									||oo->GetFactory()->GetTypeName()==wxT("Constraints")
									||oo->GetFactory()->GetTypeName()==wxT("Partitions")
									||oo->GetFactory()->GetTypeName()==wxT("Views")
									)))
								{
									if (oo) oo->ShowTreeDetail(browser);
									id = browser->GetFirstChild(id, cookie);
								}
								else
								{
									// Try a sibling of this or ancestor instead
									wxTreeItemId p = id;
									wxTreeItemId toFind;
									do
									{
										toFind = browser->GetNextSibling(p);
										p = browser->GetItemParent(p);
									}
									while (p.IsOk() && !toFind.IsOk());
									id = toFind;
								}
							}
						if ( id.IsOk() ) {
							// найден нужный элемнт
							pgObject *db=(pgTable *) browser->GetItemData(id);
							if (kind=='r') {
								// "table constraint"
								pgTable *tbl=(pgTable *)  db;
								tbl->ShowTreeDetail(browser);
								pgCollection *cll=tbl->GetConstraintCollection(browser);
								if (cll) {
									fn=cll->GetFullName();
								}
								wxTreeItemId matchItem = browser->FindItem(cll->GetId(),refname);
								if ( matchItem.IsOk() ) {
									pgObject *o=browser->GetObject(matchItem);
									//fn=o->GetName()+wxT(" - type : ")+o->GetTypeName();
									//sql+=fn+wxT("\n");
									tmpsql=browser->GetObject(matchItem)->GetSql(browser);
								}

							}
							else 
							{
								tmpsql=db->GetSql(browser);
								
							}
						} else {
							// не найдет в дереве элемент с указанным именем

						}
						findobj=findobj+table+wxT(".")+refname+wxT(",");
						wxStringTokenizer rowslist(tmpsql, wxT("\n"),wxTOKEN_RET_EMPTY );
						wxString cn;
						wxString distributionColumns;
						wxString createblocktmp=wxEmptyString;
						int i=1;
						while (rowslist.HasMoreTokens())
						{
							cn = rowslist.GetNextToken();
							if (i==1) {
								createblocktmp+=cn+wxT("\n");
								dropblock+=cn+wxT("\n");
								i++;
								continue;
							}
							if (i==3) {
								dropblock+=cn.Mid(3)+wxT("\n");
								i++;
								continue;
							}
							createblocktmp+=cn+wxT("\n");
							i++;
						}
						createblock=createblocktmp+createblock;
//								if (node->IsCollection())
//									owneritem = browser->GetParentObject(node->GetId())->GetId();
//								else
//									owneritem = browser->GetParentObject(browser->GetParentObject(node->GetId())->GetId())->GetId();
				}
				else
					collectSchEl = 0;
				item = browser->GetNextChild(collect, cookie);
			}

        }

		sql+=dropblock+wxT("\n\n\n")+wxT("-- Create depends objects (reverse order))\n")+createblock;
	return sql;
}
