
#include "pgAdmin3.h"
#include "ctl/wxTopActivity.h"
#include <wx/dcbuffer.h>
#include <wx/clipbrd.h>
#include "wx/regex.h"
#include "wx/display.h"
#include <stack>
#include "utils/FormatterSQL.h"
int s_pid_HIGHLIGH = -1;
std::vector<int> sort_vec_map(const std::vector<int>& src, int& sum) {
    std::vector<int> tmp(src.size());
    std::vector<int> ok(src.size());
    for (int i = 0; i < src.size(); i++) {
        int k = -1;
        int max = -1;
        for (int j = 0; j < src.size(); j++) {
            if (ok[j] == 0 && src[j] > max) { max = src[j]; k = j; }
        }
        if (k == -1) break;
        ok[k] = 1;
        tmp[i] = k;
        sum += src[k];
    }
    return tmp;
};

//----------------------------------------------------------------------------
// SimpleTransientPopup
//----------------------------------------------------------------------------
wxIMPLEMENT_CLASS(SimpleTransientPopup, wxFrame);

wxBEGIN_EVENT_TABLE(SimpleTransientPopup, wxFrame)
//EVT_MOUSE_EVENTS(SimpleTransientPopup::OnMouse)
EVT_SIZE(SimpleTransientPopup::OnSize)
EVT_SET_FOCUS(SimpleTransientPopup::OnSetFocus)
EVT_KILL_FOCUS(SimpleTransientPopup::OnKillFocus)
wxEND_EVENT_TABLE()

SimpleTransientPopup::SimpleTransientPopup(wxWindow* parent, bool scrolled, wxTopActivity* small_ctl, wxPoint p,wxString title)
    :wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxFRAME_NO_TASKBAR | wxRESIZE_BORDER | wxCLOSE_BOX | wxMAXIMIZE_BOX |wxTINY_CAPTION)
{
    //m_panel = new wxScrolledWindow(this, wxID_ANY);
    m_panel = new wxPanel(this, wxID_ANY);
    m_panel->SetBackgroundColour(*wxLIGHT_GREY);
    wxBoxSizer* sizerF = new wxBoxSizer(wxVERTICAL);
    m_panel->Bind(wxEVT_MOTION, &SimpleTransientPopup::OnMouse, this);
    WaitSample* w = NULL;
    int agg, right_g;
    w = small_ctl->getViewRange(agg, right_g);
    wxSize top_sz = wxSize(1000, 500);
    top = new wxTopActivity(m_panel, w, top_sz);
    top->setViewRange(agg, right_g);
   // wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* topSizer = new wxFlexGridSizer(1);
    topSizer->AddGrowableCol(0);
    topSizer->AddGrowableRow(0);
    topSizer->Add(top, 1, wxEXPAND, 5);
    int style = 0;
    dvc = new topDataViewCtrl(m_panel, wxID_ANY,
        wxDefaultPosition,
        wxSize(1000, 155),
        style,
        top
    );
    topSizer->Add(dvc, 0, wxEXPAND|wxALL, 2);
    //dvc->SetMinSize(wxSize(-1,60));
    m_index_list_model = new MyIndexListModel(w);
    dvc->AssociateModel(m_index_list_model.get());
    dvc->BuildColumn(0);
    dvc->CalcRowsDataView(true, true);
    m_taskTimer.Start(100);
    m_taskTimer.Bind(wxEVT_TIMER, &SimpleTransientPopup::OnTimerEvent, this);
    
    //dvc->AppendTextColumn("Pid", 0);
    //dvc->AppendTextColumn("String2", 1);
    wxPoint posScreen;
    wxSize sizeScreen;
    const int displayNum = wxDisplay::GetFromPoint(p);
    if (displayNum != wxNOT_FOUND)
    {
        const wxRect rectScreen = wxDisplay(displayNum).GetGeometry();
        posScreen = rectScreen.GetPosition();
        sizeScreen = rectScreen.GetSize();
    }
    else // outside of any display?
    {
        // just use the primary one then
        posScreen = wxPoint(0, 0);
        sizeScreen = wxGetDisplaySize();
    }
    if (p.x + top_sz.x > sizeScreen.x) p.x = sizeScreen.x - top_sz.x - 20;
    if (p.y + top_sz.y > sizeScreen.y) p.y = sizeScreen.y - top_sz.y - 20;
    Move(p);
    //topSizer->Add(new wxPanel(m_panel, wxID_ANY, wxDefaultPosition,        wxSize(600, 900)));

    m_panel->SetSizer(topSizer);
    //this->SetSizer(topSizer);
        // Use the fitting size for the panel if we don't need scrollbars.
    m_panel->Layout();
    //topSizer->Fit(m_panel);
    
    sizerF->Add(m_panel, 1, wxCENTRE | wxALL| wxEXPAND, 2);
    SetSizer(sizerF);
    Layout();
    //SetSize(m_panel->GetSize());
   // SetClientSize(m_panel->GetSize());
//    Layout();
    Fit();
    SetMinSize(wxSize(500, 350));
    
}

SimpleTransientPopup::~SimpleTransientPopup()
{
    s_pid_HIGHLIGH = -1;
}




void SimpleTransientPopup::OnSize(wxSizeEvent& event)
{
    //wxLogMessage("%p SimpleTransientPopup::OnSize", this);
    //Fit();
    //m_panel->Layout();
    Layout();
   // Fit();
    event.Skip();
}

void SimpleTransientPopup::OnSetFocus(wxFocusEvent& event)
{
    //wxLogMessage("%p SimpleTransientPopup::OnSetFocus", this);
    //event.Skip();
}

void SimpleTransientPopup::OnKillFocus(wxFocusEvent& event)
{
    //wxLogMessage("%p SimpleTransientPopup::OnKillFocus", this);
    //event.Skip();
}
void SimpleTransientPopup::OnClose(wxCommandEvent& event)
{
    //wxLogMessage("%p SimpleTransientPopup::OnKillFocus", this);
    //event.Skip();
    //dvc->Unbind()
    m_panel->Unbind(wxEVT_MOTION, &SimpleTransientPopup::OnMouse, this);
    m_taskTimer.Stop();
    m_taskTimer.Unbind(wxEVT_TIMER, &SimpleTransientPopup::OnTimerEvent, this);
    Close(true);
}

void SimpleTransientPopup::OnMouse(wxMouseEvent& event)
{
    wxColour colour(*wxLIGHT_GREY);
    if (event.Dragging() && event.LeftIsDown() && !dragg) { mouseDownPos_ = event.GetPosition(); dragg = true; }

    if (event.Dragging() && event.LeftIsDown())
    {
        if (mouseDownPos_ != event.GetPosition()) {
            const auto screenPosCurrent = ClientToScreen(mouseDownPos_);
            auto wp = ClientToScreen(event.GetPosition()) - screenPosCurrent;
            auto w = GetPosition() + wp;
            Move(w);
        }
    }
    if (!event.LeftIsDown()) dragg = false;
    event.Skip();
}
void SimpleTransientPopup::OnTimerEvent(wxTimerEvent& pEvent)
{
    bool isf = dvc->GetColumnPosition(dvc->GetColumn(0)) == 0;//"PID";
    if (dvc->CalcRowsDataView(false, isf))
        this->Refresh();
    top->Refresh();
}



BEGIN_EVENT_TABLE(wxTopActivity, wxPanel)

EVT_MOTION(wxTopActivity::mouseMoved)
EVT_LEFT_DOWN(wxTopActivity::mouseDown)
EVT_LEFT_UP(wxTopActivity::mouseReleased)
EVT_RIGHT_UP(wxTopActivity::mouseReleased)
EVT_LEAVE_WINDOW(wxTopActivity::mouseLeftWindow)
EVT_KEY_DOWN(wxTopActivity::keyPressed)
EVT_KEY_UP(wxTopActivity::keyReleased)
EVT_SIZE(wxTopActivity::resize)
EVT_MOUSEWHEEL(wxTopActivity::mouseWheelMoved)

// catch paint events
EVT_PAINT(wxTopActivity::paintEvent)
EVT_ERASE_BACKGROUND(wxTopActivity::OnEraseBackground)

END_EVENT_TABLE()


