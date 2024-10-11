#include "pgAdmin3.h"
#include "utils/dlgTransformText.h"
#include "frm/frmMain.h"
#include "ctl/ctlStyledText.h"
#include "utils/sysSettings.h"
#include <wx/log.h>
#include <limits>

BEGIN_EVENT_TABLE(dlgTransformText, pgDialog)
//EVT_KEY_DOWN(dlgTransformText::OnKeyDown)
//EVT_BUTTON(XRCID("btnCancel"), dlgTransformText::OnCancel)
EVT_BUTTON(wxID_CANCEL, dlgTransformText::OnCancel)

EVT_BUTTON(XRCID("btnOk"), dlgTransformText::OnOk)
EVT_BUTTON(XRCID("btnSave"), dlgTransformText::OnSave)

EVT_CHECKBOX(XRCID("chkHighLight"), dlgTransformText::OnChangeOnline)
//EVT_TEXT(XRCID("LimitCharsText"), dlgTransformText::OnChange)
//EVT_TEXT(XRCID("LimitRowsText"), dlgTransformText::OnChange)
EVT_COMBOBOX(XRCID("beforeRowCB"), dlgTransformText::OnChange)
EVT_COMBOBOX(XRCID("sepFieldCB"), dlgTransformText::OnChange)
EVT_COMBOBOX(XRCID("afterRowCB"), dlgTransformText::OnChange)
//EVT_COMBOBOX(XRCID("afterFieldCB"), dlgTransformText::OnChange)
EVT_COMBOBOX(XRCID("optionsLoadCB"), dlgTransformText::OnLoad)
EVT_TEXT(XRCID("beforeRowCB"), dlgTransformText::OnChange)
EVT_TEXT(XRCID("sepFieldCB"), dlgTransformText::OnChange)
EVT_TEXT(XRCID("afterRowCB"), dlgTransformText::OnChange)
//EVT_TEXT(XRCID("afterFieldCB"), dlgTransformText::OnChange)
EVT_TEXT(XRCID("LimitRowsText"), dlgTransformText::OnChangeLimit)
EVT_TEXT(XRCID("LimitCharsText"), dlgTransformText::OnChangeLimit)
//EVT_KILL_FOCUS(dlgTransformText::OnKillFocus)

//EVT_STC_MODIFIED(XRCID("RegText"), dlgTransformText::OnChangeRegEx)
//EVT_STC_MODIFIED(XRCID("ReplaceText"), dlgTransformText::OnChangeRegEx)
EVT_STC_UPDATEUI(XRCID("RegText"), dlgTransformText::OnChangeRegEx2)
EVT_STC_UPDATEUI(XRCID("ReplaceText"), dlgTransformText::OnChangeRegEx2)
//EVT_COMMAND_KILL_FOCUS(XRCID("RegText"), dlgTransformText::OnKillFocusRegEx)
//EVT_TEXT(XRCID("RegText"), dlgTransformText::OnChangeRegEx)
EVT_IDLE(dlgTransformText::OnIdle)

EVT_CLOSE(dlgTransformText::OnClose)

END_EVENT_TABLE()


#define btnOK                   CTRL_BUTTON("wxID_OK")
#define btnCancel               CTRL_BUTTON("wxID_CANCEL")
#define btnSave                 CTRL_BUTTON("btnSave")
#define m_msg                   CTRL_STATIC("m_msg")
//#define afterFieldCB            CTRL_COMBOBOX1("afterFieldCB")

#define srcText                 CTRL_STYLEDTEXT("srcText")
#define trgText                 CTRL_STYLEDTEXT("trgText")
#define LimitCharsText          CTRL_TEXT("LimitCharsText")
#define LimitRowsText           CTRL_TEXT("LimitRowsText")
#define chkHighLight		    CTRL_CHECKBOX("chkHighLight")

#define beforeRowCB             CTRL_COMBOBOX1("beforeRowCB")
#define afterRowCB              CTRL_COMBOBOX1("afterRowCB")
#define sepFieldCB           CTRL_COMBOBOX1("sepFieldCB")
//#define afterFieldCB            CTRL_COMBOBOX1("afterFieldCB")

#define optionsLoadCB           CTRL_COMBOBOX1("optionsLoadCB")

