//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// ctlSQLBox.cpp - SQL syntax highlighting textbox
//
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/sysopt.h>

// App headers
#include "db/pgSet.h"
#include "ctl/ctlSQLBox.h"
#include "dlg/dlgFindReplace.h"
#include "frm/menu.h"
#include "frm/frmMain.h"
#include "utils/sysProcess.h"
#include <wx/clipbrd.h>
#include <wx/aui/aui.h>
#include "utils/align/AlignWrap.h"
#include "utils/popuphelp.h"
#include "utils/FormatterSQL.h"
#include "utils/dlgTransformText.h"
#include "utils/TableColsMap.h"
#include "wx/display.h"

wxString ctlSQLBox::sqlKeywords;
static const wxString s_leftBrace(_T("([{"));
static const wxString s_rightBrace(_T(")]}"));
static const int s_indicHighlight(8);

// Additional pl/pgsql keywords we should highlight
wxString plpgsqlKeywords = wxT(" elsif exception exit loop raise record return text while call");
//
// Additional Text Search keywords we should highlight
wxString ftsKeywords = wxT(" gettoken lextypes headline init lexize");

// Additional pgScript keywords we should highlight
wxString pgscriptKeywords = wxT(" assert break columns continue date datetime file go lines ")
                            wxT(" log print record reference regexrmline string waitfor while");

BEGIN_EVENT_TABLE(ctlSQLBox, wxStyledTextCtrl)
	EVT_KEY_DOWN(ctlSQLBox::OnKeyDown)
	EVT_MENU(MNU_FIND, ctlSQLBox::OnSearchReplace)
	EVT_MENU(MNU_FUNC_HELP, ctlSQLBox::OnFuncHelp)
	EVT_MENU(MNU_TRANSFORM, ctlSQLBox::OnTransformText)
	EVT_MENU(MNU_TEXT_MARK, ctlSQLBox::OnTextMark)
	EVT_MENU(MNU_COPY, ctlSQLBox::OnCopy)
	EVT_MENU(MNU_AUTOCOMPLETE, ctlSQLBox::OnAutoComplete)
	EVT_KILL_FOCUS(ctlSQLBox::OnKillFocus)
	EVT_TIMER(TIMER_REFRESHUICARRET_ID, ctlSQLBox::OnRefreshUITimer)
//	EVT_ERASE_BACKGROUND(ctlSQLBox::OnBackGround)
#ifdef __WXMAC__
	EVT_STC_PAINTED(-1,  ctlSQLBox::OnPositionStc)
#else
	EVT_STC_UPDATEUI(-1, ctlSQLBox::OnPositionStc)
#endif
	EVT_STC_MARGINCLICK(-1, ctlSQLBox::OnMarginClick)
	EVT_STC_DOUBLECLICK(-1,ctlSQLBox::OnDoubleClick)
	EVT_END_PROCESS(-1,  ctlSQLBox::OnEndProcess)
END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(ctlSQLBox, wxStyledTextCtrl)


ctlSQLBox::ctlSQLBox()
{
	m_dlgFindReplace = 0;
	m_dlgTransformText = 0;
	m_autoIndent = false;
	m_autocompDisabled = false;
	process = 0;
	processID = 0;
	m_filename = wxEmptyString;
}


ctlSQLBox::ctlSQLBox(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style)
{
	m_dlgFindReplace = 0;
	m_dlgTransformText = 0;
	m_database = NULL;
	
	m_autocompDisabled = false;
	process = 0;
	processID = 0;
	m_hint_mode = false;
	Create(parent, id, pos, size, style);
}
//void ctlSQLBox::OnBackGround(wxEraseEvent &event) {
//	wxDC *dc=event.GetDC();
//	dc->SetBackground(wxBrush(GetBackgroundColour(), wxSOLID));
//	dc->Clear();
//
//}
wxColour ctlSQLBox::SetSQLBoxColourBackground(bool transaction) {
	wxColour bgColor = settings->GetSQLBoxColourBackground();
	if (settings->GetSQLBoxUseSystemBackground())
	{
		bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	}
	if (transaction) bgColor = wxColour(241, 241, 186);
	StyleSetBackground(wxSTC_STYLE_DEFAULT, bgColor);
//	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	return bgColor;
}
void ctlSQLBox::SetQueryBook(ctlAuiNotebook *query_book)
{
	sql_query_book=query_book;
}
void ctlSQLBox::OnRefreshUITimer(wxTimerEvent& event) {
	refreshUITimer->Stop();
	SetCaretWidthForKeyboardLayout();
	refreshUITimer->Start(250);
}
void ctlSQLBox::Create(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style)
{
	wxStyledTextCtrl::Create(parent, id , pos, size, style);
	fix_pos=-1;
	sql_query_book =NULL;
	// Clear all styles
	StyleClearAll();
	m_name=NULL;
    extern sysSettings* settings;
    wxJSONValue def(wxJSONType::wxJSONTYPE_OBJECT);
    wxJSONValue opt(wxJSONType::wxJSONTYPE_OBJECT);
	wxColour bookmarkcolor(70, 220, 234);
	int bookmarkalpha=70;
	def["bookmarkcolor"]= bookmarkcolor.GetAsString(wxC2S_HTML_SYNTAX);
	def["bookmarkalpha"]=bookmarkalpha;
	// Font
	settings->ReloadJsonFileIfNeed();
    settings->ReadJsonObect("ctlSQLBox", opt, def);
    //    settings->WriteJsonFile();
    if (!opt.IsNull()) {
		wxString txtcolor=opt["bookmarkcolor"].AsString();
		wxColour cc(txtcolor);
        if (!cc.IsOk()) opt["bookmarkalpha"]=def["bookmarkalpha"];
		int tmp=opt["bookmarkalpha"].AsInt();
        if (tmp<0 || tmp>255) opt["bookmarkalpha"]=def["bookmarkalpha"];
    }
    else opt = def;
	bookmarkcolor = wxColour(opt["bookmarkcolor"].AsString());
	bookmarkalpha = opt["bookmarkalpha"].AsInt();


	caretWidth=settings->GetWidthCaretForKeyboardLayout();
	refreshUITimer = new wxTimer(this, TIMER_REFRESHUICARRET_ID);
	refreshUITimer->Start(250);
	wxFont fntSQLBox = settings->GetSQLFont();
	wxColour bgColor=SetSQLBoxColourBackground(false);
	//wxColour bgColor = settings->GetSQLBoxColourBackground();
	//if (settings->GetSQLBoxUseSystemBackground())
	//{
	//	bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	//}

	wxColour frColor = settings->GetSQLBoxColourForeground();
	if (settings->GetSQLBoxUseSystemForeground())
	{
		frColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	}
//	StyleSetBackground(wxSTC_STYLE_DEFAULT, bgColor);
	StyleSetForeground(wxSTC_STYLE_DEFAULT, frColor);
	StyleSetFont(wxSTC_STYLE_DEFAULT, fntSQLBox);

	SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
	SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));

	SetCaretForeground(settings->GetSQLColourCaret());
	if (!settings->GetCaretUseSystemBackground()) {
		int r = bgColor.GetRed(); int g = bgColor.GetGreen(); int b = bgColor.GetBlue();
		if (r > 130) r = r - 20; else r = r + 20;
		if (g > 130) g = g - 20; else g = g + 20;
		if (b > 130) b = b - 20; else b = b + 20;
		//wxColour caretLine(r, g, b);
		wxColour caretLine(settings->GetCaretColourBackground());
		SetCaretLineBackground(caretLine);
		SetCaretLineVisible(true);
	}
	autoreplace=0;
	SetMarginWidth(1, 0);
	SetTabWidth(settings->GetIndentSpaces());
	SetUseTabs(!settings->GetSpacesForTabs());

	// Setup the different highlight colurs
	for (int i = 0; i < 34; ++ i )
	{
		if (i > 0 && i < 12)
			StyleSetForeground(i, settings->GetSQLBoxColour(i));
		else
			StyleSetForeground(i, frColor);
		StyleSetBackground(i, bgColor);
		StyleSetFont(i, fntSQLBox);
	}

	// Keywords in uppercase?

	if (settings->GetSQLKeywordsInUppercase())
		StyleSetCase(5, wxSTC_CASE_UPPER);

	// Margin style
	StyleSetBackground(wxSTC_STYLE_LINENUMBER, settings->GetSQLMarginBackgroundColour());

	// Brace maching styles
	StyleSetBackground(34, wxColour(0x99, 0xF9, 0xFF));
	StyleSetBackground(35, wxColour(0xFF, 0xCF, 0x27));
	StyleSetFont(34, fntSQLBox);
	StyleSetFont(35, fntSQLBox);

	// SQL Lexer and keywords.
	if (sqlKeywords.IsEmpty())
		FillKeywords(sqlKeywords);
	SetLexer(wxSTC_LEX_SQL);
	SetKeyWords(0, sqlKeywords + plpgsqlKeywords + ftsKeywords + pgscriptKeywords);

	// Enable folding
	SetMarginSensitive(2, true);

	SetMarginType(2, wxSTC_MARGIN_SYMBOL); // margin 2 for symbols
	SetMarginMask(2, wxSTC_MASK_FOLDERS);  // set up mask for folding symbols
	SetMarginSensitive(2, true);           // this one needs to be mouse-aware
	SetMarginWidth(2, 16);                 // set margin 2 16 px wide

	MarkerDefine(wxSTC_MARKNUM_FOLDEREND,     wxSTC_MARK_BOXPLUSCONNECTED,  *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED, *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER,  *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL,    wxSTC_MARK_LCORNER,  *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDERSUB,     wxSTC_MARK_VLINE,    *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDER,        wxSTC_MARK_BOXPLUS,  *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN,    wxSTC_MARK_BOXMINUS, *wxWHITE, *wxBLACK);

    MarkerDefine(1,wxSTC_MARK_ARROW,*wxBLACK,*wxGREEN);
	// for bookmark color
    IndicatorSetForeground(9, bookmarkcolor);
    IndicatorSetStyle(9, wxSTC_INDIC_STRAIGHTBOX);
	IndicatorSetAlpha(9,bookmarkalpha);
	SetProperty(wxT("fold"), wxT("1"));
	SetFoldFlags(16);

	// Setup accelerators
	wxAcceleratorEntry entries[6];
	entries[0].Set(wxACCEL_CTRL, (int)'F', MNU_FIND);
	entries[1].Set(wxACCEL_CTRL, WXK_SPACE, MNU_AUTOCOMPLETE);
	entries[2].Set(wxACCEL_CTRL, (int)'C', MNU_COPY);
	entries[3].Set(wxACCEL_CTRL, WXK_F1, MNU_FUNC_HELP);
	entries[4].Set(wxACCEL_CTRL, (int)'M', MNU_TRANSFORM);
	entries[5].Set(wxACCEL_CTRL, (int)'B', MNU_TEXT_MARK);
	wxAcceleratorTable accel(6, entries);
	SetAcceleratorTable(accel);

	// Autocompletion configuration
	AutoCompSetSeparator('\t');
	AutoCompSetChooseSingle(true);
	AutoCompSetIgnoreCase(true);
	AutoCompSetFillUps(wxT(" \t"));
	AutoCompSetDropRestOfWord(true);
	AutoCompSetMaxHeight(10);

	SetEOLMode(settings->GetLineEndingType());
}

