#include "pgAdmin3.h"
#include "utils/align/AlignWrap.h"
#include <vector>


wxString AlignWrap::build(wxString& strsrc, int config,wxString linesep)
{
	str = strsrc;
	cfg = config;
	lnsep = linesep;
	bool br = false;
	int p = 0;
	list.clear();
	std::vector<size_t> nline; // позиции в list начала строк
	int len = strsrc.Length();
	nline.push_back(0);
	int nrow = -1;
	// parse items
	while (p < len) {
		Item i = parseItem(p, br);
		if (i.type == Item::type::SPACE) break;
		if (i.type == Item::type::COMMENT) {
			if (list.size() == 0) return strsrc;
			Item k = list[list.size() - 1];
			wxString cc = i.getValue();
			cc = cc.Trim(true);

			if (k.br) {
				cc = lnsep + cc;
				//if (CHKCFGPARAM(cfg, ALL_LINES))
				br = false; // чтобы повторно не добавлять новую строку в nline
			}
			k.setComment(k.getComment()+cc);
			k.br = i.br;
			list.erase(list.end()-1);
			i = k;
			
		};
		list.push_back(i);
#ifdef _DEBUG
		//std::cerr << i.println();
#endif
		if (br) {
			nline.push_back(list.size()); // следующий элемент будет с новой строки
			nrow++;
		}
		br = false;
	}
	if (list.size() == 0) return strsrc;
	nline.push_back(list.size() - 1);
	for (size_t l = 1; l < nline.size() - 1; l++)
	{
		int index_u = nline[l - 1L];
		int index_c = nline[l];
		int CountItemU = index_c - index_u;
		int maxIdxU = index_c;
		int maxIdxC = nline[l + 1L];
		
		size_t ll = l;
		if (CHKCFGPARAM(cfg, FIND_UP_LONG_LINE)) {
			while ((ll > 0) && ((((int)nline[ll] - (int)nline[ll - 1L]) - (maxIdxC - maxIdxU)) < 0)) {
				// верхняя строка короче чем текущяя поднимемся вверх для поиска более длинной
				ll--;
			}
			if (ll == 0) ll = l;
		}
		index_u = nline[ll - 1L];
		maxIdxU= nline[ll];
		int size_u = 0;
		int size_c = 0;
		//  : 3456,(wwww)
		//  , : 7,(a)
		while (index_u < maxIdxU && index_c < maxIdxC) {
			int idxU = find(index_u, maxIdxU, list[index_c]);
			// проверим обратную ситуацию верхний найдём в текущей строке
			int idxC = find(index_c, maxIdxC, list[index_u]);
			if (idxU < 0)  idxU = index_u;
			if (idxC < 0)  idxC = index_c;
			int dc = (idxC - index_c);
			int du = (idxU - index_u);
			size_u = range_size(index_u, idxU);
			size_c = range_size(index_c, idxC);
			if (du==0 && dc==0 ) {
				// элементы совпадают и их можно выровнять
				// определим в какой строке это будем делать
				list[index_c].setParent(index_u);
			}
			else {
				// 4021196,'fffff',(
				// (4155,'aaaa'
				if (size_u > size_c) { // фиксируем верхний 
					// будем нижний подгонять под верхний
					size_u = range_size(index_u);
					idxU = index_u;
				} else
					if (size_u < size_c) {
						size_c = range_size(index_c);
						idxC = index_c;
					}
				list[idxC].setParent(idxU);
			}
			if (size_u < size_c) {
				// вырхний короче, его дополняем
				list[index_u].add_space(Item::align::LEFT, size_c - size_u);
				Resize(index_u, list[index_u].getMaxSize());
				idxU = idxU + 1;
				idxC++;
				
			}
			if (size_u > size_c) {
				// 
				list[index_c].add_space(Item::align::LEFT, size_u - size_c);
				Resize(index_c, list[index_c].getMaxSize());
				idxC = idxC + 1;
				idxU++;
			}
			if (size_u == size_c) {
				idxC++;
				idxU++;
			}
			index_c = idxC;
			index_u = idxU;
		}
		// 
		if (CHKCFGPARAM(cfg, FIRST_LINE)) {
			if (index_u >= maxIdxU) {
				list[(size_t)index_c - 1].br = true;
				size_t id = (nline.size() - nrow - 1);
				
				if (nline[id] != index_c)
					nline.insert(nline.end() - nrow - 1, index_c);
				else nrow--;
			}
			else
			{
				// из за коментарией -- мы внесли перевод в неожидааных местах и их нужно учесть когда до них дойдём
				if (nrow == 0) break;
				nrow--;
				cfg = cfg | FIND_UP_LONG_LINE;
			}
		}
	}
	wxString a;
	for (int j = 0; j < nline.size() - 1; j++) {
		int d = 1;
		if (j == nline.size() - 2) d = 0;
		a+=range_print(nline[j], nline[j + 1] - d);
	}
	//range_print(nline[0], nline[1L]-1);
	//range_print(nline[l], nline[l + 1L]-1);
	return a;
}