#define txtField                CTRL_STYLEDTEXT("RegText")
#define trgField                CTRL_STYLEDTEXT("ReplaceText")



dlgTransformText::dlgTransformText(ctlSQLBox* form,const wxString source) : pgDialog()
{
	this->inizialize = true;
	SetFont(settings->GetSystemFont());
	LoadResource(form, wxT("dlgTransformText"));
	RestorePosition();
	SetTitle(_("Transformation text"));
	LoadOptions();
	//srcText->Set
	//extern sysSettings* settings;
	SetDecoration(srcText);
	SetDecoration(trgText);
	SetDecoration(txtField);
	
	SetDecoration(trgField);
	txtField->setRegExphighlight(true);

	wxAcceleratorEntry entries[1];
	entries[0].Set(wxACCEL_NORMAL, WXK_ESCAPE, wxID_CANCEL);
	wxAcceleratorTable accel(1, entries);
	SetAcceleratorTable(accel);
	//optionsLoadCB->Connect(wxID_ANY, wxEVT_KILL_FOCUS, wxFocusEventHandler(dlgTransformText::OnKillFocus),this);

}
void dlgTransformText::SetDecoration(ctlStyledText *s) {
	wxFont fntSQLBox = settings->GetSQLFont();
	//srcText->StyleSetForeground(wxSTC_STYLE_DEFAULT, frColor);
	s->StyleSetFont(wxSTC_STYLE_DEFAULT, fntSQLBox);
	
	s->SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
	s->SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));

}
dlgTransformText::~dlgTransformText()
{
	SavePosition();
}
void dlgTransformText::SetSource(const wxString& src) {
	//
	//srcText->SetValue(src);
	this->src = src;
	showNumber(srcText, true);
	showNumber(trgText, true);
	this->inizialize = false;
	wxStyledTextEvent ev;
	strResult = wxEmptyString;
	strReg = wxEmptyString;
	//OnChangeRegEx2(ev);
	isNeedTransform = true;
}

