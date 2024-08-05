#include "pgAdmin3.h"
#include "wx/regex.h"
#include <stack>
using namespace FSQL;
wxString FormatterSQL::printParseArray() {
    int i = 0;
    wxString s;
    for (auto& it : items) {
        wxString e;
        //s.Append(wxString::Format("Index: %d\n",i));
        if (it.endlevel != -1) s.Append(wxString::Format("Index: %d Jump %d", i, it.endlevel));
        else s.Append(wxString::Format("Index: %d", i));
        s.Append(wxString::Format(" Type: %d", it.type));
        s.Append(wxString::Format(" widt: %d", it.width));
        s.Append(wxString::Format(" Flag: %d", it.flags));
        s.Append(wxString::Format(" Val : %s\n", it.txt));
        i++;
    }
    return s;
};
int  FormatterSQL::GetIndexItemNextSqlPosition(int sqlPosition) {
    int p = -1;
    view_item vi;
    for (auto& it : items) {
        p++;
        if (it.srcpos < sqlPosition) continue;

        while (p >= 0 && items[--p].srcpos == -1) {};
        vi = items[p];
        return p;
    }
    return items.size() - 1;
};
bool FormatterSQL::GetItem(int index, FSQL::view_item& item) {
    if (index >= 0 && index < items.size()) {
        item = items[index];
        return true;
    }
    return false; // Error
};

