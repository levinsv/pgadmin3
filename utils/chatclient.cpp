#include "pgAdmin3.h"
#include "utils/chatclient.h"
#include "utils/mdhtml.h"
#include "utils/misc.h"

//wxString convertMDtoHtml(const wxString md, bool isDark);


// Экранирование строки для JSON
wxString ChatClient::JsonEscape(const wxString& s)
{
    wxString out;
    out.reserve(s.size() + 8);
    for (size_t i = 0; i < s.size(); ++i)
    {
        const wxUniChar c = s[i];
        switch (static_cast<int>(c))
        {
            case '"':  out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n";  break;
            case '\r': out << "\\r";  break;
            case '\t': out << "\\t";  break;
            case '\b': out << "\\b";  break;
            case '\f': out << "\\f";  break;
            default:
                if (static_cast<int>(c) < 0x20)
                    out << wxString::Format("\\u%04x", static_cast<int>(c));
                else
                    out << c;
        }
    }
    return out;
}

wxString ChatClient::JsonString(const wxString& s)
{
    return "\"" + JsonEscape(s) + "\"";
}

// Тело запроса для /api/chat из накопленной истории
wxString ChatClient::BuildChatBody() const
{
    wxString body="{";
    if (!m_par.IsEmpty()) {
         body+=m_par;
         if (m_par.Find("stream")==-1) body << ",\"stream\":false";
         
    } else body << "\"stream\":false";

    body << ",\"messages\":[";
    for (size_t i = 0; i < m_history.size(); ++i)
    {
        if (i)
            body << ',';
        body << "{\"role\":" << JsonString(m_history[i].role)
             << ",\"content\":" << JsonString(m_history[i].content) << '}';
    }
    body << "]}";
    return body;
}

// Достаёт поле message.content из ответа.
// false — поле не найдено или строка не закрыта.
bool ChatClient::ExtractMessageContent(const wxString& raw, wxString& out)
{
    const wxString key = "\"content\":";
    const size_t pos = raw.find(key);
    if (pos == wxString::npos)
        return false;

    size_t i = pos + key.size();
    if (i >= raw.size() || raw[i] != '"')
        return false;
    ++i;
    wxString mm=raw.Mid(i,20);
    out.clear();
    while (i < raw.size())
    {
        const wxUniChar c = raw[i];
        if (c == '"') {
            size_t pos = raw.find(key,i);   // find next content
            if (pos == wxString::npos) return true;                       // конец строки
            i = pos + key.size();
            if (i >= raw.size() || raw[i] != '"')
                return false;
            ++i;
            continue;
        }
        if (c == '\\' && i + 1 < raw.size())
        {
            const wxUniChar n = raw[i + 1];
            switch (static_cast<int>(n))
            {
                case '"':  out << '"';  break;
                case '\\': out << '\\'; break;
                case '/':  out << '/';  break;
                case 'n':  out << '\n'; break;
                case 'r':  out << '\r'; break;
                case 't':  out << '\t'; break;
                case 'b':  out << '\b'; break;
                case 'f':  out << '\f'; break;
                case 'u':
                    if (i + 5 < raw.size())
                    {
                        const wxString hex = raw.Mid(i + 2, 4);
                        unsigned long code = 0;
                        if (hex.ToULong(&code, 16))
                            out << wxUniChar(code);
                        i += 4;
                    }
                    break;
                default: out << n; break;
            }
            i += 2;
        }
        else
        {
            out << c;
            ++i;
        }
    }
    return false;   // незакрытая строка
}

// ---------- Жизненный цикл ----------

