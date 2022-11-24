#pragma once

#ifndef _WX_ctlGitPanel_H_
#define _WX_ctlGitPanel_H_
#include "wx/wx.h"
#include "wx/notebook.h"
#include "wx/artprov.h"
#include "wx/creddlg.h"
#include "wx/webrequest.h"
#include "wx/filedlg.h"
#include "wx/image.h"
#include "utils/json/jsonval.h"
#include <wx/hyperlink.h>
#include "images/ddAddColumn.pngc"
#include "images/ddRemoveColumn.pngc"
#include "images/conversion.pngc"


class ctlGitPanel : public wxPanel
{
public:
    enum Pages
    {
        Page_Image,
        Page_Text,
        Page_Download,
        Page_Advanced
    };
    enum Files
    {
        Add_File,
        Remove_File,
        Update_File
    };

    ctlGitPanel(wxWindow* parent, frmMain* form,wxJSONValue cf);
    ~ctlGitPanel();
    void ShowPage(pgObject* data);
    void getFileRepository(pgObject* data);
    wxJSONValue execRequest(wxString url, wxJSONValue args, wxString cmd);
    //wxJSONValue setComonnArgs();
    void GetBranchList(bool refresh);
    void GetRepositoryTree(wxString branchName,wxString path,wxString typeElement,wxString value);
    wxString GetRepositoryFile(wxString branchName, wxString path);
    wxString getCurBranch(wxString dbname);
    bool CheckValidObject(pgObject *o);
    void CommandBranch(wxString branchName, wxString cmd);
    bool ApplyCommit(wxString branchName, wxJSONValue params);
    wxString ArgsForGet(wxJSONValue r);
    int ReplaceItem(wxString path, int image);
    static wxJSONValue GetConfig();
    void OnBranchListButton(wxCommandEvent& WXUNUSED(evt)) {
        GetBranchList(true);
    };
    void GetExpandedChildNodes(wxTreeItemId node, wxArrayString& expandedNodes, wxString pat, int lvl);
    void OnBranchDeleteButton(wxCommandEvent& WXUNUSED(evt)) {};
    
    void OnListRClick(wxListEvent& evt);
    void OnLoadGitButton(wxCommandEvent& WXUNUSED(evt));
    void OnCommitButton(wxCommandEvent& WXUNUSED(evt));
    void OnCancelButton(wxCommandEvent& WXUNUSED(evt));
    void OnKEY_UP(wxKeyEvent& event);
    void OnProgressTimer(wxTimerEvent& WXUNUSED(evt));
    void OnPostCheckBox(wxCommandEvent& WXUNUSED(evt));
    void OnNotebookPageChanged(wxBookCtrlEvent& event);
    void OnClose(wxCloseEvent& event);


private:
    frmMain* formMain;
    wxJSONValue cfg;
    wxString currentDBname;
    wxString syncDBname;
    wxTreeItemId nodeDB;
    wxStringToStringHashMap m_git_content,m_base_content;
    wxStringToStringHashMap m_treeName,m_base_tree;
    wxString response_link,error_msg;
    int m_count_git, m_count_db;
    wxListView* m_commit_List_View;

    wxComboBox* m_Branch_List_Ctrl;
    wxButton *m_branchDeleteButton, *m_branchListButton, *m_commitButton;
    wxHyperlinkCtrl* m_link;
    wxNotebook* m_notebook;
    wxTextCtrl* m_urlTextCtrl;
    wxButton* m_startButton;
    wxButton* m_cancelButton;
    wxStaticBitmap* m_imageStaticBitmap;
    wxWebRequest m_currentRequest;

    wxCheckBox* m_postCheckBox;
    wxTextCtrl* m_postContentTypeTextCtrl;
    wxTextCtrl* m_postRequestTextCtrl;
    wxTextCtrl* m_textResponseTextCtrl;
    wxTextCtrl* m_CommentTextCtrl;
    wxGauge* m_downloadGauge;
    wxStaticText* m_downloadStaticText;
    wxTimer m_downloadProgressTimer;

    wxStaticText* m_advCountStaticText;
    wxLongLong m_advCount;

    // Normally it would be a bad idea to permanently store credentials like
    // this, we should use wxSecretStore to load them as needed, but let's keep
    // things simple in this example.
    wxWebCredentials m_credentials;


};

#endif
