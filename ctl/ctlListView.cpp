//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// ctlListView.cpp - enhanced listview control
//
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include <wx/imaglist.h>

// App headers
#include "ctl/ctlListView.h"
#include "utils/misc.h"

int wxCALLBACK
MyCompareFunction(wxIntPtr item1, wxIntPtr item2, wxIntPtr WXUNUSED(sortData))
{
	// inverse the order
	if (item1 < item2)
		return 1;
	if (item1 > item2)
		return -1;

	return 0;
}

ctlListView::ctlListView(wxWindow* p, int id, wxPoint pos, wxSize siz, long attr)
	: wxListView(p, id, pos, siz, attr | wxLC_REPORT)
{
	nosort = false;
	order = 1;
	prev_col = -1;
	storelongstring = false;
	Connect(wxID_ANY, wxEVT_LIST_COL_CLICK, wxListEventHandler(ctlListView::OnSortGrid));
}
#include <map>
bool ctlListView::IsNumberColumn(const wxString& columnlabel) {
	bool asnum = false;
	if (columnlabel == _("CFS fragmentation") ||
		columnlabel == (_("Tuples inserted")) ||
		columnlabel == (_("Tuples updated")) ||
		columnlabel == (_("Tuples deleted")) ||
		columnlabel == (_("Tuples HOT updated")) ||
		columnlabel == (_("Live tuples")) ||
		columnlabel == (_("Dead tuples")) ||
		columnlabel == (_("CFS %")) ||
		columnlabel == (_("Autovacuum counter")) ||
		columnlabel == (_("Autoanalyze counter")) ||
		columnlabel == (_("Index Scans")) ||
		columnlabel == (_("Index Tuples Read")) ||
		columnlabel == (_("Index Tuples Fetched"))
		) {
		asnum = true;
		}
	return asnum;

}
void ctlListView::SortGrid(int colsort, bool isevent) {
	if (!nosort) {
		int col = colsort;
		if (col == prev_col && isevent) order = order * -1;
		int rows = GetItemCount();
		wxListItem listitem;
		listitem.SetMask(wxLIST_MASK_TEXT);
		GetColumn(col, listitem);
		wxString label = listitem.GetText();
		bool issize = label == _("Size");
		bool astext = true;
		if (IsNumberColumn(label)|| issize) astext = false;
		//sort numeric column
		if (astext) {
			std::multimap<wxString, int> mp;
			for (int i = 0; i < rows; i++) {
				wxString val = GetItemText(i, col);
				mp.insert(std::pair<wxString, int>(val, i));
			}
			// сопоставим сортированным значениям последовательность чисел для сортировки SortItems
			std::multimap<wxString, int>::iterator it = mp.begin();
			for (int i = 1; it != mp.end(); it++, i++) {  // выводим их
				int row = it->second; // row
				wxListView::SetItemData(row, (long)i * order);
			}

		}
		else
		{
			std::multimap<double, int> mp;
			double d;

			for (int i = 0; i < rows; i++) {
				wxString val = GetItemText(i, col);
				d = 0;
				if (val == "NaN") val = "0";
				if (val.ToCDouble(&d))
				{
					// это число
				}
				else
				{

					if (issize) // convert => MB
						d=ConvertSizeToMB(val);
				}
				mp.insert(std::pair<double, int>(d, i));
			}
			// сопоставим сортированным значениям последовательность чисел для сортировки SortItems
			std::multimap<double, int>::iterator it = mp.begin();
			for (int i = 1; it != mp.end(); it++, i++) {  // выводим их
				int row = it->second; // row
				wxListView::SetItemData(row, (long)i * order);
			}

		}

		SortItems(MyCompareFunction, 0);
		Refresh();
		prev_col = col;
	}
}
bool ctlListView::ReSort() {
	int ncols = GetColumnCount();
	if (prev_col >= 0 && prev_col < ncols)
			SortGrid(prev_col, false);
		else
			return false;
	return true;

}

