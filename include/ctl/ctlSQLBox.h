//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// ctlSQLBox.h - SQL syntax highlighting textbox
//
//////////////////////////////////////////////////////////////////////////

#ifndef CTLSQLBOX_H
#define CTLSQLBOX_H

// wxWindows headers
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/fdrepdlg.h>
#include "utils/macros.h"

#include "db/pgConn.h"
#include "dlg/dlgFindReplace.h"
#include "ctl/ctlAuiNotebook.h"

// These structs are from Scintilla.h which isn't easily #included :-(
struct CharacterRange
{
	long cpMin;
	long cpMax;
};

struct TextToFind
{
	struct CharacterRange chrg;
	char *lpstrText;
	struct CharacterRange chrgText;
};

class sysProcess;

// Class declarations
class ctlSQLBox : public wxStyledTextCtrl
{
	static wxString sqlKeywords;

public:
	ctlSQLBox(wxWindow *parent, wxWindowID id = -1, const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize, long style = 0);
	ctlSQLBox();
	~ctlSQLBox();

	void Create(wxWindow *parent, wxWindowID id = -1, const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize, long style = 0);
	void HighlightBrace(int lb, int rb);
	void SetDatabase(pgConn *db);
	void Copy();
	void OnKeyDown(wxKeyEvent &event);
	void OnAutoComplete(wxCommandEvent &event);
	void OnSearchReplace(wxCommandEvent &event);
	void OnCopy(wxCommandEvent& ev);
	void OnKillFocus(wxFocusEvent &event);
//	void OnBackGround(wxEraseEvent &event);
	void SetQueryBook(ctlAuiNotebook *query_book);
	ctlAuiNotebook* GetQueryBook()
	{
		return sql_query_book;
	};

	bool Find(const wxString &find, bool wholeWord, bool matchCase, bool useRegexps, bool startAtTop, bool reverse, bool all=false);
	bool Replace(const wxString &find, const wxString &replace, bool wholeWord, bool matchCase, bool useRegexps, bool startAtTop, bool reverse);
	bool ReplaceAll(const wxString &find, const wxString &replace, bool wholeWord, bool matchCase, bool useRegexps);
	bool DoFind(const wxString &find, const wxString &replace, bool doReplace, bool wholeWord, bool matchCase, bool useRegexps, bool startAtTop, bool reverse);
	void SetAutoReplaceList(queryMacroList *autorep);
	void SetAutoIndent(bool on)
	{
		m_autoIndent = on;
	}
	void EnableAutoComp(bool on)
	{
		m_autocompDisabled = on;
	}
	bool BlockComment(bool uncomment = false);
	bool BlockDouble(bool undouble = false);
	void UpdateLineNumber();
	wxString ExternalFormat();
	void AbortProcess();
	void SetDefFunction(wxArrayString &name, wxArrayString &def);
	CharacterRange RegexFindText(int minPos, int maxPos, const wxString &text);

	// Having multiple SQL tabs warrants the following properties to be tracked per tab
	void SetChanged(bool b);
	bool IsChanged();
	wxColor SetSQLBoxColourBackground(bool transaction);
	void SetOrigin(int origin);
	int GetOrigin();
	void SetFilename(wxString &filename);
	bool IsFileModification();
	wxString GetFilename();
	void SetTitle(wxString &title);
	wxString GetTitle(bool withChangeInd = true);
	wxString GetChangeIndicator();
	long SelectQuery(int startposition);
	DECLARE_DYNAMIC_CLASS(ctlSQLBox)
	DECLARE_EVENT_TABLE()

protected:
	void OnEndProcess(wxProcessEvent &ev);
	void UpdateTitle();

	sysProcess *process;
	long processID;
	wxString processOutput, processErrorOutput;
	int processExitCode;

private:
	void OnPositionStc(wxStyledTextEvent &event);
	void OnDoubleClick(wxStyledTextEvent &event);
	void OnMarginClick(wxStyledTextEvent &event);
	ctlAuiNotebook *sql_query_book;
	queryMacroList *autoreplace;
	wxArrayString *m_name; // field proname
	wxArrayString *m_def; // finction arguments
	wxString list_table; //  list table from section
	wxString calltip;
	int ct_hl;
	dlgFindReplace *m_dlgFindReplace;
	pgConn *m_database;
	bool m_autoIndent, m_autocompDisabled;

	// Variables to track info per SQL box
	wxString m_filename;
	time_t time_file_mod;
	wxString m_title;
	wxString m_changestr;
	bool m_changed;
	int m_origin;
	int fix_pos;

	friend class QueryPrintout;
};

#endif