wxTopActivity::wxTopActivity(wxWindow* parent, WaitSample* WS, wxSize sz) :
    wxControl(parent, wxID_ANY)
{
    ws = WS;
    m_title = "TOP activity";
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(sz);
    SetSize(sz);
    setsize = sz;
    
    if (sz.GetHeight() < 150) {
        SetName("wxTopActivitySmall");
    }
    else {
        SetName("wxTopActivityBig");
    }
    
    m_simplePopup = NULL;
}
void wxTopActivity::SetFilter(long long qid) {
    if (qid != -1) {
        unsigned long long ull = qid;
        int h = ull >> 32;
        int l = ull & 0xFFFFFFFF;
        m_filter_detail = wxString::Format("%llx", ull);
        m_qid_filter = qid;
    }
    else {
        m_filter_detail = wxEmptyString;
        m_qid_filter = -1;
    }
    m_regroup = true;
    Refresh();
    
}
void wxTopActivity::render(wxDC& dc)
{
    wxColour c;
    //dc.SetBrush(*wxGREY_BRUSH);
    dc.SetBrush(ws->GetColorGui(Color_GUI::BG));
    //dc.SetPen();
    dc.SetTextForeground(ws->GetColorGui(Color_GUI::LABEL));
    wxRect r = GetClientRect();
    wxSize sz = r.GetSize();
    //dc.DrawRectangle(0, 0, r.width, r.height);
    dc.DrawRectangle(0, 0, sz.x, sz.y);
    int VERT_SPC = 3;
    wxSize time_sz = dc.GetTextExtent("00:00");
    wxSize title_sz;
    int ypos = 2;
    int YaxisWidth = 50;
    int LegengWidth = 140;
    int xw = sz.x - YaxisWidth - LegengWidth;
    int btl_y = 0;
    if (xw > 100 && sz.y > 300)
    {
        wxDCFontChanger nfont(dc, dc.GetFont().Bold());
        wxString tit = m_title + wxString::Format(" [period: %d s]", m_agg_int / 1000);
        title_sz = dc.GetTextExtent(tit);
        wxSize dp = dc.GetTextExtent(m_filter_detail);
        wxPoint p;
        p.y = ypos;
        p.x = sz.x / 2 - (title_sz.x + dp.x) / 2;
        wxRect dpr(p.x + title_sz.x, p.y, dp.GetWidth(), dp.GetHeight());
        if (dpr.Contains(mouse) && m_click == 1) {
            // remove filter_detail
            m_click = 0;
            m_filter_detail = wxEmptyString;
            m_qid_filter = -1;
            m_regroup = true;
            return;
        }
        dc.DrawText(tit + m_filter_detail, p);
        ypos += title_sz.y + VERT_SPC;
        // button line
        btl_y = sz.y - ypos - time_sz.y - VERT_SPC;
    }
    else {
        YaxisWidth = 0;
        LegengWidth = 0;
        ypos = 0;
        xw = sz.x;
        btl_y = sz.y;
        if (m_click == 1) {
            //delete m_simplePopup;
            wxPoint pp(20, 20);
            wxPoint pos = this->GetParent()->ClientToScreen(pp);
            wxWindow* f = this->GetParent()->FindWindowByName("wxTopActivityBig", this->GetParent());
            if (f == NULL) {
                wxString tit = "TopActivity ";
                wxString s = this->GetParent()->GetParent()->GetLabel();
                //wxString s2 = this->GetParent()->GetParent()->GetName();
                m_simplePopup = new SimpleTransientPopup(this->GetParent(), false, this, pos,tit+s);
                //wxWindow* btn = (wxWindow*)event.GetEventObject();
                //m_simplePopup->Move(pos);
                //m_simplePopup->Position(pos, sz);
                //wxLogMessage("%p Simple Popup Shown pos(%d, %d) size(%d, %d)", m_simplePopup, pos.x, pos.y, sz.x, sz.y);
                if (m_simplePopup) m_simplePopup->Show();
            }
            else {
                f->SetFocus();
            }
                
        }
    }

    // Graph Area

    wxRect a(YaxisWidth, ypos, xw, btl_y);
    m_area = a;
    int max_label = xw / time_sz.x;
    int msLen = ws->GetInterval(0, -1);
    if (msLen == 0) return;
    int width_sample = 2;
    int needGrpMax = a.width / width_sample;
    int realGrp = 0;
    int tmp = ws->GetHomeInterval(-1, m_agg_int);
    if (m_lastInterval != tmp || m_regroup) {
        if (m_lastInterval != tmp && m_lastInterval == m_RightTime)
            m_RightTime = tmp; // update last position
        realGrp = ws->GetGroupingData(m_RightTime, needGrpMax, m_agg_int, m_filter + (m_filter_detail.IsEmpty() ? "" : "@" + m_filter_detail + ";"), seriosName, xAxis, yAxis, val, colors);
        m_count_wait = ws->GetCountWaits();
        m_fix_detail_idx = -1;
        m_lastInterval = tmp;
        m_regroup = false;
        m_sumtotal.clear();
        // calc sum
        for (size_t j = 0; j < m_count_wait; j++) // serios group
        {
            int Yv = val[0][j];
            if (Yv < 0) continue;
            int sum = 0;
            for (size_t i = 0; i < val.size(); i++)
            {
                sum += val[i][j];   // vertical sum 
            }
            wxString n = ws->GetName(j, WAIT_FULL);
            m_sumtotal.emplace(n, sum);
            wxString g = n.BeforeFirst(':');
            if (g.IsEmpty()) continue;
            if (m_sumtotal.find(g) != m_sumtotal.end()) {
                m_sumtotal[g] += sum;
            }
            else m_sumtotal[g] = sum;
        }

        // title info
        std::stack<title_info> s;
        bool all_remove = true;
        int cnt_rem = 0;
        m_title_i.clear();
        for (int i = seriosName.GetCount() - 1; i >= 0; i--)
        {
            wxString name = seriosName[i];
            title_info info;
            info.down_line = false;
            info.remove = false;
            if (name[0] == ' ') { 
                name = name.substr(2);
                info.iner = true;
                info.title = name;
                all_remove = false;
            }
            else if (name.StartsWith("--")) { // remove individual wait
                name = name.substr(2);
                info.iner = true;
                info.title = name;
                info.remove = true;
                cnt_rem++;
            }
            else if (name[0] == '-') { // remove group
                name = name.substr(1);
                info.iner = false;
                info.title = name;
                info.remove = true;
                all_remove = true;
            }
            else {
                info.title = name;
                info.iner = false;
                info.remove = all_remove;
                info.down_line = (!all_remove) && cnt_rem > 0;
            }
            s.push(info);
            if (info.iner == false) {
                if (m_collapse.find(name) != m_collapse.end()) m_collapse.emplace(name, 0);
                wxString fn;
                wxString grp_name = name;
                long long w_sum = 0;
                int grp_total = m_sumtotal[name];
                int start = m_title_i.size() + 1;
                while (!s.empty())
                {
                    m_title_i.push_back(s.top());
                    s.pop();
                    int i = m_title_i.size() - 1;
                    if (m_sumtotal.find(m_title_i[i].title) == m_sumtotal.end()) fn = grp_name + ':' + m_title_i[i].title;
                    else fn = m_title_i[i].title;
                    m_title_i[i].total = m_sumtotal[fn];
                }
                all_remove = true;
                cnt_rem = 0;
                // sort buuble
                for (int j = 1; j < m_title_i.size() - start; j++) {
                    bool exit = true;
                    for (int i = start; i <= m_title_i.size() - j - 1; i++) {
                        if (m_title_i[i].total < m_title_i[i + 1].total) {
                            info = m_title_i[i];
                            m_title_i[i] = m_title_i[i + 1];
                            m_title_i[i + 1] = info;
                            exit = false;
                        }
                    }
                    if (exit) break;
                }

            }
        }


    }
    else {
        realGrp = val.size();
    }
    width_sample = a.width / realGrp;
    if (width_sample < 2) width_sample = 2;
    // legend
    wxRect t(a.x + a.width + 5, a.y, sz.x - (a.x + a.width + 5 + 5), a.height);

    //dc.DrawRectangle(t.x, t.y, t.width, t.height);
    int  yy = t.y + 3;
    int _maxx = 0;
    wxPoint p;
    wxRect r1(t.x, a.y, t.width - 20, 0);   // mouse control
    wxRect grp;
    wxString grp_name;
    int iscollapse = 0;
    int total = 0;
    for (int i = 0; i < m_title_i.size() && LegengWidth>0; i++)
    {

        title_info info = m_title_i[i];

        wxSize _sz;
        wxString name = info.title;
        wxFont f = dc.GetFont();
        int addspc = 0;
        long long  w_sum = 0;
        wxString fn;
        wxString tpr;
        if (info.iner) {
            if (iscollapse == 0) continue;
            f.SetPointSize(f.GetPointSize() - 1);
            addspc = 0;
            if (info.total == 0) w_sum = -1;
            else
                w_sum = 100 * info.total / total;

            if (m_sumtotal.find(name) == m_sumtotal.end()) fn = grp_name + ':' + name;
            else fn = name;

        }
        else {
            long cl = ws->GetColorByWaitName(name);
            c.Set(cl);
            dc.SetBrush(c);
            dc.DrawRectangle(t.x + 3, yy, time_sz.GetHeight(), time_sz.GetHeight());
            iscollapse = m_collapse[name];
            grp_name = name;
            total = info.total;
            if (info.total != 0) tpr = wxString::Format("%.1fs", (float)info.total / 1000.0);
        }
        {
            wxDCFontChanger nfont(dc, f);
            _sz = dc.GetTextExtent(name);
            wxSize dp = dc.GetTextExtent(tpr);
            p.y = yy;
            p.x = t.x + time_sz.GetHeight() + 8;
            wxPoint pdp(t.x + t.width - dp.GetWidth() - 3, p.y);

            r1.y = p.y;
            r1.x = p.x;
            r1.height = _sz.GetHeight() + 1;
            wxString pr = wxString::Format("%d", (int)w_sum);
            wxPoint p3(t.x + 2, p.y);
            if (w_sum > 0) dc.DrawText(pr, p3);
            if (r1.Contains(mouse)) {
                wxDCFontChanger nfont(dc, f.Bold());
                dc.DrawText(name, p);
                if (!tpr.IsEmpty()) dc.DrawText(tpr, pdp);
                if (m_click == 1) { // LeftUp
                    if (info.iner) {
                        m_title_i[i].remove = !info.remove;
                        m_regroup = true;
                        if (info.remove) m_filter.Replace("-" + fn + ";", "");
                        else m_filter += "-" + fn + ";";

                    }
                    else {
                        int vv = m_collapse[name];
                        if (vv == 0) vv++; else vv--;
                        m_collapse[name] = vv;
                    }
                    m_click = 0;
                    return;
                }
            }
            else {
                dc.DrawText(name, p);
                if (!tpr.IsEmpty()) dc.DrawText(tpr, pdp);
            }

        }
        if (info.remove) {
            wxPoint p1(p.x, p.y + _sz.y / 2 + 1);
            wxPoint p2(p.x + _sz.GetWidth(), p.y + _sz.y / 2 + 1);
            dc.DrawLine(p1, p2);
        }
        if (info.down_line) {
            wxPoint p1(p.x, p.y + _sz.y);
            wxPoint p2(p.x + _sz.GetWidth(), p.y + _sz.y);
            dc.DrawLine(p1, p2);
        }
        yy += _sz.y + 1;
        if (_maxx < _sz.GetWidth()) _maxx = _sz.GetWidth();
    }
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    //t.width = _maxx + p.x + 3 -t.x;
    t.height = yy - t.y + 3;
    if (LegengWidth > 0) dc.DrawRectangle(t.x, t.y, t.width, yy - t.y + 3);

    dc.SetBrush(ws->GetColorGui(Color_GUI::BG));
    dc.SetPen(ws->GetColorGui(Color_GUI::LABEL));
    dc.DrawRectangle(a.x, a.y, a.width, a.height);
    wxTimeSpan maxY = yAxis[0];
    wxLongLong t_maxYms = maxY.GetMilliseconds();
    int t_maxY = t_maxYms.GetLo();
    //Yaxis
    int nlab = a.height / time_sz.GetHeight();
    nlab = nlab / 2;
    int delta = t_maxY / (nlab == 0 ? 99999 : nlab);
    int y = a.y + 1;
    int x = a.x - 1;
    float fl = 0;

    for (int i = 0; i <= nlab && LegengWidth > 0; i++)
    {
        wxString txt = wxString::Format("%.2f", fl / 1000);
        wxSize dx = dc.GetTextExtent(txt);
        y = a.y + a.height - i * (a.height / nlab);
        dc.DrawText(txt, x - dx.x - 4, y - dx.y / 2);
        fl += delta;
        dc.SetPen(ws->GetColorGui(Color_GUI::LABEL));
        wxPoint p1(a.x, y);
        wxPoint p2(a.x - 3, y);
        dc.DrawLine(p1, p2);
        {
            wxDCPenChanger npen(dc, wxPen(ws->GetColorGui(Color_GUI::GRID_LINE), 1, wxPENSTYLE_LONG_DASH));
            p2.x = a.x + a.width - 2;
            p1.x += 1;
            if (i != 0) dc.DrawLine(p1, p2);
        }
    }

    int cx = a.x + a.width - 1;
    int cy = a.y + a.height - 1;
    std::vector<wxPoint> down_poly;
    // xAxis
    int xAareaW = time_sz.GetWidth() + 24;
    int Nlabel = (a.width - 1) / xAareaW;
    int stepx = (xAareaW / width_sample);
    if (stepx == 0) stepx = 1;
    dc.SetPen(ws->GetColorGui(Color_GUI::LABEL));
    wxRect rd;
    wxPoint ptest;
    for (int i = stepx / 2; i < xAxis.size() && LegengWidth > 0; i = i + stepx)
    {
        wxDateTime t = xAxis.at(i);
        //t.Add(wxTimeSpan(0, 0, 0, m_agg_int));
        wxString labeltext = t.Format("%H:%M");
        //Tick
        wxPoint p1(cx - i * width_sample, cy + 1);
        wxPoint p2(cx - i * width_sample, cy + 3);
        dc.DrawLine(p1, p2);
        //label
        p2.x = p2.x - time_sz.GetWidth() / 2;
        p2.y += 1;
        ptest = p2;
        ptest.x = p2.x + time_sz.GetWidth();
        if (!rd.Contains(ptest)) {
            dc.DrawText(labeltext, p2);
            rd.x = p2.x; rd.y = p2.y; rd.width = time_sz.GetWidth(); rd.height = time_sz.GetHeight();
        }

    }

    // Area
    dc.SetPen(*wxTRANSPARENT_PEN);
    for (size_t i = 0; i < val.size(); i++)
    {
        down_poly.push_back(wxPoint(cx, cy));
        cx -= width_sample;
    }
    if (down_poly.size() < 2) down_poly.push_back(wxPoint(cx, cy));
    //down_poly.push_back(wxPoint(cx,cy));
    //float koef=(float)t_maxY /(float)(a.height - 10) ;
    int vert_line = m_agg_int;
    float koef = (float)(a.height - 2) / (float)t_maxY;
    wxPoint selP(0, 0);
    int selIdx = -1;
    for (size_t j = m_count_wait; j < val[0].size(); j++) // serios group
    {
        int Yv = val[0][j];
        if (Yv < 0) continue;

        int ii = down_poly.size() - 1;
        ii = 0;
        std::vector<wxPoint> up_poly;
        wxPoint p;
        for (size_t i = 0; i < val.size(); i++)
        {
            int Yv = val[i][j];
            if (Yv < 0) continue;
            int pixY = Yv * koef;
            p = down_poly[i];
            p.y = p.y - pixY;
            up_poly.push_back(wxPoint(p));
            if (width_sample > 19 && i > 0) {
                wxDCPenChanger npen(dc, wxPen(ws->GetColorGui(Color_GUI::GRID_LINE), 1, wxPENSTYLE_DOT_DASH));
                dc.DrawLine(wxPoint(p.x, down_poly[i].y - 1), wxPoint(p.x, a.y + 1));
            }
            wxPoint dis = mouse - p;
            if ((dis.x * dis.x + dis.y * dis.y) < 20 && LegengWidth > 0) {
                if (m_fix_detail_idx == -1) {
                    selP = p;
                    selIdx = i;
                }
            }
        }
        vert_line = -1;
        if (up_poly.size() < 2) {
            up_poly.push_back(wxPoint(down_poly[1].x, p.y));
        }
        std::vector<wxPoint> poly(down_poly.size());
        for (int i = 0; i < poly.size(); i++)
        {
            poly[i] = down_poly[i];
        }
        for (int i = up_poly.size() - 1; i >= 0; i--)
        {
            poly.push_back(up_poly[i]);
        }
        long cl = colors[j];
        c.Set(cl);
        dc.SetBrush(c);
        dc.DrawPolygon(poly.size(), poly.data());
        for (int i = 0; i < down_poly.size(); i++)
        {
            down_poly[i] = up_poly[i];
        }
        //break;
    }
    if (LegengWidth > 0) paintSelRange(dc, width_sample);


    // select Point
    if (selIdx != -1 || m_fix_detail_idx != -1) {
        if (m_fix_detail_idx == -1) {
            dc.SetPen(*wxWHITE_PEN);
            dc.SetBrush(*wxBLACK_BRUSH);
            dc.DrawCircle(selP, 3);
            if (m_click == 1) {
                m_fix_detail_idx = selIdx;
                m_fix_pos = selP;
                m_click = 0;
            }
        }
        else {
            selIdx = m_fix_detail_idx;
            selP = m_fix_pos;
        }
        /// Detail panel
        auto dt = xAxis[selIdx];
        int endt = ws->d_time(dt.GetValue().GetValue());
        //if (endt < 0) endt = m_agg_int;
        //endt -= m_agg_int;
        std::vector<Sample>* smp = ws->GetSamples();
        //int start_t = ws->GetHomeInterval(m_RightTime, m_agg_int) - selIdx * m_agg_int;
        int start_t = endt - m_agg_int;
        int ps = ws->getPositionByTime(start_t);

        Sample sa;
        int sz2 = smp->size();
        std::map<key3, int> qid_wait_pid_count; // [qid]=pid1,pid2
        std::map<key3, int> qid_wait_pid_BG_sum; // [qid]=pid1,pid2
        std::map<key3, int> qid_wait_pid_FG_sum; // [qid]=pid1,pid2

        std::map<int, int> pid_btype;
        // quid -> idx
        // ������� �����
        //std::map<long long,int> map_qid_idx;
        std::vector<long long> qid_;
        //std::vector<int> qid_wait_id;
            // ����� �������� ��� ������� idx
        std::vector<std::vector<int>> qid_wait_id_SUM;
        //std::vector<std::vector<int>> qid_wait_id;
        std::vector<int> sum_bg(m_count_wait);
        std::vector<int> sum_all(m_count_wait);
        while (ps < sz2)
        {
            sa = smp->at(ps++);
            if (sa.btime >= (start_t + m_agg_int)) break;
            long long q = sa.qid;
            if (sa.wait_id >= m_count_wait) continue; // new wait append
            if (m_qid_filter != -1 && m_qid_filter != q) continue;
            int idx = -1;
            for (int i = 0; i < qid_.size(); i++) {
                if (qid_[i] == q) { idx = i; break; }
            }
            if (idx == -1) {
                qid_.push_back(q);
                idx = qid_.size() - 1;
                std::vector<int> wait(m_count_wait);
                qid_wait_id_SUM.push_back(wait);

            }
            
            qid_wait_id_SUM[idx][sa.wait_id] += sa.samples;
            key3 k{ q,sa.wait_id,sa.pid };
            sum_all[sa.wait_id] += sa.samples;
            if (sa.btype == 0) ++qid_wait_pid_count[k]; // only for FG
            k.pid = sa.btype;   // pid agregate (FG or BG)
            if (sa.btype > 0) {
                qid_wait_pid_BG_sum[k] += sa.samples;
                sum_bg[sa.wait_id] += sa.samples;
                pid_btype[sa.pid] = sa.btype;
            }
            else qid_wait_pid_FG_sum[k] += sa.samples;
        }
        ///// GUI panel
        int nwx = selP.x;
        int w_right = sz.GetWidth() - LegengWidth - selP.x;
        int w_left = selP.x - YaxisWidth;
        int w_panel = 300;
        int h_panel = 300;
        if (w_left > w_right) {
            nwx = selP.x - w_panel;
            if (nwx < 0) nwx = 3;
        }
        int nwy = a.y + 5;

        wxRect pp(nwx, nwy, w_panel, h_panel);
        key3 prev{ -1,-1,-1 };
        std::map<key3, int> qid_wait_countPID; //count(pid)  GROUP (qid,wait)
        for (const auto& pair : qid_wait_pid_count) {
            key3 k = pair.first;
            if (!k.cmp2field(prev))
            {
                prev = k;
                prev.pid = -1;
                prev.sum = 0;
            }
            ++qid_wait_countPID[prev];
        }
        wxString sql;
        int itog = 0;
        std::vector<int> map_sum_all = sort_vec_map(sum_all, itog);
        int period = ws->getPeriod();
        int total = itog;
        float limit = 0.1;
        if (m_agg_int >= 10 * 60000) limit = 1.0;
        if (total != 0) {
            {
                wxDCFontChanger nfont(dc, dc.GetFont());
                wxSize normal_sz = dc.GetTextExtent("H");
                int max_title_sz_x = 0;
                for (int level = 1; level < 3; ++level) {
                    wxFont f = dc.GetFont();
                    f.SetPointSize(f.GetPointSize() - 1);
                    // panel
                    if (m_fix_detail_idx == -1)
                        dc.SetPen(ws->GetColorGui(Color_GUI::LABEL));
                    else
                        dc.SetPen(*wxBLUE_PEN);

                    dc.SetBrush(ws->GetColorGui(Color_GUI::BG));
                    dc.DrawRectangle(pp);
                    // Title
                    ypos = pp.y + 3;
                    {
                        wxDCFontChanger nfont(dc, dc.GetFont().Bold());
                        wxDateTime dttmp = dt;
                        dttmp -= wxTimeSpan(0, 0, m_agg_int / 1000);
                        wxString tit = dttmp.Format("%d %H:%M:%S ") + wxString::Format("[period: %ds]", m_agg_int / 1000);
                        title_sz = dc.GetTextExtent(tit);
                        wxPoint p;
                        p.y = ypos;
                        p.x = pp.x + pp.width / 2 - title_sz.x / 2;
                        dc.DrawText(tit, p);
                        ypos += title_sz.y + VERT_SPC;
                    }
                    //
                    for (int i = 0; i < map_sum_all.size(); i++) {
                        int wait_index = map_sum_all[i];
                        long long w_sum = (100 * sum_all[wait_index] / total);
                        float sek = (period * sum_all[wait_index]) / 1000.0;
                        if (sek < 1) continue;
                        wxString w_name = ws->GetName(wait_index, WAIT_NAME);
                        wxString w_grp = ws->GetName(wait_index, WAIT_GRP);
                        wxString pr = w_name + wxString::Format(" %lld%%(%.1fs)", w_sum, sek);
                        long clr = ws->GetColorByWaitName(w_grp);
                        c.Set(clr);
                        dc.SetBrush(c);
                        p.x = pp.x + 3;
                        p.y = ypos;
                        dc.DrawRectangle(p.x, p.y, normal_sz.GetHeight(), normal_sz.GetHeight());
                        p.x = p.x + normal_sz.GetHeight() + 3;
                        wxString bgproc;
                        int v = -1;
                        // background wait
                        int sum_bg_w = 0;
                        for (const auto& pair : qid_wait_pid_BG_sum) {
                            key3 k = pair.first;
                            if (k.w == wait_index) {
                                v = pair.second * period;
                                sum_bg_w += v;
                                int bt = -1;
                                //if (pid_btype.find(k.pid)!= pid_btype.end()) bt= pid_btype.at(k.pid);
                                bt = k.pid;
                                wxString txt;
                                if (bt >= 0) txt = ws->GetBackendTypeNameShort(bt);
                                w_sum = v;
                                float f = (float)w_sum / 1000;
                                if (f < limit) continue;
                                if (!bgproc.IsEmpty()) bgproc += ',';
                                bgproc += wxString::Format("%s(%.1fs)", txt, (float)f);
                                //title_sz = dc.GetTextExtent(txt + pr);
                                //p.x = p.x + title_sz.GetHeight() + 3;
        //                        p.y = ypos;
        //                        dc.DrawText(txt + pr, p);
        //                        ypos = p.y + title_sz.GetHeight() + 1;

                            }
                        }
                        if (!bgproc.IsEmpty()) {
                            float f = (100 * sum_bg_w / (sum_all[wait_index] * period));
                            bgproc = wxString::Format("BG[%d%%]: ", (int)f) + bgproc;
                        }
                        title_sz = dc.GetTextExtent(pr + bgproc);

                        dc.DrawText(pr + bgproc, p);
                        ypos = p.y + normal_sz.GetHeight() + 1;
                        if (max_title_sz_x < (p.x + title_sz.GetWidth())) max_title_sz_x = p.x + title_sz.GetWidth(); // max width
                        {
                            wxDCFontChanger nfont(dc, f);
                            // detail client
                            std::vector<key3> sortv;
                            for (const auto& pair : qid_wait_pid_FG_sum) {
                                key3 k = pair.first;
                                if (k.w == wait_index) {
                                    v = pair.second * period;
                                    k.sum = v;
                                    if (v < 100) continue;
                                    bool ff = true;
                                    for (int p = 0; p < sortv.size(); p++) {
                                        if (sortv[p].sum < v) {
                                            sortv.insert(sortv.begin() + p, k);
                                            ff = false;
                                            break;
                                        }
                                    }
                                    if (ff) sortv.push_back(k);
                                }
                            }
                            for (int j1 = 0; j1 < sortv.size(); j1++) {
                                key3 k = sortv[j1];
                                if (k.w == wait_index) {
                                    v = k.sum;
                                    wxString txt;
                                    w_sum = v;
                                    float fsek = (float)w_sum / 1000;
                                    if (fsek < limit) continue;
                                    unsigned long long ull = k.qid;
                                    int h = ull >> 32;
                                    int l = ull & 0xFFFFFFFF;
                                    wxString tqid = wxString::Format("%llx", ull);
                                    k.pid = -1;
                                    int cnt = qid_wait_countPID[k];

                                    bgproc = wxString::Format("%s ( %.1fs ) [%d]", tqid, (float)fsek, cnt);
                                    title_sz = dc.GetTextExtent(bgproc);
                                    //p.x = p.x + title_sz.GetHeight() + 3;
                                    p.y = ypos;
                                    wxRect r(p.x, p.y, title_sz.GetWidth(), title_sz.GetHeight());
                                    if (r.Contains(mouse) && level == 2) {
                                        wxDCFontChanger nfont(dc, f.Bold());
                                        dc.DrawText(bgproc, p);
                                        if (m_click == 0) {
                                            // hint sql
                                            sql = ws->GetQueryByQid(k.qid);
                                        }
                                        if (m_click == 2) { // RightUp
                                            // set filter qid
                                            if (wxTheClipboard->Open())
                                            {
                                                wxDataObjectComposite* dataobj = new wxDataObjectComposite();
                                                dataobj->Add(new wxTextDataObject(tqid));
                                                wxTheClipboard->SetData(dataobj);
                                                wxTheClipboard->Close();
                                            }
                                            m_click = 0;
                                        }
                                        if (m_click == 1) { // LeftUp
                                            // set filter qid
                                            m_click = 0;
                                            m_filter_detail = tqid;
                                            m_qid_filter = k.qid;
                                            m_regroup = true;
                                            return;
                                        }

                                    }
                                    else
                                        dc.DrawText(bgproc, p);
                                    ypos = p.y + title_sz.GetHeight() + 1;
                                    if (max_title_sz_x < (p.x + title_sz.GetWidth())) max_title_sz_x = p.x + title_sz.GetWidth(); // max width
                                }
                            }
                        }

                    }
                    if (!sql.IsEmpty() && level == 2) {
                        FSQL::FormatterSQL f(sql);
                        if (f.ParseSql(0) == 0) {
                            wxRect r(a.x, a.y, 0, a.height);
                            if (mouse.x > (a.x + a.width / 2)) r.width = mouse.x - a.x;
                            else { r.width = a.x + a.width - mouse.x - 50; r.x = mouse.x + 50; }
                            f.Formating(dc, r);
                        }
                    }
                    // resize 
                    if (level == 1) {
                        if (max_title_sz_x > pp.x + pp.width) {
                            pp.width = max_title_sz_x - pp.x;
                        }
                        if (ypos > pp.y + pp.height) pp.height = ypos - pp.y;
                        if (ypos > sz.y) {
                            wxFont fsmall = dc.GetFont();
                            pp.y = 0;
                            float kf = (sz.y - pp.y) / (float)(ypos - pp.y);
                            if (kf < 0.5) kf = 0.5;
                            int newSize = fsmall.GetPointSize() * kf;
                            fsmall.SetPointSize(newSize);
                            //int width, height;
                            //wxBitmap emptyBitmap(30, 30, dc);
                            //wxMemoryDC temp_dc;
                            //temp_dc.SelectObject(emptyBitmap);
                            //temp_dc.SetFont(fsmall);
                            //temp_dc.GetTextExtent("H", &width, &height);
                            dc.SetFont(fsmall);
                        }
                    }
                } // level
            } // fontchanger
        }

    }
    if (m_click == 1) {
        m_fix_detail_idx = -1;
    }
    //m_click = 0;
}