void dlgTransformText::showNumber(ctlStyledText* text, bool visible)
{
	if (visible) {
		long int width = text->TextWidth(wxSTC_STYLE_LINENUMBER,
			wxT(" ") + wxString::Format("%ld", (long int)limitLine) + wxT(" "));
		if (width != text->GetMarginWidth(0))
		{
			text->SetMarginWidth(0, width);
			text->Update();
		}
		text->SetViewWhiteSpace(wxSTC_WS_VISIBLEALWAYS);
		text->SetViewEOL(true);
	}
	else {
		text->SetMarginWidth(0, 0);
		text->Update();
	}
}
wxJSONValue dlgTransformText::LoadConfig(const wxString confname) {
	wxJSONValue conf(wxJSONType::wxJSONTYPE_OBJECT);
	wxString n;
	if (confname.IsEmpty()) n = "last"; else n = confname;
	bool ischange = false;
	bool t_inizialize = inizialize;
	inizialize = true;
	conf = opt["configurations"][n];
	bool islastconf=false;
	if (!conf.IsNull() && lastconf.Size() > 0) 	islastconf = true;
	if (!conf.IsNull()) {
		wxString s;
		s = conf["beforeRow"].AsString();
		if (islastconf && s != lastconf["beforeRow"].AsString()) {
			s = lastconf["beforeRow"].AsString();
			ischange = true;
		}
		beforeRowCB->SetValue(s);
		s = conf["afterRow"].AsString();
		if (islastconf && s != lastconf["afterRow"].AsString()) {
			s = lastconf["afterRow"].AsString();
			ischange = true;
		}
		afterRowCB->SetValue(s);
		s = conf["sepField"].AsString();
		if (islastconf && s != lastconf["sepField"].AsString()) {
			s = lastconf["sepField"].AsString();
			ischange = true;
		}
		sepFieldCB->SetValue(s);
		s = conf["regexp"].AsString();
		if (islastconf && s != lastconf["regexp"].AsString()) {
			s = lastconf["regexp"].AsString();
			ischange = true;
		}
		txtField->SetText(s);
		s = conf["replace"].AsString();
		if (islastconf && s != lastconf["replace"].AsString()) {
			s = lastconf["replace"].AsString();
			ischange = true;
		}
		trgField->SetText(s);
		//afterFieldCB->SetValue(conf["fieldSeparator"].AsString());
		if (islastconf) {
			opt["currentConfingName"] = n;
		}
		if (ischange) n = '*' + n;
		optionsLoadCB->SetValue(n);
		isChange = ischange;
		lastconf = conf;
	}
	inizialize = t_inizialize;
	return lastconf;
}
void dlgTransformText::LoadOptions() {

	wxJSONValue def(wxJSONType::wxJSONTYPE_OBJECT);
	//wxJSONValue confArray(wxJSONType::wxJSONTYPE_ARRAY);
	wxJSONValue confs(wxJSONType::wxJSONTYPE_OBJECT);
	wxJSONValue grpArray(wxJSONType::wxJSONTYPE_ARRAY);
	opt.SetType(wxJSONType::wxJSONTYPE_OBJECT);
	lastconf.SetType(wxJSONType::wxJSONTYPE_OBJECT);
	const wxString clrgroup[] = { "#ffff00"
		,"#c4e8ac","#f7d6a6","#c7c7fc","#f1cdfd","#ffc6be"
		,"#cae7d6" ,"#c4e7ab" ,"#f4d7a5" ,"#c7c8fe","#f2cffd"
		,"#ffc4bd", "#c7e7d4" };
	for (int k = 0; k < sizeof(clrgroup) / sizeof(clrgroup[0]); k++) {
		//wxJSONValue grp(wxJSONType::wxJSONTYPE_OBJECT);
		//grp["number"] = k;
		//grp["color"] = clrgroup[k];

		grpArray.Append(clrgroup[k]);
	}
	def["colorGroup"] = grpArray;
	def["newLineSeparator"] = wxString("\\r\\n|\\r[^\\n]|\\n");
	def["bgErrorColor"] = wxString("#f4dcdc");
	wxJSONValue conf(wxJSONType::wxJSONTYPE_OBJECT);
	//conf["name"] = wxString("last");
	wxString name = "last";
	//conf["name"] = wxString("last");
	def["currentConfingName"] = wxString("");
	conf["regexp"] = wxString("\\d+");
	conf["replace"] = wxString("\\g{0}");
	conf["beforeRow"] = wxString("");
	conf["afterRow"] = wxString("");
	conf["sepField"] = wxString(",");
	conf["afterField"] = wxString("");
	conf["fieldSeparator"] = wxString("");
	confs[name] = conf;
	def["configurations"] = confs;
	def["onLineRefresh"] = true;
	int limline = 50;
	def["LimitShowLine"] = limline;
	int limchar = 50000;
	def["LimitShowChar"] = limchar;
	settings->ReloadJsonFileIfNeed();
	settings->ReadJsonObect(dlgName, opt, def);
	if (!opt.IsNull()) {
		wxString strcolor = opt["bgErrorColor"].AsString();
		wxColour cc(strcolor);
		if (!cc.IsOk()) opt["bgErrorColor"] = def["bgErrorColor"];
		
	}
	else opt = def;
	bgerror = wxColour(opt["bgErrorColor"].AsString());
	lastconf = opt["configurations"][name];
	wxArrayString ar = opt["configurations"].GetMemberNames();
	for (int i = 0; i < ar.Count(); i++) {
		wxString s;
		wxJSONValue cf= opt["configurations"][ar[i]];
		wxString s2;
		if (ar[i] == "last") continue;
		s2 = cf["beforeRow"].AsString();
		beforeRowCB->Append(s2);
		s2 = cf["afterRow"].AsString();
		afterRowCB->Append(s2);
		s2 = cf["sepField"].AsString();
		sepFieldCB->Append(s2);
//		s2 = cf["afterField"].AsString();
//		afterFieldCB->Append(s2);
		s2 = ar[i];
		optionsLoadCB->Append(s2);
	}
	wxString s=opt["currentConfingName"].AsString();
	if (opt["LimitShowLine"].IsInt()) limline= opt["LimitShowLine"].AsInt();
	if (opt["LimitShowChar"].IsInt()) limchar = opt["LimitShowChar"].AsInt();
	limitLine = limline;
	limitChar = limchar;
	srcRowSep = opt["newLineSeparator"].AsString();
	wxRegEx RegNewLine(srcRowSep, wxRE_NEWLINE);
	if (!RegNewLine.IsValid()) srcRowSep= def["newLineSeparator"].AsString();
	LimitRowsText->SetValue(opt["LimitShowLine"].AsString());
	LimitCharsText->SetValue(opt["LimitShowChar"].AsString());
	isOnline = chkHighLight->IsChecked();
	LoadConfig(s);
	wxCommandEvent ev;
	OnCheckUI(ev);
}

