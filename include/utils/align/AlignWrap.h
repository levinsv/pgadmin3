#pragma once
#include "Item.h"

class AlignWrap {
public:
	enum cfg
	{
		// Выравнивание списка строк можно использовать вместе с FIND_UP_LONG_LINE
		// например insert команды с одинаковым перечнем элементов
		ALL_LINES = 1,
		// выравнивание по длинне первой строки
		// все переводы строк начиная со второй строки игнорируются
		// удобно для выравнивания списков IN
		FIRST_LINE= 2,
		// вспомогательный флаг применяется если встречаются случайные короткие строки
		// и при помощи этого флага ищутся более длинные строки обработанные ранее
		FIND_UP_LONG_LINE=4
	};
	AlignWrap() {}
#define CHKCFGPARAM(val,par) ((val & par)==par)
	
	wxString build(wxString & strsrc, int config,wxString linesep);
	void Resize(int idx, int newSize);
	wxString range_print(int s, int e);
private:
	
	int range_size(int s, int e);
	int range_size(int s);
	Item parseItem(int &pos, bool &breakline);
	int chkspace(int &pos, bool& br);
	int find(int s, int e, Item& k);
	//
	int parserows = 0;

	std::vector <Item> list;

	wxString str, lnsep;
	int cfg = 0;
};
