#ifndef _CTLSTYLEDTEXT_H_
#define _CTLSTYLEDTEXT_H_
#include <wx/stc/stc.h>
#include <vector>


struct styletextmarker {
	int rule; // rule
	int start;
	int end;	// next char
};
struct styletextdef {
	wxColour fg;
	wxColour bg;
};



class RegExpParser
{
public:
	enum  maskopt {
		caseless = 1,
		allow_duplicate_names = 2,
		multiline = 4,
		single_line = 8,
		default_ungreed = 16,
		extended = 32
	};
	enum defrule {
		digits = 1,
		digit,
		quoting,
		letter,
		letters,
		name,
		newline_conventions,
		option_setting,
		option_setting_1,
		option_setting_11,
		option_setting_111,
		option_setting_12,
		option_setting_2,
		option_setting_21,
		option_setting_22,
		option_setting_flag,
		option_setting_flag_cap,
		backtracking_control,
		backtracking_control_1,

		pcre,
		expr,
		alternation,
		element,
		atom,
		quantifier,
		comment,
		capture,
		capture_1,
		atomic_group,
		lookaround,
		backreference,
		hex,
		hex4,
		anchor,
		match_point_reset,
		character,
		character_type,
		posix_character_class,
		posix_character_class_name,
		character_class_range_atom,
		character_class_range,
		character_class_atom,
		character_class,
		conditional_pattern,
		call_out,

		other,

		MAX_COUNT_RULE
	};
	enum rule_type {
		end_rule = 0,
		start_rule = 1,
		zero_one = 2,
		one_more = 4,
		zero_more = 8,
		alternative = 16,
		char_any = 32,
		text = 64,
		none = 128,
		special_dot = 256,
		start_anonim_rule = 512,
		inversion = 1024,
		styled_parent = 1024 * 2,
		styled_ALL = 1024 * 4,
	};
	struct ERule {
		int type;
		int ruleref = 0; // use rule reference
		int ruledef; // start define rule
		const char* text;
	};
	RegExpParser() {
		init();
		intitstyle = false;
	}
	int ParseStr(const wxString str, RegExpParser::defrule start_rule);
	void SetStyleControl(wxStyledTextCtrl* ctrl);
	void StyledTextControl(wxStyledTextCtrl* ctrl);
	static void TEST() {

		const char* const t1[] = { "F0", "Z0T_a1234", "f" };
		const char* const t2[] = { "(?i-ix)", "(*UTF)" };
		const char* const t3[] = { "(*SKIP:m9)","(*:q2)", "(*FAIL)", "(*MARK:q)" };
		const char* const t4[] = { "(?#аы)","(?#d2)", };
		const char* const t5[] = { "(?|das)","(?i-i:ab)", "$ \\p{Fre}","(?>ATM)\\n\\0\\x{FFF}\\u1234abcd","(?P=ss)", "\\g{1}", "(P=ss)","\\g2",".\\V","\\Q\\s\\E" };
		const char* const t6[] = { "\\0","\\u1abc", "\\o{881}","\\xaa" };
		const char* const t7[] = { ".","\\V", "\\p{^ssa&}","\\psd" };
		const char* const t8[] = { "[[:^digit:]]", "[[:digit:]]" };
		const char* const t9[] = { "(?(sd)ffff)","(?(-23)cond_expr|no_expr_cond)" };

		test_2(RegExpParser::defrule::pcre, t5, sizeof(t5) / sizeof(t5[0]));
		test_2(RegExpParser::defrule::conditional_pattern, t9, sizeof(t9) / sizeof(t9[0]));
		test_2(RegExpParser::defrule::character_class, t8, sizeof(t8) / sizeof(t8[0]));
		test_2(RegExpParser::defrule::character_type, t7, sizeof(t7) / sizeof(t7[0]));
		test_2(RegExpParser::defrule::character, t6, sizeof(t6) / sizeof(t6[0]));
		test_2(RegExpParser::defrule::backtracking_control, t3, sizeof(t3) / sizeof(t3[0]));

		//test_2(RegExpParser::defrule::backreference, t5);
		test_2(RegExpParser::defrule::comment, t4, sizeof(t4) / sizeof(t4[0]));
		test_2(RegExpParser::defrule::option_setting, t2, sizeof(t2) / sizeof(t2[0]));
		test_2(RegExpParser::defrule::name, t1, sizeof(t1) / sizeof(t1[0]));


	}
	static void test_2(RegExpParser::defrule rule, const char* const arr[], int arraysize) {
		RegExpParser t;
		int nt = 1;
		int sz = arraysize;
		wxString ss;
		for (int i = 0; i < sz; i++) {
			ss = arr[i];
			int rez = t.ParseStr(ss, rule);
			wxString rezfail = wxString::Format("Test %d to %d failed\n Regexp expression: %s", nt, arraysize, wxString(ss));
			wxASSERT_MSG(rez >= 0, rezfail.c_str());
			nt++;
		}
	}
private:
	std::vector<styletextmarker> marker;
	std::vector<styletextdef> tablestyle;
	int stylemap[RegExpParser::defrule::MAX_COUNT_RULE] = { -1 };
	int rulemap[RegExpParser::defrule::MAX_COUNT_RULE] = { -1 };
	bool intitstyle;
	wxString t;
	int len;
	int maxPos = 0;   // last position
	int maskOpt = 0; // global options
	bool iseqchar(int pos, const char* const str);
	bool istexteq(int pos, int& nextpos, const char* str);
	void init();
	//////
	int testRule(int ruleindex, int startCharPosition, int& nextCharPosition, int ruleparent);
};
/// <summary>
/// ctlStyledText - Regexp hightli
/// </summary>
class ctlStyledText :
	public wxStyledTextCtrl
{
public:
	ctlStyledText();
	ctlStyledText(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style);
	void setRegExphighlight(bool enable);
	void setDecorate();
	DECLARE_DYNAMIC_CLASS(ctlStyledText)
		DECLARE_EVENT_TABLE()
private:
	void OnPositionStc(wxStyledTextEvent& event);
	RegExpParser m_regparser;
	bool ishighlight = false;
	// 
	//
};
//using namespace RegExpParser;
#define ZERO RegExpParser::ERule {0,0,0,0}
#define TEXTSIM(str)  RegExpParser::ERule {RegExpParser::rule_type::text,0,0,str}
#define TEXT_TYPE(type,str)  {type,0,0,str}
#define USE_RULE(type,rule)  {type,rule,0,0}
#define START_R(rule) {RegExpParser::rule_type::start_rule,0,rule,0}
#define START_R_STYLE(rule,style) {RegExpParser::rule_type::start_rule|style,0,rule,0}
#define START_A_R(type) {RegExpParser::rule_type::start_anonim_rule|RegExpParser::rule_type::styled_parent|type,0,0,0}
#define ISCHECKTYPE(i,typerule) ((rules[i].type & typerule) == typerule)
#define GETRULEREFINDEX(i) (rulemap[(rules[i].ruleref)])

