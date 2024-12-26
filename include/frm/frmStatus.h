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

#ifndef __FRMSTATUS_H
#define __FRMSTATUS_H

// wxWindows headers
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/listctrl.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>

// wxAUI
#include <wx/aui/aui.h>

#include <wx/msgqueue.h>
#include "dlg/dlgClasses.h"
#include "utils/factory.h"
#include "ctl/ctlAuiNotebook.h"
#include "ctl/ctlNavigatePanel.h"
#include "utils/WaitSample.h"
#include "ctl/wxTopActivity.h"

enum
{
    CTL_RATECBO = 250,
    CTL_REFRESHBTN,
    CTL_CANCELBTN,
    CTL_TERMINATEBTN,
    CTL_COMMITBTN,
    CTL_ROLLBACKBTN,
    CTL_LOGCBO,
    CTL_ROTATEBTN,
    CTL_STATUSLIST,
    CTL_LOCKLIST,
    CTL_XACTLIST,
    CTL_LOGLIST,
    CTL_QUERYSTATELIST,
    MNU_STATUSPAGE,
    MNU_LOCKPAGE,
    MNU_XACTPAGE,
    MNU_LOGPAGE,
    MNU_QUERYSTATEPAGE,
    MNU_TERMINATE,
    MNU_COMMIT,
    MNU_ROLLBACK,
    MNU_COPY_QUERY,
    MNU_CLEAR_FILTER_SERVER_STATUS,
    MNU_COPY_QUERY_PLAN,
    MNU_HIGHLIGHTSTATUS,
    MNU_QUERYSTATEVERBOSE,
    MNU_QUERYSTATETIME,
    MNU_QUERYSTATEBUFFER,
    MNU_QUERYSTATETRIGGER,
    MNU_WAITENABLE,
    MNU_WAITSAVE,
    TIMER_REFRESHUI_ID,
    TIMER_STATUS_ID,
    TIMER_LOCKS_ID,
    TIMER_XACT_ID,
    TIMER_LOG_ID,
    TIMER_QUERYSTATE_ID
};


enum
{
    PANE_STATUS = 1,
    PANE_LOCKS,
    PANE_XACT,
    PANE_LOG,
    PANE_QUERYSTATE
};


//
// This number MUST be incremented if changing any of the default perspectives
//
#define FRMSTATUS_PERSPECTIVE_VER wxT("8275")

#ifdef __WXMAC__
#define FRMSTATUS_DEFAULT_PERSPECTIVE wxT("layout2|name=Activity;caption=Activity;state=6293500;dir=4;layer=0;row=0;pos=0;prop=100000;bestw=321;besth=244;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=462;floaty=165;floatw=595;floath=282|name=Locks;caption=Locks;state=6293500;dir=4;layer=0;row=0;pos=1;prop=100000;bestw=321;besth=244;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-231;floaty=235;floatw=595;floath=282|name=Transactions;caption=Transactions;state=6293500;dir=4;layer=0;row=0;pos=2;prop=100000;bestw=0;besth=0;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=461;floaty=527;floatw=595;floath=282|name=Logfile;caption=Logfile;state=6293500;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=0;besth=0;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-103;floaty=351;floatw=595;floath=282|name=toolBar;caption=Tool bar;state=2124528;dir=1;layer=10;row=0;pos=0;prop=100000;bestw=808;besth=33;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=888;floaty=829;floatw=558;floath=49|dock_size(4,0,0)=583|dock_size(5,0,0)=10|dock_size(1,10,0)=35|")
#else
#ifdef __WXGTK__
#define FRMSTATUS_DEFAULT_PERSPECTIVE wxT("layout2|name=Activity;caption=Activity;state=6309884;dir=4;layer=0;row=0;pos=0;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Locks;caption=Locks;state=6293500;dir=4;layer=0;row=0;pos=1;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Transactions;caption=Prepared Transactions;state=6293500;dir=4;layer=0;row=0;pos=2;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Querystate;caption=QueryState;state=6293502;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Logfile;caption=Logfile;state=6293500;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=toolBar;caption=Tool bar;state=2108144;dir=1;layer=10;row=0;pos=0;prop=100000;bestw=690;besth=39;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(4,0,0)=800|dock_size(5,0,0)=22|dock_size(1,10,0)=41|")
#else
#define FRMSTATUS_DEFAULT_PERSPECTIVE wxT("layout2|name=Activity;caption=Activity;state=6293500;dir=4;layer=0;row=1;pos=0;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=174;floaty=216;floatw=578;floath=282|name=Locks;caption=Locks;state=6293500;dir=4;layer=0;row=1;pos=2;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=136;floaty=339;floatw=576;floath=283|name=Transactions;caption=Transactions;state=6293500;dir=4;layer=0;row=1;pos=3;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=133;floaty=645;floatw=577;floath=283|name=Querystate;caption=Query State;state=6309884;dir=4;layer=0;row=1;pos=4;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=154;floaty=255;floatw=800;floath=751|name=Logfile;caption=Logfile;state=6293500;dir=4;layer=0;row=1;pos=5;prop=100000;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=150;floaty=90;floatw=400;floath=800|name=toolBar;caption=toolBar;state=2108144;dir=1;layer=10;row=0;pos=0;prop=100000;bestw=766;besth=39;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=586;floaty=525;floatw=483;floath=49|dock_size(1,10,0)=25|dock_size(4,0,1)=1115|dock_size(5,0,0)=22|")
#endif
#endif


