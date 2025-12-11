//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// frmStatus.cpp - Status Screen
//
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include <wx/regex.h>
#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/textbuf.h>
#include <wx/clipbrd.h>
#include <wx/sysopt.h>
#include <wx/rawbmp.h>
#include <wx/stdpaths.h>
// wxAUI
#include <wx/aui/aui.h>

// App headers
#include "frm/frmAbout.h"
#include "frm/frmStatus.h"
#include "frm/frmHint.h"
#include "frm/frmMain.h"
#include "db/pgConn.h"
#include "frm/frmQuery.h"
#include "utils/pgfeatures.h"
#include "schema/pgServer.h"
#include "schema/pgUser.h"
#include "ctl/ctlMenuToolbar.h"
#include "ctl/ctlAuiNotebook.h"
#include "utils/csvfiles.h"
#include "utils/utffile.h"
#include "ctl/ctlNavigatePanel.h"
#include "ctl/wxTopActivity.h"
// Icons
#include "images/clip_copy.pngc"
#include "images/readdata.pngc"
#include "images/query_cancel.pngc"
#include "images/terminate_backend.pngc"
#include "images/delete.pngc"
#include "images/storedata.pngc"
#include "images/sortfilterclear.pngc"
#include "images/down.pngc"
#include "images/up.pngc"
#include "images/server_status.pngc"
#include "images/warning_amber_48dp.pngc"

#include <algorithm>
#include "utils/FunctionPGHelper.h"
#include "utils/PreviewHtml.h"

#include "db/pgConn.h"
#ifdef __WXMSW__
#include "wx/msw/wrapcctl.h"
#endif

extern int s_pid_HIGHLIGH;

#define CTRLID_DATABASE         4200

//wxDEFINE_EVENT(EVENT_FIND_STR, wxCommandEvent);

BEGIN_EVENT_TABLE(frmStatus, pgFrame)
    EVT_MENU(MNU_EXIT,                            frmStatus::OnExit)
    EVT_MENU(MNU_COPY,                            frmStatus::OnCopy)
    EVT_MENU(MNU_COPY_QUERY,                      frmStatus::OnCopyQuery)
    EVT_MENU(MNU_HELP,                            frmStatus::OnHelp)
    EVT_MENU(MNU_CONTENTS,                        frmStatus::OnContents)
    EVT_MENU(MNU_STATUSPAGE,                      frmStatus::OnToggleStatusPane)
    EVT_MENU(MNU_LOCKPAGE,                        frmStatus::OnToggleLockPane)
    EVT_MENU(MNU_XACTPAGE,                        frmStatus::OnToggleXactPane)
    EVT_MENU(MNU_LOGPAGE,                         frmStatus::OnToggleLogPane)
    EVT_MENU(MNU_QUERYSTATEPAGE,                  frmStatus::OnToggleQuerystatePane)
    EVT_MENU(MNU_WAITENABLE,                      frmStatus::OnToggleWaitEnable)
    EVT_MENU(MNU_QUERYSTATEVERBOSE,				  frmStatus::OnEmptyAction)
    EVT_MENU(MNU_QUERYSTATETIME,				  frmStatus::OnEmptyAction)
    EVT_MENU(MNU_QUERYSTATEBUFFER,                frmStatus::OnEmptyAction)
    EVT_MENU(MNU_QUERYSTATETRIGGER,               frmStatus::OnEmptyAction)

    EVT_MENU(MNU_TOOLBAR,                         frmStatus::OnToggleToolBar)
    EVT_MENU(MNU_DEFAULTVIEW,                     frmStatus::OnDefaultView)
    EVT_MENU(MNU_HIGHLIGHTSTATUS,                 frmStatus::OnHighlightStatus)

    EVT_AUI_PANE_CLOSE(                           frmStatus::OnPaneClose)
    //EVT_AUI_PANE_ACTIVATED(                       frmStatus::OnPaneActivated)
    EVT_COMBOBOX(CTL_RATECBO,                     frmStatus::OnRateChange)
    EVT_MENU(MNU_REFRESH,                         frmStatus::OnRefresh)
    EVT_MENU(MNU_CANCEL,                          frmStatus::OnCancelBtn)
    EVT_MENU(MNU_TERMINATE,                       frmStatus::OnTerminateBtn)
    EVT_MENU(MNU_COMMIT,                          frmStatus::OnCommit)
    EVT_MENU(MNU_ROLLBACK,                        frmStatus::OnRollback)
    EVT_MENU(MNU_CLEAR_FILTER_SERVER_STATUS,      frmStatus::OnClearFilter)
    EVT_MENU(MNU_SET_FILTER_HIGHLIGHT_STATUS,     frmStatus::OnSetHighlightFilter)
    EVT_MENU(CMD_EVENT_FIND_STR,                  frmStatus::OnCmdFindStrLog)
    EVT_COMBOBOX(CTL_LOGCBO,                      frmStatus::OnLoadLogfile)
    EVT_BUTTON(CTL_ROTATEBTN,                     frmStatus::OnRotateLogfile)

    EVT_TIMER(TIMER_REFRESHUI_ID,                 frmStatus::OnRefreshUITimer)

    EVT_TIMER(TIMER_STATUS_ID,                    frmStatus::OnRefreshStatusTimer)
    EVT_LIST_ITEM_SELECTED(CTL_STATUSLIST,        frmStatus::OnSelStatusItem)
    //EVT_LIST_ITEM_DESELECTED(CTL_STATUSLIST,      frmStatus::OnSelStatusItem)
    EVT_LIST_ITEM_RIGHT_CLICK(CTL_STATUSLIST,	  frmStatus::OnRightClickStatusItem)
    EVT_LIST_COL_CLICK(CTL_STATUSLIST,            frmStatus::OnSortStatusGrid)
    EVT_LIST_COL_RIGHT_CLICK(CTL_STATUSLIST,      frmStatus::OnRightClickStatusGrid)
    EVT_LIST_COL_END_DRAG(CTL_STATUSLIST,         frmStatus::OnChgColSizeStatusGrid)

    EVT_TIMER(TIMER_LOCKS_ID,                     frmStatus::OnRefreshLocksTimer)
    EVT_LIST_ITEM_SELECTED(CTL_LOCKLIST,          frmStatus::OnSelLockItem)
    EVT_LIST_ITEM_DESELECTED(CTL_LOCKLIST,        frmStatus::OnSelLockItem)
    EVT_LIST_COL_CLICK(CTL_LOCKLIST,              frmStatus::OnSortLockGrid)
    EVT_LIST_COL_RIGHT_CLICK(CTL_LOCKLIST,        frmStatus::OnRightClickLockGrid)
    EVT_LIST_COL_END_DRAG(CTL_LOCKLIST,           frmStatus::OnChgColSizeLockGrid)

    EVT_TIMER(TIMER_XACT_ID,                      frmStatus::OnRefreshXactTimer)
    EVT_LIST_ITEM_SELECTED(CTL_XACTLIST,          frmStatus::OnSelXactItem)
    EVT_LIST_ITEM_DESELECTED(CTL_XACTLIST,        frmStatus::OnSelXactItem)
    EVT_LIST_COL_CLICK(CTL_XACTLIST,              frmStatus::OnSortXactGrid)
    EVT_LIST_COL_RIGHT_CLICK(CTL_XACTLIST,        frmStatus::OnRightClickXactGrid)
    EVT_LIST_COL_END_DRAG(CTL_XACTLIST,           frmStatus::OnChgColSizeXactGrid)
    EVT_TIMER(TIMER_LOG_ID,                       frmStatus::OnRefreshLogTimer)
    EVT_LIST_ITEM_SELECTED(CTL_LOGLIST,           frmStatus::OnSelLogItem)
    EVT_LIST_ITEM_DESELECTED(CTL_LOGLIST,         frmStatus::OnSelLogItem)
    EVT_LIST_ITEM_RIGHT_CLICK(CTL_LOGLIST,        frmStatus::OnRightClickLogGrid)
    EVT_TIMER(TIMER_LOGHINT_ID,					  frmStatus::OnTimerHintLog)
    EVT_TIMER(TIMER_QUERYSTATE_ID,                frmStatus::OnRefreshQuerystateTimer)
    EVT_LIST_COL_RIGHT_CLICK(CTL_QUERYSTATELIST,  frmStatus::OnRightClickQuerystateGrid)
    EVT_LIST_ITEM_SELECTED(CTL_QUERYSTATELIST,    frmStatus::OnSelQuerystateItem)
    EVT_LIST_ITEM_DESELECTED(CTL_QUERYSTATELIST,  frmStatus::OnSelQuerystateItem)
    EVT_LIST_COL_END_DRAG(CTL_QUERYSTATELIST,     frmStatus::OnChgColSizeQuerystateGrid)
    EVT_COMBOBOX(CTRLID_DATABASE,                 frmStatus::OnChangeDatabase)

    EVT_CLOSE(                                    frmStatus::OnClose)
END_EVENT_TABLE();


int frmStatus::cboToRate()
{
    int rate = 0;

    if (cbRate->GetValue() == _("Don't refresh"))
        rate = 0;
    if (cbRate->GetValue() == _("1 second"))
        rate = 1;
    if (cbRate->GetValue() == _("5 seconds"))
        rate = 5;
    if (cbRate->GetValue() == _("10 seconds"))
        rate = 10;
    if (cbRate->GetValue() == _("30 seconds"))
        rate = 30;
    if (cbRate->GetValue() == _("1 minute"))
        rate = 60;
    if (cbRate->GetValue() == _("5 minutes"))
        rate = 300;
    if (cbRate->GetValue() == _("10 minutes"))
        rate = 600;
    if (cbRate->GetValue() == _("30 minutes"))
        rate = 1800;
    if (cbRate->GetValue() == _("1 hour"))
        rate = 3600;

    return rate;
}


wxString frmStatus::rateToCboString(int rate)
{
    wxString rateStr;

    if (rate == 0)
        rateStr = _("Don't refresh");
    if (rate == 1)
        rateStr = _("1 second");
    if (rate == 5)
        rateStr = _("5 seconds");
    if (rate == 10)
        rateStr = _("10 seconds");
    if (rate == 30)
        rateStr = _("30 seconds");
    if (rate == 60)
        rateStr = _("1 minute");
    if (rate == 300)
        rateStr = _("5 minutes");
    if (rate == 600)
        rateStr = _("10 minutes");
    if (rate == 1800)
        rateStr = _("30 minutes");
    if (rate == 3600)
        rateStr = _("1 hour");

    return rateStr;
}

bool frmStatus::getTextSqlbyQid(long long qid) {
    bool rez = false;
    if (wait_sample && wait_enable && (std || pro)) {
        wxString q;
        wxString view;
        if (std) view = "pg_stat_statements";
        if (pro) view = "pgpro_stats_statements";
        q=wxString::Format("select distinct queryid,query from %s s ",view);
        if (qid != 0) {
            // where 
            q+= wxString::Format("where s.queryid=%lld limit 1",qid);
        }
        wxCriticalSectionLocker lock(gs_critsect);
        pgSet* dataSet1 = connection->ExecuteSet(q);
        if (dataSet1)
        {
            while (!dataSet1->Eof())
            {
                wxULongLong qid = dataSet1->GetLongLong("queryid");
                wxString query = dataSet1->GetVal("query");
                WS.AddQuery(qid.GetValue(), query);
                rez = true;
                dataSet1->MoveNext();
            }
            delete dataSet1;
        }

    }
    return rez;
}
frmStatus::frmStatus(frmMain *form, const wxString &_title, pgConn *conn) : pgFrame(NULL, _title)
{
    wxString initquery;
    bool highlight = false;

    dlgName = wxT("frmStatus");

    loaded = false;

    mainForm = form;
    connection = conn;
    locks_connection = conn;
    logconn = NULL;
    statusTimer = 0;
    locksTimer = 0;
    xactTimer = 0;
    logTimer = 0;

    logHasTimestamp = false;
    logFormatKnown = false;

    // Only superusers can set these parameters...
    
        if (connection->IsSuperuser())
        {
            // Make the connection quiet on the logs
            if (connection->BackendMinimumVersion(8, 0))
                initquery = wxT("SET log_statement='none';SET log_duration='off';SET log_min_duration_statement=-1;SET statement_timeout=10000;");
            else
                initquery = wxT("SET log_statement='off';SET log_duration='off';SET log_min_duration_statement=-1;");
#ifndef _DEBUG
            initquery += wxT("set log_min_messages = FATAL;");
#endif // !_DEBUG

            connection->ExecuteVoid(initquery, false);
        }
        //pg_is_in_recovery()
        waitMenu = new wxMenu();
        waitMenu->Append(MNU_WAITENABLE, _("&Wait trace"), _("Enable the wait event."), wxITEM_CHECK);
        waitMenu->Append(MNU_WAITSAVE, _("&Wait event save"), _("Enable the wait event save to file sample.dat."), wxITEM_CHECK);
        settings->Read(wxT("frmStatus/WaitTraceEnable"), &wait_enable, false);
        settings->Read(wxT("frmStatus/WaitSave"), &wait_save, false);
        waitMenu->Check(MNU_WAITENABLE,wait_enable);
        waitMenu->Check(MNU_WAITSAVE, wait_save);
        //MNU_WAITSAVE
        wxString q = "select pg_is_in_recovery() recovery, \
                             (select setting from pg_settings s where name = 'pg_wait_sampling.history_size')::integer hsize, \
                             (select setting from pg_settings s where name = 'pg_wait_sampling.history_period')::integer hperiod, \
                             EXTRACT(epoch FROM coalesce(current_setting('idle_in_transaction_session_timeout',true),'30s')::interval)::integer idle_in_transaction_session_timeout,\
                             (select pg_table_is_visible(viewname::regclass) and has_table_privilege(viewname::regclass,'select') from pg_views where viewname = 'pg_wait_sampling_history') as wsh, \
                             (select pg_table_is_visible(viewname::regclass) and has_table_privilege(viewname::regclass,'select') from pg_views v where viewname='pgpro_stats_statements') as pro, \
                             (select pg_table_is_visible(viewname::regclass) and has_table_privilege(viewname::regclass,'select') from pg_views v where viewname='pg_stat_statements') as std, \
                             has_function_privilege('pg_read_binary_file(text,bigint,bigint,boolean)','execute') and has_function_privilege('pg_stat_file(text,boolean)','execute') and has_function_privilege('pg_ls_logdir()','execute') isreadlog \
                     ";

        pgSet* dataSet1 = connection->ExecuteSet(q);
        pro = false;
        std = false;
        wait_sample = false;
        is_read_log = false;
        if (dataSet1)
        {
            while (!dataSet1->Eof())
            {
                wxString v = dataSet1->GetVal(wxT("recovery"));
                pro= dataSet1->GetBool(wxT("pro"));
                std = dataSet1->GetBool(wxT("std"));
                is_read_log = dataSet1->GetBool(wxT("isreadlog"));
                idle_in_transaction_session_timeout= dataSet1->GetLong(wxT("idle_in_transaction_session_timeout"));
                isrecovery = (v == wxT("t"));
                track_commit_timestamp = connection->HasFeature(FEATURE_TRACK_COMMIT_TS);
                long sz = dataSet1->GetLong(wxT("hsize"));
                long p = dataSet1->GetLong(wxT("hperiod"));
                if (!dataSet1->GetBool(wxT("wsh"))) p = 0;
                if (sz > 1000000) {
                    wxLogWarning(_("Value parameter pg_wait_sampling.history_size = %ld greet 1000000  monitoring wait events disabled.\n"),
                        sz
                    );
                    p = 0;
                }
                if (p > 0) {
                    wait_sample = true;
                    WS.SetConfig(p,sz,this);
                }
                dataSet1->MoveNext();
            }
            delete dataSet1 ;
        }
        if (!wait_sample) {
            waitMenu->Enable(MNU_WAITENABLE, false);
            waitMenu->Enable(MNU_WAITSAVE, false);
        }
        if (wait_sample && wait_enable && (std || pro)) {
            getTextSqlbyQid(0);
        }

    // Notify wxAUI which frame to use
    manager.SetManagedWindow(this);
    manager.SetFlags(wxAUI_MGR_DEFAULT | wxAUI_MGR_TRANSPARENT_DRAG | wxAUI_MGR_ALLOW_ACTIVE_PANE);

    // Set different window's attributes
    SetTitle(_title);
    SetName(dlgName);
    SetIcon(*server_status_png_ico);
    RestorePosition(-1, -1, 700, 500, 700, 500);
    SetMinSize(FromDIP(wxSize(700, 500)));
    SetFont(settings->GetSystemFont());

    // Build menu bar
    menuBar = new wxMenuBar();

    fileMenu = new wxMenu();
    fileMenu->Append(MNU_EXIT, _("E&xit\tCtrl-W"), _("Exit query window"));

    menuBar->Append(fileMenu, _("&File"));

    editMenu = new wxMenu();
    editMenu->Append(MNU_COPY, _("&Copy\tCtrl-C"), _("Copy selected text to clipboard"), wxITEM_NORMAL);

    menuBar->Append(editMenu, _("&Edit"));

    actionMenu = new wxMenu();
    actionMenu->Append(MNU_REFRESH, _("Refresh\tCtrl-R"), _("Refresh the selected panel"), wxITEM_NORMAL);
    actionMenu->AppendSeparator();
    actionMenu->Append(MNU_COPY_QUERY, _("Copy to query tool\tCtrl-Shift-C"), _("Open the query tool with the selected query"), wxITEM_NORMAL);
    actionMenu->Append(MNU_CANCEL, _("Cancel query\tDel"), _("Cancel the selected query"), wxITEM_NORMAL);
    actionMenu->Append(MNU_TERMINATE, _("Terminate backend\tShift-Del"), _("Terminate the selected backend"), wxITEM_NORMAL);
    actionMenu->AppendSeparator();
    actionMenu->Append(MNU_COMMIT, _("Commit prepared transaction"), _("Commit the selected prepared transaction"), wxITEM_NORMAL);
    actionMenu->Append(MNU_ROLLBACK, _("Rollback prepared transaction"), _("Rollback the selected prepared transaction"), wxITEM_NORMAL);

    menuBar->Append(actionMenu, _("&Action"));

    viewMenu = new wxMenu();
    viewMenu->Append(MNU_STATUSPAGE, _("&Activity\tCtrl-Alt-A"), _("Show or hide the activity tab."), wxITEM_CHECK);
    viewMenu->Append(MNU_LOCKPAGE, _("&Locks\tCtrl-Alt-L"), _("Show or hide the locks tab."), wxITEM_CHECK);
    viewMenu->Append(MNU_XACTPAGE, _("Prepared &Transactions\tCtrl-Alt-T"), _("Show or hide the prepared transactions tab."), wxITEM_CHECK);
    viewMenu->Append(MNU_LOGPAGE, _("Log&file\tCtrl-Alt-F"), _("Show or hide the logfile tab."), wxITEM_CHECK);
    viewMenu->AppendSeparator();
    viewMenu->Append(MNU_QUERYSTATEPAGE, _("&Query state\tCtrl-Alt-Q"), _("Show or hide the query state tab."), wxITEM_CHECK);
    viewMenu->Append(MNU_QUERYSTATEVERBOSE, _("Append verbose"), _("Append verbose"), wxITEM_CHECK);
    viewMenu->Append(MNU_QUERYSTATEBUFFER, _("Append use buffers"), _("Append use buffers"), wxITEM_CHECK);
    viewMenu->Append(MNU_QUERYSTATETIME, _("Append real timing"), _("Append real timing"), wxITEM_CHECK);
    viewMenu->Append(MNU_QUERYSTATETRIGGER, _("Append triggers"), _("Append triggers"), wxITEM_CHECK);

    viewMenu->AppendSeparator();
    viewMenu->Append(MNU_TOOLBAR, _("Tool&bar\tCtrl-Alt-B"), _("Show or hide the toolbar."), wxITEM_CHECK);
    viewMenu->Append(MNU_HIGHLIGHTSTATUS, _("Highlight items of the activity list"), _("Highlight or not the items of the activity list."), wxITEM_CHECK);
    viewMenu->AppendSeparator();
    viewMenu->Append(MNU_DEFAULTVIEW, _("&Default view\tCtrl-Alt-V"), _("Restore the default view."));
    
    menuBar->Append(waitMenu, _("&Wait"));
    menuBar->Append(viewMenu, _("&View"));
    
    wxMenu *helpMenu = new wxMenu();
    helpMenu->Append(MNU_CONTENTS, _("&Help contents"), _("Open the helpfile."));
    helpMenu->Append(MNU_HELP, _("&Server status help"), _("Display help on this window."));

#ifdef __WXMAC__
    menuFactories = new menuFactoryList();
    aboutFactory *af = new aboutFactory(menuFactories, helpMenu, 0);
    wxApp::s_macAboutMenuItemId = af->GetId();
    menuFactories->RegisterMenu(this, wxCommandEventHandler(pgFrame::OnAction));
#endif

    menuBar->Append(helpMenu, _("&Help"));

    // Setup edit menu
    editMenu->Enable(MNU_COPY, false);

    // Finish menu bar
    SetMenuBar(menuBar);

    // Set statusBar
    statusBar = CreateStatusBar(1);
    SetStatusBarPane(-1);

    // Set up toolbar
    toolBar = new ctlMenuToolbar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
    toolBar->SetToolBitmapSize(FromDIP(wxSize(32, 32)));
    toolBar->AddTool(MNU_REFRESH, wxEmptyString, GetBundleSVG(readdata_png_bmp, "refresh.svg", FromDIP(wxSize(32, 32))), _("Refresh"), wxITEM_NORMAL);
    toolBar->AddSeparator();
    toolBar->AddTool(MNU_COPY, wxEmptyString, GetBundleSVG(clip_copy_png_bmp, "clip_copy.svg", FromDIP(wxSize(32, 32))), _("Copy selected text to clipboard"), wxITEM_NORMAL);
    toolBar->AddTool(MNU_COPY_QUERY, wxEmptyString, GetBundleSVG(clip_copy_png_bmp, "clip_copy_sql.svg", FromDIP(wxSize(32, 32))), _("Open the query tool with the selected query"), wxITEM_NORMAL);
    toolBar->AddSeparator();
    toolBar->AddTool(MNU_CANCEL, wxEmptyString, GetBundleSVG(query_cancel_png_bmp, "query_cancel.svg", FromDIP(wxSize(32, 32))), _("Cancel query"), wxITEM_NORMAL);
    toolBar->AddTool(MNU_TERMINATE, wxEmptyString, GetBundleSVG(terminate_backend_png_bmp, "terminate_backend.svg", FromDIP(wxSize(32, 32))), _("Terminate backend"), wxITEM_NORMAL);
    toolBar->AddTool(MNU_COMMIT, wxEmptyString, GetBundleSVG(storedata_png_bmp, "storedata.svg", FromDIP(wxSize(32, 32))), _("Commit transaction"), wxITEM_NORMAL);
    toolBar->AddTool(MNU_ROLLBACK, wxEmptyString, GetBundleSVG(delete_png_bmp, "drop.svg", FromDIP(wxSize(32, 32))), _("Rollback transaction"), wxITEM_NORMAL);
    toolBar->AddSeparator();
    toolBar->AddTool(MNU_CLEAR_FILTER_SERVER_STATUS, wxEmptyString, GetBundleSVG(sortfilterclear_png_bmp, "sortfilterclear.svg", FromDIP(wxSize(32, 32))), _("Clear filter"), wxITEM_NORMAL);
    toolBar->AddTool(MNU_SET_FILTER_HIGHLIGHT_STATUS, wxEmptyString, GetBundleSVG(warning_amber_48dp_png_bmp, "warning_amber_48dp.svg", FromDIP(wxSize(32, 32))), _("Highlight items only"), wxITEM_NORMAL);
    toolBar->AddSeparator();
    cbLogfiles = new wxComboBox(toolBar, CTL_LOGCBO, wxT(""), wxDefaultPosition, wxDefaultSize, 0, NULL,
                                wxCB_READONLY | wxCB_DROPDOWN);
    toolBar->AddControl(cbLogfiles);
    btnRotateLog = new wxButton(toolBar, CTL_ROTATEBTN, _("Rotate"));
    toolBar->AddControl(btnRotateLog);
    toolBar->AddSeparator();
    cbRate = new wxComboBox(toolBar, CTL_RATECBO, wxEmptyString, wxDefaultPosition, wxSize(-1, -1), wxArrayString(), wxCB_READONLY | wxCB_DROPDOWN);
    toolBar->AddControl(cbRate);
    toolBar->AddSeparator();
    cbDatabase = new ctlComboBoxFix(toolBar, CTRLID_DATABASE, wxDefaultPosition, wxSize(-1, -1), wxCB_READONLY | wxCB_DROPDOWN);
    toolBar->AddControl(cbDatabase);
    if (wait_sample && wait_enable) {
        toolBar->AddSeparator();
        top_small = new wxTopActivity(toolBar, &WS, wxSize(100, 32));
        toolBar->AddControl(top_small);
    }
    else top_small = NULL;

    toolBar->Realize();

    // Append items to cbo
    cbRate->Append(_("Don't refresh"));
    cbRate->Append(_("1 second"));
    cbRate->Append(_("5 seconds"));
    cbRate->Append(_("10 seconds"));
    cbRate->Append(_("30 seconds"));
    cbRate->Append(_("1 minute"));
    cbRate->Append(_("5 minutes"));
    cbRate->Append(_("10 minutes"));
    cbRate->Append(_("30 minutes"));
    cbRate->Append(_("1 hour"));

    // Disable toolbar's items
    toolBar->EnableTool(MNU_CANCEL, false);
    toolBar->EnableTool(MNU_TERMINATE, false);
    toolBar->EnableTool(MNU_COMMIT, false);
    toolBar->EnableTool(MNU_ROLLBACK, false);
    toolBar->EnableTool(MNU_CLEAR_FILTER_SERVER_STATUS, false);
    actionMenu->Enable(MNU_CANCEL, false);
    actionMenu->Enable(MNU_TERMINATE, false);
    actionMenu->Enable(MNU_COMMIT, false);
    actionMenu->Enable(MNU_ROLLBACK, false);
    cbLogfiles->Enable(false);
    btnRotateLog->Enable(false);

    // Add the database combobox
    pgSet *dataSet3 = connection->ExecuteSet(wxT("SELECT datname FROM pg_database WHERE datallowconn ORDER BY datname"));
    while (!dataSet3->Eof())
    {
        cbDatabase->Append(dataSet3->GetVal(wxT("datname")));
        dataSet3->MoveNext();
    }
    delete dataSet3;

    // Image list for all listviews
    listimages = new wxImageList(13, 8, true, 2);
    listimages->Add(*down_png_ico);
    listimages->Add(*up_png_ico);

    // Create panel
    AddStatusPane();
    AddLockPane();
    AddXactPane();
    AddQuerystatePane();
    AddLogPane();
    wxSize toolw = toolBar->GetBestSize();

    manager.AddPane(toolBar, wxAuiPaneInfo().Name(wxT("toolBar")).Caption(_("Tool bar")).ToolbarPane().Top().LeftDockable(false).RightDockable(false));

    // Now load the layout
    wxString perspective;
    settings->Read(wxT("frmStatus/Perspective-") + wxString(FRMSTATUS_PERSPECTIVE_VER), &perspective, FRMSTATUS_DEFAULT_PERSPECTIVE);
    manager.LoadPerspective(perspective, true);
    // Reset the captions for the current language
    manager.GetPane(wxT("toolBar")).Caption(_("Tool bar"));
    manager.GetPane(wxT("Activity")).Caption(_("Activity"));
    manager.GetPane(wxT("Locks")).Caption(_("Locks"));
    manager.GetPane(wxT("Transactions")).Caption(_("Prepared Transactions"));
    manager.GetPane(wxT("Logfile")).Caption(_("Logfile"));
    manager.GetPane(wxT("Querystate")).Caption(_("QueryState"));
    manager.GetPane(wxT("toolBar")).BestSize(toolw);
    //manager.GetPane(wxT("Activity")).GripperTop(true);
    // Tell the manager to "commit" all the changes just made
    manager.Update();

    // Sync the View menu options
    viewMenu->Check(MNU_STATUSPAGE, manager.GetPane(wxT("Activity")).IsShown());
    viewMenu->Check(MNU_LOCKPAGE, manager.GetPane(wxT("Locks")).IsShown());
    viewMenu->Check(MNU_XACTPAGE, manager.GetPane(wxT("Transactions")).IsShown());
    viewMenu->Check(MNU_LOGPAGE, manager.GetPane(wxT("Logfile")).IsShown());
        pgSet *set = connection->ExecuteSet(wxT("SELECT 1 FROM pg_available_extensions WHERE installed_version is not null and name='pg_query_state'"));
        bool queryenable = set->NumRows() == 1;
        viewMenu->Enable(MNU_QUERYSTATEPAGE, queryenable);
        if (!queryenable) {
            viewMenu->Check(MNU_QUERYSTATEPAGE, false);
            manager.GetPane(wxT("Querystate")).Hide();
        }
        //viewMenu->Check(MNU_QUERYSTATEPAGE,set->NumRows() == 1);
        delete set;

    viewMenu->Check(MNU_TOOLBAR, manager.GetPane(wxT("toolBar")).IsShown());
    //
    //if (!viewMenu->IsEnabled(MNU_QUERYSTATEPAGE)) {
    //	manager.GetPane(wxT("Querystate")).Hide();
    //	} else manager.GetPane(wxT("Querystate")).Show();

    viewMenu->Check(MNU_QUERYSTATEPAGE,manager.GetPane(wxT("Querystate")).IsShown());
    // Read the highlight status checkbox
    settings->Read(wxT("frmStatus/HighlightStatus"), &highlight, true);
    viewMenu->Check(MNU_HIGHLIGHTSTATUS, highlight);
    bool qu_status;
    settings->Read(wxT("frmStatus/QuerystateVerboseStatus"), &qu_status, false);
    viewMenu->Check(MNU_QUERYSTATEVERBOSE, qu_status);
    settings->Read(wxT("frmStatus/QuerystateTimeStatus"), &qu_status, true);
    viewMenu->Check(MNU_QUERYSTATETIME, qu_status);
    settings->Read(wxT("frmStatus/QuerystateBufferStatus"), &qu_status, true);
    viewMenu->Check(MNU_QUERYSTATEBUFFER, qu_status);
    settings->Read(wxT("frmStatus/QuerystateTriggerStatus"), &qu_status, true);
    viewMenu->Check(MNU_QUERYSTATETRIGGER, qu_status);

    
    // Get our PID
    backend_pid = connection->GetBackendPID();

    // Create the refresh timer (quarter of a second)
    // This is a horrible hack to get around the lack of a
    // PANE_ACTIVATED event in wxAUI.
    refreshUITimer = new wxTimer(this, TIMER_REFRESHUI_ID);
    refreshUITimer->Start(250);

    // The selected pane is the log pane by default
    // so enable/disable the widgets according to this
    wxListEvent nullevent;
    OnSelLogItem(nullevent);

    // We're good now
    loaded = true;
}


