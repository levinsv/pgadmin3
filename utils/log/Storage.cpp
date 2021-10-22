#include "pgAdmin3.h"
#include "log/Storage.h"  
#include "utils/utffile.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>

int Storage::getCountStore() {
    return storage.size();
}
int Storage::getCountFilter() {
    int cnt = frows.size();
    if (cnt == 0 && !IsFilter()) cnt = getCountStore();
    return cnt;
}
LineFilter Storage::getLineFilter(wxString strflt) {
    LineFilter lf;
    lf.col = -1;
    if (strflt.IsEmpty()) {
        return lf;
    }
    int col = wxAtoi(strflt.BeforeFirst(':'));
    lf.col = col;
    wxString l = strflt.AfterFirst(':');
    wxString val = l.AfterFirst(' ');
    int flags = 0;
    int i = 0;
    if (l[i] == '!') { flags = FL_REVERSE; i++; }
    if (l[i] == '~') flags = flags | FL_CONTAINS;
    lf.flags = flags;
    lf.val = val;
    return lf;
}
wxString Storage::_strwhere(int flags) {
    wxString expr = "= ";
    if ((flags & FL_CONTAINS) == FL_CONTAINS) expr = "~ ";
    if ((flags & FL_REVERSE) == FL_REVERSE) expr = "!" + expr;
    return expr;
}
wxString Storage::LineFilterToStr(LineFilter& lf) {
    if (lf.col == -1) {
        return "";
    }
    wxString expr = _strwhere(lf.flags)+lf.val;
    expr = wxString::Format("%d:%s", lf.col, expr);
    return expr;
}

