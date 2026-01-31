#include "pgAdmin3.h"
#include "ctl/ctlNavigatePanel.h"
#include <wx/listctrl.h>
#include <wx/dcbuffer.h>
#include <wx/debug.h>
#include "utils/json/jsonval.h"
#include "utils/csvfiles.h"
//#include <wx/window.h>

BEGIN_EVENT_TABLE(ctlNavigatePanel, wxWindow)

EVT_MOTION(ctlNavigatePanel::OnMouse)
EVT_LEFT_UP(ctlNavigatePanel::mouseReleased)
EVT_RIGHT_UP(ctlNavigatePanel::mouseReleased)
EVT_SIZE(ctlNavigatePanel::OnSize)
EVT_PAINT(ctlNavigatePanel::OnPaint)
EVT_MENU(wxID_ANY,ctlNavigatePanel::OnContextMenu)
//EVT_SET_FOCUS(ctlNavigatePanel::OnSetFocus)
//EVT_KILL_FOCUS(ctlNavigatePanel::OnKillFocus)

END_EVENT_TABLE()

void ctlNavigatePanel::Init(bool reorganization) {
    wxJSONValue def(wxJSONType::wxJSONTYPE_OBJECT);
    startdbcolor = wxColour("#f655ee");
    if (!reorganization) {
        // run construction
        def["width"] = 20;
        def["border"] = 1;
        bgcolor = *wxLIGHT_GREY;
        framecolor = *wxBLACK;
        bordercolor = *wxBLACK;
        findcolor = *wxWHITE;
        def["startdbcolor"]= startdbcolor.GetAsString(wxC2S_HTML_SYNTAX);
        def["backgroundcolor"] = bgcolor.GetAsString(wxC2S_HTML_SYNTAX);
        def["framecolor"] = framecolor.GetAsString(wxC2S_HTML_SYNTAX);
        def["bordercolor"] = bordercolor.GetAsString(wxC2S_HTML_SYNTAX);
        def["findcolor"] = findcolor.GetAsString(wxC2S_HTML_SYNTAX);
        def["contextboxsize"] = 12;
        def["findmarkwidthprocent"] = 50;
        def["findmarkwidthprocent"].AddComment("// width find market % value range 0..100 ");
        wxJSONValue cmds(wxJSONType::wxJSONTYPE_ARRAY);
        wxJSONValue cm(wxJSONType::wxJSONTYPE_OBJECT);
        cm["name"] = wxString("Find");
        cm["hotkey"] = wxString("Ctrl+F");
        cm["description"] = wxString(_("Search for a simple string."));
        cmds.Append(cm);
        wxJSONValue cm2(wxJSONType::wxJSONTYPE_OBJECT);
        cm2["name"] = wxString("GotoNextMark");
        cm2["description"] = wxString(_("Go to the next marker."));
        cm2["hotkey"] = wxString("F5");
        cmds.Append(cm2);
        wxJSONValue cm3(wxJSONType::wxJSONTYPE_OBJECT);
        cm3["name"] = wxString("GotoNextMarkEqual");
        cm3["hotkey"] = wxString("Shift+F5");
        cm3["description"] = wxString(_("Go to the next marker of the same type."));
        cmds.Append(cm3);
        wxJSONValue cm4(wxJSONType::wxJSONTYPE_OBJECT);
        cm4["name"] = wxString("FindNext");
        cm4["hotkey"] = wxString("F3");
        cm4["description"] = wxString(_("The next occurrence of the string."));
        cmds.Append(cm4);
        wxJSONValue cm5(wxJSONType::wxJSONTYPE_OBJECT);
        cm5["name"] = wxString("FindPrevious");
        cm5["hotkey"] = wxString("Shift+F3");
        cm5["description"] = wxString(_("Previous occurrence of the string."));
        cmds.Append(cm5);
        wxJSONValue cm7(wxJSONType::wxJSONTYPE_OBJECT);
        cm7["name"] = wxString("FindPid");
        cm7["hotkey"] = wxString("F7");
        cm7["description"] = wxString(_("Find all strings content current row Pid."));
        cmds.Append(cm7);
        wxJSONValue cm8(wxJSONType::wxJSONTYPE_OBJECT);
        cm8["name"] = wxString("FindState");
        cm8["hotkey"] = wxString("F8");
        cm8["description"] = wxString(_("Find all strings content current row State."));
        cmds.Append(cm8);



        wxJSONValue cm6(wxJSONType::wxJSONTYPE_OBJECT);
        cm6["name"] = wxString(_("Help"));
        cm6["hotkey"] = wxString("F1");
        cmds.Append(cm6);

        def["commands"] = cmds;
        wxJSONValue markdef(wxJSONType::wxJSONTYPE_ARRAY);
        enum LogIndexColor : int { panic, fatal, errorxx, error, startup, logxx, notfound };
        const wxColour def_mark_color[7] = { *wxRED, wxColour(255,125,125) ,wxColour(255,213,101),  *wxYELLOW, *wxCYAN, wxColour(196,255,14), wxColour(66,76,231) };
        //                                panic    fatal ,             errorxx                   error,     startup  LOG,XX000  WARNIG,01000
        //                                red      light red           light brown               yellow     cyan     lime       blue
        for (int k = 0; k < sizeof(def_mark_color)/sizeof(def_mark_color[0]); k++) {
            wxJSONValue indicator(wxJSONType::wxJSONTYPE_OBJECT);
            indicator["color"] = def_mark_color[k].GetAsString(wxC2S_HTML_SYNTAX);
            indicator["enable"] = true;
            //indicator["regexp"] = false;
            wxJSONValue inc(wxJSONType::wxJSONTYPE_ARRAY);
            wxJSONValue exclude(wxJSONType::wxJSONTYPE_ARRAY);
            if (k == 0) {
                inc.Append(wxString(",PANIC,"));
                indicator["menulabel"] = wxString("Panic");
            }
            if (k == 1) {
                inc.Append(wxString(",FATAL,"));
                exclude.Append(wxString("/,57P03,/"));
                //indicator["regexp"] = true;
                indicator["menulabel"] = wxString("Fatal -57P03");
            }
            if (k == 2) {
                inc.Append(wxString(",ERROR,XX000,"));
                indicator["menulabel"] = wxString("ERROR,XX000");
            }
            if (k == 3) {
                inc.Append(wxString(",ERROR,"));
                exclude.Append(wxString(",25P02,"));
                indicator["menulabel"] = wxString("ERROR -25P02");
            }
            if (k == 4) {
                inc.Append(wxString(",\"startup\","));
                inc.Append(wxString(",\"postmaster\","));
                indicator["menulabel"] = wxString("Start/Shutdown");
            }
            if (k == 5) {
                inc.Append(wxString("LOG,XX0"));
                exclude.Append(wxString(" Connection timed out"));
                indicator["menulabel"] = wxString("LOG,XX0");
            }
            if (k == 6) {
                inc.Append(wxString("WARNING,01000"));
                indicator["menulabel"] = wxString("WARNING,01000");
            }
            
            indicator["include"] = inc;
            indicator["exclude"] = exclude;
            markdef.Append(indicator);
        }
        markdef.AddComment("/*\ninclude - any string match\nexclude - excluding these string\n*/");
        def["indicators"] = markdef;
        settings->ReadJsonObect("LogNavigatePanel", opt, def);

        //    settings->WriteJsonFile();
        if (!opt.IsNull()) {
            width = opt["width"].AsInt32();
            border = opt["border"].AsInt32();
            wxString strcolor = opt["backgroundcolor"].AsString();
            wxColour cc(strcolor);
            if (cc.IsOk()) bgcolor = cc;
            strcolor = opt["framecolor"].AsString();
            wxColour cc2(strcolor);
            if (cc2.IsOk()) framecolor = cc2;
            strcolor = opt["bordercolor"].AsString();
            wxColour cc3(strcolor);
            if (cc3.IsOk()) bordercolor = cc3;
            strcolor = opt["findcolor"].AsString();
            wxColour cc4(strcolor);
            if (cc4.IsOk()) findcolor = cc4;
            strcolor=opt["startdbcolor"].AsString();
            if (strcolor != "null") {
                wxColour cc4(strcolor);
                if (cc4.IsOk()) startdbcolor = cc4;

            }
            int pr = opt["findmarkwidthprocent"].AsInt32();
            if (pr >= 0 && pr <= 100) {
                findwidth = pr;
            }
        }
        else opt = def;
    }

    wxJSONValue r = opt["indicators"];
    if (!reorganization) mark_color.clear();
    for (int i = 0; i < regExArray.size(); i++) 
        if (!regExArray[i]) delete regExArray[i];
    regExArray.clear();
    search_rule.clear();
    if (!r.IsNull()) {
        int idx = 0;
        int numIndicator = 0;
        for (int j = 0; j < r.Size(); j++) {
            wxJSONValue indicator = r[j];
            bool enable = indicator["enable"].AsBool();
            if (true) {
                wxString strcolor = indicator["color"].AsString();
                wxColour cc(strcolor);
                statistics_mark el{0,enable,cc};
                if (!reorganization) {
                    if (cc.IsOk())
                        mark_color.push_back(el);
                    else
                        mark_color.push_back(statistics_mark { 0, false,*wxBLACK });
                }
                //search_rule
                wxJSONValue inc = indicator["include"];
                wxJSONValue exc = indicator["exclude"];
                int szi = inc.Size();
                int sze = exc.Size();
                int nextIndicatorPos = idx + szi + sze;
                int excludePos = idx + szi;
                if (sze == 0) excludePos = -1;
                //find_s ff{ str,-1,-1,j};
                if (szi > 0) {
                    for (int k = 0; k < inc.Size(); k++) {
                        wxString s = inc[k].AsString();
                        wxRegEx* reg = NULL;
                            if (s.length() > 2 && s[0] == '/' && s[s.length() - 1] == '/') {
                                    wxString s2 = s.substr(1, s.length() - 2);
                                    reg = new wxRegEx(s2, wxRE_NEWLINE);
                                    if (!reg->IsValid()) {
                                            delete reg;
                                            reg = NULL;
                                            wxLogWarning(wxString::Format("Regular expression \"%s\" error. Indicator name=%s\n", s2, indicator["menulabel"].AsString()));
                                    }
                                    regExArray.push_back(reg);
                            }
                            else regExArray.push_back(reg);
                        search_rule.push_back({ s, enable ? nextIndicatorPos: -1,excludePos,numIndicator });
                    }
                    // exclude
                    for (int k = 0; k < exc.Size(); k++) {
                        wxString s = exc[k].AsString();
                        wxRegEx* reg = NULL;
                        if (s.length() > 2 && s[0] == '/' && s[s.length() - 1] == '/') {
                            wxString s2 = s.substr(1, s.length() - 2);
                            reg = new wxRegEx(s2, wxRE_NEWLINE);
                            if (!reg->IsValid()) {
                                delete reg;
                                reg = NULL;
                                wxLogWarning(wxString::Format("Regular expression \"%s\" error. Indicator name=%s\n", s2, indicator["menulabel"].AsString()));
                            }
                            regExArray.push_back(reg);
                        }
                        else regExArray.push_back(reg);
                        search_rule.push_back({ s,enable ? nextIndicatorPos : -1,nextIndicatorPos,numIndicator });
                    }
                    idx = nextIndicatorPos;
                }
                numIndicator++;
            }
        }
    }

}
wxMenu* ctlNavigatePanel::GetPopupMenu() {
    int Idcmd=MNU_MARK + 500;
    
    wxJSONValue cmds = opt["commands"];
    //wxAcceleratorEntry acc(event.GetModifiers(), event.GetKeyCode());
    if (!cmds.IsNull()) {
        wxMenu* m = new wxMenu();
        for (int j = 0; j < cmds.Size(); j++) {
            wxJSONValue c = cmds[j];
            wxString hotkey = c["hotkey"].AsString();
            wxString cmdName = c["name"].AsString();
            wxString desc = c["description"].AsString();
            if (desc == "null") desc = cmdName;
            wxString menulabel = c["menulabel"].AsString();
            if (menulabel == "null") menulabel = cmdName;

            //wxAcceleratorEntry* ac = wxAcceleratorEntry::Create(cmdName + '\t' + hotkey);
            m->Append(Idcmd, menulabel + '\t' + hotkey, desc);
            Idcmd++;
        }
        return m;
    } 
    return NULL;
}
void ctlNavigatePanel::SetFindString(const wxString& findstr) {
    logFindString = findstr;
    items_find.clear();
    FindText(logFindString, FOCUSNEXT, false);
    Refresh();
}

