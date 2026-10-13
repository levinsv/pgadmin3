#include <wx/wx.h>
#include <wx/tokenzr.h>
#include <vector>
#include "utils/misc.h"

// ---------- Палитры ----------
struct Palette
{
    const wxChar* pageBg;          // фон всей страницы
    const wxChar* textColor;       // основной текст
    const wxChar* headerColor;     // заголовки # .. ######
    const wxChar* linkColor;       // [текст](url)
    const wxChar* inlineCodeBg;    // фон `кода`
    const wxChar* inlineCodeColor; // цвет `кода`
    const wxChar* codeBlockBg;     // фон блока ```
      const wxChar* quoteBg;         // фон цитаты >
       const wxChar* tableBg; // фон ячеек таблицы
    const wxChar* tableHeaderBg; // фон шапки таблицы
    const wxChar* userBg; // фон запроса пользователя
};

// Индекс 0 — светлая тема, 1 — тёмная.
static const Palette kPalettes[2] =
{
    // Светлая
    { wxT("#ffffff"), wxT("#222222"),
      wxT("#111111"), // заголовки # .. ###### 
      wxT("#1a73e8"),
      wxT("#f1f1f1"), wxT("#c92a2a"), // inline and blocks
      wxT("#e1e1e1"), 
      wxT("#d1d1d1"),wxT("#ffffff"), //table
      wxT("#eef1f4"),
      wxT("#b2ffcf") // фон запроса пользователя
     },
    // Тёмная (в стиле GitHub Dark)
    { wxT("#1e1e1e"), wxT("#d4d4d4"),
      wxT("#ffffff"),
      wxT("#6cb6ff"),
      wxT("#1e1e1e"), wxT("#f84848ff"),wxT("#55595fff"),
      wxT("#9aa5bbff"),wxT("#1e1e1e"), wxT("#2d333b")
    ,wxT("#0d5227")
    }
};

static const Palette& GetPalette(bool isDark)
{
    return kPalettes[isDark ? 1 : 0];
}

// ---------- Экранирование спецсимволов HTML ----------
static void AppendEscaped(wxString& out, wxChar ch)
{
    if (ch == wxT('&'))       out << wxT("&amp;");
    else if (ch == wxT('<')) out << wxT("&lt;");
    else if (ch == wxT('>')) out << wxT("&gt;");
    else if (ch == wxT('"')) out << wxT("&quot;");
    else                     out << ch;
}
int idcells;
static wxString HtmlEscape(const wxString& s)
{
    wxString r;
    r.reserve(s.size() + 8);
    for (size_t i = 0; i < s.size(); ++i)
        AppendEscaped(r, s[i]);
    return r;
}

// ---------- Инлайн-разметка (рекурсивно, чтобы работала вложенность) ----------
static wxString ConvertInline(const wxString& src, size_t from, size_t to,
                              const Palette& pal);

static wxString ConvertInline(const wxString& s, const Palette& pal)
{
    return ConvertInline(s, 0, s.size(), pal);
}

