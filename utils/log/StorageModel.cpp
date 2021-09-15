#include "pgAdmin3.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/dataview.h"
#include "log/Storage.h"
#include "log/StorageModel.h"

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

#include "log/null.xpm"
#include "log/log_xpm.xpm"
#include "log/war_xpm.xpm"
#include "log/errorl_xpm.xpm"
#include "log/fatal_xpm.xpm"
#include "log/panic_xpm.xpm"
#include "log/user_xpm.xpm"
#include "log/wx_small.xpm"

// ----------------------------------------------------------------------------
// MyCustomRendererText
// ----------------------------------------------------------------------------

class MyCustomRendererText : public wxDataViewCustomRenderer
{
public:
    StorageModel::cols col;
    Storage* st;
    int row;
    int countRows = 0;
    // This renderer can be either activatable or editable, for demonstration
    // purposes. In real programs, you should select whether the user should be
    // able to activate or edit the cell and it doesn't make sense to switch
    // between the two -- but this is just an example, so it doesn't stop us.
    explicit MyCustomRendererText(wxDataViewCellMode mode, StorageModel::cols column)
        : wxDataViewCustomRenderer("string", mode, wxALIGN_LEFT)
    {
        EnableEllipsize(wxELLIPSIZE_END);
        col = column;
        st = NULL;
    }

    virtual bool Render(wxRect rect, wxDC* dc, int state) wxOVERRIDE
    {
        //dc->SetBrush( *wxLIGHT_GREY_BRUSH );
        dc->SetBrush(*wxYELLOW_BRUSH);
        dc->SetPen(*wxTRANSPARENT_PEN);
        rect.Deflate(2);
        wxRect orig = rect;

        //dc->DrawRoundedRectangle( rect, 3 );

        wxString s = m_value, t;
        wxString rest;
        wxString* pointer = &rest;
        int x = 0, y = 0;
        //dc->GetMultiLineTextExtent(m_value);
        wxArrayInt arr;
        wxSize h = dc->GetTextExtent("H");
        dc->GetPartialTextExtents(m_value, arr);
        bool ex = false;
        int i = 0;
        int startX = 0;

        wxRect rectCol;

        while (i < s.Len()) {

            t = "";
            x = 0;
            bool inquote = false;
            rectCol.x = rect.x;
            rectCol.y = rect.y;
            rectCol.height = h.GetHeight();
            rectCol.width = 0;
            while (i < s.Len())
            {
                if (s[i] == '\n') {
                    startX = arr[i];
                    i++;
                    break;
                }
                if (s[i] == '"') {
                    if (inquote) {
                        // закрытие кавычек
                        rectCol.width = arr[i] - rectCol.x;
                        rectCol.x = rectCol.x - startX + rect.x;
                        dc->DrawRoundedRectangle(rectCol, 3);
                        rectCol.x = arr[i];
                        rectCol.width = 0;
                    }
                    else
                        rectCol.x = arr[i];

                    inquote = !inquote;
                }
                t += s[i];
                i++;
            }
            RenderText(t,
                x, // no offset
                //wxRect(dc->GetTextExtent(m_value)).CentreIn(rect),
                rect,
                dc,
                state);
            rect.y = rect.y + h.GetHeight() + 0;
            rect.height = rect.height - (h.GetHeight() + 0);

        }

        if (countRows > 0 && col == StorageModel::cols::Col_Host) {
            dc->SetBrush(*wxGREEN_BRUSH);
            wxString str = wxString::Format("%d", countRows);
            wxSize sz = dc->GetTextExtent(str);
            sz.SetWidth(sz.GetX() + 3);
            orig.SetLeft(orig.GetLeft() + (orig.GetWidth() - sz.GetWidth()));
            orig.SetHeight(sz.GetHeight());
            orig.SetWidth(sz.GetWidth());
            dc->DrawRoundedRectangle(orig, 2);
            RenderText(str,
                1, // no offset
                //wxRect(dc->GetTextExtent(m_value)).CentreIn(rect),
                orig,
                dc,
                state);


        }

        return true;

        RenderText(m_value,
            0, // no offset
            //wxRect(dc->GetTextExtent(m_value)).CentreIn(rect),
            rect,
            dc,
            state);
        return true;
    }