int str_to_int(const wxString& s) {
	int ii = INT_MAX;
	int scanned = wxSscanf(s, "%d", &ii);
	return ii;
}
void dlgTransformText::CheckLimits() {
	wxString s = LimitRowsText->GetValue();
	int l;
	l = str_to_int(s);
	if (s.length() == 0|| l==0) {
		l = INT_MAX; // unlimited
	}
	else {
		if (l == INT_MAX) l = 50;
	}
	limitLine = l;
	s = LimitCharsText->GetValue();
	l = str_to_int(s);
	if (s.length() == 0|| l==0) {
		l = INT_MAX; // unlimited
	}
	else {
		
		if (l == INT_MAX) l = 50000;
	}
	limitChar = l;

}
std::vector<dlgTransformText::replace_opt> dlgTransformText::BuildString(const wxString repstr) {
	std::vector<replace_opt> rez;
	bool isback = false;
	wxString a;
	wxString e;
	for (int i = 0; i < repstr.length(); i++) {
		wxChar c = repstr[i];
		if (!isback && c == '\\') {
			isback = true;
			continue;
		}
		if (isback) {
			if (c == 'r') c = '\r';
			else if (c == 'n') c = '\n';
			else if (c == 't') c = '\t';
			else if (c == 'g') {
				int l = repstr.length();
				if (l - i > 2) {
					int pos = repstr.find_first_of('}', i);
					if (pos >= 0) {
						wxString ngrp = repstr.substr(i + 2, pos - i - 2);
						int ngroup = -1;
						int scanned = wxSscanf(ngrp, "%d", &ngroup);
						if (scanned == 1) {
							if (a.length() > 0) rez.push_back({ false,-1,a });
							a.clear();
							rez.push_back({ true,ngroup,wxEmptyString });
							i = pos;
							isback = false;
							continue;
						}
					}
				}
				e = wxString::Format("\\g{n} incorrect in pos %d", i);
			}
			isback = false;
		}
		a.Append(c);
		if (e.length() > 0) break;
	}
	if (a.length() > 0) rez.push_back({ false,-1,a });
	return rez;
}
void dlgTransformText::AppendTextControl(ctlStyledText* ctrl, const wxString appendtext, bool isnewline) {
	bool isLimit = limitLine <= 0 || limitChar <= 0;
	if (ctrl == srcText) {
		// control limit
		if (isnewline)
			if (limitLine > 0)
				limitLine--;
			else
				isLimit = true;
		if (limitChar > 0)
			limitChar -= appendtext.length();
		else
			isLimit = true;
		if (!isLimit) ctrl->AppendText(appendtext);
	}
	else {
		strResult.Append(appendtext);
		ctrl->AppendText(appendtext);
	}
}
wxJSONValue dlgTransformText::FillConfig() {
	wxString s;
	wxJSONValue n(wxJSONType::wxJSONTYPE_OBJECT);
	s=beforeRowCB->GetValue();
	n["beforeRow"]=s;
	s = afterRowCB->GetValue();
	n["afterRow"] = s;
	s = sepFieldCB->GetValue();
	n["sepField"] = s;
//	s= afterFieldCB->GetValue();
//	n["afterField"] = s;
//	s=beforeFieldCB->GetValue();
//	n["beforeField"] = s;
	s= txtField->GetText();
	n["regexp"] = s;
	s= trgField->GetText();
	n["replace"] = s;
	return  n;
};
void dlgTransformText::OnChange(wxCommandEvent& ev) {
	OnComboSelect(ev);
	isNeedTransform = true;
}
void dlgTransformText::OnChangeLimit(wxCommandEvent& ev) {
	CheckLimits();
	isNeedTransform = true;
}
void dlgTransformText::OnChangeOnline(wxCommandEvent& ev) {
	isOnline = chkHighLight->IsChecked();
}

