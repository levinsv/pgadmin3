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
#include "utils/sysProcess.h"
#include <wx/clipbrd.h>
#include <wx/aui/aui.h>

wxString ctlSQLBox::sqlKeywords;
static const wxString s_leftBrace(_T("([{"));
static const wxString s_rightBrace(_T(")]}"));
static const int s_indicHighlight(20);

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
	EVT_MENU(MNU_AUTOCOMPLETE, ctlSQLBox::OnAutoComplete)
	EVT_KILL_FOCUS(ctlSQLBox::OnKillFocus)
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
	m_autoIndent = false;
	m_autocompDisabled = false;
	process = 0;
	processID = 0;
}


ctlSQLBox::ctlSQLBox(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style)
{
	m_dlgFindReplace = 0;

	m_database = NULL;

	m_autocompDisabled = false;
	process = 0;
	processID = 0;

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
void ctlSQLBox::Create(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style)
{
	wxStyledTextCtrl::Create(parent, id , pos, size, style);
	fix_pos=-1;
	sql_query_book =NULL;
	// Clear all styles
	StyleClearAll();
	m_name=NULL;
	// Font
	extern sysSettings *settings;
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
	
	SetProperty(wxT("fold"), wxT("1"));
	SetFoldFlags(16);

	// Setup accelerators
	wxAcceleratorEntry entries[2];
	entries[0].Set(wxACCEL_CTRL, (int)'F', MNU_FIND);
	entries[1].Set(wxACCEL_CTRL, WXK_SPACE, MNU_AUTOCOMPLETE);
	wxAcceleratorTable accel(2, entries);
	SetAcceleratorTable(accel);

	// Autocompletion configuration
	AutoCompSetSeparator('\t');
	AutoCompSetChooseSingle(true);
	AutoCompSetIgnoreCase(true);
	AutoCompSetFillUps(wxT(" \t"));
	AutoCompSetDropRestOfWord(true);

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

void ctlSQLBox::SetFilename(wxString &filename)
{
	m_filename = filename;
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
		selEnd = selStart + find.Length();
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

void ctlSQLBox::OnKeyDown(wxKeyEvent &event)
{
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
	while ((pos--) >= 0)
	{
		ch = GetCharAt(pos);
		st = GetStyleAt(pos);
		if ((ch == '{' || ch == '}' ||
		        ch == '[' || ch == ']' ||
		        ch == '(' || ch == ')') &&
		        st != 1 &&st != 2 && st != 6 && st != 7)
		{
			match = BraceMatch(pos);
			if (match == wxSTC_INVALID_POSITION)
			{
				BraceBadLight(pos);
			}
		}
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
			wxChar c = t.GetChar(0);
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
			Update();
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

wxString ctlSQLBox::ExternalFormat()
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
	if (formatCmd.IsEmpty())
	{
		return _("You need to setup a formatting command");
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
		msg = _("Couldn't run formatting command: ") + formatCmd;
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
		return wxString::Format(_("Formatting command did not respond in %d ms"), timeoutMs);
	}

	if (processExitCode != 0)
	{
		processErrorOutput.Replace(wxT("\n"), wxT(" "));
		msg = wxString::Format(_("Error %d: "), processExitCode) + processErrorOutput;
		return msg;
	}
	else if (processOutput.Trim().IsEmpty())
	{
		return _("Formatting command error: Output is empty.");
	}

	if (isSelected)
		ReplaceSelection(processOutput);
	else
		SetText(processOutput);

	return _("Formatting complete.");
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
long ctlSQLBox::SelectQuery(int startposition)
{
	int pos = GetCurrentPos();
	wxChar ch = GetCharAt(pos - 1);
	wxChar nextch = GetCharAt(pos);
	int st = GetStyleAt(pos - 1);
	int endPos = GetTextLength();
	int match;
	int pend=endPos;
	int pstart=0;

	if ((ch == ';')) {
		pend=pos;
		pos=pos-1;
	} else
	{
		while (pos<endPos) {
		ch = GetCharAt(pos);
		st = GetStyleAt(pos) & 0x1F;
		if ((ch == ';') &&
	        st != 2 && st != 6 && st != 7)
		{
			pend=pos;
			break;
		}
		int i=IsDBCSLeadByte(ch)? 2 : 1;
		pos=pos+i;
		}
		
	}
		pos--;
		while ((pos) >= 0)
	{
		ch = GetCharAt(pos);
		st = GetStyleAt(pos) & 0x1F;
		if ((ch == ';') &&
	        st != 2 && st != 6 && st != 7)
		{
			//pstart=pos+1;
			break;
		}
		if (ch>' ') pstart=pos;
		int i=IsDBCSLeadByte(ch)? 2 : 1;
		pos=pos-i;

	}
		if (startposition<0) SetSelection(pstart,pend);
		return (pstart <<16)+pend;
		
}
void ctlSQLBox::HighlightBrace(int lb, int rb) {
	SetIndicatorCurrent(s_indicHighlight);
            {
                //SetCaretForeground(wxColour(255, 0, 0));
                //SetCaretWidth(caretWidth + 1);

                IndicatorSetForeground(s_indicHighlight, wxColour(80, 236, 120));
                IndicatorSetStyle(s_indicHighlight, wxSTC_INDIC_ROUNDBOX);
#ifndef wxHAVE_RAW_BITMAP
                IndicatorSetUnder(s_indicHighlight, true);
#endif
                SetIndicatorCurrent(s_indicHighlight);
                IndicatorFillRange(lb, 1);
				IndicatorFillRange(rb, 1);
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
	while ((pos--) >= 0)
	{
		ch = GetCharAt(pos);
		st = GetStyleAt(pos);
		if (st != 1 &&st != 2 && st != 6 && st != 7 &&(ch==';')&&startsql==0) startsql=pos+1;
		if ((ch == '{' || ch == '}' ||
		        ch == '[' || ch == ']' ||
		        ch == '(' || ch == ')') &&
		        st != 1 &&st != 2 && st != 6 && st != 7)
		{
			match = BraceMatch(pos);
			if (match == wxSTC_INVALID_POSITION)
			{
				event.Skip();
				return;
			}
		}
	}
	// получение  позици select
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
void ctlSQLBox::Copy() {
	wxString selText = GetSelectedText();
	if (!selText.IsEmpty())
	{
		wxColor frColor[40];
		wxString str;
			wxColour frc = settings->GetSQLBoxColourForeground();
			if (settings->GetSQLBoxUseSystemForeground())
			{
				frc = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
			}

			for (int i = 0; i < 34; ++ i )
			{
				frColor[i]=StyleGetForeground(i);
			if (i > 0 && i < 12)
				//StyleSetForeground(i, settings->GetSQLBoxColour(i));
				frColor[i]=StyleGetForeground(i);
			else
				frColor[i]=frc;

			//StyleSetBackground(i, bgColor);
			//StyleSetFont(i, fntSQLBox);
			}
			//<h1 style="color:blue;">
			int endp=GetSelectionEnd();
			int startp=GetSelectionStart();
			wxString prevColor=wxEmptyString;
			wxString tColor;
			wxFont fntSQLBox = settings->GetSQLFont();
			wxString fontName = fntSQLBox.GetFaceName();
			wxString sz;
			sz.Printf("%d",fntSQLBox.GetPixelSize().GetHeight() );
			
			str=wxT("<div style=\"font-family: ")+fontName+wxT("; font-size: "+sz+"px\"><font>");
			int k=0;
			int l=1;
			while (startp<endp) {
				int st = GetStyleAt(startp);
				if (st<34) tColor=frColor[st].GetAsString(wxC2S_HTML_SYNTAX);
				if (prevColor!=tColor) {
					str=str+wxT("</font><font color=\"")+tColor+wxT("\">");
					prevColor=tColor;
				}
				//str.append(str[k].GetValue());
				l=1;
				if (!selText[k].IsAscii()) l++;
				int s=0;
				if (selText[k].GetValue()==9) s=5;
				if (selText[k].GetValue()==32) s=1;
				if (s>0) for (int tt=0;tt<s;tt++) str+=wxT("&nbsp;");
				         else str+=selText[k];

				if (str.EndsWith(wxT("\r\n"))) str+=wxT("<br>"); 
				startp=startp+l;
				k++;
			}
			str=str+wxT("</font></div>");
			


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

	wxString what = GetCurLine().Left(GetCurrentPos() - PositionFromLine(GetCurrentLine()));;
	int spaceidx = what.Find(' ', true);

	char *tab_ret;
	if (spaceidx == -1)
		tab_ret = tab_complete(what.mb_str(wxConvUTF8), 0, what.Len() + 1, m_database);
	else
		tab_ret = tab_complete(what.mb_str(wxConvUTF8), spaceidx + 1, what.Len() + 1, m_database);
	wxString wxRet;
	if (tab_ret == NULL || tab_ret[0] == '\0'){
			int l=0;
			for (int kk=what.Len()-2;kk>=0;kk--) {
				//char c=myStringChars.data[kk];
				if ( what[kk]<'0') break;
				l++;
			}
			wxString f_name;
			if (l>0) {
				wxString alias=what.substr(what.Len()-1-l,l);
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
				wxString sql=wxT("select string_agg(a.attname,E'\t') from pg_attribute a where a.attrelid = (select oid from pg_class p where relname=") +qtConnString(table)
						+wxT(") and a.attisdropped IS FALSE and a.attnum>=0 order by 1");
				//pgSet *res = m_database->ExecuteSet(sql);
				prev= m_database->ExecuteScalar(sql);
				AutoCompShow(0, prev);
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
		wxRet = wxString(tab_ret, wxConvUTF8);
		free(tab_ret);
	}
	// Switch to the generic list control. Native doesn't play well with
	// autocomplete on Mac.
#ifdef __WXMAC__
	wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), true);
#endif

	if (spaceidx == -1)
		AutoCompShow(what.Len(), wxRet);
	else
		AutoCompShow(what.Len() - spaceidx - 1, wxRet);

	// Now switch back
#ifdef __WXMAC__
	wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), false);
#endif
}


ctlSQLBox::~ctlSQLBox()
{
	if (m_dlgFindReplace)
	{
		m_dlgFindReplace->Destroy();
		m_dlgFindReplace = 0;
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


// Find some text in the document.
CharacterRange ctlSQLBox::RegexFindText(int minPos, int maxPos, const wxString &text)
{
	TextToFind  ft;
	ft.chrg.cpMin = minPos;
	ft.chrg.cpMax = maxPos;
	wxWX2MBbuf buf = text.mb_str(wxConvUTF8);
	ft.lpstrText = (char *)(const char *)buf;

//не компилировалась с wx 2.8.12
	if (SendMsg(2150, wxSTC_FIND_REGEXP, (long)&ft) == -1)
	{
		ft.chrgText.cpMin = -1;
		ft.chrgText.cpMax = -1;
	}

	return ft.chrgText;
}
