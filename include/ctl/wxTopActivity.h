#pragma once
#include <wx/wx.h>
#include "utils/WaitSample.h"
#include "wx/popupwin.h"
#include "wx/dataview.h"
#include "wx/headerctrl.h"
#include <wx/tokenzr.h>
#include <vector>
//#include "utils/misc.h"
// ----------------------------------------------------------------------------
// MyCustomRendererText
// ----------------------------------------------------------------------------
extern std::vector<int> sort_vec_map(const std::vector<int>& src, int& sum);
#define sepListWait ";"
extern int s_pid_HIGHLIGH;
class MyCustomRendererGraph : public wxDataViewCustomRenderer
{
public:
    // This renderer can be either activatable or editable, for demonstration
    // purposes. In real programs, you should select whether the user should be
    // able to activate or edit the cell and it doesn't make sense to switch
    // between the two -- but this is just an example, so it doesn't stop us.
    explicit MyCustomRendererGraph(wxDataViewCellMode mode, int column)
        : wxDataViewCustomRenderer("string", mode, wxALIGN_LEFT)
    {
        EnableEllipsize(wxELLIPSIZE_END);
        col = column;
    }

    virtual bool Render(wxRect rect, wxDC* dc, int state) wxOVERRIDE
    {
        wxString l;
        wxRect orig = rect;
        if (col < 2) {
            l = m_value;
            if (m_value == "ffffffffffffffff" || m_value == "-1") {
                l = "SUM";
                RenderText(l, 0, rect, dc, state);
                return true;
            }
        }

        if (col == 0) {
            if (m_value[0] == '*') {
                l = m_value.AfterFirst('*');

                // wxFont fn = dc->GetFont().Italic();
                // wxDCFontChanger nfont(*dc, fn);
                dc->SetBrush(*wxLIGHT_GREY_BRUSH);
                dc->SetPen(*wxTRANSPARENT_PEN);
                rect.Deflate(1);
                dc->DrawRoundedRectangle(rect, 3);
                RenderText(l, 0, rect, dc, state);
            }
            else
            {
                l = m_value;
                RenderText(l, 0, rect, dc, state);
            }

            return true;

        }
        if (col == 1) {
            RenderText(l, 0, rect, dc, state);
            return true;
        }
        //dc->SetBrush( *wxLIGHT_GREY_BRUSH );
        dc->SetBrush(*wxYELLOW_BRUSH);
        //dc->SetPen(*wxTRANSPARENT_PEN);
        dc->SetPen(*wxBLACK_PEN);
        rect.Deflate(1);
        //WaitSample*ws= static_cast<topDataViewCtrl*>(GetView())->GetSample();
        //dc->DrawRoundedRectangle( rect, 3 );
        wxFont fn = dc->GetFont();
        fn.SetPointSize(fn.GetPointSize() - 1);
        wxDCFontChanger nfont(*dc, fn);

        wxString s = m_value, t;
        wxStringTokenizer tk(s, sepListWait, wxTOKEN_DEFAULT);

        bool isDis = true;
        bool isGrp = true;
        int f = 0;
        float itog = 0;
        int widt = orig.width - 20;
        wxPoint p(orig.x, orig.y);
        while (tk.HasMoreTokens())
        {
            l = tk.GetNextToken();
            if (f == 0) {
                float tmp;
                l = l.AfterLast(' ');
                wxSscanf(l, "%f", &tmp);
                itog = tmp;
                float sek = itog / 1000;
                wxString tt = wxString::Format("%.1f", sek); // time sec
                wxSize sz = GetTextExtent(tt);
                wxRect drw(orig.x + orig.width - sz.x - 2, orig.y, sz.x + 2, orig.GetHeight());
                wxRect npos(0, 0, sz.x, sz.y);
                npos = npos.CentreIn(drw);
                dc->DrawText(tt, npos.x, npos.y);
                widt = orig.width - sz.x - 2;
            }
            if (f > 0) {
                float tmp;
                wxString w;
                long cl;
                l = l.AfterFirst(' ');
                wxSscanf(l, "%ld %f", &cl, &tmp);

                wxColour c(cl);
                //c.Set(cl);
                dc->SetBrush(c);


                int ww = widt * tmp;
                wxRect drw(p.x, p.y, ww, rect.GetHeight());
                float sek = itog * tmp / 1000;
                wxString tt = wxString::Format("%.1f", sek); // time sec
                wxSize sz = GetTextExtent(tt);
                wxRect npos(0, 0, sz.x, sz.y);
                npos = npos.CentreIn(drw);
                dc->DrawRectangle(drw);
                {
                    wxColour fg(ContrastColorBlackOrWhite(c));
                    wxDCTextColourChanger setfg(*dc, fg);
                    if (sz.GetWidth() < drw.GetWidth()) dc->DrawText(tt, npos.x, npos.y);
                }
                p.x = p.x + ww;

            }
            f++;

        }

        return true;
    }