const RegExpParser::ERule rules[] = {
	ZERO,
	START_R(RegExpParser::defrule::letter),
	  TEXT_TYPE(RegExpParser::rule_type::char_any, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_"),
	  ZERO,
	START_R(RegExpParser::defrule::letters),
	  USE_RULE(RegExpParser::rule_type::one_more,RegExpParser::defrule::letter),
	  ZERO,
	START_R(RegExpParser::defrule::anchor),
	  TEXTSIM("\\"),
	  TEXT_TYPE(RegExpParser::rule_type::char_any, "bBAzZG"),
	  TEXT_TYPE(RegExpParser::rule_type::char_any | RegExpParser::rule_type::alternative, "^$"),
	  ZERO,
	START_R(RegExpParser::defrule::match_point_reset),
	  TEXTSIM("\\K"),
	  ZERO,
	START_R(RegExpParser::defrule::backtracking_control_1),
	  TEXTSIM("ACCEPT"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "FAIL"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "COMMIT"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text | RegExpParser::rule_type::zero_one, "MARK"),
	  TEXTSIM(":"),
	  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::name),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "PRUNE"),
	  START_A_R(RegExpParser::rule_type::zero_one),
		  TEXTSIM(":"),
		  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::name),
		  ZERO,
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "SKIP"),
	  START_A_R(RegExpParser::rule_type::zero_one),
		  TEXTSIM(":"),
		  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::name),
		  ZERO,
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "THEN"),
	  START_A_R(RegExpParser::rule_type::zero_one),
		  TEXTSIM(":"),
		  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::name),
		  ZERO,
	  ZERO,
	START_R_STYLE(RegExpParser::defrule::backtracking_control,RegExpParser::rule_type::styled_ALL),
	  TEXTSIM("(*"),
	  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::backtracking_control_1),
	  TEXTSIM(")"),
	ZERO,
	START_R(RegExpParser::defrule::name),
	  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::letter),
	  START_A_R(RegExpParser::rule_type::zero_more),
		  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::letter),
		  USE_RULE(RegExpParser::rule_type::alternative,RegExpParser::defrule::digit),
		  ZERO,
	  ZERO,
	START_R(RegExpParser::defrule::option_setting_flag),
	  TEXT_TYPE(RegExpParser::rule_type::char_any, "iJmsUx"),
	  ZERO,
	START_R(RegExpParser::defrule::digit),
	  TEXT_TYPE(RegExpParser::rule_type::char_any, "0123456789"),
	  ZERO,
	START_R(RegExpParser::defrule::digits),
	  USE_RULE(RegExpParser::rule_type::one_more,RegExpParser::defrule::digit),
	  ZERO,
	START_R(RegExpParser::defrule::hex),
	  TEXT_TYPE(RegExpParser::rule_type::char_any, "0123456789abcdefABCDEF"),
	  ZERO,
	START_R(RegExpParser::defrule::quoting),
	  TEXTSIM("\\Q"),
	  TEXT_TYPE(RegExpParser::rule_type::special_dot | RegExpParser::rule_type::zero_more, ""),
	  TEXTSIM("\\E"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "\\"),
	  TEXT_TYPE(RegExpParser::rule_type::special_dot, ""),
	  ZERO,
	START_R(RegExpParser::defrule::newline_conventions),
	  TEXTSIM("CRLF"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"LF"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"CR"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"ANYCRLF"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"ANY"),
	  ZERO,

	  // option_setting
	START_R(RegExpParser::defrule::option_setting_22),
	  TEXT_TYPE(RegExpParser::rule_type::text,"-"),
	  USE_RULE(RegExpParser::rule_type::one_more,RegExpParser::defrule::option_setting_flag),
	  ZERO,
	START_R(RegExpParser::defrule::option_setting_21),
	  USE_RULE(RegExpParser::rule_type::one_more,RegExpParser::defrule::option_setting_flag),
	  USE_RULE(RegExpParser::rule_type::zero_one,RegExpParser::defrule::option_setting_22),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"-"),
	  USE_RULE(RegExpParser::rule_type::one_more,RegExpParser::defrule::option_setting_flag),
	  ZERO,
	START_R(RegExpParser::defrule::option_setting_111),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "UTF"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"UCP"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"NO_AUTO_POSSESS"),
	  USE_RULE(RegExpParser::rule_type::alternative,RegExpParser::defrule::newline_conventions),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"NO_START_OPT"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"BSR_ANYCRLF"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"BSR_UNICODE"),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"LIMIT_MATCH="),
	  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::digits),
	  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text,"LIMIT_RECURSION="),
	  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::digits),
	  ZERO,
	START_R(RegExpParser::defrule::option_setting_1),
	  TEXTSIM("*"),
	  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::option_setting_111),
	  ZERO,
	START_R(RegExpParser::defrule::option_setting_2),
	  TEXTSIM("?"),
	  USE_RULE(RegExpParser::rule_type::zero_one,RegExpParser::defrule::option_setting_21),
	  ZERO,
	START_R_STYLE(RegExpParser::defrule::option_setting,RegExpParser::rule_type::styled_ALL),
	  TEXTSIM("("),
		  START_A_R(RegExpParser::rule_type::none),
		  USE_RULE(RegExpParser::rule_type::none,RegExpParser::defrule::option_setting_1),
		  USE_RULE(RegExpParser::rule_type::alternative,RegExpParser::defrule::option_setting_2),
		  ZERO,
	  TEXTSIM(")"),
	  ZERO,
	  // PCRE START rule
	  START_R(RegExpParser::defrule::pcre),
		  USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::alternation),
		  ZERO,
	  START_R(RegExpParser::defrule::alternation),
		  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::expr),
		  START_A_R(RegExpParser::rule_type::zero_more),
			  TEXTSIM("|"),
			  USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::expr),
			  ZERO,
		  ZERO,
	  START_R(RegExpParser::defrule::expr),
		  USE_RULE(RegExpParser::rule_type::one_more, RegExpParser::defrule::element),
		  ZERO,
	  START_R(RegExpParser::defrule::element),
		  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::atom),
		  USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::quantifier),
		  ZERO,
	  START_R(RegExpParser::defrule::atom),		////////////////////////////////////////////  ATOM
		  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::option_setting),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::backtracking_control),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::atomic_group),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::lookaround),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::backreference),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::conditional_pattern),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::comment),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::capture),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::character),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::character_type),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::character_class),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::posix_character_class),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::letter),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::digit),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::anchor),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::match_point_reset),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::quoting),
		  USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::other),
		  ZERO,
	  START_R_STYLE(RegExpParser::defrule::quantifier, RegExpParser::rule_type::styled_ALL),
		  TEXT_TYPE(RegExpParser::rule_type::char_any, "?*+"),
		  START_A_R(RegExpParser::rule_type::zero_one),
			  TEXTSIM("+"),
			  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "?"),
			  ZERO,
		  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "{"),
		  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::digits),
		  START_A_R(RegExpParser::rule_type::zero_one),
			  TEXTSIM(","),
			  USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::digits),
			  ZERO,
		  TEXTSIM("}"),
		  START_A_R(RegExpParser::rule_type::zero_one),
			  TEXTSIM("+"),
			  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "?"),
			  ZERO,
		  ZERO,
	  START_R(RegExpParser::defrule::comment),
		  TEXTSIM("(?#"),
		  TEXT_TYPE(RegExpParser::rule_type::inversion | RegExpParser::rule_type::char_any | RegExpParser::rule_type::one_more, ")"),
		  TEXTSIM(")"),
		  ZERO,
	  START_R(RegExpParser::defrule::option_setting_flag_cap),
		  USE_RULE(RegExpParser::rule_type::one_more, RegExpParser::defrule::option_setting_flag),
		  USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::option_setting_22),
		  ZERO,
	  START_R(RegExpParser::defrule::capture_1),
		  TEXTSIM("?"),
		  START_A_R(RegExpParser::rule_type::none),
			  TEXTSIM("<"),
			  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
			  TEXTSIM(">"),
			  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::alternation),
			  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "\'"),
			  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
			  TEXTSIM("\'"),
			  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::alternation),
			  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "P<"),
			  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
			  TEXTSIM(">"),
			  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::alternation),
			  USE_RULE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::zero_one, RegExpParser::defrule::option_setting_flag_cap),
			  TEXTSIM(":"),
			  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::alternation),
			  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "|"),
			  USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::alternation),
			  ZERO,
		  ZERO,
	  START_R(RegExpParser::defrule::capture),
		TEXTSIM("("),
		  START_A_R(RegExpParser::rule_type::none),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::capture_1),
			USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::alternation),
			//TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "?"),
			ZERO,
		TEXTSIM(")"),
		ZERO,
	  START_R(RegExpParser::defrule::atomic_group),
		TEXTSIM("(?>"),
		USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::alternation),
		TEXTSIM(")"),
		ZERO,
	  START_R(RegExpParser::defrule::lookaround),
		TEXTSIM("(?"),
		START_A_R(RegExpParser::rule_type::none),
			  TEXTSIM("="),
			  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "!"),
			  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "<="),
			  TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "<!"),
			  ZERO,
		USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::alternation),
		TEXTSIM(")"),
		ZERO,
		START_R_STYLE(RegExpParser::defrule::backreference,RegExpParser::rule_type::styled_ALL),
			START_A_R(RegExpParser::rule_type::none),
				TEXTSIM("\\g"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::digits),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "\\g{"),
				TEXT_TYPE(RegExpParser::rule_type::zero_one | RegExpParser::rule_type::text, "-"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::digits),
				TEXTSIM("}"),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "\\g{"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
				TEXTSIM("}"),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "\\k<"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
				TEXTSIM(">"),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "\\k\'"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
				TEXTSIM("\'"),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "\\k{"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
				TEXTSIM("}"),
				ZERO,
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "(?P="),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
			TEXTSIM(")"),
			ZERO,
		START_R(RegExpParser::defrule::hex4),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::hex),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::hex),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::hex),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::hex),
			ZERO,
		START_R_STYLE(RegExpParser::defrule::character,RegExpParser::rule_type::styled_ALL),
			TEXTSIM("\\"),
			START_A_R(RegExpParser::rule_type::none),
				TEXT_TYPE(RegExpParser::rule_type::char_any, "acefnrt"),
				USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::digit),
				START_A_R(RegExpParser::rule_type::zero_one),
					 USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::digit),
					 USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::digit),
					 ZERO,
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "o{"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::digit),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::digit),
				USE_RULE(RegExpParser::rule_type::one_more, RegExpParser::defrule::digit),
				TEXTSIM("}"),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "x"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::hex),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::hex),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "x{"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::hex),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::hex),
				USE_RULE(RegExpParser::rule_type::one_more, RegExpParser::defrule::hex),
				TEXTSIM("}"),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "u"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::hex4),
				USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::hex4),
				ZERO,
			ZERO,
		START_R_STYLE(RegExpParser::defrule::character_type,RegExpParser::rule_type::styled_ALL),
			TEXTSIM("."),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "\\"),
			START_A_R(RegExpParser::rule_type::none),
				TEXT_TYPE(RegExpParser::rule_type::char_any, "CdDhHNRsSvVwWX"),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "p{"),
				TEXT_TYPE(RegExpParser::rule_type::zero_one | RegExpParser::rule_type::text, "^"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
				TEXT_TYPE(RegExpParser::rule_type::zero_one | RegExpParser::rule_type::text, "&"),
				TEXTSIM("}"),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "P{"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
				TEXT_TYPE(RegExpParser::rule_type::zero_one | RegExpParser::rule_type::text, "&"),
				TEXTSIM("}"),
				TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "p"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::letter),
				USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::letter),
				ZERO,
			ZERO,
		START_R(RegExpParser::defrule::posix_character_class_name),
			TEXTSIM("alnum"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "alpha"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "ascii"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "blank"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "ctrl"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "digit"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "graph"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "lower"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "print"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "punct"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "space"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "upper"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "word"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "xdigit"),
			ZERO,
		START_R_STYLE(RegExpParser::defrule::posix_character_class,RegExpParser::rule_type::styled_ALL),
			TEXTSIM("[:"),
			TEXT_TYPE(RegExpParser::rule_type::zero_one | RegExpParser::rule_type::text, "^"),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::posix_character_class_name),
			TEXTSIM(":]"),
			//TEXT_TYPE(RegExpParser::rule_type::inversion | RegExpParser::rule_type::char_any | RegExpParser::rule_type::zero_one, "^"),
			ZERO,
		START_R(RegExpParser::defrule::character_class_range_atom),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::character),
			TEXT_TYPE(RegExpParser::rule_type::inversion | RegExpParser::rule_type::char_any , "]"),
			ZERO,
		START_R(RegExpParser::defrule::character_class_range),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::character_class_range_atom),
			TEXTSIM("-"),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::character_class_range_atom),
			ZERO,
		START_R(RegExpParser::defrule::character_class_atom),
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::quoting),
			USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::character_class_range_atom),
			USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::posix_character_class),
			USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::character),
			USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::character_type),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::inversion | RegExpParser::rule_type::char_any, "\\]"),
			ZERO,
		START_R(RegExpParser::defrule::character_class),
			TEXTSIM("["),
			TEXT_TYPE(RegExpParser::rule_type::zero_one | RegExpParser::rule_type::text, "^"),
			TEXTSIM("]"),
			USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::character_class_atom),
			TEXTSIM("]"),
			TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "["),
			TEXT_TYPE(RegExpParser::rule_type::zero_one | RegExpParser::rule_type::text, "^"),
			USE_RULE(RegExpParser::rule_type::one_more, RegExpParser::defrule::character_class_atom),
			TEXTSIM("]"),
			ZERO,
		START_R(RegExpParser::defrule::other),
			TEXT_TYPE(RegExpParser::rule_type::char_any | RegExpParser::rule_type::inversion, "\\^$.[|()?+{"),
			//TEXT_TYPE(RegExpParser::rule_type::none |RegExpParser::rule_type::special_dot, ""),
			//TEXT_TYPE(RegExpParser::rule_type::alternative |RegExpParser::rule_type::text, "\\"),
			ZERO,
		START_R(RegExpParser::defrule::call_out),
			TEXTSIM("(?C"),
			USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::digits),
			TEXTSIM(")"),
			ZERO,
		START_R(RegExpParser::defrule::conditional_pattern),
			TEXTSIM("(?"),
			START_A_R(RegExpParser::rule_type::none),
				TEXTSIM("("),
					START_A_R(RegExpParser::rule_type::none),
						TEXT_TYPE(RegExpParser::rule_type::char_any | RegExpParser::rule_type::zero_one, "+-"),
						USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::digits),
						TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "<"),
						USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
						TEXT_TYPE(RegExpParser::rule_type::text, ">"),
						TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "\'"),
						USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
						TEXT_TYPE(RegExpParser::rule_type::text, "\'"),
						TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "R&"),
						USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::name),
						TEXT_TYPE(RegExpParser::rule_type::alternative | RegExpParser::rule_type::text, "R"),
						USE_RULE(RegExpParser::rule_type::zero_one, RegExpParser::defrule::digits),
						USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::name),
						ZERO,
				TEXTSIM(")"),
				USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::call_out),
				USE_RULE(RegExpParser::rule_type::alternative, RegExpParser::defrule::lookaround),
				ZERO,
			USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::expr),
			START_A_R(RegExpParser::rule_type::zero_one),
				TEXTSIM("|"),
				USE_RULE(RegExpParser::rule_type::none, RegExpParser::defrule::expr),
				ZERO,
			TEXTSIM(")"),
			ZERO,
ZERO
};
#endif
