#include "pgAdmin3.h"
#include <wx/wx.h>
#include <wx/html/htmlwin.h>
#include <vector>
#include "utils/chatclient.h"
#include <wx/file.h>
#include <wx/splitter.h>
#include "ctl/ctlChat.h"
#include "frm/frmQuery.h"
//wxString kSystemPrompt =    L"Ты - ассистент по SQL для PostgreSQL. Объясняй запросы простыми словами.Не выдумывай таблицы и поля.";
//    L"- краткий итог в 1-2 предложениях; - что делают JOIN, WHERE, "
//    L"GROUP BY, HAVING и т.д. по пунктам; - замечания по производительности, если есть."
#ifdef _DEBUG
static void SaveFile(const wxString& path, const wxString& text)
    {
        wxFile file(path, wxFile::write);
        if (file.IsOpened())
        {
            const wxWX2MBbuf mb = text.mb_str(wxConvUTF8);
            file.Write(static_cast<const char*>(mb), strlen(mb));
        }
    }
#endif    
wxString GetTextFromContainer(const wxHtmlCell *cellup) {
    wxString str;
    wxHtmlContainerCell *cc = wxDynamicCast(cellup, wxHtmlContainerCell);
    if (cc) {
            const wxHtmlCell *c;
            bool br=true;
            int len2=str.Length();
            int c2=0;
            for ( c = cellup->GetFirstChild();
                    c ; c = c->GetNext())
            {
                int len=str.Length();
                str+=GetTextFromContainer(c);
                if (len==str.Length() && br) {
                    br=false;
                }
                c2++;
            }
            if (br && c2) str+='\n';

    } else {
        wxHtmlWordCell *w=wxDynamicCast(cellup, wxHtmlWordCell);
        if (w) {
            str=cellup->ConvertToText(NULL);
        }
    }
    return str;
}    