void dlgTransformText::OnLoad(wxCommandEvent& ev) {
	wxString s = optionsLoadCB->GetValue();
	//lastconf.Clear();
	for (auto key: lastconf.GetMemberNames())
		lastconf.Remove(key);
	LoadConfig(s);
	OnCheckUI(ev);
}
void dlgTransformText::OnComboSelect(wxCommandEvent& ev) {
	//compare lastconf
	if (inizialize) return;
	wxJSONValue formconf = FillConfig();
	wxArrayString ar;
	ar=formconf.GetMemberNames();
	isChange = false;
	for (auto s:ar) {
		wxString ls = lastconf[s].AsString();
		wxString form = formconf[s].AsString();
		if (form == ls) continue;
		isChange = true;
		// 
		if (s == "regexp") {

		}
		break;
	}
	wxString s = optionsLoadCB->GetValue();
	bool isSimbol = s.length() > 1 && s.Mid(0, 1) == '*';
	wxString name;
	if (isSimbol) name = s.Mid(1); else name = s;
	if (optionsLoadCB->FindString(name) < 0) {
		isChange = false; // new config

	}
	if (isChange && !isSimbol) {
		//wxString s = optionsLoadCB->GetValue();
		if (!isSimbol) {
			s = '*' + name;
			optionsLoadCB->SetValue(s);
		}
	}
	if (!isChange && isSimbol && ev.GetEventObject() != optionsLoadCB) optionsLoadCB->SetValue(name);
	OnCheckUI(ev);

}
void dlgTransformText::OnSave(wxCommandEvent& ev) {
	wxString s= optionsLoadCB->GetValue();

	if (s.length() > 1 && s.Mid(0, 1) == '*') {
		s = s.Mid(1);
	}
	else {
		if (optionsLoadCB->FindString(s)<0) optionsLoadCB->Append(s);
	}

	opt["currentConfingName"] = s;
	wxJSONValue formconf = FillConfig();
	lastconf = formconf;
	opt["configurations"][s] = formconf;
	opt["configurations"]["last"] = formconf;
	settings->WriteJsonObect(dlgName, opt);
	settings->WriteJsonFile();
	isChange = false;
	optionsLoadCB->SetValue(s);
	OnCheckUI(ev);
}

