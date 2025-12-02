#include "pgAdmin3.h"
#include <utils/FunctionPGHelper.h>
#include <wx/stdpaths.h>
#include <wx/textfile.h>
#include <wx/filename.h>
#include "db/pgSet.h"
#include "frm/frmMain.h"
#include "ctl/ctlSQLBox.h"
#include <stack>

extern sysSettings* settings;
extern frmMain *winMain;


wxString FunctionPGHelper::getHelpString(wxString fnd, bool isPart)
{
        if (!isValid()) return wxEmptyString;
        auto search = body.find(fnd);
        wxString txt;

        if (search != body.end())
            txt = search->second;
        else
        {
            std::vector<wxString> list;
            int l = fnd.Len();
            wxString b;
            for (const auto& e : body) {
                if (e.first.Len() > l && e.first.StartsWith(fnd)) {
                    list.push_back(e.first);
                    b = e.second;
                }
            }
            if (list.size() == 1) txt = b;
            else {
                for (const auto& s : list) {
                    txt += wxString::Format("<a href=\"%s\">%s</a><br>", s, s);
                }
            }
        }
        //if (i == wxNOT_FOUND) return wxEmptyString;
        if (txt.Len()>1 && txt[0]=='@') {
                txt=txt.substr(1);
                txt=getHelpFile(txt);
        }
        return txt;
}
void FunctionPGHelper::setDbConn(pgConn *db) {
        dblast=db;
}
#include "schema/pgDatabase.h"
#include "schema/pgSchema.h"
#include "schema/pgTable.h"
#include "schema/pgView.h"
#include "schema/pgFunction.h"
#include "schema/pgTrigger.h"

