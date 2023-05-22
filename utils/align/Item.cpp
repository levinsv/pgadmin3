#include "pgAdmin3.h"
#include "utils/align/Item.h"
bool Item::operator==(const Item& rh)
{
	if (type == rh.type && type == ANCHOR) {
		if (it == rh.it) return true;
		return false;
	}
	// null ( NUM or LITERAL)
	if (it.IsSameAs("null", false) && true) {
		if (rh.type == LITERAL || rh.type == NUM || rh.type == WORD)
			return true;
		return false;
	}
	//NUM or LITERAL
	if (type == NUM || type == LITERAL ) {
		if (rh.it.IsSameAs("null", false) && true)
			return true; 
		if (rh.type == LITERAL || rh.type == NUM ) return true;
		return false;
	}
	// 
	if (type == rh.type) return true;

	return false;
}
bool Item::operator!=(const Item& rh) {
	return !this->operator==(rh);
}
wxString Item::println() {
	wxString s = wxString::Format("Type %d val:\"%s\"", type, it);
	if (ls > 0) s += wxString::Format(" L:\"%d\"", ls);
	if (rs > 0) s += wxString::Format(" R:\"%d\"", rs);
	if (br) s += wxString::Format(" BR");
	s += wxString::Format("\n");
	return s;
}
wxString Item::print() {

	wxString s = wxString::Format("%s", it);
	wxString r(' ', rs), l(' ', ls);

	s = l + s + r + wxString::Format("%s", comment);
	std::cout << s;
	return s;
}
const int Item::getMaxSize() {
	int tmp = rs + ls + it.Length();
	if (!br) tmp += comment.Length();
	return  tmp;
}
void Item::setMaxSize(int newSize) {
	ls = 0;
	int tmp = newSize - it.Length() - comment.Length();
	if (tmp >= 0) {
		rs = tmp;
	}
	else {
		//this->println();
		return;
	}
}