ctlChat::ctlChat(wxWindow *parent, frmQuery *frm):
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
    {
        queryfrm=frm;
        // ---------- Верхняя панель: чат ----------
        auto* topPanel = new wxPanel(this);
        auto* topSizer = new wxBoxSizer(wxVERTICAL);
        //topSizer->Add(new wxStaticText(topPanel, wxID_ANY, L"Чат:"),0, wxALL, 3);
        m_html = new wxHtmlWindow(topPanel, wxID_ANY);
        m_html->SetBackgroundColour(parent->GetBackgroundColour());
        //wxColour(GetPalette(isDark()).pageBg);

        topSizer->Add(m_html, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);
        topPanel->SetSizer(topSizer);

        // ---------- Нижняя панель: ввод и кнопки ----------
        auto* bottomPanel = new wxPanel(this);
        auto* bottomSizer = new wxBoxSizer(wxVERTICAL);

 m_input = new wxTextCtrl(bottomPanel, wxID_ANY, wxEmptyString,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_MULTILINE);
//        m_input->SetValue(L"форматирование ");
int fontHeight = m_input->GetFont().GetPixelSize().y;
// Формула: (высота строки + межстрочный интервал) * кол-во строк + отступы сверху/снизу
// Обычно межстрочный интервал ~4px, отступы ~6px
int inputHeight = (fontHeight + 4) * 2 + 6; 
const int btnFixedH = inputHeight/2;
        // Колонка навигации по истории справа от поля ввода
        m_upBtn   = new wxButton(bottomPanel, wxID_ANY, _("Up"));
        m_downBtn = new wxButton(bottomPanel, wxID_ANY, _("Down"));
        m_downBtn->Disable();
m_upBtn->SetMaxSize(wxSize(-1, btnFixedH));
m_downBtn->SetMaxSize(wxSize(-1, btnFixedH));

        auto* histCol = new wxBoxSizer(wxVERTICAL);
        histCol->Add(m_upBtn,   0,wxLEFT | wxRIGHT | wxBOTTOM, 2);
        histCol->Add(m_downBtn, 0, wxLEFT | wxRIGHT,2);

        auto* inputRow = new wxBoxSizer(wxHORIZONTAL);
        inputRow->Add(m_input,  1, wxEXPAND | wxRIGHT, 1);
        inputRow->Add(histCol,  0, wxALL,1 );

        m_sendBtn = new wxButton(bottomPanel, wxID_ANY, _("Send"));
        m_cancelBtn = new wxButton(bottomPanel, wxID_ANY, _("Cancel"));
        m_cancelBtn->Disable();
        m_clearBtn = new wxButton(bottomPanel, wxID_ANY, _("Clear history"));
        m_clearBtn->Disable();
// --- НАСТРОЙКА РАЗМЕРОВ ---


// 2. Вычисляем высоту для m_input (ровно 2 строки текста)
// Получаем высоту одного символа шрифта в пикселях

m_input->SetMinSize(wxSize(-1, inputHeight));
m_input->SetMaxSize(wxSize(-1, inputHeight)); // Фиксируем высоту, чтобы не росла
// 1. Фиксируем высоту кнопок (например, 80 пикселей)


m_sendBtn->SetMinSize(wxSize(-1, btnFixedH));
m_cancelBtn->SetMinSize(wxSize(-1, btnFixedH));
m_clearBtn->SetMinSize(wxSize(-1, btnFixedH));

// Опционально: запрещаем кнопкам растягиваться шире этого значения, если сайзер попытается
m_sendBtn->SetMaxSize(wxSize(-1, btnFixedH));
m_cancelBtn->SetMaxSize(wxSize(-1, btnFixedH));
m_clearBtn->SetMaxSize(wxSize(-1, btnFixedH));

// ---------------------------

//        m_status = new wxStaticText(bottomPanel, wxID_ANY, L"Готово");

        //bottomSizer->Add(new wxStaticText(bottomPanel, wxID_ANY, L"SQL-запрос:"),0, wxALL, 5);
        bottomSizer->Add(inputRow, 1, wxEXPAND | wxLEFT | wxRIGHT, 2);

        auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
        btnRow->Add(m_sendBtn,   1, wxRIGHT, 3);
        btnRow->Add(m_cancelBtn, 1, wxLEFT,  3);
        btnRow->Add(m_clearBtn,  1, wxLEFT,  3);
        bottomSizer->Add(btnRow, 0, wxALL, 2);
        //bottomSizer->Add(m_status, 0, wxALL, 2);
        
// ---------- Разделитель ----------
bottomPanel->SetSizer(bottomSizer);
bottomPanel->Layout();

// Минимальная высота нижней панели по содержимому
// (учтёт поле ввода, кнопки и статус — пересчитывать вручную не нужно)
const int bottomH = bottomSizer->CalcMin().GetHeight();


auto* mainSizer = new wxBoxSizer(wxVERTICAL);
mainSizer->Add(topPanel,    1, wxEXPAND);           // верх растягивается
mainSizer->Add(bottomPanel, 0, wxEXPAND);           // низ — по своему минимуму, навсегда
SetSizer(mainSizer);

        Bind(wxEVT_BUTTON,         &ctlChat::OnSend, this,m_sendBtn->GetId());
        Bind(wxEVT_BUTTON, &ctlChat::OnCancel, this, m_cancelBtn->GetId());
        Bind(wxEVT_BUTTON, &ctlChat::OnClearHistory, this, m_clearBtn->GetId());
        Bind(wxEVT_BUTTON, &ctlChat::OnHistoryUp,   this, m_upBtn->GetId());
        Bind(wxEVT_BUTTON, &ctlChat::OnHistoryDown, this, m_downBtn->GetId());
        m_input->Bind(wxEVT_KEY_DOWN, &ctlChat::OnKeyDown, this);

        m_client = new ChatClient();   // владелец отвечает за удаление

        m_client->SetParameters(
            settings->GetAIBaseURL(),
            settings->GetAIChatURL(),
            "",
            settings->GetAIParameters(),
            settings->GetAISystemPrompt()
        );
        m_client->OnAnswer = [this](const wxString& answer) {
            yesAnswer=true;
#ifdef _DEBUG
            SaveFile("answer.md", answer);
#endif
        };
        Bind(wxEVT_SHOW, [this](wxShowEvent& event) {
            if (event.IsShown() ) {
            CallAfter([this]() {
                        m_input->SetFocus();
                    });
            };
            event.Skip();
        });
        // Что делать при сбое и отмене
        m_client->OnState = [this](wxWebRequest::State st, const wxString& err) {
            switch (st) {
                case wxWebRequest::State_Failed:
                    ShowError(-1, err);
                    break;
                case wxWebRequest::State_Cancelled:
                    //if (m_closing) { Destroy(); return; }   // закрытие разрешено
                    if (m_closing) {
                        wxString msg=wxString::Format("%s %s.",_("Cancel"),_("The window can be closed"));
                        SetStatus(msg);
                    } else 
                    {
                        SetStatus(_("Cancel"));
                    }
                    
                    break;
                default:
                    break;
            }
        };
        // Конец запроса в любом случае — разблокируем интерфейс
        m_client->OnFinished = [this](const std::vector<ChatClient::Message>& history) {
            if (m_closing) return;
            m_sendBtn->Enable();
            m_clearBtn->Enable();
            m_cancelBtn->Disable();
            if (yesAnswer) {
                wxString all=m_client->GetHtmlAll();
                m_html->SetPage(all);
                m_html->ScrollPages(999999);
#ifdef _DEBUG
                SaveFile("html.html", all);
#endif
                SetStatus(_("Done"));
            }
            queryfrm->SetBitmapForoutputPane(this,wxNullColour);
        };
        m_input->Bind(wxEVT_CHAR_HOOK,[&](wxKeyEvent& event) {
            const int mods = event.GetModifiers();
            const int key  = event.GetKeyCode();
            if (mods == wxMOD_CONTROL && key == 'A' )
            {
                m_input->SelectAll();
                return;
            }
            if (mods == wxMOD_CONTROL && key == 'Z' )
            {
                 if (m_input->CanUndo())
                 {
                     m_input->Undo();
                    
                 }
                return;
            }

            event.Skip();
        } );
        m_html->Bind(wxEVT_HTML_CELL_CLICKED, [&](wxHtmlCellEvent& event) {
        //m_html->Bind(wxEVT_RIGHT_UP, [&](wxMouseEvent& event) {
                //wxPoint pt = event.GetPosition();
                if (!event.GetMouseEvent().RightUp() ) return;
                wxHtmlCell* cell = event.GetCell();
                if (!cell) return;
                //wxHtmlContainerCell *c=cell->GetParent();
                wxHtmlCell *c;
                c=cell;
                wxHtmlCell *ce=NULL;
                wxHtmlCell *start;
                // `inline` 
                if (cell->GetParent() ) start=cell->GetParent()->GetFirstChild();
                while (start) {
                    wxHtmlFontCell *cc = wxDynamicCast(start, wxHtmlFontCell);
                    if (cc) {
                        ce=start;
                    }
                    if (start==cell) break;
                    start=start->GetNext();
                }
                if (ce) c=ce; else ce=cell;
                bool isConainer=false;
                wxString str;
                while (c) {
                    wxHtmlFontCell *cc = wxDynamicCast(c, wxHtmlFontCell);
                    if (cc!=NULL) {
                        //ce=cc->GetParent();
                        c=c->GetNext();
                        while (c) {
                            wxHtmlFontCell *end = wxDynamicCast(c, wxHtmlFontCell);
                            if (end!=NULL) {
                                break;
                            }
                            str+=GetTextFromContainer(c);
                            c=c->GetNext();
                        }
                        ce=NULL;
                        break;
                    }
                    if (c->GetId().StartsWith("cell") ) {
                        wxString ss=c->GetId();
//                         std::cout << ss.c_str(); 
                         ce=c;
                         isConainer=true;
                         break;
                    }
                    if (c==ce) {
                        c=c->GetParent();
                        ce=c;
                        if (c) {
                            if (c->GetDepth()>0)
                                c=c->GetParent()->GetFirstChild();
                              else 
                                break;
                        }
                        if (!c) {
                            break;
                        }
                        continue;
                    }
                    c=c->GetNext();
                }
                if (ce) {
                        str=GetTextFromContainer(ce);
                        if (str.Right(1)=="\n") str=str.Left(str.Len()-1);
                }
                if (!str.IsEmpty() && wxTheClipboard->Open() )
                {
                    wxTheClipboard->SetData(new wxTextDataObject(str));
                    wxTheClipboard->Close();
                    int l=str.Length();
                    wxString aa=wxString::Format(wxPLURAL("%d char", "%d chars", l), l);
                    SetStatus(wxString::Format("%s %s" ,_("Copy done"),aa));
                }
                event.Skip();
        });
    }
 void ctlChat::SaveInputToHistory(const wxString& text)
    {
        // Дубликат подряд не добавляем — как в консоли
        if (m_textHistory.empty() || m_textHistory.back() != text)
            m_textHistory.push_back(text);
        m_histPos = -1;          // сброс навигации
        m_draft.clear();
        m_downBtn->Disable();
    }
    void ctlChat::OnKeyDown(wxKeyEvent& event)
    {
        const int mods = event.GetModifiers();
        const int key  = event.GetKeyCode();

        // Ctrl+Enter — отправка
        if (mods == wxMOD_CONTROL &&
            (key == WXK_RETURN || key == WXK_NUMPAD_ENTER))
        {
            if (m_sendBtn->IsEnabled())
                wxPostEvent(this, wxCommandEvent(wxEVT_BUTTON, m_sendBtn->GetId()));
            return;
        }

        // Ctrl+Up / Ctrl+Down — навигация по истории
        if (mods == wxMOD_CONTROL && key == WXK_UP)
        {
            if (m_upBtn->IsEnabled())
                wxPostEvent(this, wxCommandEvent(wxEVT_BUTTON, m_upBtn->GetId()));
            return;
        }
        if (mods == wxMOD_CONTROL && key == WXK_DOWN)
        {
            if (m_downBtn->IsEnabled())
                wxPostEvent(this, wxCommandEvent(wxEVT_BUTTON, m_downBtn->GetId()));
            return;
        }

        event.Skip();   // остальные клавиши — как обычно
    }

    void ctlChat::OnHistoryUp(wxCommandEvent&)
    {
        if (m_textHistory.empty())
            return;

        if (m_histPos == -1) {                 // начинаем листать: запоминаем черновик
            m_histPos = static_cast<int>(m_textHistory.size()) - 1;
            m_draft = m_input->GetValue();
        } else if (m_histPos > 0) {
            --m_histPos;
        } else {
            return;                            // самый старый текст, дальше некуда
        }
        m_input->SetValue(m_textHistory[m_histPos]);
        m_downBtn->Enable();
    }

    void ctlChat::SetStatus(const wxString msg) {
        
        wxString m=wxString::Format("%s:%s.",_("AI Chat"),msg);
        queryfrm->SetStatusText(m, STATUSPOS_MSGS);
    }
    void ctlChat::OnHistoryDown(wxCommandEvent&)
    {
        if (m_histPos == -1)                   // листание не начато
            return;

        if (m_histPos < static_cast<int>(m_textHistory.size()) - 1) {
            ++m_histPos;
            m_input->SetValue(m_textHistory[m_histPos]);
        } else {                               // вернулись к черновику
            m_histPos = -1;
            m_input->SetValue(m_draft);
            m_draft.clear();
            m_downBtn->Disable();
        }
    }
 void ctlChat::OnClearHistory(wxCommandEvent&)
    {
        if (!m_client->ClearHistory())
            return;                   // запрос активен — не трогаем
        m_html->SetPage("");
        m_textHistory.clear();
        m_histPos = -1;
        m_draft.clear();
        m_upBtn->Disable();
        m_downBtn->Disable();        
        SetStatus(_("History clear"));
    }
    void ctlChat::OnCancel(wxCommandEvent&) {
         m_client->Cancel();
    }