frmStatus::~frmStatus()
{
    // Delete the refresh timer
    delete refreshUITimer;
    // If the status window wasn't launched in standalone mode...
    if (mainForm)
        mainForm->RemoveFrame(this);

    // Save the window's position
    settings->Write(wxT("frmStatus/Perspective-") + wxString(FRMSTATUS_PERSPECTIVE_VER), manager.SavePerspective());
    manager.UnInit();
    SavePosition();

    // Save the highlight status checkbox
    settings->WriteBool(wxT("frmStatus/HighlightStatus"), viewMenu->IsChecked(MNU_HIGHLIGHTSTATUS));
    settings->WriteBool(wxT("frmStatus/QuerystateVerboseStatus"), viewMenu->IsChecked(MNU_QUERYSTATEVERBOSE));
    settings->WriteBool(wxT("frmStatus/QuerystateTimeStatus"), viewMenu->IsChecked(MNU_QUERYSTATETIME));
    settings->WriteBool(wxT("frmStatus/QuerystateBufferStatus"), viewMenu->IsChecked(MNU_QUERYSTATEBUFFER));
    settings->WriteBool(wxT("frmStatus/QuerystateTriggerStatus"), viewMenu->IsChecked(MNU_QUERYSTATETRIGGER));
    if (wait_sample) settings->WriteBool(wxT("frmStatus/WaitTraceEnable"), waitMenu->IsChecked(MNU_WAITENABLE));
    if (wait_sample) settings->WriteBool(wxT("frmStatus/WaitSave"), waitMenu->IsChecked(MNU_WAITSAVE));
    if (wait_sample && wait_enable && waitMenu->IsChecked(MNU_WAITSAVE)) WS.SaveFileSamples(); else {
        // remove file
        WS.RemoveFiles();
    }


    // For each current page, save the slider's position and delete the timer
    settings->WriteInt(wxT("frmStatus/RefreshStatusRate"), statusRate);
    delete statusTimer;
    settings->WriteInt(wxT("frmStatus/RefreshLockRate"), locksRate);
    delete locksTimer;
    if (viewMenu->IsEnabled(MNU_XACTPAGE))
    {
        settings->WriteInt(wxT("frmStatus/RefreshXactRate"), xactRate);
        if (xactTimer)
        {
            delete xactTimer;
            xactTimer = NULL;
        }
    }
    if (viewMenu->IsEnabled(MNU_LOGPAGE))
    {
        settings->WriteInt(wxT("frmStatus/RefreshLogRate"), logRate);
        emptyLogfileCombo();
        if (logTimer)
        {
            delete logTimer;
            logTimer = NULL;
        }
    }
    if (viewMenu->IsEnabled(MNU_QUERYSTATEPAGE))
    {
        settings->WriteInt(wxT("frmStatus/RefreshQuerystateRate"), querystateRate);
        if (querystateTimer)
        {
            delete querystateTimer;
            querystateTimer = NULL;
        }
    }

    // If connection is still available, delete it
    if (locks_connection && locks_connection != connection)
    {
        if (locks_connection->IsAlive())
            delete locks_connection;
    }
    if (connection)
    {
        if (connection->IsAlive())
            delete connection;
    }
    if (logThread) {
        logThread->BreakRead();
        wxMilliSleep(50);
        logThread->DoTerminate();
        //s_CloseLog.Wait();
        while (logThread != NULL) wxMilliSleep(50) ;
    }
    if (logconn)
    {
        if (logconn->IsAlive())
            delete logconn;
    }
}


void frmStatus::Go()
{
    // Show the window
    Show(true);

    // Send RateChange event to launch each timer
    wxScrollEvent nullScrollEvent;
    if (viewMenu->IsChecked(MNU_STATUSPAGE))
    {
        currentPane = PANE_STATUS;
        cbRate->SetValue(rateToCboString(statusRate));
        OnRateChange(nullScrollEvent);
    }
    if (viewMenu->IsChecked(MNU_LOCKPAGE))
    {
        currentPane = PANE_LOCKS;
        cbRate->SetValue(rateToCboString(locksRate));
        OnRateChange(nullScrollEvent);
    }
    if (viewMenu->IsEnabled(MNU_XACTPAGE) && viewMenu->IsChecked(MNU_XACTPAGE))
    {
        currentPane = PANE_XACT;
        cbRate->SetValue(rateToCboString(xactRate));
        OnRateChange(nullScrollEvent);
    }
    if (viewMenu->IsEnabled(MNU_LOGPAGE) && viewMenu->IsChecked(MNU_LOGPAGE))
    {
        currentPane = PANE_LOG;
        cbRate->SetValue(rateToCboString(logRate));
        OnRateChange(nullScrollEvent);
    }
    if (viewMenu->IsEnabled(MNU_QUERYSTATEPAGE) && viewMenu->IsChecked(MNU_QUERYSTATEPAGE))
    {
        currentPane = PANE_QUERYSTATE;
        cbRate->SetValue(rateToCboString(querystateRate));
        OnRateChange(nullScrollEvent);
    }

    // Refresh all pages
    wxCommandEvent nullEvent;
    OnRefresh(nullEvent);
}


void frmStatus::OnClose(wxCloseEvent &event)
{

    frm_exit = true;
    if (logThread && logisread) {
        logThread->BreakRead();
        event.Veto();
    } else Destroy();
    //Destroy();
}


void frmStatus::OnExit(wxCommandEvent &event)
{
    wxCloseEvent e;
    OnClose(e);
    //Destroy();
}


void frmStatus::OnChangeDatabase(wxCommandEvent &ev)
{
    wxString initquery;

    if (locks_connection != connection)
    {
        delete locks_connection;
    }

    locks_connection = new pgConn(connection->GetHostName(), connection->GetService(), connection->GetHostAddr(), cbDatabase->GetValue(),
                                  connection->GetUser(), connection->GetPassword(), connection->GetPort(), connection->GetRole(),"", connection->GetSslMode(),
                                  0, connection->GetApplicationName(), connection->GetSSLCert(), connection->GetSSLKey(), connection->GetSSLRootCert(), connection->GetSSLCrl(),
                                  connection->GetSSLCompression());

    pgUser *user = new pgUser(locks_connection->GetUser());
    if (user)
    {
        if (user->GetSuperuser())
        {
            if (locks_connection->BackendMinimumVersion(8, 0))
                initquery = wxT("SET log_statement='none';SET log_duration='off';SET log_min_duration_statement=-1;");
            else
                initquery = wxT("SET log_statement='off';SET log_duration='off';SET log_min_duration_statement=-1;");
            locks_connection->ExecuteVoid(initquery, false);
        }
        delete user;
    }
}


void frmStatus::AddStatusPane()
{
    // Create panel
    wxPanel *pnlActivity = new wxPanel(this);

    // Create flex grid
    wxFlexGridSizer *grdActivity = new wxFlexGridSizer(1, 1, 5, 5);
    grdActivity->AddGrowableCol(0);
    grdActivity->AddGrowableRow(0);

    // Add the list control
#ifdef __WXMAC__
    // Switch to the generic list control.
    // Disable sort on Mac.
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), true);
#endif
    wxListCtrl *lstStatus = new wxListCtrl(pnlActivity, CTL_STATUSLIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxSUNKEN_BORDER);
    // Now switch back
#ifdef __WXMAC__
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), false);
#endif
    grdActivity->Add(lstStatus, 0, wxGROW, 3);

    // Add the panel to the notebook
    manager.AddPane(pnlActivity,
                    wxAuiPaneInfo().
                    Name(wxT("Activity")).Caption(_("Activity")).
                    CaptionVisible(true).CloseButton(true).MaximizeButton(true).
                    Dockable(true).Movable(true));

    // Auto-sizing
    pnlActivity->SetSizer(grdActivity);
    grdActivity->Fit(pnlActivity);

    // Add each column to the list control
    statusList = (ctlListView *)lstStatus;
    statusList->AddColumn(_("PID"), 35);
    if (connection->BackendMinimumVersion(8, 5))
        statusList->AddColumn(_("Application name"), 70);
    statusList->AddColumn(_("Database"), 70);
    statusList->AddColumn(_("User"), 70);
    if (connection->BackendMinimumVersion(8, 1))
    {
        statusList->AddColumn(_("Client"), 70);
        statusList->AddColumn(_("Client start"), 80);
    }
    if (connection->BackendMinimumVersion(7, 4))
        statusList->AddColumn(_("Query start"), 50);
    if (connection->BackendMinimumVersion(8, 3))
        statusList->AddColumn(_("TX start"), 50);
    if (connection->BackendMinimumVersion(9, 2))
    {
        statusList->AddColumn(_("State"), 35);
        statusList->AddColumn(_("State change"), 35);
    }
    if (connection->BackendMinimumVersion(9, 4))
    {
        statusList->AddColumn(_("Backend XID"), 35);
        statusList->AddColumn(_("Backend XMin"), 35);
    }
    if (connection->BackendMinimumVersion(9, 6))
    {
        statusList->AddColumn(_("W_Event_T"), 35);
        statusList->AddColumn(_("W_Event"), 35);
    }
    statusList->AddColumn(_("Blocked by"), 35);
    statusList->AddColumn(_("Query"), 500);

    // Get through the list of columns to build the popup menu
    // and reinitialize column's width if we find a saved width
    statusPopupMenu = new wxMenu();
    wxListItem item;
    item.SetMask(wxLIST_MASK_TEXT);
    int savedwidth;
    for (int col = 0; col < statusList->GetColumnCount(); col++)
    {
        // Get column
        statusList->GetColumn(col, item);

        // Reinitialize column's width
        settings->Read(wxT("frmStatus/StatusPane_") + item.GetText() + wxT("_Width"), &savedwidth, statusList->GetColumnWidth(col));
        if (savedwidth > 0)
            statusList->SetColumnWidth(col, savedwidth);
        else
            statusList->SetColumnWidth(col, 0);
        statusColWidth[col] = savedwidth;

        // Add new check item on the popup menu
        statusPopupMenu->AppendCheckItem(1000 + col, item.GetText());
        statusPopupMenu->Check(1000 + col, statusList->GetColumnWidth(col) > 0);
        this->Connect(1000 + col, wxEVT_COMMAND_MENU_SELECTED,
                      wxCommandEventHandler(frmStatus::OnStatusMenu));
    }

    // Build image list
    statusList->SetImageList(listimages, wxIMAGE_LIST_SMALL);

    // Read statusRate configuration
    settings->Read(wxT("frmStatus/RefreshStatusRate"), &statusRate, 10);

    // Initialize sort order
    statusSortColumn = 1;
    statusSortOrder = wxT("ASC");
    // Create the timer
    statusTimer = new wxTimer(this, TIMER_STATUS_ID);
}


void frmStatus::AddLockPane()
{
    // Create panel
    wxPanel *pnlLock = new wxPanel(this);

    // Create flex grid
    wxFlexGridSizer *grdLock = new wxFlexGridSizer(1, 1, 5, 5);
    grdLock->AddGrowableCol(0);
    grdLock->AddGrowableRow(0);

    // Add the list control
#ifdef __WXMAC__
    // Switch to the generic list control.
    // Disable sort on Mac.
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), true);
#endif
    wxListCtrl *lstLocks = new wxListCtrl(pnlLock, CTL_LOCKLIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxSUNKEN_BORDER);
    // Now switch back
#ifdef __WXMAC__
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), false);
#endif
    grdLock->Add(lstLocks, 0, wxGROW, 3);

    // Add the panel to the notebook
    manager.AddPane(pnlLock,
                    wxAuiPaneInfo().
                    Name(wxT("Locks")).Caption(_("Locks")).
                    CaptionVisible(true).CloseButton(true).MaximizeButton(true).
                    Dockable(true).Movable(true));

    // Auto-sizing
    pnlLock->SetSizer(grdLock);
    grdLock->Fit(pnlLock);

    // Add each column to the list control
    lockList = (ctlListView *)lstLocks;
    lockList->AddColumn(wxT("PID"), 35);
    lockList->AddColumn(_("Database"), 50);
    lockList->AddColumn(_("Relation"), 50);
    lockList->AddColumn(_("User"), 50);
    if (locks_connection->BackendMinimumVersion(8, 3))
        lockList->AddColumn(_("XID"), 50);
    lockList->AddColumn(_("TX"), 50);
    lockList->AddColumn(_("Mode"), 50);
    lockList->AddColumn(_("Granted"), 50);
    if (locks_connection->BackendMinimumVersion(7, 4))
        lockList->AddColumn(_("Start"), 50);
    lockList->AddColumn(_("Query"), 500);

    // Get through the list of columns to build the popup menu
    lockPopupMenu = new wxMenu();
    wxListItem item;
    item.SetMask(wxLIST_MASK_TEXT);
    int savedwidth;
    for (int col = 0; col < lockList->GetColumnCount(); col++)
    {
        // Get column
        lockList->GetColumn(col, item);
        int currwidth = lockList->GetColumnWidth(col);
        // Reinitialize column's width
        settings->Read(wxT("frmStatus/LockPane_") + item.GetText() + wxT("_Width"), &savedwidth, currwidth);
        if (savedwidth > 0)
            lockList->SetColumnWidth(col, savedwidth);
        else
            lockList->SetColumnWidth(col, 0);
        lockColWidth[col] = savedwidth;

        // Add new check item on the popup menu
        lockPopupMenu->AppendCheckItem(2000 + col, item.GetText());
        lockPopupMenu->Check(2000 + col, lockList->GetColumnWidth(col) > 0);
        this->Connect(2000 + col, wxEVT_COMMAND_MENU_SELECTED,
                      wxCommandEventHandler(frmStatus::OnLockMenu));
    }

    // Build image list
    lockList->SetImageList(listimages, wxIMAGE_LIST_SMALL);

    // Read locksRate configuration
    settings->Read(wxT("frmStatus/RefreshLockRate"), &locksRate, 10);

    // Initialize sort order
    lockSortColumn = 1;
    lockSortOrder = wxT("ASC");

    // Create the timer
    locksTimer = new wxTimer(this, TIMER_LOCKS_ID);
}