void Storage::addLineFilterStr(wxString strflt) {
    LineFilter lf = getLineFilter(strflt);
    filterload.push_back(lf);
}
Storage::Storage() {

    bgErr[MyConst::iconIndex::log] = wxNullColour;
    bgErr[MyConst::iconIndex::war] = wxNullColour;
    bgErr[MyConst::iconIndex::user] = wxColor(250, 196, 186);
    bgErr[MyConst::iconIndex::error] = wxColor(220, 150, 150);
    bgErr[MyConst::iconIndex::fatal] = wxColor(209, 100, 100);
    bgErr[MyConst::iconIndex::panic] = wxColor(246, 10, 10);

    // load filter
    wxString tempDir = wxStandardPaths::Get().GetUserConfigDir() + wxT("\\postgresql\\");
    
    wxString f = tempDir + "filter_load.txt";
    if (wxFileExists(f)) {
        wxString str;
        wxUtfFile file(f, wxFile::read, wxFONTENCODING_UTF8);
        if (file.IsOpened()) {
            file.Read(str);
            file.Close();
            wxStringTokenizer tk(str, "\n", wxTOKEN_RET_EMPTY_ALL);
            
            bool end = false;
            LineFilter lf;
            while (tk.HasMoreTokens())
            {
                wxString l = tk.GetNextToken();
                if (end && l.IsEmpty()) continue;
                 addLineFilterStr(l);
                end = l.IsEmpty();
            }
            if (!end) addLineFilterStr("");

        }

    }
}
void Storage::saveFilters() {
    if (filterload.size() == 0) return;
    wxString tempDir = wxStandardPaths::Get().GetUserConfigDir() + wxT("\\postgresql\\");
    wxString f = tempDir + "filter_load.txt";
    wxUtfFile file(f, wxFile::write, wxFONTENCODING_UTF8);
    if (file.IsOpened())
    {
        wxString text;
        for (auto lf:filterload) {
            wxString s = LineFilterToStr(lf);

            if (!s.IsEmpty()) text.Append(s);
            text.Append("\n");
        }
        if ((file.Write(text) == 0))
            wxMessageBox(_("Query text incomplete.\nQuery contained characters that could not be converted to the local charset.\nPlease correct the data or try using UTF8 instead."));
        file.Close();
    }

}
void Storage::getLineToCache(int row, bool filter) {
    int r = row;
    if (filter && IsFilter()) row = frows[row];
    m_cacheLine = storage.at(row);
    m_cacheIndex = r;
}
int Storage::GetSeverityIndex(int rowvisual) {
    if (rowvisual != m_cacheIndex) getLineToCache(rowvisual, true);
    return m_cacheLine.icon;
}
wxColor& Storage::GetBgColorLine(int rowvisual) {
    if (rowvisual != m_cacheIndex) getLineToCache(rowvisual, true);
    int i = m_cacheLine.icon;
    return bgErr[i];
}
void Storage::DropColFilter(int index) {
    fCol.RemoveAt(index);
    fVal.RemoveAt(index);
    fFlags.RemoveAt(index);
    // оставшиеся фильтры будут применены по всему storage
    frows.clear();
}
wxString Storage::GetStringFilterExpr(int positionArrayFilter,bool addNumCol) {
    wxString expr;
    expr = _strwhere(fFlags[positionArrayFilter]);
    expr = expr + fVal[positionArrayFilter];
    if (addNumCol) {
        expr = wxString::Format("%d:%s", fCol[positionArrayFilter], expr);
        return expr;
    }
    if (expr.Len() > 50) expr = expr.substr(0, 50);
    return expr;
}
int Storage::testFilter(MyConst::colField col, int position) {
    int k = 0;
    for (auto i : fCol) {
        if (i == col && (position <= k)) return k;
        k++;
    }
    return -1;
}
bool Storage::CompareFilterLine(int row, bool filter) {
    int c = 0;
    MyConst::colField f;
    wxString valFlt;
    wxString valField;
    bool reverse = false;
    bool rez = false;
    int flags = 0;
    for (auto i : fCol) {
        flags = fFlags[c];
        reverse = (flags & FL_REVERSE) == FL_REVERSE;
        valFlt = fVal[c++];
        f = (MyConst::colField) i;
        valField = GetFieldStorage(row, f, filter);
        if ((flags & FL_CONTAINS) == 0) rez = valField != valFlt;
        else
            rez = !valField.Contains(valFlt);
        if (reverse)  rez = !rez;
        if (rez) {
            return false;
        }
    }
    // Показываем строку только если она стала новой группой
    if (IsGroupFilter())
    {
        // если проверяем не добавленную строку то никаих проверок
        if (filter) return true;

        // в детальном режиме новые группы не показываем
        if (faddgroup && detailGroup != -1) return false;
        // Если это не детальная информация то тоже не показываем
        if (detailGroup != -1 && detailGroup == prevRow && !filter) {
            // новая строка попадает в детальную группу
            // сдвинем вершину на новую строку
            detailGroup = row;
            return true;
        }

        if (!faddgroup) return false;

    }
    return true;
}
//номер строка из из отфильтрованных
void Storage::setDetailGroupRow(int rowGroup) {
    if (IsGroupFilter()) {
        // 
        if (rowGroup != -1) detailGroup = frows[rowGroup]; else detailGroup = -1;
    }
    else detailGroup = -1;
}
// для указанной строки проверяем из strage (без фильтра)
bool Storage::ApplyFilter(int row) {
    //if (!IsFilter()) return true;
    if (row != -1) {
        // verify row
        if (!IsFilter()) return true;
        // filter enable
        if (CompareFilterLine(row, false)) {
            frows.push_back(row);
            faddgroup = false;
            return true;
        }
        if (detailGroup != -1) return false;
        if (IsGroupFilter()) {
            // подменить в фильтре строк на новую 
            for (int i = 0; i < frows.size(); i++) {
                // тут потенциальная проблема производительности
                if (frows[i] != prevRow) continue;
                frows[i] = row;
                return false;
            }

        }
        // no visible
        return false;
    }
    if (!IsFilter()) { frows.clear();  return true; }

    bool f = false;
    faddgroup = true;
    if (IsGroupFilter()) {
        // при включенном GroupFilter смотрим только строки hashKeyToRow
        frows.clear();
        MyHashToRow::iterator it;
        for (it = hashKeyToRow.begin(); it != hashKeyToRow.end(); ++it)
        {
            int keyhash = it->first, lastrow = it->second;
            // do something useful with key and value
            if (detailGroup != -1) {
                if (detailGroup == lastrow)
                {
                    int l = lastrow;
                    while (l != -1) {
                        frows.push_back(l);
                        if (l != m_cacheIndex) getLineToCache(l, false);
                        l = m_cacheLine.prevRowGroup;
                    }
                    break;
                }
            }
            else
                frows.push_back(lastrow);
            //frows.push_back(keyhash);
        }
        sort(frows.begin(), frows.end());

    }
    if (frows.size() > 0) {
        // набор фильтра изменился перепроверим отфильтрованные строки ещё раз
        std::deque<int> tmp;
        for (int i = 0; i < frows.size(); i++) {
            if (CompareFilterLine(i, true)) {

                tmp.push_back(frows[i]);
                f = true;
            };

        }
        if (f) frows = tmp;
        else frows.clear();
    }
    else
    {
        // набор фильтроывнных строк пустой проверим все строки на соответствие фильтру
        for (int i = 0; i < storage.size(); i++) {
            if (CompareFilterLine(i, false)) {
                frows.push_back(i);
                f = true;
            };

        }
    }
    faddgroup = false;
    return f;
}

