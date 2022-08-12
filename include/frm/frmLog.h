//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// frmStatus.h - Status Screen
//
//////////////////////////////////////////////////////////////////////////

#ifndef __FRMLOG_H
#define __FRMLOG_H

// wxWindows headers
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/notebook.h>
#include <wx/dynarray.h>


// wxAUI
#include <wx/aui/aui.h>

#include "dlg/dlgClasses.h"
#include "utils/factory.h"
#include "ctl/ctlAuiNotebook.h"
#include "log/StorageModel.h"
#include "log/MyDataViewCtrl.h"

class RemoteConn2
{
public:
    RemoteConn2(pgConn* c)
    {
        conn = c;
    }
    ~RemoteConn2()
    {
        if (conn) delete conn;
    }

    pgConn* conn;
    wxDateTime nextrun;
};

WX_DECLARE_OBJARRAY(RemoteConn2, RemoteConnArray2);

enum
{
    ID_SET_GROUP = 207,
    ID_CLEAR_ALL_FILTER = 208,
    ID_SET_DETAILGROUP = 209,
    ID_ADD_FILTER = 210,
    ID_ADD_UFilter=211,
    ID_DEL_UFilter = 212,
    ID_CBOX_UFilter = 213,
    ID_TEXT_UFilter = 214,
    ID_CBOX_SMART = 215,
    MNU_SEND_MAIL=216,
    MNU_FIND_TEXT=217,
    ID_HELP_LOG=218,
    ID_NEXT_MAX
};



//
// This number MUST be incremented if changing any of the default perspectives
//
#define FRMLOG_PERSPECTIVE_VER wxT("8274")

#ifdef __WXMAC__
#define FRMLOG_DEFAULT_PERSPECTIVE wxT("layout2|name=Activity;caption=Activity;state=6293500;dir=4;layer=0;row=0;pos=0;prop=100000;bestw=321;besth=244;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=462;floaty=165;floatw=595;floath=282|name=Locks;caption=Locks;state=6293500;dir=4;layer=0;row=0;pos=1;prop=100000;bestw=321;besth=244;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-231;floaty=235;floatw=595;floath=282|name=Transactions;caption=Transactions;state=6293500;dir=4;layer=0;row=0;pos=2;prop=100000;bestw=0;besth=0;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=461;floaty=527;floatw=595;floath=282|name=Logfile;caption=Logfile;state=6293500;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=0;besth=0;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-103;floaty=351;floatw=595;floath=282|name=toolBar;caption=Tool bar;state=2124528;dir=1;layer=10;row=0;pos=0;prop=100000;bestw=808;besth=33;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=888;floaty=829;floatw=558;floath=49|dock_size(4,0,0)=583|dock_size(5,0,0)=10|dock_size(1,10,0)=35|")
#else
#ifdef __WXGTK__
#define FRMLOG_DEFAULT_PERSPECTIVE wxT("layout2|name=Activity;caption=Activity;state=6293500;dir=4;layer=0;row=1;pos=0;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=174;floaty=216;floatw=578;floath=282|name=Locks;caption=Locks;state=6293500;dir=4;layer=0;row=1;pos=2;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=136;floaty=339;floatw=576;floath=283|name=Transactions;caption=Transactions;state=6293500;dir=4;layer=0;row=1;pos=3;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=133;floaty=645;floatw=577;floath=283|name=Querystate;caption=Query State;state=6309884;dir=4;layer=0;row=1;pos=1;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=154;floaty=255;floatw=1360;floath=751|name=Logfile;caption=Logfile;state=6293500;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=toolBar;caption=toolBar;state=2108144;dir=1;layer=10;row=0;pos=0;prop=100000;bestw=716;besth=23;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=586;floaty=525;floatw=483;floath=49|dock_size(1,10,0)=25|dock_size(4,0,1)=1115|dock_size(5,0,0)=22|")
#else
#define FRMLOG_DEFAULT_PERSPECTIVE wxT("layout2|name=Activity;caption=Activity;state=6293500;dir=4;layer=0;row=1;pos=0;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=174;floaty=216;floatw=578;floath=282|name=Locks;caption=Locks;state=6293500;dir=4;layer=0;row=1;pos=2;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=136;floaty=339;floatw=576;floath=283|name=Transactions;caption=Transactions;state=6293500;dir=4;layer=0;row=1;pos=3;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=133;floaty=645;floatw=577;floath=283|name=Querystate;caption=Query State;state=6309884;dir=4;layer=0;row=1;pos=1;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=154;floaty=255;floatw=1360;floath=751|name=Logfile;caption=Logfile;state=6293500;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=toolBar;caption=toolBar;state=2108144;dir=1;layer=10;row=0;pos=0;prop=100000;bestw=716;besth=23;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=586;floaty=525;floatw=483;floath=49|dock_size(1,10,0)=25|dock_size(4,0,1)=1115|dock_size(5,0,0)=22|")
#endif
#endif




class frmLog : public pgFrame
{
public:
	frmLog(frmMain *form, const wxString &_title, pgServer *srv);
	~frmLog();
	void Go();
    void getFilename();
    void readLogFile(wxString  logfileName, long& lenfile, long& logfileLength, wxString& savedPartialLine, pgConn* conn);
    void AddNewConn(pgConn* con);
    pgServer* getServer(wxString& strserver);
    pgConn* createConn(pgServer* srv);
    bool CheckConn(wxString host, int port);
private:
	static const int timerInterval = 5000; // 1000 ms
    	wxTimer   m_timer;
	wxAuiManager manager;

	frmMain *mainForm;
	pgConn *connection;
    RemoteConnArray2 conArray;
    MyDataViewCtrl* my_view;
	wxNotebook* m_notebook;
    wxStaticText* status;
    wxCheckBox *group, *detail;
    wxCheckListBox* lb;
    wxComboBox* listUserFilter;
    wxComboBox* smart;
    wxTextCtrl* contentFilter;
    wxObjectDataPtr<StorageModel> m_storage_model;
    wxArrayString logfileName;
    wxArrayString savedPartialLine;
    wxArrayLong logfileLength;
    wxArrayLong len;
    void OnSetGroup(wxCommandEvent& event);
    void OnSetDetailGroup(wxCommandEvent& event);
    void OnClearAllFilter(wxCommandEvent& event);
    void OnAddFilterIgnore(wxCommandEvent& event);
    void OnAddUFilter(wxCommandEvent& event);
    void OnDelUFilter(wxCommandEvent& event);
    void OnChangeUFilter(wxCommandEvent& event);
    void OnChangeSmart(wxCommandEvent& event);
    void OnSendMail(wxCommandEvent& event);
    void OnFind(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);

    void OnSetFocus(wxFocusEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnActivate(wxActivateEvent& event);
    void OnTimer(wxTimerEvent& event);
    void seticon(bool errflag);

    wxIcon idef;
    wxIcon idefRed;

	DECLARE_EVENT_TABLE()
};

class LogFactory : public contextActionFactory
{
public:
    LogFactory(menuFactoryList* list, wxMenu* mnu, ctlMenuToolbar* toolbar);
    wxWindow* StartDialog(frmMain* form, pgObject* obj);
    bool CheckEnable(pgObject* obj);
};


#endif
