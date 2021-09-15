#include "pgAdmin3.h"

#include "log/MyDataViewCtrl.h"
#include "log/StorageModel.h"
#include "wx/headerctrl.h"
#include "wx/itemattr.h"
#include "wx/dataview.h"
//#include <map>
#include <set>


BEGIN_EVENT_TABLE(MyDataViewCtrl, wxDataViewCtrl)
EVT_MOTION(MyDataViewCtrl::OnMouseMove)

END_EVENT_TABLE()

#ifdef MYTEST
#include <wx/textfile.h>
void MyDataViewCtrl::OnTimer(wxTimerEvent& event) {
    //  wxLogMessage("OnTimer called.");
    if (linenumber == -1) {
        wxTextFile      tfile;
        wxString str;
        tfile.Open("logAdd.csv");

        // read the first line
        str = tfile.GetFirstLine() + "\n";

        // read all lines one by one
        // until the end of the file
        while (!tfile.Eof())
        {
            str += tfile.GetNextLine() + "\n";
        }
        tfile.Close();
        CSVLineTokenizer tk(str);
        wxString line;
        while (tk.HasMoreLines())
        {
            line.Clear();

            bool partial;
            line = tk.GetNextLine(partial);
            if (partial)
            {
                break;
            }
            //Prepend(line);
            logadd.Add(line);

        }
        linenumber = 0;
    }
    if (logadd.GetCount() > linenumber) {
        StorageModel* m = dynamic_cast<StorageModel*>(GetModel());
        //m->Prepend(logadd[linenumber++]);
        AddRow(logadd[linenumber++]);
        wxString l = wxString::Format("rows %d", m->GetCount());
        st->SetLabelText(l);
        if (m->getGroupFilter()) Refresh();
    }
}
#endif
void MyDataViewCtrl::setSettingString(wxString setstr) {
    wxString s;
    wxStringTokenizer tokenizer(setstr, ";");

    for (int i = 0; i < GetColumnCount(); i++) {
        if (!tokenizer.HasMoreTokens()) break;
        wxString token = tokenizer.GetNextToken();
        wxDataViewColumn* vc = GetColumn(i);
        int w;
        w = wxAtoi(token);
        vc->SetWidth(w);
    }
    return;
}