/**
 * @brief Запускает ассистента с заданным запросом
 * 
 * @param text 
 * @return true - упешно запущен
 * @return false - ассистент занят
 */
bool ctlChat::SendText(const wxString text) {
    if (m_client->IsBusy()) return false;
    m_input->SetValue(text);
    wxCommandEvent ev;
    OnSend(ev);
    return true;
}
wxString ctlChat::GetNameChat() {
    if (m_client->IsBusy()) 
         return '*'+_("AI Chat");
        else
         return _("AI Chat");
}

 void ctlChat::OnSend(wxCommandEvent& )
    {
        const wxString sql = m_input->GetValue();
        if (sql.empty())
            return;

        if (!m_client->Send(sql))     // занят или пустой ввод
            return;
        SaveInputToHistory(sql);      // <-- история текстов перед отправкой
        yesAnswer=false;
        m_sendBtn->Disable();
        m_clearBtn->Disable();
        m_cancelBtn->Enable();
        SetStatus(_("I think.."));
    wxColour clr(255,0,0);
    queryfrm->SetBitmapForoutputPane(this,clr);

    }
 bool ctlChat::RequestClose() {
    if (m_client->IsBusy())
    {
        m_closing = true;
        m_client->Cancel();
        return false;             // страницы пока нет — "veto"
    }
    return true;   
 }
    void ctlChat::ShowError(int status, const wxString& msg) {
        wxString m=wxString::Format("%s %d: %s",_("Error"), status, msg);
        SetStatus(m);
       // wxLogError("%s", msg);
    }