static wxString ConvertInline(const wxString& src, size_t from, size_t to,
                              const Palette& pal)
{
    wxString out;
    size_t i = from;

    while (i < to) {
        const wxChar ch = src[i];

        // Инлайн-код: фон через bgcolor у <font> (wxWidgets 3.x)
        if (ch == wxT('`')) {
            size_t end = src.find(wxT('`'), i + 1);
            if (end != wxString::npos && end <= to) {
                out << wxT("<font bgcolor=\"") << pal.inlineCodeBg
                    << wxT("\" color=\"") << pal.inlineCodeColor
                    << wxT("\"><code>")
                    << HtmlEscape(src.Mid(i + 1, end - i - 1))
                    << wxT("</code></font>");
                i = end + 1;
                continue;
            }
            AppendEscaped(out, ch); ++i; continue;
        }

        // Картинка ![alt](url)
        if (ch == wxT('!') && i + 1 < to && src[i + 1] == wxT('[')) {
            size_t cb = src.find(wxT(']'), i + 2);
            if (cb != wxString::npos && cb + 1 < to && src[cb + 1] == wxT('(')) {
                size_t cp = src.find(wxT(')'), cb + 2);
                if (cp != wxString::npos && cp <= to) {
                    out << wxT("<img src=\"") << src.Mid(cb + 2, cp - cb - 2)
                        << wxT("\" alt=\"") << HtmlEscape(src.Mid(i + 2, cb - i - 2))
                        << wxT("\">");
                    i = cp + 1;
                    continue;
                }
            }
        }

        // Ссылка [текст](url)
        if (ch == wxT('[')) {
            size_t cb = src.find(wxT(']'), i + 1);
            if (cb != wxString::npos && cb + 1 < to && src[cb + 1] == wxT('(')) {
                size_t cp = src.find(wxT(')'), cb + 2);
                if (cp != wxString::npos && cp <= to) {
                    out << wxT("<a href=\"") << HtmlEscape(src.Mid(cb + 2, cp - cb - 2))
                        << wxT("\"><font color=\"") << pal.linkColor
                        << wxT("\">")
                        << ConvertInline(src, i + 1, cb, pal)
                        << wxT("</font></a>");
                    i = cp + 1;
                    continue;
                }
            }
        }

        // **жирный**, *курсив*, ~~зачёркнутый~~ (и синонимы __ и _)
        if (ch == wxT('*') || ch == wxT('_') || ch == wxT('~')) {
            const size_t mlen = (i + 1 < to && src[i + 1] == ch) ? 2 : 1;
            if (ch == wxT('~') && mlen == 1) { // одиночная ~ — обычный символ
                AppendEscaped(out, ch); ++i; continue;
            }
            const wxString marker(ch, mlen);

            size_t end = src.find(marker, i + mlen);
            if (end != wxString::npos && end <= to) {
                wxString inner = ConvertInline(src, i + mlen, end, pal);
                if (mlen == 2 && ch != wxT('~'))
                    out << wxT("<b>") << inner << wxT("</b>");
                else if (ch == wxT('~'))
                    out << wxT("<strike>") << inner << wxT("</strike>");
                else
                    out << wxT("<i>") << inner << wxT("</i>");
                i = end + mlen;
                continue;
            }
        }

        AppendEscaped(out, ch);
        ++i;
    }
    return out;
}


// ---------- Таблицы (GFM pipe tables) ----------
enum TableAlign { TABLE_ALIGN_DEFAULT, TABLE_ALIGN_LEFT, TABLE_ALIGN_CENTER, TABLE_ALIGN_RIGHT };

// Строка-разделитель шапки: | --- | :---: | ---: |
static bool IsTableSeparator(const wxString& line)
{
    wxString t = line;
    t.Trim(false).Trim(true);
    if (t.empty() || t.Find(wxT('|')) == wxString::npos) return false;
    bool hasDash = false;
    for (size_t i = 0; i < t.size(); ++i) {
        const wxChar c = t[i];
        if (c == wxT('-')) hasDash = true;
        else if (c != wxT(':') && c != wxT('|') && c != wxT(' ')) return false;
    }
    return hasDash;
}

// Разбивает строку таблицы на ячейки; поддерживает экранирование '\|'
static wxArrayString SplitTableRow(const wxString& line)
{
    wxString t = line;
    t.Trim(false).Trim(true);
    if (t.StartsWith(wxT("|"))) t.Remove(0, 1);
    if (!t.empty() && t.Last() == wxT('|')) t.RemoveLast();

    wxArrayString cells;
    wxString cur;
    bool isinline=false;
    for (size_t i = 0; i < t.size(); ++i) {
        const wxChar c = t[i];
        if (isinline)  {
            cur << c;
            if (c=='`') isinline=false;
            continue;    
        }
        if (c == wxT('\\') && i + 1 < t.size() && t[i + 1] == wxT('|')) {
            cur << wxT('|'); ++i; continue;
        }
        if (c=='`') isinline=true;
        if (c == wxT('|')) { cells.Add(cur); cur.Clear(); continue; }
        cur << c;
    }
    cells.Add(cur);
    return cells;
}