int wxTopActivity::getTimeSelRange(bool isLeftRange) {
    wxDateTime r;
    if (isLeftRange) r = fix_pos_L; else r = fix_pos_R;
    if (!r.IsValid()) return -1;
    wxLongLong ll(r.GetValue());
    int msvaluetime = ws->d_time(ll.GetValue());
    return msvaluetime;
}
void wxTopActivity::paintSelRange(wxDC& dc, int width_sample) {
    // selCuror or selRect
    wxDateTime t;
    if (m_fix_detail_idx != -1)
        return;
    int cx = m_area.x + m_area.width - 1;
    if (m_area.Contains(mouse)) {
        int dl = cx - mouse.x;
        dl = dl / width_sample;
        if (dl >= 0 && dl < xAxis.size()) {
            t = xAxis.at(dl);
            // t.Add(wxTimeSpan(0, 0, 0, m_agg_int));
        }
    }
    wxColour c;
    c.Set(0x00001fff);
    dc.SetPen(c);
    long dx1 = 0, dx2 = 0;
    wxDateTime t1, t2;
    if (xAxis.size() > 0) t1 = xAxis.at(0);
    if (fix_pos_L.IsValid()) {
        //t1.Add(wxTimeSpan(0, 0, 0, m_agg_int));
        if (t1 > fix_pos_L) {
            wxTimeSpan ll = t1 - fix_pos_L;
            dx1 = (ll.GetMilliseconds() * width_sample / m_agg_int).GetValue();
            if (m_area.width > (dx1)) {
                wxPoint p1(cx - dx1, m_area.y + 1);
                wxPoint p2(cx - dx1, m_area.y + m_area.height);
                {
                    wxDCPenChanger npen(dc, wxPen(ws->GetColorGui(Color_GUI::CURSOR_LINE), 2, wxPENSTYLE_SOLID));
                    dc.DrawLine(p1, p2);
                }
            }
            else dx1 = m_area.width;
            if (fix_pos_R.IsValid()) {
                if (t1 > fix_pos_R) {
                    wxTimeSpan ll = t1 - fix_pos_R;
                    dx2 = ((ll.GetMilliseconds() * width_sample / m_agg_int)).GetValue();
                    if (m_area.width > (dx2)) {
                        wxPoint p1(cx - dx2, m_area.y + 1);
                        wxPoint p2(cx - dx2, m_area.y + m_area.height);
                        {
                            wxDCPenChanger npen(dc, wxPen(ws->GetColorGui(Color_GUI::CURSOR_LINE), 2, wxPENSTYLE_SOLID));
                            dc.DrawLine(p1, p2);
                        }
                    }
                }
                else dx2 = 0;

            }
        }
        //t1.Add(wxTimeSpan(0, 0, 0, m_agg_int));
    }
    wxString labeltext;
    if (t.IsValid()) labeltext = t.Format("%d %H:%M:%S");
    wxSize sz = dc.GetTextExtent(labeltext);
    wxPoint p(mouse.x + 3, mouse.y - 3 - sz.y);
    wxPoint p1(mouse.x, m_area.y + 1);
    wxPoint p2(mouse.x, m_area.y + m_area.height - 1);
    wxColour bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK);
    if (!fix_pos_R.IsValid())
    {
        wxRect r(p.x - 1, p.y - 1, sz.x + 2, sz.y + 2);
        {
            // current vert line
            if (m_area.Contains(p2) && m_area.Contains(p1) && t.IsValid()) {
                wxDCTextBgColourChanger bb(dc,wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));
                wxDCTextColourChanger ff(dc,wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));
                dc.SetBrush(bgCol);
                dc.SetPen(*wxBLACK);
                dc.DrawRectangle(r);
                dc.DrawText(labeltext, p);
                dc.SetPen(ws->GetColorGui(Color_GUI::CURSOR_LINE));
                dc.DrawLine(p1, p2);
            }
        }

        dx2 = cx - p1.x;
        if (dx2 < 0) dx2 = 0;
    }
    // arrow line
    wxPoint points[3];
    if (fix_pos_L.IsValid() && (dx2 < dx1)) {
        wxDCTextBgColourChanger bb(dc,wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));
        wxDCTextColourChanger ff(dc,wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));

        wxString labelLeft = fix_pos_L.Format("%d %H:%M:%S");
        wxSize szf = dc.GetTextExtent(labelLeft);
        wxPoint p1(cx - dx1, m_area.y + 25);
        wxPoint p2(cx - dx2, m_area.y + 25);
        dc.SetBrush(wxBrush(ws->GetColorGui(Color_GUI::CURSOR_LINE)));
        dc.SetPen(wxPen(ws->GetColorGui(Color_GUI::CURSOR_LINE)));
        {
            wxDCPenChanger npen(dc, wxPen(ws->GetColorGui(Color_GUI::CURSOR_LINE), 2, wxPENSTYLE_SOLID));
            dc.DrawLine(p1, p2);
        }

        points[0].x = cx - dx1;     points[0].y = m_area.y + 25;
        points[1].x = cx - dx1 + 5; points[1].y = m_area.y + 25 - 4;
        points[2].x = cx - dx1 + 5; points[2].y = m_area.y + 25 + 4;
        wxPoint tx1(cx - dx1 + 2, points[0].y - szf.GetHeight() - 5);
        dc.DrawPolygon(3, points);
        points[0].x = cx - dx2;     points[0].y = m_area.y + 25;
        points[1].x = cx - dx2 - 5; points[1].y = m_area.y + 25 - 4;
        points[2].x = cx - dx2 - 5; points[2].y = m_area.y + 25 + 4;
        dc.DrawPolygon(3, points);
        wxTimeSpan ll;

        if (fix_pos_R.IsValid()) {
            ll = fix_pos_R - fix_pos_L;
            labeltext = fix_pos_R.Format("%d %H:%M:%S");
            sz = dc.GetTextExtent(labeltext);
        }
        else if (t.IsValid()) ll = t - fix_pos_L; else return;
        wxPoint tx2(cx - dx2 - 2 - sz.GetWidth(), points[0].y - sz.GetHeight() - 5);
        wxRect r(tx1.x - 1, tx1.y, szf.x + 2, szf.y);
        dc.SetBrush(bgCol);
        dc.SetPen(*wxBLACK);

        wxString l3text = ElapsedTimeToStr(ll.GetValue());
        wxSize sz3 = dc.GetTextExtent(l3text);
        wxRect r2(tx2.x - 1, tx2.y, sz.x + 2, sz.y);
        int maxx = 0; bool isup = true;
        if (szf.x < (p2.x - p1.x)) {
            // | labelLeft 
            dc.DrawRectangle(r); // inner
            dc.DrawText(labelLeft, r.x+1,r.y);
            maxx = r.width;
        }
        else {
            // labelLeft |
            r.x = tx1.x - 1 - r.width;
            dc.DrawRectangle(r);
            dc.DrawText(labelLeft, r.x + 1,r.y);
        }
        if (sz.x + maxx < (p2.x - p1.x) && fix_pos_R.IsValid()) {
            // labelRight |
            dc.DrawRectangle(r2);
            dc.DrawText(labeltext, r2.x+1,r2.y);
            maxx += r2.width;
        }
        else {
            // | labelRight
            r2.x = cx - dx2 + 2;
            dc.DrawRectangle(r2);
            dc.DrawText(labeltext, r2.x + 1,r2.y);
        }
        if (sz3.x + maxx < (p2.x - p1.x) && false) { // up 
            wxRect r3(r.x + r.width + (p2.x - p1.x - maxx) / 2 - sz3.x / 2, r.y, sz3.x + 2, sz3.y);
            dc.DrawRectangle(r3);
            dc.DrawText(l3text, r3.x + 1, r3.y);
            maxx += r3.width;
        }
        else if (sz3.x < (p2.x - p1.x) ) { // down 
            wxRect r3(tx1.x - 1 + (p2.x - p1.x ) / 2 - sz3.x / 2, points[0].y + 5, sz3.x + 2, sz3.y);
            dc.DrawRectangle(r3);
            dc.DrawText(l3text, r3.x + 1, r3.y);
        }
    }

    if (m_click == 2 && !fix_pos_L.IsValid()) {
        fix_pos_L = t;
    }
    else if (m_click == 2 && !fix_pos_R.IsValid()) {
        if (fix_pos_L < t) fix_pos_R = t; else fix_pos_L = wxLongLong(wxINT64_MIN);

    }
    else if (m_click == 2 && fix_pos_L.IsValid() && fix_pos_R.IsValid()) {
        fix_pos_L = wxLongLong(wxINT64_MIN);
        fix_pos_R = wxLongLong(wxINT64_MIN);
    }



}
void wxTopActivity::paintEvent(wxPaintEvent& evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxBufferedPaintDC dc(this);
    int tmp = m_click;
    render(dc);
    if (tmp != m_click) render(dc);
    m_click = 0;
    //wxString sql = "select $1 '\'' ''' ($$fsda$$ $as$vvv  $as$), e'\\'text' \"b\" $1,45.78 \"sas\"  U&' '' ' '1line'\n'2 line' from table a\n where a.id=23";

    //FormatterSQL f(sql);
    //int rez=f.ParseSql(0);
    //if (rez < 0) {
    //    wxLogError(wxString::Format("sql parse error = %d",rez));
    //    return;
    //}
    //wxSize sz=GetSize();
    //wxRect r(0, 0, sz.x, sz.y);
    //f.Formating(dc, r);
}