void dlgTransformText::OnCheckUI(wxCommandEvent& ev) {
	wxString s = optionsLoadCB->GetValue();
	bool issave = false;
	if (s.length() > 1 && s.Mid(0, 1) == '*') issave = true;
	if (s.length() > 0 && s.Mid(0, 1) != '*') {
		if (optionsLoadCB->FindString(s) < 0) issave = true;;
		
	}
	btnSave->Enable(issave && isChange);
	bool iscopy = strResult.length() > 0;
	if (strResult.length()>0) btnSave->Enable(iscopy);
}
void dlgTransformText::OnIdle(wxIdleEvent& ev) {
	if (inizialize) return;
	if (!isOnline) return;
//	int u = ev.GetUpdated();
//	if (!(u & wxSTC_UPDATE_CONTENT)) return;
	wxRegEx regfld;
	wxString v = txtField->GetText();
	wxString r = trgField->GetText();
	if (v.length() == 0 || src.length() == 0 || r.length() == 0) return;
	if (!isNeedTransform) return;

	wxCommandEvent evt;
	strResult = wxEmptyString;
	bool fieldexp = false;
	//if (v == strReg && strRep == r) return;
	{
		wxLogNull logNo;
		fieldexp = regfld.Compile(v, wxRE_NEWLINE);
		if (!fieldexp) {
			trgText->ClearAll();
			m_msg->SetLabelText(wxString::Format("Error regexp compile!!!"));
		}
	}
	if (fieldexp) {
		//txtField->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

		//txtField->StyleSetBackground(wxSTC_STYLE_DEFAULT, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	}
	else {

		//txtField->SetBackgroundColour(bgerror);
		//txtField->StyleSetBackground(wxSTC_STYLE_DEFAULT, bgerror);
		//srcText->ClearAll();
		//int ls = srcText->GetText().length();
		//if ((ls> limitChar )ls!=src.length())
		//srcText->SetText(src.Mid(0, limitChar));
		srcText->ClearDocumentStyle();
		
		
	}
	//txtField->Update();
	if (fieldexp) {
		TransformText(regfld);
		strRep = r;
		strReg = v;

		OnComboSelect(evt);
	}
	else
	{
		strReg = "";
		strRep = "";
	}
	OnCheckUI(evt);
	isNeedTransform = false;
	ev.Skip();
}
void dlgTransformText::OnChangeRegEx2(wxStyledTextEvent& ev) {
	int u = ev.GetUpdated();
	if ((u & wxSTC_UPDATE_CONTENT)) isNeedTransform = true;

}
void dlgTransformText::OnClose(wxCloseEvent& ev) {
	this->Hide();
	if (ev.CanVeto())
		ev.Veto();
}
void dlgTransformText::OnCancel(wxCommandEvent& ev)
{
	//SaveSettings();
	this->Hide();
}

void dlgTransformText::OnOk(wxCommandEvent& ev) {
	wxCloseEvent event;
	if (strResult.length() > 0) {

	
	//pgDialog::OnClose(event);
	if (wxTheClipboard->Open())
	{
		wxDataObjectComposite* dataobj = new wxDataObjectComposite();
		dataobj->Add(new wxTextDataObject(strResult));
		wxTheClipboard->SetData(dataobj);
		wxTheClipboard->Close();
	}
	}
	this->Hide();
}
void dlgTransformText::SetStyled(ctlStyledText* s) {
	int regstyle = wxSTC_STYLE_LASTPREDEFINED + 1;
	s->StyleClearAll();
	countGroupColor = opt["colorGroup"].Size();
	for (int i = regstyle; i < regstyle + countGroupColor; i++) {
		wxString strcl = opt["colorGroup"][i - regstyle].AsString();
		wxColour clr(strcl);
		if (clr.IsOk())
		{
			s->StyleSetBackground(i, clr);
		}
		else {
			s->StyleSetBackground(i, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
		}
		s->StyleSetForeground(i, *wxBLACK);
	}

}
void dlgTransformText::TransformText(const wxRegEx &regfld) {
// Check params
	struct interval {
		size_t start = -1;
		size_t len = 0;
		int style = 0;
	};
	interval m[100];
	wxRegEx RegNewLine(srcRowSep, wxRE_NEWLINE);
	srcText->StyleClearAll();
	//srcText->SetText(src);
	srcText->ClearAll();
	trgText->StyleClearAll();
	trgText->ClearAll();
	strResult = wxEmptyString;

	wxString c = opt["bgErrorColor"].AsString();
	wxColour red(c);
	//c = opt["colorGroup"][0].AsString();
	wxColour sel("#ffff00");
	int regstyle = wxSTC_STYLE_LASTPREDEFINED + 1;
	SetStyled(srcText);
	SetStyled(trgText);
	//bool fieldexp = regfld.Compile(v, wxRE_NEWLINE);
	if (regfld.IsValid()) {
		int sstart = 0;
		//int s_indicHighlight = 20;
		//srcText->IndicatorSetForeground(s_indicHighlight, wxColour(80, 236, 120));
		//srcText->IndicatorSetForeground(s_indicHighlight, *wxBLUE);
		//srcText->IndicatorSetStyle(s_indicHighlight, wxSTC_INDIC_DASH);
		wxString tmp = src;
		int numLine = 0;
		bool isnewline = false;
//		wxString sepRows = txtSepRows->GetValue();
//		wxString sepField = txtSepField->GetValue();
		wxString trgFieldReg = trgField->GetText();
		std::vector<replace_opt> repArray;
		repArray = BuildString(trgFieldReg);
		if (repArray.size() == 0) {
			trgField->SetBackgroundColour(red);
		}
		else {
			trgField->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
		}
		wxString sepRbegin, sepRend, sepF;
		sepRbegin=beforeRowCB->GetValue();
		sepRend= afterRowCB->GetValue();
		sepF = sepFieldCB->GetValue();
		//sepFend = afterFieldCB->GetValue();

		// build Rows separator
		std::vector<replace_opt> sepRowEndArray;
		sepRowEndArray = BuildString(sepRend);
		if (sepRowEndArray.size() == 0) {
			afterRowCB->SetBackgroundColour(red);
		}
		else {
			afterRowCB->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
		}
		bool isSepRowEndConst = true;
		wxString SepRowEndConst;
		for (auto& j : sepRowEndArray) {
			if (j.isGroup) {
				isSepRowEndConst = false;
				break;
			}
			else {
				SepRowEndConst.Append(j.text);
			}
		}
		// Matchs regexpp
		wxString sepLine = "";
		int numField = 0;
		int cntMathes = 0;
		int cntLines = 0;
		int tmp_limitChar = limitChar;
		int tmp_limitLine = limitLine;
		while (regfld.Matches(tmp)) {
			size_t start, len;
			int lastBytePosition = srcText->GetLastPosition();
			int lastCharPosition = srcText->GetTextLength();
			regfld.GetMatch(&start, &len, 0);
			if (len == 0) break;
			cntMathes++;
			wxString no_match = tmp.Mid(0, start);
			int maxgroup = regfld.GetMatchCount();
			// build sepLine string dynamic for next line
			sepRend = wxEmptyString;
			if (isSepRowEndConst) {
				sepRend = SepRowEndConst;
			}
			else
				for (auto& j : sepRowEndArray) {
					if (j.isGroup) {
						if (j.nGroup >= 0 && j.nGroup < maxgroup) {
							wxString s = regfld.GetMatch(tmp, j.nGroup);
							sepRend.Append(s);
						}
					}
					else {
						sepRend.Append(j.text);
					}
				}
			// end build
			if (no_match.length() > 0) {
				isnewline = RegNewLine.Matches(no_match);
				//srcText->AddText(no_match);
				AppendTextControl(srcText, no_match, isnewline);
				if (isnewline) AppendTextControl(trgText, sepRend + sepLine);
				if (isnewline) cntLines++;

			}
			wxString fnd = tmp.Mid(start, len);
			if (isnewline) {
				//trgText->AddText(sepRbegin );
				AppendTextControl(trgText, sepRbegin);
				numLine++;
				numField = 0;
				isnewline = false;
			}
			else {
				if (numField > 0) AppendTextControl(trgText, sepF);
			}
			if (numLine == 0 && numField == 0) AppendTextControl(trgText, sepRbegin);

			//srcText->AddText(fnd);
			
			AppendTextControl(srcText, fnd);
			numField++;
			if (maxgroup > 0) {
				// colorer groups
				size_t start1, len1;
				size_t start_frame = start;
				int currentIdx = 0;
				for (int i = 0; i < maxgroup; i++)
				{
					regfld.GetMatch(&start1, &len1, i);
					if (i == 0) m[currentIdx++] = { start1, len1,0 + regstyle };
					if (i == 0 || len1 == 0) continue;
					// group color
					// разделение отрезков 
					int maxSizeArray = sizeof(m) / sizeof(m[0]);
					bool isApeendInterval = false;
					int shiftcount = 0;
					for (int idx = 0; idx < currentIdx; idx++) {
						interval ii = m[idx];
						int nextinterval = ii.start + ii.len;
						if (nextinterval < start1 || ii.len == 0) continue;		// предудущие интервалы
						//nextinterval < start1 + len1 &&
						if (nextinterval < start1 + len1) {
							if (currentIdx - 1 != idx) continue; // только в конеч добавляем.
							//следующие интеравлы, добавим сюда наш новый интервал

							shiftcount = 1;
							isApeendInterval = true;
							if (currentIdx + shiftcount >= maxSizeArray) {
								// переполнение массива
								break;
							}
							if (shiftcount > 0) for (int j = currentIdx - 1; j > idx; j--) m[j + shiftcount] = m[j];
							m[idx + 1] = { start1,len1,i + regstyle };
							break;
						}
						// попали в текущий интервал, разделим его
						// сдвинем элементы на два,один или ноль вправо
						shiftcount = 2;
						int oldleftidx = idx;
						int oldrightidx = idx + 2;
						int newcenter = idx + 1;
						if (start1 == ii.start) {
							shiftcount--; oldleftidx = -1; newcenter = idx; oldrightidx--;
						}; // левая часть полностью замещается, стыковака к левой границе

						if (nextinterval == start1 + len1) {
							oldrightidx = -1; newcenter = idx + 1;
							shiftcount--; // правая часть не нужна стыкова к правой границе
							// сдвиг на shiftcount элементов
							if (shiftcount == 0) newcenter = idx;
						}
						if (currentIdx + shiftcount >= maxSizeArray) {
							// переполнение массива
							isApeendInterval = true;
							break;
						}
						if (shiftcount > 0) for (int j = currentIdx - 1; j > idx; j--) m[j + shiftcount] = m[j];
						currentIdx += shiftcount;
						if (oldleftidx != -1) m[idx].len = start1 - ii.start; // правая часть старого интеравла корректируется по длине
						m[newcenter] = { start1,len1,i + regstyle };     // новый интервал в конец массива
						if (oldrightidx != -1)
							m[oldrightidx] = { start1 + len1,ii.len - (start1 - ii.start) - len1 ,ii.style };     // вторая часть старого интервала в конец массива
						isApeendInterval = true;
						break;
					}
					if (!isApeendInterval) m[currentIdx++] = { start1,len1,i + regstyle };
				}
				// set style for text
				int maxchar = srcText->GetTextLength();
				
				for (auto& ii : m) {
					if (--currentIdx < 0) break;
					if (sstart + ii.start + ii.len > maxchar) break;
					int CharPos = srcText->PositionRelative(0, sstart+ii.start);
					if (CharPos == 0 && sstart>1000) {
						//wxTrap();
						continue;
					}
					srcText->StartStyling(CharPos);
					if (ii.style - regstyle > countGroupColor) ii.style = countGroupColor + regstyle - 1;
					int CharPosEnd = srcText->PositionRelative(CharPos, ii.len);
					srcText->SetStyling(CharPosEnd - CharPos, ii.style);
				}
				// build replace string
				wxString replstr;
				interval m2[100];
				currentIdx = 0;
				start_frame = 0;
				for (auto& j : repArray) {
					wxString s;
					if (j.isGroup) {
						if (j.nGroup >= 0 && j.nGroup < maxgroup) {
							s = regfld.GetMatch(tmp, j.nGroup);
							replstr.Append(s);
							m2[currentIdx++] = { start_frame ,s.length(),j.nGroup };
						}
					}
					else {
						s = j.text;
						replstr.Append(s);
					}
					start_frame += s.length();
				}
				//trgText->AddText(replstr);
				//AppendTextControl(trgText, );
				int start2 = trgText->GetTextLength();
				AppendTextControl(trgText, replstr);
				maxchar = trgText->GetTextLength();
				for (auto& ii : m2) {
					if (--currentIdx < 0) break;
					if (start2 + ii.start + ii.len > maxchar) break;
					int CharPos = trgText->PositionRelative(start2,  ii.start);
					if (CharPos < start2 ) {
						//wxTrap();
						continue;
					}
					trgText->StartStyling(CharPos);
					if (ii.style > countGroupColor) ii.style = countGroupColor + regstyle - 1;
					int CharPosEnd = trgText->PositionRelative(CharPos, ii.len);
					trgText->SetStyling(CharPosEnd - CharPos, ii.style + regstyle);
				}


			}
			else {
				AppendTextControl(trgText, fnd);
			}

			//if (sstart == -1) sstart = 0;

			sstart += start + len;
			tmp = tmp.Mid(start + len);
		}
		if (numField > 0) AppendTextControl(trgText, sepRend);

		if (sstart == 0) {
			srcText->SetText(src.Mid(0, limitChar));
		}
		else if (tmp.length() > 0 && limitChar>0 ) AppendTextControl(srcText, tmp.Mid(0, limitChar)); // no_match
		limitChar = tmp_limitChar;
		limitLine = tmp_limitLine;
		int srcl = src.length();
		m_msg->SetLabelText(wxString::Format("Lines %d Char %d Matches %d", cntLines, srcl, cntMathes));
	}
	else
	{
		//txtField->SetBackgroundColour(red);
		srcText->SetText(src.Mid(0, limitChar));
	}
	if (srcText->GetTextLength()>= limitChar) srcText->AppendText("\n...");
	srcText->Update();
	txtField->Refresh();

}