bool ctlNavigatePanel::RunKeyCommand(wxKeyEvent& event,int numCmd) {
    //wxAcceleratorEntry::ParseAccel(const wxString & text, int* flagsOut, int* keyOut);
    wxJSONValue cmds = opt["commands"];
    wxAcceleratorEntry acc(event.GetModifiers(), event.GetKeyCode());
    if (!cmds.IsNull()) {
        wxString helpstr;
        bool help = false;
        for (int j = 0; j < cmds.Size(); j++) {
            wxJSONValue c = cmds[j];
            wxString hotkey = c["hotkey"].AsString();
            wxString cmdName = c["name"].AsString();
            wxString desc = c["description"].AsString();
            if (desc == "null") desc = cmdName;
            wxAcceleratorEntry* ac = wxAcceleratorEntry::Create(cmdName+'\t'+hotkey);
            if (ac == NULL) {
                wxMessageBox(wxString::Format(_("Incorrect hotkey \"%s\" for command \"%s\""), hotkey, cmdName));
                continue;
            }
            wxString translite=wxGetTranslation(desc);
            helpstr += wxString::Format("%s \t\t- %s\n",hotkey,translite);
            bool isok = *ac == acc;
            if (numCmd != -1) {
                isok = j == numCmd;
            }
            delete ac;
            bool shift = false;
            bool directionUp = false;
            if (isok && cmdName == "Help") help = true;
            if (isok && cmdName == "FindNext") {
                // F3
                if (logFindString.IsEmpty()) cmdName = "Find";
                else
                {
                    FindText(logFindString, FOCUSNEXT, directionUp);
                    return true;
                }
            }
            if (isok && cmdName == "FindPrevious") {
                // Shift+F3
                directionUp = !directionUp;
                if (logFindString.IsEmpty()) cmdName = "Find";
                else
                {
                    FindText(logFindString, FOCUSNEXT, directionUp);
                    return true;
                }
            }
            if (isok && cmdName=="Find") {
                // 
                wxTextEntryDialog dialog(this,
                    _("Please enter find string\n")
                    ,
                    _("Find"),
                    logFindString,
                    wxOK | wxCANCEL);		//setName( dlg.GetValue().wc_str() );
                if (dialog.ShowModal() == wxID_OK) {
                    logFindString = dialog.GetValue();
                    items_find.clear();
                    if (logFindString.IsEmpty()) {
                        Refresh();
                        return true;
                    }
                }
                else
                {
                    return true;
                }
                FindText(logFindString, FOCUSNEXT, directionUp);
                Refresh();
                return true;
            }

            if (isok && cmdName == "GotoNextMarkEqual") {
                shift = true;
                cmdName = "GotoNextMark";
            }
            if (isok && cmdName == "GotoNextMark") {
                // goto marker

                long currentrow = ctrl->GetFocusedItem();
                if (currentrow < 0) currentrow = 0;
                const wxColour currCol = ctrl->GetItemBackgroundColour(currentrow);
                int coloridx = GetIndexColor(currCol);
                if (shift && coloridx == -1 && lastUseMark != -1) coloridx = lastUseMark;
                if (!shift && coloridx == -1 && lastUseMark != -1) coloridx = lastUseMark;
                long rowmark;
                long last = -1;
                int mark_clr;
                for (int i = 0; i < GetCountMark(); i++) {
                    rowmark = GetItemMark(i);
                    mark_clr = color_items_mark[i];

                    if (rowmark <= currentrow || !mark_color[mark_clr].enable) continue;
                    last = i;
                    if (shift) {
                        //GetIndexColor(ctrl->GetItemBackgroundColour(rowmark))
                        if (coloridx >= 0 && mark_clr == coloridx) {
                            break;
                        }
                        else {
                            last = -1;
                            continue;
                        }
                    }
                    break;
                }
                if (last == -1) {
                    wxBell();
                }
                else {
                    lastUseMark = mark_clr;
                    ctrl->Focus(rowmark);
                    RowVisibleCenter(rowmark);
                }
                return true;
            }
            int ncmd = 0;
            if (isok && (cmdName == "FindPid")) ncmd = 1;
            if (isok && (cmdName == "FindState")) ncmd = 2;
            if (isok && (ncmd>0)) {
                long itemFocus = ctrl->GetFocusedItem();
                wxString str = ctrl->GetTextLong(itemFocus);
                CSVTokenizer tk(str);

                // Get the fields from the CSV log.
                wxString logTime = tk.GetNextToken();
                wxString logUser = tk.GetNextToken();
                wxString logDatabase = tk.GetNextToken();
                wxString logPid = tk.GetNextToken();
                wxString logSession;
                wxString logHost = tk.GetNextToken();       // Postgres puts port with Hostname
                logSession = tk.GetNextToken();
                wxString logLineNumber = tk.GetNextToken();
                wxString logCommandTag = tk.GetNextToken();
                wxString logSessionStart = tk.GetNextToken();
                wxString logVXid = tk.GetNextToken();
                wxString logTransaction = tk.GetNextToken();

                wxString logSeverity = tk.GetNextToken();
                wxString logState = tk.GetNextToken();
                wxString logMessage = tk.GetNextToken();
                wxString logDetail = tk.GetNextToken();
                wxString logHint = tk.GetNextToken();
                wxString logIntQuery = tk.GetNextToken();
                wxString logIntQuerypos = tk.GetNextToken();
                wxString logContext = tk.GetNextToken();
                wxString logQuery = tk.GetNextToken();
                wxString logQuerypos = tk.GetNextToken();
                wxString logLocation = tk.GetNextToken();

                wxString logAppName = tk.GetNextToken();
                wxString logBackendType = tk.GetNextToken();
                wxString logLeaderPid = tk.GetNextToken();
                wxString logQid = tk.GetNextToken();
                if (ncmd==1) logFindString = "," + logPid + ",";
                else if (ncmd==2) logFindString = "," + logState + ",";
                items_find.clear();
                FindText(logFindString, FOCUSNEXT, directionUp);
                Refresh();
                return true;
            }

        }
        if (help) {
            wxMessageBox(helpstr, _("Help"));
            return true;
        }
    }
    return false;
}
ctlNavigatePanel::~ctlNavigatePanel() {
    for (int i = 0; i < regExArray.size(); i++) if (regExArray[i]) delete regExArray[i];
}
ctlNavigatePanel::ctlNavigatePanel(wxWindow* parent, ctlListView* lst) : wxWindow(parent, wxID_ANY)
{
    settings->ReloadJsonFileIfNeed();
    Init(false);

    this->SetBackgroundStyle(wxBG_STYLE_PAINT);
    //bgColor = lst->GetBackgroundColour();
    ctrl = lst;
    ctrl->SetFocus();
    DisableFocusFromKeyboard();
//    ctrl->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent& event) {
//        Refresh();
//        });
    ctrl->Bind(wxEVT_PAINT, [&](wxPaintEvent& event) {
        long toprow = ctrl->GetTopItem();
        if (topvisible != toprow) {
            Refresh();
            topvisible = toprow;
        }
        });

    wxSize sz(width, -1);
    this->SetMinSize(sz);

}
wxSize ctlNavigatePanel::DoGetBestSize() const
{
    wxSize psz = GetParent()->GetClientSize();
    return wxSize(width, psz.GetHeight() - 2*border);
}
int ctlNavigatePanel::GetIndexColor(const wxColour &color) const {
    int idx = 0;
    int sz = mark_color.size();
    for (int i1 = 0; i1 < sz; i1++) {
        if (color.GetRGB() == mark_color[i1].color.GetRGB()) break;
        idx++;
        if (idx == sz) {
           /// wxTrap();
            return -1;
        }
    }
return idx;
}
void ctlNavigatePanel::render(wxDC& dc) {
    wxColour bg = ctrl->GetBackgroundColour();

    dc.SetBrush(bgcolor);
    // wxRect r=dc.GetWindow()->GetClientRect();
    wxRect r = GetClientRect();
    wxSize sz = r.GetSize();
    //dc.DrawRectangle(0, 0, r.width, r.height);
    {
        wxDCPenChanger npen(dc, wxPen(bordercolor, border, wxPENSTYLE_SOLID));
        dc.DrawRectangle(border/2, border/2, sz.x-border/2, sz.y-border/2);
    }

    int page = ctrl->GetCountPerPage();
    wxRect c = GetClientRect();
    int vpix = c.GetHeight() - 2*border;
    long cnt = ctrl->GetItemCount();
    float f_frame_start = 1;
    float f_frame_end = 1;
    wxPoint pp1, pp2;
    //int xr = c.x + c.GetWidth() - 2*border;
    int xr = c.GetWidth() - border;
    int xl = c.x + border;
    pp1.x = xl;
    pp2.x = xr;
    // line mark
    wxColour prevc = *wxWHITE;
    int previndex = -1;
    int prevy=-100;
    int idx = 0;
    int countmark = items_mark.size();
    {
        wxDCPenChanger npen(dc, wxPen(bordercolor, 2, wxPENSTYLE_SOLID));
        int idx_item_mark = 0;
        for (auto i : items_mark) {
            int numColor = color_items_mark[idx_item_mark++];
            if (!mark_color[numColor].enable) continue;
            f_frame_start = i * 1.0f / cnt;
            pp1.y = border + f_frame_start * vpix;
            pp2.y = pp1.y;
            pp2.x = xr;
            //wxColour cll = ctrl->GetItemBackgroundColour(i);
            wxColour cll = mark_color[numColor].color;
            bool needdraw = false;
            int newidx = 0;
            if (prevy == pp1.y) {
                newidx = numColor;
                if (newidx >= idx || newidx == -1) continue;
                needdraw = true;
                // draw only high priority
            }
            if (prevy != pp1.y || needdraw) {
                dc.SetPen(cll);
                dc.DrawLine(pp1, pp2);
                if (prevy + 1 < pp1.y) {
                    // big line 
                    dc.DrawLine(wxPoint(pp1.x, pp1.y-1), wxPoint(pp2.x, pp2.y - 1));
                }
                prevc = cll;
                prevy = pp1.y;
                if (needdraw)
                    idx = newidx;
                else
                {
                    idx = numColor;
                }
                //wxCHECK(idx!=LogIndexColor::notfound);
            }
        }
    }
    int findwidthpix = ((pp2.x - pp1.x) * findwidth) / 100;
    if (findwidth!=0 || findwidthpix!=0) {
        // find mark
        wxDCPenChanger npen(dc, wxPen(findcolor, 1, wxPENSTYLE_SOLID));


        prevy = -100;
        for (auto i : items_find) {
            f_frame_start = i * 1.0f / cnt;
            pp1.y = border + f_frame_start * vpix;
            pp2.y = pp1.y;
            pp2.x = pp1.x+ findwidthpix;
            if (prevy == pp1.y) continue;
            dc.DrawLine(pp1, pp2);
            prevy = pp1.y;
        }
    }
    if (ctrl->GetItemCount() == 0) return;
    // bracket frame 
    dc.SetPen(framecolor);
    long top = ctrl->GetTopItem();
    //if (top > 0) top--;
    if (cnt == 0) cnt = 1;
    f_frame_start = top * 1.0f / cnt;
    f_frame_end = (top + page) * 1.0f / cnt;
    // left line
    wxPoint p1(xl, border + f_frame_start * vpix);
    wxPoint p2(xl, border + f_frame_end * vpix);
    dc.DrawLine(p1, p2);
    // right line
    wxPoint p11(xr-1, border + f_frame_start * vpix);
    wxPoint p22(xr-1, border + f_frame_end * vpix);
    dc.DrawLine(p11, p22);
    //left risk
    dc.DrawLine(p1, wxPoint(p1.x + 2, p1.y));
    dc.DrawLine(p2, wxPoint(p2.x + 2, p2.y));
    // right risk
    dc.DrawLine(wxPoint(p11.x - 1, p11.y), p11);
    dc.DrawLine(wxPoint(p22.x - 1, p22.y), wxPoint(p22.x + 1, p22.y));
    // start db intervals
    {
        wxDCPenChanger npen(dc, wxPen(startdbcolor, 2, wxPENSTYLE_SOLID));
        //dc.SetPen(startdbcolor);

        for (int i = 0; i < startdbintervals.size(); i++) {
            long row = startdbintervals[i];
            // start
            f_frame_start = row * 1.0f / cnt;
            pp1.y = border + f_frame_start * vpix;
            pp1.x = xl;

            //accept
            if ((i + 1) == startdbintervals.size()) {
                row = cnt - 1; //last row
                // no accept
            }
            else
                row = startdbintervals[i + 1];
            f_frame_start = row * 1.0f / cnt;
            pp2.y = border + f_frame_start * vpix;
            pp2.x = pp1.x;
            dc.DrawLine(pp1, pp2);
            i++;
        }
    }
}
void ctlNavigatePanel::AddMarkItem(long item, int numcolor) {
    items_mark.push_back(item);
    color_items_mark.push_back(numcolor);
}
void ctlNavigatePanel::ClearMark() {
    items_mark.clear();
    color_items_mark.clear();
    items_find.clear();
    startdbintervals.clear();
    sinterval = -1; einterval = -1;
    for (auto &c : mark_color) {
        c.count = 0;
        c.enable = true;
    }
//    for (int i = 0; i < regExArray.size(); i++) if (regExArray[i]) delete regExArray[i];
//    regExArray.clear();
    topvisible = -1;
}

