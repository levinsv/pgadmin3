
#ifndef FUNCTIONPGHELPER_H
#define FUNCTIONPGHELPER_H
#include <utils/sysSettings.h>
#include <wx/regex.h>
#include <map>
#include <vector>
#include <wx/stdpaths.h>
#include <wx/textfile.h>
#include <wx/filename.h>

extern sysSettings *settings;

class FunctionPGHelper
{
public:
    FunctionPGHelper() {};
    wxString getHelpString(wxString fnd, bool isPart = true) {
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
        return txt;
    }
    wxString getSqlCommandHelp(wxString fnd) {
        wxUniChar sep = wxFileName::GetPathSeparator();
        fnd.Replace(" ", "");
        wxString f = wxFindFirstFile(path + sep+"sql-"+fnd+"*.html");
        wxString last,txt;
        
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
        else if (c==1) {
            return getHelpFile(last);
        }
        else {
            return txt;
        }

    }
    wxString getHelpFile(wxString filename) {
        wxString tempDir = path + wxFileName::GetPathSeparator() + filename;
        if (!wxFileExists(tempDir)) return wxEmptyString;
        wxTextFile  tfile;
        tfile.Open(tempDir);
        // read the first line
        wxString str, sbody;
        sbody = tfile.GetFirstLine();
        bool flag = true;
        wxRegEx b("(<body .*?>)");
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
            sbody += str;
        }
        return sbody;

    }
    bool isValid() {
        if (!isload) loadfile();
        return isload;
    }
private:
    bool isload = false;
    wxString path;
    std::map<wxString, wxString> body;
    void loadfile() {
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

};

#endif