wxString FormatterSQL::get_list_columns(int startindex, union Byte zone) {
    wxString cols;

    wxArrayString ar;
    int i = startindex;
    int ngroup = 0;
    int nbracket = 0;
    if (zone.b.from || zone.b.with) {
        while (next_item_no_space(i) != -1) {
            if (items[i].txt == ',')
            {
                ngroup = 0;
            }
            if (items[i].txt == '(') nbracket++;
            if (items[i].type == name || items[i].type == identifier)
            {
                if (ngroup < 1) ar.Add(items[i].txt); // only first name
                ngroup++;
            }
            if (items[i].txt == ')')
                if (--nbracket == 0) break;
            i++;
        }
        cols = wxJoin(ar, ',');

    }
    return cols;
}
wxString FormatterSQL::GetListTable(int cursorPos) {
    int s = 0;
    wxString r = "";
    while (s < listTable.size()) {
        complite_element* el = &listTable[s++];
        r += wxString::Format("[ %s,%s] %s\n", el->table, el->alias, el->columnList);
    }
    return r;
}
wxString FormatterSQL::GetColsList(wxString what, wxString& listfieldOut, wxString& nameTableOut) {
    wxString r = "";
    wxString f = "";
    wxString t = "";
    nameTableOut = "";

    wxArrayString ar = wxSplit(what, '.');
    int iTab = 0;
    int iFld = 1;
    if (ar.GetCount() > 2) { iTab++; iFld++; }
    if (ar.GetCount() == 1) { iTab = 0; iFld = -1; }
    bool astreplace = false;
    if (iFld != -1 && ar[iFld] == '*') astreplace = true;
    if (iFld != -1 && !astreplace) f = ar[iFld].Lower();
    t = ar[iTab].Lower();
    std::map<wxString, int> tablename;
    // check recursive
    // removed dublicate empty
    // 
    //
    wxString tmp = t;
    int nc = 20; // max depth recursion
iteration_remove_dublicate:
    int k = 0;
    tablename.clear();
    nc--;
    while (nc > 0 && k < listTable.size()) {
        complite_element* el = &listTable[k++];
        if (el->table.Lower() == tmp && el->columnList.Len() == 0) { // for table
            tablename[tmp] = k - 1;
        }
        else if (el->table != "@" && el->alias.Lower() == tmp && el->columnList.Len() == 0) { // for alias
            tmp = el->table.Lower();
            //tablename[tmp] = k - 1;
            goto iteration_remove_dublicate;
        }
        else if (el->alias.Lower() == tmp && el->columnList.Len() > 0 && tablename.count(tmp) > 0) {
            // remove empty dublicate
            int j = tablename[tmp];

            listTable.erase(listTable.begin() + j);
            //listTable.
            goto iteration_remove_dublicate;
        }

    }
    tablename.clear();
    //
iteration2:
    int s = 0;
    complite_element* el;
    while (s < listTable.size()) {
        el = &listTable[s++];
        //r += wxString::Format("[ %s,%s] %s\n", el->table, el->alias, el->columnList);
        if (el->table != '@' && el->table.Lower() == t) {
            nameTableOut = t;
        }
        else if (el->alias.Lower() != t) {
            continue;
        }
        else {
            nameTableOut = el->table;
            if (nameTableOut == "@") nameTableOut = el->alias.Lower();
        }
        // 
        if (el->columnList.Len() == 0) {
            // transition name 
            if (tablename.count(nameTableOut) != 0) {
                //break;

            }
            else {
                tablename[nameTableOut] = s - 1;
                t = nameTableOut;
                goto iteration2;
            }
        }
        wxArrayString ar = wxSplit(el->columnList, ',');
        wxArrayString rez;
        wxString ff;
        for (int j = 0; j < ar.GetCount(); j++) {
            ff = ar[j].AfterLast('.');
            if (astreplace || ff.Lower().StartsWith(f)) rez.Add(ff);
        }
        r = wxJoin(rez, '\t');
        break;
    }
    return r;
}
wxString FormatterSQL::BuildAutoComplite(int startIndex, int level) {
    int len_items = items.size();
    int n_element = startIndex;
    int found_index = startIndex;
    int start_select_list;
    int indexlastID;
    union Byte zone { 0 };
    wxString lastname;
    wxString query_cols_list;
    wxArrayString cols, colsfirst;
    complite_element el;

    wxArrayString objName;
    wxString cols_name;
    bool isfunction = false;
    bool isskipnext = false;
    el.columnList = ""; el.alias = ""; el.table = "";
    if (level == 0) listTable.clear();
    while (next_item_no_space(found_index) != -1) {
        view_item* vi = &items[found_index];
        if (vi->type == comment) {
            found_index++;
            continue;
        }
        if (vi->type == keyword) {
            union Byte z = zone;
            if (vi->txt.Lower() == "from" && vi->flags != 0) {
                if (zone.b.select_list) {
                    if (!lastname.IsEmpty())cols.Add(lastname);
                }
                zone.b.from = 1; zone.b.select_list = 0;
                objName.Clear(); isfunction = false; isskipnext = false; el.columnList = ""; el.alias = ""; el.table = "";
            }
            if (vi->txt.Lower() == "with") {
                zone.b.with = 1; zone.b.skip = 1;
            }
            if (vi->txt.Lower() == "select") {
                zone.b.select_list = 1; zone.b.with = 0; start_select_list = found_index + 1;
                isfunction = false;
                zone.b.skip = 0;
                cols.Clear();
                //el.startIndex = found_index + 1;
            }
            if (vi->txt.Lower().Find("join") > -1) {

                goto close_element_from;
            }
            if (vi->txt.Lower() == "on") {
                goto close_element_from;
            }

            if ((vi->flags & end_from) != 0) {
                zone.b.from = 0;
                zone.b.skip = 1;
                isskipnext = true;
                //objName.Clear();
                if (colsfirst.GetCount() == 0) colsfirst = cols;
                // после from нам нужно дойти до union и прочих объединений

                if (objName.Count() > 0) {
                    if (isfunction) {
                        // [ LATERAL ] ( выборка ) [ AS ] псевдоним
                        // [ LATERAL ] имя_функции ( [ аргумент [, ...] ] ) [WITH ORDINALITY] [[ AS ] псевдоним
                        el.table = "@";
                        el.alias = lastname;
                    }
                    else {
                        //[ ONLY ] имя_таблицы [ * ] [ [ AS ] псевдоним 
                        if (objName.GetCount() > 0) el.table = objName[0];
                        if (objName.GetCount() > 1) el.alias = objName[1];
                        el.columnList = "";
                    }
                    listTable.push_back(el);
                    el.columnList = ""; el.alias = ""; el.table = "";
                    objName.Clear();
                }
            }
            if (z.byte != zone.byte) {
                found_index++;
                continue;
            }

        }
        else if ((vi->txt == ',' && zone.b.skip == 0) || vi->txt == ')') {
            if (zone.b.select_list) {
                cols.Add(lastname);
            }
            else if (zone.b.from) {
            close_element_from:
                if (!isskipnext) {
                    if (isfunction) {
                        // [ LATERAL ] ( выборка ) [ AS ] псевдоним
                        // [ LATERAL ] имя_функции ( [ аргумент [, ...] ] ) [WITH ORDINALITY] [[ AS ] псевдоним
                        el.table = "@";
                        el.alias = lastname;
                    }
                    else {
                        //[ ONLY ] имя_таблицы [ * ] [ [ AS ] псевдоним 
                        if (objName.GetCount() > 0) el.table = objName[0];
                        if (objName.GetCount() > 1) el.alias = objName[1];
                        el.columnList = "";
                    }
                    listTable.push_back(el);
                }

            }
            objName.Clear(); isfunction = false; isskipnext = false; el.columnList = ""; el.alias = ""; el.table = "";
            cols_name = "";
            if (vi->txt == ')') {
                zone.b.from = 0;
                break;
            }
            if (vi->txt.Lower() == "on") {
                isskipnext = true; // skip after on 
                ;
            }

        }
        else if (isskipnext) {
            // пропускаем всё до следующей запятой
        }
        else if (vi->type == identifier || vi->type == name) {
            int i = found_index - 1;
            if (next_item_no_space(i) != -1) {
                if (items[i].type == separation && items[i].txt == "::") {
                    i = found_index;
                    lastname = "";
                    int prev = i;
                    while (next_item_no_space(i) != -1) {
                        if (items[i].type == separation || (items[i].type == keyword && items[i].txt.Lower() != "as")) break;
                        if (i - prev > 1) lastname += ' ';
                        //lastname += items[i].txt;
                        lastname = items[i].txt;
                        prev = i;
                        i++;
                    }
                    indexlastID = found_index;
                    found_index = i - 1;
                    if (i == -1) break;

                }
                else {
                    lastname = vi->txt;
                    indexlastID = found_index;
                }
                objName.Add(lastname);
            }
        }
        else if (vi->txt == '(') {
            int jump = vi->endlevel;
            if (zone.b.select_list) {
                found_index = jump + 1;
                continue;
            }
            if (check_bracket(found_index) == 1) {
                // запрос
                found_index++;
                if (next_item_no_space(found_index) != -1) {
                    query_cols_list = BuildAutoComplite(found_index, level + 1);
                    if (zone.b.with) {
                        el.alias = lastname;
                        el.table = "@";
                        if (el.columnList.IsEmpty()) el.columnList = query_cols_list;
                        //to do  check recursive table

                        listTable.push_back(el);
                        el.columnList = ""; el.alias = ""; el.table = "";
                        objName.Clear();
                    }
                    else
                        el.columnList = query_cols_list;
                    isfunction = true;
                    found_index = jump + 1;
                    continue;
                }
            }
            else {
                if (objName.GetCount() == 1) {

                    if (zone.b.with) {
                        cols_name = get_list_columns(found_index, zone); // columns name
                        el.columnList = cols_name;
                        found_index = jump + 1;
                        continue;
                    }
                    // это функция с аргументами
                    if (!isfunction) {
                        isfunction = true;
                        found_index = jump + 1;
                        continue;
                    }
                    // определение функции но бех псевдонима
                    //  [ LATERAL ] имя_функции ( [ аргумент [, ...] ] ) AS ( определение_столбца [, ...] )

                }
                // 
                cols_name = get_list_columns(found_index, zone);
                //    get_list_columns(found_index, zone, el);
                if (!cols_name.IsEmpty()) {
                    if (isfunction)
                        el.table = "@";
                    else
                        if (objName.Count() > 0) el.table = objName[0];
                        else
                            el.table = "-";
                    el.alias = lastname;
                    el.columnList = cols_name;
                    listTable.push_back(el);
                }
                isskipnext = true;
                found_index = jump + 1;
                continue;
            }
        }
        else if (vi->txt == '*') {
            if (zone.b.select_list) {
                //cols.Add('*');
                lastname = '*';
            }
        }
        else if (vi->txt == ".*") {
            if (zone.b.select_list) {
                if (items[found_index - 1].type == name) {
                    //cols.Add(items[found_index - 1].txt + vi->txt);
                    lastname = items[found_index - 1].txt + vi->txt;
                }
            }
        }
        found_index++;
    }
    if (zone.b.select_list) {
        cols.Add(lastname);
    }
    if (zone.b.from) { // не было ключевых слов с end_from флагом, значит нужно обработать последний элемент from
        if (!isskipnext) {
            if (isfunction) {
                // [ LATERAL ] ( выборка ) [ AS ] псевдоним
                // [ LATERAL ] имя_функции ( [ аргумент [, ...] ] ) [WITH ORDINALITY] [[ AS ] псевдоним
                el.table = "@";
                el.alias = lastname;

            }
            else {
                //[ ONLY ] имя_таблицы [ * ] [ [ AS ] псевдоним 
                if (objName.GetCount() > 0) el.table = objName[0];
                if (objName.GetCount() > 1) el.alias = objName[1];
                el.columnList = "";
            }
            listTable.push_back(el);
        }

    };
    if (colsfirst.GetCount() > 0) return wxJoin(colsfirst, ',');
    else
        return wxJoin(cols, ',');
}
/// <summary>
/// <c>ParseSql</c> Выполнение разбора текста как SQL выражения
/// </summary>
/// <param name="flags"></param>
/// <returns>Возвращает код ошибки если SQL выражение было не полное или не корректное</returns>
int FormatterSQL::ParseSql(int flags) {
    int i = 0;
    int lhome = 0;
    bool str_literal = false;
    bool ext = false;
    bool newline = false;
    int iscomment = 0;
    wxRegEx regnumeric("^([0-9]*[.]?[0-9]*([Ee][-+]?[0-9]+)?)|(inf)|(nan)", wxRE_EXTENDED | wxRE_ICASE);
    wxRegEx regident("(^[[:alpha:]][[:alnum:]_$]*)", wxRE_EXTENDED | wxRE_ICASE);
    wxRegEx regdol("(^[[:alpha:]][[:alnum:]_]*[$])", wxRE_EXTENDED | wxRE_ICASE);
    wxChar qt;
    wxString cons;
    std::stack<int> bracket;
    items.clear();
    wxString dollarSep;
    view_item vi;
    bool ex = false;
    bool needafterspace = false;
    wxChar c;
    while (!ex) {
        c = '\0';
        if (i < sql.length()) c = sql[i++];
        else ex = true;

        wxChar c2(0);
        if (i < sql.length()) c2 = sql[i];
        if (iscomment > 0) {
            if (ex) {
                iscomment = 0;
                vi.txt = sql.substr(lhome, i - lhome);
                vi.type = comment;
                ex = false;
                continue;
            }
            if (iscomment == 1 && (c == '\r' || c == '\n')) {
                iscomment = 0;
                vi.txt = sql.substr(lhome, i - lhome - 1);
                vi.type = comment;
                newline = true;
                i--;
                continue;
            }
            if (iscomment == 2 && c == '*' && c2 == '/') {
                iscomment = 0;
                vi.txt = sql.substr(lhome, i - lhome + 1);
                vi.type = comment;
                i++;
                continue;
            }
            continue;
        }

        if (vi.type != unknown) {
            if (c == '\n' && vi.type == comment && vi.txt[0] == '/') { // \n append comment if exists
                vi.txt.append(c);
            }
            if (vi.type != spaces && newline) {
                vi.newline = newline;
                newline = false;
            }
            if (vi.type == name || vi.type == identifier) {
                int kj = items.size() - 2;
                if (items.size() > 1 && items[kj + 1].txt == "." && (items[kj].type == name || items[kj].type == identifier)) {
                    // union name.name
                    items[kj].type = identifier;
                    items[kj].txt = items[kj].txt + "." + vi.txt;
                    items[kj].width = items[kj].txt.Len();
                    //if ("trabopt.value_num" == items[kj].txt) {
                    //    wxLogError("d");
                    //}
                    items.resize(kj + 1);
                    view_item tmp;
                    vi = tmp;
                    vi.srcpos = i - 1;
                    i--;
                    continue;
                }
            }

            vi.height = 1;
            vi.width = vi.txt.Len();
            if (vi.width > 0) {
                if (vi.txt.Freq('\n') > 0) {
                    int fnd = 0;
                    int cur = 0;
                    int m = 0;
                    int c = 0;
                    int d = 0;
                    while (cur != -1) {
                        fnd = vi.txt.find('\n', cur);
                        if (fnd == -1) {
                            d = vi.txt.Len() - cur;
                            cur = -1;
                        }
                        else {
                            d = fnd - cur;
                            cur = fnd + 1;
                        }
                        //if (m < d) m = d;
                        m = d; // last line width
                        c++;
                    }
                    vi.width = m;
                    vi.height = c;
                }
            }
            else vi.width = 1;
            items.push_back(vi);
            if (vi.type != spaces && needafterspace) {
                view_item tmps;
                tmps.type = spaces;
                tmps.srcpos = -1;
                tmps.width = 1;
                items.push_back(tmps);
            }
            needafterspace = false;
            view_item tmp;
            vi = tmp;
            vi.srcpos = i - 1;
            if (ex) continue;
        }
        if (ex && (str_literal)) continue;
        // literal string or identifier
        if (str_literal) {
            if (c == qt) {
                if (c2 == qt) {
                    // duble quote
                    cons.Append(c);
                    cons.Append(c2);
                    i++;
                    continue;
                }
                int k = i;
                while (c2 == '\n') {
                    // verify string const multiline
                    if (i < sql.length()) c2 = sql[i++]; else break;
                }
                if (c2 == '\'' && i > k) continue;     // multiline string const
                // end literal
                cons.Append(c);
                i = k;
                str_literal = false;
                ext = false;
                vi.txt = cons;
                vi.type = qt == '"' ? identifier : literal;
                continue;
            }
            if (ext && c == '\\' && c2 == '\'') {
                cons.Append(c);
                cons.Append(c2);
                i++;
            }
            cons.Append(c);
            continue;
        }
        // dollar string
        if (c == '$') {
            wxString tmp = sql.substr(i, 64);
            bool matches = regdol.Matches(tmp, 0);
            if (matches) {
                tmp = regdol.GetMatch(tmp, 0);
                int l = tmp.length();
                if (tmp[l - 1] == '$') {
                    dollarSep = "$" + regdol.GetMatch(tmp, 0);
                    i = i + l;

                }
            }
            else {
                if (c2 == '$') {
                    dollarSep = "$$";
                    i++;
                }
                else {
                    int k = i - 1;
                    if (wxIsdigit(c2)) {
                        // bind arg test
                        while (wxIsdigit(c2)) {
                            if (i < sql.length()) c2 = sql[i++]; else break;
                        }
                        if (i == sql.length()) i++;
                        vi.txt = sql.substr(k, i - k - 1);
                        vi.type = bindarg;
                        i--;
                        continue;
                    }
                }
            }
            if (!dollarSep.IsEmpty()) {
                int l = dollarSep.Len();
                wxString tmp = sql.substr(i);
                int pos = tmp.Find(dollarSep);
                if (pos != -1) {
                    vi.txt = sql.substr(i - l, l + l + pos);
                    i += pos + l;
                }
                else {
                    // no close dollar
                    vi.txt = sql.substr(i - l);
                    i = sql.size();
                }
                vi.type = literal;
                continue;
            }

        }
        if (c == '\'' || c == '"') {
            qt = c;
            str_literal = true;
            cons = qt;
            continue;
        }
        if ((c == 'e' || c == 'E') && (c2 == '\'')) {
            str_literal = true;
            cons = c;
            cons.Append(c2);
            ext = true;
            qt = c2;
            i++;
            continue;
        }
        // Unicode const
        if ((c == 'U' || c == 'u') && (c2 == '&')) {
            str_literal = true;
            ext = true;
            lhome = i - 1;
            i++;
            cons = c;
            cons.Append(c2);
            if (i < sql.length()) c = sql[i++];
            qt = c;
            cons.Append(c);
            continue;
        }

        // comment line
        if (c == '-' && c2 == '-') {
            iscomment = 1;
            lhome = i - 1;
            i++;
            continue;
        }
        // comment multiline
        if (c == '/' && c2 == '*') {
            iscomment = 2;
            lhome = i - 1;
            i++;
            continue;
        }
        // new line
        if (c == '\n') {
            vi.type = spaces;
            newline = true;
            continue;
        }
        if (c == '\r' && c2 == '\n') {
            vi.type = spaces;
            newline = true;
            i++;
            continue;
        }

        //numeric
        if (c == '.' && (wxIsdigit(c2)) || wxIsdigit(c)) {
            wxString tmp = sql.substr(i - 1, 30);
            bool matches = regnumeric.Matches(tmp, 0);
            if (matches) {

                vi.txt = regnumeric.GetMatch(tmp, 0);
                int l = vi.txt.Len();
                vi.type = numeric;
                i += l - 1;
                continue;
            }
        }
        // spaces
        int k = i - 1;
        while (c == ' ' || c == '\t') {
            if (i < sql.length()) c = sql[i++]; else { i++;  break; }
        }
        if ((i - 1) > k) {
            if (items.size() != 0 && items[items.size() - 1].type == spaces) {
                // multi space ---> one spaces
            }
            else
                vi.type = spaces;
            i--;
            continue;
        }
        // keyword
        // 
        if ((c >= 'a' && c <= 'z') || ((c >= 'A' && c <= 'Z'))) {
            unsigned n;
            int pos = i - 1;
            wxString tmp = sql.substr(pos, 20).Lower();
            for (n = 0; n < WXSIZEOF(keyEntities); n++)
            {
                const KEYWORDEntity& xmlEnt = keyEntities[n];
                if (tmp.compare(0, xmlEnt.len, xmlEnt.name) == 0
                    )
                {
                    if (pos + xmlEnt.len < sql.length()) c2 = sql[pos + xmlEnt.len]; else break;
                    if (wxIsalpha(c2) || c2 == '_' || c2 == '$' || wxIsxdigit(c2)) continue;
                    break;
                }
            }
            if (n < WXSIZEOF(keyEntities)) {
                tmp = sql.substr(pos, keyEntities[n].len);
                int flg = keyEntities[n].flags;
                int i2 = items.size();
                if (i2 >= 2 && items[items.size() - 2].type == keyword
                    && items[items.size() - 1].type == spaces
                    && items[items.size() - 2].flags == flg
                    && (items[items.size() - 2].flags != special || flg != special)
                    ) {
                    wxString txt = items[items.size() - 2].txt;
                    if (!txt.IsEmpty()) txt += " ";
                    items[items.size() - 2].txt = txt + tmp; // add previous
                    items[items.size() - 2].width = items[items.size() - 2].txt.Len();
                    //items[items.size() - 2].flags |= keyEntities[n].flags;
                    //items.resize(items.size() - 1);
                    items.erase(items.end() - 1);
                }
                else {
                    vi.txt = tmp;
                    vi.type = keyword;
                    vi.flags = flg;
                    if (tmp == "from" && i2 >= 2) {
                        wxString s = items[items.size() - 2].txt;
                        if (s.Len() >= 8 && s.substr(s.Len() - 8).CmpNoCase("distinct") == 0) {
                            vi.flags = 0;
                        }
                    }
                    if (keyEntities[n].name == "case") {
                        bracket.push(items.size());
                    }
                    else if (keyEntities[n].name == "end") {
                        int idx = -1;
                        if (bracket.size() > 0) {
                            idx = bracket.top();
                            bracket.pop();
                        }
                        else {
                            return -1; //braket no parent
                        }
                        vi.endlevel = idx;
                        if (idx != -1) items[idx].endlevel = items.size();

                    }
                }
                i += tmp.Len() - 1;
                continue;
            }

        }
        // bracket
        if (c == '(' || c == '[') {
            bracket.push(items.size());
            vi.txt = c;
            vi.type = ::bracket;
            if (c == '(') {
                // is function 
                int pp2 = items.size() - 1;
                int pp3 = next_item_no_space(pp2, -1);
                if (pp3 != -1 && (items[pp3].type == name || items[pp3].type == identifier)) {
                    vi.flags = isFUNCTION;
                }
            }
            continue;
        }
        if (c == ')' || c == ']') {
            //bracket.push(items.size());
            int idx = -1;
            if (bracket.size() > 0) {
                idx = bracket.top();
                bracket.pop();
            }
            else {
                return -1; //braket no parent
            }
            vi.txt = c;
            vi.type = ::bracket;
            vi.endlevel = idx;
            if (idx != -1) items[idx].endlevel = items.size();
            continue;
        }
        // identifier
        if ((c >= 'a' && c <= 'z') || ((c >= 'A' && c <= 'Z'))) {
            i--;
            wxString tmp = sql.substr(i, 64);
            bool matches = regident.Matches(tmp, 0);
            if (matches) {
                tmp = regident.GetMatch(tmp, 0);
                vi.txt = tmp;
                vi.type = name;
                i += tmp.Len();
                continue;
            }
        }
        if (c == ';') break;
        // separat
        if (c == ',') {
            vi.txt = c;
            vi.type = separation;
            needafterspace = true;
            continue;
        }
        wxString sepa;
        while (true) {
            if (c == '-' || c == '+' || c == '.' ||
                c == ':' || c == '~' ||
                c == '@' || c == '*' || c == '/' ||
                c == '=' || c == '!' || c == '#' ||
                c == '&' || c == '%' || c == '^' ||
                c == '|' || c == '`' || c == '?' ||
                c == '>' || c == '<'
                )
            {
                sepa += c;
                if (sepa == ".*" || sepa == "::") {
                    //i++;
                    break;
                }
            }
            else { i--; break; };
            if (i < sql.length()) c = sql[i++]; else break;
        }
        if (!sepa.IsEmpty()) {
            // i=i - (i == sql.length() ? 0: 0);
            vi.txt = sepa;
            vi.type = separation;
            if (items.size() > 0 && items[items.size() - 1].type != spaces) {
                // before add space
                if (vi.txt != ":" && vi.txt != "." && vi.txt != "::" && vi.txt != ".*") {
                    view_item vtmp;
                    vtmp.type = spaces;
                    vtmp.width = 1;
                    vtmp.srcpos = -1;
                    items.push_back(vtmp);
                    needafterspace = true;
                }
            }
            continue;
        }
    }
    // end big loop
    if (str_literal) {
        return -2; // literal no close
    }
    if (bracket.size() > 0)
        return -1;

    return 0;
}
// mode =2 draw position x,y
// mode =1 draw flow
// mode =0 only calc size
wxSize FormatterSQL::best_sizeAndDraw(wxDC& dc, wxPoint& pos, view_item& vi, int mode) {
    wxFont f = dc.GetFont();
    wxSize sz;
    bool draw = true;
    if (mode == 2) {
        draw = false;
        pos.x = vi.x;
        pos.y = vi.y;
    }
    dc.SetTextForeground(*wxBLACK);
    dc.SetFont(f.GetBaseFont());
    if (vi.type == keyword) {
        wxDCFontChanger nfont(dc, f.Bold());

        dc.SetTextForeground(wxColour(0, 0, 127));
        //wxDCBrushChanger nb(dc, *wxBLUE_BRUSH);
        //wxDCPenChanger np(dc, *wxBLUE_PEN);
        //dc.SetBrush(*wxGREY_BRUSH);
        sz = dc.GetTextExtent(vi.txt);
        if ((pos.x + sz.x) > (rect.x + rect.width) && draw) {
            pos.x = rect.x + 1;
            pos.y = pos.y + sz.y;
        }
        if (mode > 0) dc.DrawText(vi.txt, pos);
    }
    else if (vi.type == literal || vi.type == identifier) {
        //wxDCPenChanger np(dc, *wxRED_PEN);
        dc.SetTextForeground(wxColour(127, 0, 127));
        //dc.SetBrush(*wxGREY_BRUSH);
        sz = dc.GetMultiLineTextExtent(vi.txt);
        if ((pos.x + sz.x) > (rect.x + rect.width) && draw) {
            pos.x = rect.x + 1;
            pos.y = pos.y + sz.y;
        }
        if (mode > 0) dc.DrawText(vi.txt, pos);
    }
    else if (vi.type == spaces) {
        sz = dc.GetTextExtent(" ");
        if ((pos.x + sz.x) > (rect.x + rect.width) && draw) {
            pos.x = rect.x + 1;
            pos.y = pos.y + sz.y;
        }
    }
    else if (vi.type == numeric) {
        // wxDCPenChanger np(dc, *wxCYAN_PEN);
        dc.SetTextForeground(wxColour(0, 127, 127));
        sz = dc.GetTextExtent(vi.txt);
        if ((pos.x + sz.x) > (rect.x + rect.width) && draw) {
            pos.x = rect.x + 1;
            pos.y = pos.y + sz.y;
        }
        if (mode > 0) dc.DrawText(vi.txt, pos);
    }
    else if (vi.type == bindarg) {
        // wxDCPenChanger np(dc, *wxCYAN_PEN);
        //dc.SetTextForeground(wxColour(0, 127, 127));
        wxDCFontChanger nfont(dc, f.Bold());
        sz = dc.GetTextExtent(vi.txt);
        if ((pos.x + sz.x) > (rect.x + rect.width) && draw) {
            pos.x = rect.x + 1;
            pos.y = pos.y + sz.y;
        }
        if (mode > 0) dc.DrawText(vi.txt, pos);
    }
    else if (vi.type == comment) {
        // wxDCPenChanger np(dc, *wxCYAN_PEN);
        dc.SetTextForeground(wxColour(0, 127, 0));
        //wxDCFontChanger nfont(dc, f.Bold());
        sz = dc.GetMultiLineTextExtent(vi.txt);
        if ((pos.x + sz.x) > (rect.x + rect.width) && draw) {
            pos.x = rect.x + 1;
            pos.y = pos.y + sz.y;
        }
        if (mode > 0) dc.DrawText(vi.txt, pos);
    }
    else {
        sz = dc.GetTextExtent(vi.txt);
        if ((pos.x + sz.x) > (rect.x + rect.width) && draw) {
            pos.x = rect.x + 1;
            pos.y = pos.y + sz.y;
        }
        if (mode > 0) dc.DrawText(vi.txt, pos);
        if (maxYheightLine < sz.GetHeight()) maxYheightLine = sz.GetHeight();
    }

    if (draw) {
        vi.width = sz.GetWidth();
        vi.height = sz.GetHeight();
        pos.x += sz.GetWidth();
    }

    return sz;
}
/// <summary>
///  Получение индекса следующего или предудущего элемента списка отличного от <c>space</c>
/// </summary>
/// <param name="index">Переменная содержащая индекс элемента списка с которого начинаем поиск</param>
/// <param name="direction">Направление перемещения по списку -1 назад, 1 вперед.</param>
/// <returns>индекс найденого элемента или -1 дошли до конца списка. ВАЖНО: в переменной <c>index</c> будет содержаться найденное значение.</returns>
int FormatterSQL::next_item_no_space(int& index, int direction) {
    bool f = true;
    while (index >= 0 && index < items.size()) {
        if (items[index].type != spaces) {
            f = false;
            break;
        }
        index += direction;
    }
    if (f) return -1;
    return index;
}
int FormatterSQL::get_prev_value(int indx, wxString keyword) {
    bool f = true;
    int indxs = indx;
    while (--indx >= 0) {
        if (items[indx].endlevel != -1 && items[indx].endlevel > indxs) { // start bracet = stop find
            break;
        }
        if (items[indx].endlevel != -1 && items[indx].endlevel < indx) {
            indx = items[indx].endlevel; // 
            continue;
        }

        if (items[indx].type == type_item::keyword && items[indx].txt.Lower() == keyword) {
            f = false;
            break;
        }
    }
    if (f) return -1;
    return indx;
}
/// <summary>
/// Проверка что в скобках подзапрос
/// </summary>
/// <param name="index"> индекс элемента типа <c>bracket</c> созначениме '(' </param>
/// <returns>1 если подзапрос
/// <p>
/// 0 иначе</p></returns>
int FormatterSQL::check_bracket(int index) {
    int s = index;
    wxChar b = items[s].txt[0];
    s++;
    if (b == '(') {
        while (next_item_no_space(s) != -1) {
            if (items[s].type == keyword && items[s].flags & newLineComma) { // only select update delete insert
                // subquery
                return 1;
            }
            if (items[s].type == comment) {
                s++;
                continue;
            }
            break;
        }
    }
    return 0;
}
wxSize FormatterSQL::max_width_size(int index) {
    int e_idx = index;
    int s_idx = items[e_idx].endlevel;
    int max_w = 0;
    wxSize sz(0, 0);
    if (s_idx != -1) {
        if (s_idx > e_idx) {
            int t = s_idx;
            s_idx = e_idx;
            e_idx = t;
        }
        int min_y = 9999999;
        int max_y = 0;
        int min_x = 9999999;
        int max_x = 0;
        while (++s_idx < e_idx) {
            //max_w += items[s_idx].width;
            if (min_y > items[s_idx].y) min_y = items[s_idx].y;
            if (max_y < (items[s_idx].y + items[s_idx].height)) max_y = items[s_idx].y + items[s_idx].height;
            if (min_x > items[s_idx].x) min_x = items[s_idx].x;
            if (max_x < (items[s_idx].x + items[s_idx].width)) max_x = items[s_idx].x + items[s_idx].width;
        }
        sz.x = max_x - min_x;
        sz.y = max_y - min_y;
    }
    return sz;
}
wxPoint FormatterSQL::align_level(int start_i, int level, int Xpos, int Ypos, int flag) {
    int s = start_i;
    int xstart = Xpos;
    int padd = flag & isCASE ? 0 : pad;
    bool iswith = false;
    bool empty_line = true;
    neededNewLine.x = -1;
    neededNewLine.y = -1;
    bool isOneNL = false;
    while (s < items.size()) {
        view_item* vi = &items[s++];
        if (vi->type == unknown) continue;
        int fl = vi->flags;
        wxPoint p(0, 0);
        wxString txt = wxString(vi->txt);
        if (vi->type == keyword && txt == "with") iswith = true;
        if ((vi->txt.Lower() == "dblink")) {
            //            neededNewLine.y = -1;
        }
        if ((s == 2511)) {
            p.x = p.x;
        }
        if (fl & newLineComma) {
            flag = fl;
        }
        else if (fl & HorizontalAlign) {
            int iprev = get_prev_value(s - 1, txt.Lower());
            if (iprev != -1) {
                view_item v = items[iprev];
                if (v.y < Ypos && v.x >= Xpos) {
                    Xpos = v.x;
                }
            }
        }
        else if (fl & newLineXPrevValue) {
            int iprev = get_prev_value(s - 1, txt.Lower());
            if (txt.Lower() == "and") {
                int ibetween = get_prev_value(s - 1, "between"); // exclude between ... and
                int ion = get_prev_value(s - 1, "on"); // exclude on
                if (ibetween != -1 && ion < ibetween) {
                    int nx = items[ibetween].x + items[ibetween].width - vi->width;
                    neededNewLine = wxPoint(nx, Ypos + maxYheightLine);
                    iprev = -1;
                }
                if (ion > iprev && iprev != -1) iprev = -1;
                ion = get_prev_value(s - 1, "where"); // exclude on
                if (ion > iprev && iprev != -1) iprev = -1;
            }
            if (iprev != -1) {
                view_item v = items[iprev];
                //if (Ypos - v.y == 0)
                {
                    neededNewLine = wxPoint(v.x, Ypos + maxYheightLine);
                }
            }
        }
        else if (vi->type == bracket || (vi->endlevel != -1)) {
            // recursion 
            if ((vi->txt.Lower() == "end"
                || (vi->endlevel < s)) && level > 0) {
                if (items[start_i - 1].endlevel == (s - 1)) {
                    // this subquey
                    // exit recursion bracet

                    return wxPoint(Xpos, Ypos);
                }
            }
            else
                //if (check_bracket(s - 1) > 0)
            {

                // subquery
                bool subq = check_bracket(s - 1) > 0;
                if ((iswith && subq)) {
                    Xpos = xstart;
                    Ypos += maxYheightLine;
                }
                vi->x = Xpos;
                vi->y = Ypos;
                Xpos += vi->width;
                int nl = 0; //(subq ? maxYheightLine : 0)
                int flag_in = 0;
                if (fl & isCASE) flag_in = fl;
                if (flag & newLineBracet) flag_in |= newLineBracet;
                if (flag & isCASE &&
                    fl & isCASE) { // nested case 2 level
                    Xpos = xstart + casepad;
                    Ypos += maxYheightLine;
                    vi->x = Xpos;
                    vi->y = Ypos;
                    Xpos += vi->width;
                    //Ypos = vi->y;

                }
                bool ex = false;
                while (!ex) {
                    p = align_level(s, level + 1, Xpos, Ypos + nl, flag_in); // new line
                    // need draw close bracet
                    ex = true;
                    if (subq || fl & isCASE) {
                        //Xpos = vi->x; // vertical aling bracket
                        //Ypos = p.y + maxYheightLine; // new line close braket
                        neededNewLine = wxPoint(vi->x, p.y + maxYheightLine);
                    }
                    else {
                        wxSize sz_br = max_width_size(s - 1);
                        if ((flag & newLineBracet) && sz_br.x > max_exp_bracet_width) {
                            isOneNL = true;
                        }
                        if (sz_br.x > rect.width && ((flag & newLineBracet) == 0)) {
                            if (start_i > 0 && (items[start_i - 1].txt[0] == '(') && p.y == items[start_i - 1].y)
                                return p;
                            if (!(flag_in & newLineBracet)) {
                                flag_in = newLineBracet;
                                ex = false;
                                continue;
                            }
                        }
                        if (sz_br.y > maxYheightLine /*&& !isOneNL*/) { // 2 > line
                            //p.x = vi->x; // vertical aling bracket
                            //p.y = p.y + maxYheightLine; // new line close braket
                            neededNewLine = wxPoint(vi->x, p.y + maxYheightLine);
                        }
                        else {
                            Xpos = p.x;
                            Ypos = p.y;
                        }

                    }
                }
                p.x = 0; p.y = 0;
                s = vi->endlevel;
                vi = &items[s++];
                fl = vi->flags;
                if (fl & isCASE) fl = 0;
            }
        }

        if (p.x == 0 && p.y == 0) {
            if ((vi->txt.Lower() == "from" ||
                vi->txt.Lower() == "order"
                ) && ((flag & newLineComma) == 0)) // for function args
                fl = none;
            if (flag & newLineComma && vi->type == separation && vi->txt[0] == ',') {
                neededNewLine = wxPoint(xstart + padd, Ypos + maxYheightLine);
                if (s < items.size() && items[s].type == spaces) items[s].type = unknown; // disable next space
            }
            else if (fl & new_line_align_next) {
                if (neededNewLine == wxPoint(-1, -1)) {
                    neededNewLine = wxPoint(xstart + padd, Ypos + maxYheightLine);
                }
            }
            else if (fl & new_line_align_no_pad) {
                if (neededNewLine == wxPoint(-1, -1) && Ypos != 0) {
                    neededNewLine = wxPoint(xstart, Ypos + maxYheightLine);
                }
            }
            if (vi->type != spaces && neededNewLine != wxPoint(-1, -1)) {
                Xpos = neededNewLine.x;
                Ypos = neededNewLine.y;
                neededNewLine = wxPoint(-1, -1);
            };
            if ((Xpos + vi->width) >= (rect.width + rect.x)) { // long line
                //Xpos = xstart;
                //Ypos += maxYheightLine;
            }
            vi->x = Xpos;
            vi->y = Ypos;
            Xpos += vi->width;
            if (vi->height > maxYheightLine) Ypos = Ypos + vi->height - maxYheightLine;

            if (vi->type == comment && txt[0] == '-') {
                neededNewLine = wxPoint(xstart + padd, Ypos + maxYheightLine);
            }
            if (isOneNL) {
                neededNewLine = wxPoint(xstart, Ypos + maxYheightLine);
                isOneNL = false;
            }
        }
    }
    return wxPoint(0, 0);
}
void FormatterSQL::Formating(wxDC& dc, wxRect re, bool isTest) {
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawRectangle(re);
    maxYheightLine = 0;
    rect.x = re.x;
    rect.y = re.y;
    rect.width = re.width;
    rect.height = re.height;
    max_exp_bracet_width = 200;
    pad = 16;
    casepad = 8;
    wxPoint p(0, 0);
    // calc size items
    for (auto& vv : items) {
        wxSize sz = best_sizeAndDraw(dc, p, vv, 0);

    }
    // 
    align_level(0, 0, rect.x, rect.y, 0);
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawRectangle(re);
    for (auto& vv : items) {
        p.x = -1;
        if (vv.type == unknown) continue;
        wxSize sz = best_sizeAndDraw(dc, p, vv, 2);
    }
}
// re - (0,0,width simbol max,height max)
wxString FormatterSQL::Formating(wxRect re) {
    rect.x = re.x;
    rect.y = re.y;
    rect.width = re.width;
    rect.height = re.height;
    maxYheightLine = 1;
    max_exp_bracet_width = 100;
    pad = 2;
    casepad = 1;

    align_level(0, 0, rect.x, rect.y, 0);
    // calc string items
    wxString str;
    int cy = 0;
    int cx = 0;
    for (auto& vv : items) {
        if (vv.type == unknown) continue;
        if (cy < vv.y) {
            wxString r('\n', vv.y - cy);
            str.append(r);
            cy += vv.y - cy;
            cx = 0;
        }
        {
            int d = vv.x - cx;
            if (d > 0) {
                wxString r(' ', d);
                str.append(r);
                cx += d;
            }
            if (vv.txt.Len() > 0) str.append(vv.txt); else str.append(' ');
            if (vv.height > maxYheightLine) {
                cy = cy + vv.height - maxYheightLine;
                cx = vv.width;
            }
            else cx += vv.width;
        }
    }
    return str;
}