void frmStatus::AddXactPane()
{
    // Create panel
    wxPanel *pnlXacts = new wxPanel(this);

    // Create flex grid
    wxFlexGridSizer *grdXacts = new wxFlexGridSizer(1, 1, 5, 5);
    grdXacts->AddGrowableCol(0);
    grdXacts->AddGrowableRow(0);

    // Add the list control
#ifdef __WXMAC__
    // Switch to the generic list control.
    // Disable sort on Mac.
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), true);
#endif
    wxListCtrl *lstXacts = new wxListCtrl(pnlXacts, CTL_XACTLIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxSUNKEN_BORDER);
    // Now switch back
#ifdef __WXMAC__
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), false);
#endif
    grdXacts->Add(lstXacts, 0, wxGROW, 3);

    // Add the panel to the notebook
    manager.AddPane(pnlXacts,
                    wxAuiPaneInfo().
                    Name(wxT("Transactions")).Caption(_("Transactions")).
                    CaptionVisible(true).CloseButton(true).MaximizeButton(true).
                    Dockable(true).Movable(true));

    // Auto-sizing
    pnlXacts->SetSizer(grdXacts);
    grdXacts->Fit(pnlXacts);

    // Add the xact list
    xactList = (ctlListView *)lstXacts;

    // We don't need this report if server release is less than 8.1
    // GPDB doesn't have external global transactions.
    // Perhaps we should use this display to show our
    // global xid to local xid mappings?
    if (!connection->BackendMinimumVersion(8, 1) || connection->GetIsGreenplum())
    {
        // manager.GetPane(wxT("Transactions")).Show(false);
        lstXacts->InsertColumn(lstXacts->GetColumnCount(), _("Message"), wxLIST_FORMAT_LEFT, 800);
        lstXacts->InsertItem(lstXacts->GetItemCount(), _("Prepared transactions not available on this server."), -1);
        lstXacts->Enable(false);
        xactTimer = NULL;

        // We're done
        return;
    }

    // Add each column to the list control
    xactList->AddColumn(wxT("XID"), 50);
    xactList->AddColumn(_("Global ID"), 200);
    xactList->AddColumn(_("Time"), 100);
    xactList->AddColumn(_("Owner"), 50);
    xactList->AddColumn(_("Database"), 50);

    // Get through the list of columns to build the popup menu
    xactPopupMenu = new wxMenu();
    wxListItem item;
    item.SetMask(wxLIST_MASK_TEXT);
    int savedwidth;
    for (int col = 0; col < xactList->GetColumnCount(); col++)
    {
        // Get column
        xactList->GetColumn(col, item);
        int currwidth = xactList->GetColumnWidth(col);
        // Reinitialize column's width
        settings->Read(wxT("frmStatus/XactPane_") + item.GetText() + wxT("_Width"), &savedwidth, currwidth);
        if (savedwidth > 0)
            xactList->SetColumnWidth(col, savedwidth);
        else
            xactList->SetColumnWidth(col, 0);
        xactColWidth[col] = savedwidth;

        // Add new check item on the popup menu
        xactPopupMenu->AppendCheckItem(3000 + col, item.GetText());
        xactPopupMenu->Check(3000 + col, xactList->GetColumnWidth(col) > 0);
        this->Connect(3000 + col, wxEVT_COMMAND_MENU_SELECTED,
                      wxCommandEventHandler(frmStatus::OnXactMenu));
    }

    // Build image list
    xactList->SetImageList(listimages, wxIMAGE_LIST_SMALL);

    // Read xactRate configuration
    settings->Read(wxT("frmStatus/RefreshXactRate"), &xactRate, 10);

    // Initialize sort order
    xactSortColumn = 2;
    xactSortOrder = wxT("ASC");

    // Create the timer
    xactTimer = new wxTimer(this, TIMER_XACT_ID);
}
void frmStatus::AddQuerystatePane()
{
    // Create panel
    wxPanel *pnlQuerystate = new wxPanel(this);

    // Create flex grid
    wxFlexGridSizer *grdQuerystate = new wxFlexGridSizer(1, 1, 5, 5);
    grdQuerystate->AddGrowableCol(0);
    grdQuerystate->AddGrowableRow(0);

    // Add the list control
#ifdef __WXMAC__
    // Switch to the generic list control.
    // Disable sort on Mac.
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), true);
#endif
    wxListCtrl *lstQuerystate = new wxListCtrl(pnlQuerystate, CTL_QUERYSTATELIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxSUNKEN_BORDER);
    // Now switch back
#ifdef __WXMAC__
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), false);
#endif
    grdQuerystate->Add(lstQuerystate, 0, wxGROW, 3);

    // Add the panel to the notebook
    manager.AddPane(pnlQuerystate,
                    wxAuiPaneInfo().Center().
                    Name(wxT("Querystate")).Caption(_("Query State")).
                    CaptionVisible(true).CloseButton(true).MaximizeButton(true).
                    Dockable(true).Movable(true));

    // Auto-sizing
    pnlQuerystate->SetSizer(grdQuerystate);
    grdQuerystate->Fit(pnlQuerystate);

    // Add the xact list
    querystateList = (ctlListView *)lstQuerystate;

    // Add each column to the list control
    querystateList->AddColumn(wxT("Pid"), 20);
    querystateList->AddColumn(wxT("Fn"), 20);
    querystateList->AddColumn(_("Query Text"), 200);
    querystateList->AddColumn(_("Plan"), 300);
    querystateList->AddColumn(wxT("Leader_pid"), 20);

    // Get through the list of columns to build the popup menu
    querystatePopupMenu = new wxMenu();
    wxListItem item;
    item.SetMask(wxLIST_MASK_TEXT);
    int savedwidth;
    for (int col = 0; col < querystateList->GetColumnCount(); col++)
    {
        // Get column
        querystateList->GetColumn(col, item);
        wxString namec=item.GetText();
        int currwidth=querystateList->GetColumnWidth(col);
        // Reinitialize column's width
        settings->Read(wxT("frmStatus/QuerystatePane_") + namec + wxT("_Width"), &savedwidth, currwidth);
        if (savedwidth > 0)
            querystateList->SetColumnWidth(col, savedwidth);
        else
            querystateList->SetColumnWidth(col, 0);
        querystateColWidth[col] = savedwidth;

        // Add new check item on the popup menu
        querystatePopupMenu->AppendCheckItem(4000 + col, item.GetText());
        querystatePopupMenu->Check(4000 + col, querystateList->GetColumnWidth(col) > 0);
        this->Connect(4000 + col, wxEVT_COMMAND_MENU_SELECTED,
                      wxCommandEventHandler(frmStatus::OnQuerystateMenu));
    }

    // Build image list
    querystateList->SetImageList(listimages, wxIMAGE_LIST_SMALL);

    // Read querystateRate configuration
    settings->Read(wxT("frmStatus/RefreshQuerystateRate"), &querystateRate, 10);

    // Initialize sort order
    //QuerystateortColumn = 2;
    //QuerystateortOrder = wxT("ASC");

    // Create the timer
    querystateTimer = new wxTimer(this, TIMER_QUERYSTATE_ID);
    
         pgSet *set = connection->ExecuteSet(wxT("SELECT 1 FROM pg_available_extensions WHERE installed_version is not null and name='pg_query_state'"));
            if (set->NumRows() == 1)
                viewMenu->Check(MNU_QUERYSTATEPAGE,true);
            else
                viewMenu->Check(MNU_QUERYSTATEPAGE,false);
            delete set;

}


void frmStatus::AddLogPane()
{
    int rc = -1;
    wxString hint = HINT_INSTRUMENTATION;

    // Create panel
    wxPanel *pnlLog = new wxPanel(this);
    //pnlLog->SetBackgroundColour(*wxRED);
    // Create flex grid
    wxFlexGridSizer *grdLog = new wxFlexGridSizer(1, 2, 2, 0);
    //wxBoxSizer* grdLog = new wxBoxSizer(wxHORIZONTAL);
    grdLog->AddGrowableCol(1);
    grdLog->AddGrowableRow(0);

    // Add the list control
#ifdef __WXMAC__
    // Switch to the generic list control.
    // Disable sort on Mac.
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), true);
#endif

    //wxListCtrl *lstLog = new wxListCtrl(pnlLog, CTL_LOGLIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxSUNKEN_BORDER);
    logList = new ctlListView(pnlLog, CTL_LOGLIST, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
    logList->SetToolTip(NULL);
    // hide tooltip for windows
#ifdef __WXMSW__
    HWND hwndList = (HWND)logList->GetHandle();
    LPARAM style = ::SendMessage(hwndList, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    ::SendMessage(hwndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, style & ~LVS_EX_LABELTIP);
#endif
    // Add the log list
    //logList = (ctlListView*)lstLog;
    logList->SetModeStoreLongString();
    nav = new ctlNavigatePanel(pnlLog, logList);
    logList->Bind(wxEVT_KEY_UP, &frmStatus::OnLogKeyUp, this);
    //lstLog->Bind(wxEVT_MENU, &ctlNavigatePanel::OnContextMenu, this);
    Bind(wxEVT_THREAD, &frmStatus::OnAddLabelTextThread, this);
    logList->Bind(wxEVT_MENU, &frmStatus::OnLogContextMenu, this);
    logList->Bind(wxEVT_MOTION, &frmStatus::OnMoveMouseLog, this);
    //Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(frmLog::OnAddLabelTextThread), NULL, this);
    // Now switch back
#ifdef __WXMAC__
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), false);
#endif
    delayHitLog = new wxTimer(this, TIMER_LOGHINT_ID);
    //Bind(wxEVT_TIMER, &frmStatus::OnTimerHintLog, this);
    ///delayHitLog->Start(DELAYHITLOGPERIOD);

    grdLog->Add(nav, 0, wxSHRINK, 3);
    grdLog->Add(logList, 0, wxALL | wxEXPAND, 3);
    // Add the panel to the notebook
    manager.AddPane(pnlLog,
                    wxAuiPaneInfo().Center().
                    Name(wxT("Logfile")).Caption(_("Logfile")).
                    CaptionVisible(true).CloseButton(true).MaximizeButton(true).
                    Dockable(true).Movable(true));

    // Auto-sizing
    pnlLog->SetSizer(grdLog);
    grdLog->Fit(pnlLog);

    logcol[0] = logList->GetBackgroundColour();
    logcol[1] = wxColour("#afafaf");
    // We don't need this report (but we need the pane)
    // if server release is less than 8.0 or if server has no adminpack
    if (!is_read_log) {
        logList->InsertColumn(logList->GetColumnCount(), _("Message"), wxLIST_FORMAT_LEFT, 700);
        logList->AppendItemLong(-1, _("Functions pg_read_binary_file(text,bigint,bigint,boolean), pg_stat_file(text,boolean),pg_ls_logdir()  permission denied."));
        logList->Enable(false);
        logTimer = NULL;
        // We're done
        return;

    }
    if (!(connection->BackendMinimumVersion(8, 0) &&
            connection->HasFeature(FEATURE_FILEREAD)))
    {
        // if the server release is 9.1 or more and the server has no adminpack
        if (connection->BackendMinimumVersion(9, 1))
        {
            // Search the adminpack extension
            pgSet *set = connection->ExecuteSet(wxT("SELECT 1 FROM pg_available_extensions WHERE name='adminpack'"));
            if (set->NumRows() == 1)
                hint = HINT_INSTRUMENTATION_91_WITH;
            else
                hint = HINT_INSTRUMENTATION_91_WITHOUT;
            delete set;
        }

        if (connection->BackendMinimumVersion(8, 0))
            rc = frmHint::ShowHint(this, hint);

        if (rc == HINT_RC_FIX)
            connection->ExecuteVoid(wxT("CREATE EXTENSION adminpack"), true);

        if (!connection->HasFeature(FEATURE_FILEREAD, true))
        {
            logList->InsertColumn(logList->GetColumnCount(), _("Message"), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE_USEHEADER);
            logList->AppendItemLong(-1,_("Logs are not available for this server."));
            logList->Enable(false);
            logTimer = NULL;
            // We're done
            return;
        }
    }

    // Add each column to the list control
    logFormat = connection->ExecuteScalar(wxT("SHOW log_line_prefix"));
    if (logFormat == wxT("unset"))
        logFormat = wxEmptyString;
    logFmtPos = logFormat.Find('%', true);

    if (logFmtPos < 0)
        logFormatKnown = true;  // log_line_prefix not specified.
    else if (!logFmtPos && logFormat.Mid(logFmtPos, 2) == wxT("%t") && logFormat.Length() > 2)  // Timestamp at end of log_line_prefix?
    {
        logFormatKnown = true;
        logHasTimestamp = true;
    }
    else if (connection->GetIsGreenplum())
    {
        // Always %m|%u|%d|%p|%I|%X|:- (timestamp w/ millisec) for 3.2.x
        // Usually CSV formatted for 3.3
        logFormatKnown = true;
        logHasTimestamp = true;
    }


    if (connection->GetIsGreenplum() && connection->BackendMinimumVersion(8, 2, 13))
    {
        // Be ready for GPDB CSV format log file
        logList->AddColumn(_("Timestamp"), 120);  // Room for millisecs
        logList->AddColumn(_("Level"), 35);
        logList->AddColumn(_("Log entry"), 400);
        logList->AddColumn(_("Connection"), 45);
        logList->AddColumn(_("Cmd number"), 48);
        logList->AddColumn(_("Dbname"), 48);
        logList->AddColumn(_("Segment"), 45);
    }
    else    // Non-GPDB or non-CSV format log
    {
        if (logHasTimestamp)
            logList->AddColumn(_("Timestamp"), 100);

        if (logFormatKnown)
            logList->AddColumn(_("Level"), 35);

        logList->AddColumn(_("Log entry"), 800);
    }

    if (!connection->HasFeature(FEATURE_ROTATELOG))
        btnRotateLog->Disable();

    // Re-initialize variables
    logfileLength = 0;

    // Read logRate configuration
    settings->Read(wxT("frmStatus/RefreshLogRate"), &logRate, 10);

    // Create the timer
    logTimer = new wxTimer(this, TIMER_LOG_ID);
}

void frmStatus::OnCopy(wxCommandEvent &ev)
{
    ctlListView *list;
    int row, col;
    wxString text;

    switch(currentPane)
    {
        case PANE_STATUS:
            list = statusList;
            break;
        case PANE_LOCKS:
            list = lockList;
            break;
        case PANE_XACT:
            list = xactList;
            break;
        case PANE_LOG:
            list = logList;
            break;
        case PANE_QUERYSTATE:
            list = querystateList;
            break;
        default:
            // This shouldn't happen.
            // If it does, it's no big deal, we just need to get out.
            return;
            break;
    }
    if (currentPane==PANE_QUERYSTATE) {
            wxString s;
            for (row = 0; row < list->GetItemCount(); row++)
            {
                s=list->GetText(row, 2);
                if (s.Length()>0) {
                    text.Append(wxT("SQL QUERY: ")).Append(s);
                    #ifdef __WXMSW__
                            text.Append(wxT("\r\n"));
                    #else
                            text.Append(wxT("\n"));
                    #endif
                }
                text.Append(list->GetText(row, 3));
                #ifdef __WXMSW__
                        text.Append(wxT("\r\n"));
                #else
                        text.Append(wxT("\n"));
                #endif
            }
//		list->GetText(row,3);
    } else 
    {
        row = list->GetFirstSelected();

        while (row >= 0)
        {
            for (col = 0; col < list->GetColumnCount(); col++)
            {
                text.Append(list->GetText(row, col) + wxT("\t"));
            }
    #ifdef __WXMSW__
            text.Append(wxT("\r\n"));
    #else
            text.Append(wxT("\n"));
    #endif
            row = list->GetNextSelected(row);
        }
    }
    if (text.Length() > 0 && wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
    }
}


void frmStatus::OnCopyQuery(wxCommandEvent &ev)
{
    ctlListView *list;
    int row, col;
    wxString text = wxT("");
    wxString dbname = wxT("");
    unsigned int maxlength;

    // Only the status list shows the query
    list = statusList;

    // Get the database
    row = list->GetFirstSelected();
    if (row != -1) {
        col = connection->BackendMinimumVersion(9, 0) ? 2 : 1;
        dbname.Append(list->GetText(row, col));

        // Get the actual query
        row = list->GetFirstSelected();
        text.Append(queries.Item(row));
    }
    else return;
    // Check if we have a query whose length is maximum
    maxlength = 1024;
    if (connection->BackendMinimumVersion(8, 4))
    {
        pgSet *set;
        set = connection->ExecuteSet(wxT("SELECT setting FROM pg_settings\n")
                                     wxT("  WHERE name='track_activity_query_size'"));
        if (set)
        {
            maxlength = set->GetLong(0);
            delete set;
        }
    }

    if (text.Length() == maxlength)
    {
        wxLogError(_("The query you copied is at the maximum length.\nIt may have been truncated."));
    }

    // If we have some real query, launch the query tool
    if (text.Length() > 0 && dbname.Length() > 0
            && text.Trim() != wxT("<IDLE>") && text.Trim() != wxT("<IDLE in transaction>"))
    {
        pgConn *conn = new pgConn(connection->GetHostName(), connection->GetService(), connection->GetHostAddr(), dbname,
                                  connection->GetUser(), connection->GetPassword(),
                                  connection->GetPort(), connection->GetRole(),"", connection->GetSslMode(), connection->GetDbOid(),
                                  connection->GetApplicationName(),
                                  connection->GetSSLCert(), connection->GetSSLKey(), connection->GetSSLRootCert(), connection->GetSSLCrl(),
                                  connection->GetSSLCompression());
        if (conn)
        {
            frmQuery *fq = new frmQuery(mainForm, wxEmptyString, conn, text);
            fq->Go();
            mainForm->AddFrame(fq);
        }
    }
}

void frmStatus::OnPaneActivated(wxAuiManagerEvent& evt) {
    wxTimerEvent event;
    OnRefreshUITimer(event);
}
void frmStatus::OnPaneClose(wxAuiManagerEvent &evt)
{
    if (evt.pane->name == wxT("Activity"))
    {
        viewMenu->Check(MNU_STATUSPAGE, false);
        statusTimer->Stop();
    }
    if (evt.pane->name == wxT("Locks"))
    {
        viewMenu->Check(MNU_LOCKPAGE, false);
        locksTimer->Stop();
    }
    if (evt.pane->name == wxT("Transactions"))
    {
        viewMenu->Check(MNU_XACTPAGE, false);
        if (xactTimer)
            xactTimer->Stop();
    }
    if (evt.pane->name == wxT("Logfile"))
    {
        viewMenu->Check(MNU_LOGPAGE, false);

        if (logThread) logThread->BreakRead();
        if (logTimer)
            logTimer->Stop();
    }
    if (evt.pane->name == wxT("Querystate"))
    {
        viewMenu->Check(MNU_QUERYSTATEPAGE, false);
        if (querystateTimer)
            querystateTimer->Stop();
    }
}


void frmStatus::OnToggleStatusPane(wxCommandEvent &event)
{
    if (viewMenu->IsChecked(MNU_STATUSPAGE))
    {
        manager.GetPane(wxT("Activity")).Show(true);
        cbRate->SetValue(rateToCboString(statusRate));
        if (statusRate > 0)
            statusTimer->Start(statusRate * 1000L);
    }
    else
    {
        manager.GetPane(wxT("Activity")).Show(false);
        statusTimer->Stop();
    }

    // Tell the manager to "commit" all the changes just made
    manager.Update();
}


void frmStatus::OnToggleLockPane(wxCommandEvent &event)
{
    if (viewMenu->IsChecked(MNU_LOCKPAGE))
    {
        manager.GetPane(wxT("Locks")).Show(true);
        cbRate->SetValue(rateToCboString(locksRate));
        if (locksRate > 0)
            locksTimer->Start(locksRate * 1000L);
    }
    else
    {
        manager.GetPane(wxT("Locks")).Show(false);
        locksTimer->Stop();
    }

    // Tell the manager to "commit" all the changes just made
    manager.Update();
}


void frmStatus::OnToggleXactPane(wxCommandEvent &event)
{
    if (viewMenu->IsEnabled(MNU_XACTPAGE) && viewMenu->IsChecked(MNU_XACTPAGE))
    {
        manager.GetPane(wxT("Transactions")).Show(true);
        cbRate->SetValue(rateToCboString(xactRate));
        if (xactRate > 0 && xactTimer)
            xactTimer->Start(xactRate * 1000L);
    }
    else
    {
        manager.GetPane(wxT("Transactions")).Show(false);
        if (xactTimer)
            xactTimer->Stop();
    }

    // Tell the manager to "commit" all the changes just made
    manager.Update();
}


void frmStatus::OnToggleLogPane(wxCommandEvent &event)
{
    if (viewMenu->IsEnabled(MNU_LOGPAGE) && viewMenu->IsChecked(MNU_LOGPAGE))
    {
        manager.GetPane(wxT("Logfile")).Show(true);
        cbRate->SetValue(rateToCboString(logRate));
        if (logRate > 0 && logTimer)
            logTimer->Start(logRate * 1000L);
        wxTimerEvent e;
        OnRefreshLogTimer(e);
    }
    else
    {
        manager.GetPane(wxT("Logfile")).Show(false);
        if (logThread) logThread->BreakRead();
        if (logTimer)
            logTimer->Stop();
    }

    // Tell the manager to "commit" all the changes just made
    manager.Update();
}
void frmStatus::OnToggleWaitEnable(wxCommandEvent& event) {

}
void frmStatus::OnToggleQuerystatePane(wxCommandEvent &event)
{
    if (viewMenu->IsEnabled(MNU_QUERYSTATEPAGE) && viewMenu->IsChecked(MNU_QUERYSTATEPAGE))
    {
        manager.GetPane(wxT("Querystate")).Show(true);
        cbRate->SetValue(rateToCboString(querystateRate));
        if (querystateRate > 0 && querystateTimer)
            querystateTimer->Start(querystateRate * 1000L);
    }
    else
    {
        manager.GetPane(wxT("Querystate")).Show(false);
        if (querystateTimer)
            querystateTimer->Stop();
    }

    // Tell the manager to "commit" all the changes just made
    manager.Update();
}


void frmStatus::OnToggleToolBar(wxCommandEvent &event)
{
    if (viewMenu->IsChecked(MNU_TOOLBAR))
    {
        manager.GetPane(wxT("toolBar")).Show(true);
    }
    else
    {
        manager.GetPane(wxT("toolBar")).Show(false);
    }

    // Tell the manager to "commit" all the changes just made
    manager.Update();
}


