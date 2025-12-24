#pragma once
#include <wx/wx.h>

#define MAX_TEXT_LEN_COLORIZE 32000
#define MAX_TEXT_LEN_WARNING 500000
#define PREVIEW_SEP 1
#define PREVIEW_DIGITS 2
#define PREVIEW_WORD 4
#define PREVIEW_SPACE 8
#define PREVIEW_ENDFIELD 16
#define PREVIEW_ENDROW 32
#define PREVIEW_QUOTE 64
#define CHKFLAG2(val,par) ((val & par)>0)
enum class fmtpreview {
    AUTO, AUTOVACCUM,CSV,SIMPLE_TEXT
};

struct Element {
    wxString src;
    wxString html;
    int flags = 0;

} ;
static wxString titles_log[] = {
    L"log_time",
L"user_name",
L"database_name",
L"process_id",
L"connection_from",
L"session_id",
L"session_line_num",
L"command_tag",
L"session_start_time",
L"virtual_transaction_id",
L"transaction_id",
L"error_severity",
L"sql_state_code",
L"message",
L"detail",
L"hint",
L"internal_query",
L"internal_query_pos",
L"context",
L"query",
L"query_pos",
L"location",
L"application_name",
L"backend_type",
L"leader_pid",
L"query_id"
};

class PreviewHtml
{
public:
    //void SetColors();
    PreviewHtml() { InitColor(); };
    wxString Preview(const wxString& txt, fmtpreview type);
private:
    void InitColor();
    bool saveTokenIfNotEmpty(wxString& savestr, int flag) {
        if (savestr.Length() > 0) {
            wxString tmp = savestr;
            tmp=escapeHtml(tmp,true);
            tmp.Replace(" ", "&nbsp;");
            if (CHKFLAG2(flag, PREVIEW_ENDROW)) tmp = "<br>";
            if (CHKFLAG2(flag, PREVIEW_DIGITS)) {
                int l = savestr.Length();
                if (l > 4 && !savestr.Contains('.')) {
                    int dl = 3;
                    wxString fmt;
                    std::vector<wxString> dd;
                    bool smalll = true;
                    while (l > 0) {
                        l = l - dl;
                        if (l < 0) {
                            dl = dl + l;
                            l = 0;
                        }
                        wxString d3 = savestr.Mid(l, dl);
                        if (smalll)
                            dd.push_back("<font size=+1>" + d3 + "</font>");
                        else 
                            dd.push_back(d3);
                        smalll = !smalll;
                    }
                    for (auto i = dd.rbegin(); i != dd.rend(); i++)
                     {
                        fmt += *i;
                    }
                    tmp = fmt;
               }
                wxString t = wxString::Format("<font color=\"%s\">%s</font>", numcolor, tmp);
                tmp = t;

            }
            if (CHKFLAG2(flag, PREVIEW_QUOTE)) {
                wxString t = wxString::Format("<font color=\"%s\">%s</font>",quotecolor,tmp);
                tmp = t;
            }
            tokens.push_back({savestr,tmp,flag});
            savestr = "";
            return true;
        }
        return false;
    }
    int FindElement(int start_pos, int flag_find, wxString& value_find, int is_what_find) {
        int p = start_pos;
        int rez = -1;
        while (p < tokens.size()) {
            Element t = tokens[p];
            bool f1 = is_what_find & 1;
            bool f2 = is_what_find & 2;
            bool r1 = false;
            bool r2 = false;
            if (f1 && CHKFLAG2(t.flags, flag_find)) {
                r1 = true;
            }
            if (f2 && t.src==value_find ) {
                r2 = true;
            }
            if ((is_what_find == 3 && r1 && r2)
                || (is_what_find == 2 && r2)
                || (is_what_find == 1 && r1)
                ) {
                rez = p;
                break;
            }
            p++;
        }
        return rez;
    }
    wxString generateHtml() {
        wxString s;
        for (size_t i = 0; i < tokens.size(); i++) {
            Element t = tokens[i];
            s += t.html;
        }
       // s = "<html><body BGCOLOR=\"" + bgcolor + "\">" + s + "</body></html>";
//        s=wxString::Format("<html><body BGCOLOR=\"%s\" FGCOLOR=\"%s\"><font face=\"%s\" size=\"%d\">%s</font></body></html>", bgcolor,fgcolor,fname,fsize, s);
        

        return s;
    }
    std::vector<Element> tokens;
    // colors
    wxString bgcolor, numcolor, fgcolor, quotecolor;
    wxString fname;
    int fsize;
};