void wxTopActivity::mouseDown(wxMouseEvent& event)
{
    wxPoint p = event.GetPosition();
    if (m_area.Contains(p)) {
        mouseS = p;
    }
    else mouseS = wxPoint(0, 0);
}


void wxTopActivity::mouseReleased(wxMouseEvent& event)
{
    //delete m_simplePopup;
    //m_simplePopup = new SimpleTransientPopup(this, false);
    //wxPoint pos = ClientToScreen(event.GetPosition());
    //wxSize sz = GetSize();
    //m_simplePopup->Move(pos);
    //m_simplePopup->Popup();

    if (event.LeftUp()) m_click = 1;
    if (event.RightUp())
        m_click |= 2;
    mouseS = wxPoint(0, 0);

    Refresh();
    // wxMessageBox(wxT("You pressed a custom button"));
}
void wxTopActivity::mouseLeftWindow(wxMouseEvent& event)
{
}
void wxTopActivity::OnEraseBackground(wxEraseEvent& event) {

}
// currently unused events
void wxTopActivity::mouseMoved(wxMouseEvent& event) {
    wxPoint pos = event.GetPosition();
    mouse = pos;
    if (mouseS != wxPoint(0, 0)) {
        // shift right bound
        int d = pos.x - mouseS.x;
        mouseS = pos;
        if (d > 0) {
            // back shift
            int vtime = ws->GetHomeInterval(m_RightTime, m_agg_int);
            //vtime--;
            //vtime = ws->GetHomeInterval(vtime, m_agg_int);
            int max_grp = m_area.width / 2;
            if (vtime <= m_agg_int * max_grp) return;
            if (vtime / m_agg_int > 50) m_RightTime = vtime - d * m_agg_int;
            if (m_RightTime <= 0) m_RightTime = vtime;

        }
        else {
            d = d * -1;
            int vtime = ws->GetHomeInterval(m_RightTime, m_agg_int);
            int vend = ws->GetHomeInterval(-1, m_agg_int);
            if (vend != vtime) {
                m_RightTime = vtime + d * m_agg_int;
                if (m_RightTime > vend) m_RightTime = vend;
            }
        }
        m_regroup = true;
    }
    Refresh();
}
wxSize wxTopActivity::DoGetBestClientSize() {
    return setsize;
};

