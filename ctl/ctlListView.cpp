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
	Connect(wxID_ANY, wxEVT_LIST_COL_CLICK, wxListEventHandler(ctlListView::OnSortGrid));
}
#include <map>
void ctlListView::OnSortGrid(wxListEvent& event)
{
	if (!nosort) {
		int col = event.GetColumn();
		if (col == prev_col) order = order * -1;
		int rows = GetItemCount();
		wxListItem listitem;
		listitem.SetMask(wxLIST_MASK_TEXT);
		GetColumn(col, listitem);
		wxString label = listitem.GetText();
		bool issize = label == _("Size");
		bool astext = true;
		if (label == _("CFS fragmentation") ||
			label == (_("Tuples inserted")) ||
			label == (_("Tuples updated")) ||
			label == (_("Tuples deleted")) ||
			label == (_("Tuples HOT updated")) ||
			label == (_("Live tuples")) ||
			label == (_("Dead tuples")) ||
			label == (_("CFS %")) ||
			label == (_("Autovacuum counter")) ||
			label == (_("Autoanalyze counter")) ||
			label == (_("Index Scans")) ||
			label == (_("Index Tuples Read")) ||
			label == (_("Index Tuples Fetched")) ||
			issize
			) {
			astext = false;
		}
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
				if (val == "NaN") val = "0";
				if (val.ToCDouble(&d))
				{
					// это число
				}
				else
				{
					
					if (issize)
						if (val.Right(2) == "kB") d = d / 1024;
						else if (val.Right(2) == "GB") d = d * 1024;
						else if (val.Right(5) == "bytes") d = d / 1024 / 1024;
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
long ctlListView::GetSelection()
{
	return GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}


wxString ctlListView::GetText(long row, long col)
{
	wxListItem item;
	item.SetId(row);
	item.SetColumn(col);
	item.SetMask(wxLIST_MASK_TEXT);
	GetItem(item);
	return item.GetText();
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
	long pos = InsertItem(GetItemCount(), val, icon);
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