void frmStatus::OnDefaultView(wxCommandEvent &event)
{
    manager.LoadPerspective(FRMSTATUS_DEFAULT_PERSPECTIVE, true);

    // Reset the captions for the current language
    manager.GetPane(wxT("toolBar")).Caption(_("Tool bar"));
    manager.GetPane(wxT("Activity")).Caption(_("Activity"));
    manager.GetPane(wxT("Locks")).Caption(_("Locks"));
    manager.GetPane(wxT("Transactions")).Caption(_("Prepared Transactions"));
    manager.GetPane(wxT("Logfile")).Caption(_("Logfile"));
    manager.GetPane(wxT("Querystate")).Caption(_("Query State"));

    // tell the manager to "commit" all the changes just made
    manager.Update();

    // Sync the View menu options
    viewMenu->Check(MNU_TOOLBAR, manager.GetPane(wxT("toolBar")).IsShown());
    viewMenu->Check(MNU_STATUSPAGE, manager.GetPane(wxT("Activity")).IsShown());
    viewMenu->Check(MNU_LOCKPAGE, manager.GetPane(wxT("Locks")).IsShown());
    viewMenu->Check(MNU_XACTPAGE, manager.GetPane(wxT("Transactions")).IsShown());
    viewMenu->Check(MNU_LOGPAGE, manager.GetPane(wxT("Logfile")).IsShown());
    viewMenu->Check(MNU_QUERYSTATEPAGE, manager.GetPane(wxT("Querystate")).IsShown());
}


void frmStatus::OnHighlightStatus(wxCommandEvent &event)
{
    wxTimerEvent evt;

    OnRefreshStatusTimer(evt);
}
void frmStatus::OnEmptyAction(wxCommandEvent &event)
{
}


void frmStatus::OnHelp(wxCommandEvent &ev)
{
    DisplayHelp(wxT("status"), HELP_PGADMIN);
}


void frmStatus::OnContents(wxCommandEvent &ev)
{
    DisplayHelp(wxT("index"), HELP_PGADMIN);
}

void frmStatus::OnRateChange(wxCommandEvent &event)
{
    wxTimer *timer;
    int rate;

    switch(currentPane)
    {
        case PANE_STATUS:
            timer = statusTimer;
            rate = cboToRate();
            statusRate = rate;
            break;
        case PANE_LOCKS:
            timer = locksTimer;
            rate = cboToRate();
            locksRate = rate;
            break;
        case PANE_XACT:
            timer = xactTimer;
            rate = cboToRate();
            xactRate = rate;
            break;
        case PANE_LOG:
            timer = logTimer;
            rate = cboToRate();
            logRate = rate;
            break;
        case PANE_QUERYSTATE:
            timer = querystateTimer;
            rate = cboToRate();
            querystateRate = rate;
            break;
        default:
            // This shouldn't happen.
            // If it does, it's no big deal, we just need to get out.
            return;
            break;
    }

    if (timer)
    {
        timer->Stop();
        if (rate > 0)
            timer->Start(rate * 1000L);
    }
    OnRefresh(event);
}


void frmStatus::OnRefreshUITimer(wxTimerEvent &event)
{
    wxListEvent evt;
    refreshUITimer->Stop();
    if (frm_exit && !logisread) Destroy();
    for (unsigned int i = 0; i < manager.GetAllPanes().GetCount(); i++)
    {
        wxAuiPaneInfo &pane = manager.GetAllPanes()[i];

        if (pane.HasFlag(wxAuiPaneInfo::optionActive))
        {
            if (pane.name == wxT("Activity") && currentPane != PANE_STATUS)
            {
                OnSelStatusItem(evt);
            }
            if (pane.name == wxT("Locks") && currentPane != PANE_LOCKS)
            {
                OnSelLockItem(evt);
            }
            if (pane.name == wxT("Transactions") && currentPane != PANE_XACT)
            {
                OnSelXactItem(evt);
            }
            if (pane.name == wxT("Logfile") && currentPane != PANE_LOG)
            {
                OnSelLogItem(evt);
            }
            if (pane.name == wxT("Querystate") && currentPane != PANE_QUERYSTATE)
            {
                OnSelQuerystateItem(evt);
            }
        }
    }

    refreshUITimer->Start(250);
}


void frmStatus::OnRefreshStatusTimer(wxTimerEvent &event)
{
    long pid = 0;
    wxString pidcol = connection->BackendMinimumVersion(9, 2) ? wxT("p.pid") : wxT("p.procpid");
    wxString querycol = connection->BackendMinimumVersion(9, 2) ? wxT("query") : wxT("current_query");

    if (! viewMenu->IsChecked(MNU_STATUSPAGE))
        return;

    checkConnection();
    if (!connection)
    {
        statusTimer->Stop();
        locksTimer->Stop();
        if (xactTimer)
            xactTimer->Stop();
        if (logTimer)
            logTimer->Stop();
        if (querystateTimer)
            querystateTimer->Stop();
        return;
    }

    wxCriticalSectionLocker lock(gs_critsect);
    wxLongLong startTime = wxGetLocalTimeMillis();
    long row = 0;
    wxString q = wxT("SELECT ");
    wait_enable = waitMenu->IsChecked(MNU_WAITENABLE);
    // PID
    q += pidcol + wxT(" AS pid, ");

    // Application name (when available)
    if (connection->BackendMinimumVersion(8, 5))
        q += wxT("application_name, ");
    if (connection->BackendMinimumVersion(14, 0))
        q += wxT("query_id, ");
    
    // Database, and user name
    q += wxT("p.datname, usename,\n");

    // Client connection method
    if (connection->BackendMinimumVersion(8, 1))
    {
        q += wxT("CASE WHEN client_port=-1 THEN 'local pipe' ");
        if (connection->BackendMinimumVersion(9, 1))
            q += wxT("WHEN length(client_hostname)>0 THEN client_hostname||':'||client_port ");
        q += wxT("ELSE textin(inet_out(client_addr))||':'||client_port END AS client,\n");
    }

    // Backend start timestamp
    if (connection->BackendMinimumVersion(8, 1))
        q += wxT("date_trunc('second', backend_start) AS backend_start, ");

    // Query start timestamp (when available)
    if (connection->BackendMinimumVersion(9, 2))
    {
        q += wxT("CASE WHEN state='active' THEN date_trunc('second', query_start)::text ELSE '' END ");
    }
    else if (connection->BackendMinimumVersion(7, 4))
    {
        q += wxT("CASE WHEN ") + querycol + wxT("='' OR ") + querycol + wxT("='<IDLE>' THEN '' ")
             wxT("     ELSE date_trunc('second', query_start)::text END ");
    }
    else
    {
        q += wxT("'' ");
    }
    q += wxT("AS query_start,\n");

    // Transaction start timestamp
    if (connection->BackendMinimumVersion(8, 3))
        q += wxT("xact_start AS xact_start_full,date_trunc('second', xact_start) AS xact_start, ");

    // State
    if (connection->BackendMinimumVersion(9, 2))
        q += wxT("state, date_trunc('second', state_change) AS state_change, ");

    // Xmin and XID
    if (connection->BackendMinimumVersion(9, 4))
        q += wxT("backend_xid::text, backend_xmin::text, ");

    // Blocked by...
    //q +=   wxT("(SELECT min(l1.pid) FROM pg_locks l1 WHERE GRANTED AND (")
    //       wxT("relation IN (SELECT relation FROM pg_locks l2 WHERE l2.pid=") + pidcol + wxT(" AND NOT granted)")
    //       wxT(" OR ")
    //       wxT("transactionid IN (SELECT transactionid FROM pg_locks l3 WHERE l3.pid=") + pidcol + wxT(" AND NOT granted)")
    //       wxT(")) AS blockedby,\n");
    q += "pg_blocking_pids(p.pid) AS blockedby,\n";
    // Query
    q += querycol + wxT(" AS query,\n");

    // Slow query?
    if (connection->BackendMinimumVersion(9, 2))
    {
        q += wxT("CASE WHEN query_start IS NULL OR state<>'active' THEN false ELSE query_start < now() - '10 seconds'::interval END ");
    }
    else if (connection->BackendMinimumVersion(7, 4))
    {
        q += wxT("CASE WHEN query_start IS NULL OR ") + querycol + wxT(" LIKE '<IDLE>%' THEN false ELSE query_start < now() - '10 seconds'::interval END ");
    }
    else
    {
        q += wxT("false");
    }
    q += wxT("AS slowquery\n");
    wxString progress13="(select format('%s %s',phase,pr) progress_info,pid from (\
        select case phase\
        when 'scanning heap' then\
        Round(100 * heap_blks_scanned / greatest(heap_blks_total, 1), 1) || '%'\
    else\
        Round(100 * heap_blks_vacuumed / greatest(heap_blks_total, 1), 1) || '%'\
        end pr\
        , phase, pid from pg_stat_progress_vacuum\
        union all\
        select \
        Round(100 * backup_streamed / greatest(backup_total, backup_streamed,1)) || '% (' || pg_size_pretty(backup_streamed) || ')'\
        pr\
        , phase, pid from pg_stat_progress_basebackup\
        union all\
        select case phase\
        when 'writing new heap' then\
        Round(100 * heap_tuples_written / greatest(heap_tuples_scanned, 1), 1) || '%'\
    else\
        pg_size_pretty(heap_tuples_scanned) || ' rows'\
        end pr\
        , phase, pid from pg_stat_progress_cluster\
        union all\
        select\
        round(100 * sample_blks_scanned / greatest(sample_blks_total, 1), 2) || '%' pr\
        , phase, pid from pg_stat_progress_analyze\
        union all\
        select Round(100 * blocks_done / greatest(blocks_total, 1), 1) || '%' pr\
        , phase, pid from pg_stat_progress_create_index\
        ) v) v\
        ";
    bool iswalsend = false;
    if (connection->BackendMinimumVersion(10, 0))
    {
        if (connection->BackendMinimumVersion(13, 0))
            {
                q += wxT(",backend_type,wait_event_type,wait_event,v.progress_info,case when backend_type='autovacuum launcher' then (select min(xmin::text::bigint) from pg_replication_slots) end av_replica\n");
                iswalsend = true;
                if (wait_sample && wait_enable) {
                    q += ",transaction_timestamp() tt,hs.wait_sample,hs.maxts";
                }
            }
            else
            q += wxT(",backend_type,wait_event_type,wait_event,v.heap_blks_total,v.heap_blks_vacuumed,v.heap_blks_scanned,v.phase\n");

        if (!track_commit_timestamp)
            q += wxT(",coalesce(sl.xmin,sl.catalog_xmin)::text xmin_slot,':'||slot_name||'['||sl.slot_type||']' slotinfo,'LagSent:'||pg_size_pretty(pg_wal_lsn_diff(pg_last_wal_receive_lsn(),coalesce(confirmed_flush_lsn,restart_lsn)))||' LagXmin: -1'||' s' xminlag,-1 xminslotdelta\n");
        else if (isrecovery) 
            q += wxT(",coalesce(sl.xmin,sl.catalog_xmin)::text xmin_slot,':'||slot_name||'['||sl.slot_type||']' slotinfo,'LagSent:'||pg_size_pretty(pg_wal_lsn_diff(pg_last_wal_receive_lsn(),coalesce(confirmed_flush_lsn,restart_lsn)))||' LagXmin: '||coalesce(extract(epoch from (pg_last_committed_xact()).timestamp - pg_xact_commit_timestamp(xmin))::int,0)||' s' xminlag,coalesce(extract(epoch from (pg_last_committed_xact()).timestamp - pg_xact_commit_timestamp(xmin))::int,0) xminslotdelta\n");
        else
            q += wxT(",coalesce(sl.xmin,sl.catalog_xmin)::text xmin_slot,':'||slot_name||'['||sl.slot_type||']' slotinfo,'LagSent:'||pg_size_pretty(pg_wal_lsn_diff(pg_current_wal_lsn(),coalesce(confirmed_flush_lsn,restart_lsn)))||' LagXmin: '||coalesce(extract(epoch from (pg_last_committed_xact()).timestamp - pg_xact_commit_timestamp(xmin))::int,0)||' s' xminlag,coalesce(extract(epoch from (pg_last_committed_xact()).timestamp - pg_xact_commit_timestamp(xmin))::int,0) xminslotdelta\n");
        if (connection->BackendMinimumVersion(13, 0))
            q += "FROM pg_stat_activity p LEFT JOIN "+progress13+" ON p.pid=v.pid\n";
        else
            q += wxT("FROM pg_stat_activity p LEFT JOIN pg_stat_progress_vacuum v ON p.pid=v.pid\n");
        q += wxT("LEFT JOIN pg_replication_slots sl ON p.pid=sl.active_pid\n");
        if (wait_sample && wait_enable)
        {
            q += " left join ( \
            select pid, string_agg(format('%s:%s', coalesce(event_type,'CPU') || ':' || coalesce(event,'CPU')||':'||queryid, cnt), ';') wait_sample, max(maxts) maxts from( \
                select pid,queryid, event_type, event, max(ts) maxts, count(*) cnt from pg_wait_sampling_history h \
        where h.event not in('ArchiverMain', 'WalWriterMain', 'CheckpointerMain', 'Extension', 'LogicalLauncherMain', 'CfsGcEnable', 'WalSenderMain' \
            , 'AutoVacuumMain', '-ClientRead', 'CheckpointWriteDelay', 'BgWriterMain', 'BgWriterHibernate') \
            or h.event is null\
            ";

            if (!first_tt.IsEmpty()) q += "and h.ts > '"+first_tt+"'";

            q+="group by pid, event_type, event,queryid \
                ) l group by pid) hs on p.pid = hs.pid\n";
        }
        
        //backend_type
    } else
    {
        q += wxT("FROM pg_stat_activity p ");
    }

    // And the rest of the query...
    
    q += wxT(" ORDER BY ") + NumToStr((long)statusSortColumn) + wxT(" ") + statusSortOrder;
    wxString msgerror;
    pgSet *dataSet1 = connection->ExecuteSet(q,false);
    if (dataSet1)
    {
        msgerror = connection->GetLastError();
        statusBar->SetStatusText(_("Refreshing status list."));
        statusList->Freeze();

        // Clear the queries array content
        queries.Clear();
        wxString blocked=wxT("");
        int countsenders=0;
        long pidAV=-1;
        wxArrayLong pids;
        wxDateTime tt;
        while (!dataSet1->Eof())
        {
            pid = dataSet1->GetLong(wxT("pid"));
            wxString slinfo=wxEmptyString;
            wxString backend_xid;
            wxDateTime start_transaction;
            wxTimeSpan deltaidle;
            if (iswalsend) {
                slinfo = dataSet1->GetVal(wxT("slotinfo"));
            }
            if (!tt.IsValid()&&wait_sample && wait_enable) {
                tt = dataSet1->GetDateTime("tt");
                WS.BeginSeriosSample(tt.GetValue().GetValue());
                first_tt = dataSet1->GetVal("tt");

            }
            // Update the UI
            if (pid != backend_pid)
            {

                if (row >= statusList->GetItemCount())
                {
                    statusList->InsertItem(row, NumToStr(pid), -1);
                    row = statusList->GetItemCount() - 1;
                }
                else
                {
                    statusList->SetItem(row, 0, NumToStr(pid),-1);
                }

                wxString qry = dataSet1->GetVal(wxT("query"));
                wxString app_name = dataSet1->GetVal(wxT("application_name"));
                wxString backend_type;
                if (connection->BackendMinimumVersion(13, 0)) {
                    backend_type = dataSet1->GetVal(wxT("backend_type"));
                    if (app_name.IsEmpty() && backend_type != "client backend") app_name = backend_type;
                }
                if (connection->BackendMinimumVersion(9, 6))
                {
                    if (connection->BackendMinimumVersion(13, 0)) {
                        wxString progress_info = dataSet1->GetVal(wxT("progress_info"));
                        if (!progress_info.IsEmpty()) app_name = progress_info;
                    } else
                    {
                        wxString heap_blks_total = dataSet1->GetVal(wxT("heap_blks_total"));
                        if (!heap_blks_total.IsEmpty()) {
                            wxString heap_blks_vacuumed = dataSet1->GetVal(wxT("heap_blks_vacuumed"));
                            wxString heap_blks_scanned = dataSet1->GetVal(wxT("heap_blks_scanned"));
                            wxString phase = dataSet1->GetVal(wxT("phase"));
                            double total;
                            double vac = 0;
                            double proc;
                            heap_blks_vacuumed.ToDouble(&vac);
                            if (!phase.CmpNoCase(wxT("scanning heap"))) {
                                heap_blks_scanned.ToDouble(&vac);
                            }
                            heap_blks_total.ToDouble(&total);
                            proc = vac * 100 / total;
                            wxString str;
                            str.Printf(wxT("%s %5.2f%%"), phase, proc);
                            app_name = str;
                        }
                    }
                    if (!slinfo.IsEmpty()) app_name += slinfo;
                }
                int colpos = 1;
                if (connection->BackendMinimumVersion(8, 5))
                    statusList->SetItem(row, colpos++, app_name);
                statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("datname")));
                statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("usename")));

                if (connection->BackendMinimumVersion(8, 1))
                {
                    statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("client")));
                    statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("backend_start")));
                }
                if (connection->BackendMinimumVersion(7, 4))
                {
                    statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("query_start")));
                }

                if (connection->BackendMinimumVersion(8, 3)) {
                    start_transaction= dataSet1->GetDateTime("xact_start_full");
                    statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("xact_start")));
                    if (start_transaction.IsValid()) {
                        deltaidle = wxDateTime::Now() - start_transaction;
                    }
                }

                if (connection->BackendMinimumVersion(9, 2))
                {
                    statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("state")));
                    statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("state_change")));
                }

                if (connection->BackendMinimumVersion(9, 4))
                {
                    backend_xid= dataSet1->GetVal(wxT("backend_xid"));
                    statusList->SetItem(row, colpos++, backend_xid);
                    if (!slinfo.IsEmpty()) {
                         statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("xmin_slot")));
                         countsenders++;
                    }
                    else
                    {
                        wxString av_replica;
                        if (iswalsend) av_replica= dataSet1->GetVal(wxT("av_replica"));
                        if (av_replica.IsEmpty())
                            statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("backend_xmin")));
                        else {
                            statusList->SetItem(row, colpos++, av_replica);
                            pidAV=pid;
                        }
                            
                    }
                }
                if (connection->BackendMinimumVersion(9, 6))
                {
                    wait_event_type_col=colpos;
                    statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("wait_event_type")));
                    statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("wait_event")));
                }
                wxString blockedby = dataSet1->GetVal(wxT("blockedby"));
                blockedby=blockedby.substr(1, blockedby.Length() - 2);
                if (wait_sample && wait_enable) {
                    bool isClientReadTransaction = false;
                    //if ( spt.count>1 && ll!=0) isClientReadTransaction = true;
                    if (start_transaction.IsValid()
                        && start_transaction < tt
                        && !backend_xid.IsEmpty()
                        )isClientReadTransaction = true;

                    WS.AddSample(pid, isClientReadTransaction, backend_type, dataSet1->GetVal("wait_sample"));
                    //wxULongLong qid = dataSet1->GetLongLong("query_id");
                    //WS.AddQuery(qid.GetValue(), qry);
                }
                statusList->SetItem(row, colpos++, blockedby);
                if (!slinfo.IsEmpty()) {
                    statusList->SetItem(row, colpos, dataSet1->GetVal(wxT("xminlag")));
                } 
                else 
                    statusList->SetItem(row, colpos, qry);

                // Colorize the new line
                if (viewMenu->IsChecked(MNU_HIGHLIGHTSTATUS))
                {
                    statusList->SetItemBackgroundColour(row,
                                                        wxColour(settings->GetActiveProcessColour()));
                    if (qry == wxT("<IDLE>") || qry == wxT("<IDLE> in transaction0"))
                        statusList->SetItemBackgroundColour(row,
                                                            wxColour(settings->GetIdleProcessColour()));
                    if (connection->BackendMinimumVersion(9, 2))
                    {
                        if (dataSet1->GetVal(wxT("state")) != wxT("active"))
                            statusList->SetItemBackgroundColour(row,
                                                                wxColour(settings->GetIdleProcessColour()));
                    }

                    if (dataSet1->GetBool(wxT("slowquery")))
                        statusList->SetItemBackgroundColour(row,
                                                            wxColour(settings->GetSlowProcessColour()));
                    if (blockedby.Length() > 0) {
                        statusList->SetItemBackgroundColour(row,
                                                            wxColour(settings->GetBlockedProcessColour()));
                        blocked += blockedby;
                        blocked += wxT(",");
                    }
                    else if (deltaidle.GetSeconds() > idle_in_transaction_session_timeout) {
                        statusList->SetItemBackgroundColour(row,
                            wxColour(settings->GetIdle_in_transaction_session_timeoutProcessColour()));

                    }
                    if (!slinfo.IsEmpty()) {
                        // walsender
                        long xmindelta = dataSet1->GetLong(wxT("xminslotdelta"));
                        
                        if (xmindelta>=1800)
                            statusList->SetItemBackgroundColour(row,wxColour(wxT("#FF8028"))); // orange
                        else
                            statusList->SetItemBackgroundColour(row, wxColour(settings->GetIdleProcessColour())); // idle
                    }
                    if (app_name.StartsWith(wxT("pgp-s super"))||app_name.StartsWith(wxT("pgp-s manager"))) statusList->SetItemBackgroundColour(row, wxColour(settings->GetIdleProcessColour())); // idle

                }
                else
                    statusList->SetItemBackgroundColour(row, *wxWHITE);

                // filter apply
                bool flt = false;
                for (int i = 0; i < filterColumn.size(); i++) {
                    int col = filterColumn[i];
                    wxListItem listitem;
                    listitem.SetMask(wxLIST_MASK_TEXT);
                    statusList->GetColumn(col, listitem);
                    wxString label = listitem.GetText();
                    wxString tabval=statusList->GetItemText(row, col);
                    wxString fval = filterValue[i];
                    if (label == _("Client")) {
                        tabval = tabval.BeforeLast(':');
                        fval = fval.BeforeLast(':');
                    }
                    if (tabval != fval) {
                        flt = true;
                        break;
                    }
                }
                if (!flt) {
                    // Add the query content to the queries array
                    queries.Add(qry);
                    pids.Add(pid);
                    if (pid == s_pid_HIGHLIGH) {
                        statusList->SetItemBackgroundColour(row, wxColour(wxT("#FF8028")));
                        statusList->EnsureVisible(row);
                    }

                    row++;
                }
            }
            dataSet1->MoveNext();
        }
        delete dataSet1;
        if (viewMenu->IsChecked(MNU_HIGHLIGHTSTATUS))
        {
            if (countsenders == 0 && pidAV>0) {
                // all walsender active='f'
                for(long i = 0; i < pids.size(); i++)
                {
                    if (pids[i]==pidAV) 
                        statusList->SetItemBackgroundColour(i,
                                                            wxColour(settings->GetBlockedProcessColour()));
                }
            }

            // who blocking
            wxString numstr;
            wxString str;
            numstr=blocked.BeforeFirst(',',&str);
            while (!numstr.IsEmpty()) {
                int number = wxAtoi(numstr);
                for(long i = 0; i < pids.size(); i++)
                {
                    if (pids[i]==number) {
                        statusList->SetItemBackgroundColour(i,
                                                    wxColour(settings->GetBlockedbyProcessColour()));

                    }
                }
                blocked=str.Clone();
                numstr=blocked.BeforeFirst(',',&str);
            }
            if (onlyhightligth) {
                wxArrayString newquerys;
                long r = statusList->GetItemCount();
                //wxColour bg = *WHITE;
                wxColour bgidle=wxColour(settings->GetIdleProcessColour());
                long  rr = 0;
                int nquery = 0;
                //long selrow = -1;
                //selrow = statusList->GetFirstSelected();
                while ((rr) < statusList->GetItemCount() && rr<row) 
                    {
                        if (statusList->GetItemBackgroundColour(rr) != bgidle) {
                            newquerys.Add(queries[nquery]);
                            rr++;
                        }
                        else {
                            statusList->DeleteItem(rr);
                            row--;
                        }
                        nquery++;
                    }
                if (newquerys.Count() < queries.Count())  queries = newquerys;
            }

        }
        if (tt.IsValid() && wait_sample && wait_enable) {
            WS.EndSeriosSample();
        }
        bool selverify=true;
        long r = statusList->GetItemCount();
        long fuck_rows=0;
        while ((row) < statusList->GetItemCount()) {
            statusList->Select(row,false);
            long item = -1;
            item = statusList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
            if (item>=row && row>0) statusList->Focus(row-1);
            if (statusList->GetItemCount()>row) 
            {
                #ifdef __WXGTK__
                    //DeleteItem in GTK flicker problem
                    if (row+1 == r && filterColumn.size()>0) {
                        for(int cc=0;cc<statusList->GetColumnCount();cc++)
                            statusList->SetItem(row, cc, " ");
                        fuck_rows=1;
                        break;
                    } else statusList->DeleteItem(row);
                #else
                statusList->DeleteItem(row);
                #endif
            }
        }
        r = statusList->GetItemCount()-fuck_rows;
        wxString tit = _("Activity") + "(" + NumToStr(r) + ")";
        wxString old = manager.GetPane(wxT("Activity")).caption;
        if (tit != old) {
            manager.GetPane(wxT("Activity")).Caption(tit);
            manager.Update();
        }
        statusList->Thaw();
        if (wait_sample && wait_enable && top_small) top_small->Refresh();
        
        wxListEvent ev;
        //OnSelStatusItem(ev);
        wxString run_interval = ElapsedTimeToStr(
            wxGetLocalTimeMillis() - startTime);

        if (msgerror.length() > 0) {
            statusBar->SetStatusText(msgerror);
        }
        else
        {

            statusBar->SetStatusText(_("Done.")+ run_interval);
        }
    }
    else
        checkConnection();
}