static wxString TableAlignAttr(TableAlign a)
{
    switch (a) {
        case TABLE_ALIGN_LEFT:   return wxT(" align=\"left\"");
        case TABLE_ALIGN_CENTER: return wxT(" align=\"center\"");
        case TABLE_ALIGN_RIGHT:  return wxT(" align=\"right\"");
    }
    return wxT("");
}

static void FlushTable(wxArrayString& rows, wxString& out, const Palette& pal)
{
    if (rows.empty()) return;

    wxArrayString header = SplitTableRow(rows[0]);
    const size_t cols = header.GetCount();

    // Выравнивание колонок из строки-разделителя
    std::vector<TableAlign> aligns(cols, TABLE_ALIGN_DEFAULT);
    if (rows.GetCount() > 1) {
        wxArrayString sep = SplitTableRow(rows[1]);
        for (size_t i = 0; i < cols && i < sep.GetCount(); ++i) {
            wxString s = sep[i]; s.Trim(false).Trim(true);
            const bool l = s.StartsWith(wxT(":"));
            const bool r = s.EndsWith(wxT(":"));
            if (l && r) aligns[i] = TABLE_ALIGN_CENTER;
            else if (r) aligns[i] = TABLE_ALIGN_RIGHT;
            else if (l) aligns[i] = TABLE_ALIGN_LEFT;
        }
    }

    out << wxT("<table width=\"100%\" border=\"1\" cellspacing=\"0\" cellpadding=\"5\" bgcolor=\"")
        << pal.tableBg << wxT("\">\n");

    // Шапка: bgcolor + жирный (wxHTML не всегда корректно понимает <th>)
    out << wxT("<tr>");
    for (size_t i = 0; i < cols; ++i) {
        wxString cell = (i < header.GetCount()) ? header[i] : wxString();
        cell.Trim(false).Trim(true);
        out << wxT("<td bgcolor=\"") << pal.tableHeaderBg << wxT("\"")
            << TableAlignAttr(aligns[i]) << wxT("><b>")
            << ConvertInline(cell, pal)
            << wxT("</b></td>");
    }
    out << wxT("</tr>\n");

    // Тело
    for (size_t r = 2; r < rows.GetCount(); ++r) {
        wxArrayString cells = SplitTableRow(rows[r]);
        out << wxT("<tr>");
        for (size_t i = 0; i < cols; ++i) {
            wxString cell = (i < cells.GetCount()) ? cells[i] : wxString();
            cell.Trim(false).Trim(true);
            out << wxT("<td id=\"cell")<< idcells++ << "\" " << TableAlignAttr(aligns[i]) << wxT(">")
                << ConvertInline(cell, pal)
                << wxT("</td>");
        }
        out << wxT("</tr>\n");
    }

    out << wxT("</table>\n");
    rows.Clear();
}


// ---------- Блочные элементы ----------
struct OpenList { int indent; bool ordered; };

static bool IsHrLine(wxString& line)
{
    wxString t = line.Trim(false).Trim(true);
    if (t.size() < 3) return false;
    const wxChar c = t[0];
    if (c != wxT('-') && c != wxT('*') && c != wxT('_')) return false;
    for (size_t i = 1; i < t.size(); ++i)
        if (t[i] != c) return false;
    return true;
}