ChatClient::ChatClient()
{
    m_systemPrompt =
        L"Ты - ассистент по SQL для PostgreSQL. Объясняй запросы простыми словами.Не выдумывай таблицы и поля.";
    Bind(wxEVT_WEBREQUEST_STATE, &ChatClient::OnRequestState, this);
    Bind(wxEVT_WEBREQUEST_DATA,  &ChatClient::OnRequestData,  this);
}
void ChatClient::SetParameters(const wxString baseurl, const wxString chaturl,const wxString header,const wxString param,const wxString sysprompt) {
    m_url=baseurl;
    m_chat=chaturl;
    m_par=param;
    m_systemPrompt=sysprompt;
    m_header=header;
    m_history.push_back({"system", m_systemPrompt,""});

}
bool ChatClient::Send(const wxString& userText)
{
    if (m_request.IsOk())          // защита от повторной отправки
        return false;

    const wxString text = userText;
    if (text.empty())
        return false;
    wxString html=convertMDtoHtml(text,isDark(),"user");
    m_history.push_back({"user", text,html});

    const wxString body = BuildChatBody();
    m_lastBody = body;
    wxString url=m_url+ m_chat;
    m_buffer.Clear();
    m_request = m_session.CreateRequest(this, url);
    //if (!m_header.IsEmpty()) m_request.SetHeader();
    m_request.SetMethod("POST");
    m_request.SetData(m_lastBody, "application/json");
    m_request.Start();
    return true;
}

void ChatClient::Cancel()
{
    if (!m_request.IsOk())
        return;
    if (m_request.GetState() == wxWebRequest::State_Active)
        m_request.Cancel();

}

bool ChatClient::ClearHistory()
{
    if (m_request.IsOk())          // во время запроса историю не трогаем
        return false;

    m_history.clear();
    m_history.push_back({"system", m_systemPrompt,""});
    return true;
}

void ChatClient::Rollback()
{
    if (!m_history.empty() && m_history.back().role == "user")
        m_history.pop_back();
}

wxString ChatClient::GetHtmlAll() {
    wxString all;
    for (int i=0;i<(int)m_history.size();i++) {
        if (m_history[i].role!="system") {
            if (!all.IsEmpty()) all+="<br>";
            all+=m_history[i].html;
        }
    }
    all=PageHtml(all, isDark());
    return all;
}
void ChatClient::Finish()
{
    m_request = wxWebRequest();   // сбрасываем умный указатель
    m_lastBody.clear();
    if (OnFinished)
        OnFinished(m_history);
}

// ---------- Обработка событий ----------

void ChatClient::OnRequestData(wxWebRequestEvent& event)
{
    // Накапливаем сырые байты: порция может разрезать юникодный
    // символ посередине, поэтому в wxString конвертируем только в конце
    //m_buffer.AppendData(event.GetDataBuffer(), event.GetDataSize());
    //int l=event.GetDataSize();

}

void ChatClient::OnRequestState(wxWebRequestEvent& event)
{
    switch (event.GetState())
    {
        case wxWebRequest::State_Completed:
        {
            wxWebResponse response = event.GetRequest().GetResponse();

            wxString raw = wxString::FromUTF8(
                static_cast<const char*>(m_buffer.GetData()),
                m_buffer.GetDataLen());
            const wxString asString = response.AsString();
            if (!asString.empty())
                raw += asString;

            wxString answer;
            if (response.GetStatus() == 200 && ExtractMessageContent(raw, answer))
            {
                wxString htmlContent = convertMDtoHtml(answer,isDark(),"assistant");
                m_history.push_back({"assistant", answer,htmlContent});

                if (OnAnswer)
                    OnAnswer(answer);
            }
            else
            {
                // В старом коде здесь break обходил FinishRequest —
                // кнопки оставались заблокированными до перезапуска.
                const wxString err = (response.GetStatus() != 200)
                    ? wxString::Format(L"HTTP %d: %s", response.GetStatus(),
                                       response.GetStatusText())
                    : _("Not response answer server");
                Rollback();
                if (OnState)
                    OnState(wxWebRequest::State_Failed, err);
            }
            Finish();
            break;
        }

        case wxWebRequest::State_Failed:
        {
            const wxString err = event.GetErrorDescription();
            Rollback();
            if (OnState)
                OnState(wxWebRequest::State_Failed, err);
            Finish();
            break;
        }

        case wxWebRequest::State_Cancelled:
            Rollback();
            if (OnState)
                OnState(wxWebRequest::State_Cancelled, wxEmptyString);
            Finish();
            break;

        default:
            break;   // Idle / Active / Authorizing — ничего не делаем
    }
}