void frmStatus::OnRefreshLocksTimer(wxTimerEvent &event)
{
    long pid = 0;

    if (! viewMenu->IsChecked(MNU_LOCKPAGE))
        return;

    checkConnection();
    if (!locks_connection)
    {
        statusTimer->Stop();
        locksTimer->Stop();
        if (xactTimer)
            xactTimer->Stop();
        if (logTimer)
            logTimer->Stop();
        if (querystateTimer)
            querystateTimer->Stop();
        return;
    }

    wxCriticalSectionLocker lock(gs_critsect);

    // There are no sort operator for xid before 8.3
    if (!connection->BackendMinimumVersion(8, 3) && lockSortColumn == 5)
    {
        wxLogError(_("You cannot sort by transaction id on your PostgreSQL release. You need at least 8.3."));
        lockSortColumn = 1;
    }

    long row = 0;
    wxString sql;
    if (locks_connection->BackendMinimumVersion(8, 3))
    {
        sql = wxT("SELECT pg_stat_get_backend_pid(svrid) AS pid, ")
              wxT("(SELECT datname FROM pg_database WHERE oid = pgl.database) AS dbname, ")
              wxT("coalesce(pgc.relname, pgl.relation::text) AS class, ")
              wxT("pg_get_userbyid(pg_stat_get_backend_userid(svrid)) as user, ")
              wxT("pgl.virtualxid::text, pgl.virtualtransaction::text AS transaction, pgl.mode, pgl.granted, ")
              wxT("date_trunc('second', pg_stat_get_backend_activity_start(svrid)) AS query_start, ")
              wxT("pg_stat_get_backend_activity(svrid) AS query ")
              wxT("FROM pg_stat_get_backend_idset() svrid, pg_locks pgl ")
              wxT("LEFT JOIN pg_class pgc ON pgl.relation=pgc.oid ")
              wxT("WHERE pgl.pid = pg_stat_get_backend_pid(svrid) ")
              wxT("ORDER BY ") + NumToStr((long)lockSortColumn) + wxT(" ") + lockSortOrder;
    }
    else if (locks_connection->BackendMinimumVersion(7, 4))
    {
        sql = wxT("SELECT pg_stat_get_backend_pid(svrid) AS pid, ")
              wxT("(SELECT datname FROM pg_database WHERE oid = pgl.database) AS dbname, ")
              wxT("coalesce(pgc.relname, pgl.relation::text) AS class, ")
              wxT("pg_get_userbyid(pg_stat_get_backend_userid(svrid)) as user, ")
              wxT("pgl.transaction, pgl.mode, pgl.granted, ")
              wxT("date_trunc('second', pg_stat_get_backend_activity_start(svrid)) AS query_start, ")
              wxT("pg_stat_get_backend_activity(svrid) AS query ")
              wxT("FROM pg_stat_get_backend_idset() svrid, pg_locks pgl ")
              wxT("LEFT JOIN pg_class pgc ON pgl.relation=pgc.oid ")
              wxT("WHERE pgl.pid = pg_stat_get_backend_pid(svrid) ")
              wxT("ORDER BY ") + NumToStr((long)lockSortColumn) + wxT(" ") + lockSortOrder;
    }
    else
    {
        sql = wxT("SELECT pg_stat_get_backend_pid(svrid) AS pid, ")
              wxT("(SELECT datname FROM pg_database WHERE oid = pgl.database) AS dbname, ")
              wxT("coalesce(pgc.relname, pgl.relation::text) AS class, ")
              wxT("pg_get_userbyid(pg_stat_get_backend_userid(svrid)) as user, ")
              wxT("pgl.transaction, pgl.mode, pgl.granted, ")
              wxT("pg_stat_get_backend_activity(svrid) AS query ")
              wxT("FROM pg_stat_get_backend_idset() svrid, pg_locks pgl ")
              wxT("LEFT JOIN pg_class pgc ON pgl.relation=pgc.oid ")
              wxT("WHERE pgl.pid = pg_stat_get_backend_pid(svrid) ")
              wxT("ORDER BY ") + NumToStr((long)lockSortColumn) + wxT(" ") + lockSortOrder;
    }

    pgSet *dataSet2 = locks_connection->ExecuteSet(sql);
    if (dataSet2)
    {
        statusBar->SetStatusText(_("Refreshing locks list."));
        lockList->Freeze();

        while (!dataSet2->Eof())
        {
            pid = dataSet2->GetLong(wxT("pid"));

            if (pid != backend_pid)
            {
                if (row >= lockList->GetItemCount())
                {
                    lockList->InsertItem(row, NumToStr(pid), -1);
                    row = lockList->GetItemCount() - 1;
                }
                else
                {
                    lockList->SetItem(row, 0, NumToStr(pid));
                }

                int colpos = 1;
                lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("dbname")));
                lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("class")));
                lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("user")));
                if (locks_connection->BackendMinimumVersion(8, 3))
                    lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("virtualxid")));
                lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("transaction")));
                lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("mode")));

                if (dataSet2->GetVal(wxT("granted")) == wxT("t"))
                {
                    lockList->SetItem(row, colpos++, _("Yes"));
                    lockList->SetItemBackgroundColour(row,lockList->GetBackgroundColour());
                }
                else {
                        lockList->SetItem(row, colpos++, _("No"));
                        lockList->SetItemBackgroundColour(row, wxColour(settings->GetBlockedProcessColour()));
                        
                    }

                wxString qry = dataSet2->GetVal(wxT("query"));

                if (locks_connection->BackendMinimumVersion(7, 4))
                {
                    if (qry.IsEmpty() || qry == wxT("<IDLE>"))
                        lockList->SetItem(row, colpos++, wxEmptyString);
                    else
                        lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("query_start")));
                }
                lockList->SetItem(row, colpos++, qry.Left(250));

                row++;
            }
            dataSet2->MoveNext();
        }

        delete dataSet2;

        while (row < lockList->GetItemCount())
            lockList->DeleteItem(row);

        lockList->Thaw();
        wxListEvent ev;
        //OnSelLockItem(ev);
        statusBar->SetStatusText(_("Done."));
    }
    else
        checkConnection();
}


void frmStatus::OnRefreshXactTimer(wxTimerEvent &event)
{
    if (! viewMenu->IsEnabled(MNU_XACTPAGE) || ! viewMenu->IsChecked(MNU_XACTPAGE) || !xactTimer)
        return;

    checkConnection();
    if (!connection)
    {
        statusTimer->Stop();
        locksTimer->Stop();
        xactTimer->Stop();
        querystateTimer->Stop();
        if (logTimer)
            logTimer->Stop();
        return;
    }

    wxCriticalSectionLocker lock(gs_critsect);

    // There are no sort operator for xid before 8.3
    if (!connection->BackendMinimumVersion(8, 3) && xactSortColumn == 1)
    {
        wxLogError(_("You cannot sort by transaction id on your PostgreSQL release. You need at least 8.3."));
        xactSortColumn = 2;
    }

    long row = 0;
    wxString sql;
    if (connection->BackendMinimumVersion(8, 3))
        sql = wxT("SELECT transaction::text, gid, prepared, owner, database ")
              wxT("FROM pg_prepared_xacts ")
              wxT("ORDER BY ") + NumToStr((long)xactSortColumn) + wxT(" ") + xactSortOrder;
    else
        sql = wxT("SELECT transaction, gid, prepared, owner, database ")
              wxT("FROM pg_prepared_xacts ")
              wxT("ORDER BY ") + NumToStr((long)xactSortColumn) + wxT(" ") + xactSortOrder;

    pgSet *dataSet3 = connection->ExecuteSet(sql);
    if (dataSet3)
    {
        statusBar->SetStatusText(_("Refreshing transactions list."));
        xactList->Freeze();

        while (!dataSet3->Eof())
        {
            long xid = dataSet3->GetLong(wxT("transaction"));

            if (row >= xactList->GetItemCount())
            {
                xactList->InsertItem(row, NumToStr(xid), -1);
                row = xactList->GetItemCount() - 1;
            }
            else
            {
                xactList->SetItem(row, 0, NumToStr(xid));
            }

            int colpos = 1;
            xactList->SetItem(row, colpos++, dataSet3->GetVal(wxT("gid")));
            xactList->SetItem(row, colpos++, dataSet3->GetVal(wxT("prepared")));
            xactList->SetItem(row, colpos++, dataSet3->GetVal(wxT("owner")));
            xactList->SetItem(row, colpos++, dataSet3->GetVal(wxT("database")));

            row++;
            dataSet3->MoveNext();
        }
        delete dataSet3;

        while (row < xactList->GetItemCount())
            xactList->DeleteItem(row);

        xactList->Thaw();
        wxListEvent ev;
        //OnSelXactItem(ev);
        statusBar->SetStatusText(_("Done."));
    }
    else
        checkConnection();
}
long frmStatus::getlongvalue(wxString source,wxString match_str) {
    long aa=0;
    wxRegEx foundstr(match_str);
    if (foundstr.Matches(source)) {
            wxString v=foundstr.GetMatch(source,1);
            v.ToLong(&aa);
    }
    return aa;
}
    

void frmStatus::OnRefreshQuerystateTimer(wxTimerEvent &event)
{
    if (! viewMenu->IsEnabled(MNU_QUERYSTATEPAGE) || ! viewMenu->IsChecked(MNU_QUERYSTATEPAGE) || !querystateTimer)
        return;

    checkConnection();
    if (!connection)
    {
        statusTimer->Stop();
        locksTimer->Stop();
        xactTimer->Stop();
        if (querystateTimer)
            querystateTimer->Stop();
        if (logTimer)
            logTimer->Stop();
        return;
    }
    int row = statusList->GetFirstSelected();
    if (row<0)
        return;
    wxString pid=statusList->GetText(row, 0);
    wxString dbname=statusList->GetText(row, 2); // dbname 
    wxString wait_event_type= statusList->GetText(row, wait_event_type_col);
    if (dbname.IsEmpty()||wait_event_type==wxT("Extension")) return;

    wxString flags=wxT("");
    if (viewMenu->IsChecked(MNU_QUERYSTATEVERBOSE)) 
        flags += wxT(",true,false");
    else
        flags += wxT(",false,false");
    if (viewMenu->IsChecked(MNU_QUERYSTATETIME)) 
        flags += wxT(",true");
    else
        flags += wxT(",false");
    if (viewMenu->IsChecked(MNU_QUERYSTATEBUFFER)) 
        flags += wxT(",true");
    else
        flags += wxT(",false");
    if (viewMenu->IsChecked(MNU_QUERYSTATETRIGGER)) 
        flags += wxT(",true");
    else
        flags += wxT(",false");

    flags += wxT(",'text'::text");
    wxCriticalSectionLocker lock(gs_critsect);

    row = 0;
    wxString sql;
        sql = wxT("select pid,frame_number,query_text,unnest(string_to_array(plan, E'\n')) pln,leader_pid from pg_query_state(")
              +pid+flags+wxT(") s");
    
    pgSet *dataSet3 = connection->ExecuteSet(sql,false);
    if (dataSet3)
    {
        statusBar->SetStatusText(_("Refreshing query state list."));
        querystateList->Freeze();
        long prev_fn=100000000;
        while (!dataSet3->Eof()&&dataSet3->NumCols()>0)
        {
            long pid = dataSet3->GetLong(wxT("pid"));

            if (row >= querystateList->GetItemCount())
            {
                querystateList->InsertItem(row, NumToStr(pid), -1);
                row = querystateList->GetItemCount() - 1;
            }
            else
            {
                querystateList->SetItem(row, 0, NumToStr(pid));
            }
            
            int colpos = 1;
            long fn=dataSet3->GetLong(wxT("frame_number"));
            querystateList->SetItem(row, colpos++, NumToStr(fn));
            querystateList->SetItem(row, colpos++, dataSet3->GetVal(wxT("query_text")));
            wxString p=dataSet3->GetVal(wxT("pln"));
            querystateList->SetItem(row, colpos++, p);
            querystateList->SetItem(row, colpos++, dataSet3->GetVal(wxT("leader_pid")));
            if (prev_fn==fn) {
                querystateList->SetItem(row, 1, wxT(""));
                querystateList->SetItem(row, 2, wxT(""));
            }
            wxColour wc;
            if (p.Find(wxT("->"))<0) {
                    wc=*wxWHITE;
                            if (getlongvalue(p,wxT("Rows Removed by Join Filter: ([0-9]+)"))>1000000) {
                                wc=wxColour(201,83,2);
                                //querystateList->SetItemBackgroundColour(row, wxColour(201,83,2));
                            }

                querystateList->SetItemBackgroundColour(row, wc);
                //querystateList->SetItemBackgroundColour(row, );
            } else
            {
                if (getlongvalue(p,wxT("actual rows=([0-9]+)"))>1000000) {
                    querystateList->SetItemBackgroundColour(row, wxColour(255,174,200)); // red
                } else
                    querystateList->SetItemBackgroundColour(row, wxColour(224,255,224)); // gren
            }

            row++;
            prev_fn=fn;
            dataSet3->MoveNext();
        }
        delete dataSet3;

        while (row < querystateList->GetItemCount())
            querystateList->DeleteItem(row);

        querystateList->Thaw();
        wxListEvent ev;
        //OnSelQuerystateItem(ev);
        statusBar->SetStatusText(_("Done."));
    }
    else
        checkConnection();
}


void frmStatus::OnRefreshLogTimer(wxTimerEvent &event)
{
    if (logisread) return;
    if (! viewMenu->IsEnabled(MNU_LOGPAGE) || ! viewMenu->IsChecked(MNU_LOGPAGE) || !logTimer)
        return;

    checkConnection();
    if (!connection)
    {
        statusTimer->Stop();
        locksTimer->Stop();
        if (xactTimer)
            xactTimer->Stop();
        logTimer->Stop();

        if (logThread) {
            logThread->BreakRead();
            wxMilliSleep(50);
            logThread->DoTerminate();
            wxMilliSleep(5);
            //s_CloseLog.Wait();
            while (logThread != NULL) wxMilliSleep(5);
        }
        return;
    }
    if (!logThread) {
        wxString applicationname = "pgAdmin III LogReader";
        logconn = new pgConn(connection->GetHostName(), connection->GetService(), connection->GetHostAddr(), connection->GetDbname(),
            connection->GetUser(), connection->GetPassword(), connection->GetPort(), connection->GetRole(), "", connection->GetSslMode(),
            0, applicationname, connection->GetSSLCert(), connection->GetSSLKey(), connection->GetSSLRootCert(), connection->GetSSLCrl(),
            connection->GetSSLCompression());
        
        if (!logconn->IsAlive()) {
            wxString err=logconn->GetLastError();
            //wxMessageBox(err);
            statusBar->SetStatusText(err);
            delete logconn;
            logconn = NULL;
            return;
        }
        else
        {
        }
        if (log_queue.IsOk()) log_queue.Clear();

        logThread = new ReadLogThread(logconn, this,&log_queue);
        if (logThread->Create() != wxTHREAD_NO_ERROR)
        {
            wxLogError(wxT("Can’t create log thread!"));
            delete logThread;
            logThread = NULL;
        } else
            logThread->Run();
    }
    else
        if (logThread) // The thread 
        {
            // Recive lines
            if (logThread->isBreak()) return;
            wxString sstr;
            long lastrow = logList->GetItemCount();
            bool remote = lastrow - 1 == logList->GetFocusedItem();
            long cnt = 0;
            float pr = 0;
            long timeout = logTimer->GetInterval();
            const wxMilliClock_t waitUntil = wxGetLocalTimeMillis() + timeout;
            logisread = true;
            if (!logList->IsFrozen()) logList->Freeze();
            while (logThread && log_queue.ReceiveTimeout(1, sstr) == wxMSGQUEUE_NO_ERROR) {
                if (logThread->isBreak()) break;
                    addLogLine(sstr, true, true);
                    if (logThread->isBreak()) break;
                if ((cnt++ % 1000) == 0) {
                        //pr = 100 * cnt / total;
                        wxString ll = wxString::Format("append rows %ld ...", cnt+ lastrow);
                        statusBar->SetStatusText(ll);
                        wxYield();
                        if (frm_exit) { logisread = false; return; }
                }
                //if (logThread->isBreak()) return;
                const wxMilliClock_t now = wxGetLocalTimeMillis();
                if (now >= waitUntil) break;

            };
            if (logList->IsFrozen())
            {
                logList->Thaw();
                nav->Refresh();
            }
            logisread = false;
            if (logThread && logThread->isBreak()) {
                return;
            }
            if (cnt > 0) {
                cnt += lastrow;
                //wxString ll = wxString::Format("append rows GUI %ld ...", cnt);
                //statusBar->SetStatusText(ll);
                addodd++;
                if (remote) {
                    logList->Focus(logList->GetItemCount() - 1);
                }
                logList->Refresh();
            }
            //statusBar->SetStatusText(s);
            if (logThread && logThread->isReadyRows()) {
                long read = logThread->GetReadByteFile(savedPartialLine);  // result last read
                logfileLength = read;
                // Ready next lines
            }
            else {
                return; /// not ready next lines
            }
        }
        
        
    
    wxCriticalSectionLocker lock(gs_critsect);
    if (logList->IsFrozen()) logList->Thaw();

    if (connection->GetLastResultError().sql_state == wxT("42501"))
    {
        // Don't have superuser privileges, so can't do anything with the log display
        logTimer->Stop();
        cbLogfiles->Disable();
        btnRotateLog->Disable();
        manager.GetPane(wxT("Logfile")).Show(false);
        manager.Update();
        return;
    }

    long newlen = 0;

    if (logDirectory.IsEmpty())
    {
        // freshly started
        logDirectory = connection->ExecuteScalar(wxT("SHOW log_directory"));
        if (connection->GetLastResultError().sql_state == wxT("42501"))
        {
            // Don't have superuser privileges, so can't do anything with the log display
            logTimer->Stop();
            cbLogfiles->Disable();
            btnRotateLog->Disable();
            manager.GetPane(wxT("Logfile")).Show(false);
            manager.Update();
            return;
        }
        if (fillLogfileCombo())
        {
            savedPartialLine.Clear();
            cbLogfiles->SetSelection(0);
            wxCommandEvent ev;
            OnLoadLogfile(ev);
            return;
        }
        else
        {
            //logDirectory = wxEmptyString;
            return;
            logDirectory = wxT("-");
            if (connection->BackendMinimumVersion(8, 3))
                logList->AppendItemLong(-1, wxString(_("logging_collector not enabled or log_filename misconfigured")));
            else
                logList->AppendItemLong(-1, wxString(_("redirect_stderr not enabled or log_filename misconfigured")));
            cbLogfiles->Disable();
            btnRotateLog->Disable();
        }
    }

    if (logDirectory == wxT("-"))
        return;

    if (isCurrent && logfileName.Len()>0)
    {
        // check if the current logfile changed
        
        pgSet* set;
        if ((connection->BackendMinimumVersion(10, 0)))
            set = connection->ExecuteSet(wxT("select size len from pg_stat_file(") + connection->qtDbString(logfileName) + wxT(",true)"),false);
            else 
            set = connection->ExecuteSet(wxT("SELECT pg_file_length(") + connection->qtDbString(logfileName) + wxT(") AS len"));
        if (set )
        {
            if (set->NumCols() == 0) {
                // error server
                // continue after
                wxString errtext=connection->GetLastError();
                statusBar->SetStatusText("Error db: "+ errtext);
                return;
            }
            if (set->NumCols()>0 && !set->IsNull(0)) newlen = set->GetLong(wxT("len")); 
            else {
                logDirectory = ""; // reRead directory
                logfileName = "";
                newlen = 0;
                showCurrent = true;
            }
            delete set;
        }
        else
        {
            checkConnection();
            logDirectory = ""; // reRead directory
            logfileName = "";
            return;
        }
        if (newlen > logfileLength)
        {

            statusBar->SetStatusText(_("Refreshing log list."));
            addLogFile(logfileName, logfileTimestamp, newlen, logfileLength, false);
            statusBar->SetStatusText(_("Wait..."));
            // as long as there was new data, the logfile is probably the current
            // one so we don't need to check for rotation
            return;
        }
    }

    //
    wxString newDirectory = connection->ExecuteScalar(wxT("SHOW log_directory"));

    int newfiles = 0;
    if (newDirectory != logDirectory)
        cbLogfiles->Clear();

    newfiles = fillLogfileCombo();

    if (newfiles)
    {
        if (!showCurrent)
            isCurrent = false;

        if (isCurrent)
        {
            wxCommandEvent ev;
            if (cbLogfiles->GetCount() > 0) {
                cbLogfiles->SetSelection(0);
                OnLoadLogfile(ev);
            }
            else {

                int pos = cbLogfiles->GetCount() - newfiles;
                bool skipFirst = true;

                while (newfiles--)
                {
                    addLogLine(_("pgadmin:Logfile rotated."), false);
                    wxDateTime* ts = (wxDateTime*)cbLogfiles->wxItemContainer::GetClientData(pos++);
                    wxASSERT(ts != 0);
                    if (logThread) logThread->BreakRead();
                    addLogFile(ts, skipFirst);
                    skipFirst = false;

                    pos++;
                }
            }
        }
    }
}