void ctlSQLBox::SetDatabase(pgConn *db)
{
	m_database = db;
}

void ctlSQLBox::SetChanged(bool b)
{
	if (m_changed != b)
	{
		m_changed = b;
		UpdateTitle();
	}
}

bool ctlSQLBox::IsChanged()
{
	return m_changed;
}

void ctlSQLBox::SetOrigin(int origin)
{
	m_origin = origin;
}

int ctlSQLBox::GetOrigin()
{
	return m_origin;
}
bool ctlSQLBox::IsFileModification()
{
	if (!m_filename.IsEmpty()) {
		return time_file_mod != wxFileModificationTime(m_filename);
	}
	return false;
}

void ctlSQLBox::SetFilename(wxString &filename)
{
	m_filename = filename;
	time_file_mod = wxFileModificationTime(filename);
	UpdateTitle();
}

wxString ctlSQLBox::GetFilename()
{
	return m_filename;
}

void ctlSQLBox::SetTitle(wxString &title)
{
	m_title = title;
}

wxString ctlSQLBox::GetTitle(bool withChangeInd)
{
	wxString title = m_title;
	wxString chStr;
	if (!withChangeInd)
	{
		chStr = GetChangeIndicator();
		if (title.EndsWith(chStr))
			title = title.Mid(0, title.Len() - chStr.Len());
	}
	return title;
}

wxString ctlSQLBox::GetChangeIndicator()
{
	if (m_changestr.IsEmpty())
		m_changestr = _("*");
	return m_changestr;
}

void ctlSQLBox::UpdateTitle()
{
	bool hasCh = false;
	wxString chStr = GetChangeIndicator();
	wxString title = GetFilename();

	if (!title.IsEmpty())
		title = wxFileName::FileName(title).GetFullName();
	else
		title = GetTitle();

	hasCh = title.EndsWith(chStr);

	if (IsChanged() && !hasCh)
		title = title + chStr;
	else if (!IsChanged() && hasCh)
		title = title.Mid(0, title.Len() - chStr.Len());

	SetTitle(title);
}
void ctlSQLBox::OnTextMark(wxCommandEvent& ev) {
			int startPos = GetSelectionStart();
			int endPos = GetSelectionEnd();
			int curr=GetCurrentPos();
			int len=0;
			if (startPos!=endPos) {
				SetIndicatorCurrent(9);
				len=endPos-startPos;
				IndicatorFillRange(startPos,len);
			} else {
					int ind=IndicatorValueAt(9,curr);
					int epos=0;
					int spos=0;
					if (ind>0) {
						spos=IndicatorStart(9,curr);
						if (spos>=0) { 
							epos=IndicatorEnd(9,curr);
							len=epos-spos;
							//len=PositionRelative(spos,epos);
							SetIndicatorCurrent(9);
							IndicatorClearRange(spos,len);
						}
					} else {
							// goto 
								epos=IndicatorEnd(9,curr);
								if (epos==0) return;
								if (epos==GetTextLength()) {
									int ind=IndicatorValueAt(9,0);
								 	epos=IndicatorEnd(9,0);
									if (ind>0) {
										GotoPos(epos);
										return;
									}
										 
								}
								if (epos!=GetTextLength()) {
									epos=IndicatorEnd(9,epos);
									GotoPos(epos);
								}
									
					}
			}

}
void ctlSQLBox::OnTransformText(wxCommandEvent& ev) {
	wxString selText = GetSelectedText();
	if (!selText.IsEmpty())
	{
		//m_dlgTransformText->SetSource(selText);
	}
	else {
		if (wxTheClipboard->Open())
		{
			if (wxTheClipboard->IsSupported(wxDF_TEXT))
			{
				wxTextDataObject textData;
				wxTheClipboard->GetData(textData);
				selText = textData.GetText();
			}
			wxTheClipboard->Close();
		}

	}
	if (selText.IsEmpty()) return;
	if (!m_dlgTransformText)
	{
		wxString s;
		m_dlgTransformText = new dlgTransformText(this,s);
		m_dlgTransformText->SetSource(selText);
		m_dlgTransformText->Show(true);
	}
	else
	{
		m_dlgTransformText->SetSource(selText);
		m_dlgTransformText->Show(true);
		m_dlgTransformText->SetFocus();
	}


}
void ctlSQLBox::OnSearchReplace(wxCommandEvent &ev)
{
	if (!m_dlgFindReplace)
	{
		m_dlgFindReplace = new dlgFindReplace(this);
		m_dlgFindReplace->Show(true);
	}
	else
	{
		m_dlgFindReplace->Show(true);
		m_dlgFindReplace->SetFocus();
	}

	wxString selText = GetSelectedText();
	if (!selText.IsEmpty())
	{
		m_dlgFindReplace->SetFindString(selText);
	}

	m_dlgFindReplace->FocusSearch();
}

bool ctlSQLBox::Find(const wxString &find, bool wholeWord, bool matchCase, bool useRegexps, bool startAtTop, bool reverse, bool all)
{
	if (!DoFind(find, wxString(wxEmptyString), false, wholeWord, matchCase, useRegexps, startAtTop, reverse))
	{
		if (!all) {
			wxWindow *w = wxWindow::FindFocus();
			wxMessageBox(_("Reached the end of the document"), _("Find text"), wxICON_EXCLAMATION | wxOK, w);
		}
		return false;
	}
	return true;
}

bool ctlSQLBox::Replace(const wxString &find, const wxString &replace, bool wholeWord, bool matchCase, bool useRegexps, bool startAtTop, bool reverse)
{
	if (!DoFind(find, replace, true, wholeWord, matchCase, useRegexps, startAtTop, reverse))
	{
		wxWindow *w = wxWindow::FindFocus();
		wxMessageBox(_("Reached the end of the document"), _("Replace text"), wxICON_EXCLAMATION | wxOK, w);
		return false;
	}
	return true;
}

bool ctlSQLBox::ReplaceAll(const wxString &find, const wxString &replace, bool wholeWord, bool matchCase, bool useRegexps)
{
	// Use DoFind to repeatedly replace text
	int count = 0;
	int initialPos = GetCurrentPos();
	GotoPos(0);

	while(DoFind(find, replace, true, wholeWord, matchCase, useRegexps, false, false))
		count++;

	GotoPos(initialPos);

	wxString msg;
	msg.Printf(wxPLURAL("%d replacement made.", "%d replacements made.", count), count);
	wxMessageBox(msg, _("Replace all"), wxOK | wxICON_INFORMATION);

	if (count)
		return true;
	else
		return false;
}

