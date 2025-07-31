#pragma once

#include <wx/listctrl.h>

class ctlNavigatePanel :
    public wxWindow
{
    struct find_s {
        wxString s;
        long i;
        long e;
        int num;
    };
    struct statistics_mark {
        long count = 0;
        bool enable = true;
        wxColour color;
    };

    enum {
        MNU_MARK = 250
    };
    enum find_pos{
        FOCUSNEXT,
        HOME,
        ALLFIND
    };
    
private:
    void OnPaint(wxPaintEvent& evt);
    void OnSize(wxSizeEvent& evt);
    void OnMouse(wxMouseEvent& evt);
    void mouseReleased(wxMouseEvent& evt);
    
    void render(wxDC& dc);
    void Init(bool reorganization);
    int  binary_search(long item, const std::vector<long>& arr);
    ctlListView* ctrl;
    long topvisible = -1;
    int width=20;
    int findwidth = 50;
    int border = 1;
    wxColour bgcolor,framecolor, bordercolor,findcolor,startdbcolor;
    wxJSONValue opt;
    std::vector<statistics_mark> mark_color;
    std::vector<long> items_mark; // rows mark
    std::vector<int> color_items_mark; // color rows mark
    std::vector<find_s> search_rule;
    std::vector<wxRegEx *> regExArray;
    // find 
    std::vector<long> items_find;
    wxString logFindString;
    int lastUseMark = -1;
    // startdb intervals
    std::vector<long> startdbintervals;
    long sinterval, einterval;
    

protected:
    virtual wxSize DoGetBestSize() const wxOVERRIDE;
public:
    ctlNavigatePanel(wxWindow* parent, ctlListView* lst);
    ~ctlNavigatePanel();
    void OnContextMenu(wxCommandEvent& event);
    void AddMarkItem(long item, int numcolor);
    int TryMarkItem(long row, const wxString &str);
    wxColour GetColorByIndex(int colorindex);
    int GetIndexColor(const wxColour &color) const;
    void ClearMark();
    void RowVisibleCenter(long row);
    void SetFindString(const wxString &findstr);
    int  GetCountMark();
    wxMenu* GetPopupMenu();
    bool RunKeyCommand(wxKeyEvent& event,int numCmd=-1);
    int  GetItemMark(long position);
    int FindText(wxString findtext, int position,bool directionUp);
    // Set/Reset color 
    int ReColorizeItems(int numIndictor, bool enableColor);
    DECLARE_EVENT_TABLE()
};