void wxTopActivity::resize(wxSizeEvent& event) {
    Refresh();
    m_regroup = true;
};


void wxTopActivity::mouseWheelMoved(wxMouseEvent& event) {
    int r = event.GetWheelRotation();
    int n = -1;
    for (int i = 0; i < sizeof(m_inter) / sizeof(int); i++)
    {
        if (m_inter[i] == m_agg_int) {
            n = i;
            if (r < 0) {
                i--;
                n = i < 0 ? -1 : i;
                break;
            }
            else {
                i++;
                n = i >= (sizeof(m_inter) / sizeof(int)) ? -1 : i;
                break;
            }
        }
    }
    if (n != -1) {
        m_agg_int = m_inter[n];
        m_regroup = true;
        Refresh(true);
    }
}
void wxTopActivity::setViewRange(int m_aggregate_interval, int RightTime) {
    m_regroup = true;
    m_agg_int = m_aggregate_interval;
    m_RightTime = RightTime;
}
/// <summary>
/// ��������� ������� WaitSample � ��������� ����������� ����������.
/// 
/// </summary>
/// <param name="m_aggregate_interval"> ���������� � ������� ������������ ������� ���������� ������</param>
/// <param name="RightTime"> ��������� � ������� ������������ ������� ������ �������</param>
/// <returns> ���������� ����� WaitSample �������</returns>
WaitSample* wxTopActivity::getViewRange(int& m_aggregate_interval, int& RightTime) {

    m_aggregate_interval = m_agg_int;
    RightTime = m_RightTime;
    return ws;
}

