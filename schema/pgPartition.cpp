//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// gpPartition.cpp - Greenplum Table Partition class
//
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"


#include "utils/misc.h"
#include "frm/frmHint.h"
#include "frm/frmMain.h"
#include "frm/frmMaintenance.h"
#include "schema/pgTable.h"
#include "schema/pgPartition.h"
#include "schema/pgColumn.h"
#include "schema/pgIndexConstraint.h"
#include "schema/pgForeignKey.h"
#include "schema/pgCheck.h"
#include "utils/sysSettings.h"
#include "utils/pgfeatures.h"
#include "schema/pgRule.h"
#include "schema/pgTrigger.h"
#include "schema/pgConstraints.h"


// App headers

pgPartition::pgPartition(pgSchema *newSchema, const wxString &newName)
	: pgTable(newSchema, pg_partitionFactory, newName)
{
}

pgPartition::~pgPartition()
{
}

bool pgPartition::CanCreate()
{
	return true;
}

wxMenu *pgPartition::GetNewMenu()
{
	wxMenu *menu = pgObject::GetNewMenu();
	if (schema->GetCreatePrivilege())
	{

	}
	return menu;
}
/*
wxString gpPartition::GetCreate()
{
    wxString sql;

   // sql = GetQuotedIdentifier() + wxT(" ")
    //    + GetTypeName().Upper() + GetDefinition();
    sql = wxT("Not implemented yet..sorry");
    return sql;
};
*/

wxString pgPartition::GetSql(ctlTree *browser)
{
	wxString sql;
	sql = wxT("-- ");
	sql += _("Note: This DDL is a representation of how the partition might look as a table.");
	sql += wxT("\n\n");

	sql += pgTable::GetSql(browser);
	return sql;
}
ctlTree *tmp_br;
pgObject *pgPartition::Refresh(ctlTree *browser, const wxTreeItemId item)
{
	pgPartition *partition = 0;
	pgCollection *coll = browser->GetParentCollection(item);
	tmp_br=browser;
	if (coll)
		partition = (pgPartition *)pg_partitionFactory.CreateObjects(coll, 0, wxT("\n   AND rel.oid=") + GetOidStr());

	return partition;
}

///////////////////////////////////////////////////////////