    virtual bool ActivateCell(const wxRect& WXUNUSED(cell),
        wxDataViewModel* WXUNUSED(model),
        const wxDataViewItem& WXUNUSED(item),
        unsigned int WXUNUSED(col),
        const wxMouseEvent* mouseEvent) wxOVERRIDE
    {
        return false;
    }

    virtual wxSize GetSize() const wxOVERRIDE
    {
        wxSize txtSize = GetTextExtent(m_value);
        int lines = m_value.Freq('\n') + 1;
        if (lines > 1) {
            // wxLogMessage("MyCustomRendererText GetSize() %s", position);
            txtSize.SetHeight(txtSize.GetHeight() * lines + 1 * lines);
        }
        else {
#ifdef __WXGTK__
            txtSize.SetHeight(txtSize.GetHeight() + 3);
#else
            txtSize.SetHeight(-1);
#endif


        }
        if (col == 2) txtSize.SetWidth(-1);
        else
        {
            //maxw = txtSize.GetWidth();
            //maxw=wxMax(txtSize.GetWidth(), maxw);
            txtSize.SetWidth(txtSize.GetWidth() + 1);
        }
        return txtSize;

        //return GetView()->FromDIP(wxSize(60, 20));
    }

    virtual bool SetValue(const wxVariant& value) wxOVERRIDE
    {
        m_value = value.GetString();
        return true;
    }

    virtual bool GetValue(wxVariant& WXUNUSED(value)) const wxOVERRIDE { return true; }

#if wxUSE_ACCESSIBILITY
    virtual wxString GetAccessibleDescription() const wxOVERRIDE
    {
        return m_value;
    }
#endif // wxUSE_ACCESSIBILITY

    virtual bool HasEditorCtrl() const wxOVERRIDE { return false; }
private:
    wxString m_value;
    int col;
    int maxw = -1;
};

class MyIndexListModel : public wxDataViewIndexListModel
{
public:
    MyIndexListModel(WaitSample* w) { ws = w; }

    void SetAllRows(const std::vector<key3>& keys, const std::map<key3, vec_int>& wait, bool isfp) {
        k = keys;
        w = wait;
        //IsFirstPid = isfp;

        Reset(keys.size());

    }
    key3 GetRowValue(long row) {
        key3 r{};
        if (row >= 0 && row < k.size()) {
            r = k[row];
        }
        return r;
    }
    // Implement base class pure virtual methods.
    unsigned GetCount() const wxOVERRIDE { return k.size(); }
    void GetValueByRow(wxVariant& val, unsigned row, unsigned col) const wxOVERRIDE
    {
        //val=wxString::Format("r:%d,c:%d",row,col);
        //val = m_strings[row];
        //if (row + 1 < m_strings.Count() && col == 1 ) val = m_strings[row + 1];
        bool ishint = false;
        int r = row;
        if (!val.IsNull()) {
            //r = r * -1;
            ishint = true;
        }
        if (k.size() > 0) {
            key3 kk = k.at(r);
            if ((col == 0)) {
                long ttmp = kk.pid;
                if (kk.sum > 0) val = wxString::Format("*%ld", ttmp); // backend
                else
                    val = wxString::Format("%ld", ttmp);

            }
            else
                val = wxString::Format("%llx", kk.qid);
            if (col == 2) {
                vec_int v = w.at(kk);
                int sz = v.size();
                val = wxString::Format("count w=%d", sz);
                int itog = 0;
                std::vector<int> map_sum_all = sort_vec_map(v, itog);
                int period = ws->getPeriod();
                int total = itog;
                wxString h;
                if (total != 0) {
                    wxString x = wxString::Format("all %ld", (long)itog * period);
                    for (int i = 0; i < map_sum_all.size(); i++) {
                        int wait_index = map_sum_all[i];
                        if (v[wait_index] == 0) continue;
                        float w_sum = (v[wait_index] / (float)total);
                        float sek = ((period * v[wait_index]) / 1000.0);
                        wxString w_name = ws->GetName(wait_index, WAIT_NAME);
                        wxString w_grp = ws->GetName(wait_index, WAIT_GRP);
                        wxString full = ws->GetName(wait_index, WAIT_FULL);
                        long clr = ws->GetColorByWaitName(full);
                        wxString pr = wxString::Format("%s %ld %.3f", w_name, clr, w_sum);
                        if (ishint) {
                            if (!h.IsEmpty()) h += "\n";
                            h.Append(wxString::Format("%-8.1f%-15s", sek, w_name));
                        }
                        else {
                            if (!x.IsEmpty()) x += sepListWait;
                            x += pr;
                        }
                    }
                    if (ishint) val = h;
                    else
                        val = x;
                }

            }
        }
    }
    bool SetValueByRow(const wxVariant&, unsigned, unsigned) wxOVERRIDE
    {
        return false;
    }
    unsigned GetRow(const wxDataViewItem& item) const wxOVERRIDE { return (unsigned long)item.GetID() - 1; } // 0 row = header 
    unsigned int GetColumnCount() const wxOVERRIDE { return 0; }
    wxString GetColumnType(unsigned int) const wxOVERRIDE { return ""; }


private:
    bool IsFirstPid = true;
    std::vector<key3> k;
    std::map<key3, vec_int> w;
    WaitSample* ws = NULL;
    wxDECLARE_NO_COPY_CLASS(MyIndexListModel);
};