void ctlListView::OnSortGrid(wxListEvent& event)
{
	int col = event.GetColumn();
	SortGrid(col, true);
}
long ctlListView::GetSelection()
{
	return GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

wxString ctlListView::GetTextLong(long row, long col)
{
	wxListItem item;
	item.SetId(row);
	item.SetColumn(col);
	item.SetMask(wxLIST_MASK_TEXT);
	GetItem(item);
	wxString v = item.GetText();
	if (storelongstring) {
		int len = v.Length();
		if (len == 200 && row >= 0 && col == 0 && row < longstring.size()) {
			return  longstring[row];
		}
	}
	return v;
};

wxString ctlListView::GetText(long row, long col)
{
	wxListItem item;
	item.SetId(row);
	item.SetColumn(col);
	item.SetMask(wxLIST_MASK_TEXT);
	GetItem(item);
	wxString v = item.GetText();
	return v;
};


void ctlListView::AddColumn(const wxString& text, int size, int format)
{
	if (size == wxLIST_AUTOSIZE || size == wxLIST_AUTOSIZE_USEHEADER)
	{
		InsertColumn(GetColumnCount(), text, format, size);
	}
	else
	{
		InsertColumn(GetColumnCount(), text, format, ConvertDialogToPixels(wxPoint(size, 0)).x);
	}
}


long ctlListView::AppendItem(int icon, const wxString& val, const wxString& val2, const wxString& val3, const wxString& val4)
{
	long idx = GetItemCount();
	long pos;
	pos = InsertItem(idx, val, icon);
	if (!val2.IsEmpty())
		SetItem(pos, 1, val2);
	if (!val3.IsEmpty())
		SetItem(pos, 2, val3);
	if (!val4.IsEmpty())
		SetItem(pos, 3, val4);

	return pos;
}
long ctlListView::AppendItemLong(int icon, const wxString& val, const wxString& val2, const wxString& val3, const wxString& val4)
{
	long idx = GetItemCount();
	long pos;
	if (storelongstring) {
		if (val.Length() > 200) {
			longstring.push_back(val);
			pos = InsertItem(idx, val.Mid(0, 200), icon);
		}
		else {
			longstring.push_back(wxEmptyString);
			pos = InsertItem(idx, val, icon);
		}
	}
	else
		pos = InsertItem(idx, val, icon);
	if (!val2.IsEmpty())
		SetItem(pos, 1, val2);
	if (!val3.IsEmpty())
		SetItem(pos, 2, val3);
	if (!val4.IsEmpty())
		SetItem(pos, 3, val4);

	return pos;
}


void ctlListView::CreateColumns(wxImageList* images, const wxString& left, const wxString& right, int leftSize)
{
	int rightSize;
	if (leftSize < 0)
	{
#ifdef __WXMAC__
		leftSize = rightSize = (GetParent()->GetSize().GetWidth() - 20) / 2;
#else
		leftSize = rightSize = GetSize().GetWidth() / 2;
#endif
	}
	else
	{
		if (leftSize)
			leftSize = ConvertDialogToPixels(wxPoint(leftSize, 0)).x;

#ifdef __WXMAC__
		rightSize = (GetParent()->GetSize().GetWidth() - 20) - leftSize;
#else
		rightSize = GetClientSize().GetWidth() - leftSize;
#endif
	}
	if (!leftSize)
	{
		InsertColumn(0, left, wxLIST_FORMAT_LEFT, GetClientSize().GetWidth());
	}
	else
	{
		InsertColumn(0, left, wxLIST_FORMAT_LEFT, leftSize);
		InsertColumn(1, right, wxLIST_FORMAT_LEFT, rightSize);
	}

	if (images)
		SetImageList(images, wxIMAGE_LIST_SMALL);
}


void ctlListView::CreateColumns(wxImageList* images, const wxString& str1, const wxString& str2, const wxString& str3, int leftSize)
{
	int rightSize;
	if (leftSize < 0)
	{
#ifdef __WXMAC__
		leftSize = rightSize = (GetParent()->GetSize().GetWidth() - 20) / 2;
#else
		leftSize = rightSize = GetSize().GetWidth() / 2;
#endif
	}
	else
	{
		if (leftSize)
			leftSize = ConvertDialogToPixels(wxPoint(leftSize, 0)).x;

#ifdef __WXMAC__
		rightSize = (GetParent()->GetSize().GetWidth() - 20) - leftSize;
#else
		rightSize = GetClientSize().GetWidth() - leftSize;
#endif
	}
	if (!leftSize)
	{
		InsertColumn(0, str1, wxLIST_FORMAT_LEFT, GetClientSize().GetWidth());
	}
	else
	{
		InsertColumn(0, str1, wxLIST_FORMAT_LEFT, leftSize);
		InsertColumn(1, str2, wxLIST_FORMAT_LEFT, rightSize / 2);
		InsertColumn(2, str3, wxLIST_FORMAT_LEFT, rightSize / 2);
	}

	if (images)
		SetImageList(images, wxIMAGE_LIST_SMALL);
}