void frmStatus::OnRefresh(wxCommandEvent &event)
{
    wxTimerEvent evt;

    OnRefreshStatusTimer(evt);
    OnRefreshLocksTimer(evt);
    OnRefreshXactTimer(evt);
    OnRefreshLogTimer(evt);
    OnRefreshQuerystateTimer(evt);
}


void frmStatus::checkConnection()
{
    if (connection) {
        if (!locks_connection->IsAlive())
        {
            locks_connection = connection;
        }
        if (!connection->IsAlive())
        {
            if (locks_connection==connection) locks_connection = 0;
            delete connection;
            connection = 0;
            statusTimer->Stop();
            locksTimer->Stop();
            if (xactTimer)
                xactTimer->Stop();
            if (logTimer)
                logTimer->Stop();
            if (querystateTimer)
                querystateTimer->Stop();
            actionMenu->Enable(MNU_REFRESH, false);
            toolBar->EnableTool(MNU_REFRESH, false);
            statusBar->SetStatusText(_("Connection broken."));
            SetTitle(_("Connection broken."));
        }
    }
}


void frmStatus::addLogFile(wxDateTime *dt, bool skipFirst)
{
    pgSet* set;
    if (settings->GetASUTPstyle()) {
        wxString sql = "select current_setting('log_directory')||'/'||name filename,modification filetime,size len\n"
            "  FROM pg_ls_logdir()  where name ~ '.csv' and modification >= '" + DateToAnsiStr(*dt) + "'::timestamp order by modification-'" + DateToAnsiStr(*dt) + "'::timestamp limit 1";
        set = connection->ExecuteSet(sql);
    } else
        set = connection->ExecuteSet(
                     wxT("SELECT modification filetime, name filename, size AS len ")
                     wxT("  FROM pg_ls_logdir()")
                     wxT(" WHERE modification = '") + DateToAnsiStr(*dt) + wxT("'::timestamp"),false);
    if (set)
    {
        logfileName = set->GetVal(wxT("filename"));
        logfileTimestamp = set->GetDateTime(wxT("filetime"));
        long len = set->GetLong(wxT("len"));
        //if (logThread) logThread->BreakRead();
        logfileLength = 0;
        addLogFile(logfileName, logfileTimestamp, len, logfileLength, skipFirst);

        delete set;
    }
}


void frmStatus::addLogFile(const wxString &filename, const wxDateTime timestamp, long len, long &read, bool skipFirst)
{
    wxString line;

    if (skipFirst)
    {
        long maxServerLogSize = settings->GetMaxServerLogSize();

        if (!logfileLength && maxServerLogSize && logfileLength > maxServerLogSize)
        {
            long maxServerLogSize = settings->GetMaxServerLogSize();
            len = read - maxServerLogSize;
        }
        else
            skipFirst = false;
    }

    // If GPDB 3.3 and later, log is normally in CSV format.  Let's get a whole log line before calling addLogLine,
    // so we can do things smarter.

    // PostgreSQL can log in CSV format, as well as regular format.  Normally, we'd only see
    // the regular format logs here, because pg_logdir_ls only returns those.  But if pg_logdir_ls is
    // changed to return the csv format log files, we should handle it.

    bool csv_log_format = filename.Right(4) == wxT(".csv");
    
    if (csv_log_format && savedPartialLine.length() > 0)
    {
        if (read == 0)  // Starting at beginning of log file
            savedPartialLine.clear();
        else
            line = savedPartialLine;
    }
    if (logThread) {
        if (logThread->isReadyRows()) {
            logThread->SetParameters(filename,len,read,savedPartialLine);
            logThread->GoReadRows();
            return;
        }
    }
    wxString funcname = "pg_read_binary_file(";
    //if (!settings->GetASUTPstyle()) funcname = "pg_file_read(";
    wxString msg = _("Reading log from server...");
    while (len > read)
    {
#define PG_READ_BUFFER 500000
        float pr = 100.0 * read / len;
        statusBar->SetStatusText(wxString::Format("%s %.2f MB (%.1f %%)", msg, len / 1048576.0, pr));
        wxString readsql = wxString::Format("select %s%s,%s, %d,true)", funcname, connection->qtDbString(filename), NumToStr(read), PG_READ_BUFFER);
        pgSet *set = connection->ExecuteSet(readsql);
        if (!set)
        {
            connection->IsAlive();
            return;
        }
        wxSafeYield();
        char *raw1 = set->GetCharPtr(0);

        if (!raw1 || !*raw1)
        {
            delete set;
            break;
        }
        char* raw;
        unsigned char m[PG_READ_BUFFER + 1];
        if (settings->GetASUTPstyle()) {
            
            raw =( char *) &m[0];
            unsigned char c;
            unsigned char* startChar;
            int pos = 0;
            raw1 = raw1 + 2;
            int utf8charLen = 0;
            while (*raw1!=0) {
                c = *raw1;
                c = c - '0';
                if (c > 9) c = *raw1 - 'a' + 10;
                raw1++;
                m[pos] = c << 4;
                c = *raw1 - '0';
                if (c > 9) c = *raw1 - 'a' + 10;
                c = c | m[pos];
                m[pos] =  c;
                // check utf-8 char
                if (utf8charLen == 0) {
                    startChar = &m[pos];
                    if(c >> 7 == 0)
                        utf8charLen = 1;
                    else if (c >> 5 == 0x6)
                            utf8charLen = 2;
                    else if (c >> 4 == 0xE)
                            utf8charLen = 3;
                    else if (c >> 5 == 0x1E)
                            utf8charLen = 4;
                    else
                            utf8charLen=0;
                    // bad utf8 format
                }
                pos++;
                raw1++;
                utf8charLen--;
            }
            // 
            if (utf8charLen != 0) {
                //read = startChar - &m[0];
                // remove bad utf-8 char
                *startChar = 0;
            } else 
                m[pos] = 0;
        } else {
            raw = raw1;
        }
            read += strlen(raw);

            wxString str;
            str = line + wxTextBuffer::Translate(wxString(raw, set->GetConversion()), wxTextFileType_Unix);
            //if (wxString(wxString(raw, wxConvLibc).wx_str(), wxConvUTF8).Len() > 0)
            //	str = line + wxString(wxString(raw, wxConvLibc).wx_str(), wxConvUTF8);
            //else {
            //	str = line + wxTextBuffer::Translate(wxString(raw, set->GetConversion()), wxTextFileType_Unix);
            //}
        
#undef PG_READ_BUFFER
        delete set;

        if (str.Len() == 0)
        {
            wxString msgstr = _("The server log contains entries in multiple encodings and cannot be displayed by pgAdmin.");
            wxMessageBox(msgstr);
            return;
        }

        if (csv_log_format)
        {
            // This will work for any DB using CSV format logs

            if (logHasTimestamp)
            {
                // Right now, csv format logs from GPDB and PostgreSQL always start with a timestamp, so we count on that.

                // And the only reason we need to do that is to make sure we are in sync.

                // Bad things happen if we start in the middle of a
                // double-quoted string, as we would never find a correct line terminator!

                // In CSV logs, the first field must be a Timestamp, so must start with "2009" or "201" or "202" (at least for the next 20 years).
                if (str.length() > 4 && str.Left(4) != wxT("2009") && str.Left(3) != wxT("201") && str.Left(3) != wxT("202"))
                {
                    wxLogNotice(wxT("Log line does not start with timestamp: %s \n"), str.Mid(0, 100).c_str());
                    // Something isn't right, as we are not at the beginning of a csv log record.
                    // We should never get here, but if we do, try to handle it in a smart way.
                    str = str.Mid(str.Find(wxT("\n20")) + 1); // Try to re-sync.
                }
            }

            CSVLineTokenizer tk(str);

            logList->Freeze();

            while (tk.HasMoreLines())
            {
                line.Clear();

                bool partial;
                str = tk.GetNextLine(partial);
                if (partial)
                {
                    line = str; // Start of a log line, but not complete.  Loop back, Read more data.
                    break;
                }

                // Some extra debug checking, assuming csv logs line start with timestamps.
                // Not really necessary, but it is good for debugging if something isn't right.
                if (logHasTimestamp)
                {
                    // The first field must be a Timestamp, so must start with "2009" or "201" or "202" (at least for the next 20 years).
                    // This is just an extra check to make sure we haven't gotten out of sync with the log.
                    if (str.length() > 5 && str.Left(4) != wxT("2009") && str.Left(3) != wxT("201") && str.Left(3) != wxT("202"))
                    {
                        // BUG:  We are out of sync on the log
                        wxLogNotice(wxT("Log line does not start with timestamp: %s\n"), str.c_str());
                    }
                    else if (str.length() < 20)
                    {
                        // BUG:  We are out of sync on the log, or the log is garbled
                        wxLogNotice(wxT("Log line too short: %s\n"), str.c_str());
                    }
                }

                // Looks like we have a good complete CSV log record.
                addLogLine(str.Trim(), true, true);
            }

            logList->Thaw();
        }
        else
        {
            // Non-csv format log file

            bool hasCr = (str.Right(1) == wxT("\n"));

            wxStringTokenizer tk(str, wxT("\n"));

            logList->Freeze();

            while (tk.HasMoreTokens())
            {
                str = tk.GetNextToken();
                if (skipFirst)
                {
                    // could be truncated
                    skipFirst = false;
                    continue;
                }

                if (tk.HasMoreTokens() || hasCr)
                    addLogLine(str.Trim());
                else
                    line = str;
            }

            logList->Thaw();
        }
    }

    savedPartialLine.clear();

    if (!line.IsEmpty())
    {
        // We finished reading to the end of the log file, but still have some data left
        if (csv_log_format)
        {
            savedPartialLine = line;    // Save partial log line for next read of the data file.
            line.Clear();
        }
        else
            addLogLine(line.Trim());
    }

}


void frmStatus::addLogLine(const wxString &str, bool formatted, bool csv_log_format)
{
    int row = logList->GetItemCount();

    int idxTimeStampCol = -1, idxLevelCol = -1;
    int idxLogEntryCol = 0;

    if (logFormatKnown)
    {
        // Known Format first will be level, then Log entry
        // idxLevelCol : 0, idxLogEntryCol : 1, idxTimeStampCol : -1
        idxLevelCol++;
        idxLogEntryCol++;
        if (logHasTimestamp)
        {
            // idxLevelCol : 1, idxLogEntryCol : 2, idxTimeStampCol : 0
            idxTimeStampCol++;
            idxLevelCol++;
            idxLogEntryCol++;
        }
    }

    if (!logFormatKnown) {
        logList->AppendItemLong(-1, str);
        int colorindex = nav->TryMarkItem(row, str);
        if (colorindex>=0)
            logList->SetItemBackgroundColour(row, nav->GetColorByIndex(colorindex));
        else 
            logList->SetItemBackgroundColour(row, logcol[addodd % 2]);
    }
    else if ((!csv_log_format) && str.Find(':') < 0)
    {
        // Must be a continuation of a previous line.
        logList->InsertItem(row, wxEmptyString, -1);
        logList->SetItem(row, idxLogEntryCol, str);
    }
    else if (!formatted)
    {
        // Not from a log, from pgAdmin itself.
        if (logHasTimestamp)
        {
            logList->InsertItem(row, wxEmptyString, -1);
            logList->SetItem(row, idxLevelCol, str.BeforeFirst(':'));
        }
        else
        {
            logList->InsertItem(row, str.BeforeFirst(':'), -1);
        }
        logList->SetItem(row, idxLogEntryCol, str.AfterFirst(':'));
    }
    else // formatted log
    {
        if (csv_log_format)
        {
            // Log is in CSV format (GPDB 3.3 and later, or Postgres if only csv log enabled)
            // In this case, we are always supposed to have a complete log line in csv format in str when called.

            if (logHasTimestamp && (str.Length() < 20 || (logHasTimestamp && (str[0] != wxT('2') || str[1] != wxT('0')))))
            {
                // Log line too short or does not start with an expected timestamp...
                // Must be a continuation of the previous line or garbage,
                // or we are out of sync in our CSV handling.
                // We shouldn't ever get here.
                logList->InsertItem(row, wxEmptyString, -1);
                logList->SetItem(row, 2, str);
            }
            else
            {
                CSVTokenizer tk(str);

                bool gpdb = connection->GetIsGreenplum();

                // Get the fields from the CSV log.
                wxString logTime = tk.GetNextToken();
                wxString logUser = tk.GetNextToken();
                wxString logDatabase = tk.GetNextToken();
                wxString logPid = tk.GetNextToken();

                wxString logSession;
                wxString logCmdcount;
                wxString logSegment;

                if (gpdb)
                {
                    wxString logThread =  tk.GetNextToken();        // GPDB specific
                    wxString logHost = tk.GetNextToken();
                    wxString logPort = tk.GetNextToken();           // GPDB (Postgres puts port with Host)
                    wxString logSessiontime = tk.GetNextToken();
                    wxString logTransaction = tk.GetNextToken();
                    logSession = tk.GetNextToken();
                    logCmdcount = tk.GetNextToken();
                    logSegment = tk.GetNextToken();
                    wxString logSlice = tk.GetNextToken();
                    wxString logDistxact = tk.GetNextToken();
                    wxString logLocalxact = tk.GetNextToken();
                    wxString logSubxact = tk.GetNextToken();
                }
                else
                {
                    wxString logHost = tk.GetNextToken();       // Postgres puts port with Hostname
                    logSession = tk.GetNextToken();
                    wxString logLineNumber = tk.GetNextToken();
                    wxString logPsDisplay = tk.GetNextToken();
                    wxString logSessiontime = tk.GetNextToken();
                    wxString logVXid = tk.GetNextToken();
                    wxString logTransaction = tk.GetNextToken();
                }

                wxString logSeverity = tk.GetNextToken();
                wxString logState = tk.GetNextToken();
                wxString logMessage = tk.GetNextToken();
                wxString logDetail = tk.GetNextToken();
                wxString logHint = tk.GetNextToken();
                wxString logQuery = tk.GetNextToken();
                wxString logQuerypos = tk.GetNextToken();
                wxString logContext = tk.GetNextToken();
                wxString logDebug = tk.GetNextToken();
                wxString logCursorpos = tk.GetNextToken();

                wxString logStack;
                if (gpdb)
                {
                    wxString logFunction = tk.GetNextToken();       // GPDB.  Postgres puts func, file, and line together
                    wxString logFile = tk.GetNextToken();
                    wxString logLine = tk.GetNextToken();
                    logStack = tk.GetNextToken();                   // GPDB only.
                }
                else
                    wxString logFuncFileLine = tk.GetNextToken();

                logList->InsertItem(row, logTime, -1);      // Insert timestamp (with time zone)

                logList->SetItem(row, 1, logSeverity);

                // Display the logMessage, breaking it into lines
                wxStringTokenizer lm(logMessage, wxT("\n"));
                logList->SetItem(row, 2, lm.GetNextToken());

                logList->SetItem(row, 3, logSession);
                logList->SetItem(row, 4, logCmdcount);
                logList->SetItem(row, 5, logDatabase);
                if ((!gpdb) || (logSegment.length() > 0 && logSegment != wxT("seg-1")))
                {
                    logList->SetItem(row, 6, logSegment);
                }
                else
                {
                    // If we are reading the masterDB log only, the logSegment won't
                    // have anything useful in it.  Look in the logMessage, and see if the
                    // segment info exists in there.  It will always be at the end.
                    if (logMessage.length() > 0 && logMessage[logMessage.length() - 1] == wxT(')'))
                    {
                        int segpos = -1;
                        segpos = logMessage.Find(wxT("(seg"));
                        if (segpos <= 0)
                            segpos = logMessage.Find(wxT("(mir"));
                        if (segpos > 0)
                        {
                            logSegment = logMessage.Mid(segpos + 1);
                            if (logSegment.Find(wxT(' ')) > 0)
                                logSegment = logSegment.Mid(0, logSegment.Find(wxT(' ')));
                            logList->SetItem(row, 6, logSegment);
                        }
                    }
                }

                // The rest of the lines from the logMessage
                while (lm.HasMoreTokens())
                {
                    int controw = logList->GetItemCount();
                    logList->InsertItem(controw, wxEmptyString, -1);
                    logList->SetItem(controw, 2, lm.GetNextToken());
                }

                // Add the detail
                wxStringTokenizer ld(logDetail, wxT("\n"));
                while (ld.HasMoreTokens())
                {
                    int controw = logList->GetItemCount();
                    logList->InsertItem(controw, wxEmptyString, -1);
                    logList->SetItem(controw, 2, ld.GetNextToken());
                }

                // And the hint
                wxStringTokenizer lh(logHint, wxT("\n"));
                while (lh.HasMoreTokens())
                {
                    int controw = logList->GetItemCount();
                    logList->InsertItem(controw, wxEmptyString, -1);
                    logList->SetItem(controw, 2, lh.GetNextToken());
                }

                if (logDebug.length() > 0)
                {
                    wxString logState3 = logState.Mid(0, 3);
                    if (logState3 == wxT("426") || logState3 == wxT("22P") || logState3 == wxT("427")
                            || logState3 == wxT("42P") || logState3 == wxT("458")
                            || logMessage.Mid(0, 9) == wxT("duration:") || logSeverity == wxT("FATAL") || logSeverity == wxT("PANIC"))
                    {
                        // If not redundant, add the statement from the debug_string
                        wxStringTokenizer lh(logDebug, wxT("\n"));
                        if (lh.HasMoreTokens())
                        {
                            int controw = logList->GetItemCount();
                            logList->InsertItem(controw, wxEmptyString, -1);
                            logList->SetItem(controw, 2, wxT("statement: ") + lh.GetNextToken());
                        }
                        while (lh.HasMoreTokens())
                        {
                            int controw = logList->GetItemCount();
                            logList->InsertItem(controw, wxEmptyString, -1);
                            logList->SetItem(controw, 2, lh.GetNextToken());
                        }
                    }
                }

                if (gpdb)
                    if (logSeverity == wxT("PANIC") ||
                            (logSeverity == wxT("FATAL") && logState != wxT("57P03") && logState != wxT("53300")))
                    {
                        // If this is a severe error, add the stack trace.
                        wxStringTokenizer ls(logStack, wxT("\n"));
                        if (ls.HasMoreTokens())
                        {
                            int controw = logList->GetItemCount();
                            logList->InsertItem(controw, wxEmptyString, -1);
                            logList->SetItem(controw, 1, wxT("STACK"));
                            logList->SetItem(controw, 2, ls.GetNextToken());
                        }
                        while (ls.HasMoreTokens())
                        {
                            int controw = logList->GetItemCount();
                            logList->InsertItem(controw, wxEmptyString, -1);
                            logList->SetItem(controw, 2, ls.GetNextToken());
                        }
                    }
            }
        }
        else if (connection->GetIsGreenplum())
        {
            // Greenplum 3.2 and before.  log_line_prefix =  "%m|%u|%d|%p|%I|%X|:-"

            wxString logSeverity;
            // Skip prefix, get message.  In GPDB, always follows ":-".
            wxString rest = str.Mid(str.Find(wxT(":-")) + 1) ;
            if (rest.Length() > 0 && rest[0] == wxT('-'))
                rest = rest.Mid(1);

            // Separate loglevel from message

            if (rest.Length() > 1 && rest[0] != wxT(' ') && rest.Find(':') > 0)
            {
                logSeverity = rest.BeforeFirst(':');
                rest = rest.AfterFirst(':').Mid(2);
            }

            wxString ts = str.BeforeFirst(logFormat.c_str()[logFmtPos + 2]);
            if (ts.Length() < 20  || (logHasTimestamp && (ts.Left(2) != wxT("20") || str.Find(':') < 0)))
            {
                // No Timestamp?  Must be a continuation of a previous line?
                // Not sure if it is possible to get here.
                logList->InsertItem(row, wxEmptyString, -1);
                logList->SetItem(row, 2, rest);
            }
            else if (logSeverity.Length() > 1)
            {
                // Normal case:  Start of a new log record.
                logList->InsertItem(row, ts, -1);
                logList->SetItem(row, 1, logSeverity);
                logList->SetItem(row, 2, rest);
            }
            else
            {
                // Continuation of previous line
                logList->InsertItem(row, wxEmptyString, -1);
                logList->SetItem(row, 2, rest);
            }
        }
        else
        {
            // All Non-csv-format non-GPDB PostgreSQL systems.

            wxString rest;

            if (logHasTimestamp)
            {
                if (formatted)
                {
                    rest = str.Mid(logFmtPos + 22).AfterFirst(':');
                    wxString ts = str.Mid(logFmtPos, str.Length() - rest.Length() - logFmtPos - 1);

                    int pos = ts.Find(logFormat.c_str()[logFmtPos + 2], true);
                    logList->InsertItem(row, ts.Left(pos), -1);
                    logList->SetItem(row, idxLevelCol, ts.Mid(pos + logFormat.Length() - logFmtPos - 2));
                    logList->SetItem(row, idxLogEntryCol, rest.Mid(2));
                }
                else
                {
                    logList->InsertItem(row, wxEmptyString, -1);
                    logList->SetItem(row, idxLevelCol, str.BeforeFirst(':'));
                    logList->SetItem(row, idxLogEntryCol, str.AfterFirst(':').Mid(2));
                }
            }
            else
            {
                if (formatted)
                    rest = str.Mid(logFormat.Length());
                else
                    rest = str;

                int pos = rest.Find(':');

                if (pos < 0) {
                    logList->InsertItem(row, rest, -1);
                }
                else
                {
                    logList->InsertItem(row, rest.BeforeFirst(':'), -1);
                    logList->SetItem(row, idxLogEntryCol, rest.AfterFirst(':').Mid(2));
                }
            }
        }
    }
}