    virtual bool ActivateCell(const wxRect& WXUNUSED(cell),
        wxDataViewModel* WXUNUSED(model),
        const wxDataViewItem& WXUNUSED(item),
        unsigned int WXUNUSED(col),
        const wxMouseEvent* mouseEvent) wxOVERRIDE
    {
        wxString position;
        if (mouseEvent)
            position = wxString::Format("via mouse at %d, %d", mouseEvent->m_x, mouseEvent->m_y);
        else
            position = "from keyboard";
        wxLogMessage("MyCustomRendererText ActivateCell() %s", position);
        return false;
    }

    virtual wxSize GetSize() const wxOVERRIDE
    {
        wxSize txtSize = GetTextExtent(m_value);
        //wxSize txtSize = GetDC()->GetMultiLineTextExtent(GetMultiLineTextExtent);
        //wxSize txtSize = wxDataViewRenderer::GetDC()->GetMultiLineTextExtent(m_value);
        int lines = m_value.Freq('\n') + 1;
        if (lines > 1) {
            wxString position;
            position = wxString::Format("lines %d,hieght 1 row %d full h=%d", lines, txtSize.GetHeight(), txtSize.GetHeight() * lines + 1 * lines);
            // wxLogMessage("MyCustomRendererText GetSize() %s", position);
            txtSize.SetHeight(txtSize.GetHeight() * lines + 1 * lines);
        }
        else
            txtSize.SetHeight(-1);
        txtSize.SetWidth(-1);
        return txtSize;

        //return GetView()->FromDIP(wxSize(60, 20));
    }

    virtual bool SetValue(const wxVariant& value) wxOVERRIDE
    {
        m_value = value.GetString();
        if (!st) {
            StorageModel* m = dynamic_cast<StorageModel*>(GetView()->GetModel());
            st = m->getStorage();
        }
        if (st->IsGroupFilter()) {
            row = st->getLastRowIndex();
            countRows = st->getCountGroup(row);
        }
        else countRows = -1;

        return true;
    }

    virtual bool GetValue(wxVariant& WXUNUSED(value)) const wxOVERRIDE { return true; }

#if wxUSE_ACCESSIBILITY
    virtual wxString GetAccessibleDescription() const wxOVERRIDE
    {
        return m_value;
    }
#endif // wxUSE_ACCESSIBILITY

    virtual bool HasEditorCtrl() const wxOVERRIDE { return true; }

    virtual wxWindow*
        CreateEditorCtrl(wxWindow* parent,
            wxRect labelRect,
            const wxVariant& value) wxOVERRIDE
    {
        wxTextCtrl* text = new wxTextCtrl(parent, wxID_ANY, value,
            labelRect.GetPosition(),
            labelRect.GetSize(),
            wxTE_PROCESS_ENTER);
        text->SetInsertionPointEnd();

        return text;
    }

    virtual bool
        GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value) wxOVERRIDE
    {
        wxTextCtrl* text = wxDynamicCast(ctrl, wxTextCtrl);
        if (!text)
            return false;
        wxString sel = text->GetStringSelection();
        if (sel.IsEmpty()) return false;
        value = sel;

        return true;
    }

private:
    wxString m_value;
};

// ----------------------------------------------------------------------------
// StorageModel
// ----------------------------------------------------------------------------

static int my_sort_reverse(int* v1, int* v2)
{
    return *v2 - *v1;
}

static int my_sort(int* v1, int* v2)
{
    return *v1 - *v2;
}

#define INITIAL_NUMBER_OF_ITEMS 0