void wxTopActivity::rightClick(wxMouseEvent& event) {}
void wxTopActivity::keyPressed(wxKeyEvent& event) { }
void wxTopActivity::keyReleased(wxKeyEvent& event) {}
/////
BEGIN_EVENT_TABLE(topDataViewCtrl, wxDataViewCtrl)
//EVT_MOTION(topDataViewCtrl::OnMouseMove)
END_EVENT_TABLE()

//void topDataViewCtrl::OnMouseMove(wxMouseEvent& event) {}

bool topDataViewCtrl::CalcRowsDataView(bool force, bool IsPidFirst) {
    int sel_left = top->getTimeSelRange(true);
    agg = top->getAggregateInterval();
    int sel_right = top->getTimeSelRange(false);
    if (sel_left > -1 && sel_right > -1) {
        // set select range
    }
    else {
        sel_left = 0;
        sel_right = w->GetHomeInterval(-1, agg);
    }
    if (sel_left == left_g && sel_right == right_g && !force)
        return false;
    // recalc
    int ps = w->getPositionByTime(sel_left);
    std::vector<Sample>* smp = w->GetSamples();
    Sample sa;
    int m_count_wait = w->GetCountWaits();
    std::map<key3, vec_int> wait_id_SUM; // Group by qid,pid, sum(wait)
    std::map<key3, int> PID_SUM; // 
    std::map<key3, int> QID_SUM; // 
    std::map<key3, int> QID_PID_SUM; // 
    std::map<key3p, int> PID_QID_SUM; //
    std::map<int, int> PID_BG; // 
    int sz2 = smp->size();
    long long col1 = -3;
    while (ps < sz2)
    {
        sa = smp->at(ps++);
        if (sa.btime >= (sel_right)) break;
        long long q = sa.qid;
        if (sa.btype > 0 && ignoreBG) {
            continue;
        }
        if (PID_BG.count(sa.pid) == 0) PID_BG[sa.pid] = sa.btype;
        key3 k{ q,-1,sa.pid }; // mask wait_id 
        key3p k2{ q,-1,sa.pid }; // mask wait_id 
        QID_PID_SUM[k] += sa.samples;
        PID_QID_SUM[k2] += sa.samples;
        // group by pid,qid
        auto iter = wait_id_SUM.find(k);
        if (iter != wait_id_SUM.end()) {
            //std::cout << "Found the key " << iter->first << " with the value " << iter->second << "\n";
            iter->second[sa.wait_id] += sa.samples;
        }
        else {
            vec_int array_wait(m_count_wait);
            array_wait[sa.wait_id] += sa.samples;
            wait_id_SUM.emplace(k, array_wait);
        }
        // Add Group by Pid
        key3 k_all_pid{ -1,-1,sa.pid }; // mask wait_id ,qid
        iter = wait_id_SUM.find(k_all_pid);
        if (iter != wait_id_SUM.end()) {
            //std::cout << "Found the key " << iter->first << " with the value " << iter->second << "\n";
            iter->second[sa.wait_id] += sa.samples;
        }
        else {
            vec_int array_wait(m_count_wait);
            array_wait[sa.wait_id] += sa.samples;
            wait_id_SUM.emplace(k_all_pid, array_wait);
        }
        PID_SUM[k_all_pid] += sa.samples; //All waits for PID
        // Add Group by Qid
        key3 k_all_qid{ sa.qid,-1,-1 }; // mask wait_id ,pid
        iter = wait_id_SUM.find(k_all_qid);
        if (iter != wait_id_SUM.end()) {
            //std::cout << "Found the key " << iter->first << " with the value " << iter->second << "\n";
            iter->second[sa.wait_id] += sa.samples;
        }
        else {
            vec_int array_wait(m_count_wait);
            array_wait[sa.wait_id] += sa.samples;
            wait_id_SUM.emplace(k_all_qid, array_wait);
        }
        QID_SUM[k_all_qid] += sa.samples;//All waits for QID
    }
    //
    //bool IsPidFirst = false;
    //if (GetColumnAt(0)->GetTitle() == "PID") IsPidFirst = true;
    typedef std::pair<key3, int> pair;
    // sort map by value;
    std::vector<pair> vecp;
    if (IsPidFirst) {
        std::copy(PID_SUM.begin(),
            PID_SUM.end(),
            std::back_inserter<std::vector<pair>>(vecp));

    }
    else
    {
        std::copy(QID_SUM.begin(),
            QID_SUM.end(),
            std::back_inserter<std::vector<pair>>(vecp));

    }
    std::sort(vecp.begin(), vecp.end(),
        [](const pair& l, const pair& r)
        {
            if (l.second != r.second) {
                return !(l.second < r.second);
            }
            return l.first < r.first;
        });
    int rows = 0;
    std::vector<key3> ROWS;
    for (auto const& pa : vecp) {
        int col = 0;
        std::vector<pair> vecSORT;
        if (IsPidFirst) {
            key3p k3{ -1,999,pa.first.pid };
            k3.pid--;
            auto l = PID_QID_SUM.upper_bound(k3);
            k3.pid++;
            while (l != PID_QID_SUM.end()) {
                if (l->first.pid != k3.pid) break;
                key3 k4{ l->first.qid,l->first.w, l->first.pid };
                vecSORT.push_back(std::make_pair(k4, l->second));
                ++l;
            }
            if (vecSORT.size() > 1) vecSORT.push_back(std::make_pair(key3{ -1,pa.first.w,pa.first.pid }, pa.second));
        }
        else {
            key3 k3 = pa.first;
            k3.w = 999;
            k3.qid--;
            auto l = QID_PID_SUM.upper_bound(k3);
            k3.qid++;
            while (l != QID_PID_SUM.end()) {
                if (l->first.qid != k3.qid) break;
                vecSORT.push_back(*l);
                ++l;
            }
            if (vecSORT.size() > 1) vecSORT.push_back(std::make_pair(key3{ pa.first.qid,pa.first.w,-1 }, pa.second));
        }
        std::sort(vecSORT.begin(), vecSORT.end(),
            [](const pair& l, const pair& r)
            {
                if (l.second != r.second) {
                    return !(l.second < r.second);
                }
                return l.first < r.first;
            });
        for (auto& k5 : vecSORT) {
            if (PID_BG.count(k5.first.pid)) k5.first.sum = PID_BG[k5.first.pid]; else k5.first.sum = 0;
            ROWS.push_back(k5.first);
        }
    }
    wxDataViewItem selectRow = GetSelection();

    MyIndexListModel* m = static_cast<MyIndexListModel*>(GetModel());
    m->SetAllRows(ROWS, wait_id_SUM, IsPidFirst);
    if (selectRow.IsOk()) {
        SetCurrentItem(selectRow);
        //        EnsureVisible(selectRowGroup);
    }

    left_g = sel_left;
    right_g = sel_right;
    return true;
}
void topDataViewCtrl::BuildColumn(int mode)
{
    AppendColumn(
        new wxDataViewColumn("PID",
            new MyCustomRendererGraph(wxDATAVIEW_CELL_INERT, 0),
            0,
            wxCOL_WIDTH_AUTOSIZE,
            wxALIGN_NOT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE
        ));
    AppendColumn(
        new wxDataViewColumn("QID",
            new MyCustomRendererGraph(wxDATAVIEW_CELL_INERT, 1),
            1,
            wxCOL_WIDTH_AUTOSIZE,
            wxALIGN_NOT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE
        ));
    AppendColumn(
        new wxDataViewColumn("graph",
            new MyCustomRendererGraph(wxDATAVIEW_CELL_INERT, 2),
            2,
            wxCOL_WIDTH_AUTOSIZE,
            wxALIGN_NOT,
            wxDATAVIEW_COL_RESIZABLE
        ));

};
/// <summary>
/// /// Small panel 
/// </summary>
class wxCustomButton : public wxWindow
{

