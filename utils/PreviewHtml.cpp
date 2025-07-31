#include "pgAdmin3.h"
#include "utils/PreviewHtml.h"
#include "utils/json/jsonval.h"
#include "utils/csvfiles.h"
#include "log/Storage.h"

void PreviewHtml::InitColor() {
    wxJSONValue def(wxJSONType::wxJSONTYPE_OBJECT);
    def.SetType(wxJSONType::wxJSONTYPE_NULL);
    wxJSONValue opt(wxJSONType::wxJSONTYPE_OBJECT);
    opt.SetType(wxJSONType::wxJSONTYPE_NULL);
    extern sysSettings* settings;
    settings->ReadJsonObect("PreviewOptions", opt, def);

    //    settings->WriteJsonFile();
    if (!opt.IsNull()) {
        bgcolor = opt["bgcolor"].AsString();
        fgcolor = opt["fgcolor"].AsString();
        numcolor = opt["numcolor"].AsString();
        quotecolor = opt["quotecolor"].AsString();
        //fname = opt["fontname"].AsString();
        //fsize=opt["fontsize"].AsInt();

    }
    else {
        wxColour frColor = settings->GetSQLBoxColourForeground();
        if (settings->GetSQLBoxUseSystemForeground())
        {
            frColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        }
        fgcolor=frColor.GetAsString(wxC2S_HTML_SYNTAX);
        opt["fgcolor"] = fgcolor;
        bgcolor = "#FFFFE0";
        //4
        numcolor = settings->GetSQLBoxColour(4);
        wxColour cc(numcolor);
        if (cc.IsOk()) numcolor = cc.GetAsString(wxC2S_HTML_SYNTAX);
        //6
        quotecolor = settings->GetSQLBoxColour(6);
        wxColour ccc(quotecolor);
        if (ccc.IsOk()) quotecolor = ccc.GetAsString(wxC2S_HTML_SYNTAX);
       // wxFont fnt = settings->GetSQLFont();
      //  fname = fnt.GetFaceName();
      //  fsize = fnt.GetPointSize();
        //fnt.SetPointSize(fsize);

        opt["fgcolor"] = fgcolor;
        opt["bgcolor"] = bgcolor;
        opt["numcolor"] = numcolor;
        opt["quotecolor"] = quotecolor;
      //  opt["fontname"] = fname;
     //   opt["fontsize"]=fsize;
        settings->WriteJsonObect("PreviewOptions", opt);
    }
}
wxString PreviewHtml::Preview(const wxString& txt, fmtpreview type) {
    fmtpreview fmt = type;
    wxString fieldSep=",";
    wxString rowSep = "\n";
    bool iscsv = false;
    if (txt.StartsWith("automatic vacuum")&& fmt == fmtpreview::AUTO) fmt = fmtpreview::AUTOVACCUM;
    if (txt.StartsWith("automatic analyze") && fmt == fmtpreview::AUTO) fmt = fmtpreview::AUTOVACCUM;

// 

    if (fmt == fmtpreview::AUTOVACCUM) {
        fieldSep = ",";
        rowSep = "\n";
    }
    else if (fmt == fmtpreview::CSV) {
        iscsv = true;
    }
    else if (fmt == fmtpreview::AUTO) {
        //CSVTokenizer tk(txt);
        //int nfield = 0;
        //while (tk.HasMoreTokens()) {
        //    wxString field = tk.GetNextToken();
        //    nfield++;
        //}
        //if (nfield>1) iscsv = true;
    }
    // prepare csv 
    std::vector<wxString> strlist;
    wxString tmpstr = txt;
    if (iscsv) {
        CSVTokenizer tk(txt);
        tmpstr = "";
        int nfield = 0;
        while (tk.HasMoreTokens()) {
            wxString field = tk.GetNextToken();
            
            if (!field.IsEmpty()) {
                if (!tmpstr.IsEmpty()) tmpstr += '\n'; // All not empty field as list rows
                tmpstr += field;
            }
            strlist.push_back(field);
            nfield++;
        }
        if (nfield == 26) {
            // log db csv format 14,15,16,17 version
            Line l=Storage::getLineParse(txt, true, "");
            strlist[15] = Storage::get_field(l, MyConst::colField::logHint); // bind value
            strlist[13] = Storage::get_field(l, MyConst::colField::logMessage); // bind value
        }
    
    }
    if (strlist.size() == 0) strlist.push_back(tmpstr);
    wxString html;
    int nf = 0;
    for (int nf = 0; nf < strlist.size();nf++) {

        tmpstr = strlist[nf];
        int pos = 0;
        int len = tmpstr.Length();
        if (len == 0) continue;
        wxUniChar c;
        wxString currWord;
        wxString currDigits;
        wxString currSep;
        int flag = 0;
        tokens.clear();
        bool quote = false;
        wxUniChar prevchar;
        int startstr = -1;
        while (pos < len) {
            c = tmpstr[pos++];
            bool isquote = c == '"';
            if (quote) {
                if (prevchar == c && isquote) {
                    // repeat quote
                    prevchar = '\0';
                    continue;
                }
                if (prevchar == '"' && !isquote) {
                    // end quote string
                    wxString tmp = tmpstr.Mid(startstr, pos - startstr - 1);
                    saveTokenIfNotEmpty(tmp, PREVIEW_QUOTE);
                    quote = false;
                }
                else {
                    prevchar = c;
                    continue;
                }
            }
            if (quote == false && isquote) {
                saveTokenIfNotEmpty(currWord, PREVIEW_WORD);
                saveTokenIfNotEmpty(currDigits, PREVIEW_DIGITS);
                saveTokenIfNotEmpty(currSep, PREVIEW_SEP);
                quote = true;
                prevchar = '\0';
                startstr = pos - 1;
                continue;
            }
            if (fieldSep.Length() > 0 && fieldSep[0] == c) {
                //flag |= PREVIEW_ENDFIELD;
                saveTokenIfNotEmpty(currWord, PREVIEW_WORD);
                saveTokenIfNotEmpty(currDigits, PREVIEW_DIGITS);
                saveTokenIfNotEmpty(currSep, PREVIEW_SEP);
                currSep = c;
                saveTokenIfNotEmpty(currSep, PREVIEW_ENDFIELD);
                continue;
            }
            else if (rowSep.Length() > 0 && rowSep[0] == c) {
                //flag |= PREVIEW_ENDROW;
                saveTokenIfNotEmpty(currWord, PREVIEW_WORD);
                saveTokenIfNotEmpty(currDigits, PREVIEW_DIGITS);
                saveTokenIfNotEmpty(currSep, PREVIEW_SEP);
                currSep = c;
                saveTokenIfNotEmpty(currSep, PREVIEW_ENDROW);
                continue;
            }

            if ((c >= '0' && c <= '9') || (currDigits.Length() > 0 && c == '.')) {
                if (currWord.Length() == 0) { // diget not after word
                    saveTokenIfNotEmpty(currSep, PREVIEW_SEP);
                    currDigits += c;
                    continue;
                }
            }
            if (c == '-' || c == '+' || c == '.' || c == ',' ||
                c == ':' || c == '~' ||
                c == '@' || c == '*' || c == '/' ||
                c == '=' || c == '!' || c == '#' ||
                c == '&' || c == '%' || c == '^' ||
                c == '|' || c == '`' || c == '?' ||
                c == '>' || c == '<'
                || c == ';' || c == '_' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}'
                || c == ' '
                )
            {
                saveTokenIfNotEmpty(currWord, PREVIEW_WORD);
                saveTokenIfNotEmpty(currDigits, PREVIEW_DIGITS);
                currSep += c;
                continue;
            }
            saveTokenIfNotEmpty(currSep, PREVIEW_SEP);
            if (currDigits.Length() > 0) {
                currWord = currDigits + currWord;
                currDigits = "";
            }
            currWord += c;
        }
        // last element
        if (quote) {
            wxString tmp = tmpstr.Mid(startstr);
            saveTokenIfNotEmpty(tmp, PREVIEW_QUOTE);
        }
        saveTokenIfNotEmpty(currWord, PREVIEW_WORD);
        saveTokenIfNotEmpty(currDigits, PREVIEW_DIGITS);
        saveTokenIfNotEmpty(currSep, PREVIEW_SEP);


        // Additonal styled
        wxString findstr = "";
        int p = 0;
        while (p >= 0) {
            int fp = FindElement(p, PREVIEW_ENDFIELD | PREVIEW_ENDROW, findstr, 1);
            if (fp > 0) {
                int pp = fp;
                while (pp > 0 && pp > p) {
                    pp--;
                    Element t2 = tokens[pp];
                    if (CHKFLAG(t2.flags, PREVIEW_SEP) && t2.src.StartsWith(":")) {
                        // Right bound title
                        if (pp - p > 0) {
                            tokens[p].html = "<b>" + tokens[p].html;
                            tokens[pp - 1].html = tokens[pp - 1].html + "</b>";
                        }
                    }
                }
                fp++; // next field 
                p = fp;
            }
            else break;
        }

        if (strlist.size() > 1) {
            // csv log 
            wxString tit="field"+wxString::Format("%d",nf+1);
            if (strlist.size() == 26) tit = titles_log[nf];
            tit = "<tr><td valign=\"top\"><b>" + tit + "</b></td>";
            html=html+tit+"<td>" + generateHtml() + "</td></tr>";

        } else 
            html+= generateHtml(); // tokens -> html
    }
    if (strlist.size() > 1) html = "<table CELLPADDING=\"1\">" + html + "</table>";
    wxString ttt = wxString::Format("<html><body BGCOLOR=\"%s\" FGCOLOR=\"%s\">%s</body></html>", bgcolor, fgcolor, html);
    return ttt;
}