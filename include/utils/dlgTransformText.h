#pragma once
#include "pgAdmin3.h"
#include <wx/clipbrd.h>
#include "ctl/ctlStyledText.h"
enum class eTypeGroup {
	SIMPLE_TEXT      = -1,
	USER_GROUP_START = -2,
	USER_GROUP_END   = -3
};
struct interval {
	size_t start = -1;
	size_t len = 0;
	int style = 0;
};

class dlgTransformText :
    public pgDialog
{
public:
	dlgTransformText(ctlSQLBox* form,const wxString source);
	~dlgTransformText();
	
	void SetSource(const wxString& src);
private:
	void OnCheckUI(wxCommandEvent& ev);
	void OnOk(wxCommandEvent& ev);
	void OnClose(wxCloseEvent& ev);
	void OnCancel(wxCommandEvent& ev);
	void OnComboSelect(wxCommandEvent& ev);
	void OnChange(wxCommandEvent& ev);
	void OnChangeLimit(wxCommandEvent& ev);
	void OnChangeOnline(wxCommandEvent& ev);
	void OnSave(wxCommandEvent& ev);
	void OnLoad(wxCommandEvent& ev);
	
	void OnChangeRegEx2(wxStyledTextEvent& ev);
	void TransformText(const wxRegEx &regfld);
	void OnIdle(wxIdleEvent &ev);
	struct  replace_opt {
		bool isGroup = false;
		int nGroup = -1;
		wxString text;
		int flags = 0;
		int start = -1;
		int len = 0;
	};
	wxJSONValue LoadConfig(const wxString confname);
	void CheckLimits();
	wxJSONValue FillConfig();
	void LoadOptions();
	void SetDecoration(ctlStyledText* s);
	void SetStyled(ctlStyledText* s);
	void showNumber(ctlStyledText* text, bool visible);
	void AppendTextControl(ctlStyledText* ctrl, const wxString appendtext, bool isnewline = false);
	/// <summary>
	/// Обработка строки замены и подготовка специальной струкутуры.
	/// Допустимые спец. комбинации \t \r\ \n
	///   \g{n} Ссылка на группы wxRegExp n=0..N
	///   \G{n} Не выводить содержимое группы. Имеет смысл использовать для замены текста группы на свой
	///    [ ] Пользовальская группа обладающая следующим свойством:
	///    Если в этой группе присутствуют \g{n} и они все пустые то вся группа [ ] считается пустой
	///    Пользовательские группы могут быть вложенными.
	/// </summary>
	/// <param name="repstr"></param>
	/// <returns></returns>
	std::vector<replace_opt> BuildString(const wxString repstr);
	wxString ReplaceFormatting(
		const wxString& src,
		const wxRegEx& r,
		const std::vector<dlgTransformText::replace_opt> st,
		int &position,
		interval m2[],
		int maxsizeintervalarray,
		int &currindex,
		size_t &start_frame);

	wxString src;
	
	wxString srcRowSep;
	wxJSONValue opt,lastconf;
	int countGroupColor = 0;
	int limitLine;
	int limitChar;
	wxString strResult;
	// UI
	int toplineTrg=0;
	int toplineSrc=0;
	bool inizialize;
	bool isChange = false;
	bool isNeedTransform = false;
	bool isOnline = false;
	wxString strReg,strRep;
	wxColour bgerror;
	DECLARE_EVENT_TABLE()

};