extern pgDatabaseFactory databaseFactory;
extern pgSchemaFactory schemaFactory;
extern pgTableFactory tableFactory;
extern pgViewFactory viewFactory;
extern pgFunctionFactory functionFactory;
extern pgProcedureFactory procedureFactory;
extern pgTriggerFunctionFactory triggerFunctionFactory;
wxString FunctionPGHelper::getDBinfoKeyword(wxString objname, bool islower)  {
        if (dblast) {
            wxString s,n,a,tmp;
            tmp=objname;
            a=objname.AfterFirst('(');
            if (!a.IsEmpty()) {
                tmp=objname.BeforeFirst('(');
                a=a.substr(0,a.Len()-1);
            } else a="%";
            make_identifier(tmp,s,n,islower);
            if (s.IsEmpty()) {
                s="%";
            }
wxString querytemplate=R"(
select p.oid,p.relnamespace::regnamespace nsp,p.relname objname, p.relkind::text,
case when p.relkind in ('v','m')
then
--pg_get_viewdef(p.oid,true)
''
else
''
end
 define , '' args, obj_description(oid,'pg_class') comment from pg_class p 
where p.relkind in ('r','p','m','v') and p.relnamespace::regnamespace::text not in ('pg_catalog','information_schema')
and p.relnamespace::regnamespace::text like '%s' and p.relname like '%s'
and (select count(*) from pg_partition_ancestors(oid) ) <=1
union all
select p.oid,p.pronamespace::regnamespace nsp,p.proname objname,
case when pg_get_function_result(p.oid) is null then 'P' 
     when pg_get_function_result(p.oid) = 'trigger' then 't' else 'f' end,
'' prosrc ,pg_get_function_arguments(oid), obj_description(oid,'pg_proc') from pg_proc p
where p.pronamespace::regnamespace::text like '%s' and p.proname like '%s' and pg_get_function_arguments(oid) like '%s'
and p.pronamespace::regnamespace::text not in ('pg_catalog','information_schema')
order by objname;    
)";
            wxString es=s;
            wxString en=n;
            es.Replace("'","''");
            en.Replace("'","''");
            wxString sql=wxString::Format(querytemplate
            ,es,en,es,en,a);
            pgSet *res = dblast->ExecuteSet(sql);
            wxString txt,ns,on,kind,args,def,oid;
            bool isfunc=false;
            int c=0;
            while (!res->Eof())
            {
                ns =  res->GetVal("nsp");
                oid =  res->GetVal("oid");
                on =  res->GetVal("objname");
                kind =  res->GetVal("relkind");
                args = res->GetVal("args");
                def = res->GetVal("define");
                wxString link=ns+"."+on;
                if (kind=="f" || kind=="p") {
                    link+="("+args+")";
                    isfunc=true;
                } else isfunc=false;
                txt += wxString::Format("<a href=\"%s\">%s</a><br>", link, link);
                c++;
                res->MoveNext();
            }
            if(res)
            {
                delete res;
                res = NULL;
            }
            if (c>1) return txt;
            if (c==1)
            {
                    wxString html;
                    if (def.IsEmpty())
                    {
                        wxTreeItemIdValue foldercookie;
                        ctlTree *browser=winMain->GetBrowser();
                        wxTreeItemId folderitem = browser->GetFirstChild(browser->GetRootItem(), foldercookie);
                        
                        while (folderitem)
                        {
                            if (browser->ItemHasChildren(folderitem))
                            {
                                wxTreeItemIdValue servercookie;
                                wxTreeItemId serveritem = browser->GetFirstChild(folderitem, servercookie);
                                wxString host=dblast->GetHost();
                                wxString db=dblast->GetDbname();
                                int port=dblast->GetPort();
                                wxString fullid=on;
                                if (!args.IsEmpty()) fullid+="("+args+")";
                                wxString idf=wxString::Format("%s:%d",host,port);
                                while (serveritem)
                                {
                                    pgServer *server = (pgServer *)browser->GetItemData(serveritem);
                                    if (server != NULL && server->IsCreatedBy(serverFactory)) {
                                        if (server->GetIdentifier()==idf ) {
                                            if (server->connection()) {
                                                // is open connect
                                                //wxTreeItemId serveritemc1 = browser->GetFirstChild(folderitem, servercookie);
                                                pgCollection *coll=browser->FindCollection(databaseFactory,serveritem);
                                                wxTreeItemId dbssId;
                                                if (coll) dbssId=coll->GetId();
                                                wxCookieType cookie2;
                                                wxTreeItemId item = browser->GetFirstChild(dbssId, cookie2);
                                                
                                                while (item && item.IsOk())
                                                {
                                                    //wxString tt=browser->GetItemText(item);
                                                    pgObject *obj = browser->GetObject(item);
                                                    if (obj && obj->IsCreatedBy(databaseFactory) && db==obj->GetName() )
                                                    {
                                                        pgCollection *coll=browser->FindCollection(schemaFactory,item);
                                                        wxTreeItemId schemasId;
                                                        if (coll) schemasId=coll->GetId();
                                                        if (schemasId) {
                                                                pgObject *obj2 = browser->GetObject(item);
                                                                //obj2->expandedKids
                                                                wxCookieType cookie4;
                                                                item = browser->GetFirstChild(schemasId, cookie4);
                                                                while (item)
                                                                {
                                                                    pgSchema *obj3 = (pgSchema *)browser->GetObject(item);
                                                                    if (obj3 && obj3->IsCreatedBy(schemaFactory) && ns==obj3->GetName() ) {
                                                                        obj3->ShowTreeDetail(browser);
                                                                        wxTreeItemId objcollId;
                                                                        pgCollection *coll=NULL;
                                                                        if (kind=='r'||kind=='p' )  coll = browser->FindCollection(tableFactory,item);
                                                                        if (kind=='v'||kind=='m')  coll = browser->FindCollection(viewFactory,item);
                                                                        if (kind=='f')  coll = browser->FindCollection(functionFactory,item);
                                                                        if (kind=='P')  coll = browser->FindCollection(procedureFactory,item);
                                                                        //if (kind=='p')  coll = browser->FindCollection(pg_partitionFactory,item);
                                                                        if (kind=='t')  coll = browser->FindCollection(triggerFunctionFactory,item);
                                                                        if (coll) {
                                                                            objcollId=coll->GetId();
                                                                            pgObject *obj4 = browser->GetObject(objcollId);
                                                                            obj4->ShowTreeDetail(browser);
                                                                        }
                                                                        wxCookieType cookie5;
                                                                        item = browser->GetFirstChild(objcollId, cookie5);
                                                                        while (item)
                                                                        {
                                                                            pgObject *obj5 = browser->GetObject(item);
                                                                            //wxString soid=obj5->GetOidStr();
                                                                            if (obj5 && NumToStr(obj5->GetOid())==oid) {
                                                                                pgObject *obj5 = browser->GetObject(item);
                                                                                     obj5->ShowTreeDetail(browser);
                                                                                     def=obj5->GetSql(browser);
                                                                                if (kind=='r'|| kind=='m' || kind=='v' || kind=='f') {
                                                                                     //pgTable *o=(pgTable *)(obj5);
                                                                                     //brower->FindObject(tableFactory,)
                                                                                     //o->AppendStuff(def,browser,tableFactory);
                                                                                }
                                                                                goto exitloop;
                                                                            }
                                                                            item = browser->GetNextChild(objcollId, cookie5);

                                                                        }

                                                                    }
                                                                    item = browser->GetNextChild(schemasId, cookie4);
                                                                }
                                                        }
                                                        goto exitloop;
                                                    }
                                                    item = browser->GetNextChild(dbssId, cookie2);
                                                } // next 

                                            }

                                            goto exitloop; 
                                        }
                                    }
                                    serveritem = browser->GetNextChild(folderitem, servercookie);
                                }
                            }
                            folderitem = browser->GetNextChild(browser->GetRootItem(), foldercookie);
                        }
                    }
                    exitloop:
                    FSQL::FormatterSQL f(def);
                    std::vector<FSQL::complite_element> listobj=f.ParsePLpgsql();
                    ctlSQLBox* box = new ctlSQLBox((wxWindow*) winMain, CTL_SQLQUERY, wxDefaultPosition, wxSize(0, 0), wxTE_MULTILINE | wxTE_RICH2);
                    box->SetText(def);
                    int l = def.Length();
                    box->Colourise(0, box->GetLength());
                    //bg = box->SetSQLBoxColourBackground(false).GetAsString(wxC2S_CSS_SYNTAX);
                    html = box->TextToHtml(0, box->GetLength(),false,listobj);
                    wxColour cbg=box->GetBackgroundColour();
                    wxString bg=cbg.GetAsString(wxC2S_HTML_SYNTAX);
                    delete box;
                    html = "<html><body BGCOLOR=\"" + bg + "\">" + html + "</body></html>";
                    return html;
            }

        }

        return wxEmptyString;
}
wxString FunctionPGHelper::getSqlCommandHelp(wxString fnd) {
        wxUniChar sep = wxFileName::GetPathSeparator();
        fnd.Replace(" ", "");
        wxString f = wxFindFirstFile(path + sep + "sql-" + fnd + "*.html");
        wxString last, txt;

        int c = 0;
        while (!f.empty())
        {
            f = f.AfterLast(sep);
            last = f;
            txt += wxString::Format("<a href=\"%s\">%s</a><br>", f, f);
            f = wxFindNextFile();
            c++;
        }
        if (last.empty()) {
            return wxEmptyString;
        }
        else if (c == 1) {
            return getHelpFile(last);
        }
        else {
            return txt;
        }

}
wxString FunctionPGHelper::getTextForAnchor(wxString name) {
    wxString n;
    if (name.Len()>0 && name[0]=='#') n = name.substr(1); else n=name;
    for (auto s:an) {
        if (s.id==n) {
            n=filecontext.substr(s.start,s.end-s.start+1);
            return n;
        }
    }
    return wxEmptyString;
}
wxString FunctionPGHelper::getHelpFile(wxString filename) {
        wxString a;
        int p=filename.Find('#');
        if (p>0) {a=filename.substr(p+1); filename=filename.BeforeFirst('#');}

        wxString tempDir = path + wxFileName::GetPathSeparator() + filename;
        an.clear();
        filecontext=wxEmptyString;
        if (!wxFileExists(tempDir)) return wxEmptyString;
        wxTextFile  tfile;
        tfile.Open(tempDir);
        // read the first line
        wxString str, sbody;
        sbody = tfile.GetFirstLine();
        bool flag = true;
        wxRegEx b("(<body .*?>)");
        wxString ss;
        std::stack<anchor_src> bg;
        while (!tfile.Eof())
        {
            str = tfile.GetNextLine();
            if (flag && b.Matches(str)) {
                size_t start, len;
                b.GetMatch(&start, &len, 0);
                str = str.Mid(start + len);
                sbody = "<body>";
                flag = false;
            }
            str.Replace(filename,"");
            sbody += str;
        }
        // find ancor
            str=sbody;
            filecontext=sbody;
            int l=str.Len();
            int i=0;
            
            while (i<l) {
                wxChar c=str[i++];
                if (c=='<' && (i+3)<l
                    && str[i+0]=='d'
                    && str[i+1]=='i'
                    && str[i+2]=='v'
                    ) {
                    anchor_src s {i-1,-1,wxEmptyString};
                    i=i+3;
                    // find id="anchor name"
                    while (i<l && str[i]!='>') {
                        if (i>3 && str[i]=='"') {
                            if (str[i-1]=='='
                                && str[i-2]=='d'
                                && str[i-3]=='i'
                            ) {
                                wxString n;
                                i++;
                                while (i<l && str[i]!='"') n=n+str[i++];
                                s.id=n;
                            }
                        }
                        i++;
                    }
                    i++;
                    bg.push(s); // end =-1

                } else if (c=='<' && (i+6)<l
                    && str[i+0]=='/'
                    && str[i+1]=='d'
                    && str[i+2]=='i'
                    && str[i+3]=='v'
                    && str[i+4]=='>'
                    ) {
                    // close
                    i=i+4;
                    anchor_src s1=bg.top();
                    s1.end=i;
                    if (s1.id.Len()>0) an.push_back(s1);
                    bg.pop();
                    if (a.Len()>0 && s1.id==a) sbody=getTextForAnchor(a);
                    i++;
                }
            }

        return sbody;

}
bool FunctionPGHelper::isValid() {
        if (!isload) loadfile();
        return isload;
}
void FunctionPGHelper::loadfile() {
        if (isload) return;
        body.clear();
        path = settings->GetPgHelpPath();
        wxString tempDir = path + "_func.txt";
        //tempDir="C:\\Users\\lsv\\Source\\Repos\\wxHtmlhint\\1";
        if (!wxFileExists(tempDir)) return;
        wxTextFile  tfile;
        tfile.Open(tempDir);

        // read the first line
        wxString str, sbody;
        wxString name = tfile.GetFirstLine();

        //wxSortedArrayString  names;
        name = name.AfterFirst('#');
        // read all lines one by one
        // until the end of the file
        while (!tfile.Eof())
        {
            str = tfile.GetNextLine();
            if (str.Left(1) == '#') {
                body.emplace(name, sbody);
                sbody = "";
                name = str.AfterFirst('#');
            }
            else sbody += str;
        }
        body.emplace(name, sbody);
        isload = true;
};