bool ctlSQLBox::DoFind(const wxString &find, const wxString &replace, bool doReplace, bool wholeWord, bool matchCase, bool useRegexps, bool startAtTop, bool reverse)
{
	int flags = 0;
	int startPos = GetSelectionStart();
	int endPos = GetTextLength();

	// Setup flags
	if (wholeWord)
		flags |= wxSTC_FIND_WHOLEWORD;

	if (matchCase)
		flags |= wxSTC_FIND_MATCHCASE;

	// Replace the current selection, if there is one and it matches the find param.
	wxString current = GetSelectedText();
	if (doReplace)
	{
		if (useRegexps)
		{
			CharacterRange cr = RegexFindText(GetSelectionStart(), GetSelectionEnd(), find);
			if (GetSelectionStart() == cr.cpMin && GetSelectionEnd() == cr.cpMax)
			{
				if (cr.cpMin == cr.cpMax) // Must be finding a special char, such as $ (line end)
				{
					InsertText(cr.cpMax, replace);
					SetSelection(cr.cpMax, cr.cpMax + replace.Length());
					SetCurrentPos(cr.cpMax + replace.Length());

					// Stop if we've got to the end. This is important for the $
					// case where it'll just keep finding the end of the line!!
					if ((int)(cr.cpMin + replace.Length()) == GetLength())
						return false;
				}
				else
				{
					ReplaceSelection(replace);
					SetSelection(startPos, startPos + replace.Length());
					SetCurrentPos(startPos + replace.Length());
				}
			}
		}
		else if ((matchCase && current == find) || (!matchCase && current.Upper() == find.Upper()))
		{
			ReplaceSelection(replace);
			if (!reverse)
			{
				SetSelection(startPos, startPos + replace.Length());
				SetCurrentPos(startPos + replace.Length());
			}
			else
			{
				SetSelection(startPos + replace.Length(), startPos);
				SetCurrentPos(startPos);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////
	// Figure out the starting position for the next search
	////////////////////////////////////////////////////////////////////////

	if (startAtTop)
	{
		startPos = 0;
		endPos = GetTextLength();
	}
	else
	{
		if (reverse)
		{
			endPos = 0;
			startPos = GetCurrentPos();
		}
		else
		{
			endPos = GetTextLength();
			startPos = GetCurrentPos();
		}
	}

	size_t selStart = 0, selEnd = 0;

	if (useRegexps)
	{
		CharacterRange cr = RegexFindText(startPos, endPos, find);
		selStart = cr.cpMin;
		selEnd = cr.cpMax;
	}
	else
	{
		selStart = FindText(startPos, endPos, find, flags);
		selEnd = PositionRelative(selStart, find.Length());
		//selEnd = selStart + find.Length();
	}

	if (selStart != (size_t)(-1))
	{
		if (reverse)
		{
			SetCurrentPos(selStart);
			SetSelection(selEnd, selStart);
		}
		else
		{
			SetCurrentPos(selEnd);
			SetSelection(selStart, selEnd);
		}
		EnsureCaretVisible();
		return true;
	}
	else
		return false;
}
void ctlSQLBox::SetAutoReplaceList(queryMacroList *autorep) {
	if (!autorep) {
		autoreplace = queryMacroFileProvider::LoadAutoReplace(true);
	} else autoreplace=autorep;
}
void ctlSQLBox::SetDefFunction(wxArrayString &name, wxArrayString &def) {
	m_name=&name;
	m_def=&def;
}
void ctlSQLBox::OnCopy(wxCommandEvent& ev) {
	Copy();
}
void ctlSQLBox::OnFuncHelp(wxCommandEvent& ev) {
	
	FunctionPGHelper *fh=winMain->GetFunctionPGHelper();
	int pos = GetCurrentPos();
	if (!fh->isValid()) return;
	wxPoint p =  ClientToScreen( PointFromPosition(pos));
	wxString current = GetSelectedText();
	wxString key = "";
	if (!current.IsEmpty())
			key = current;
		else {
			wxChar ch;
			wxString tmp;
			while ((pos--) >= 0)
			{
				ch = GetCharAt(pos);
				if (ch < 35) { break; }
				if ( wxIsalnum(ch) || ch=='_' || ch> 255) tmp.Append(ch);
				//pos--;
			}

			for (int i = tmp.Len()-1; i >=0; i--) key+=tmp[i];
	}
	delete 	m_PopupHelp;
	wxSize rr(450, 370);
	m_PopupHelp = new popuphelp(this->GetParent(), key.Lower(), fh,p,rr);
	if (m_PopupHelp && m_PopupHelp->IsValid() && rr != m_PopupHelp->GetSizePopup()) {
		// recreate with new size
		rr = m_PopupHelp->GetSizePopup();
		delete 	m_PopupHelp;
		m_PopupHelp = new popuphelp(this->GetParent(), key.Lower(), fh, p, rr);

	}
	if (m_PopupHelp && m_PopupHelp->IsValid()) {
		//m_PopupHelp->UpdateWindowUI(true);
		wxSize top_sz=m_PopupHelp->GetSizePopup();
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
		if (oldp==p) p.x = p.x + 20;
		m_PopupHelp->Move(p);
		//m_PopupHelp->Position(p, wxSize(0, 17));
		m_PopupHelp->Popup();
	}

}
void ctlSQLBox::Colourise(int start, int end)
{
	wxStyledTextCtrl::Colourise(start, end);
	if (start == 0 && end == GetLength() && settings->GetUseHintWords()) {
		// build hint dictionary
		m_hint_words.clear();
		wxString s = GetText();
		int pos = start;
		int pend = -1;
		int endPos = end;
		int hword = pos;
		wxChar ch;
		int st;
		bool isword = false;
		while (pos < endPos) {
			ch = GetCharAt(pos);
			st = GetStyleAt(pos) & 0x1F;
			if (
				//				st != wxSTC_SQL_COMMENTLINE &&
				st == wxSTC_SQL_STRING
             || st == wxSTC_SQL_CHARACTER
				//				&& st != wxSTC_SQL_COMMENT
				)
			{
				if (isword) goto add_dic;
				goto nextchar;
			}
			if (wxIsalnum(ch)|| ch=='_') {
				if (!isword || pos==0) {
					isword = true; hword = pos;
					goto nextchar;
				}
				else
					goto nextchar;
			}
			else {
				// no word
				if (!isword) goto nextchar;
			}
			//
		add_dic:
			isword = false;
			if (hword >= 0) {
				wxString w = GetTextRange(hword, pos);
				if (w.length() > 1 && !wxIsdigit(w[0])) {
					auto [iter, has_been_inserted] = m_hint_words.insert(w);
				}
			}
			hword = -1;
			
nextchar:
#ifdef WIN32
			int i = IsDBCSLeadByte(ch) ? 2 : 1;
#else
			int i = 1;
			if (ch > 255) i = 2;
#endif
			pos = pos + i;
		}

	}
}
void ctlSQLBox::OnKeyDown(wxKeyEvent &event)
{
	if (event.GetKeyCode() == WXK_ESCAPE && m_PopupHelp) { delete m_PopupHelp; m_PopupHelp = NULL; }
	int pos = GetCurrentPos();
	wxChar ch = GetCharAt(pos - 1);
	wxChar nextch = GetCharAt(pos);
	int st = GetStyleAt(pos - 1);
			if (event.GetKeyCode() == WXK_LEFT) nextch=' ';
			if (event.GetKeyCode() == WXK_RIGHT) ch=' ';

	int match;
	BraceBadLight(wxSTC_INVALID_POSITION);
	// BraceHighlight(-1, -1);
	if ((fix_pos!=-1)) {
		//GetIndicatorCurrent()==s_indicHighlight
		SetIndicatorCurrent(s_indicHighlight);
		IndicatorClearRange(0,GetTextLength());
		fix_pos=-1;
	}

	// Check for braces that aren't in comment styles,
	// double quoted styles or single quoted styles
	if ((ch == '{' || ch == '}' ||
	        ch == '[' || ch == ']' ||
	        ch == '(' || ch == ')') &&
	        st != 1 && st != 2 && st != 6 && st != 7)
	{
		match = BraceMatch(pos - 1);
		if (match != wxSTC_INVALID_POSITION) {
			BraceHighlight(pos - 1, match);
		}
	}
	else if ((nextch == '{' || nextch == '}' ||
	          nextch == '[' || nextch == ']' ||
	          nextch == '(' || nextch == ')') &&
	         st != 1 &&st != 2 && st != 6 && st != 7)
	{
		match = BraceMatch(pos);
		if (match != wxSTC_INVALID_POSITION)
			BraceHighlight(pos, match);
	}
	int homewordpos = pos;
	bool istop = false;
	int sqllen = GetLength();
	if (sqllen < 100000) {

		while ((pos--) >= 0)
		{
			ch = GetCharAt(pos);
			st = GetStyleAt(pos);
			if ((ch == '{' || ch == '}' ||
				ch == '[' || ch == ']' ||
				ch == '(' || ch == ')') &&
				st != 1 && st != 2 && st != 6 && st != 7)
			{
				match = BraceMatch(pos);
				if (match == wxSTC_INVALID_POSITION)
				{
					BraceBadLight(pos);
				}
			}
			if ((wxIsalnum(ch) || ch == '_') && !istop && st != 7) homewordpos--; else istop = true;
		}
	}
	// hints words
	wxChar uc = event.GetUnicodeKey();
	bool rep = event.IsAutoRepeat();
	if (event.GetModifiers() == wxMOD_SHIFT && uc == '-') uc = '_';
	pos = GetCurrentPos();
	wxString s;
	wxString hint;
	int showpos;
	if (settings->GetUseHintWords()) {
		if (homewordpos >= 0 && homewordpos != pos && !rep) {
			bool isAddDict = false;
			s = GetTextRange(homewordpos, pos);
			if (uc != WXK_NONE)
			{
				if (uc == 8) {s = s.Left(s.length() - 1);}
				else {if (uc >= 'A') s += uc;}
			}
			if (uc >= ' ' && uc < 'A') isAddDict = true;
			auto it = m_hint_words.lower_bound(s);
			wxString strlist;
			if (s.length() > 0) {
				while (it != m_hint_words.end()) {
					hint = *it++;
					wxString f1 = s.Upper();
					wxString f2 = hint.substr(0, s.length()).Upper();
					if (!(f1 == f2)) {
						hint = "";
						break;
					}

					if (s.length() == hint.length()) {
						hint = "";
						isAddDict = false;
						continue;
					}
					break;
				}
			}
			showpos = PositionRelative(pos, -s.length());
			if (isAddDict && hint.IsEmpty()) {
				if (s.length() > 1 && !wxIsdigit(s[0])) {
					auto [iter, has_been_inserted] = m_hint_words.insert(s);
				}

			}
			if ((uc >= ' ' && uc < 'A')) hint = "";
		}
		if (!AutoCompActive()) {
			// work only hide autocomplite
		
			if (m_hint_mode) {
				if (event.GetKeyCode() == WXK_RIGHT && event.GetModifiers() == wxMOD_ALT && !hint.IsEmpty()) {
					wxString ins = hint.substr(s.length());
					InsertText(GetCurrentPos(), ins);
					SetCurrentPos(GetCurrentPos() + ins.Length());
					SetSelection(GetCurrentPos(), GetCurrentPos());
					CallTipCancel();
					m_hint_mode = false;
					return;
				}
				if (hint.IsEmpty() && !(event.GetModifiers() == wxMOD_ALT || event.GetModifiers() == wxMOD_CONTROL)) {
					m_hint_mode = false;
					CallTipCancel();
				}
				else {
					if (uc != WXK_NONE && !hint.IsEmpty()) {
						if (uc == 8 && CallTipActive()) {
							//CallTipSetPosAtStart(0);
							CallTipSetHighlight(s.length(), hint.length());
						}
						else {
							CallTipShow(showpos, hint);
							CallTipSetPosAtStart(0);
							CallTipSetHighlight(s.length(), hint.length());
							m_hint_mode = true;
						}
					}
				}
			}
			else {
				if (!CallTipActive() && !hint.IsEmpty() && !(event.GetModifiers() == wxMOD_ALT || event.GetModifiers() == wxMOD_CONTROL)) {
					if (uc != WXK_NONE) {
						if (uc == 8) {
							//showpos--;
							showpos = showpos - 1;
						}
						else showpos += 1;
						if (!CallTipActive()) CallTipShow(showpos, hint);
						CallTipSetPosAtStart(0);
						CallTipSetHighlight(s.length(), hint.length());
						m_hint_mode = true;
					}
					else {
						if (CallTipActive()) {
							CallTipCancel();
							m_hint_mode = false;
						}
					}
				}

			}
		}
		//
	}
	//wxString autoreplace[]={wxT("se"),wxT("select * from"),wxT("sc"),wxT("select count(*) from"),wxT("si"),wxT("select * from info_oper where"),wxT("sh"),wxT("select * from info_history where")};
#ifdef __WXGTK__
	event.m_metaDown = false;
#endif
	if (m_name)
	{
		if (((event.GetKeyCode() == '0')&&(event.GetModifiers() == (wxMOD_SHIFT)))
			||event.GetKeyCode() == WXK_UP
			||event.GetKeyCode() == WXK_DOWN)
		{

				if (CallTipActive()) CallTipCancel();
			event.Skip();
			m_hint_mode = false;
			return;

		}
		
		if ((event.GetKeyCode() == '9')&&(event.GetModifiers() == (wxMOD_SHIFT)))
		{
			int pos = GetCurrentPos() ;
			//CallTipSetBackground(*wxYELLOW);
			wxString line = GetLine(GetCurrentLine());
			//int max = line.Length() - (GetLineEndPosition(GetCurrentLine()) - GetCurrentPos()) - offset;
			int max= PositionFromLine(LineFromPosition(pos));
			int l=0;
			wxCharBuffer myStringChars = line.mb_str();
			for (int kk=pos-max-1;kk>=0;kk--) {
				//char c=myStringChars.data[kk];
				if ( line[kk].GetValue()<'0') break;
				l++;
			}
			wxString f_name=GetTextRange(pos-l, pos);
			int idx=m_name->Index(f_name,false,false);
			if (idx!=wxNOT_FOUND) {
						calltip=f_name +wxT("(")+m_def->Item(idx)+wxT(")");
						ct_hl=calltip.Find(wxT(","));
						if (ct_hl==wxNOT_FOUND) ct_hl=l;
						for (int jj=idx+1;jj<m_name->Count();jj++) {
							if (m_name->Item(jj)== f_name) {
								calltip+=wxT("\n")+f_name +wxT("(")+m_def->Item(jj)+wxT(")");
							} else break;
						}

						CallTipShow(pos-l,calltip);
					    CallTipSetHighlight(0,ct_hl);
						m_hint_mode = false;

			}
			//t = GetTextRange(pos-rpl.Length(), pos);

			event.Skip();
			return;
		}
	}
	if (CallTipActive()&&(event.GetKeyCode() == ','
					||event.GetKeyCode() == WXK_RIGHT
					||event.GetKeyCode() == WXK_LEFT
	)) {
		//int ptip=CallTipPosAtStart();
						int direction=1;
					    if (event.GetKeyCode() == WXK_LEFT)
							direction=-1;
						char c=GetCharAt(GetCurrentPos());
						if (event.GetKeyCode() != ','&&c!=',' ) {
							//direction=ct_hl;
						} else
						{
							int pos=ct_hl+direction;
							int a=0;
							while ( (pos<calltip.Len())&&(pos>0)) {
								c=calltip[pos].GetValue();
								if ((c==',')||(c=='\n')) {
									if (direction==1) break;
									if (a==1) break;
									a++;
									ct_hl=pos;
								}
								pos=pos+direction;
							}
							//int l=calltip.find(wxT(","),ct_hl+1);
							int prev=ct_hl;
							if (direction==-1) {
								prev=pos;
								pos=ct_hl;
							} else
							{
							if (pos==wxNOT_FOUND||(pos==calltip.length())) pos=ct_hl;
							}
							CallTipSetHighlight(prev,pos);
							
							ct_hl=pos;
						}
	}
	// Get the line ending type
	wxString lineEnd;
	switch (GetEOLMode())
	{
		case wxSTC_EOL_LF:
			lineEnd = wxT("\n");
			break;
		case wxSTC_EOL_CRLF:
			lineEnd = wxT("\r\n");
			break;
		case wxSTC_EOL_CR:
			lineEnd = wxT("\r");
			break;
	}
		// AutoReplace
		if (event.GetKeyCode() == ' ') {
	    //int start = GetSelectionStart();
		//int column = GetColumn(start);
		//int curLineNum = GetCurrentLine();
		int pos = GetCurrentPos();
		//wxString line = GetLine(GetCurrentLine());
		wxString t;
		wxString wc;
		//char c;
		bool f;
		wxString rpl = wxT("");
		wxString rpl2 = wxT("");
		for (int k=0;autoreplace!=0&&k<autoreplace->Count();k=k+1 ) {
			f=false;
			queryMacroItem *mi=autoreplace->GetItem(k);
			rpl = mi->GetName();
			//rpl2 =ss[k+1];
			//rpl=mi->GetName();
			rpl2 =mi->GetQuery();
			if ((pos-rpl.Length())==0) {
				t = GetTextRange(pos-rpl.Length(), pos);
				f=true;
			} else {t = GetTextRange(pos-rpl.Length()-1, pos);}
			//wc = GetTextRange(pos-rpl.Length()-1, pos-rpl.Length()-1);
			wxChar c='\00';
			if (t.Len()>0) c = t.GetChar(0);
			if (!((c >= '0' && c <= '9')) &&
			        !(c >= 'a' && c  <= 'z') &&
					!(c >= 'A' && c  <= 'Z') &&
			        !(c == '_'))
			{
				f=true;
			}
			

		if (t.EndsWith(rpl)&&f)
		{
				//SetTargetStart(pos-2);
				//SetTargetEnd(pos);
				//ReplaceTarget(rpl2);

			SetSelection(pos-rpl.Length(),pos);
			// Lose any selected text.
			ReplaceSelection(wxEmptyString);
			// Insert a replacement \n (or whatever), and the indent at the insertion point.
			InsertText(GetCurrentPos(), rpl2);
			// Now, reset the position, and clear the selection
			SetCurrentPos(GetCurrentPos() + rpl2.Length());
			SetSelection(GetCurrentPos(), GetCurrentPos());
			break;
		}
		}
		}
	// Block comment/uncomment
	if (event.GetKeyCode() == 'K')
	{
		// Comment (Ctrl+k)
		if (event.GetModifiers() == wxMOD_CONTROL)
		{
			if (BlockComment(false))
				return;
		}
		// Uncomment (Ctrl+Shift+K)
		else if (event.GetModifiers() == (wxMOD_CONTROL | wxMOD_SHIFT))
		{
			if (BlockComment(true))
				return;
		}
	}

	// Autoindent
	if (m_autoIndent && event.GetKeyCode() == WXK_RETURN)
	{
		wxString indent, line;
		line = GetLine(GetCurrentLine());

		// Get the offset for the current line - basically, whether
		// or not it ends with a \r\n, \n or \r, and if so, the length
		int offset =  0;
		if (line.EndsWith(wxT("\r\n")))
			offset = 2;
		else if (line.EndsWith(wxT("\n")))
			offset = 1;
		else if (line.EndsWith(wxT("\r")))
			offset = 1;

		// Get the indent. This is every leading space or tab on the
		// line, up until the current cursor position.
		int x = 0;
		int max = line.Length() - (GetLineEndPosition(GetCurrentLine()) - GetCurrentPos()) - offset;
		if(line != wxEmptyString)
		{
			while ((x < max) &&(line[x].GetValue() == '\t' || line[x].GetValue() == ' ')) {
				wxChar ccc=line[x];
				indent += line[x++];
			}
		}

		// Select any indent in front of the cursor to be removed. If
		// the cursor is positioned after any non-indent characters,
		// we don't remove anything. If there is already some selected,
		// don't select anything new at all.
		if (indent.Length() != 0 &&
		        (unsigned int)GetCurrentPos() <= ((GetLineEndPosition(GetCurrentLine()) - line.Length()) + indent.Length() + offset) &&
		        GetSelectedText() == wxEmptyString)
			SetSelection(GetLineEndPosition(GetCurrentLine()) - line.Length() + offset, GetLineEndPosition(GetCurrentLine()) - line.Length() + indent.Length() + offset);

		// Lose any selected text.
		ReplaceSelection(wxEmptyString);

		// Insert a replacement \n (or whatever), and the indent at the insertion point.
		InsertText(GetCurrentPos(), lineEnd + indent);

		// Now, reset the position, and clear the selection
		SetCurrentPos(GetCurrentPos() + indent.Length() + lineEnd.Length());
		SetSelection(GetCurrentPos(), GetCurrentPos());
	}
	else if (m_dlgFindReplace && event.GetKeyCode() == WXK_F3)
	{
		m_dlgFindReplace->FindNext();
	}
	else
		event.Skip();
}
bool ctlSQLBox::BlockDouble(bool undouble)
{
	int start = GetSelectionStart();

	if (!GetSelectedText().IsEmpty())
	{
		wxString selection = GetSelectedText();
		if (!undouble)
		{
			selection.Replace("'", "''");
		}
		else
		{
			selection.Replace("''", "'");

		}
		ReplaceSelection(selection);
		SetSelection(start, start + selection.Length());
	}
	return true;
}
bool ctlSQLBox::BlockComment(bool uncomment)
{
	wxString lineEnd;
	switch (GetEOLMode())
	{
		case wxSTC_EOL_LF:
			lineEnd = wxT("\n");
			break;
		case wxSTC_EOL_CRLF:
			lineEnd = wxT("\r\n");
			break;
		case wxSTC_EOL_CR:
			lineEnd = wxT("\r");
			break;
	}

	// Save the start position
	const wxString comment = wxT("-- ");
	int start = GetSelectionStart();

	if (!GetSelectedText().IsEmpty())
	{
		wxString selection = GetSelectedText();
		if (!uncomment)
		{
			selection.Replace(lineEnd, lineEnd + comment);
			selection.Prepend(comment);
			if (selection.EndsWith(comment))
				selection = selection.Left(selection.Length() - comment.Length());
		}
		else
		{
			selection.Replace(lineEnd + comment, lineEnd);
			if (selection.StartsWith(comment))
				selection = selection.Right(selection.Length() - comment.Length());
		}
		ReplaceSelection(selection);
		SetSelection(start, start + selection.Length());
	}
	else
	{
		// No text selection - (un)comment the current line
		int column = GetColumn(start);
		int curLineNum = GetCurrentLine();
		int pos = PositionFromLine(curLineNum);

		if (!uncomment)
		{
			InsertText(pos, comment);
		}
		else
		{
			wxString t = GetTextRange(pos, pos + comment.Length());
			if (t == comment)
			{
				// The line starts with a comment, so we remove it
				SetTargetStart(pos);
				SetTargetEnd(pos + comment.Length());
				ReplaceTarget(wxT(""));
			}
			else
			{
				// The line doesn't start with a comment, do nothing
				return false;
			}
		}

		if (GetLineCount() > curLineNum)
		{
			wxString nextLine = GetLine(curLineNum + 1);
			if (nextLine.EndsWith(lineEnd))
				nextLine = nextLine.Left(nextLine.Length() - lineEnd.Length());

			int nextColumn = (nextLine.Length() < (unsigned int)column ? nextLine.Length() : column);
			GotoPos(PositionFromLine(curLineNum + 1) + nextColumn);
		}
	}

	return true;
}

void ctlSQLBox::OnKillFocus(wxFocusEvent &event)
{
	AutoCompCancel();
	event.Skip();
}

void ctlSQLBox::UpdateLineNumber()
{
	bool showlinenumber;

	settings->Read(wxT("frmQuery/ShowLineNumber"), &showlinenumber, false);
	if (showlinenumber)
	{
		long int width = TextWidth(wxSTC_STYLE_LINENUMBER,
		                           wxT(" ") + NumToStr((long int)GetLineCount()) + wxT(" "));
		if (width != GetMarginWidth(0))
		{
			SetMarginWidth(0, width);
#ifndef __WXGTK__
			Update();
#endif
		}
	}
	else
	{
		SetMarginWidth(0, 0);
	}
}

void ctlSQLBox::OnEndProcess(wxProcessEvent &ev)
{
	if (process)
	{
		processErrorOutput = process->ReadErrorStream();
		processOutput += process->ReadInputStream();
		processExitCode = ev.GetExitCode();
		delete process;
		process = 0;
		processID = 0;
	}
}

wxString ctlSQLBox::ExternalFormat(int typecmd)
{
	wxString msg;
	processOutput = wxEmptyString;

	bool isSelected = true;
	wxString processInput = GetSelectedText();
	if (processInput.IsEmpty())
	{
		processInput = GetText();
		isSelected = false;
	}
	if (processInput.IsEmpty())
		return _("Nothing to format.");

	wxString formatCmd = settings->GetExtFormatCmd();
	wxString msgword = "formatt";
	if (typecmd == 1) {
		formatCmd = settings->GetExtAlignCmd();
		msgword = "align";
	}
	if (formatCmd.IsEmpty())
	{
		if (typecmd == 1) {
			//internal align
			AlignWrap a;
			wxString lineEnd;
			switch (GetEOLMode())
			{
			case wxSTC_EOL_LF:
				lineEnd = wxT("\n");
				break;
			case wxSTC_EOL_CRLF:
				lineEnd = wxT("\r\n");
				break;
			case wxSTC_EOL_CR:
				lineEnd = wxT("\r");
				break;
			}
			wxArrayString choiceCmpOpts;
			
			choiceCmpOpts.Add(_("All line (use all EOL)"));
			choiceCmpOpts.Add(_("First line pattern (ignore all but the first EOL)"));
			choiceCmpOpts.Add(_("Try looking for patterns above"));
			choiceCmpOpts.Add(_("Compact view"));
			choiceCmpOpts.Add(_("Remove multi spaces"));
			wxMultiChoiceDialog dialog(this,
				_("A multi-choice convenience dialog"),
				_("Please select several align options"),
				choiceCmpOpts);
			dialog.SetSelections(choiceSelectOpts);
			int cfg = 0;
			if (dialog.ShowModal() == wxID_OK) {
				choiceSelectOpts = dialog.GetSelections();

				for (size_t n = 0; n < choiceSelectOpts.GetCount(); n++) {
						if (choiceSelectOpts[n] == 0) cfg |= AlignWrap::ALL_LINES;
						if (choiceSelectOpts[n] == 1 ) cfg |= AlignWrap::FIRST_LINE ;
						if (choiceSelectOpts[n] == 2) cfg |= AlignWrap::FIND_UP_LONG_LINE;
						if (choiceSelectOpts[n] == 3) cfg |= AlignWrap::COMPACT_VIEW;
						if (choiceSelectOpts[n] == 4) cfg |= AlignWrap::ONLY_SINGLE_SPACE;

				}
				if (CHKCFGPARAM(cfg, AlignWrap::ALL_LINES) && CHKCFGPARAM(cfg, AlignWrap::FIRST_LINE)) cfg -= AlignWrap::FIRST_LINE;
			}
			else return _("Cancel");


			processOutput=a.build(processInput, cfg, lineEnd);
			if (isSelected)
				ReplaceSelection(processOutput);
			else
				SetText(processOutput);

			return _("" + msgword + "ing complete.");
		}
		if (typecmd == 0) {
			FSQL::FormatterSQL f(processInput);
			int rez = f.ParseSql(0);
			if (rez >= 0) {
				wxRect rr(0, 0, 120, 2000);
				wxString processOutput = f.Formating(rr);
				if (isSelected)
					ReplaceSelection(processOutput);
				else
					SetText(processOutput);
				return _("Formatting Ok");
			} else
				return wxString::Format("Error parse sql %d",rez);
		}
		return _("You need to setup a "+msgword+"ing command");
	}

	if (process)
	{
		delete process;
		process = NULL;
		processID = 0;
	}
	processOutput = wxEmptyString;
	processErrorOutput = wxEmptyString;
	processExitCode = 0;

	process = new sysProcess(this, wxConvUTF8);
	processID = wxExecute(formatCmd, wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER, process);
	if (!processID)
	{
		delete process;
		process = NULL;
		processID = 0;
		msg = _("Couldn't run " + msgword + "ing command: ") + formatCmd;
		return msg;
	}
	process->WriteOutputStream(processInput);
	process->CloseOutput();

	int timeoutMs = settings->GetExtFormatTimeout();
	int timeoutStepMs = 100;
	int i = 0;
	while (process && i * timeoutStepMs < timeoutMs)
	{
		wxSafeYield();
		if (process)
			processOutput += process->ReadInputStream();
		wxSafeYield();
		wxMilliSleep(timeoutStepMs);
		i++;
	}

	if (process)
	{
		AbortProcess();
		return wxString::Format(_("" + msgword + "ing command did not respond in %d ms"), timeoutMs);
	}

	if (processExitCode != 0)
	{
		processErrorOutput.Replace(wxT("\n"), wxT(" "));
		msg = wxString::Format(_("Error %d: "), processExitCode) + processErrorOutput;
		return msg;
	}
	else if (processOutput.Trim().IsEmpty())
	{
		return _("" + msgword + "ing command error: Output is empty.");
	}
	if (isSelected)
		ReplaceSelection(processOutput);
	else
		SetText(processOutput);

	return _("" + msgword + "ing complete.");
}

void ctlSQLBox::AbortProcess()
{
	if (process && processID)
	{
#ifdef __WXMSW__
		// SIGTERM is useless for Windows console apps
		wxKill(processID, wxSIGKILL, NULL, wxKILL_CHILDREN);
#else
		wxKill(processID, wxSIGTERM, NULL, wxKILL_CHILDREN);
#endif
		processID = 0;
	}
}
std::pair<int,int> ctlSQLBox::SelectQuery(int startposition)
{
	struct result { int start; int end; };
	int pos = GetCurrentPos();
	wxChar ch = GetCharAt(pos - 1);
	wxChar nextch = GetCharAt(pos);
	int st = GetStyleAt(pos - 1);
	int endPos = GetLength();
	int pend=endPos;
	int pstart=0;

	if (ch == ';') {
		pend=pos;
		pos=pos-1;
	} else
	{
		while (pos<endPos) {
		ch = GetCharAt(pos);
		st = GetStyleAt(pos) & 0x1F;
		if ((ch == ';') &&
	        st != wxSTC_SQL_COMMENTLINE &&
			st != wxSTC_SQL_STRING &&
			st != wxSTC_SQL_CHARACTER &&
			st!= wxSTC_SQL_COMMENT)
		{
			pend=pos;
			break;
		}
		#ifdef WIN32
		int i=IsDBCSLeadByte(ch)? 2 : 1;
		#else
		int i=1;
		if (ch>255) i=2;
		#endif
		pos=pos+i;
		}
		
	}
		pos--;
		while ((pos) >= 0)
	{
		ch = GetCharAt(pos);
		st = GetStyleAt(pos) & 0x1F;
		if ((ch == ';') &&
			st != wxSTC_SQL_COMMENTLINE &&
			st != wxSTC_SQL_STRING &&
			st != wxSTC_SQL_CHARACTER &&
			st != wxSTC_SQL_COMMENT
			)
		{
			//pstart=pos+1;
			break;
		}
		if (ch>' ') pstart=pos;
		#ifdef WIN32
		int i=IsDBCSLeadByte(ch)? 2 : 1;
		#else
		int i=1;
		if (ch>255) i=2;
		#endif
		pos=pos-i;

	}
		if (startposition<0) SetSelection(pstart,pend);
		//return result {pstart,pend};
		return std::make_pair(pstart, pend);
		
}
void ctlSQLBox::HighlightBrace(int start, int len, int inicator) {
            {
                //SetCaretForeground(wxColour(255, 0, 0));
                //SetCaretWidth(caretWidth + 1);
				if (inicator==8) { // double click word
                	IndicatorSetForeground(inicator, wxColour(80, 236, 120));
                	IndicatorSetStyle(inicator, wxSTC_INDIC_STRAIGHTBOX);
				}
#ifndef wxHAVE_RAW_BITMAP
                IndicatorSetUnder(s_indicHighlight, true);
#endif
                SetIndicatorCurrent(inicator);
                IndicatorFillRange(start, len);
                return;
            }

}
void ctlSQLBox::OnDoubleClick(wxStyledTextEvent &event)
{
	fix_pos = GetCurrentPos();
	wxString find = GetSelectedText();
	int flags =0;
		flags |= wxSTC_FIND_WHOLEWORD;
		flags |= wxSTC_FIND_MATCHCASE;

	size_t selStart = 0, selEnd = 0;
	if (!find.IsEmpty())
	{
			int startPos = GetSelectionStart();
			int endPos = GetTextLength();

                IndicatorSetForeground(s_indicHighlight, wxColour(246, 185, 100));
				IndicatorSetAlpha(s_indicHighlight,50);
                IndicatorSetStyle(s_indicHighlight, wxSTC_INDIC_ROUNDBOX);
#ifndef wxHAVE_RAW_BITMAP
                IndicatorSetUnder(s_indicHighlight, true);
#endif
                SetIndicatorCurrent(s_indicHighlight);
				startPos=0;
		while ((selStart != (size_t)(-1))) {

		selStart = FindText(startPos, endPos, find, flags);
		selEnd = selStart + find.Length();
		if ((selStart != (size_t)(-1))) {
                IndicatorFillRange(selStart, find.Length());
				//IndicatorFillRange(rb, 1);
				startPos=selEnd;
		}

		}
	}
}
void ctlSQLBox::SetCaretWidthForKeyboardLayout() {
	int currentwidth = GetCaretWidth();
	int newwidth = currentwidth;
#ifdef __WXMSW__
	HKL la = GetKeyboardLayout(0);
	if (((long long)la & 0xFFFF) == 0x409) {
		//en
		newwidth = 1;
	}
	else {
		// locale
		newwidth = caretWidth;
	}
#endif
	if (newwidth != currentwidth) SetCaretWidth(newwidth);
	return;
}
void ctlSQLBox::OnPositionStc(wxStyledTextEvent &event)
{
	int pos = GetCurrentPos();
	wxChar ch = GetCharAt(pos - 1);
	wxChar nextch = GetCharAt(pos);
	int st = GetStyleAt(pos - 1);
	int match;
	// Line numbers
	// Ensure we don't recurse through any paint handlers on Mac
#ifdef __WXMAC__
	Freeze();
#endif
	UpdateLineNumber();
#ifdef __WXMAC__
	Thaw();
#endif
	int startsql=0;
	// Roll back through the doc and highlight any unmatched braces
	int tmp=pos;
	list_table.Clear();
	int sqllen = GetLength();
	if (sqllen < 100000) {
		while ((pos--) >= 0)
		{
			ch = GetCharAt(pos);
			st = GetStyleAt(pos);
			if (st != 1 && st != 2 && st != 6 && st != 7 && (ch == ';') && startsql == 0) startsql = pos + 1;
			if ((ch == '{' || ch == '}' ||
				ch == '[' || ch == ']' ||
				ch == '(' || ch == ')') &&
				st != 1 && st != 2 && st != 6 && st != 7)
			{
				match = BraceMatch(pos);
				if (match == wxSTC_INVALID_POSITION)
				{
					event.Skip();
					return;
				}
			}
		}
	}
	// position select
	pos=tmp;
	wxString keyword;
	wxString ident;
	while ((pos--) >= 0)
	{
		ch = GetCharAt(pos);
		st = GetStyleAt(pos);
		if (st==5) {ident.append(ch);} else
		{
			if ((!ident.IsEmpty())) {

			if (ident.CmpNoCase(wxT("tceles"))==0) {
				startsql=pos+1;
				break;
			}
			ident.Clear();
			}
			
		}
		if (st != 1 &&st != 2 && st != 6 && st != 7 &&(ch==';')&&startsql==0) startsql=pos+1;
		if (( ch == '}' ||
		         ch == ']' ||
		         ch == ')') &&
		        st != 1 &&st != 2 && st != 6 && st != 7)
		{
			match = BraceMatch(pos);
			if (match == wxSTC_INVALID_POSITION || (match>=pos))
			{
				event.Skip();
				return;
			} else
			{
				pos=match-1;
			}
		}
	}

	
	int endtext= GetLength();
	bool isfrom=false;
	pos=startsql;
	
	ident.Clear();
	while (pos<endtext) {
		ch = GetCharAt(pos);
		st = GetStyleAt(pos);
		if (st != 1 &&st != 2 && st != 6 && st != 7 &&((ch=='\r')||(ch=='\n'))) {pos++; continue;};
		if (st != 1 &&st != 2 && st != 6 && st != 7 &&(ch==';')) break;
		if (st==11||st==6) {ident.append(ch);}
		if (st==5) {keyword.append(ch);
		} else {
			if ((keyword.CmpNoCase(wxT("from"))==0)) {
				ident.Clear();
				isfrom=true;
			} else if (keyword.CmpNoCase(wxT("join"))==0) {
				list_table.Append(wxT(","));
				ident.Clear();
				isfrom=true;
			} else if (keyword.CmpNoCase(wxT("on"))==0) {
				list_table.Append(wxT(","));
				isfrom=false;
			}else if (keyword.CmpNoCase(wxT("where"))==0
				    ||keyword.CmpNoCase(wxT("group"))==0
					||keyword.CmpNoCase(wxT("union"))==0
					||keyword.CmpNoCase(wxT("limit"))==0
					||keyword.CmpNoCase(wxT("for"))==0
					||keyword.CmpNoCase(wxT("window"))==0
					) {
				//list_table.Append(ident);
				isfrom=false;
				break;
			}
			if (!keyword.IsEmpty()) keyword.Clear();
		}
		if ((ch == '{' || ch == '}' ||
		        ch == '[' || ch == ']' ||
		        ch == '(' || ch == ')') &&
		        st != 1 &&st != 2 && st != 6 && st != 7)
		{
			match = BraceMatch(pos);
			if (match == wxSTC_INVALID_POSITION)
			{
				//BraceBadLight(pos);
				break;
			} else
			{
				if (match>pos) pos=match;
				ident.Clear();
			}
		}
		if (st != 1 &&st != 2 && st != 6 && st != 7 &&(ch==';')) break;
		if ((st==10||st==0)&&(!ident.IsEmpty())) {
			if (isfrom) {
				list_table.Append(wxT(" "));
				list_table.Append(ident);
				if (ch==',') list_table.Append(ch);
				//if (!name.IsEmpty()) alias=ident; else name=ident;
			}
			ident.Clear();
		}
		pos++;

	}
	if (isfrom&&(!ident.IsEmpty())) {list_table.Append(wxT(" "));list_table.Append(ident);};


	event.Skip();
}


void ctlSQLBox::OnMarginClick(wxStyledTextEvent &event)
{
	if (event.GetMargin() == 2)
		ToggleFold(LineFromPosition(event.GetPosition()));

	event.Skip();
}
wxString ctlSQLBox::TextToHtml(int start, int end,bool isAddNewLine) {
	wxColor frColor[40];
	wxString str;
	wxColour frc = settings->GetSQLBoxColourForeground();
	wxString selText = GetTextRange(start,end);
	if (settings->GetSQLBoxUseSystemForeground())
	{
		frc = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	}

	for (int i = 0; i < 34; ++i)
	{
		frColor[i] = StyleGetForeground(i);
		if (i > 0 && i < 12)
			//StyleSetForeground(i, settings->GetSQLBoxColour(i));
			frColor[i] = StyleGetForeground(i);
		else
			frColor[i] = frc;

		//StyleSetBackground(i, bgColor);
		//StyleSetFont(i, fntSQLBox);
	}
	//<h1 style="color:blue;">
	int endp = end;
	int startp = start;
	wxString prevColor = wxEmptyString;
	wxString tColor;
	wxFont fntSQLBox = settings->GetSQLFont();
	wxString fontName = fntSQLBox.GetFaceName();
	wxString sz;
	sz.Printf("%d", fntSQLBox.GetPixelSize().GetHeight());

	//str = wxT("<div style=\"font-family: ") + fontName + wxT("; font-size: " + sz + "px\"><font>");
	str = wxString::Format("<div style=\"font-family: %s; font-size: %spx\"><font face=\"%s\" >", fontName, sz, fontName);
	int k = 0;
	int l = 1;
	wxString newline = "<br>";
	if (isAddNewLine) newline = L"\u2936<br>";
	//if (isAddNewLine) newline = L"&ldca;<br>";
	//if (isAddNewLine) newline = L"<br>\x0b78";
	int lenstr = selText.Length();
	while (k<lenstr) {
		int st = GetStyleAt(startp);
		if (st < 34) tColor = frColor[st].GetAsString(wxC2S_HTML_SYNTAX);
		if (prevColor != tColor) {
			str = str + wxT("</font><font color=\"") + tColor + wxT("\">");
			prevColor = tColor;
		}
		//str.append(str[k].GetValue());
		l = 1;
		wxUniChar c = selText[k];
		if (!c.IsAscii()) l++;
		int s = 0;
		//wxUniChar c = selText[k].GetValue();
		if (c == '\r') { startp = startp + l; k++; continue; };
		
		if (c == '\n') { str += newline; startp = startp + l; k++; continue; };
		if (c == 9) s = 5;
		if (c == 32) s = 1;
		if (c == '<') { str+="&lt;"; startp = startp + l; k++; continue; };
		if (c == '>') { str+="&gt;"; startp = startp + l; k++; continue; };
		if (c == '&') { str+="&amp;"; startp = startp + l; k++; continue; };		
		if (s > 0) for (int tt = 0; tt < s; tt++) str += wxT("&nbsp;");
		else str += c;
		startp = startp + l; k++;
	}
	str = str + wxT("</font></div>");
	return str;
}
void ctlSQLBox::Copy() {
	wxString selText = GetSelectedText();
	if (!selText.IsEmpty())
	{
		wxString str;
		str = TextToHtml(GetSelectionStart(), GetSelectionEnd());

		if (wxTheClipboard->Open())
		{
			 wxDataObjectComposite* dataobj = new wxDataObjectComposite();
			dataobj->Add(new wxTextDataObject(selText));
			dataobj->Add(new wxHTMLDataObject(str));
			wxTheClipboard->SetData(dataobj);
			wxTheClipboard->Close();
		}
		
	} else wxStyledTextCtrl::Copy();




   }

extern "C" char *tab_complete(const char *allstr, const int startptr, const int endptr, void *dbptr);
void ctlSQLBox::OnAutoComplete(wxCommandEvent &rev)
{
	if (GetReadOnly())
		return;
	if (m_database == NULL)
		return;
	if (m_autocompDisabled)
		return;
	int pos = GetCurrentPos();
	//wxString what = GetCurLine().Left(pos - PositionFromLine(GetCurrentLine()));
	wxString what = GetTextRange(PositionFromLine(GetCurrentLine()),pos);
	int spaceidx = what.Find(' ', true);
	int spacecharidx=spaceidx;
	int poshome=PositionFromLine(GetCurrentLine());
	int posspc=PositionRelative(poshome,spaceidx);
	if (spaceidx != -1) {
		
		while (poshome< posspc) {
			int ch = GetCharAt(posspc);
			int st = GetStyleAt(posspc) & 0x1F;
			if (st == wxSTC_SQL_STRING || st == wxSTC_SQL_CHARACTER)
			{
				posspc--;
			} else if (ch=='"' || ch=='.') {
				posspc--;
			} else if (ch==' ') {
				break;
			}
			
		}
		wxString lastexp=GetTextRange(PositionFromLine(GetCurrentLine()),posspc);
		spacecharidx=lastexp.Length();
		spaceidx=posspc-poshome;
	}
	char *tab_ret;
	if (spaceidx == -1)
		tab_ret = tab_complete(what.mb_str(wxConvUTF8), 0, what.Len() + 1, m_database);
	else
		tab_ret = tab_complete(what.mb_str(wxConvUTF8), spaceidx + 1, what.Len() + 1, m_database);
	wxString wxRet;
	if (tab_ret != NULL) {
		wxRet = wxString(tab_ret, wxConvUTF8);
		free(tab_ret);
	}
	if ((wxRet.IsEmpty())){ // my autocomplite
		int extflag = 0;
			auto [s,e] = SelectQuery(pos);
			wxString sql = GetTextRange(s, e);
		    FSQL::FormatterSQL f(sql);
			int rez = f.ParseSql(0);
			if (rez >= 0) {
				// ok query
				f.BuildAutoComplite(0, 0);
				int sqlPos = CountCharacters(s,pos);
				int ItemPos=f.GetIndexItemNextSqlPosition(sqlPos);
				FSQL::view_item vi;
				f.GetItem(ItemPos, vi);
				if (vi.type == FSQL::spaces) {
					ItemPos--;
					f.GetItem(ItemPos, vi);
					extflag += TableColsMap::Flag::NOT_ADD_FIRST_SPACE;
				}
				wxString leftexp;
				if (vi.type == FSQL::separation && vi.txt == '=') {
					f.GetItem(ItemPos, vi);
					ItemPos--;
					if (f.next_item_no_space(ItemPos, -1) != -1) {
						f.GetItem(ItemPos, vi);
						if (vi.type == FSQL::identifier || vi.type == FSQL::name) {
							leftexp = vi.txt;
						}
						else return;
					};
				}
				wxString field;
				bool ast = false;
				if (vi.type == FSQL::separation && !CHKFLAG(extflag, TableColsMap::Flag::NOT_ADD_FIRST_SPACE)) {
					ast=vi.txt == ".*";
					while (f.GetItem(--ItemPos, vi)) {
						if (vi.type == FSQL::identifier || vi.type == FSQL::name) {
							field = vi.txt;
							break;
						}
						if (vi.srcpos != -1) break;
					};
				}
				else if (vi.type == FSQL::identifier && !CHKFLAG(extflag, TableColsMap::Flag::NOT_ADD_FIRST_SPACE && leftexp.IsEmpty())) {
					field = vi.txt;
				}
				else if (vi.type == FSQL::keyword || !leftexp.IsEmpty()) {
					// where, on ,and , ...
					wxArrayString tn, an;
					int ccc=f.GetTableListBeforePosition(ItemPos,tn,an);
					if (ccc > 1) {
						// decode view select field
						TableColsMap tmaps;
						int flag = TableColsMap::Flag::ALL_LEFT_TO_RIGHT | TableColsMap::Flag::USE_TRANSIT_FK;
						if (leftexp.IsEmpty()) {
							if (vi.txt.Lower() == "where" || vi.txt.Lower() == "and" || vi.txt.Lower() == "or") flag = TableColsMap::Flag::SEQUENCE_LIST_TABLE | TableColsMap::Flag::USE_TRANSIT_FK;
						} else flag = TableColsMap::Flag::SEQUENCE_LIST_TABLE | TableColsMap::Flag::USE_TRANSIT_FK;
						flag |= extflag;
						wxString list=tmaps.AddTableList(m_database, tn, an, (TableColsMap::Flag) flag,leftexp);
						if (!list.IsEmpty()) {
							int l2 = 0;
							AutoCompShow(l2, list);
						}

						//
						//wxString r = wxJoin(tn, '\t');
					}
					return;
				}
				if (!field.IsEmpty()) {
					wxString lf;
					wxString tabn;
					
					wxString r=f.GetColsList(field, lf, tabn);
					if (r == "\t") r.clear();
					int l2 = 0;
					wxString flt = "";
					wxString prev=tabn;
					wxString fld = field.AfterFirst('.');
					if (fld.Len()>0) l2 = fld.Len();
					if (tabn.Len() > 0 && r.Len()==0) {
						if (!field.AfterFirst('.').IsEmpty()) flt = " and a.attname ~* " + qtConnString(fld);
						wxString sch;
						if (tabn.Find('.') != -1) sch = tabn.BeforeFirst('.');
						if (sch.Len() > 0) {
							tabn = tabn.AfterFirst('.');
							if (sch[0] == '"') sch.Replace("\"", ""); else sch = sch.Lower();
							sch = " and relnamespace =" + qtConnString(sch) + "::regnamespace";
						}
						if (tabn[0] == '"') tabn.Replace("\"", ""); else tabn = tabn.Lower();
						wxString sql2 = wxT("select string_agg(a.attname,E'\t') from pg_attribute a where a.attrelid = (select oid from pg_class p where relname=") + qtConnString(tabn) + sch
							+ wxT(") and a.attisdropped IS FALSE and a.attnum>=0 ") + flt
							+ wxT("");
						//pgSet *res = m_database->ExecuteSet(sql);
						r = m_database->ExecuteScalar(sql2);

					}
					if (ast) {
						// replace name.*
						r.Replace("\t", ", "+ field +".");
						int npos = pos - 2 - field.Len();
						DeleteRange(npos, field.Len() + 2);
						InsertText(npos, field + "."+r);
					}
					else {
						wxArrayString sort=wxSplit(r, '\t');
						sort.Sort();
						r=wxJoin(sort, '\t');
						AutoCompShow(l2, r);
					}
					return;
				}
				if (vi.type == FSQL::name) {
					field = vi.txt;
					// for any name found table
					bool isok = false;
					while (f.GetItem(ItemPos--, vi)) {
						if (vi.endlevel != -1 && vi.endlevel < ItemPos) ItemPos = vi.endlevel;
						if (vi.type == FSQL::keyword && vi.txt.Lower() == "from") { isok = true;  break; }
						if (vi.type == FSQL::keyword && vi.txt.Lower() == "where") {  break; }
						if (vi.type == FSQL::keyword && vi.txt.Lower() == "group") { break; }
					}
					if (isok) {
						what = "from " + field;
						spaceidx = what.Find(' ');
						tab_ret = tab_complete(what.mb_str(wxConvUTF8), spaceidx + 1, what.Len() + 1, m_database);
						if (tab_ret != NULL) {
							wxString wxRet;
							wxRet = wxString(tab_ret, wxConvUTF8);
							bool empty = tab_ret[0] == '\0';
							free(tab_ret);
							if (!empty) AutoCompShow(what.Len() - spaceidx - 1, wxRet);
						}
					}
				}
				return;
			}

			int l=0;
			wxString alias;
			wxString fld,tmp;
			bool fl = true;
			for (int kk=what.Len()-1;kk>=0;kk--) {
				//char c=myStringChars.data[kk];
				if (what[kk] <= ' ') {
					for (wxString::reverse_iterator it = tmp.rbegin(); it != tmp.rend(); ++it)
					{
						alias.Append(*it);
					}
					break;
				}
				if (what[kk] == '.' && fl) {
					fl = false;
					for (wxString::reverse_iterator it = tmp.rbegin(); it != tmp.rend(); ++it)
					{
						fld.Append(*it);
					}
					tmp = wxEmptyString;
					continue;
				}
				tmp += what[kk];
			}
			wxString f_name;
			if (!fl && !alias.IsEmpty()) {
				
				wxString lst=list_table;
				wxString table;
				//alias.Replace(wxT("."), wxT(""));
				lst.Replace(wxT("\""), wxT(""));
				wxStringTokenizer tokenizer(lst, wxT(" ,"));
				wxString prev;
				bool found=false;
				int p=0;
				while ( tokenizer.HasMoreTokens() )
				{
					wxString token = tokenizer.GetNextToken();
					if (token.IsEmpty()) {p=0; continue;}
					token=token.AfterLast('.');
					found=token.CmpNoCase(alias)==0;
					if (tokenizer.GetLastDelimiter()==' '&&found) {
						if (p==1) table=prev; else table=token;
						break;
					}
					if ((tokenizer.GetLastDelimiter()==',')&&found) {
						if (p==1) table=prev; else table=token;
						break;
					}
					if (found) {if (p==1) table=prev; else table=token; break;}
					prev=token;
					p++;
				}
				if (found) {
					wxString flt="";
					if (!fld.IsEmpty()) flt = " and a.attname ~ " + qtConnString(fld);
				wxString sql=wxT("select string_agg(a.attname,E'\t') from pg_attribute a where a.attrelid in (select oid from pg_class p where relname=") +qtConnString(table)
						+wxT(") and a.attisdropped IS FALSE and a.attnum>=0 ")+flt
						+wxT(" order by 1");
				//pgSet *res = m_database->ExecuteSet(sql);
				prev= m_database->ExecuteScalar(sql);
				int l2 = 0;
				if (!fld.IsEmpty()) l2 = fld.Len();
				AutoCompShow(l2, prev);
				return;
				}
				//list_table.Find(alias	
			}
			l=0;
			for (int kk=what.Len()-1;kk>=0;kk--) {
				//char c=myStringChars.data[kk];
				if ( what[kk]<'0') break;
				l++;
			}
			if (l==0) return;
			f_name=what.Right(l);
			what=f_name;
			size_t i,
				   lo = 0,
				   hi = m_name->Count();
			int res;
			while ( lo < hi ) {
			  i = (lo + hi)/2;
			  if (m_name->Item(i).StartsWith(f_name)) break;
			  res= f_name.CmpNoCase(m_name->Item(i));
			  //res = wxStrcmp(sz, m_pItems[i]);
			  if ( res < 0 )
				hi = i;
			  else if ( res > 0 )
				lo = i + 1;
			  else
			     {  break; }// i ok
			}
			if (lo >= hi) return;
			int k=i;
			while ( k>=0&&m_name->Item(k).StartsWith(f_name) ) k--;
			k++;
			wxString prev;
			while ( (k<m_name->GetCount())&&m_name->Item(k).StartsWith(f_name) )
				{
					if (prev!=m_name->Item(k)) wxRet+=m_name->Item(k)+wxT("\t");
					prev=m_name->Item(k);
					k++;
				};
			spaceidx = what.Len()-1;
			spaceidx = -1;
		//return; /* No autocomplete available for this string */
	} else {
	}
	// Switch to the generic list control. Native doesn't play well with
	// autocomplete on Mac.
#ifdef __WXMAC__
	wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), true);
#endif

	if (spaceidx == -1)
		AutoCompShow(what.Len(), wxRet);
	else
		//AutoCompShow(what.Len() - spacecharidx - 1, wxRet);
		AutoCompShow(pos - posspc - 1, wxRet);

	// Now switch back
#ifdef __WXMAC__
	wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), false);
