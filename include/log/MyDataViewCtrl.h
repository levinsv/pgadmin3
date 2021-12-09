#pragma once
#include "wx\dataview.h"
#include "wx/wx.h"
#include "Storage.h"
class MyDataViewCtrl :
    public wxDataViewCtrl
{
public:
    MyDataViewCtrl(wxWindow* parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = 0,
        const wxValidator& validator = wxDefaultValidator,
        const wxString& name = wxASCII_STR(wxDataViewCtrlNameStr)
    ) : wxDataViewCtrl(parent, id, pos, size, style, validator, name)
    {


    }
    void setStatusObj(wxStaticText* o) {
        st = o;
    }
    int GetLastMouseRow() { return lastrow; };
    int GetLastMouseCol() { return lastcol; };
    wxString getSettingString();
    void setSettingString(wxString setstr);
    void setGroupMode(bool mode);
    void ViewGroup(bool view);
    void ClearAllFilter(bool no_apply);
    
    void ModUserFilter(wxString FilterName, wxString wxOper, wxComboBox* combo, wxTextCtrl* textctrl);
    void AddFilterIgnore(wxString Fname);
    void AddRow(wxString csvtext);
    void OnMouseMove(wxMouseEvent& event);
    void OnKEY_DOWN(wxKeyEvent& event);
    void OnKEY_UP(wxKeyEvent& event);
#ifdef MYTEST
    void OnTimer(wxTimerEvent& event);
#endif

    void OnEVT_DATAVIEW_COLUMN_HEADER_CLICK(wxDataViewEvent& event);
    void OnEVT_DATAVIEW_CONTEXT_MENU(wxCommandEvent& event);
    void OnEVT_DATAVIEW_SELECTION_CHANGED(wxDataViewEvent& event);
    void OnContextMenu(wxDataViewEvent& event);
    wxComboBox* smart;
    DECLARE_EVENT_TABLE()

private:
    int lastrow = -1, lastcol = 1;
    wxDataViewItem selectRowGroup;
    bool modctrl = false;
    wxStaticText* st;
    
    //bool visibleEndLine = false;
#ifdef MYTEST
    int linenumber = -1;
    wxArrayString logadd;
#endif
};