// Разбирает "  - текст" или "  1. текст"
static bool IsListItem(const wxString& line, int& indent, bool& ordered, wxString& content)
{
    size_t i = 0;
    while (i < line.size() && line[i] == wxT(' ')) ++i;
    indent = (int)i;
    if (i >= line.size()) return false;

    const wxChar c = line[i];
    if (c == wxT('-') || c == wxT('*') || c == wxT('+')) {
        if (i + 1 < line.size() && line[i + 1] == wxT(' ')) {
            ordered = false;
            content = line.Mid(i + 2);
            return true;
        }
        return false;
    }
    if (wxIsdigit(c)) {
        size_t j = i;
        while (j < line.size() && wxIsdigit(line[j])) ++j;
        if (j + 1 < line.size() && line[j] == wxT('.') && line[j + 1] == wxT(' ')) {
            ordered = true;
            content = line.Mid(j+2);
            return true;
        }
    }
    return false;
}

static void CloseLists(wxString& out, std::vector<OpenList>& lists)
{
    while (!lists.empty()) {
        out << (lists.back().ordered ? wxT("</ol>\n") : wxT("</ul>\n"));
        lists.pop_back();
    }
}

static void ConvertBlock(const wxString& text, wxString& out, const Palette& pal)
{
    wxString src = text;
    src.Replace(wxT("\r\n"), wxT("\n"));
    src.Replace(wxT("\r"), wxT("\n"));

    wxArrayString lines = wxStringTokenize(src, wxT("\n"), wxTOKEN_RET_EMPTY_ALL);

    wxString para;                    // накопитель абзаца
    wxString codeBody;                // тело fenced-блока ```
    bool inCodeFence = false;
    bool inTable = false;
    wxArrayString tableRows;
    std::vector<OpenList> lists;

    auto flushPara = [&]() {
        if (para.Trim().empty()) { para.Clear(); return; }
        CloseLists(out, lists);
        out << wxT("<p>") << ConvertInline(para, pal) << wxT("</p>\n");
        para.Clear();
    };

    for (size_t li = 0; li < lines.GetCount(); ++li) {
        wxString line = lines[li];
        wxString t = line;
        t.Trim(false).Trim(true);

        // Внутри ``` — копируем как есть
        if (inCodeFence) {
            if (t.StartsWith(wxT("```"))) {
                out << HtmlEscape(codeBody) << wxT("</pre></td></tr></table>\n");
                codeBody.Clear();
                inCodeFence = false;
            } else {
                codeBody << line << wxT("\n");
            }
            continue;
        }

        if (t.StartsWith(wxT("```"))) {
            flushPara(); //CloseLists(out, lists);
            FlushTable(tableRows, out, pal); inTable = false;
            out << wxT("<table width=\"100%\" bgcolor=\"") << pal.codeBlockBg
                << wxT("\" border=\"0\" cellspacing=\"0\" cellpadding=\"6\">")
                << wxT("<tr><td id=\"cell") << idcells++ << "\"><pre>";
            codeBody.Clear();
            inCodeFence = true;
            continue;
        }
        // --- Таблицы: старт | a | b |, следом | --- | --- | ---
        if (!inTable && lists.empty() && t.Contains(wxT('|'))
            && li + 1 < lines.GetCount() && IsTableSeparator(lines[li + 1])) {
            flushPara(); CloseLists(out, lists);
            inTable = true;
            tableRows.Add(line);
            continue;
        }
        if (inTable) {
            if (IsTableSeparator(t))                    // разделитель после шапки — пропускаем
                continue;
            if (!t.empty() && t.Contains(wxT('|'))) {   // очередная строка таблицы
                tableRows.Add(line);
                continue;
            }
            // таблица закончилась — выводим, текущую строку обрабатываем дальше
            FlushTable(tableRows, out, pal);
            inTable = false;
            // fallthrough
        }

        if (t.empty() && (lists.size()==0)) { flushPara(); continue; }

        if (IsHrLine(t)) {
            flushPara(); CloseLists(out, lists);
            out << wxT("<hr width=\"100%\" noshade>\n");
            continue;
        }

        // Заголовки # .. ######
        if (t[0] == wxT('#')) {
            size_t lvl = 0;
            while (lvl < t.size() && t[lvl] == wxT('#')) ++lvl;
            if (lvl <= 6 && lvl < t.size() && t[lvl] == wxT(' ')) {
                flushPara(); CloseLists(out, lists);
                wxString content = t.Mid(lvl); content.Trim(false);
                out << wxT("<h") << (int)lvl << wxT("><font color=\"")
                    << pal.headerColor << wxT("\">")
                    << ConvertInline(content, pal)
                    << wxT("</font></h") << (int)lvl << wxT(">\n");
                continue;
            }
        }

        // Цитата: подряд идущие строки обрабатываем рекурсивно
        if (t[0] == wxT('>')) {
            flushPara(); CloseLists(out, lists);
            FlushTable(tableRows, out, pal); inTable = false;
            wxString qText;
            while (li < lines.GetCount()) {
                wxString q = lines[li]; q.Trim(false);
                if (!q.StartsWith(wxT(">"))) break;
                q.Remove(0, 1);
                if (q.StartsWith(wxT(" "))) q.Remove(0, 1);
                if (!qText.empty()) qText << wxT("\n");
                qText << q;
                ++li;
            }
            --li; // компенсируем ++li цикла for

            wxString inner;
            ConvertBlock(qText, inner, pal);
            out << wxT("<blockquote bgcolor=\"") << pal.quoteBg
                << wxT("\">") << inner << wxT("</blockquote>\n");
            continue;
        }

        // Элементы списков (со вложенностью по отступам)
        int indent=0; bool ordered; wxString content;
        if (IsListItem(line, indent, ordered, content)) {
            //flushPara();
            while (!lists.empty() && lists.back().indent > indent) {
                out << (lists.back().ordered ? wxT("</ol>\n") : wxT("</ul>\n"));
                lists.pop_back();
            }
            if (lists.empty() || lists.back().indent < indent) {
                lists.push_back({ indent, ordered });
                out << (ordered ? wxT("<ol>\n") : wxT("<ul>\n"));
            } else if (lists.back().ordered != ordered) {
                out << (lists.back().ordered ? wxT("</ol>\n") : wxT("</ul>\n"));
                lists.back().ordered = ordered;
                out << (ordered ? wxT("<ol>\n") : wxT("<ul>\n"));
            }
            out << wxT("<li>") << ConvertInline(content, pal) << wxT("</li>\n");
            continue;
        }
        if (indent>0) {
            out << ConvertInline(t, pal);
            continue;
        }
        // Обычная строка — в текущий абзац
        if (!para.empty()) para << wxT("\n");
        para << t;
    }

    if (inCodeFence) // незакрытый ``` — выводим то, что есть
        out << HtmlEscape(codeBody) << wxT("</code></pre></td></tr></table>\n");
    if (inTable)                       // таблица оборвалась на конце текста
        FlushTable(tableRows, out, pal);

    flushPara();
    CloseLists(out, lists);
}

// ---------- Точка входа ----------
wxString convertMDtoHtml(const wxString md, bool isDark, const wxString role)
{
    const Palette& pal = GetPalette(isDark);
    int idcells=1;
    if (role=="user") {
        wxString html=wxString::Format("<table width=\"100%\" bgcolor=\"%s\" border=\"0\" cellspacing=\"0\" cellpadding=\"6\"><tr><td id=\"cell0\"><pre>",pal.userBg)+escapeHtml(md,true)
                      +"</tr></td></pre></table>";

        return html;
    }

    wxString out;
    ConvertBlock(md, out, pal);
    return out;
}
wxString PageHtml(const wxString htmlinner, bool isDark) {
    const Palette& pal = GetPalette(isDark);
    wxString out;
    // Фон и цвет текста всей страницы задаём на <body> —
    // иначе при тёмной теме wxHtmlWindow оставит белый фон
    out = wxString(wxT("<html><body bgcolor=\"")) + pal.pageBg
        + wxT("\" text=\"") + pal.textColor + wxT("\">")
        + htmlinner + wxT("</body></html>");
    return out;

}