    bool pressedDown;
    wxString text;
    wxPoint cur;
    wxPoint cur_prev;
    WaitSample* ws;
    static const int buttonWidth = 200;
    static const int buttonHeight = 50;

public:
    wxCustomButton(wxFrame* parent, wxString text, WaitSample* WS);

    void paintEvent(wxPaintEvent& evt);
    void OnEraseBackground(wxEraseEvent& event);
    void paintNow();

    void render(wxDC& dc);

    // some useful events
    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void rightClick(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);
    void keyPressed(wxKeyEvent& event);
    void keyReleased(wxKeyEvent& event);

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(wxCustomButton, wxPanel)

EVT_MOTION(wxCustomButton::mouseMoved)
EVT_LEFT_DOWN(wxCustomButton::mouseDown)
EVT_LEFT_UP(wxCustomButton::mouseReleased)
EVT_RIGHT_DOWN(wxCustomButton::rightClick)
EVT_LEAVE_WINDOW(wxCustomButton::mouseLeftWindow)
EVT_KEY_DOWN(wxCustomButton::keyPressed)
EVT_KEY_UP(wxCustomButton::keyReleased)
EVT_MOUSEWHEEL(wxCustomButton::mouseWheelMoved)

// catch paint events
EVT_PAINT(wxCustomButton::paintEvent)
EVT_ERASE_BACKGROUND(wxCustomButton::OnEraseBackground)

END_EVENT_TABLE()



wxCustomButton::wxCustomButton(wxFrame* parent, wxString text, WaitSample* WS) :
    wxWindow(parent, wxID_ANY)
{
    ws = WS;
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(buttonWidth, buttonHeight));
    this->text = text;
    pressedDown = false;

}

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