static wxCriticalSection gs_critsect;


// Class declarations
//static wxSemaphore s_CloseLog(0, 1);

class ReadLogThread : public wxThread
{
public:
    ReadLogThread(pgConn *conlog, wxWindow* p, wxMessageQueue<wxString> *queue)
    {
        //wxSemaphore goReadLog(0,1);
        //wxSemaphore ResultOkLog(0, 1);
        //s_goReadLog=goReadLog;
        //s_ResultOkLog = ResultOkLog;

        log_queue = queue;
        theParent = p;
            logfileName = wxEmptyString;
            savedPartialLine=wxEmptyString;
            logfileLength=0;
            len=0;
            conn = conlog;
            //nextrun = wxDateTime(0);
    }
    ~ReadLogThread();
    void DoTerminate();
    bool IsTerminate() { return m_exit; }
    void SetParameters(wxString plogfileName, long plenfile, long plogfileLength, wxString psavedPartialLine) {
        logfileName = plogfileName;
        len = plenfile;
        logfileLength = plogfileLength;
        if (plogfileLength==0) inquote = false;
        savedPartialLine = psavedPartialLine;
    }
    long GetReadByteFile(wxString &part) {
        part = savedPartialLine;
        return logfileLength;
    }
    bool isReadyRows() {
        wxMutexError e = s_mutexDBLogReading.TryLock();
        if (e == wxMUTEX_NO_ERROR) {
            s_mutexDBLogReading.Unlock();
            return true;
        }
        return false;
    }
    void BreakRead();
    bool isBreak() { return m_break; }
    bool GoReadRows() {
        m_break = false;
        wxSemaError e = s_goReadLog.Post();
        if (e == wxSEMA_NO_ERROR) {
            return true;
        }
        //wxLogError("Semafore return error");
        return false;
    }

    virtual void* Entry();
private:
    void readLogFile(wxString logfileName, long& lenfile, long& logfileLength, wxString& savedPartialLine);
    void getFilename();
    void sendText(wxString s) {
        wxThreadEvent e(wxEVT_THREAD);
        e.SetString(s);
        theParent->GetEventHandler()->AddPendingEvent(e);
    }
    wxMutex s_mutexDBLogReading;
    wxSemaphore s_goReadLog;
    wxSemaphore s_ResultOkLog;

    wxDateTime nextrun;
    wxWindow* theParent;
    bool m_exit = false;
    bool m_break = false;
    pgConn *conn;
    wxMessageQueue<wxString>* log_queue;
    //
    wxString logfileName;
    wxString savedPartialLine;
    long logfileLength;
    long len;
    bool inquote = false;

};

class frmStatus : public pgFrame
{
public:
    frmStatus(frmMain *form, const wxString &_title, pgConn *conn);
    ~frmStatus();
    void Go();
    bool getTextSqlbyQid(long long qid);
    ReadLogThread* logThread = NULL;
private:
    wxAuiManager manager;

    frmMain *mainForm;
    pgConn *connection, *locks_connection, *logconn;

    wxString logFormat,logFindString;
    bool logHasTimestamp, logFormatKnown;
    int logFmtPos;
    long logFinditem=-1;
    
    wxMessageQueue<wxString> log_queue;
    wxDateTime logfileTimestamp, latestTimestamp;
    wxString logDirectory, logfileName;

    wxString savedPartialLine;

    bool showCurrent, isCurrent;

    long backend_pid;
    int wait_event_type_col;
    bool isrecovery,track_commit_timestamp, is_read_log;
    bool wait_sample, wait_enable, wait_save,std,pro;
    bool frm_exit = false; // need close form
    bool logisread = false; // need close form
    WaitSample WS;
    wxTopActivity* top_small;
    wxString first_tt;
    bool loaded;
    long logfileLength;
    wxColour logcol[2];
    int addodd = 0;
    int currentPane;

    int statusSortColumn;
    wxString statusSortOrder;
    int lockSortColumn;
    wxString lockSortOrder;
    int xactSortColumn;
    wxString xactSortOrder;

    wxComboBox    *cbRate;
    wxComboBox    *cbLogfiles;
    wxButton      *btnRotateLog;
    ctlComboBoxFix *cbDatabase;

    wxTimer *refreshUITimer;
    wxTimer *statusTimer, *locksTimer, *xactTimer, *logTimer, *querystateTimer;
    int statusRate, locksRate, xactRate, logRate, querystateRate;
    
    ctlListView   *statusList;
    ctlListView   *lockList;
    ctlListView   *xactList;
    ctlListView   *logList;
    ctlNavigatePanel* nav;
    ctlListView   *querystateList;

