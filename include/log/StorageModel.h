#pragma once
#include "pgAdmin3.h"
#include "wx/hashmap.h"
#include "wx/vector.h"
#include "Storage.h"
#include "MyDataViewCtrl.h"


WX_DECLARE_STRING_HASH_MAP(int, MyHashCount);

class StorageModel : public wxDataViewVirtualListModel
{
public:
    enum cols
    {
        Col_ToggleIconText,
        Col_LogTime,
        Col_User,
        Col_Db,
        Col_PID,
        Col_Host,
        Col_App,
        Col_Hint,
        Col_Detail,
        Col_Message,
        Col_Server,
        Col_Max
    };

    StorageModel(MyDataViewCtrl* view);

    // helper methods to change the model

    bool Prepend(const wxString& text);
    void DeleteItem(const wxDataViewItem& item);
    void DeleteItems(const wxDataViewItemArray& items);
    void AddMany();
    bool setFilter(int col, wxString val, int flags, MyDataViewCtrl* view);
    void DropColFilter(int index);
    int testFilter(int col, int position);
    void ApplyFilter();
    bool getGroupFilter()
    {
        return store->IsGroupFilter();
    }
    void setGroupFilter(bool val)
    {
        store->setGroupFilter(val);
        ApplyFilter();
    }
    void BuildColumns(MyDataViewCtrl* ctrl);
    void IncCountFreq(int col, wxString &val) {
        MyHashCount::const_iterator it = freqValues[col].find(val);
        int cnt=0;
        if (it != freqValues[col].end()) 
            cnt = it->second;
        cnt++;
        freqValues[col][val] = cnt;
    }

    // implementation of base class virtuals to define model

    virtual unsigned int GetColumnCount() const wxOVERRIDE
    {
        return Col_Max;
    }

    unsigned int GetRowCount() const
    {
        return store->getCountFilter();
    }

    virtual wxString GetColumnType(unsigned int col) const wxOVERRIDE
    {
        if (col == Col_ToggleIconText)
            return wxDataViewCheckIconTextRenderer::GetDefaultType();

        return "string";
    }
    Storage* getStorage() {
        return store;
    }
    virtual void GetValueByRow(wxVariant& variant,
        unsigned int row, unsigned int col) const wxOVERRIDE;
    virtual bool GetAttrByRow(unsigned int row, unsigned int col,
        wxDataViewItemAttr& attr) const wxOVERRIDE;
    virtual bool SetValueByRow(const wxVariant& variant,
        unsigned int row, unsigned int col) wxOVERRIDE;
    MyHashCount freqValues[Col_Max];
    wxBitmap bitmapflt;
    unsigned int  lastrow;
    MyDataViewCtrl* m_view;
private:
    ///IntToStringMap   m_customColValues;
    wxIcon           m_icon[MyConst::iconIndex::MAX_COL];
    
    Storage* store;
    MyConst::colField colmap[Col_Max];
    

};