#endif
}


ctlSQLBox::~ctlSQLBox()
{
	delete refreshUITimer;
	if (m_dlgFindReplace)
	{
		m_dlgFindReplace->Destroy();
		m_dlgFindReplace = 0;
	}
	if (m_dlgTransformText)
	{
		m_dlgTransformText->Destroy();
		m_dlgTransformText = 0;
	}
	AbortProcess();
}


/*
 * Callback function from tab-complete.c, bridging the gap between C++ and C.
 * Execute a query using the C++ APIs, returning it as a tab separated
 * "char*-string"
 * The query is expected to return only one column, and will have an ORDER BY
 * clause for this column added automatically.
 */
extern "C"
char *pg_query_to_single_ordered_string(char *query, void *dbptr)
{
	pgConn *db = (pgConn *)dbptr;
	pgSet *res = db->ExecuteSet(wxString(query, wxConvUTF8) + wxT(" ORDER BY 1"));
	if (!res)
		return NULL;

	wxString ret = wxString();
	wxString tmp;

	while (!res->Eof())
	{
		tmp =  res->GetVal(0);
		if (tmp.Mid(tmp.Length() - 1) == wxT("."))
			ret += tmp + wxT("\t");
		else
			ret += tmp + wxT(" \t");

		res->MoveNext();
	}

	if(res)
	{
		delete res;
		res = NULL;
	}
	ret.Trim();
	// Trims both space and tab, but we want to keep the space!
	if (ret.Length() > 0)
		ret += wxT(" ");

	return strdup(ret.mb_str(wxConvUTF8));
}
extern "C"
int get_id_encoding(void *dbptr)
{
	pgConn *db = (pgConn *)dbptr;
	return db->Get_client_encoding_id();
}

// Find some text in the document.
CharacterRange ctlSQLBox::RegexFindText(int minPos, int maxPos, const wxString &text)
{
	TextToFind  ft;
	ft.chrg.cpMin = minPos;
	ft.chrg.cpMax = maxPos;
	wxWX2MBbuf buf = text.mb_str(wxConvUTF8);
	ft.lpstrText = (char *)(const char *)buf;

//
	if (SendMsg(2150, wxSTC_FIND_REGEXP, (std::uintptr_t)&ft) == -1)
	{
		ft.chrgText.cpMin = -1;
		ft.chrgText.cpMax = -1;
	}

	return ft.chrgText;
}