void StorageModel::BuildColumns(MyDataViewCtrl* ctrl) {
    wxDataViewColumn* const colIconText = new wxDataViewColumn
    (
        "Severity",
        new wxDataViewIconTextRenderer(),
        StorageModel::Col_ToggleIconText,
        wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER_VERTICAL
    );
    colmap[StorageModel::Col_ToggleIconText] = MyConst::colField::logSqlstate;
    ctrl->AppendColumn(colIconText);

    ctrl->AppendColumn(
        new wxDataViewColumn("logTime",
            new MyCustomRendererText(wxDATAVIEW_CELL_EDITABLE, StorageModel::Col_LogTime),
            StorageModel::Col_LogTime,
            wxCOL_WIDTH_AUTOSIZE,
            wxALIGN_LEFT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE
        ));
    colmap[StorageModel::Col_LogTime] = MyConst::colField::logtime;

    ctrl->AppendColumn(
        new wxDataViewColumn("UserName",
            new MyCustomRendererText(wxDATAVIEW_CELL_EDITABLE, StorageModel::Col_User),
            StorageModel::Col_User,
            wxCOL_WIDTH_AUTOSIZE,
            wxALIGN_LEFT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE
        ));
    colmap[StorageModel::Col_User] = MyConst::colField::loguser;
    ctrl->AppendColumn(
        new wxDataViewColumn("DbName",
            new MyCustomRendererText(wxDATAVIEW_CELL_EDITABLE, StorageModel::Col_Db),
            StorageModel::Col_Db,
            30,
            wxALIGN_LEFT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE
        ));
    colmap[StorageModel::Col_Db] = MyConst::colField::logdb;
    ctrl->AppendColumn(
        new wxDataViewColumn("Pid",
            new MyCustomRendererText(wxDATAVIEW_CELL_EDITABLE, StorageModel::Col_PID),
            StorageModel::Col_PID,
            60,
            wxALIGN_LEFT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE
        ));
    colmap[StorageModel::Col_PID] = MyConst::colField::logpid;
    ctrl->AppendColumn(
        new wxDataViewColumn("Host",
            new MyCustomRendererText(wxDATAVIEW_CELL_EDITABLE, StorageModel::Col_Host),
            StorageModel::Col_Host,
            90,
            wxALIGN_LEFT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE
        ));
    colmap[StorageModel::Col_Host] = MyConst::colField::loghost;
    ctrl->AppendColumn(
        new wxDataViewColumn("AppName",
            new MyCustomRendererText(wxDATAVIEW_CELL_EDITABLE, StorageModel::Col_App),
            StorageModel::Col_App,
            100,
            wxALIGN_LEFT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE
        ));
    colmap[StorageModel::Col_App] = MyConst::colField::logappname;
    ctrl->AppendColumn(
        new wxDataViewColumn("Hint",
            new MyCustomRendererText(wxDATAVIEW_CELL_EDITABLE, StorageModel::Col_Hint),
            StorageModel::Col_Hint,
            90,
            wxALIGN_LEFT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE
        ));
    colmap[StorageModel::Col_Hint] = MyConst::colField::logHint;
    ctrl->AppendColumn(
        new wxDataViewColumn("Detail",
            new MyCustomRendererText(wxDATAVIEW_CELL_EDITABLE, StorageModel::Col_Detail),
            StorageModel::Col_Detail,
            60,
            wxALIGN_LEFT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE
        ));
    colmap[StorageModel::Col_Detail] = MyConst::colField::logDetail;

    ctrl->AppendColumn(
        new wxDataViewColumn("Message",
            new MyCustomRendererText(wxDATAVIEW_CELL_EDITABLE, StorageModel::Col_Message),
            StorageModel::Col_Message,
            wxCOL_WIDTH_AUTOSIZE,
            wxALIGN_LEFT,
            wxDATAVIEW_COL_REORDERABLE | wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE
        ));
    colmap[StorageModel::Col_Message] = MyConst::colField::logMessage;


}
#include <wx/textfile.h>
#include "log/filter_xpm.xpm"
StorageModel::StorageModel(MyDataViewCtrl* view) :
    wxDataViewVirtualListModel(INITIAL_NUMBER_OF_ITEMS)
{
    m_view = view;
    //m_icon[0] = wxIcon(null_xpm);
    m_icon[MyConst::iconIndex::log] = wxIcon(log_xpm);
    m_icon[MyConst::iconIndex::war] = wxIcon(war_xpm);
    m_icon[MyConst::iconIndex::user] = wxIcon(user_xpm);
    m_icon[MyConst::iconIndex::error] = wxIcon(errorl_xpm);
    m_icon[MyConst::iconIndex::fatal] = wxIcon(fatal_xpm);
    m_icon[MyConst::iconIndex::panic] = wxIcon(panic_xpm);
    bitmapflt = wxBitmap((wxIcon(filter_xpm)));
    store = new Storage();
}
bool StorageModel::setFilter(int col, wxString val, int flags, MyDataViewCtrl* view) {

    bool r = store->SetFilter(colmap[col], val, flags);
    Reset(store->getCountFilter());
    if (col != -1) {
        wxDataViewColumn* vc = view->GetColumn(col);
        vc->SetBitmap(bitmapflt);
    }
    return r;
}
int StorageModel::testFilter(int col, int position = 0) {
    int idx = 0;
    idx = store->testFilter(colmap[col], position);
    return idx;
}
void StorageModel::ApplyFilter() {
    store->ApplyFilter();
    Reset(store->getCountFilter());
}