pgPartitionCollection::pgPartitionCollection(pgaFactory *factory, pgPartition *_table)
	: pgTableCollection(factory, _table->GetSchema())
{
	iSetOid(_table->GetOid());
}
void pgPartitionCollection::ShowStatistics(frmMain *form, ctlListView *statistics)
{
	ShowStatisticsTables(form, statistics, this);
	return;
	wxLogInfo(wxT("Displaying statistics for tables on %s"), GetSchema()->GetIdentifier().c_str());

	bool hasSize = GetConnection()->HasFeature(FEATURE_SIZE);

	// Add the statistics view columns
	statistics->ClearAll();
	statistics->AddColumn(_("Table Name"));
	if (hasSize)
		statistics->AddColumn(_("Size"));
	if (GetConnection()->GetIsPgProEnt()) statistics->AddColumn(_("CFS %"));
	statistics->AddColumn(_("Live tuples"));
	statistics->AddColumn(_("Tuples inserted"));
	statistics->AddColumn(_("Tuples updated"));
	statistics->AddColumn(_("Tuples deleted"));
	if (GetConnection()->BackendMinimumVersion(8, 3))
	{
		statistics->AddColumn(_("Tuples HOT updated"));
		statistics->AddColumn(_("Dead tuples"));
	}
	if (GetConnection()->BackendMinimumVersion(8, 2))
	{
		statistics->AddColumn(_("Last vacuum"));
		statistics->AddColumn(_("Last autovacuum"));
		statistics->AddColumn(_("Last analyze"));
		statistics->AddColumn(_("Last autoanalyze"));
	}
	if (GetConnection()->BackendMinimumVersion(9, 1))
	{
		statistics->AddColumn(_("Vacuum counter"));
		statistics->AddColumn(_("Autovacuum counter"));
		statistics->AddColumn(_("Analyze counter"));
		statistics->AddColumn(_("Autoanalyze counter"));
	}

	wxString sql = wxT("SELECT st.relname, n_tup_ins, n_tup_upd, n_tup_del");
	if (GetConnection()->BackendMinimumVersion(8, 3))
		sql += wxT(", n_tup_hot_upd, n_live_tup, n_dead_tup");
	if (GetConnection()->BackendMinimumVersion(8, 2))
		sql += wxT(", last_vacuum, last_autovacuum, last_analyze, last_autoanalyze");
	if (GetConnection()->BackendMinimumVersion(9, 1))
		sql += wxT(", vacuum_count, autovacuum_count, analyze_count, autoanalyze_count");
	if (hasSize)
		sql += wxT(", pg_size_pretty(pg_relation_size(st.relid)")
		       wxT(" + CASE WHEN cl.reltoastrelid = 0 THEN 0 ELSE pg_relation_size(cl.reltoastrelid) + COALESCE((SELECT SUM(pg_relation_size(indexrelid)) FROM pg_index WHERE indrelid=cl.reltoastrelid)::int8, 0) END")
		       wxT(" + COALESCE((SELECT SUM(pg_relation_size(indexrelid)) FROM pg_index WHERE indrelid=st.relid)::int8, 0)) AS size");
	if (GetConnection()->GetIsPgProEnt()) sql += wxT(",left((cfs_fragmentation(cl.oid)*100)::text,5)::text AS cfs_ratio");
	sql += wxT("\n  FROM pg_stat_all_tables st")
	       wxT("  JOIN pg_class cl on cl.oid=st.relid\n")
		   wxT("  JOIN pg_inherits i ON (cl.oid = i.inhrelid)")
		   wxT(" WHERE schemaname = ") + qtDbString(GetSchema()->GetName())+ wxT(" AND i.inhparent = ")+GetOidStr()
	       +  wxT("\n ORDER BY relname");

	pgSet *stats = GetDatabase()->ExecuteSet(sql);

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
			if (GetConnection()->GetIsPgProEnt()) statistics->SetItem(pos, i++, stats->GetVal(wxT("cfs_ratio")));
			statistics->SetItem(pos, i++, stats->GetVal(wxT("n_tup_ins")));
			statistics->SetItem(pos, i++, stats->GetVal(wxT("n_tup_upd")));
			statistics->SetItem(pos, i++, stats->GetVal(wxT("n_tup_del")));
			if (GetConnection()->BackendMinimumVersion(8, 3))
			{
				statistics->SetItem(pos, i++, stats->GetVal(wxT("n_tup_hot_upd")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("n_live_tup")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("n_dead_tup")));
			}
			if (GetConnection()->BackendMinimumVersion(8, 2))
			{
				statistics->SetItem(pos, i++, stats->GetVal(wxT("last_vacuum")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("last_autovacuum")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("last_analyze")));
				statistics->SetItem(pos, i++, stats->GetVal(wxT("last_autoanalyze")));
				if (stats->GetVal(wxT("last_analyze")).IsEmpty() && stats->GetVal(wxT("last_autoanalyze")).IsEmpty())
					statistics->SetItemBackgroundColour(pos, wxColour(wxT("#FF8028")));

			}
			if (GetConnection()->BackendMinimumVersion(9, 1))
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


pgObject *pgPartitionFactory::CreateObjects(pgCollection *coll, ctlTree *browser, const wxString &restriction)
{
	pgPartitionCollection *collection = (pgPartitionCollection *)coll;
	wxString query;
	pgPartition *table = 0;
	ctlTree *br=browser;
	if (!br) br=tmp_br;
	// Greenplum returns reltuples and relpages as tuples per segmentDB and pages per segmentDB,
	// so we need to multiply them by the number of segmentDBs to get reasonable values.
	long gp_segments = 1;

	//query = wxT("SELECT count(*) AS gp_segments from pg_catalog.gp_configuration where definedprimary = 't' and content >= 0");
	//gp_segments = StrToLong(collection->GetDatabase()->ExecuteScalar(query));
	//if (gp_segments <= 1)
	//	gp_segments = 1;
	

	pgSet *tables;

		query = wxT("SELECT rel.oid, rel.relname,rel.relnamespace::regnamespace AS nspace, rel.reltablespace AS spcoid, spc.spcname, pg_get_userbyid(rel.relowner) AS relowner, rel.relacl, ")
		        wxT("rel.relhassubclass, rel.reltuples, des.description, con.conname, con.conkey,\n")
		        wxT("       EXISTS(select 1 FROM pg_trigger\n")
		        wxT("                       JOIN pg_proc pt ON pt.oid=tgfoid AND pt.proname='logtrigger'\n")
		        wxT("                       JOIN pg_proc pc ON pc.pronamespace=pt.pronamespace AND pc.proname='slonyversion'\n")
		        wxT("                     WHERE tgrelid=rel.oid) AS isrepl,\n");
		query += wxT("       (select count(*) FROM pg_trigger\n")
		         wxT("                     WHERE tgrelid=rel.oid AND tgisinternal = FALSE) AS triggercount\n");
		query += wxT(", rel.relpersistence \n");
		if (collection->GetDatabase()->connection()->GetIsPgProEnt()) query += wxT(",left((cfs_fragmentation(rel.oid)*100)::text,5)::text AS cfs_ratio\n");
			else query += wxT(",null::text AS cfs_ratio\n");
		if (collection->GetConnection()->HasFeature(FEATURE_TRACK_COMMIT_TS))
			query += wxT(", pg_xact_commit_timestamp(typ2.xmin) create_ts\n");

		query += wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'fillfactor=([0-9]*)') AS fillfactor \n");
		query += wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'autovacuum_enabled=([a-z|0-9]*)') AS autovacuum_enabled \n")
			         wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'autovacuum_vacuum_threshold=([0-9]*)') AS autovacuum_vacuum_threshold \n")
			         wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'autovacuum_vacuum_scale_factor=([0-9]*[.][0-9]*)') AS autovacuum_vacuum_scale_factor \n")
			         wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'autovacuum_analyze_threshold=([0-9]*)') AS autovacuum_analyze_threshold \n")
			         wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'autovacuum_analyze_scale_factor=([0-9]*[.][0-9]*)') AS autovacuum_analyze_scale_factor \n")
			         wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'autovacuum_vacuum_cost_delay=([0-9]*)') AS autovacuum_vacuum_cost_delay \n")
			         wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'autovacuum_vacuum_cost_limit=([0-9]*)') AS autovacuum_vacuum_cost_limit \n")
			         wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'autovacuum_freeze_min_age=([0-9]*)') AS autovacuum_freeze_min_age \n")
			         wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'autovacuum_freeze_max_age=([0-9]*)') AS autovacuum_freeze_max_age \n")
			         wxT(", substring(array_to_string(rel.reloptions, ',') FROM 'autovacuum_freeze_table_age=([0-9]*)') AS autovacuum_freeze_table_age \n")
			         wxT(", substring(array_to_string(tst.reloptions, ',') FROM 'autovacuum_enabled=([a-z|0-9]*)') AS toast_autovacuum_enabled \n")
			         wxT(", substring(array_to_string(tst.reloptions, ',') FROM 'autovacuum_vacuum_threshold=([0-9]*)') AS toast_autovacuum_vacuum_threshold \n")
			         wxT(", substring(array_to_string(tst.reloptions, ',') FROM 'autovacuum_vacuum_scale_factor=([0-9]*[.][0-9]*)') AS toast_autovacuum_vacuum_scale_factor \n")
			         wxT(", substring(array_to_string(tst.reloptions, ',') FROM 'autovacuum_analyze_threshold=([0-9]*)') AS toast_autovacuum_analyze_threshold \n")
			         wxT(", substring(array_to_string(tst.reloptions, ',') FROM 'autovacuum_analyze_scale_factor=([0-9]*[.][0-9]*)') AS toast_autovacuum_analyze_scale_factor \n")
			         wxT(", substring(array_to_string(tst.reloptions, ',') FROM 'autovacuum_vacuum_cost_delay=([0-9]*)') AS toast_autovacuum_vacuum_cost_delay \n")
			         wxT(", substring(array_to_string(tst.reloptions, ',') FROM 'autovacuum_vacuum_cost_limit=([0-9]*)') AS toast_autovacuum_vacuum_cost_limit \n")
			         wxT(", substring(array_to_string(tst.reloptions, ',') FROM 'autovacuum_freeze_min_age=([0-9]*)') AS toast_autovacuum_freeze_min_age \n")
			         wxT(", substring(array_to_string(tst.reloptions, ',') FROM 'autovacuum_freeze_max_age=([0-9]*)') AS toast_autovacuum_freeze_max_age \n")
			         wxT(", substring(array_to_string(tst.reloptions, ',') FROM 'autovacuum_freeze_table_age=([0-9]*)') AS toast_autovacuum_freeze_table_age \n")
			         wxT(", rel.reloptions AS reloptions, tst.reloptions AS toast_reloptions \n")
			         wxT(", (CASE WHEN rel.reltoastrelid = 0 THEN false ELSE true END) AS hastoasttable\n");
			query += wxT(", rel.reloftype, typ.typname\n");
			query += wxT(",\n(SELECT array_agg(label) FROM pg_seclabels sl1 WHERE sl1.objoid=rel.oid AND sl1.objsubid=0) AS labels");
			query += wxT(",\n(SELECT array_agg(provider) FROM pg_seclabels sl2 WHERE sl2.objoid=rel.oid AND sl2.objsubid=0) AS providers");
			query += wxT(",case when lk.relation=rel.oid then null else pg_get_partkeydef(rel.oid) end \n AS partkeydef");
			query += wxT(",case when lk.relation=rel.oid then 'AccessExclusiveLock' else pg_get_expr(rel.relpartbound, rel.oid) end \n AS partexp");
			query += wxT(",(select count(*)from pg_inherits ii where ii.inhparent=rel.oid)>0 AS ispartitioned");
			
			if (collection->GetDatabase()->BackendMinimumVersion(10, 0))
			{
				//query += wxT(",\n pg_get_statisticsobjdef(stat_ext.oid) AS stat_stmt");
				//query += wxT(",\nCASE WHEN stat_ext.stxowner<>rel.relowner then 'ALTER STATISTICS '||substring(pg_get_statisticsobjdef(stat_ext.oid) from 'ICS (.+?)\\s\\(')||' OWNER TO '||stat_ext.stxowner::regrole else null end AS alter_stmt");
				query += ",(select string_agg(pg_get_statisticsobjdef(stat_ext.oid)||CASE WHEN stat_ext.stxowner<>rl.relowner then E';\\nALTER STATISTICS '||substring(pg_get_statisticsobjdef(stat_ext.oid) from 'ICS (.+?)\\s')||' OWNER TO '||stat_ext.stxowner::regrole else '' end"
					;
				if (collection->GetDatabase()->BackendMinimumVersion(13, 0)) {
					query += "||CASE WHEN stat_ext.stxstattarget<>-1  then E';\\nALTER STATISTICS '||substring(pg_get_statisticsobjdef(stat_ext.oid) from 'ICS (.+?)\\s')||' SET STATISTICS '||stat_ext.stxstattarget else '' end";
				}
				query += ",E';\\n' order by stat_ext.stxrelid) stat_stmt from pg_class rl join  pg_statistic_ext stat_ext on rl.oid=stat_ext.stxrelid where stat_ext.stxrelid=rel.oid) stat_stmt";
			}

		query += wxT("  FROM pg_class rel\n")
	             wxT("  JOIN pg_inherits i ON (rel.oid = i.inhrelid) \n")
			     wxT("  LEFT JOIN  pg_locks lk ON locktype='relation' and granted=true and mode='AccessExclusiveLock' and relation=rel.oid\n")
		         wxT("  LEFT OUTER JOIN pg_tablespace spc on spc.oid=rel.reltablespace\n")
		         wxT("  LEFT OUTER JOIN pg_description des ON (des.objoid=rel.oid AND des.objsubid=0 AND des.classoid='pg_class'::regclass)\n")
		         wxT("  LEFT OUTER JOIN pg_constraint con ON con.conrelid=rel.oid AND con.contype='p'\n");
		query += wxT("  LEFT OUTER JOIN pg_class tst ON tst.oid = rel.reltoastrelid\n");
			query += wxT("LEFT JOIN pg_type typ ON rel.reloftype=typ.oid\n");
		if (collection->GetConnection()->HasFeature(FEATURE_TRACK_COMMIT_TS))
				query += wxT("LEFT JOIN pg_type typ2 ON rel.oid=typ2.typrelid\n");

		query += wxT(" WHERE rel.relkind IN ('r','s','t','p')\n");
		// show partitions in other schema
		//query += wxT(" AND rel.relnamespace = ") + collection->GetSchema()->GetOidStr() + wxT("\n");
		query += wxT("--AND (not (rel.relkind='r' and rel.relpartbound IS NOT NULL))\n ")
				 wxT(" AND i.inhparent = ") + collection->GetOidStr() + wxT("\n");

		query += restriction +
		         wxT(" ORDER BY rel.relname");


	tables = collection->GetDatabase()->ExecuteSet(query);
	if (tables)
	{
		wxString currns=collection->GetSchema()->GetName();
//		wxArrayObject schemaArr;
		std::vector<pgSchema*> schemaArr;
		while (!tables->Eof())
		{
			wxString ns=tables->GetVal(wxT("nspace"));
			if (currns!=ns) {
				// find pgSchema object in tree
			  if (!(schemaArr.size()>0)) {
				wxTreeItemId currentItem = collection->GetSchema()->GetId();
				pgCollection *collsch;
				wxTreeItemId scItem;
				if (currentItem) scItem = br->GetItemParent(currentItem);
				    collsch = (pgCollection *) br->GetObject(scItem);
					if (!collsch) continue;
					treeObjectIterator schemaIterator(br, collsch);
					pgSchema *s;
					while ((s = (pgSchema *)schemaIterator.GetNextObject()) != 0)
						schemaArr.push_back(s);
			  }
			  table = 0;
			  for (size_t j=0;j<schemaArr.size();j++)	{
				  if (ns==schemaArr[j]->GetName()) {
					  table = new pgPartition(schemaArr[j], tables->GetVal(wxT("relname")));
					  break;
				  }
			  }
			  if (!table) continue;
			} else 	table = new pgPartition(collection->GetSchema(), tables->GetVal(wxT("relname")));

			table->iSetOid(tables->GetOid(wxT("oid")));
			table->iSetOwner(tables->GetVal(wxT("relowner")));
			table->iSetAcl(tables->GetVal(wxT("relacl")));
			table->iSetRatio(tables->GetVal(wxT("cfs_ratio")));
			if (collection->GetConnection()->HasFeature(FEATURE_TRACK_COMMIT_TS)) {
				table->iSetCreateTableTS(tables->GetVal(wxT("create_ts")));
			}
			if (tables->GetOid(wxT("spcoid")) == 0)
					table->iSetTablespaceOid(collection->GetDatabase()->GetTablespaceOid());
				else
					table->iSetTablespaceOid(tables->GetOid(wxT("spcoid")));

				if (tables->GetVal(wxT("spcname")) == wxEmptyString)
					table->iSetTablespace(collection->GetDatabase()->GetTablespace());
				else
					table->iSetTablespace(tables->GetVal(wxT("spcname")));
			table->iSetOfTypeOid(tables->GetOid(wxT("reloftype")));
			table->iSetOfType(tables->GetVal(wxT("typname")));
			table->iSetComment(tables->GetVal(wxT("description")));
				table->iSetUnlogged(tables->GetVal(wxT("relpersistence")) == wxT("u"));
			table->iSetHasOids(false);
			table->iSetEstimatedRows(tables->GetDouble(wxT("reltuples")) * gp_segments);
				table->iSetFillFactor(tables->GetVal(wxT("fillfactor")));
			if (collection->GetConnection()->BackendMinimumVersion(8, 4))
			{
				table->iSetRelOptions(tables->GetVal(wxT("reloptions")));
				if (table->GetCustomAutoVacuumEnabled())
				{
					if (tables->GetVal(wxT("autovacuum_enabled")).IsEmpty())
						table->iSetAutoVacuumEnabled(2);
					else if (tables->GetBool(wxT("autovacuum_enabled")))
						table->iSetAutoVacuumEnabled(1);
					else
						table->iSetAutoVacuumEnabled(0);
					table->iSetAutoVacuumVacuumThreshold(tables->GetVal(wxT("autovacuum_vacuum_threshold")));
					table->iSetAutoVacuumVacuumScaleFactor(tables->GetVal(wxT("autovacuum_vacuum_scale_factor")));
					table->iSetAutoVacuumAnalyzeThreshold(tables->GetVal(wxT("autovacuum_analyze_threshold")));
					table->iSetAutoVacuumAnalyzeScaleFactor(tables->GetVal(wxT("autovacuum_analyze_scale_factor")));
					table->iSetAutoVacuumVacuumCostDelay(tables->GetVal(wxT("autovacuum_vacuum_cost_delay")));
					table->iSetAutoVacuumVacuumCostLimit(tables->GetVal(wxT("autovacuum_vacuum_cost_limit")));
					table->iSetAutoVacuumFreezeMinAge(tables->GetVal(wxT("autovacuum_freeze_min_age")));
					table->iSetAutoVacuumFreezeMaxAge(tables->GetVal(wxT("autovacuum_freeze_max_age")));
					table->iSetAutoVacuumFreezeTableAge(tables->GetVal(wxT("autovacuum_freeze_table_age")));
				}
				table->iSetHasToastTable(tables->GetBool(wxT("hastoasttable")));
				if (table->GetHasToastTable())
				{
					table->iSetToastRelOptions(tables->GetVal(wxT("toast_reloptions")));

					if (table->GetToastCustomAutoVacuumEnabled())
					{
						if (tables->GetVal(wxT("toast_autovacuum_enabled")).IsEmpty())
							table->iSetToastAutoVacuumEnabled(2);
						else if (tables->GetBool(wxT("toast_autovacuum_enabled")))
							table->iSetToastAutoVacuumEnabled(1);
						else
							table->iSetToastAutoVacuumEnabled(0);

						table->iSetToastAutoVacuumVacuumThreshold(tables->GetVal(wxT("toast_autovacuum_vacuum_threshold")));
						table->iSetToastAutoVacuumVacuumScaleFactor(tables->GetVal(wxT("toast_autovacuum_vacuum_scale_factor")));
						table->iSetToastAutoVacuumVacuumCostDelay(tables->GetVal(wxT("toast_autovacuum_vacuum_cost_delay")));
						table->iSetToastAutoVacuumVacuumCostLimit(tables->GetVal(wxT("toast_autovacuum_vacuum_cost_limit")));
						table->iSetToastAutoVacuumFreezeMinAge(tables->GetVal(wxT("toast_autovacuum_freeze_min_age")));
						table->iSetToastAutoVacuumFreezeMaxAge(tables->GetVal(wxT("toast_autovacuum_freeze_max_age")));
						table->iSetToastAutoVacuumFreezeTableAge(tables->GetVal(wxT("toast_autovacuum_freeze_table_age")));
					}
				}
			}
			table->iSetHasSubclass(tables->GetBool(wxT("relhassubclass")));
			table->iSetPrimaryKeyName(tables->GetVal(wxT("conname")));
			table->iSetIsReplicated(tables->GetBool(wxT("isrepl")));
			table->iSetTriggerCount(tables->GetLong(wxT("triggercount")));
			wxString cn = tables->GetVal(wxT("conkey"));
			cn = cn.Mid(1, cn.Length() - 2);
			table->iSetPrimaryKeyColNumbers(cn);
			table->iSetPartitionDef(wxT(""));
			table->iSetIsPartitioned(false);
			if (collection->GetConnection()->BackendMinimumVersion(10, 0))
			{
				table->iSetPartKeyDef(tables->GetVal(wxT("partkeydef")));
				table->iSetPartExp(tables->GetVal(wxT("partexp")));
				table->iSetIsPartitioned(tables->GetBool(wxT("ispartitioned")));
				wxString st = tables->GetVal(wxT("stat_stmt"));
				//wxString at = tables->GetVal(wxT("alter_stmt"));
				if (!st.IsEmpty()) if ((st.Right(1) != ";")) st = st + wxT(";\n");
				table->iSetStatExt(st);

			}
			if (collection->GetConnection()->GetIsGreenplum())
			{
				Oid lo = tables->GetOid(wxT("localoid"));
				wxString db = tables->GetVal(wxT("attrnums"));
				db = db.Mid(1, db.Length() - 2);
				table->iSetDistributionColNumbers(db);
				if (lo > 0 && db.Length() == 0)
					table->iSetDistributionIsRandom();
				table->iSetAppendOnly(tables->GetVal(wxT("appendonly")));
				table->iSetCompressLevel(tables->GetVal(wxT("compresslevel")));
				table->iSetOrientation(tables->GetVal(wxT("orientation")));
				table->iSetCompressType(tables->GetVal(wxT("compresstype")));
				table->iSetBlocksize(tables->GetVal(wxT("blocksize")));
				table->iSetChecksum(tables->GetVal(wxT("checksum")));


				if (collection->GetConnection()->BackendMinimumVersion(8, 2, 9))
				{
					table->iSetIsPartitioned(tables->GetBool(wxT("ispartitioned")));
				}

			}

			if (collection->GetConnection()->BackendMinimumVersion(9, 1))
			{
				table->iSetProviders(tables->GetVal(wxT("providers")));
				table->iSetLabels(tables->GetVal(wxT("labels")));
			}

			if (browser)
			{
				browser->AppendObject(collection, table);
				tables->MoveNext();
			}
			else
				break;
		}
		if (schemaArr.size()>0) schemaArr.clear();
		delete tables;
	}
	return table;
}

void pgPartitionFactory::AppendMenu(wxMenu *menu)
{
}

#include "images/table.pngc"
#include "images/table-sm.pngc"
#include "images/tables.pngc"

pgPartitionFactory::pgPartitionFactory()
	: pgTableObjFactory(__("Partition"), __("New Partition..."), __("Create a new Partition."), table_png_img, table_sm_png_img)
{
	metaType = PGM_TABLE;
	typeName = wxT("TABLE");
}

pgCollection *pgPartitionFactory::CreateCollection(pgObject *obj)
{
	return new pgPartitionCollection(GetCollectionFactory(), (pgPartition *)obj );
}

pgPartitionFactory pg_partitionFactory;
static pgaCollectionFactory cpf(&pg_partitionFactory, __("Partitions"), tables_png_img);


