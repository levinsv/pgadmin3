#pragma once
#include <deque>
#include <wx/wx.h>
#include "wx/hashmap.h"
#include "utils/csvfiles.h"
#define MYTEST 1

#define FL_REVERSE  1
#define FL_CONTAINS 2

WX_DECLARE_HASH_MAP(int, int, wxIntegerHash, wxIntegerEqual, MyHashToRow);

namespace MyConst {
	enum colField {
		logtime,
		loguser,
		logdb,
		logpid,
		loghost,
		logtag,
		logSessiontime,
		logSeverity,
		logSqlstate,
		logMessage,
		logDetail,
		logHint,
		logappname,
		logbtype,
		logSERVER,
		Col_Max
	};
	enum iconIndex {
		log,
		war,
		user,
		error,
		fatal,
		panic,
		MAX_COL
	};
	enum ltype {
		SIMPLE_TEXT,
		InGroup,
		Group
	};
}
struct ps {
	unsigned short int s;
	unsigned short int l;
};
struct Line {
	unsigned short type : 3;
	unsigned short icon : 3;
	unsigned short visible : 1;
	int prevRowGroup = -1;
	int hash;
	ps logtime = { 0,0 };
	ps loguser = { 0,0 };
	ps logdb = { 0,0 };
	ps logpid = { 0,0 };
	ps loghost = { 0,0 };
	ps logtag = { 0,0 };
	ps logSessiontime = { 0,0 };
	ps logSeverity = { 0,0 };
	ps logSqlstate = { 0,0 };
	ps logMessage = { 0,0 };
	ps logDetail = { 0,0 };
	ps logHint = { 0,0 };
	ps logappname = { 0,0 };
	ps logbtype = { 0,0 };
	ps logSERVER = { 0,0 };
	wxString text;
};
struct LineFilter {
	int col;
	int flags;
	wxString val;
	wxString NameFilter;
};

class Storage
{
public:
	bool AddLineTextCSV(const wxString& strcsv);
	wxString GetField(int row, MyConst::colField col);
	wxString GetFieldStorage(int row, MyConst::colField col, bool filter);
	Storage();
	int GetSeverityIndex(int row);
	void SetHost(const wxString& host) { currhost = host; };
	wxString GetHost() { return currhost; };
	void SetErrMsgFlag(bool flag) { err_msg = flag; };
	bool GetErrMsgFlag() { return err_msg; };
	void GetRowsStat(int& Rowsadd, int& Rowsignore) {
		Rowsadd=rowsadd;
		Rowsignore=rowsignore;
	};
	void ClearRowsStat() {
		rowsadd = 0;
		rowsignore = 0;
	};
	wxColor& GetBgColorLine(int row);
	// установка фильтра на колонку
	int SetFilter(int colfld, wxString& val, int flags);
	// приминить фильтр для строки или для все строк хранилища
	// true если строка не отфильтровалась (видна)
	bool ApplyFilter(int row = -1);
	int SetFilterArray(std::deque<LineFilter> arr);
	wxString getStrGroup(wxString source);
	int testFilter(MyConst::colField col, int position);
	wxString GetStringFilterExpr(int positionArrayFilter,bool addNumCol=false);
	void addLineFilterStr(wxString strflt,wxString fn);
	wxString _strwhere(int flags);
	void saveFilters();
	wxString LineFilterToStr(LineFilter& lf);
		//
	int getHashString(wxString str) {
		std::hash<wxString> string_hash;
		return string_hash(str);
	}
	void DropColFilter(int index);
	void setDetailGroupRow(int rowGroup);
	bool IsGroupFilter() {
		return groupFilterUse;
	};
	bool IsFilter() {
		return fCol.size() > 0 || IsGroupFilter();
	};
	void setGroupFilter(bool val) {
		detailGroup = -1;
		groupFilterUse = val;
		MyHashToRow::iterator it;
		for (it = hashKeyToCount.begin(); it != hashKeyToCount.end(); ++it)
		{
			int keyhash = it->first, lastrow = it->second;
			// do something useful with key and value
			it->second = 0;
		}
		m_cacheIndex = -1;
		frows.clear();
	}
	void ClearCount(int rowfilter);
	bool IsAddGroupNew() {
		return faddgroup;
	};
	std::deque<LineFilter> getFilter(wxString FilterName);
	wxString getFilterString(std::deque<LineFilter> arr);
	int getFilterNames(wxArrayString& arr);
	void removeFilter(wxString FilterName);

	int getLastRowIndex() { return m_cacheIndex; }
	// всего строк в хранилище
	int getCountStore();
	int getCountFilter();
	int getCountGroup(int row);
	int GetTotalCountGroup(int rowfilter);
	wxArrayString GetAllFields(int row, bool isfilter);
private:
	bool checkFilter(Line& l);
	Line getLineParse(const wxString& str, bool csv = false);
	wxString get_field(Line& l, MyConst::colField col);
	LineFilter getLineFilter(wxString strflt,wxString fn);
	void getLineToCache(int row, bool filter = true);
	bool CompareFilterLine(int row, bool filter);

	std::deque<Line> storage;
	std::deque<LineFilter> filterload;
	// hash Group to row
	MyHashToRow hashKeyToRow;
	MyHashToRow hashKeyToCount;
	MyHashToRow hashKeyTotal;
	//filter setting
	std::deque<int> frows;
	wxArrayInt fCol;
	wxArrayInt fFlags;
	wxArrayString fVal;
	// признак ошибок.
	bool err_msg;
	// режим группировки 
	bool groupFilterUse = false;
	bool faddgroup = false;
	int prevRow = -1;
	int detailGroup = -1;
	//
	int m_cacheIndex = -1;
	Line m_cacheLine;
	//
	int rowsadd;
	int rowsignore;
	wxColor bgErr[MyConst::iconIndex::MAX_COL];
	wxString colname[MyConst::colField::Col_Max];
	wxString currhost;
};

