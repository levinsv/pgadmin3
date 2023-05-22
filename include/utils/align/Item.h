#pragma once
#include <wx/wx.h>
class Item {
public:
	enum align
	{
		LEFT = 0,
		RIGHT
	};
	enum type
	{
		SPACE = 0,
		NUM,
		WORD,
		LITERAL,
		ANCHOR,
		COMMENT
	};

	Item(const wxString &str, int type_item, int align_type=LEFT) {
		it = str;
		type = type_item;
		align = align_type;
	}
	bool operator==(const Item& rh);
	bool operator!=(const Item& rh);
	wxString print();
	wxString println();
	const wxString getValue() { return it; }
	const wxString getComment() { return comment; }
	const int	   getMaxSize();
	const int	   getParent() { return up_item; }
	void	       setMaxSize(int newSize);
	const void     add_space(int align, int count_space) { if (align == Item::align::LEFT) rs += count_space; else ls+= count_space;}
	const void     setParent(int idxPar) { up_item = idxPar; };
	const void     setComment(wxString c) { comment = c; };
	int rs = 0;
	int ls = 0;
	int type = 0;
	int up_item = -1;
	bool br = false;
private:
	int align = LEFT;
	int maxlen = 0;
	wxString it;
	wxString comment;

};