//----------------------------------------------------------------------------
// SimpleTransientPopup
//----------------------------------------------------------------------------
class wxTopActivity;
class topDataViewCtrl;

class SimpleTransientPopup : public wxFrame
{
public:
    friend class wxTopActivity;
    friend class topDataViewCtrl;
    SimpleTransientPopup(wxWindow* parent, bool scrolled, wxTopActivity* small_ctl, wxPoint p, wxString title);
    virtual ~SimpleTransientPopup();

    // wxPopupTransientWindow virtual methods are all overridden to log them

private:
    //wxScrolledWindow* m_panel;
    wxPanel* m_panel;
    wxTopActivity* top;
    topDataViewCtrl* dvc;
    wxObjectDataPtr<MyIndexListModel> m_index_list_model;
    wxPoint mouseDownPos_;
    bool dragg = false;
    wxTimer m_taskTimer;
private:
    void OnMouse(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnSetFocus(wxFocusEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    void OnTimerEvent(wxTimerEvent& pEvent);
    void OnClose(wxCommandEvent& event);

private:
    wxDECLARE_ABSTRACT_CLASS(SimpleTransientPopup);
    wxDECLARE_EVENT_TABLE();
};

struct title_info {
    bool iner = false;
    bool remove;
    bool down_line;
    int total;
    wxString title;

};
/// <summary>
/// Графический элемент отображающий данные WaitSample
/// Может использоваться в двух режимах.
/// </summary>
class wxTopActivity : public wxControl
{
    WaitSample* ws;
    wxString m_title;
    wxSize setsize;
    wxArrayString seriosName;
    std::vector<wxDateTime> xAxis;
    std::vector<wxTimeSpan> yAxis;
    std::vector<vec_int> val;
    std::vector<long> colors;
    std::vector<title_info> m_title_i;
    std::map<wxString, int> m_collapse;
    std::map<wxString, int> m_sumtotal;
    wxPoint mouse, mouseS;
    wxRect m_area;
    int m_click = 0;
    int m_fix_detail_idx = -1;
    // fix sel Range
    wxDateTime fix_pos_L;
    wxDateTime fix_pos_R;

    wxPoint m_fix_pos; // context win
    wxString m_filter_detail;
    long long m_qid_filter = -1;
    bool m_regroup = false;
    int m_lastInterval = 0;
    int m_RightTime = -1;
    wxString m_filter;
    int m_agg_int = 5000;
    int m_count_wait;
    int m_inter[9] = {
        5000      ,// 5 sek
        10000     ,//10 sek 
        30000     ,//30 s
        60000     ,//1 min
        5 * 60000 ,//5 min
        10 * 60000,//10 min
        15 * 60000,//15 min
        30 * 60000,//30 min
        60 * 60000 //60 min
    };
    SimpleTransientPopup* m_simplePopup;
    void paintSelRange(wxDC& dc, int width_sample);
public:
    wxTopActivity(wxWindow* parent, WaitSample* WS, wxSize sz);
    wxSize DoGetBestClientSize();
    void setViewRange(int m_aggregate_interval, int RightTime);
    WaitSample* getViewRange(int& m_aggregate_interval, int& RightTime);
    void paintEvent(wxPaintEvent& evt);
    void OnEraseBackground(wxEraseEvent& event);
    void paintNow();
    void SetFilter(long long qid);
    /// <summary>
    /// Возвращает время указаной границы
    /// </summary>
    /// <param name="isLeftRange">true для левой границы, инаце правая</param>
    /// 
    int getTimeSelRange(bool isLeftRange);
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
    void resize(wxSizeEvent& event);
    DECLARE_EVENT_TABLE()
};

class topDataViewCtrl :
    public wxDataViewCtrl
{
public:
    topDataViewCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = 0,
        wxTopActivity* topactive = NULL,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxASCII_STR(wxDataViewCtrlNameStr)
    ) : wxDataViewCtrl(parent, id, pos, size, style, validator, name)
    {
        //SetMinSize(wxSize(300,200));
        top = topactive;
        w = NULL;
        m_win_s = NULL;
        w = top->getViewRange(agg, right_g);
        Bind(wxEVT_DATAVIEW_COLUMN_HEADER_CLICK, [&](wxDataViewEvent& event) {
            int col = event.GetColumn();
            if (GetColumn(col)->GetTitle() == "PID") {
                ignoreBG = !ignoreBG;
                CalcRowsDataView(true, ispidfirst);
                Refresh();

            }
            });
        Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [&](wxDataViewEvent& event) {
            int col = event.GetColumn();
            wxDataViewItem item(event.GetItem());
            long row = (long)item.GetID();
            row--;
            if (col == 1) //qid
            {
                MyIndexListModel* m = static_cast<MyIndexListModel*>(GetModel());
                key3 k = m->GetRowValue(row);
                if (k.qid != -1) top->SetFilter(k.qid);
            }
            if (col == 0) { //pid
                MyIndexListModel* m = static_cast<MyIndexListModel*>(GetModel());
                key3 k = m->GetRowValue(row);
                if (k.pid > 0) s_pid_HIGHLIGH = k.pid;
                //  else
                //      s_pid_HIGHLIGH = -1;
            }
            });

        Bind(wxEVT_DATAVIEW_COLUMN_REORDERED, [&](wxDataViewEvent& event) {
            wxDataViewColumn* const col = event.GetDataViewColumn();
            if (!col)
            {
                return;
            }

            ispidfirst = col->GetTitle() == "PID" && event.GetColumn() == 0;
            if (!ispidfirst) ispidfirst = col->GetTitle() == "QID" && event.GetColumn() == 1;
            CalcRowsDataView(true, ispidfirst);
            Refresh();
            if (m_win_s != NULL)  m_win_s->Refresh();
            });
        Bind(wxEVT_CHAR, [&](wxKeyEvent& event) {
            bool fnd = false;
            wxChar charcode = event.GetUnicodeKey();
            int l = m_find.length();
            if (event.GetKeyCode() == WXK_ESCAPE) {
                //GetParent()->Close(true);
                m_find = "";
                if (m_win_s != NULL) {
                    m_win_s->Destroy();
                    m_win_s = NULL;
                }

            }
            else
                if (event.GetKeyCode() == WXK_F3) {
                    //GetParent()->Close(true);
                    fnd = true;

                }
                else
                    if (event.GetKeyCode() == WXK_BACK) {

                        if (l > 0) m_find.RemoveLast();
                        //m_win_s->SetValue(m_find);
                        fnd = true;
                    }
                    else
                        if (wxIsprint(charcode))
                        {
                            //txt->EmulateKeyPress(event);
                            m_find += charcode;

                            fnd = true;
                        }
            //else
            //    event.Skip();
            if (fnd) {
                if (m_win_s == NULL && m_find.length() > 0) {
                    wxRect r;
                    r.width = GetColumn(0)->GetWidth();
#ifdef __WXGTK__
                    r.height = GTKGetUniformRowHeight();
#endif
                    wxWindow* t = this;
#ifdef WIN32
                    //wxWindow* t = this->GenericGetHeader();
                    r.height = this->GenericGetHeader()->GetSize().GetHeight();
#else
#endif
                    m_win_s = new wxTextCtrl(t, wxID_ANY, "",
                        r.GetPosition(),
                        r.GetSize(),
                        wxTE_PROCESS_ENTER
                        | wxTE_READONLY
                    );

                    m_win_s->SetInsertionPointEnd();
                }
                if (m_win_s != NULL) {
                    m_win_s->SetValue(m_find); m_win_s->Refresh();
                }
                if (m_find.length() > 0) {

                    MyIndexListModel* m = static_cast<MyIndexListModel*>(GetModel());
                    wxDataViewItem item(GetCurrentItem());
                    long row = (long)item.GetID();
                    if (!(row < m->GetCount())) row = 0;

                    while (row != -1 && row < m->GetCount()) {
                        key3 k = m->GetRowValue(row);

                        wxString v = wxString::Format("%d,%llx", k.pid, k.qid);
                        // m->GetValueByRow(v, row, 0);
                        bool isfind = v.Contains(m_find);
                        if (isfind) {
                            wxDataViewItem it = wxDataViewItem(wxUIntToPtr(row + 1));
                            SetCurrentItem(it);
                            EnsureVisible(it);
                            return;
                        }
                        row++;
                    }
                }
                return;
            }
            // end function
            event.Skip();
            });
        GetMainWindow()->Bind(wxEVT_MOTION, [&](wxMouseEvent& event) {
            if (event.Dragging())
                return;

            wxClientDC dc(GetMainWindow());
            PrepareDC(dc);

            //wxPoint logPos(event.GetLogicalPosition(dc));
            //wxPoint mc= GetMainWindow()->ScreenToClient(event.GetPosition());
            wxPoint mc = event.GetPosition();
            wxString position;
            //position = wxString::Format("x=%d y=%d", logPos.x, logPos.y);
            //wxLogMessage("Mouse pos %s", position);
            int dy = 0;
            wxSize sz;
#ifdef WIN32
            wxHeaderCtrl* const header = GenericGetHeader();
            if (header) {
                sz = header->GetSize();
                //header->Refresh();
                dy = sz.GetHeight();
            }
            mc.y += dy;
#else
            mc.y += 18; // only linux compile
#endif
            wxDataViewItem item;
            wxDataViewColumn* column;

            HitTest(mc, item, column);
            if (item != NULL && column != NULL)
            {
                int row = (long)item.GetID() - 1;
                int ncol = column->GetModelColumn();
                if (row >= 0) {
                    if (column && column->GetModelColumn() == 2) {
                        wxVariant vr;
                        vr = "gethint";
                        if ((lastcol != ncol) || (lastrow != row)) {
                            MyIndexListModel* m = static_cast<MyIndexListModel*>(GetModel());
                            m->GetValueByRow(vr, row, ncol);
                            GetMainWindow()->SetToolTip(vr.GetString());
                            lastrow = row;
                            lastcol = ncol;
                        }

                    }
                    else if (column && column->GetModelColumn() == 0) {
                        // PID
                        MyIndexListModel* m = static_cast<MyIndexListModel*>(GetModel());
                        key3 k = m->GetRowValue(row);
                        if (k.sum > 0) {
                            if (((lastcol != ncol) || (lastrow != row))) {
                                wxString s = w->GetBackendTypeName(k.sum);
                                GetMainWindow()->SetToolTip(s);
                                lastrow = row;
                                lastcol = ncol;

                            }
                        }
                        else {
                            GetMainWindow()->UnsetToolTip(); lastcol = -1;
                        }
                    }
                    else
                    {
                        GetMainWindow()->UnsetToolTip();
                        lastcol = -1;
                    }
                }
            }
            else
            {
                GetMainWindow()->UnsetToolTip();
                lastcol = -1;
            }

            });

    }
    WaitSample* GetSample() { return w; }
    void BuildColumn(int mode);
    bool CalcRowsDataView(bool force, bool IsPidFirst);
    //void AddRow(wxString csvtext);
    //void OnMouseMove(wxMouseEvent& event);
    //void OnMouseDown(wxMouseEvent& event);
    //void OnKEY_DOWN(wxKeyEvent& event);
    //void OnKEY_UP(wxKeyEvent& event);
    //void OnEVT_DATAVIEW_COLUMN_HEADER_CLICK(wxDataViewEvent& event);
    //void OnEVT_DATAVIEW_CONTEXT_MENU(wxCommandEvent& event);
    //void OnEVT_DATAVIEW_SELECTION_CHANGED(wxDataViewEvent& event);
    //void OnContextMenu(wxDataViewEvent& event);

    DECLARE_EVENT_TABLE()


private:
    int lastrow = -1, lastcol = 1;
    int agg, right_g, left_g;
    bool ignoreBG = false;
    bool ispidfirst = true;
    wxString m_find;
    wxTextCtrl* m_win_s;
    wxTopActivity* top;
    WaitSample* w;
};

#undef sepListWait