void frmStatus::OnLogKeyUp(wxKeyEvent& event)
{
    if (currentPane == PANE_LOG) {
        if (nav->RunKeyCommand(event)) return;
    }
    event.Skip();
}

void frmStatus::emptyLogfileCombo()
{
    if (cbLogfiles->GetCount()) // first entry has no client data
        cbLogfiles->Delete(0);

    while (cbLogfiles->GetCount())
    {
        wxDateTime *dt = (wxDateTime *)cbLogfiles->wxItemContainer::GetClientData(0);
        if (dt)
            delete dt;
        cbLogfiles->Delete(0);
    }
}


int frmStatus::fillLogfileCombo()
{
    int count = cbLogfiles->GetCount();
    if (!count)
        cbLogfiles->Append(_("Current log"));
    else
        count--;
    pgSet* set;
    if (settings->GetASUTPstyle())
        set = connection->ExecuteSet(
        wxT("select name filename,modification filetime\n")
        wxT("  FROM pg_ls_logdir()  where name ~ '.csv'\n")
        wxT(" ORDER BY modification DESC"),false);

    else set = connection->ExecuteSet(
           wxT("SELECT name filename,modification filetime\n")
           wxT("  FROM pg_ls_logdir()\n")
           wxT(" ORDER BY filetime DESC"),false);


    if (set)
    {
        if (set->NumRows() <= count)
        {
            delete set;
            return 0;
        }

        set->Locate(count + 1);
        count = 0;

        while (!set->Eof())
        {
            count++;
            wxString fn = set->GetVal(wxT("filename"));
            wxDateTime ts = set->GetDateTime(wxT("filetime"));

            cbLogfiles->Append(DateToAnsiStr(ts), (void *)new wxDateTime(ts));

            set->MoveNext();
        }

        delete set;
    }
    if (count > 0) {
        unsigned int i;
        int maxWidth(0), width;
        for (i = 0; i < cbLogfiles->GetCount(); i++)
        {
            cbLogfiles->GetTextExtent(cbLogfiles->GetString(i), &width, NULL);
            if (width > maxWidth)
                maxWidth = width;
        }

        //cbLogfiles->SetMinSize(wxSize(maxWidth + 2, -1));
        //int sz = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, cbLogfiles);
        int sz = 24;
        cbLogfiles->SetSize(wxSize(maxWidth + sz, -1));
        toolBar->Realize();
        
        wxSize newszT = toolBar->GetBestSize();
        manager.GetPane(wxT("toolBar")).BestSize(newszT);
        //manager.GetPane(wxT("toolBar")).MinSize(newszT);
        manager.Update();
    }
    return count;
}


void frmStatus::OnLoadLogfile(wxCommandEvent &event)
{
    int pos = cbLogfiles->GetCurrentSelection();
    if (pos >= 0)
    {
        showCurrent = (pos == 0);
        isCurrent = showCurrent || (pos == 1);

        wxDateTime *ts = (wxDateTime *)cbLogfiles->wxItemContainer::GetClientData(showCurrent ? 1 : pos);
        wxASSERT(ts != 0);

        if (ts != NULL && (!logfileTimestamp.IsValid() || *ts != logfileTimestamp))
        {
            if (logThread) logThread->BreakRead();

            logList->DeleteAllItemsWithLong();
            nav->ClearMark();
            //bgColor = wxColour("#afafaf");
            //bgColor = logList->GetBackgroundColour();
            addodd = 0;
            addLogFile(ts, true);
            nav->Refresh();
        }
    }
}
ReadLogThread::~ReadLogThread() {
    //s_CloseLog.Post();
    ((frmStatus *) theParent)->logThread = NULL;
}
void ReadLogThread::BreakRead() {
    if (!isReadyRows()) {
        m_break = true;
        while (!isReadyRows()) {
            wxMilliSleep(20);
        }
    }
    log_queue->Clear();
}
void ReadLogThread::DoTerminate() {
     m_exit = true;
     if (isReadyRows()) {
         s_goReadLog.Post();
     }
     return; 
}

void* ReadLogThread::Entry() {
    int size_rows = 0;
    while (!m_exit)
    {
        {
            wxMutexLocker lock(s_mutexDBLogReading);
            //s_mutexDBLogReading.Lock();
            //rows.clear();
            getFilename();
        }
        if (m_exit) break;
        s_goReadLog.Wait();
    }
    return NULL;
}
void ReadLogThread::getFilename() {
    pgSet* set;
    if (logfileName.length() == 0) return;
    if (m_exit || m_break) return;
        if (!conn->IsAlive()) {
            wxDateTime n = wxDateTime::Now();
            if (!nextrun.IsValid() || nextrun < n) {
                if (!conn->Reconnect(false))
                {
                    wxTimeSpan sp(0, 2);
                    nextrun = wxDateTime::Now() + sp;
                }
            }
            return;
        }
        readLogFile(logfileName, len, logfileLength, savedPartialLine);
    
    //if (namepage.IsEmpty()) namepage = "not connect";
    //if (m_notebook->GetPageText(0) != namepage) m_notebook->SetPageText(0, namepage);
}
void ReadLogThread::readLogFile(wxString logfileName, long& lenfile, long& logfileLength, wxString& savedPartialLine) {
    wxString line;

    // If GPDB 3.3 and later, log is normally in CSV format.  Let's get a whole log line before calling addLogLine,
    // so we can do things smarter.

    // PostgreSQL can log in CSV format, as well as regular format.  Normally, we'd only see
    // the regular format logs here, because pg_logdir_ls only returns those.  But if pg_logdir_ls is
    // changed to return the csv format log files, we should handle it.

    bool csv_log_format = logfileName.Right(4) == wxT(".csv");
    if (csv_log_format && savedPartialLine.length() > 0)
    {
        if (logfileLength == 0)  // Starting at beginning of log file
            savedPartialLine.clear();
        line = savedPartialLine;
    }
    wxString funcname = "pg_read_binary_file(";
    int countLine = 0;
    long logf = logfileLength;
    while (lenfile > logfileLength)
    {
        //statusBar->SetStatusText(_("Reading log from server..."));
        if (m_exit) return;

#define PG_READ_BUFFER 500000
        wxString readsql = wxString::Format("select %s%s,%s, %d,true)", funcname, conn->qtDbString(logfileName), NumToStr(logfileLength), PG_READ_BUFFER);
        pgSet* set = conn->ExecuteSet(readsql,false);
        if (!set)
        {
            conn->IsAlive();
            return;
        }
        if (set->NumRows() == 0 || set->NumCols() ==0 || set->GetVal(0).IsNull()) {
            delete set;
            break;
        }
        char* raw1 = set->GetCharPtr(0);
        if (!raw1 || !*raw1)
        {
                delete set;
                break;
        }
        char* rawbuff;
        char m[PG_READ_BUFFER + 1];
        if (true) {

            rawbuff = &m[0];
            unsigned char c;
            unsigned char* startChar;
            unsigned char* startLine= (unsigned char*)&m[0];
            int pos = 0;
            raw1 = raw1 + 2; // bytea data \x01cf23 ....
            int utf8charLen = 0;
            unsigned int unichar = 0;
            unsigned int unicharcurrent = 0;
            int shift;
            while (*raw1 != 0) {
                if (m_exit) return;
                if (m_break) {
                    log_queue->Clear();
                    return;
                }
                c = *raw1;
                c = c - '0';
                if (c > 9) c = *raw1 - 'a' + 10;
                raw1++;
                m[pos] = c << 4;
                c = *raw1 - '0';
                if (c > 9) c = *raw1 - 'a' + 10;
                c = c | m[pos]; // hex string -> byte
                m[pos] = c;
                // check utf-8 char
                if (utf8charLen == 0) {
                    startChar = (unsigned char*) &m[pos];

                    if (c >> 7 == 0)
                        utf8charLen = 1;
                    else if (c >> 5 == 0x6)
                        utf8charLen = 2;
                    else if (c >> 4 == 0xE)
                        utf8charLen = 3;
                    else if (c >> 5 == 0x1E)
                        utf8charLen = 4;
                    else
                        utf8charLen = 0;
                    // bad utf8 format
                    shift = 0;
                    unichar = c;
                }
                unicharcurrent = c << shift;
                shift += 8;
                unichar = unichar  | unicharcurrent;
                pos++;
                raw1++;
                utf8charLen--;
                if (utf8charLen == 0) {
                    // utf-8 char complite
                    if (unichar == '\"' && csv_log_format)
                        inquote = !inquote;
                    //wxString linebuff(startChar, lencsvstr);
                    if (unichar == '\n' && !inquote) {
                        // end line
                        size_t lencsvstr = (unsigned char*)&m[pos] - startLine;
                        wxString linebuff(startLine, set->GetConversion(),lencsvstr); // include '\n'
                        if (line.length() > 0) {
                            linebuff = line + linebuff;
                            line.clear();
                        }
                        startLine = (unsigned char*)&m[pos];
                        //rows.push_back(linebuff.Trim());
                        log_queue->Post(linebuff.Trim());
                        countLine++;
                    }

                }
            }
            // 
            m[pos] = 0;
            if (utf8charLen != 0) {
                //read = startChar - &m[0];
                // remove bad utf-8 char
                *startChar = 0;
                pos = startChar- (unsigned char*)&m[0];
                // start position bad utf-8 char
            } else 
                m[pos] = 0;
            if (startLine != (unsigned char*)&m[pos]) {
                // partial line
                size_t lencsvstr = (unsigned char*)&m[pos] - startLine;
                wxString linebuff(startLine, lencsvstr); // include '\n'
                if (line.length() > 0) {
                    linebuff = line + linebuff;
                    line.clear();
                }
                line = linebuff;
            }
        }
        else {
            rawbuff = raw1;
        }
        int l = strlen(rawbuff);
        logfileLength += l;
        //status->SetLabelText(wxString::Format("%s Load bytes %ld", host, logfileLength));
        
        wxString str;
        //str = line + wxTextBuffer::Translate(wxString(rawbuff, set->GetConversion()), wxTextFileType_Unix);
        sendText(wxString::Format("@Load log line %d ...", countLine));
#undef PG_READ_BUFFER
        delete set;

        if (countLine == 0)
        {
            wxString msgstr = "@ The server log contains entries in multiple encodings and cannot be displayed by pgAdmin.";
            //wxMessageBox(msgstr);
            sendText(msgstr);
            return;
        }

    } 

    savedPartialLine.clear();

    if (!line.IsEmpty())
    {
        // We finished reading to the end of the log file, but still have some data left
        if (csv_log_format)
        {
            savedPartialLine = line;    // Save partial log line for next read of the data file.
            line.Clear();
        }
        else
            //my_view->AddRow(line.Trim());
            log_queue->Post(line.Trim());
            //rows.push_back(line.Trim());

    }
    if (logf==0) sendText(wxString::Format("Done %d", countLine));
}

void frmStatus::OnAddLabelTextThread(wxThreadEvent& event) {
    wxString s = event.GetString();
    if (s[0] == '@') {
        s = s.substr(1);
        statusBar->SetStatusText(s);
        return;
    } 
    {
        wxTimerEvent event;
        OnRefreshLogTimer(event);
    }
    
}
/////////////////////////////// 

void frmStatus::OnRotateLogfile(wxCommandEvent &event)
{
    if (wxMessageBox(_("Are you sure the logfile should be rotated?"), _("Logfile rotation"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) == wxYES)
        connection->ExecuteVoid(wxT("select pg_logfile_rotate()"));
}


void frmStatus::OnCancelBtn(wxCommandEvent &event)
{
    switch(currentPane)
    {
        case PANE_STATUS:
            OnStatusCancelBtn(event);
            break;
        case PANE_LOCKS:
            OnLocksCancelBtn(event);
            break;
        default:
            // This shouldn't happen. If it does, it's no big deal
            break;
    }
}


void frmStatus::OnStatusCancelBtn(wxCommandEvent &event)
{
    long item = statusList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to cancel the selected query(s)?"), _("Cancel query?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) != wxYES)
        return;

    while  (item >= 0)
    {
        wxString pid = statusList->GetItemText(item);
        wxString sql = wxT("SELECT pg_cancel_backend(") + pid + wxT(");");
        connection->ExecuteScalar(sql);

        item = statusList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    wxMessageBox(_("A cancel signal was sent to the selected server process(es)."), _("Cancel query"), wxOK | wxICON_INFORMATION);
    OnRefresh(event);
    wxListEvent ev;
    OnSelStatusItem(ev);
}


void frmStatus::OnLocksCancelBtn(wxCommandEvent &event)
{
    long item = lockList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to cancel the selected query(s)?"), _("Cancel query?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) != wxYES)
        return;

    while  (item >= 0)
    {
        wxString pid = lockList->GetItemText(item);
        wxString sql = wxT("SELECT pg_cancel_backend(") + pid + wxT(");");
        connection->ExecuteScalar(sql);

        item = lockList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    wxMessageBox(_("A cancel signal was sent to the selected server process(es)."), _("Cancel query"), wxOK | wxICON_INFORMATION);
    OnRefresh(event);
    wxListEvent ev;
    OnSelLockItem(ev);
}


void frmStatus::OnTerminateBtn(wxCommandEvent &event)
{
    switch(currentPane)
    {
        case PANE_STATUS:
            OnStatusTerminateBtn(event);
            break;
        case PANE_LOCKS:
            OnLocksTerminateBtn(event);
            break;
        default:
            // This shouldn't happen. If it does, it's no big deal
            break;
    }
}


void frmStatus::OnStatusTerminateBtn(wxCommandEvent &event)
{
    long item = statusList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to terminate the selected server process(es)?"), _("Terminate process?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) != wxYES)
        return;

    while  (item >= 0)
    {
        wxString pid = statusList->GetItemText(item);
        wxString sql = wxT("SELECT pg_terminate_backend(") + pid + wxT(");");
        connection->ExecuteScalar(sql);

        item = statusList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    wxMessageBox(_("A terminate signal was sent to the selected server process(es)."), _("Terminate process"), wxOK | wxICON_INFORMATION);
    OnRefresh(event);
    wxListEvent ev;
    OnSelStatusItem(ev);
}


void frmStatus::OnLocksTerminateBtn(wxCommandEvent &event)
{
    long item = lockList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to terminate the selected server process(es)?"), _("Terminate process?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) != wxYES)
        return;

    while  (item >= 0)
    {
        wxString pid = lockList->GetItemText(item);
        wxString sql = wxT("SELECT pg_terminate_backend(") + pid + wxT(");");
        connection->ExecuteScalar(sql);

        item = lockList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    wxMessageBox(_("A terminate signal was sent to the selected server process(es)."), _("Terminate process"), wxOK | wxICON_INFORMATION);
    OnRefresh(event);
    wxListEvent ev;
    OnSelLockItem(ev);
}


void frmStatus::OnStatusMenu(wxCommandEvent &event)
{
    wxListItem column;
    column.SetMask(wxLIST_MASK_TEXT);

    for (unsigned int i = 0; i < statusPopupMenu->GetMenuItemCount(); i++)
    {
        // Save column's width in a variable so that we can restore the old width
        // if we make this column "invisible"
        int currwidth= statusList->GetColumnWidth(i);
        if (statusList->GetColumnWidth(i) > 0)
            statusColWidth[i] = statusList->GetColumnWidth(i);

        wxMenuItem *menu = statusPopupMenu->FindItemByPosition(i);
        if (menu && menu->IsChecked())
            statusList->SetColumnWidth(i, statusColWidth[i]);
        else if (statusList->GetColumnWidth(i) > 0)
            statusList->SetColumnWidth(i, 0);

        // Save current width to restore it at next launch
        statusList->GetColumn(i, column);
        if (currwidth > 0)
            settings->WriteInt(wxT("frmStatus/StatusPane_") + column.GetText() + wxT("_Width"),
                               statusColWidth[i]);
        else
            settings->WriteInt(wxT("frmStatus/StatusPane_") + column.GetText() + wxT("_Width"),
                               -statusColWidth[i]);
    }
}


void frmStatus::OnLockMenu(wxCommandEvent &event)
{
    wxListItem column;
    column.SetMask(wxLIST_MASK_TEXT);

    for (unsigned int i = 0; i < lockPopupMenu->GetMenuItemCount(); i++)
    {
        // Save column's width in a variable so that we can restore the old width
        // if we make this column "invisible"
        int currwidth = lockList->GetColumnWidth(i);
        if (lockList->GetColumnWidth(i) > 0)
            lockColWidth[i] = lockList->GetColumnWidth(i);

        wxMenuItem *menu = lockPopupMenu->FindItemByPosition(i);
        if (menu && menu->IsChecked())
            lockList->SetColumnWidth(i, lockColWidth[i]);
        else if (lockList->GetColumnWidth(i) > 0)
            lockList->SetColumnWidth(i, 0);

        // Save current width to restore it at next launch
        lockList->GetColumn(i, column);
        if (currwidth > 0)
            settings->WriteInt(wxT("frmStatus/LockPane_") + column.GetText() + wxT("_Width"),
                               lockColWidth[i]);
        else
            settings->WriteInt(wxT("frmStatus/LockPane_") + column.GetText() + wxT("_Width"),
                               -lockColWidth[i]);
    }
}


void frmStatus::OnXactMenu(wxCommandEvent &event)
{
    wxListItem column;
    column.SetMask(wxLIST_MASK_TEXT);

    for (unsigned int i = 0; i < xactPopupMenu->GetMenuItemCount(); i++)
    {
        // Save column's width in a variable so that we can restore the old width
        // if we make this column "invisible"
        int currwidth = xactList->GetColumnWidth(i);
        if (xactList->GetColumnWidth(i) > 0)
            xactColWidth[i] = xactList->GetColumnWidth(i);

        wxMenuItem *menu = xactPopupMenu->FindItemByPosition(i);
        if (menu && menu->IsChecked())
            xactList->SetColumnWidth(i, xactColWidth[i]);
        else if (xactList->GetColumnWidth(i) > 0)
            xactList->SetColumnWidth(i, 0);

        // Save current width to restore it at next launch
        xactList->GetColumn(i, column);
        if (currwidth > 0)
            settings->WriteInt(wxT("frmStatus/XactPane_") + column.GetText() + wxT("_Width"),
                               xactColWidth[i]);
        else
            settings->WriteInt(wxT("frmStatus/XactPane_") + column.GetText() + wxT("_Width"),
                               -xactColWidth[i]);
    }
}
void frmStatus::OnQuerystateMenu(wxCommandEvent &event)
{
    wxListItem column;
    column.SetMask(wxLIST_MASK_TEXT);

    for (unsigned int i = 0; i < querystatePopupMenu->GetMenuItemCount(); i++)
    {
        // Save column's width in a variable so that we can restore the old width
        // if we make this column "invisible"
        int currwidth=querystateList->GetColumnWidth(i);
        if (querystateList->GetColumnWidth(i) > 0)
            querystateColWidth[i] = querystateList->GetColumnWidth(i);

        wxMenuItem *menu = querystatePopupMenu->FindItemByPosition(i);
        if (menu && menu->IsChecked())
            querystateList->SetColumnWidth(i, querystateColWidth[i]);
        else if (querystateList->GetColumnWidth(i) > 0)
            querystateList->SetColumnWidth(i, 0);

        // Save current width to restore it at next launch
        querystateList->GetColumn(i, column);
        if (currwidth > 0)
            settings->WriteInt(wxT("frmStatus/QuerystatePane_") + column.GetText() + wxT("_Width"),
                               querystateColWidth[i]);
        else
            settings->WriteInt(wxT("frmStatus/QuerystatePane_") + column.GetText() + wxT("_Width"),
                               -querystateColWidth[i]);
    }
}


void frmStatus::OnCommit(wxCommandEvent &event)
{
    long item = xactList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to commit the selected prepared transactions?"), _("Commit transaction?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) != wxYES)
        return;

    while  (item >= 0)
    {
        wxString xid = xactList->GetText(item, 1);
        wxString sql = wxT("COMMIT PREPARED ") + connection->qtDbString(xid);

        // We must execute this in the database in which the prepared transaction originated.
        if (connection->GetDbname() != xactList->GetText(item, 4))
        {
            pgConn *tmpConn = new pgConn(connection->GetHost(),
                                         connection->GetService(),
                                         connection->GetHostAddr(),
                                         xactList->GetText(item, 4),
                                         connection->GetUser(),
                                         connection->GetPassword(),
                                         connection->GetPort(),
                                         connection->GetRole(),
                                         "",
                                         connection->GetSslMode(),
                                         0,
                                         connection->GetApplicationName(),
                                         connection->GetSSLCert(),
                                         connection->GetSSLKey(),
                                         connection->GetSSLRootCert(),
                                         connection->GetSSLCrl(),
                                         connection->GetSSLCompression());
            if (tmpConn)
            {
                if (tmpConn->GetStatus() != PGCONN_OK)
                {
                    wxMessageBox(wxT("Connection failed: ") + tmpConn->GetLastError());
                    return ;
                }
                tmpConn->ExecuteScalar(sql);

                tmpConn->Close();
                delete tmpConn;
            }
        }
        else
            connection->ExecuteScalar(sql);

        item = xactList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    OnRefresh(event);
    wxListEvent ev;
    OnSelXactItem(ev);
}

