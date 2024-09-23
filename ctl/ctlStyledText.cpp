
#include "pgAdmin3.h"

#include "ctl/ctlStyledText.h"
#include <wx/log.h>
#include <wx/regex.h>

BEGIN_EVENT_TABLE(ctlStyledText, wxStyledTextCtrl)
//EVT_KEY_DOWN(ctlStyledText::OnKeyDown)
//	EVT_ERASE_BACKGROUND(ctlSQLBox::OnBackGround)
#ifdef __WXMAC__
EVT_STC_PAINTED(-1, ctlStyledText::OnPositionStc)
#else
EVT_STC_UPDATEUI(-1, ctlStyledText::OnPositionStc)
#endif
//EVT_STC_MARGINCLICK(-1, ctlSQLBox::OnMarginClick)
//EVT_STC_DOUBLECLICK(-1, ctlSQLBox::OnDoubleClick)

END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(ctlStyledText, wxStyledTextCtrl)

ctlStyledText::ctlStyledText()
{
	m_regparser.SetStyleControl(this);
	setDecorate();
}
void ctlStyledText::setDecorate() {
	extern sysSettings* settings;
	wxFont fntSQLBox = settings->GetSQLFont();

	StyleSetBackground(wxSTC_STYLE_BRACELIGHT, wxColour(0x99, 0xF9, 0xFF));
	StyleSetBackground(wxSTC_STYLE_BRACEBAD, wxColour(0xFF, 0xCF, 0x27));
	StyleSetFont(wxSTC_STYLE_BRACELIGHT, fntSQLBox);
	StyleSetFont(wxSTC_STYLE_BRACEBAD, fntSQLBox);

}
ctlStyledText::ctlStyledText(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
	Create(parent, id, pos, size, style);
	m_regparser.SetStyleControl(this);
	setDecorate();
}
void ctlStyledText::OnPositionStc(wxStyledTextEvent& event) {

	int u = event.GetUpdated();
	if ((u & wxSTC_UPDATE_CONTENT) && ishighlight) {
		wxString t = GetText();
		{
			//			wxLogNull logNo;
			//			wxRegEx r(t, wxRE_NEWLINE);
		}
		if (m_regparser.ParseStr(t, RegExpParser::defrule::pcre) > 0) {
		}
		m_regparser.StyledTextControl(this);

	}

	int pos = GetCurrentPos();
	wxChar ch = GetCharAt(pos - 1);
	wxChar nextch = GetCharAt(pos);
	int st = GetStyleAt(pos - 1);
	int match;
	// Line numbers
	// Ensure we don't recurse through any paint handlers on Mac
	// Roll back through the doc and highlight any unmatched braces
	int tmp = pos;

	while ((pos--) >= 0)
	{
		ch = GetCharAt(pos);
		st = GetStyleAt(pos)- (wxSTC_STYLE_LASTPREDEFINED + 1);
		if ( (
			//ch == '{' || ch == '}' ||
			//ch == '[' || ch == ']' ||
			ch == '(' || ch == ')') &&
			st != 5 )
		{
			match = BraceMatch(pos);
			if (match == wxSTC_INVALID_POSITION)
			{
				event.Skip();
				return;
			}
			BraceHighlight(pos, match);
			break;
		}
	}


	event.Skip();
}
void ctlStyledText::setRegExphighlight(bool enable) {
	ishighlight = enable;
	if (!enable) m_regparser.SetStyleControl(NULL);
}

///////////////////////////// RegExpParser

