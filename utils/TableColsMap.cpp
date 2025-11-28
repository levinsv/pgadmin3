#include "pgAdmin3.h"
#include "utils/TableColsMap.h"
Table::Table(const wxString &tablename) {
    make_name(tablename);
}
void Table::make_name(const wxString& tablename) {
    if (tablename.length() > 0) {
        size_t pospoint = tablename.Find('.');
        size_t pos = 0;
        std::vector<wxString> names;
        while (pos < tablename.length()) {
            size_t posstart = tablename.find('"',pos);
            size_t posend = -1;
            if (posstart>=0) posend=tablename.find('"', posstart + 1);
            wxString tmp2;
            if (pospoint != -1 && pospoint < posstart) posstart = -1;
            if (posstart != -1 && posend != -1)  {
                tmp2 = tablename.substr(pos + 1, posend - pos - 1);

                names.push_back(tmp2);
                pos = posend+1;
                if (pos == pospoint) {
                    //"schema".table || "schema"."table"
                    pos++;
                    pospoint = -1;
                    continue;
                }
                else {
                    // "schema"
                    continue;
                }
            }
            else {
                if (pospoint != -1) {
                    // scema.table || scema."table"
                    tmp2 = tablename.substr(pos, pospoint - pos);
                    names.push_back(tmp2.MakeLower());
                    pos = pospoint + 1;
                    pospoint = -1;
                }
                else {
                    tmp2 = tablename.substr(pos);
                    names.push_back(tmp2.MakeLower());
                    pos = tablename.length();
                }
            }
            
        }
        if (names.size() > 0) {
            int pp = 0;
            if (names.size() == 2) schema = names[pp++];
            name = names[pp];
        }
    }
}
void TableColsMap::Clear() {
    for (auto& e : oids) {
        Table* t = e.second;
        delete t;
    }
    oids.clear();
}
void TableColsMap::checkDBconn(pgConn* dbconn) {
    if (db == NULL || dbconn != db) {
        Clear();
        db = dbconn;
    }
    //alias.clear();
}
/// <summary>
///  map заполняется проброшенными через вьюхи связанными таблицами и колонками
/// </summary>
/// <param name="reltab"></param>
/// <param name="alias"></param>
/// <param name="map"></param>
void TableColsMap::BuildMapTableColumnsToSQLexp(Table *reltab, const wxString &alias, std::map<tab_col_struct, wxString> &map) {
    if (reltab != NULL) {
        Table tmp = *reltab;
        for (int c = 0; c < reltab->GetColsCount(); c++) {
            Table* rel;
            if (tmp.GetType() == 'v') rel = tmp[c].relTable; else rel = reltab;
            if (rel == NULL) continue;
            int ncol = tmp[c].relcol;
            if (ncol < 1) continue;
            tab_col_struct fnd = { rel,ncol };
            wxString sqlname = qtIdent(tmp[c].name);
            if (!alias.IsEmpty()) sqlname = alias + "." + sqlname;
            map.insert({fnd,sqlname});
        }
    }
    return;
}
TableColsMap::~TableColsMap()
{ Clear(); }
wxString TableColsMap::AddTableList(pgConn* dbconn, const wxArrayString& tables, const wxArrayString& alias, const TableColsMap::Flag flag, const wxString &leftexp) {
    checkDBconn(dbconn);
    Table right;
    wxString dbl = "";
    wxString onlytab = "";
    for (int i = tables.GetCount() - 1; i >= 0; i--) {
        Table t(tables[i]);
        if (i == 0) right = t;
        wxString tn = t.GetName();
        wxString sc = t.GetSchema();
        if (!sc.IsEmpty()) dbl = dbl + ((dbl.length() > 0 ? "," : "") + wxString::Format("('%s','%s')", sc, tn));
        else onlytab = onlytab + ((onlytab.length() > 0 ? "," : "") + wxString::Format("'%s'", tn));

        t.SetAlias(alias[i]);
        //this->alias.emplace(curr, );
    }
    if (!dbl.IsEmpty()) dbl = " (cv.relnamespace::regnamespace::text,cv.relname) in (" + dbl + ")"; else dbl = " false";
    if (!onlytab.IsEmpty()) onlytab = " or (cv.relname) in (" + onlytab + ")";
    whereexp = dbl + onlytab;
    getDatabaseViews();
    getDatabaseTables(flag);
    MapViewColToRelCol(flag);
    wxString rezstr;

    if (CHKFLAG(flag, TableColsMap::Flag::SEQUENCE_LIST_TABLE) || true) {
        std::vector<std::map<tab_col_struct, wxString>> all_maps;
        std::vector<Table*> all_tables;
        for (int i = tables.GetCount()-1; i >=0; i--) {
            Table t(tables[i]);
            wxString tn = t.GetName();
            wxString sc = t.GetSchema();
            Table* r = GetTableByName(t.GetSchema(), t.GetName());
            if (r != NULL) {
                r->SetAlias(alias[i]);
                std::map<tab_col_struct, wxString> maplefttables;
                BuildMapTableColumnsToSQLexp(r, alias[i], maplefttables);
                all_maps.push_back(maplefttables);
                all_tables.push_back(r);
                if (all_tables.size() > 100) break;
            }
        }
        wxArrayString aar;
        wxString s;
        bool isadd = false;
        bool isexp = !leftexp.IsEmpty();
        std::unordered_set<wxString> full;
        for (int i = 0; i < all_maps.size(); i++) {
            //Table* r = all_tables[i];
            int position = i + 1;
            if (CHKFLAG(flag, TableColsMap::Flag::ALL_LEFT_TO_RIGHT) && ((i+1)!= all_maps.size())) position = all_maps.size() - 1; // only right table
            for (int j = position; j < all_maps.size(); j++) {
                wxString tmp= MapTableToTable(all_maps[i], all_maps[j],flag,leftexp);
                if (tmp.length() > 0) {
                    if (tmp.Find('\t') == -1) {
                        full.insert(tmp);
                        if (full.size() > 1 && !isexp) isadd = true;
                        //if (!s.IsEmpty()) { s += " AND"; isadd = true; }
                        //s += tmp;
                        aar.Add(tmp);
                    }
                    else {
                        wxArrayString tmp2 = wxSplit(tmp,'\t');
                        for (int k = 0; k < tmp2.GetCount();k++) {
                            aar.Add(tmp2[k]);
                        }
                    }
                    
                }
            }
        }
        //if (!s.IsEmpty() && isadd) rezstr = s + '\t';
        wxString space;
        if (CHKFLAG(flag, TableColsMap::Flag::NOT_ADD_FIRST_SPACE)) space = " ";
        if (full.size() > 1 && isadd) {
            for (auto &s1: full) {
                if (!rezstr.IsEmpty()) { rezstr += " AND"+space;}
                rezstr += s1;
            }
            if (rezstr.length() > 0 && aar.GetCount()>0) rezstr += '\t';
        }
        rezstr = rezstr+wxJoin(aar, '\t');
        return rezstr;
    }
    // build all table.colum for join tables
    std::map<tab_col_struct,wxString> maplefttables;
    for (int i = tables.GetCount() - 1; i > 0; i--) {
        Table t(tables[i]);
        wxString tn = t.GetName();
        wxString sc = t.GetSchema();
        Table* r = GetTableByName(t.GetSchema(), t.GetName());
        if (r != NULL) {
            r->SetAlias(alias[i]);
            BuildMapTableColumnsToSQLexp(r,alias[i], maplefttables);
        }
    }
    Table* r = GetTableByName(right.GetSchema(),right.GetName());
    if (r != NULL) {
        Table tmp = *r;
        std::map<tab_col_struct, wxString> maprighttable;
        BuildMapTableColumnsToSQLexp(r, alias[0], maprighttable);
        rezstr=MapTableToTable(maplefttables, maprighttable,flag,leftexp);
    }
    return rezstr;
}
wxString TableColsMap::MapTableToTable(std::map<tab_col_struct, wxString>& maplefttables, std::map<tab_col_struct, wxString>& maprighttable, const TableColsMap::Flag flag, const wxString& leftexp) {
    std::map<tab_tab_struct, wxString> sqlvariants;
    bool isexp = !leftexp.IsEmpty();
    for (auto& e : maprighttable) {
        tab_col_struct fnd = e.first;
        //find in all FK key
        for (auto itr = all_fk_index.find(fnd); itr != all_fk_index.end(); itr++) {
            if (itr->first.t != fnd.t || itr->first.column != fnd.column) continue; // find not work
            tab_col_struct fk = itr->second;
            tab_tab_struct tt = { fnd.t,fk.t };
            if (auto it2 = maplefttables.find(fk); it2 != maplefttables.end()) {
                wxString sqlcol2 = it2->second;
                wxString sqlcol1 = e.second;
                wxString ttt;
                if (isexp) {
                    // only equal leftexp
                    if (sqlcol2 == leftexp) {
                        ttt = sqlcol1;
                    }
                    else if (sqlcol1 == leftexp) {
                        ttt = sqlcol2;
                    }
                    else continue;
                } else 
                    ttt = sqlcol2 + " = " + sqlcol1;
                
                if (auto it3 = sqlvariants.find(tt); it3 != sqlvariants.end()) {
                    if (it3->second.Find(ttt) >= 0 || isexp) {
                        ttt = it3->second;
                    }
                    else
                        ttt = it3->second + " AND " + ttt;
                    sqlvariants.erase(tt);
                }
                sqlvariants.insert({ tt,ttt });
            }

        }
    }
    wxArrayString rez;
    wxString space;
    if (!CHKFLAG(flag, TableColsMap::Flag::NOT_ADD_FIRST_SPACE)) space = " ";
    for (auto& e : sqlvariants) {
        rez.Add(space + e.second);
    }

    return wxJoin(rez, '\t');

}
Table* TableColsMap::GetTablebyOID(const wxString oid) {
    if (auto it2 = oids.find(oid); it2 != oids.end()) {
        return it2->second;
    }
    return NULL;
}
/// <summary>
///  Рекурсивный спуск по view до таблицы и возврат таблицы и реальной колонки
/// </summary>
/// <param name="oid"></param>
/// <param name="ncolview"></param>
/// <param name="outrelcol"></param>
/// <returns></returns>
Table* TableColsMap::GetRelTableForViewCol(const wxString oid, int ncolview, int& outrelcol) {
    Table *tt = GetTablebyOID(oid);
    if (tt != NULL) {
        Table t = *tt;
        if (tt->GetType() == 'v') {
            wxString toid = t[ncolview - 1].linkOid;
            if (toid == '0') return NULL;
            int ncol = t[ncolview - 1].linknumcol;
            if (ncol < 1) return NULL;
            return GetRelTableForViewCol(toid,ncol,outrelcol);
        }
        else {
            outrelcol = ncolview;
            return tt;
        }
    }
    return NULL;
}
void TableColsMap::MapViewColToRelCol(const TableColsMap::Flag flag) {
    for (auto& e : oids) {
        Table* t = e.second;
        
         {
            Table tmp = *t;
            for (int c = 0; c < t->GetColsCount(); c++) {
                int linkcol = tmp[c].linknumcol;
                Cols cl = tmp[c];
                if (t->GetType() != 'v') {
                    cl.relTable = t;
                    cl.relcol = cl.num;
                    t->SetCol(c, cl);
                    continue;
                }
                if (linkcol > 0 && cl.relTable==NULL && cl.linkOid!='0') {
                    if (cl.relcol == -1) continue;
                    wxString oid = cl.linkOid;
                    int outrelcol = -1;
                    Table* rel = GetRelTableForViewCol(oid,linkcol, outrelcol);
                    cl.relcol = outrelcol;
                    if (rel != NULL) {
                        cl.relTable = rel;
                    }
                    t->SetCol(c, cl); // save cl 
                }

            }
        }
    }
}
Table *TableColsMap::GetTableByName(const wxString& sch, const wxString& tname) {
    for (auto& e : oids) {
        Table* t = e.second;
        if (t->GetName() == tname) {
            if (!sch.IsEmpty() && t->GetSchema() == sch
                || sch.IsEmpty()) {
                return t;
            }
        }
    }
    return NULL;
}
void Table::Set(wxString kind, wxString sch, wxString toid, wxString tname) {
    schema = sch;
    if (tname != name) name = tname;
    if (kind.length() > 0) type = kind[0]; else type = 0;
    oid = toid;
}
void Table::AddColumn(wxString ncol, wxString colname, wxString oidTable, wxString ncolTable) {
    Cols c;
    c.num = (int)StrToLong(ncol);
    c.linkOid = oidTable;
    c.linknumcol = (int)StrToLong(ncolTable);
    c.name = colname;
    cols.push_back(c);
}
void TableColsMap::getDatabaseViews() {
    wxString sql = R"(select cv.oid,cv.relname,cv.relnamespace::regnamespace,cv.relkind, r.ev_action from --pg_class cv 
              pg_rewrite r
              join pg_depend dp on dp.objid=r.oid and dp.deptype='i' and r.rulename='_RETURN' and dp.classid::regclass::text='pg_rewrite'
              join pg_class cv  on cv.oid=dp.refobjid
              
where
)";
    wxString oidslist="";
    wxString sql2;
    sql2= sql + whereexp;
    while (sql2.length() > 0) {
        pgSet* dataSet1 = db->ExecuteSet(sql2);
        if (dataSet1)
        {
            oidslist = "";
            Table* t;
            while (!dataSet1->Eof())
            {
                wxString sc = dataSet1->GetVal(wxT("relnamespace"));
                wxString tn = dataSet1->GetVal(wxT("relname"));
                wxString kind = dataSet1->GetVal(wxT("relkind"));
                wxString action = dataSet1->GetVal(wxT("ev_action"));
                wxString oid = dataSet1->GetVal(wxT("oid"));
                //std::map<Table*, std::vector<LinkTable>>::iterator it;
                //for (it = table_links.begin(); it != table_links.end();++it) {
                int ncolmax = 0;
                                if (auto it = oids.find(oid); it != oids.end())
                                {
                                    t = it->second;
                                }
                                else {
                                    t = new Table();
                                    oids.insert({ oid, t });
                                }
                                if (t->GetType() == 0)
                                {
                                    t->Set(kind, sc, oid, tn);
                                    while (regaction.Matches(action))
                                    {
                                        size_t start, len;
                                        regaction.GetMatch(&start, &len, 0);
                                        if (len == 0) break;
                                        wxString ncol = regaction.GetMatch(action, 3);
                                        int tmp = (int)StrToLong(ncol);
                                        wxString colname = regaction.GetMatch(action, 4);
                                        wxString oidTable = regaction.GetMatch(action, 5);
                                        wxString ncolTable = regaction.GetMatch(action, 6);
                                        if (tmp <= ncolmax) break; // ignore columns
                                        ncolmax = tmp;
                                        t->AddColumn(ncol, colname, oidTable, ncolTable);
                                        action = action.Mid(start + len);
                                        if (oidTable != "0" && oids.find(oidTable) == oids.end()) {
                                            wxString tmpname;
                                            tmpname = "";
                                            Table* t2 = new Table();
                                            t2->Set("", "", oidTable, tmpname);
                                            oids.insert({ oidTable, t2 });
                                            //cuurent_list.push_back(t2);
                                            //oids.insert({ oid, t });
                                            if (!oidslist.IsEmpty()) oidslist += ',';
                                            oidslist += wxString::Format("'%s'", oidTable);
                                        }
                                    }
                                }
                
                dataSet1->MoveNext();
            }
            delete dataSet1;
            // 
            if (oidslist.IsEmpty()) sql2 = ""; else {
                sql2 = sql + " cv.oid in (" + oidslist + ")";
            }
        }
        else break;

    }
}
void TableColsMap::getDatabaseTables(const TableColsMap::Flag flag) {
    //std::vector<Table*> cuurent_list;
    wxString onlyoid;
    for (auto& e : oids) {
        Table* t = e.second;
        if (t->GetType() == 0 && !t->GetOID().IsEmpty()) {
            wxString oid = t->GetOID();
            onlyoid = onlyoid + ((onlyoid.length() > 0 ? "," : "") + wxString::Format("'%s'", oid));
        }
    }
    
    wxString sql = R"(
select cv.oid,cv.relname,cv.relnamespace::regnamespace, cv.relkind, a.attnum,a.attname from pg_class cv,pg_attribute a 
 where cv.oid=a.attrelid and a.attnum>0 and a.attisdropped=false and ()";
    
    if (!onlyoid.IsEmpty()) sql = sql + "cv.oid in(" + onlyoid + ") "; else sql = sql + "false";

    if (!whereexp.IsEmpty()) {
        sql = sql + " or (" + whereexp + " )";
    }
    sql+= +") order by cv.oid,attnum";
    pgSet* dataSet1 = db->ExecuteSet(sql);
    if (dataSet1)
    {
        Table* t;
        wxString prevoid;
        while (!dataSet1->Eof())
        {
            
            wxString oid = dataSet1->GetVal(wxT("oid"));
            wxString sc = dataSet1->GetVal(wxT("relnamespace"));
            wxString tn = dataSet1->GetVal(wxT("relname"));
            wxString kind = dataSet1->GetVal(wxT("relkind"));
            wxString ncol= dataSet1->GetVal(wxT("attnum"));
            int num= (int)StrToLong(ncol);
            wxString colname = dataSet1->GetVal(wxT("attname"));
            if (kind != "v") {
                if (prevoid != oid) {
                    prevoid = oid;
                    if (auto it = oids.find(oid); it != oids.end()) {
                        t = it->second;
                        t->Set(kind, sc, oid, tn);
                    }
                    else {
                        // first table
                        // сохраним oid для получения FK
                        onlyoid = onlyoid + ((onlyoid.length() > 0 ? "," : "") + wxString::Format("'%s'", oid));
                        t = new Table();
                        t->Set(kind, sc, oid, tn);
                        oids.insert({ oid, t });
                    }
                }
                t->AddColumn(ncol, colname, "", "");
            }
            dataSet1->MoveNext();
        }
        delete dataSet1;
    }
    // FK info
    wxString sqlfk = R"(
select rel.oid
   ,( select string_agg(attnum::text, ',' order by ordinality)
    from pg_attribute,
    unnest(c.conkey) with ordinality
    where attrelid = c.conrelid
    and attnum = unnest
 ) con_col_list
 ,relf.oid foid
 ,( select string_agg(attnum::text, ',' order by ordinality)
    from pg_attribute,
    unnest(c.confkey) with ordinality
    where attrelid = c.confrelid
    and attnum = unnest
 ) conf_col_list
  from pg_constraint c
   left join pg_class rel  on rel.oid=c.conrelid
   left join pg_class relf on relf.oid=c.confrelid
   where c.contype in ('f','p') 
and rel.oid in ()" + onlyoid+") order by oid,foid";
    if (onlyoid.IsEmpty()) return;
    dataSet1 = db->ExecuteSet(sqlfk);
    std::set<fk_full_struct> uniq_fk_index;
    if (dataSet1)
    {
        Table* t=NULL;
        std::vector<LinkTableFK> all_fk_table;
        std::map<wxString, tab_col_struct> forein_unknown_tab; //[foid+fcol]=childTable
        while (!dataSet1->Eof())
        {
            wxString oid = dataSet1->GetVal(wxT("oid"));
            wxString foid = dataSet1->GetVal(wxT("foid"));
            wxString collist = dataSet1->GetVal(wxT("con_col_list"));
            wxString colflist = dataSet1->GetVal(wxT("conf_col_list"));
                if (auto it = oids.find(oid); it != oids.end()) {
                    if (t != it->second) {
                        //if (all_fk_table.size()>0) tablechild_fk[tf] = fk;
                    }
                    t = it->second;
                    bool flag_pk = false;
                    if (foid.IsEmpty()) {
                        flag_pk = true;
                        foid = oid; // PK = self FK
                        colflist = collist;
                    }
                    wxArrayString ar = wxSplit(collist, ',');
                    wxArrayString arf = wxSplit(colflist, ',');
                    Table tmp = *t;
                    //tablechild_fk
                    LinkTableFK fk;
                    fk.parent = t;
                    Table* tf = NULL;
                    fk.child = NULL;
                    for (int k = 0; k < ar.GetCount(); k++) {
                        int i = (int)StrToLong(ar[k]);
                        fk.colsp.push_back(i);
                        if (flag_pk) {
                            tmp[i - 1].pk = true;
                        }
                        int fi = (int)StrToLong(arf[k]);
                        fk.colsc.push_back(fi);
                        tmp[i-1].linknumcol = fi;
                        tmp[i-1].linkOid = foid;
                        if (auto it2 = oids.find(foid); it2 != oids.end()) {
                            tf = it2->second;
                            tmp[i - 1].linkTable = tf;
                            if (fk.child == NULL) fk.child = tf;
                        }
                        else {
                            wxString key = foid +','+ arf[k];
                            
                            if (auto it2 = forein_unknown_tab.find(key); it2 != forein_unknown_tab.end()) {
                                // Уже была эта таблица и колонка. Свяжем две наши таблицы через третью.
                                // можно связать наши таблицы через неизвестную .Увы только один уровень вложенности.
                                tab_col_struct parent= it2->second;
                                tf = parent.t;
                                tmp[i - 1].linkTable = tf;
                                fi = parent.column;
                                if (fk.child == NULL) fk.child = tf;
                            }
                            else {
                                // ссылка на не известную таблицу запомним её и Нашу таблицу
                                tab_col_struct parent = { t,i };
                                forein_unknown_tab.insert({ key,parent });
                            }
                            
                        }
                        if (fk.child != NULL) {
                            tab_col_struct parent = { t,i }, child = {tf,fi};
                            all_fk_index.insert({ parent,child });
                            all_fk_index.insert({ child,parent }); // учтем комутативность a=b , то b=a
                            // uniq index
                            uniq_fk_index.insert({ parent,child });
                            uniq_fk_index.insert({ child,parent });
                            
                        }

                    }
                    if (tf != NULL) {
                        //all_fk_table=tablechild_fk[tf];
                        //all_fk_table.push_back(fk);
                        //tablechild_fk[tf] = all_fk_table;
                    }
                    //t->Set(kind, sc, oid, tn);
                }
            dataSet1->MoveNext();
        }
        delete dataSet1;
    }
    
    if (CHKFLAG(flag, TableColsMap::Flag::USE_TRANSIT_FK)) {
    // добавим транзитивности a=b b=c , то a=c
        std::set<fk_full_struct> transit;
        int lvl = 0;
        for (auto e: all_fk_index) {
            tab_col_struct fnd=e.second;
            search_link(e.first, e.second,transit,lvl);
        }
        for (auto &e : transit) {
            //tab_col_struct fnd = e.second;
            if (uniq_fk_index.find(e)== uniq_fk_index.end()) {
                all_fk_index.insert({e.left,e.right});
            }
            
        }

    }
}
tab_col_struct TableColsMap::search_link(tab_col_struct start, tab_col_struct chknode, std::set<fk_full_struct> &traz, int level) {
    //tab_col_struct fnd = e.second;
    if (start.t == chknode.t || level>10) return { NULL,1 };
    fk_full_struct full_search = { start,chknode };
    for (auto itr = all_fk_index.find(chknode); itr != all_fk_index.end(); itr++) {
        if (itr->first.t != chknode.t || itr->first.column != chknode.column) {
            //wxTrap();
            continue; // find not work
        }
        if (start == itr->second) // back reference ignore
            continue;
        if (itr->second.t == chknode.t) continue;
        fk_full_struct unik = { start,itr->second };
        auto [iter, has_been_inserted]=traz.insert(unik);
        //tab_col_struct fk = itr->second;
        if (has_been_inserted) search_link(start, itr->second, traz, level + 1);
    }

    return { NULL,1 };
};