void frmStatus::OnRollback(wxCommandEvent &event)
{
    long item = xactList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to rollback the selected prepared transactions?"), _("Rollback transaction?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) != wxYES)
        return;

    while  (item >= 0)
    {
        wxString xid = xactList->GetText(item, 1);
        wxString sql = wxT("ROLLBACK PREPARED ") + connection->qtDbString(xid);

        // We must execute this in the database in which the prepared transaction originated.
        if (connection->GetDbname() != xactList->GetText(item, 4))
        {
            pgConn *tmpConn = new pgConn(connection->GetHost(),
                                         connection->GetService(),
                                         connection->GetHostAddr(),
                                         xactList->GetText(item, 4),
                                         connection->GetUser(),
                                         connection->GetPassword(),
                                         connection->GetPort(),
                                         connection->GetRole(),
                                         "",
                                         connection->GetSslMode(),
                                         0,
                                         connection->GetApplicationName(),
                                         connection->GetSSLCert(),
                                         connection->GetSSLKey(),
                                         connection->GetSSLRootCert(),
                                         connection->GetSSLCrl(),
                                         connection->GetSSLCompression());
            if (tmpConn)
            {
                if (tmpConn->GetStatus() != PGCONN_OK)
                {
                    wxMessageBox(wxT("Connection failed: ") + tmpConn->GetLastError());
                    return ;
                }
                tmpConn->ExecuteScalar(sql);

                tmpConn->Close();
                delete tmpConn;
            }
        }
        else
            connection->ExecuteScalar(sql);

        item = xactList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    OnRefresh(event);
    wxListEvent ev;
    OnSelXactItem(ev);
}


void frmStatus::OnSelStatusItem(wxListEvent &event)
{
#ifdef __WXGTK__1
    manager.GetPane(wxT("Activity")).SetFlag(wxAuiPaneInfo::optionActive, true);
    manager.GetPane(wxT("Locks")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Transactions")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Logfile")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Querystate")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.Update();
#endif
    currentPane = PANE_STATUS;
    cbRate->SetValue(rateToCboString(statusRate));
    statusList->SetFocus();
    ActivatePane("Activity");
    if (connection && connection->BackendMinimumVersion(8, 0))
    {
        if(statusList->GetSelectedItemCount() > 0)
        {
            toolBar->EnableTool(MNU_CANCEL, true);
            actionMenu->Enable(MNU_CANCEL, true);
            if (connection->HasFeature(FEATURE_TERMINATE_BACKEND))
            {
                toolBar->EnableTool(MNU_TERMINATE, true);
                actionMenu->Enable(MNU_TERMINATE, true);
            }
        }
        else
        {
            toolBar->EnableTool(MNU_CANCEL, false);
            actionMenu->Enable(MNU_CANCEL, false);
            toolBar->EnableTool(MNU_TERMINATE, false);
            actionMenu->Enable(MNU_TERMINATE, false);
        }
    }
    toolBar->EnableTool(MNU_COMMIT, false);
    actionMenu->Enable(MNU_COMMIT, false);
    toolBar->EnableTool(MNU_ROLLBACK, false);
    actionMenu->Enable(MNU_ROLLBACK, false);
    cbLogfiles->Enable(false);
    btnRotateLog->Enable(false);

    editMenu->Enable(MNU_COPY, statusList->GetFirstSelected() >= 0);
    actionMenu->Enable(MNU_COPY_QUERY, statusList->GetFirstSelected() >= 0);
    toolBar->EnableTool(MNU_COPY_QUERY, statusList->GetFirstSelected() >= 0);

    //OnRefresh(event);
    wxTimerEvent evt;
    OnRefreshStatusTimer(evt);
    OnRefreshLocksTimer(evt);
    OnRefreshXactTimer(evt);
    OnRefreshQuerystateTimer(evt);

}


void frmStatus::OnSelLockItem(wxListEvent &event)
{
#ifdef __WXGTK__1
    manager.GetPane(wxT("Activity")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Locks")).SetFlag(wxAuiPaneInfo::optionActive, true);
    manager.GetPane(wxT("Transactions")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Logfile")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Querystate")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.Update();
#endif
    currentPane = PANE_LOCKS;
    lockList->SetFocus();
    ActivatePane("Locks");
    cbRate->SetValue(rateToCboString(locksRate));
    if (connection && connection->BackendMinimumVersion(8, 0))
    {
        if(lockList->GetSelectedItemCount() > 0)
        {
            toolBar->EnableTool(MNU_CANCEL, true);
            actionMenu->Enable(MNU_CANCEL, true);
            if (connection->HasFeature(FEATURE_TERMINATE_BACKEND))
            {
                toolBar->EnableTool(MNU_TERMINATE, true);
                actionMenu->Enable(MNU_TERMINATE, true);
            }
        }
        else
        {
            toolBar->EnableTool(MNU_CANCEL, false);
            actionMenu->Enable(MNU_CANCEL, false);
            toolBar->EnableTool(MNU_TERMINATE, false);
            actionMenu->Enable(MNU_TERMINATE, false);
        }
    }
    toolBar->EnableTool(MNU_COMMIT, false);
    actionMenu->Enable(MNU_COMMIT, false);
    toolBar->EnableTool(MNU_ROLLBACK, false);
    actionMenu->Enable(MNU_ROLLBACK, false);
    cbLogfiles->Enable(false);
    btnRotateLog->Enable(false);

    editMenu->Enable(MNU_COPY, lockList->GetFirstSelected() >= 0);
    actionMenu->Enable(MNU_COPY_QUERY, false);
    toolBar->EnableTool(MNU_COPY_QUERY, false);
}


void frmStatus::OnSelXactItem(wxListEvent &event)
{
#ifdef __WXGTK__1
    manager.GetPane(wxT("Activity")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Locks")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Transactions")).SetFlag(wxAuiPaneInfo::optionActive, true);
    manager.GetPane(wxT("Logfile")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Querystate")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.Update();
#endif
    currentPane = PANE_XACT;
    xactList->SetFocus();
    ActivatePane("Transactions"); 
    cbRate->SetValue(rateToCboString(xactRate));
    if(xactList->GetSelectedItemCount() > 0)
    {
        toolBar->EnableTool(MNU_COMMIT, true);
        actionMenu->Enable(MNU_COMMIT, true);
        toolBar->EnableTool(MNU_ROLLBACK, true);
        actionMenu->Enable(MNU_ROLLBACK, true);
    }
    else
    {
        toolBar->EnableTool(MNU_COMMIT, false);
        actionMenu->Enable(MNU_COMMIT, false);
        toolBar->EnableTool(MNU_ROLLBACK, false);
        actionMenu->Enable(MNU_ROLLBACK, false);
    }
    toolBar->EnableTool(MNU_CANCEL, false);
    actionMenu->Enable(MNU_CANCEL, false);
    toolBar->EnableTool(MNU_TERMINATE, false);
    actionMenu->Enable(MNU_TERMINATE, false);
    cbLogfiles->Enable(false);
    btnRotateLog->Enable(false);

    editMenu->Enable(MNU_COPY, xactList->GetFirstSelected() >= 0);
    actionMenu->Enable(MNU_COPY_QUERY, false);
    toolBar->EnableTool(MNU_COPY_QUERY, false);
}

void frmStatus::OnSelQuerystateItem(wxListEvent &event)
{
#ifdef __WXGTK__1
    manager.GetPane(wxT("Activity")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Locks")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Transactions")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Logfile")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Querystate")).SetFlag(wxAuiPaneInfo::optionActive, true);
    manager.Update();
#endif
    currentPane = PANE_QUERYSTATE;
    querystateList->SetFocus();
    ActivatePane("Querystate");
    cbRate->SetValue(rateToCboString(querystateRate));
    {
        toolBar->EnableTool(MNU_COMMIT, false);
        actionMenu->Enable(MNU_COMMIT, false);
        toolBar->EnableTool(MNU_ROLLBACK, false);
        actionMenu->Enable(MNU_ROLLBACK, false);
    }
    toolBar->EnableTool(MNU_CANCEL, false);
    actionMenu->Enable(MNU_CANCEL, false);
    toolBar->EnableTool(MNU_TERMINATE, false);
    actionMenu->Enable(MNU_TERMINATE, false);
    cbLogfiles->Enable(false);
    btnRotateLog->Enable(false);

    editMenu->Enable(MNU_COPY, querystateList->GetFirstSelected() >= 0);
    actionMenu->Enable(MNU_COPY_QUERY, false);
    toolBar->EnableTool(MNU_COPY_QUERY, false);
}

void frmStatus::ActivatePane(wxString name) {
    if (!manager.GetPane(name).HasFlag(wxAuiPaneInfo::optionActive)) {
        //manager.GetPane("Logfile").HasFlag(wxAuiPaneInfo::optionActive);
        int i, pane_count;
        wxAuiPaneInfo* active_paneinfo = NULL;
        for (unsigned int i = 0; i < manager.GetAllPanes().GetCount(); i++)
        {
            wxAuiPaneInfo& pane = manager.GetAllPanes()[i];
            pane.state &= ~wxAuiPaneInfo::optionActive;
            if (pane.name == name)
            {
                pane.state |= wxAuiPaneInfo::optionActive;
                active_paneinfo = &pane;
            }
        }
        if (active_paneinfo) manager.Update();
    }
}


void frmStatus::OnSelLogItem(wxListEvent &event)
{
#ifdef __WXGTK__1
    manager.GetPane(wxT("Activity")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Locks")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Transactions")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Logfile")).SetFlag(wxAuiPaneInfo::optionActive, true);
    manager.GetPane(wxT("Querystate")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.Update();
#endif
    currentPane = PANE_LOG;
    cbRate->SetValue(rateToCboString(logRate));
    logList->SetFocus();
    ActivatePane("Logfile");
    // if there's no log, don't enable items
    if (logDirectory != wxT("-"))
    {
        cbLogfiles->Enable(true);
        btnRotateLog->Enable(true);
        toolBar->EnableTool(MNU_CANCEL, false);
        toolBar->EnableTool(MNU_TERMINATE, false);
        toolBar->EnableTool(MNU_COMMIT, false);
        toolBar->EnableTool(MNU_ROLLBACK, false);
        actionMenu->Enable(MNU_CANCEL, false);
        actionMenu->Enable(MNU_TERMINATE, false);
        actionMenu->Enable(MNU_COMMIT, false);
        actionMenu->Enable(MNU_ROLLBACK, false);
    }

    editMenu->Enable(MNU_COPY, logList->GetFirstSelected() >= 0);
    actionMenu->Enable(MNU_COPY_QUERY, false);
    toolBar->EnableTool(MNU_COPY_QUERY, false);
    event.Skip();
}


void frmStatus::SetColumnImage(ctlListView *list, int col, int image)
{
    wxListItem item;
    item.SetMask(wxLIST_MASK_IMAGE);
    item.SetImage(image);
    list->SetColumn(col, item);
}

void frmStatus::OnRightClickStatusItem(wxListEvent& event)
{
    int row = event.GetIndex();
    //wxString txt = event.GetText();
    if ((row<0) || (row>=statusList->GetItemCount())) return;
    wxRect r;
    //statusList->GetItemRect(row, r);
    wxString ss = wxEmptyString;
    int col = -1;
    for (int cc = 0; cc < statusList->GetColumnCount();cc++) {
        statusList->GetSubItemRect(row, cc, r, wxLIST_RECT_BOUNDS);
        if (r.Contains(event.GetPoint())) {
            ss = wxString::Format("\rBounding rect of item %d column %d is (%d, %d)-(%d, %d)", row,cc, r.x, r.y, r.x + r.width, r.y + r.height);
            col = cc;
            break;
        }
    }
    if (col == -1) return;
    wxString val=statusList->GetItemText(row, col);
    wxString txt = wxString::Format("gettext=%s\r index=%d\r column=%d", val.c_str(), row, col);
    txt = txt + ss;
    
    wxListItem listitem;
    listitem.SetMask(wxLIST_MASK_TEXT);
    statusList->GetColumn(col, listitem);
    wxString label = listitem.GetText()+" = "+val;
    //wxMessageBox(txt, "test", wxICON_WARNING | wxOK);
    wxString hint=label;
    if (filterColumn.size() > 0) hint = toolBar->GetToolShortHelp(MNU_CLEAR_FILTER_SERVER_STATUS)+"\n"+label;
    filterColumn.Add(col);
    filterValue.Add(val);
    toolBar->SetToolShortHelp(MNU_CLEAR_FILTER_SERVER_STATUS, hint);
    toolBar->EnableTool(MNU_CLEAR_FILTER_SERVER_STATUS, true);
    toolBar->EnableTool(MNU_SET_FILTER_HIGHLIGHT_STATUS, false);
    
    wxTimerEvent evt;
    OnRefreshStatusTimer(evt);
}

void frmStatus::OnSetHighlightFilter(wxCommandEvent& event) {
    toolBar->SetToolShortHelp(MNU_CLEAR_FILTER_SERVER_STATUS, "");
    toolBar->EnableTool(MNU_CLEAR_FILTER_SERVER_STATUS, true);
    toolBar->EnableTool(MNU_SET_FILTER_HIGHLIGHT_STATUS, false);
    onlyhightligth = true;
}
void frmStatus::OnClearFilter(wxCommandEvent& event) {
    toolBar->EnableTool(MNU_CLEAR_FILTER_SERVER_STATUS, false);
    toolBar->EnableTool(MNU_SET_FILTER_HIGHLIGHT_STATUS, true);
    
    toolBar->SetToolShortHelp(MNU_CLEAR_FILTER_SERVER_STATUS, "Clear filter");
    filterColumn.Clear();
    filterValue.Clear();
    onlyhightligth = false;
    wxTimerEvent evt;
    OnRefreshStatusTimer(evt);

}

void frmStatus::OnSortStatusGrid(wxListEvent &event)
{
    // Get the information for the SQL ORDER BY
    if (event.GetColumn()<0) return;
    if (statusSortColumn == event.GetColumn() + 1)
    {
        if (statusSortOrder == wxT("ASC"))
            statusSortOrder = wxT("DESC");
        else
            statusSortOrder = wxT("ASC");
    }
    else
    {
        statusSortColumn = event.GetColumn() + 1;
        statusSortOrder = wxT("ASC");
    }


    // Re-initialize all columns' image
    for (int i = 0; i < statusList->GetColumnCount(); i++)
    {
        SetColumnImage(statusList, i, -1);
    }

    // Set the up/down image
    if (statusSortOrder == wxT("ASC"))
        SetColumnImage(statusList, statusSortColumn - 1, 0);
    else
        SetColumnImage(statusList, statusSortColumn - 1, 1);

    // Refresh grid
    wxTimerEvent evt;
    OnRefreshStatusTimer(evt);
}


void frmStatus::OnSortLockGrid(wxListEvent &event)
{
    // Get the information for the SQL ORDER BY
    if (lockSortColumn == event.GetColumn() + 1)
    {
        if (lockSortOrder == wxT("ASC"))
            lockSortOrder = wxT("DESC");
        else
            lockSortOrder = wxT("ASC");
    }
    else
    {
        lockSortColumn = event.GetColumn() + 1;
        lockSortOrder = wxT("ASC");
    }

    // There are no sort operator for xid before 8.3
    if (!connection->BackendMinimumVersion(8, 3) && lockSortColumn == 5)
    {
        wxLogError(_("You cannot sort by transaction id on your PostgreSQL release. You need at least 8.3."));
        lockSortColumn = 1;
    }

    // Re-initialize all columns' image
    for (int i = 0; i < lockList->GetColumnCount(); i++)
    {
        SetColumnImage(lockList, i, -1);
    }

    // Set the up/down image
    if (lockSortOrder == wxT("ASC"))
        SetColumnImage(lockList, lockSortColumn - 1, 0);
    else
        SetColumnImage(lockList, lockSortColumn - 1, 1);

    // Refresh grid
    wxTimerEvent evt;
    OnRefreshLocksTimer(evt);
}


void frmStatus::OnSortXactGrid(wxListEvent &event)
{
    // Get the information for the SQL ORDER BY
    if (xactSortColumn == event.GetColumn() + 1)
    {
        if (xactSortOrder == wxT("ASC"))
            xactSortOrder = wxT("DESC");
        else
            xactSortOrder = wxT("ASC");
    }
    else
    {
        xactSortColumn = event.GetColumn() + 1;
        xactSortOrder = wxT("ASC");
    }

    // There are no sort operator for xid before 8.3
    if (!connection->BackendMinimumVersion(8, 3) && xactSortColumn == 1)
    {
        wxLogError(_("You cannot sort by transaction id on your PostgreSQL release. You need at least 8.3."));
        xactSortColumn = 2;
    }

    // Re-initialize all columns' image
    for (int i = 0; i < xactList->GetColumnCount(); i++)
    {
        SetColumnImage(xactList, i, -1);
    }

    // Set the up/down image
    if (xactSortOrder == wxT("ASC"))
        SetColumnImage(xactList, xactSortColumn - 1, 0);
    else
        SetColumnImage(xactList, xactSortColumn - 1, 1);

    // Refresh grid
    wxTimerEvent evt;
    OnRefreshXactTimer(evt);
}


void frmStatus::OnRightClickStatusGrid(wxListEvent &event)
{
    statusList->PopupMenu(statusPopupMenu, event.GetPoint());
}

void frmStatus::OnRightClickLockGrid(wxListEvent &event)
{
    lockList->PopupMenu(lockPopupMenu, event.GetPoint());
}

void frmStatus::OnRightClickXactGrid(wxListEvent &event)
{
    xactList->PopupMenu(xactPopupMenu, event.GetPoint());
}
void frmStatus::OnRightClickQuerystateGrid(wxListEvent &event)
{
    querystateList->PopupMenu(querystatePopupMenu, event.GetPoint());
}
void frmStatus::OnLogContextMenu(wxCommandEvent& event) {
    nav->OnContextMenu(event);
}
int count = 0;
void frmStatus::OnTimerHintLog(wxTimerEvent& event)
{
    delayHitLog->Stop();
    wxPoint pm = logList->GetScreenPosition();
    wxRect rc = logList->GetSize();
    wxPoint m = wxGetMousePosition();
    rc.x = pm.x;
    rc.y = pm.y;
    count++;
    //statusBar->SetStatusText(wxString::Format(" TIMER COUNT %d x:%d,y:%d",count, m.x, m.y));
    if (!rc.Contains(m)) {
    }
    else {
        wxPoint curr = logList->ScreenToClient(m);
        if (lastmouse == m && lastlogitem != -1 && lastlogitemShow != lastlogitem) {
            {
                wxString s;
                if (m_Popup != NULL) {
                    delete m_Popup;
                    m_Popup = NULL;
                }
                else {
                }
                s = logList->GetTextLong(lastlogitem);
                lastlogitemShow = lastlogitem;
                lastlogitem = -1;
                wxSize rr(350, 25);
                // 
                wxString key = "content";
                int x, y;
                wxPoint p = m;
                p.x = p.x + 5;
                p.y = p.y + 5;
                PreviewHtml v;
                wxString tt = v.Preview(s, fmtpreview::CSV);
                s = tt;
                FunctionPGHelper fh(s);
                fh.SetTimerClose(1500);
                //m_Popup = new popuphelp((wxWindow*)winMain, key, &fh, p, rr);
                m_Popup = new popuphelp(this, key, &fh, p, rr);
                if (m_Popup && m_Popup->IsValid() && rr != m_Popup->GetSizePopup()) {
                    // recreate with new size
                    rr = m_Popup->GetSizePopup();
                    delete 	m_Popup;
                    m_Popup = new popuphelp(this, key, &fh, p, rr);
                }
                if (m_Popup && m_Popup->IsValid()) {
                    //m_PopupHelp->UpdateWindowUI(true);
                    wxSize top_sz = m_Popup->GetSizePopup();
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
                    wxSize top_new(top_sz);
                    wxPoint oldp(p);
                    if (p.x + top_new.x > sizeScreen.x) p.x = sizeScreen.x - top_new.x - 20;
                    if (p.y + top_new.y > sizeScreen.y) p.y = sizeScreen.y - top_new.y - 20;
                    if (oldp == p) p.x = p.x + 20;
                    m_Popup->Move(p);
                    wxRect r = m_Popup->GetScreenRect();
                    //m_PopupHelp->Position(p, wxSize(0, 17));
                    m_Popup->Popup();
                    //wxPopupTransientWindow
                    
                }

            }
        }
    }
}
void frmStatus::OnCmdFindStrLog(wxCommandEvent& event) {
    wxString s = event.GetString();
    //statusBar->SetStatusText(wxString::Format(" FIND CMD: %s",s));
    nav->SetFindString(s);
}
void frmStatus::OnMoveMouseLog(wxMouseEvent& event)
{
//	wxMenu* logListPopupMenu;
//	logListPopupMenu = nav->GetPopupMenu();
    wxPoint mp = event.GetPosition();
    lastmouse = wxGetMousePosition();
    int flags = wxLIST_HITTEST_ONITEMLABEL;
    long item=logList->HitTest(mp,flags);
    //logList->PopupMenu(logListPopupMenu, event.GetPoint());
   // statusBar->SetStatusText(wxString::Format("x:%d,y:%d",lastmouse.x,lastmouse.y));
    wxString s;
    
    if (lastlogitem != -1 || lastlogitem != item) {
#define DELAYHITLOGPERIOD 1000
        delayHitLog->Stop();
        delayHitLog->StartOnce(DELAYHITLOGPERIOD);
    }
    else 
        delayHitLog->Stop();
    lastlogitem = item;
    event.Skip();
    return;
}

void frmStatus::OnRightClickLogGrid(wxListEvent& event)
{
    delayHitLog->Stop();
    wxMenu* logListPopupMenu;
    logListPopupMenu = nav->GetPopupMenu();
    logList->PopupMenu(logListPopupMenu, event.GetPoint());
}


void frmStatus::OnChgColSizeStatusGrid(wxListEvent &event)
{
    wxCommandEvent ev;
    OnStatusMenu(ev);
}

void frmStatus::OnChgColSizeLockGrid(wxListEvent &event)
{
    wxCommandEvent ev;
    OnLockMenu(ev);
}

void frmStatus::OnChgColSizeXactGrid(wxListEvent &event)
{
    wxCommandEvent ev;
    OnXactMenu(ev);
}

void frmStatus::OnChgColSizeQuerystateGrid(wxListEvent &event)
{
    wxCommandEvent ev;
    OnQuerystateMenu(ev);
}


serverStatusFactory::serverStatusFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar) : actionFactory(list)
{
    mnu->Append(id, _("&Server Status\tCtrl-S"), _("Displays the current database status."));
}


wxWindow *serverStatusFactory::StartDialog(frmMain *form, pgObject *obj)
{

    pgServer *server = obj->GetServer();
    wxString applicationname = appearanceFactory->GetLongAppName() + wxT(" - Server Status");

    pgConn *conn = server->CreateConn(wxEmptyString, 0, applicationname);
    if (conn)
    {
        wxString txt = server->GetDescription()
                       + wxT(" (") + server->GetName() + wxT(":") + NumToStr((long)server->GetPort()) + wxT(")");

        frmStatus *status = new frmStatus(form, txt, conn);
        status->Go();
        return status;
    }
    return 0;
}


bool serverStatusFactory::CheckEnable(pgObject *obj)
{
    return obj && obj->GetServer() != 0;
}