wxString MyDataViewCtrl::getSettingString() {
    wxString s;
    for (int i = 0; i < GetColumnCount(); i++) {
        wxDataViewColumn* vc = GetColumn(i);
        int width = vc->GetWidth();
        if (!s.IsEmpty()) s = s + ";";
        s = s + wxString::Format("%d", width);
    }
    //s = s + wxString::Format(";%d", modeGroup);
    return s;
}
void MyDataViewCtrl::setGroupMode(bool mode) {
    StorageModel* m = dynamic_cast<StorageModel*>(GetModel());
    m->setGroupFilter(mode);
    wxItemAttr attr;
    if (m->getGroupFilter())
    {
        //attr.SetTextColour(*wxRED);
        //attr.GetFont().Underlined();
        attr.SetFont(attr.GetFont().Underlined());
        //attr.GetFont().
        //wxFontStyle s=attr.GetFont().GetStyle();
        //attr.SetFont(attr.GetFont().Strikethrough());
    }
    //else: leave it as default to disable custom header attributes
    wxString l = wxString::Format("rows %d", m->GetRowCount());
    st->SetLabelText(l);

    if (!SetHeaderAttr(attr))
        wxLogMessage("Sorry, header attributes not supported on this platform");

}
void MyDataViewCtrl::ViewGroup(bool view) {
    StorageModel* m = dynamic_cast<StorageModel*>(GetModel());
    Storage* sta = m->getStorage();
    if (view) {
        selectRowGroup = GetSelection();
        if (!selectRowGroup.IsOk()) return;
        int rowselectGroup = m->GetRow(selectRowGroup);
        sta->setDetailGroupRow(rowselectGroup);

    }
    else sta->setDetailGroupRow(-1);
    m->ApplyFilter();
    if (!view) {
        SetCurrentItem(selectRowGroup);
        EnsureVisible(selectRowGroup);
    }
    wxString l = wxString::Format("rows %d", m->GetRowCount());
    st->SetLabelText(l);


}
void MyDataViewCtrl::AddRow(wxString csvtext) {
    StorageModel* m = dynamic_cast<StorageModel*>(GetModel());
    wxDataViewItem select;
    if (HasSelection()) select = GetSelection();
    

    if (m->Prepend(csvtext)) {
        if (!select.IsOk()) return;
        int rowselect = m->GetRow(select);
        // add visible row
        int rowlast = m->GetRowCount() - 1;
        if ((rowlast - 1) == rowselect) {
            //m->GetItem();
            select = m->GetItem(rowlast);
            SetCurrentItem(select);
            EnsureVisible(select);
            ScrollPages(1);
        }

    }


}
void MyDataViewCtrl::OnEVT_DATAVIEW_SELECTION_CHANGED(wxDataViewEvent& event) {
    StorageModel* m = dynamic_cast<StorageModel*>(GetModel());
    int r = m->GetRow(event.GetItem());
    if (m->getGroupFilter() && r != -1) {
        Storage* sta = m->getStorage();
        sta->ClearCount(r);
        int a=sta->GetTotalCountGroup(r);

        wxString l = wxString::Format("Total rows in group %d", a);
        st->SetLabelText(l);

        //EnsureVisible(event.GetItem());
    }

    //wxLogMessage("wxEVT_DATAVIEW_SELECTION_CHANGED, First selected Item row: %d", r);

}
void MyDataViewCtrl::ClearAllFilter() {
    int colt = 0;
    int col = 0;
    bool all = false;
    bool clear = false;
    wxArrayInt fCol;
    StorageModel* m = dynamic_cast<StorageModel*>(GetModel());
    for (int j = 0; j < GetColumnCount(); j++) {
        col = j;
        clear = false;
        colt = 0;
        while (colt != -1) {
            colt = m->testFilter(col, colt);
            if (colt >= 0) {
                m->DropColFilter(colt);
                clear = true;
                all = true;
                colt++;
            }
        }
        if (clear) {
            //            m->ApplyFilter();
            fCol.Add(col);
            //           wxDataViewColumn* vc = GetColumn(col);
           //            vc->SetBitmap(wxNullBitmap);
        }
    }
    if (all) {
        m->ApplyFilter();
        for (auto i : fCol) {
            wxDataViewColumn* vc = GetColumn(i);
            vc->SetBitmap(wxNullBitmap);
        }
    }
    wxString l = wxString::Format("rows %d", m->GetRowCount());
    st->SetLabelText(l);
}
void MyDataViewCtrl::AddFilterIgnore() {
    int colt = 0;
    int col = 0;
    bool all = false;
    bool clear = false;
    wxArrayInt fCol;
    StorageModel* m = dynamic_cast<StorageModel*>(GetModel());
    Storage* st = m->getStorage();
    wxString text;
    for (int j = 0; j < GetColumnCount(); j++) {
        col = j;
        clear = false;
        colt = 0;
        while (colt != -1) {
            colt = m->testFilter(col, colt);
            if (colt >= 0) {
                //m->DropColFilter(colt);
                wxString expr=st->GetStringFilterExpr(colt,true);
                st->addLineFilterStr(expr);
                text = text + expr + "\r\n";
                all = true;
                colt++;
            }
        }
    }

    if (all) {
        st->addLineFilterStr("");
        wxMessageBox("Create load filter\r\n\r\n"+text, _("Warning"), wxICON_INFORMATION | wxOK);

    }
    else {
        wxMessageBox("Filter empty.", _("Warning"), wxICON_INFORMATION | wxOK);
    }

}
void MyDataViewCtrl::OnEVT_DATAVIEW_CONTEXT_MENU(wxCommandEvent& event) {
    int id = event.GetId();
    wxMenu* mi = static_cast<wxMenu*>(event.GetEventObject());
    wxString label = mi->GetLabelText(id);
    StorageModel* m = dynamic_cast<StorageModel*>(GetModel());

    int col = (int)mi->GetClientData();
    if (id > 100) {
        int colt = 0;
        bool clear = false;
        //int pos = 0;
        bool all = (label == "Clear All");
        Storage* stor = m->getStorage();
        wxString expr;
        while (colt != -1) {
            colt = m->testFilter(col, colt);
            if (colt >= 0) {
                expr = stor->GetStringFilterExpr(colt);
                if (expr == label || all) {
                    m->DropColFilter(colt);
                    clear = true;
                }
                colt++;
            }
        }
        if (clear) {
            m->ApplyFilter();
            if (m->testFilter(col, 0) == -1) {
                wxDataViewColumn* vc = GetColumn(col);
                vc->SetBitmap(wxNullBitmap);
            }
        }
    }
    else {
        m->setFilter(col, label.AfterFirst(' '), 0, this);
        //wxDataViewColumn* vc=GetColumn(col);
        //vc->SetBitmap(m->bitmapflt);

    }
    wxString l = wxString::Format("rows %d", m->GetRowCount());
    st->SetLabelText(l);

    if (col == StorageModel::Col_User) {


        //m->setFilter(col, label.AfterFirst(' '));
       // wxMessageBox("You have selected Item 1", "Your selection", wxOK | wxICON_INFORMATION);
    }
    if (event.GetId() == 2) {
        // wxMessageBox("You have selected Item 2", "Your selection", wxOK | wxICON_INFORMATION);
    }

}
// правая кнопка на ячейке
void MyDataViewCtrl::OnContextMenu(wxDataViewEvent& event) {
    //wxString title = m_music_model->GetTitle(event.GetItem());
    //wxLogMessage("wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, Item: %s", title);
    StorageModel* m = dynamic_cast<StorageModel*>(GetModel());
    int row = m->GetRow(event.GetItem());
    int ncol;
    wxVariant vr;
    if (row >= 0)
    {

        //int ncol = GetColumn(event.GetColumn())->GetModelColumn();
        ncol = event.GetColumn();
        m->GetValueByRow(vr, row, ncol);
        if (ncol == 0) {
            wxDataViewIconText ic;
            ic << vr;
            wxString str = ic.GetText();
            vr = str;
        }
    }
    int pos = GetColumnPosition(event.GetDataViewColumn());

    wxString str = wxString::Format("col: %d row: %d val: %s", pos, row, vr.GetString());
    int flags = 0;
    if (modctrl) flags = FL_REVERSE;
    m->setFilter(ncol, vr.GetString(), flags, this);
    //wxDataViewColumn* vc = GetColumn(event.GetColumn());
    //vc->SetBitmap(m->bitmapflt);

    //wxMessageBox(str, "Your selection", wxOK | wxICON_INFORMATION);


}
auto cmp = [](std::pair<std::string, int> const& a, std::pair<std::string, int> const& b)
{
    return a.second != b.second ? a.second < b.second : a.first < b.first;
};
void MyDataViewCtrl::OnEVT_DATAVIEW_COLUMN_HEADER_CLICK(wxDataViewEvent& event) {
    StorageModel* m = dynamic_cast<StorageModel*>(GetModel());
    int col = event.GetColumn();
    if (event.GetColumn() == StorageModel::Col_User)
    {
    }
    const int rowChanged = m->GetRow(event.GetItem());
    wxMenu menu;
    //menu.SetTitle(event.GetDataViewColumn()->GetTitle());
    int i = 1;
    int colt = m->testFilter(col, 0);
    if (colt >= 0) {
        wxMenu* menus = new wxMenu();
        menus->SetClientData((void*)col);
        Storage* stor = m->getStorage();
        menus->Append(101 + i++, "Clear All");
        wxString expr;
        while (colt != -1) {
            expr = stor->GetStringFilterExpr(colt);
            menus->Append(101 + i++, expr);
            colt = m->testFilter(col, colt + 1);
        }
        menu.AppendSubMenu(menus, "Clear filters");
    }
    std::set<std::pair<int, std::string>> items;
    std::pair<int, std::string> v;
    int cc = 0;
    MyHashCount::iterator it;
    for (it = m->freqValues[col].begin(); it != m->freqValues[col].end(); ++it)
    {
        v = std::make_pair(it->second, it->first.ToStdString());
        items.emplace(v);
        //ma.emplace(it->first.ToStdString(), it->second);
        cc++;
    }
    //std::sort(items.begin(), items.end(),cmp);
    cc = cc - 20;
    int cc0 = 0;
    for (auto const& vk : items) {
        cc0++;
        if (cc0 < cc) continue;
        wxString key = wxString(vk.second);
        int value = vk.first;
        wxString str = wxString::Format("%d %s", value, key);
        menu.Append(i++, str);

    }

    //for (it = m->freqValues[col].begin(); it != m->freqValues[col].end(); ++it)
    //{
    //    wxString key = it->first;
    //    int value = it->second;
    //    wxString str = wxString::Format("%d %s",value,key);
    //    menu.Append(i++, str);
    //}

    menu.SetClientData((void*)col);
    this->PopupMenu(&menu);
    event.Skip(false);
    return;

    event.Skip(true);
}
void MyDataViewCtrl::OnKEY_DOWN(wxKeyEvent& event) {
    if ((event.GetModifiers() & wxMOD_CONTROL) == wxMOD_CONTROL) {
        modctrl = true;
    }
    else modctrl = false;
    event.Skip(true);

}
void MyDataViewCtrl::OnMouseMove(wxMouseEvent& event) {

    //event.Skip(true);
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
    wxHeaderCtrl* const header = GenericGetHeader();
    int dy = 0;
    wxSize sz;
    if (header) {
        sz = header->GetSize();
        //header->Refresh();
        dy = sz.GetHeight();
    }
    mc.y += dy;
    wxDataViewItem item;
    wxDataViewColumn* column;

    HitTest(mc, item, column);
    if (item != NULL && column != NULL)
    {
        StorageModel* m;
        m = dynamic_cast<StorageModel*>(GetModel());

        int row = m->GetRow(item);

        if (row >= 0)
        {
            wxVariant vr;
            int ncol = column->GetModelColumn();
            if ((lastcol != ncol) || (lastrow != row)) {
                m->GetValueByRow(vr, row, ncol);
                wxSize szext = dc.GetTextExtent(vr.GetString());
                int w = column->GetWidth();
                if (szext.GetWidth() > w)
                {
                    GetMainWindow()->SetToolTip(vr.GetString());
                }
                else
                    GetMainWindow()->UnsetToolTip();
                lastrow = row;
                lastcol = ncol;
            }

        }
        else
        {
            GetMainWindow()->UnsetToolTip();
        }
    }
    else
    {
        GetMainWindow()->UnsetToolTip();
    }
}
