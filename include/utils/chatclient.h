#pragma once
#include <wx/wx.h>
#include <wx/webrequest.h>
#include <functional>
#include <utility>
#include <vector>

class ChatClient : public wxEvtHandler
{
public:
    struct Message
    {
        wxString role;
        wxString content;
        wxString html;
    };

   
    // Колбэки для владельца
    std::function<void(const wxString& answer)>                    OnAnswer;   // успешный ответ
    std::function<void(wxWebRequest::State, const wxString& err)>  OnState;    // Failed / Cancelled
    std::function<void(const std::vector<Message>& )>              OnFinished; // любой конечный итог

    ChatClient();
//    virtual ~ChatClient() noexcept(false) = default;
    void SetParameters(const wxString baseurl, const wxString chaturl,const wxString header,const wxString param,const wxString sysprompt);

    // Отправляет сообщение пользователя. false — если запрос уже активен
    // или текст пуст. При неудаче запись откатывается автоматически.
    bool Send(const wxString& userText);

    void Cancel();
    bool IsBusy() const { return m_request.IsOk(); }

    // Очищает историю, возвращает system-промпт. false — если запрос активен.
    bool ClearHistory();
    wxString GetHtmlAll();

    const std::vector<Message>&  GetHistory() const { return m_history; }

private:
    void OnRequestData(wxWebRequestEvent& event);
    void OnRequestState(wxWebRequestEvent& event);
    void Finish();
    void Rollback();

    static wxString JsonEscape(const wxString& s);
    static wxString JsonString(const wxString& s);
    wxString        BuildChatBody() const;
    static bool     ExtractMessageContent(const wxString& raw, wxString& out);

    wxWebSession&    m_session = wxWebSession::GetDefault();
    wxString         m_url;
    wxString         m_chat;
    wxString         m_par;
    wxString         m_header;
    wxString         m_systemPrompt;

    wxWebRequest     m_request;      // refcounted, живёт пока IsOk()
    wxMemoryBuffer   m_buffer;       // копим порции тела ответа
    wxString         m_lastBody;     // тело последнего отправленного запроса
    std::vector<Message>   m_history;

};
