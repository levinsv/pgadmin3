#pragma once
#include "utils/chatclient.h"

class frmQuery;

class ctlChat : public wxPanel {
public:
        ctlChat(wxWindow *parent,frmQuery *frm);
        bool RequestClose();
        ~ctlChat() {

        };
bool SendText(const wxString text);
wxString GetNameChat();
private:
 void SaveInputToHistory(const wxString& text);
 void SetStatus(const wxString msg);
 void OnKeyDown(wxKeyEvent& event);
 void OnHistoryUp(wxCommandEvent&);
void OnHistoryDown(wxCommandEvent&);
void OnClearHistory(wxCommandEvent&);
void OnCancel(wxCommandEvent&);
void OnSend(wxCommandEvent& );
void OnClose(wxCloseEvent& event);
void ShowError(int status, const wxString& msg);

    wxTextCtrl*    m_input   = nullptr;
    wxTextCtrl*    m_output  = nullptr;
    wxHtmlWindow*   m_html = nullptr;
    wxButton*      m_sendBtn = nullptr;
    wxButton*     m_cancelBtn = nullptr;
    wxStaticText*  m_status  = nullptr;
    wxButton* m_clearBtn = nullptr;
    wxButton* m_upBtn   = nullptr;
    wxButton* m_downBtn = nullptr;
    std::vector<wxString> m_textHistory;   // история введённых текстов
    int       m_histPos = -1;              // текущая позиция, -1 = вне истории
    wxString  m_draft;                     // черновик, набранный до листания

    ChatClient *m_client;
    frmQuery* queryfrm;
    std::vector<std::pair<wxString, wxString>> m_history;
    bool           m_closing = false;
    bool yesAnswer=false;

};