void wxCustomButton::paintEvent(wxPaintEvent& evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxBufferedPaintDC dc(this);
    render(dc);
}

/*
 * Alternatively, you can use a clientDC to paint on the panel
 * at any time. Using this generally does not free you from
 * catching paint events, since it is possible that e.g. the window
 * manager throws away your drawing when the window comes to the
 * background, and expects you will redraw it when the window comes
 * back (by sending a paint event).
 */
void wxCustomButton::paintNow()
{
    // depending on your system you may need to look at double-buffered dcs
    //wxPaintDC dc(this);
    //render(dc);
    this->Refresh(false);
}

struct ClrSum {
    int clr;
    int sum = 0;
};
/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void wxCustomButton::render(wxDC& dc)
{
    if (pressedDown)
        dc.SetBrush(*wxRED_BRUSH);
    else
        dc.SetBrush(ws->GetColorGui(Color_GUI::BG));
    wxRect r = dc.GetWindow()->GetClientRect();
    wxSize sz = r.GetSize();
    //dc.DrawRectangle(0, 0, r.width, r.height);
    dc.DrawRectangle(0, 0, sz.x, sz.y);
    wxColour c;
    //dc.SetPen(*wxTRANSPARENT_PEN);
    int widthLine = 4;
    int period = 10, interval = 1000;
    int countLine = sz.GetWidth() / widthLine;
    int full = sz.GetHeight() - 3;
    int minHeight = 4;
    int maxHeight = minHeight * 2;
    int minRange = 200; // ms 
    int x = sz.GetWidth() - widthLine;
    int y = sz.GetHeight();
    std::vector<Sample>* smp = ws->GetSamples();
    Sample sa;
    int vtime = 0;
    std::vector<Sample>::iterator it = smp->end();


    int idx = smp->size() - 1;

    // vertical serios
    std::vector<int> clr = ws->GetColors();
    std::vector<int> hz = clr;
    std::vector<ClrSum> graph;
    for (int i = 0; i < hz.size(); i++) hz[i] = 0;
    // sum(samples) from  group by wait_id
    int prev_c = -1;
    int sum_by_clr = 0;
    int serios_sum = 0;
    int prev_t = -1;
    while (idx >= 0 && countLine > 0) {
        sa = smp->at(idx);
        if (prev_t == -1) prev_t = sa.btime;
        hz[sa.wait_id] += sa.samples; // sum()
        int c = clr[sa.wait_id];

        if (sa.btime != prev_t) {
            // new serios
            // new color
            ClrSum cs;
            cs.clr = prev_c;
            cs.sum = sum_by_clr;
            graph.push_back(cs);
            prev_c = c;
            sum_by_clr = 0;
            y = sz.GetHeight();
            bool isNext = true;
            while (isNext) {
                int h = 0;
                isNext = false;
                for (auto i : graph) {
                    if (i.sum * period < minRange) h += minHeight;
                    else h += maxHeight;
                    if (h > full && minHeight >= 3) {
                        if (minRange == 1) {
                            minHeight--;
                        }
                        if (minRange > 1) minRange--;
                        isNext = true;
                        break;
                    }
                }
            }
            // draw serios
            int h = 0;
            wxColour c;
            for (auto i : graph) {
                if (i.sum * period < minRange) h = minHeight;
                else h = maxHeight;
                unsigned long clr = i.clr;
                c.Set(clr);
                dc.SetBrush(c);
                dc.DrawRectangle(x, y - h, widthLine, h);
                y = y - h;
            }
            x = x - widthLine;
            int skip = (prev_t - sa.etime) / interval; // �� ���� �������� 
            if (skip > 0) {
                x -= skip * widthLine;
            }

            graph.clear();
            serios_sum = 0;
            countLine--;
            // return;
        }
        if (prev_c == -1) prev_c = c;
        if (prev_c != c) {
            // new color
            ClrSum cs;
            cs.clr = prev_c;
            cs.sum = sum_by_clr;
            graph.push_back(cs);
            prev_c = c;
            sum_by_clr = 0;
        }
        sum_by_clr += sa.samples;
        serios_sum += sa.samples;
        prev_t = sa.btime;
        idx--;
    }


    wxString s = wxString::Format("x:%d,y:%d", cur.x, cur.y);
    dc.DrawText(s, 20, 15);
}

void wxCustomButton::mouseDown(wxMouseEvent& event)
{
    pressedDown = true;
    paintNow();
}

void wxCustomButton::mouseReleased(wxMouseEvent& event)
{
    pressedDown = false;
    paintNow();
    //delete m_simplePopup;
    //m_simplePopup = new SimpleTransientPopup(this, false);
    //wxPoint pos = ClientToScreen(event.GetPosition());
    //wxSize sz = GetSize();
    //m_simplePopup->Move(pos);
    //m_simplePopup->Popup();


   // wxMessageBox(wxT("You pressed a custom button"));
}
void wxCustomButton::mouseLeftWindow(wxMouseEvent& event)
{
    if (pressedDown)
    {
        pressedDown = false;
        paintNow();
    }
}
void wxCustomButton::OnEraseBackground(wxEraseEvent& event) {

}
// currently unused events
void wxCustomButton::mouseMoved(wxMouseEvent& event) {
    wxPoint pos = event.GetPosition();

    if (cur != pos) {
        cur = pos;
        paintNow();
    }
}
void wxCustomButton::mouseWheelMoved(wxMouseEvent& event) {}
void wxCustomButton::rightClick(wxMouseEvent& event) {}
void wxCustomButton::keyPressed(wxKeyEvent& event) {}
void wxCustomButton::keyReleased(wxKeyEvent& event) {}