void ctlNavigatePanel::OnPaint(wxPaintEvent& evt) {
    wxBufferedPaintDC dc(this);
    render(dc);
}
void ctlNavigatePanel::OnSize(wxSizeEvent& evt)
{
    //wxLogMessage("%p SimpleTransientPopup::OnSize", this);
    SetSize(DoGetBestSize());
    evt.Skip();
}
int  ctlNavigatePanel::GetCountMark() {
    return items_mark.size();
}
int  ctlNavigatePanel::GetItemMark(long position) {
    if (position < items_mark.size() && position >= 0)
        return items_mark[position];
    else
        return -1;
}

int ctlNavigatePanel::binary_search(long item, const std::vector<long> &arr) {
    // binary search
    int max = arr.size() - 1;
    int min = 0;
    int i = (max - min) / 2;
    if (max == -1) i = -1;
    while (i != -1) {
        long t = arr[i];
        if (t < item) {
            min = i;
            i = min + (max - min) / 2;
        }
        else if (t > item) {
            max = i;
            i = min + (max - min) / 2;
        }
        else {
            while (i > 0 && arr[i - 1] == t) {
                i--;
            }
            break;
        }
        if (max - min < 2) {
            i = max;
            break;
        }
    }
    return i;
}
void ctlNavigatePanel::OnMouse(wxMouseEvent& evt) {
    int yy = evt.GetY() - border;
    int page = ctrl->GetCountPerPage();
    wxRect c = GetClientRect();
    int vpix = c.GetHeight() - 2*border;
    long cnt = ctrl->GetItemCount();
    float f_frame_start = 1;
    float f_frame_end = 1;
    f_frame_start = yy * 1.0f / vpix;
    long pos = cnt * f_frame_start;
    int i = binary_search(pos,items_mark);
    wxString tt;
    if (i >= 0 && i < items_mark.size() && items_mark[i] > pos) i--;
    if (i >= 0 && i < items_mark.size() ) {
        tt = ctrl->GetTextLong(items_mark[i]);
        this->SetToolTip(tt);
    }
    else
        this->UnsetToolTip();
    //return i;


}
void ctlNavigatePanel::RowVisibleCenter(long row) {
    int page = ctrl->GetCountPerPage();
    long cnt = ctrl->GetItemCount();

    int center = (row - page / 2);
    if (center < 0) center = 0;
    ctrl->EnsureVisible(row);
    long top = ctrl->GetTopItem();
    if ((row - top) < (page / 2)) ctrl->EnsureVisible(center); 
    else
        if ((row + page / 2) < cnt) ctrl->EnsureVisible(row + page / 2); 
        else 
            if (cnt - row < page) {
                ctrl->EnsureVisible(cnt - 1); // last page
                return;
            }
}
wxBitmap CreateBitmap(const wxColour& colour, bool check,int sz)
{
    if (sz <= 0 || sz > 60) sz = 15;
    wxMemoryDC dc;
    wxBitmap bmp(sz, sz);
    int w = sz, h = sz;
    dc.SelectObject(bmp);
    if (colour == wxNullColour)
        dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
    else
        dc.SetBrush(wxBrush(colour));
    dc.DrawRectangle(0, 0, w, h);
    if (!check) {
        wxPen p = dc.GetPen();
        wxDCPenChanger ppen(dc,wxPen(p.GetColour(),2));
        dc.DrawLine(wxPoint(0,0), wxPoint(w, h));
        dc.DrawLine(wxPoint(w, 0), wxPoint(0, h));
    }

    return bmp;
}
// ReScan List full
int ctlNavigatePanel::ReColorizeItems(int numIndictor, bool enableColor) {
    wxColour bg = ctrl->GetBackgroundColour();
    int count = 0;
//    mark_color[numIndictor].enable = enableColor;
    //items_mark.clear();
    int idx = 0;
    for (long i = 0; i < items_mark.size(); i++) {
        //int idx = TryMarkItem(i,ctrl->GetText(i));
        int item = items_mark[i];
        int numColor = color_items_mark[i];
        if (numColor == numIndictor) {
            if (enableColor)
                ctrl->SetItemBackgroundColour(item, GetColorByIndex(numColor));
            else
                ctrl->SetItemBackgroundColour(item, bg);
            count++;
        }
        //else ctrl->SetItemBackgroundColour(i, bg);
    }
    return count;
}
void ctlNavigatePanel::OnContextMenu(wxCommandEvent& event) {
    int numMark = event.GetId() - MNU_MARK;
    int numCommand = event.GetId() - MNU_MARK - 500;
    if (numCommand >= 0 && numCommand < opt["commands"].Size()) {
        // context menu command
        wxKeyEvent kevt;
        RunKeyCommand(kevt, numCommand);
        return;
    }
    if (numMark < 0|| numMark>=mark_color.size()) return;
   // wxMessageBox(wxString::Format("Num command = %d",numMark), "Command");
    // context menu marker
    bool controlkey = wxGetKeyState(WXK_CONTROL);
    if (controlkey) {
        wxJSONValue indicator = opt["indicators"][numMark];
        if (indicator["enable"].IsBool()) {
            bool en = !indicator["enable"].AsBool();
            opt["indicators"][numMark]["enable"] = en;
            wxMenuItem* mi = (wxMenuItem*)event.GetEventObject();
            mi->Check(en);
            Init(true);
            mark_color[numMark].enable = en;
            ReColorizeItems(numMark, en);
            Refresh();
        }
    }
    else {
        // goto first mark
        int idx = 0;
        for (auto i : color_items_mark) {
            if (i == numMark) {
                long item=GetItemMark(idx);
                ctrl->Focus(item);
                RowVisibleCenter(item);
                break;
            }
            idx++;
        }
    }

}
void ctlNavigatePanel::mouseReleased(wxMouseEvent& evt) {
    bool left = evt.LeftUp();
    bool right = evt.RightUp();
    bool controlkey= wxGetKeyState(WXK_CONTROL);
    if (left) {
        int yy = evt.GetY() - border;
        int page = ctrl->GetCountPerPage();
        wxRect c = GetClientRect();
        int vpix = c.GetHeight() - 2*border;
        long cnt = ctrl->GetItemCount();
        float f_frame_start = 1;
        if (cnt == 0) cnt = 1;
        f_frame_start = yy * 1.0f / vpix;
        long pos = cnt * f_frame_start;
        if (!controlkey) {
            // only left
        } else 
            if (pos < cnt) {
                    // up market find
                    int i = binary_search(pos,items_mark);
                    if (i >= 0 && i < items_mark.size() && items_mark[i] > pos) i--;
                    if (i >= 0 && i < items_mark.size()) pos = items_mark[i];
             }
        RowVisibleCenter(pos);
        Refresh();
        ctrl->SetFocus();

    }
    else {
        // right
          // Show popupmenu at position
        //menu.Append(MNU_MARK, wxT("&About"));
        
        wxJSONValue r = opt["indicators"];
        if (!r.IsNull()) {
            int idx = 0;
            int numIndicator = 0;
            int sz = opt["contextboxsize"].AsInt();
            wxString fndstr = wxEmptyString;
            if (logFindString.Len() > 30)
                fndstr = logFindString.Left(30) + "...";
            else
                fndstr = logFindString;
            int fndcount = items_find.size();
            if (fndcount > 0) fndstr = wxString::Format("[%d] \"%s\"", fndcount,fndstr);
            wxMenu menu(fndstr);

            int spacewidth,tmp;
            DoGetTextExtent(" ", &spacewidth, &tmp);
            int maxwidth = 0;
            int x, y;
            for (int j = 0; j < r.Size(); j++) {
                if (mark_color[j].count > 0) {
                    wxString counttext = wxString::Format("[%ld] ", mark_color[j].count);
                    DoGetTextExtent(counttext, &x, &y);
                    if (x > maxwidth) maxwidth = x;
                }
            }
            for (int j = 0; j < r.Size(); j++) {
                wxJSONValue indicator = r[j];
                bool enable = indicator["enable"].AsBool();
                wxString name = indicator["menulabel"].AsString();
                if (name=="null") {
                    name = wxString::Format("Marker %d",j);
                }
                // align
                wxString counttext="";
                if (mark_color[j].count > 0) {
                    counttext = wxString::Format("[%ld] ", mark_color[j].count);
                    DoGetTextExtent(counttext, &x, &y);
                    int cnt = (maxwidth - x) / spacewidth;
                    if (x< maxwidth) counttext=wxString(' ',cnt)+ counttext;
                }
                else {
                    int cnt = (maxwidth) / spacewidth;
                    wxString s(' ', cnt);
                    counttext=s+counttext;
                }
                name = wxString::Format("%s%s", counttext, name);
                wxColour clr=GetColorByIndex(j);
                wxMenuItem* mi = new wxMenuItem(&menu, MNU_MARK + j, name,wxEmptyString , wxITEM_CHECK);
                menu.Append(mi);
                mi->Check(enable);
#if defined(__WXMSW__)
                mi->SetBitmaps(CreateBitmap(clr, true,sz), CreateBitmap(clr, false,sz));
#endif
            }
            PopupMenu(&menu, evt.GetPosition());
        }
     //   mi->SetBackgroundColour(wxColour(255, 0, 0));
        
       
    }
}