void AlignWrap::Resize(int idx,int newSize) {
	int pr = idx;
	while (pr>=0) {
	Item &i=list[pr];
	int oldsz = i.getMaxSize();
	pr = i.getParent();
	i.setMaxSize(newSize);
	//if (pr>=0) Resize(pr,newSize);
	}
}


int AlignWrap::range_size(int s) {
	int se = 0;
	se = list[s].getMaxSize();
	return se;
}
int AlignWrap::range_size(int s, int e) {
	int se = 0;
	for (int j = s; j <= e; j++) {
		se += list[j].getMaxSize();
		}
	return se;
}
wxString AlignWrap::range_print(int s, int e) {
	wxString ss = "";
	for (int j = s; j <= e; j++) {
		ss = ss+list[j].print();
		
	}
	ss += lnsep;
	//std::cout << ss;
	return ss;
}

int AlignWrap::find(int s,int e, Item &k) {

	std::vector<Item>::iterator it;
	if (e >= list.size()) e = list.size() - 1;
	for (int j = s; j <= e; j++) {
		if (list[j] == k) {
			return j;
		}
	}
		return -1;
}
Item AlignWrap::parseItem(int& pos, bool& breakline) {

	wxChar c;
	wxChar c2;
	int p = pos;
	int len = str.Length();
	int l = 0;
	int sp = 0;
	int spl = 0;
	int spbe = 0;
	int type;
	int comment = 0;
	while (true) {
		spl = chkspace(p, breakline);
		if (breakline) {
			// empty line ignore
			breakline = false;
			spl = 0;
			continue;
		}
		pos = p;
		break;
	}
	if (p >= len) {
		Item i("", 0);
		return i;
	}
	c = str[p];
	if (c == '\'' || c == '"') {
		p++;
		
		while (p < len)
		{
			c2 = str[p];
			if (c2 == c) {
				if (p + 1 < len)
					if (str[p+1] == c) { p = p + 2; continue; }
				p++;
				break;
			}
			p++;
		}
		l = p - pos;
		type = Item::type::LITERAL;
		goto theend;
		
	}
	if ((p + 1) < len) c2 = str[p + 1];

	if (wxIsalpha(c)) {
		// WORD
		p++;
		while (p < len)
		{
			c = str[p];
			if (!(wxIsalnum(c) || c == '_' || c == '$'|| c==':'|| c=='.')) break;
			p++;
		}
		l = p - pos;
		type = Item::type::WORD;
		goto theend;

	}
	if ((c == '-' || wxIsdigit(c)) &&(c2!='-')) {
		p++;
		while (p < len)
		{
			c = str[p];
			if (!(wxIsdigit(c) || c == '.' || c == 'e' || c == 'E')) break;
			p++;
		}
		l = p - pos;
		type = Item::type::NUM;
		goto theend;

	}
	type = Item::type::ANCHOR;
	if (p + 1 < len) {
		p++;
		c2 = str[p];
		p++;
		l = p - pos;
		// 2-х символьные операторы
		if (c == '<' && c2 == '=') goto theend;
		if (c == '>' && c2 == '=') goto theend;
		if (c == '<' && c2 == '>') goto theend;
		if (c == '-' && c2 == '-') {
			int cntNL = 0;
			while (p < len)
			{
				if (str[p] == '\n' || str[p] == '\r') {
					cntNL++;
				}
				else {
					if (cntNL > 0) break;
				}
				p++;

			}
			l = p - pos;
			comment = 1;
			type = Item::type::COMMENT;
			goto theend;
		}
		if (c == '/' && c2 == '*') {
			while (p < (len - 1))
			{
				if (str[p] == '*' || str[p + 1] == '/')
					break;
				p++;
			}
			p=p+2;
			l = p - pos;
			type = Item::type::COMMENT;
			goto theend;
		}
		p = p - 2;
	}
	p++;
	l = p - pos;
	
	goto theend;


theend:
		wxString v = str.substr(pos, l);
		sp = chkspace(p, breakline);
		if (comment == 1) breakline = true;
		Item i(v,type);
		i.rs = sp;
		//i.ls = spl;
		i.br = breakline;
		pos = p;
		return i;
}
int AlignWrap::chkspace(int &pos, bool& br) {
	wxChar c,prev='\0';
	int nl = parserows;
	int l = 0;
	bool endline = false;
	while (pos < str.Length()) {
		c = str[pos++];
		if (c == '\n' || c == '\r') {
			if (c=='\r' && str[pos] == '\n') {
				// windows
				endline = true;
				pos++;
			}
			parserows++;
			if (CHKCFGPARAM(cfg,ALL_LINES)) {
				br = true;
				break;
			}
			if ((CHKCFGPARAM(cfg,FIRST_LINE)) && parserows == 1)
			{
				// first row
				br = true;
				break;
			}
			continue;
		}

		if (c == ' ' || c == '\t') { l++; continue; }
		pos--;
		break;
	}
	if (nl < parserows) l++;
	return l;
}