    wxMenu        *actionMenu;
    wxMenu        *statusPopupMenu;
    wxMenu        *lockPopupMenu;
    wxMenu        *xactPopupMenu;
    wxMenu        *querystatePopupMenu;
    wxString queryplan;
    wxArrayString queries;
    wxArrayInt filterColumn;
    wxArrayString filterValue;

    int statusColWidth[12], lockColWidth[10], xactColWidth[5], querystateColWidth[5];

    int cboToRate();
    wxString rateToCboString(int rate);

    wxImageList *listimages;
    long getlongvalue(wxString source,wxString match_str);
    void AddStatusPane();
    void AddLockPane();
    void AddXactPane();
    void AddLogPane();
        void AddQuerystatePane();

    void OnHelp(wxCommandEvent &ev);
    void OnContents(wxCommandEvent &ev);
    void OnExit(wxCommandEvent &event);

    void OnCopy(wxCommandEvent &ev);
    void OnCopyQuery(wxCommandEvent &ev);

    void OnToggleStatusPane(wxCommandEvent &event);
    void OnToggleLockPane(wxCommandEvent &event);
    void OnToggleXactPane(wxCommandEvent &event);
    void OnToggleLogPane(wxCommandEvent &event);
    void OnToggleQuerystatePane(wxCommandEvent &event);
    void OnToggleWaitEnable(wxCommandEvent& event);
    void OnEmptyAction(wxCommandEvent &event);
    void OnLogContextMenu(wxCommandEvent& event);

    void OnToggleToolBar(wxCommandEvent &event);
    void OnDefaultView(wxCommandEvent &event);
    void OnHighlightStatus(wxCommandEvent &event);

    void OnRefreshUITimer(wxTimerEvent &event);
    void OnRefreshStatusTimer(wxTimerEvent &event);
    void OnRefreshLocksTimer(wxTimerEvent &event);
    void OnRefreshXactTimer(wxTimerEvent &event);
    void OnRefreshLogTimer(wxTimerEvent &event);
    void OnRefreshQuerystateTimer(wxTimerEvent &event);

    void SetColumnImage(ctlListView *list, int col, int image);
    void OnSortStatusGrid(wxListEvent &event);
    void OnRightClickStatusItem(wxListEvent& event);
    void OnSortLockGrid(wxListEvent &event);
    void OnSortXactGrid(wxListEvent &event);

    void OnRightClickStatusGrid(wxListEvent &event);
    void OnRightClickLockGrid(wxListEvent &event);
    void OnRightClickXactGrid(wxListEvent &event);
    void OnRightClickQuerystateGrid(wxListEvent &event);
    void OnRightClickLogGrid(wxListEvent& event);

    void OnStatusMenu(wxCommandEvent &event);
    void OnLockMenu(wxCommandEvent &event);
    void OnXactMenu(wxCommandEvent &event);
    void OnQuerystateMenu(wxCommandEvent &event);
                                     
    void OnChgColSizeStatusGrid(wxListEvent &event);
    void OnChgColSizeLockGrid(wxListEvent &event);
    void OnChgColSizeXactGrid(wxListEvent &event);
    void OnChgColSizeQuerystateGrid(wxListEvent &event);
    
    void OnRateChange(wxCommandEvent &event);

    void OnPaneClose(wxAuiManagerEvent &evt);
    void OnPaneActivated(wxAuiManagerEvent& evt);

    void OnClose(wxCloseEvent &event);
    void OnRefresh(wxCommandEvent &event);
    void OnCancelBtn(wxCommandEvent &event);
    void OnStatusCancelBtn(wxCommandEvent &event);
    void OnLocksCancelBtn(wxCommandEvent &event);
    void OnTerminateBtn(wxCommandEvent &event);
    void OnStatusTerminateBtn(wxCommandEvent &event);
    void OnLocksTerminateBtn(wxCommandEvent &event);
    void OnSelStatusItem(wxListEvent &event);
    void OnSelLockItem(wxListEvent &event);
    void OnSelXactItem(wxListEvent &event);
    void OnSelLogItem(wxListEvent &event);
    void OnSelQuerystateItem(wxListEvent &event);
    void OnLoadLogfile(wxCommandEvent &event);
    void OnRotateLogfile(wxCommandEvent &event);
    void OnCommit(wxCommandEvent &event);
    void OnRollback(wxCommandEvent &event);
    void OnClearFilter(wxCommandEvent& event);
    void OnLogKeyUp(wxKeyEvent& event);
    void OnAddLabelTextThread(wxThreadEvent& event);
    void ActivatePane(wxString name);

    void OnChangeDatabase(wxCommandEvent &ev);

    int fillLogfileCombo();
    void emptyLogfileCombo();

    void addLogFile(wxDateTime *dt, bool skipFirst);
    void addLogFile(const wxString &filename, const wxDateTime timestamp, long len, long &read, bool skipFirst);
    void addLogLine(const wxString &str, bool formatted = true, bool csv_log_format = false);
    void checkConnection();

    DECLARE_EVENT_TABLE()
};


class serverStatusFactory : public actionFactory
{
public:
    serverStatusFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar);
    wxWindow *StartDialog(frmMain *form, pgObject *obj);
    bool CheckEnable(pgObject *obj);
};

#endif