int Storage::SetFilter(int col, wxString& val, int flags) {
    fCol.Add(col);
    fVal.Add(val);
    fFlags.Add(flags);
    ApplyFilter();
    return frows.size();
}
wxString Storage::GetField(int row, MyConst::colField col) {
    return GetFieldStorage(row, col, IsFilter());
}
wxString Storage::get_field(Line& l, MyConst::colField col) {
    wxString val;
    switch (col)
    {
    case MyConst::colField::logtime:
        return l.text.substr(l.logtime.s, l.logtime.l);
        break;
    case MyConst::colField::loguser:
        return l.text.substr(l.loguser.s, l.loguser.l);
        break;
    case MyConst::colField::logdb:
        return l.text.substr(l.logdb.s, l.logdb.l);
        break;
    case MyConst::colField::logpid:
        return l.text.substr(l.logpid.s, l.logpid.l);
        break;
    case MyConst::colField::loghost:
        return l.text.substr(l.loghost.s, l.loghost.l);
        break;
    case MyConst::colField::logtag:
        return l.text.substr(l.logtag.s, l.logtag.l);
        break;
    case MyConst::colField::logSessiontime:
        return l.text.substr(l.logSessiontime.s, l.logSessiontime.l);
        break;
    case MyConst::colField::logSeverity:
        return l.text.substr(l.logSeverity.s, l.logSeverity.l);
        break;
    case MyConst::colField::logSqlstate:
        return l.text.substr(l.logSqlstate.s, l.logSqlstate.l);
        break;
    case MyConst::colField::logMessage:
        return l.text.substr(l.logMessage.s, l.logMessage.l);
        break;
    case MyConst::colField::logDetail:
        return l.text.substr(l.logDetail.s, l.logDetail.l);
        break;
    case MyConst::colField::logHint:
        return l.text.substr(l.logHint.s, l.logHint.l);
        break;
    case MyConst::colField::logappname:
        val = l.text.substr(l.logappname.s, l.logappname.l);
        if (val.IsEmpty()) val = l.text.substr(l.logbtype.s, l.logbtype.l);
        return val;
        break;
    case MyConst::colField::logbtype:
        return l.text.substr(l.logbtype.s, l.logbtype.l);
        break;
    case MyConst::colField::logSERVER:
        return l.text.substr(l.logSERVER.s, l.logSERVER.l);
        break;
    default:
        break;
    }
    return "bad";

}
wxString Storage::GetFieldStorage(int row, MyConst::colField col, bool filter) {
    if (row != m_cacheIndex) getLineToCache(row, filter);
    return get_field(m_cacheLine, col);
}
Line Storage::getLineParse(const wxString& str, bool csv) {
    Line st;
    if (csv) {
        CSVTokenizer tk(str);
        // Get the fields from the CSV log.

        wxString t;
        wxString logTime = tk.GetNextToken();
        //fields.Add(logTime);
        st.logtime = { 0,static_cast<unsigned short int>(logTime.Len()) };
        wxString logUser = tk.GetNextToken();
        t = logTime;
        st.loguser = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logUser.Len()) };
        t += logUser;
        wxString logDatabase = tk.GetNextToken();
        if (logDatabase.IsEmpty()) logDatabase = GetHost();
        st.logdb = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logDatabase.Len()) };
        
        t += logDatabase;
        wxString logPid = tk.GetNextToken();
        st.logpid = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logPid.Len()) };
        t += logPid;
        wxString logSession;
        wxString logCmdcount;
        wxString logSegment;
        wxString logHost = tk.GetNextToken();       // Postgres puts port with Hostname
        // delete port
        logHost = logHost.BeforeFirst(':');
        st.loghost = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logHost.Len()) };
        t += logHost;

        logSession = tk.GetNextToken();

        wxString logLineNumber = tk.GetNextToken();
        wxString logTag = tk.GetNextToken();
        st.logtag = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logTag.Len()) };
        t += logTag;
        wxString logSessiontime = tk.GetNextToken();
        st.loghost = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logHost.Len()) };
        t += logHost;

        wxString logVXid = tk.GetNextToken();
        wxString logTransaction = tk.GetNextToken();
        wxString logSeverity = tk.GetNextToken();
        //fields.Add(logSeverity);
        st.logSeverity = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logSeverity.Len()) };
        t += logSeverity;

        wxString logState = tk.GetNextToken();
        //fields.Add(logState);
        st.logSqlstate = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logState.Len()) };
        t += logState;

        wxString logMessage = tk.GetNextToken();
        //fields.Add(logMessage);
        wxString logDetail = tk.GetNextToken();
        //fields.Add(logDetail);
        st.logDetail = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logDetail.Len()) };
        t += logDetail;
        wxString logHint = tk.GetNextToken();
        //fields.Add(logHint);
        wxString logQuery = tk.GetNextToken();
        wxString logQuerypos = tk.GetNextToken();
        wxString logContext = tk.GetNextToken();
        wxString logDebug = tk.GetNextToken();
        wxString logCursorpos = tk.GetNextToken();
        if (!logDebug.IsEmpty() && logHint.IsEmpty())
            logHint = logDebug;
        st.logHint = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logHint.Len()) };
        t += logHint;
        st.logMessage = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logMessage.Len()) };
        t += logMessage;

        //location
        logCursorpos = tk.GetNextToken();

        wxString logApp = tk.GetNextToken();
        //fields.Add(logApp);
        st.logappname = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logApp.Len()) };
        t += logApp;
        wxString logType = tk.GetNextToken();
        st.logbtype = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logType.Len()) };
        t += logType;
        logCursorpos = GetHost();
        st.logSERVER = { static_cast<unsigned short int>(t.Len()),static_cast<unsigned short int>(logCursorpos.Len()) };
        t += logCursorpos;
        //fields.Add(logType);
        //st.logType = { t.Len(),logType.Len() };
        //t += logType;
        st.type = MyConst::ltype::SIMPLE_TEXT;
        int i = MyConst::iconIndex::log;
        if (logSeverity == "WARNING")  i = MyConst::iconIndex::war;
        if (logSeverity == "ERROR")  i = MyConst::iconIndex::error;
        if (logSeverity == "FATAL")  i = MyConst::iconIndex::fatal;
        if (logSeverity == "PANIC")  i = MyConst::iconIndex::panic;
        st.icon = i;
        st.visible = 1;
        st.text = t;

    }
    return st;
}
// получение обобщенной ключевой строки
wxString Storage::getStrGroup(wxString source) {
    int i = 0;
    int l = source.Length();
    wxString n;
    bool quote = false;
    while (i < l) {
        char c = source[i++];
        if (c >= '0' && c <= '9' && !quote) continue;
        if (c == '"') {

            if ((i < l) && source[i] == c) {
                n.Append(c, 2);
                //n.Append(c);
                i++;
                continue;
            }
            else
                quote = !quote;
        }
        n.Append(c);
    }
    return n;
}
int Storage::GetTotalCountGroup(int rowfilter) {
    if (rowfilter != m_cacheIndex) getLineToCache(rowfilter, true);
    MyHashToRow::const_iterator it = hashKeyTotal.find(m_cacheLine.hash);
    int a = 0;
    if (it != hashKeyTotal.end())
    {
        a = hashKeyTotal[m_cacheLine.hash];
    }
    if (a==0) {
        int h = m_cacheLine.hash;
        int lastrow=hashKeyToRow[h];
        a = 0;
        m_cacheIndex = -1;
        while (lastrow!=-1) {
            getLineToCache(lastrow, false);
            lastrow = m_cacheLine.prevRowGroup;
            a++;
        }
        hashKeyTotal[h]=a;
        m_cacheIndex = -1;
    }
    return a;
}
void Storage::ClearCount(int rowfilter) {
    if (rowfilter != m_cacheIndex) getLineToCache(rowfilter, true);
    MyHashToRow::const_iterator it = hashKeyTotal.find(m_cacheLine.hash);
    int cnt = hashKeyToCount[m_cacheLine.hash];
    int a = 0;
    if (it != hashKeyTotal.end())
    {
        a = hashKeyTotal[m_cacheLine.hash];
    }
    hashKeyTotal[m_cacheLine.hash] = a + cnt;
    hashKeyToCount[m_cacheLine.hash] = 0;
}
int Storage::getCountGroup(int row) {

    wxString valField = GetFieldStorage(row, MyConst::colField::logtime, false);
    int h = m_cacheLine.hash;
    int c = hashKeyToCount[h];
    return c;
}
bool Storage::checkFilter(Line& l) {

    if (filterload.size() > 0) {
        wxString sf;
        bool rez=true;
        int last = filterload.size() - 1;
        int i = 0;
        for (auto fl:filterload) {
            if (fl.col == -1) {
                if (rez || (last==i)) break;
                rez = true;
            }
            else {
                if (rez) {
                    sf = get_field(l, (MyConst::colField)fl.col);
                    int flags = fl.flags;
                    bool reverse = (flags & FL_REVERSE) == FL_REVERSE;

                    if ((flags & FL_CONTAINS) == 0) rez = sf == fl.val;
                    else
                        rez = sf.Contains(fl.val);
                    if (reverse)  rez = !rez;
                }
            }
            i++;

        }
        return rez;
    }
    return false;
}
bool Storage::AddLineTextCSV(const wxString& strcsv) {
    Line st = getLineParse(strcsv, true);

    if (checkFilter(st)) {
        rowsignore++;
        return false;
    }
    rowsadd++;
    wxString msg = st.text.substr(st.logMessage.s, st.logMessage.l);
    if (st.icon > MyConst::iconIndex::war) SetErrMsgFlag(true);

    wxString gstr = getStrGroup(msg);
    int h = getHashString(gstr);
    MyHashToRow::const_iterator it = hashKeyToRow.find(h);
    prevRow = -1;
    int cnt = 1;
    faddgroup = true;
    if (it != hashKeyToRow.end())
    {
        faddgroup = false;
        prevRow = it->second;
        cnt = hashKeyToCount[h];
        cnt++;
    }
    int nextrow = storage.size();
    st.prevRowGroup = prevRow;
    st.hash = h;
    hashKeyToRow[h] = nextrow;
    hashKeyToCount[h] = cnt;
    storage.push_back(st);
    return true;
}