void RegExpParser::init() {
	for (int i = 0; i < sizeof(rulemap) / sizeof(rulemap[0]); i++) rulemap[i] = -1;
	for (int i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
		if (ISCHECKTYPE(i, rule_type::start_rule)) {
			wxASSERT_MSG(rules[i].ruledef >= 0, "rule < 0 this bad");
			rulemap[rules[i].ruledef] = i;
		}
	}
	// check all refernce
	for (int i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
		if (rules[i].ruleref > 0) {
			wxASSERT_MSG(rulemap[rules[i].ruleref] >= 0, wxString::Format("rule %d not define", rules[i].ruleref));
		}
	}
};
int RegExpParser::ParseStr(const wxString str, defrule start_rule) {
	t = str;
	len = t.length();
	maxPos = 0;
	int idx = rulemap[start_rule];
	int n;
	marker.clear();
	int r = testRule(idx, 0, n, -1);
	if (r > 0) {
		if (n == len) return 1;
		if (n < len) {
			if (marker.size() > 0) marker.push_back(styletextmarker{ 0, n,len });
		}
		return -11;
	}

	return -1;
}
void RegExpParser::SetStyleControl(wxStyledTextCtrl* ctrl) {
	if (ctrl == NULL) {
		intitstyle = false;
		return;
	}
	//ctrl->StyleClearAll();
	for (int i = 0; i < sizeof(stylemap) / sizeof(stylemap[0]); i++) stylemap[i] = -1;
	wxColour bgdef = ctrl->GetBackgroundColour();
	wxColour fgdef = ctrl->GetForegroundColour();
	tablestyle.push_back(styletextdef{ fgdef,wxColour("#ddd0fe") }); // 00 verb
	tablestyle.push_back(styletextdef{ fgdef,wxColour("#fed1ff") }); // 1 capture (reed)
	tablestyle.push_back(styletextdef{ fgdef,wxColour("#e3e3e3") }); // 2 backslash
	tablestyle.push_back(styletextdef{ fgdef,wxColour("#d3a776") }); // 3 class char 
	tablestyle.push_back(styletextdef{ fgdef,wxColour("#99beff") }); // 4 quantifier
	tablestyle.push_back(styletextdef{ wxColour("#989898"),bgdef }); // 5 comment
	tablestyle.push_back(styletextdef{ fgdef,wxColour("#bae634") }); // 6 captute (green)

	tablestyle.push_back(styletextdef{ fgdef,wxColour("#ed5c65") }); // last color.  error bg
	stylemap[0] = tablestyle.size() - 1; // error bg

	stylemap[defrule::option_setting] = 0;
	stylemap[defrule::atomic_group] = 0;
	stylemap[defrule::lookaround] = 0;
	stylemap[defrule::quantifier] = 4;
	stylemap[defrule::comment] = 5;
	stylemap[defrule::character_class] = 3;
	stylemap[defrule::posix_character_class] = 3;
	stylemap[defrule::character_type] = 2;
	stylemap[defrule::character] = 2;
	stylemap[defrule::character_type] = 2;
	stylemap[defrule::backreference] = 2;
	stylemap[defrule::anchor] = 2;
	stylemap[defrule::match_point_reset] = 2;
	stylemap[defrule::backtracking_control] = 0;
	stylemap[defrule::alternation] = 0;
	stylemap[defrule::capture] = 6;
	stylemap[defrule::capture_1] = 6;
	stylemap[defrule::conditional_pattern] = 1;
	//wxLogNull logNo;
	int count = 0;
	int userstyle = wxSTC_STYLE_LASTPREDEFINED + 1;
	for (int i = 0; i < sizeof(stylemap) / sizeof(stylemap[0]); i++) {
		if (stylemap[i] != -1) {
			int k = stylemap[i];
			stylemap[i] += userstyle;
			ctrl->StyleSetBackground(stylemap[i], tablestyle[k].bg);
			ctrl->StyleSetForeground(stylemap[i], tablestyle[k].fg);
		}
	}
	intitstyle = true;
}
void RegExpParser::StyledTextControl(wxStyledTextCtrl* ctrl) {
	if (!intitstyle) SetStyleControl(ctrl);
	ctrl->ClearDocumentStyle();
	if (marker.size() == 0) {
		marker.push_back(styletextmarker{ 0,maxPos + 1,maxPos + 2 });
	}
	int userstyle = wxSTC_STYLE_LASTPREDEFINED + 1;
	for (auto m : marker) {
		if (m.rule >= 0) {
			int style = stylemap[m.rule];
			if (style < 0) continue;
			int CharPos = ctrl->PositionRelative(0, m.start);
			ctrl->StartStyling(CharPos);
			int CharPosEnd = ctrl->PositionRelative(CharPos, m.end - m.start);
			int ll = CharPosEnd - CharPos;
			//ctrl->SetStyling(m.end - m.start, style);
			ctrl->SetStyling(ll, style);
		}
	}
}
bool RegExpParser::iseqchar(int pos, const char* const str) {
	int i = 0;
	int n = len - pos;
	char c;
	if (n == 0)
		return false;
	wxChar tc = t[pos];
	while (str[i] != 0) {
		c = str[i++];
		if (c == tc) return true;
	}
	return false;
}
bool RegExpParser::istexteq(int pos, int& nextpos, const char* str) {
	int i = 0;
	int n = len - pos;
	char c;
	while (str[i] != 0) {
		if (n - i == 0) {
			return false; // end of string
		}
		c = str[i];
		wxChar tc = t[pos + i++];
		if (c == tc) continue;
		return false;
	}
	//equal
	nextpos = pos + i;
	return true;
}
int RegExpParser::testRule(int ruleindex, int startCharPosition, int& nextCharPosition, int ruleparent) {
	int p = startCharPosition;
	int n = p;
	bool isruleok = false;
	bool isanonim = false;
	int ri = ruleindex;
	int rule = -1;
	int ruletype = 0;
	int sizemarker = marker.size();
	if (ISCHECKTYPE(ri, start_anonim_rule)) {
		ri++;
		rule = ruleparent;
	};
	int countOkrule = 0;
	int nextel = true;
	int nextRule = 0;
	while (nextel) {
		p = n;
		int incnextrule = 1;
		if (rules[ri].type == 0) { // end rule
			break;
		}
		if (ISCHECKTYPE(ri, start_rule)) {
			rule = rules[ri].ruledef;
			ruletype = rules[ri].type;
			ri = ri + incnextrule;
			continue;
		}
		isruleok = false;
		if (ISCHECKTYPE(ri, text)) {
			// 
			isruleok = (istexteq(p, n, rules[ri].text));
			//if (ISCHECKTYPE(ri, inversion) && p != len) isruleok = !isruleok;
			if (isruleok) marker.push_back(styletextmarker{ rule,p,n });
		}
		else if (ISCHECKTYPE(ri, char_any)) {
			int n2 = p;
			if (rule == defrule::other && (maskOpt & maskopt::extended) && (p < len && t[p] == '#')) {
				//extended comment 
				while (n2 < len && t[n2] != '\n') n2++;
				if (n2 < len) n2++;
				marker.push_back(styletextmarker{ defrule::comment,p,n2 });
				isruleok = true;
				n = n2;
			}
			else {
				isruleok = iseqchar(p, rules[ri].text);
				if (ISCHECKTYPE(ri, inversion) && p != len) isruleok = !isruleok;
				if (isruleok) n = p + 1;
				if (isruleok) marker.push_back(styletextmarker{ rule,p,n });
			}
		}
		else if (ISCHECKTYPE(ri, special_dot)) {
			isruleok = false;
			int n2 = p;
			{
				if (ISCHECKTYPE(ri, zero_more) && ISCHECKTYPE(ri + 1, text)) {
					isruleok = istexteq(p, n2, rules[ri + 1].text);
					isruleok = !isruleok;
					if (isruleok) n2++;
				}
				else {
					isruleok = true; // only 1 spec dot
					n2++;
				}

				if (p == len || n2 == p) isruleok = false; // end expression alaways false
				if (isruleok) n = n2;
			}
		}
		else if (rules[ri].ruleref >= 0) {
			//Use defibe rule
			int r = GETRULEREFINDEX(ri);
			isanonim = ISCHECKTYPE(ri, start_anonim_rule);
			if (isanonim) r = ri;
			nextRule = testRule(r, p, n, rule);
			isruleok = nextRule > 0;
			if (isanonim) {
				if (!isruleok) nextRule = nextRule * -1;
				incnextrule = nextRule - ri + 1; // next element after ZERO
			}
			else
				nextRule = 0;
		}
		// 
		if (isruleok) countOkrule++;
		if (countOkrule < 2 && (ISCHECKTYPE(ri, zero_one))) {
			isruleok = true;
		}
		if (countOkrule >= 0 && (ISCHECKTYPE(ri, zero_more))) {
			if (countOkrule > 0 && isruleok) continue;
			if (countOkrule >= 0) isruleok = true;
		}
		if ((ISCHECKTYPE(ri, one_more))) {
			if (countOkrule > 0 && isruleok) continue;
			// zero - this false rule
			if (countOkrule > 0) isruleok = true;
		}
		// next element
		countOkrule = 0;
		ri = ri + incnextrule;
		// NEXT elemenT
		if (isruleok && ISCHECKTYPE(ri, alternative)) break; // end rule
		if (isruleok) continue;
		n = p; // undo position
		n = startCharPosition;
		// find alternative or end rule (control anonim rule)
		int llvel = 0;
		while (!((rules[ri].type == 0 || ISCHECKTYPE(ri, alternative)
			) && llvel == 0)
			)
		{ // find alternative or end rule

			if (ISCHECKTYPE(ri, start_anonim_rule)) {
				llvel++; // inner rule 
			}
			if ((rules[ri].type == 0) && (llvel != 0)) {
				llvel--; //end inner rule
			}
			ri++;
		}
	}

	// 
	// isruleok

	if (isruleok) {
		p = n;
		nextCharPosition = p;
		if (p > maxPos) maxPos = p;
		// find end rule (control anonim rule)
		int llvel = 0;
		while (!((rules[ri].type == 0
			) && llvel == 0)
			) { // find end rule
			if (ISCHECKTYPE(ri, start_anonim_rule)) {
				llvel++; // inner rule 
			}
			if ((rules[ri].type == 0) && (llvel != 0)) {
				llvel--; //end inner rule
			}
			ri++;

		}
		if ((ruletype & styled_ALL) == styled_ALL) {
			// undo marker
			marker.resize(sizemarker);
			// replace all rule
			marker.push_back(styletextmarker{ rule,startCharPosition,n });
		}
		// check is option_setting ?

		if (rule == defrule::option_setting && (startCharPosition + 1<len) && t[startCharPosition + 1] == '?') {
			// (?is-xJ)
			int maskset = 0;
			int maskreset = 0;
			bool neg = false;
			int mask = 0;
			for (int i = startCharPosition; i < nextCharPosition; i++) {
				wxChar c = t[i];
				if (c == 'i') mask |= (int)maskopt::caseless;
				if (c == 'J') mask |= (int)maskopt::allow_duplicate_names;
				if (c == 'm') mask |= (int)maskopt::multiline;
				if (c == 's') mask |= (int)maskopt::single_line;
				if (c == 'U') mask |= (int)maskopt::default_ungreed;
				if (c == 'x') mask |= (int)maskopt::extended;
				if (c == '-') {
					maskset = mask;
					mask = 0;
					neg = true;
				}
			}
			if (neg) {
				maskOpt |= maskset;
				maskreset = ~mask;
				maskOpt &= maskreset;
			}
			else if (mask > 0) {
				maskOpt |= mask;
			}
		}
		return ri; // end rule position , isruleok=true;
	}
	else {
		// undo marker
		marker.resize(sizemarker);

		nextCharPosition = startCharPosition;
		return -ri; //- end rule position isruleok=false;
	}

}