int ctlNavigatePanel::TryMarkItem(long row, const wxString& str) {
    int sz = search_rule.size();
    int numIndicator = -1;
    bool fnd = false;
    int j = 0;
    while (j < sz) {
        find_s ff= search_rule[j];
        if (ff.i == -1) { j++; continue; } // disable rule
        if (!fnd && ff.e == ff.i) {
            // goto next indicator ecxlude ignore
            j = ff.i;
            continue;
        }
        if (fnd && numIndicator < ff.num) { //  exclude ending
            break;
        }
        bool iswhere = false;
        if (regExArray[j] == NULL)
            iswhere = str.Find(ff.s) >= 0;
        else
            iswhere = regExArray[j]->Matches(str);

        if (iswhere) {
            if (ff.e == ff.i) { // exclude found, continue 
                j = ff.i; // goto next indicator string
                fnd = false;
                numIndicator = -1;
            }
            else {
                // found include string
                // comapre exclude if needed
                numIndicator = ff.num;
                if (ff.e == -1) break; // skip empty exclude
                j = ff.e;
                fnd = true;

            }
        }
        else j++;
    }
    if (numIndicator >= 0) {
        AddMarkItem(row, numIndicator);
        mark_color[numIndicator].count++;
    }
    // find mark
    if (logFindString.length()>0 && (str.Find(logFindString) > -1)) {
        items_find.push_back(row);
    }
    // starting db
    if ((str.Find("\"postmaster\"") > -1)
        &&
        (((str.Find("LOG,00000,\"starting ") > -1 )
        ||
        (str.Find("LOG,00000,\"all server processes terminated; reinitializing") > -1) // postmaster restart iher process

        ))
        && sinterval==-1) {
        // start db
        sinterval = row;
        startdbintervals.push_back(sinterval);
        einterval = -1;
    }
    else if (str.Find("LOG,00000,\"database system is ready to a") > -1) {
        // accept connect db
        einterval = row;
        if (sinterval==-1) startdbintervals.push_back(0);
        startdbintervals.push_back(row);
        sinterval = -1;
    }

    return numIndicator;
}
wxColour ctlNavigatePanel::GetColorByIndex(int colorindex) {
    
    if (colorindex >= 0 && colorindex < mark_color.size()) {
        return mark_color[colorindex].color;
    }
    return *wxBLACK;
}
int ctlNavigatePanel::FindText(wxString findtext, int position, bool directionUp) {
    long lcount = ctrl->GetItemCount();
    long item = 0;
    if (position == HOME && lcount>0) {
        ctrl->Focus(0);
    }
    long itemFocus = ctrl->GetFocusedItem();
    if (items_find.size() == 0) {
        while (item < lcount && item >= 0)
        {
            // this item is selected - do whatever is needed with it
            //wxLogMessage("Item %ld is focused.", item);
            //long fpos = logList->FindItem(item, logFindString, true);
            wxString s = ctrl->GetTextLong(item);
            item++;
            if (!(s.Find(logFindString) > -1)) {
                continue;
            }
            //ctrl->Focus(item);
            //lastPositionFind = item + dir;
            items_find.push_back(item - 1);
        }
        if (items_find.size() == 0) {
            wxBell();
            return 0;
        }
    }
    long nextitem = -1;
    long dir = 1;
    if (directionUp) dir = -1;
    int posF = binary_search(itemFocus, items_find);
    if (items_find[posF] > itemFocus && !directionUp) nextitem = posF;
    if (items_find[posF] < itemFocus && directionUp)  nextitem = posF;
    if (items_find[posF] == itemFocus)  nextitem = posF + dir;
    if (nextitem == -1) {
        if (items_find[posF] > itemFocus && directionUp) nextitem = posF - 1;
        if (items_find[posF] < itemFocus && !directionUp)  nextitem = posF + 1;
    }

    if (nextitem == -1 || nextitem>= items_find.size())
        wxBell();
    else
    {
        ctrl->Focus(items_find[nextitem]);
        RowVisibleCenter(items_find[nextitem]);
    }
    return 0;
}