void StorageModel::DropColFilter(int index) {
    if (index >= 0) store->DropColFilter(index);
}
bool StorageModel::Prepend(const wxString& text)
{
    //m_toggleColValues.insert(m_toggleColValues.begin(), 0);
    //m_textColValues.Insert(text, 0);
    bool add=store->AddLineTextCSV(text);
    if (!add) return false;
    int row = store->getCountStore() - 1;
    wxString val = store->GetFieldStorage(row, MyConst::colField::loguser, false);
    IncCountFreq(StorageModel::Col_User, val);
    IncCountFreq(StorageModel::Col_Db, store->GetFieldStorage(row, MyConst::colField::logdb, false));
    IncCountFreq(StorageModel::Col_Host, store->GetFieldStorage(row, MyConst::colField::loghost, false));
    val = store->GetFieldStorage(row, MyConst::colField::logappname, false);
    //if (val.IsEmpty()) val = store->GetFieldStorage((int)row, MyConst::colField::logbtype,false);
    IncCountFreq(StorageModel::Col_App, val);
    IncCountFreq(StorageModel::Col_PID, store->GetFieldStorage(row, MyConst::colField::logpid, false));
    IncCountFreq(StorageModel::Col_Hint, store->GetFieldStorage(row, MyConst::colField::logHint, false));
    val = store->GetFieldStorage(row, MyConst::colField::logSqlstate, false);
    IncCountFreq(StorageModel::Col_ToggleIconText, val);


    if (store->ApplyFilter(row)) {
        RowAppended();
        //RowPrepended();
        return true;
    }
    if (store->IsGroupFilter()) {

    }
    return false;

}
void StorageModel::DeleteItem(const wxDataViewItem& item)
{
    unsigned int row = GetRow(item);

    RowDeleted(row);
}

void StorageModel::DeleteItems(const wxDataViewItemArray& items)
{
    unsigned i;
    wxArrayInt rows;
}

void StorageModel::AddMany()
{
    Reset(GetCount() + 1000);
}

void StorageModel::GetValueByRow(wxVariant& variant,
    unsigned int row, unsigned int col) const
{
    MyConst::colField fldcsv = colmap[col];
    wxString val;
    val = store->GetField((int)row, fldcsv);
    if (col == StorageModel::Col_ToggleIconText) {
        int i = store->GetSeverityIndex((int)row);
        variant << wxDataViewIconText(val, m_icon[i]);
        return;
    }
    else if (col == StorageModel::Col_App) {
        //if (val.IsEmpty()) val = store->GetField((int)row, MyConst::colField::logbtype);
    }
    else if (col == StorageModel::Col_User) {

    };

    variant = val;
}

bool StorageModel::GetAttrByRow(unsigned int row, unsigned int col,
    wxDataViewItemAttr& attr) const
{

    //attr.SetBackgroundColour(*wxLIGHT_GREY);
    wxColor c = store->GetBgColorLine((int)row);
    if (c.IsOk()) {
        attr.SetBackgroundColour(c);
        return true;
    }
    return false;
}

bool StorageModel::SetValueByRow(const wxVariant& variant,
    unsigned int row, unsigned int col)
{
    wxString flt = variant.GetString();
    int flags = FL_CONTAINS;
    if (flt[0] == '!') {
        flags = flags | FL_REVERSE;
        flt = flt.substr(1);
    }
    setFilter(col, flt, flags, m_view);

    return false;
}
