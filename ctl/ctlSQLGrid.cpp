//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// ctlSQLGrid.cpp - SQL Query result window
//
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/generic/gridctrl.h>
#include "db/pgConn.h"
#include "ctl/ctlSQLGrid.h"
#include "utils/sysSettings.h"
#include "frm/frmExport.h"
#include <wx/regex.h>
#include "ctl/ctlSQLResult.h"

#define EXTRAEXTENT_HEIGHT 6
#define EXTRAEXTENT_WIDTH  6

BEGIN_EVENT_TABLE(ctlSQLGrid, wxGrid)
	EVT_MOUSEWHEEL(ctlSQLGrid::OnMouseWheel)
	EVT_GRID_COL_SIZE(ctlSQLGrid::OnGridColSize)
	EVT_GRID_LABEL_LEFT_CLICK(ctlSQLGrid::OnLabelClick)
	EVT_GRID_CELL_RIGHT_CLICK(  ctlSQLGrid::OnCellRightClick)
	EVT_PAINT( ctlSQLGrid::OnPaint )
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(ctlSQLGrid, wxGrid)

ctlSQLGrid::ctlSQLGrid()
{
}

ctlSQLGrid::ctlSQLGrid(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size)
	: wxGrid(parent, id, pos, size, wxWANTS_CHARS | wxVSCROLL | wxHSCROLL)
{
	// Set cells font
	wxFont fntCells(settings->GetSQLFont());
	SetDefaultCellFont(fntCells);
	// Set labels font
	wxFont fntLabel(settings->GetSystemFont());
	fntLabel.SetWeight(wxBOLD);
	SetLabelFont(fntLabel);
	SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
	SetRowLabelSize(50);
	SetDefaultRowSize(fntCells.GetPointSize() * 2 + 2);
	SetColLabelSize(fntLabel.GetPointSize() * 4);
	SetDefaultCellOverflow(false);
	//SetDefaultRenderer(new  wxGridCellAutoWrapStringRenderer);
	SetDefaultRenderer(new  CursorCellRenderer);
	//SetUseNativeColLabels(true);
	//UseNativeColHeader(true);
	SetCellHighlightColour(wxColor(0, 0, 0));
	grp=NULL;
	isSort=false;
	Connect(wxID_ANY, wxEVT_GRID_LABEL_LEFT_DCLICK, wxGridEventHandler(ctlSQLGrid::OnLabelDoubleClick));
}
#include "wx/renderer.h"
#include "wx/headerctrl.h"

void ctlSQLGrid::DrawColLabel( wxDC& dc, int col ) {
	wxGrid::DrawColLabel(dc,col);
	if (!IsSort()) return;
    int colLeft = GetColLeft(col);

    wxRect rect(colLeft, 0, GetColWidth(col), m_colLabelHeight);
    sqlResultTable *t=(sqlResultTable *)GetTable();

		wxHeaderSortIconType sortArrow=t->getSortColumn(col)!=0
                                        ? t->getSortColumn(col)>0
                                            ? wxHDR_SORT_ICON_UP
                                            : wxHDR_SORT_ICON_DOWN
                                        : wxHDR_SORT_ICON_NONE;

    if (sortArrow != wxHDR_SORT_ICON_NONE )
    {
        wxRect ar = rect;

        // make a rect for the arrow
        ar.height = 4;
        ar.width = 8;
        ar.y += (rect.height - ar.height)/2;
        ar.x = ar.x + rect.width - 3*ar.width/2;
	    int arrowSpace = 0;
        arrowSpace = 3*ar.width/2; // space to preserve when drawing the label
        wxPoint triPt[3];
        if ( sortArrow & wxHDR_SORT_ICON_UP )
        {
            triPt[0].x = ar.width / 2;
            triPt[0].y = 0;
            triPt[1].x = ar.width;
            triPt[1].y = ar.height;
            triPt[2].x = 0;
            triPt[2].y = ar.height;
        }
        else
        {
            triPt[0].x = 0;
            triPt[0].y = 0;
            triPt[1].x = ar.width;
            triPt[1].y = 0;
            triPt[2].x = ar.width / 2;
            triPt[2].y = ar.height;
        }

       wxColour c;

		c = wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW);
			 for (int k=0;k<MAX_COL_SORT;k++) {
				 int cl=t->colsortnumber[k];
				 if (cl!=-1) {
					 if (cl==col) {
						 if (k==0) c=wxColor(155,17,48); // red
						 if (k==1) c=wxColor(255,255,0); // yellow
						 if (k==2) c=wxColor(34,199,76); // green
						 if (k==3) c=wxColor(12,38,160);  // blue
					 }
				 } else break;
			 }
        wxDCPenChanger setPen(dc, c);
        wxDCBrushChanger setBrush(dc, c);

        wxDCClipper clip(dc, rect);
        dc.DrawPolygon( 3, triPt, ar.x, ar.y);
    }

        //wxRendererNative::Get().DrawHeaderButton
        //                        (m_colWindow,
        //                            //GetColLabelWindow(),
        //                            dc,
        //                            rect,
        //                            0,
        //                            IsSortingBy(col)
        //                                ? IsSortOrderAscending()
        //                                    ? wxHDR_SORT_ICON_UP
        //                                    : wxHDR_SORT_ICON_DOWN
        //                                : wxHDR_SORT_ICON_NONE
        //                        );


}
void ctlSQLGrid::OnGridColSize(wxGridSizeEvent &event)
{
	// Save key="index:label", value=size
	int col = event.GetRowOrCol();
	colSizes[GetColKeyValue(col)] = GetColSize(col);

}
void ctlSQLGrid::OnCopy(wxCommandEvent &ev)
{
	Copy(false);
}

void ctlSQLGrid::OnMouseWheel(wxMouseEvent &event)
{
	if (event.ControlDown() || event.CmdDown())
	{
		wxFont fontlabel = GetLabelFont();
		wxFont fontcells = GetDefaultCellFont();
		if (event.GetWheelRotation() > 0)
		{
			fontlabel.SetPointSize(fontlabel.GetPointSize() - 1);
			fontcells.SetPointSize(fontcells.GetPointSize() - 1);
		}
		else
		{
			fontlabel.SetPointSize(fontlabel.GetPointSize() + 1);
			fontcells.SetPointSize(fontcells.GetPointSize() + 1);
		}
		SetLabelFont(fontlabel);
		SetDefaultCellFont(fontcells);
		SetColLabelSize(fontlabel.GetPointSize() * 4);
		SetDefaultRowSize(fontcells.GetPointSize() * 2);
		for (int index = 0; index < GetNumberCols(); index++)
			SetColSize(index, -1);
		ForceRefresh();
	}
	else
		event.Skip();
}

wxString ctlSQLGrid::GetExportLine(int row)
{
	return GetExportLine(row, 0, GetNumberCols() - 1);
}


wxString ctlSQLGrid::GetExportLine(int row, int col1, int col2)
{
	wxArrayInt cols;
	wxString str;
	int i;

	if (col2 < col1)
		return str;

	cols.Alloc(col2 - col1 + 1);
	for (i = col1; i <= col2; i++)
	{
		cols.Add(i);
	}

	return GetExportLine(row, cols);
}

wxString ctlSQLGrid::GetExportLine(int row, wxArrayInt cols)
{
	wxString str;
	unsigned int col;
	
	if (GetNumberCols() == 0 || GetRowSize(row)==0)
		return str;
	wxString colsep=settings->GetCopyColSeparator();
	if (generatesql == 2|| generatesql == 1) colsep=wxT(",");
	if (generatesql == 3) colsep = wxT(" and ");
	wxString qtsimbol=settings->GetCopyQuoteChar();
	if (generatesql>0) qtsimbol=wxT("'");
	wxString head=wxT("insert into tbl(");
	if (cols.Count()>1 && generatesql > 1) str.Append("(");
	for (col = 0 ; col < cols.Count() ; col++)
	{
		if (col > 0)
			str.Append(colsep);
		if (col > 0) head.Append(colsep);
		head=head+GetColumnName(cols[col]);
		wxString text = GetCellValue(row, cols[col]);
		wxString cname = GetColumnName(cols[col]);
		bool needQuote  = false;
		if (settings->GetCopyQuoting() == 1)
		{
			needQuote = IsColText(cols[col]);
		}
		else if (settings->GetCopyQuoting() == 2)
			/* Quote everything */
			needQuote = true;
		if (text.Length()==0&&generatesql>0) {needQuote  = false;} else
			if (generatesql>0) needQuote = IsColText(cols[col]);

		if (generatesql > 0) {
			if (text.Length() != 0) {
				text.Replace(wxT("'"), wxT("''"));
			}
			else text = wxT("null");

		}
		if (generatesql == 3) if (text == "null") str.Append(cname).Append(" is "); else
			str.Append(cname).Append("=");


		if (needQuote)
			str.Append(qtsimbol);
		str.Append(text);
		if (needQuote)
			str.Append(qtsimbol);
	}
	if (cols.Count() > 1 && generatesql > 1) str.Append(")");
	if (generatesql == 1) str=head+wxT(") values (")+str+");";

	return str;
}

wxString ctlSQLGrid::GetColumnName(int colNum)
{
	wxString columnName = GetColLabelValue(colNum);
	columnName = columnName.Left(columnName.find(wxT("\n")));
	return columnName;
}

void ctlSQLGrid::AppendColumnHeader(wxString &str, int start, int end)
{
	size_t i, arrsize;
	arrsize = (end - start + 1);
	wxArrayInt columns;

	for(i = 0; i < arrsize; i++)
	{
		columns.Add(start + i);
	}

	AppendColumnHeader(str, columns);
}

void ctlSQLGrid::AppendColumnHeader(wxString &str, wxArrayInt columns)
{
	if(settings->GetColumnNames()|| generatesql == 2)
	{
		bool CopyQuoting = (settings->GetCopyQuoting() == 1 || settings->GetCopyQuoting() == 2);
		size_t i;
		wxString fielddelim = ",";
		if (generatesql == 3) return;
		if (generatesql == 1) return;
		for(i = 0; i < columns.Count() ; i++)
		{
			long columnPos = columns.Item(i);
			if (generatesql == 2) {
				if (i > 0) str.Append(fielddelim);
				if (i == 0 && columns.Count() > 1) str.Append("(");
				str.Append(GetColumnName(columnPos));
				if (i == columns.Count()-1 && columns.Count() > 1 && generatesql == 2) str.Append(") in (");
				if (columns.Count() == 1 && generatesql == 2) str.Append(" in (");

			}
			else
			{
				if (i > 0)
					str.Append(settings->GetCopyColSeparator());

				if (CopyQuoting)
					str.Append(settings->GetCopyQuoteChar());
				str.Append(GetColumnName(columnPos));
				if (CopyQuoting)
					str.Append(settings->GetCopyQuoteChar());
			}
		}

		str.Append(END_OF_LINE);
	}
}
int compare_int(int* a, int* b)
{
	if (*a > * b) return 1;
	else if (*a < *b) return -1;
	else return 0;
}
#include <map>
class  selMerge {
public:
	std::map<int, std::map<int, int>> getmap() { return grid; }
	void add(int row, int col);
	void setArrayColumns(int row, wxArrayInt& columns);
	void addRange(int r1,int r2, int c1, int c2) {
		//std::map<int, int> v;
		
		for (int r = r1; r <= r2; r++)
		{
			auto &v = grid[r];
			for (int c = c1; c <= c2; c++) {
				v[c] = c;
			}
			//grid[r].swap(v);
		}
	}

private:
	std::map<int, std::map<int, int>> grid;
};
void selMerge::add(int row, int col) {
	std::map<int, int> v;
	v = grid[row];
	v[col] = col;
}

void  selMerge::setArrayColumns(int row, wxArrayInt& columns) {
	auto &v = grid[row];
	for (const auto& pair : v) {
		//std::cout << pair.first << ": " << pair.second << '\n';
		int k = pair.second;
		columns.Add(k);
	}
};
int ctlSQLGrid::Copy(int gensql)
{
	wxString str,tmp,linedelim="";
	int copied = 0;
	size_t i;
	generatesql=gensql;
	//sqlResultTable* t = (sqlResultTable*)GetTable();
	wxString sql = sqlquerytext.Lower();
	int j = 0;
	wxChar c;
	wxString tn=wxEmptyString;
	while ((j = sql.find("from", j)) > -1) {
		j = j + 4;
		c = sql[j];
		i = j;
		while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			i++;
			c = sql[i];
		}
		j = i;
		if (c == '(') {
			continue;
		}
		while (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != ',' && c != '(' && c != ')') {
			j++;
			if (sql.Len() == j) break;
			c = sql[j];
		}
		tn = sql.SubString(i,j-1); // table name
		if (sql.Len() == j) break;
		j = j+1;
		i = j;
		if (sql.Len() == j) break;
		c = sql[j];
		while (c > ' ' && c != ',' && c != '(' && c != ')') {
			j++;
			if (sql.Len() == j) break;
			c = sql[j];
		}
		if (j>i) tn = sql.SubString(i, j - 1); // alias name
		break;
	}

	if (gensql == 2) linedelim = ",";
	else if(gensql == 3) linedelim = " or ";

	if (GetSelectedRows().GetCount())
	{
		AppendColumnHeader(str, 0, (GetNumberCols() - 1));

		wxArrayInt rows = GetSelectedRows();

		for (i = 0 ; i < rows.GetCount() ; i++)
		{
			tmp=GetExportLine(rows.Item(i));
			if (tmp.IsEmpty()) continue;
			if (!tn.IsEmpty() && (generatesql == 1)) tmp.Replace("tbl", tn, false);
			str.Append(tmp);
			if (i < rows.GetCount() - 1 && (generatesql > 1)) str.Append(linedelim);
			if (rows.GetCount() > 1)
				str.Append(END_OF_LINE);
		}

		copied = rows.GetCount();
	}
	else if (GetSelectedCols().GetCount())
	{
		wxArrayInt cols = GetSelectedCols();
		size_t numRows = GetNumberRows();

		AppendColumnHeader(str, cols);

		for (i = 0 ; i < numRows ; i++)
		{
			tmp = GetExportLine(i, cols);
			if (!tn.IsEmpty() && (generatesql == 1)) tmp.Replace("tbl", tn, false);
			str.Append(tmp);
			if (i<(numRows-1) && (generatesql > 1)) str.Append(linedelim);
			if (numRows > 1)
				str.Append(END_OF_LINE);
		}
		copied = numRows;
	}
	else if ( GetSelectionBlockTopLeft().GetCount() > 0 &&
	         GetSelectionBlockBottomRight().GetCount() > 0)
	{
		unsigned int x1, x2, y1, y2;
		int count = GetSelectionBlockTopLeft().GetCount();

		x1 = GetSelectionBlockTopLeft()[0].GetCol();
		x2 = GetSelectionBlockBottomRight()[0].GetCol();
		y1 = GetSelectionBlockTopLeft()[0].GetRow();
		y2 = GetSelectionBlockBottomRight()[0].GetRow();
		copied = 0;
		AppendColumnHeader(str, x1, x2);
		selMerge m;
		for (size_t n = 0; n < count; n++)
		{
			x1 = GetSelectionBlockTopLeft()[n].GetCol();
			x2 = GetSelectionBlockBottomRight()[n].GetCol();
			y1 = GetSelectionBlockTopLeft()[n].GetRow();
			y2 = GetSelectionBlockBottomRight()[n].GetRow();
			if (generatesql > 1) {
				m.addRange(y1, y2, x1, x2);
			} else
			{
				for (i = y1; i <= y2; i++)
				{
					tmp = GetExportLine(i, x1, x2);
					if (!tn.IsEmpty() && (generatesql==1)) tmp.Replace("tbl", tn, false);
					str.Append(tmp);
					if (i < y2 && (generatesql > 1)) str.Append(linedelim);
					if (y2 > y1)
						str.Append(END_OF_LINE);
				}
				
			}
			copied = copied + (y2 - y1 + 1);
		}
		if (generatesql > 1) {
			auto &g = m.getmap();
			wxArrayInt cols;
			int maxr = g.size();
			int i = 1;
			for (const auto& pair : g) {
				int r = pair.first;
				cols.Clear();
				m.setArrayColumns(r, cols);
				str.Append(GetExportLine(r, cols));
				if (i < maxr) str.Append(linedelim);
				i++;
			}
		}
	}
	else
	{
		int row, col;
		if (generatesql > 1) {
			wxGridCellCoordsArray cord = GetSelectedCells();
			int count = cord.GetCount();
			int curr_row = 1000000000;
			int next_row = 1000000000;
			
			for (size_t n = 0; n < count; n++)
			{
				wxGridCellCoords& coords = cord[n];
				if (coords.GetRow() < curr_row) curr_row = coords.GetRow();
			}
			while (curr_row != 1000000000)
			{
				wxArrayInt colsNum;
				next_row = 1000000000;
				for (size_t n = 0; n < count; n++)
				{
					wxGridCellCoords& coords = cord[n];
					//wxString msg;
					//msg.Printf(wxT("Координаты выбраных ячеек row=%d col=%d.\n"), coords.GetRow(), coords.GetCol());
					//wxMessageBox(msg, wxT("About coord"), wxOK | wxICON_INFORMATION, this);
					int r = coords.GetRow();
					if (r == curr_row) {
						colsNum.Add(coords.GetCol());
					}
					else if (r > curr_row && r < next_row) next_row = r;

				}
				colsNum.Sort(compare_int);
				if (generatesql == 2) AppendColumnHeader(str, colsNum);
				str.Append(GetExportLine(curr_row, colsNum));
				if ((next_row != 1000000000) && (generatesql == 3)) str.Append(linedelim);
				if ((next_row != 1000000000) && (generatesql == 2)) str.Append(") or ");
				str.Append(END_OF_LINE);
				curr_row = next_row;
				copied++;
			}
		}
		else {
			row = GetGridCursorRow();
			col = GetGridCursorCol();

			AppendColumnHeader(str, col, col);
			tmp = GetExportLine(row, col, col);
			if (!tn.IsEmpty() && (generatesql == 1)) tmp.Replace("tbl", tn, false);
			str.Append(tmp);
			copied = 1;
		}
	}
	if (copied && (generatesql == 2)) str.Append(")");
	if (copied && wxTheClipboard->Open())
	{
		wxTheClipboard->SetData(new wxTextDataObject(str));
		wxTheClipboard->Close();
	}
	else
	{
		copied = 0;
	}

	return copied;
}

void ctlSQLGrid::OnLabelDoubleClick(wxGridEvent &event)
{
	int maxHeight, maxWidth;
	GetClientSize(&maxWidth, &maxHeight);
	int row = event.GetRow();
	int col = event.GetCol();

	int extent, extentWant = 0;

	if (row >= 0)
	{
		for (col = 0 ; col < GetNumberCols() ; col++)
		{
			extent = GetBestSize(row, col).GetHeight();
			if (extent > extentWant)
				extentWant = extent;
		}

		extentWant += EXTRAEXTENT_HEIGHT;
		extentWant = wxMax(extentWant, GetRowMinimalAcceptableHeight());
		extentWant = wxMin(extentWant, maxHeight * 3 / 4);
		int currentHeight = GetRowHeight(row);

		if (currentHeight >= maxHeight * 3 / 4 || currentHeight == extentWant)
			extentWant = GetRowMinimalAcceptableHeight();
		else if (currentHeight < maxHeight / 4)
			extentWant = wxMin(maxHeight / 4, extentWant);
		else if (currentHeight < maxHeight / 2)
			extentWant = wxMin(maxHeight / 2, extentWant);
		else if (currentHeight < maxHeight * 3 / 4)
			extentWant = wxMin(maxHeight * 3 / 4, extentWant);

		if (extentWant != currentHeight)
		{
			BeginBatch();
			if(IsCellEditControlShown())
			{
				HideCellEditControl();
				SaveEditControlValue();
			}

			SetRowSize(row, extentWant);
			EndBatch();
		}
	}
	else if (col >= 0)
	{
		// Holding Ctrl or Meta switches back to automatic column's sizing
		if (event.ControlDown() || event.CmdDown())
		{
			colSizes.erase(GetColKeyValue(col));
			BeginBatch();
			if(IsCellEditControlShown())
			{
				HideCellEditControl();
				SaveEditControlValue();
			}
			AutoSizeColumn(col, false);
			EndBatch();
		}
		else // toggle between some predefined sizes
		{

			if (col < (int)colMaxSizes.GetCount() && colMaxSizes[col] >= 0)
				extentWant = colMaxSizes[col];
			else
			{
				for (row = 0 ; row < GetNumberRows() ; row++)
				{
					if (CheckRowPresent(row))
					{
						extent = GetBestSize(row, col).GetWidth();
						if (extent > extentWant)
							extentWant = extent;
					}
				}
			}

			extentWant += EXTRAEXTENT_WIDTH;
			extentWant = wxMax(extentWant, GetColMinimalAcceptableWidth());
			extentWant = wxMin(extentWant, maxWidth * 3 / 4);
			int currentWidth = GetColSize(col);

			if (currentWidth >= maxWidth * 3 / 4 || currentWidth == extentWant)
				extentWant = GetColMinimalAcceptableWidth();
			else if (currentWidth < maxWidth / 4)
				extentWant = wxMin(maxWidth / 4, extentWant);
			else if (currentWidth < maxWidth / 2)
				extentWant = wxMin(maxWidth / 2, extentWant);
			else if (currentWidth < maxWidth * 3 / 4)
				extentWant = wxMin(maxWidth * 3 / 4, extentWant);

			if (extentWant != currentWidth)
			{
				BeginBatch();
				if(IsCellEditControlShown())
				{
					HideCellEditControl();
					SaveEditControlValue();
				}
				SetColSize(col, extentWant);
				EndBatch();
				colSizes[GetColKeyValue(col)] = extentWant;
			}
		}
	}
}

	void ctlSQLGrid::OnCellRightClick(wxGridEvent &event)
{
	int row = event.GetRow();
	int col = event.GetCol();
	
	//SetRowLabelValue(row-1,);
	//HideRow(row);
}
void ctlSQLGrid::OnLabelClick(wxGridEvent &event)
{
	int row = event.GetRow();
	int col = event.GetCol();
	if (row >= 0 && grp) {
		grp->VisibleGroup(row,GetRowSize(row+1)==0);
		Refresh();
		return;
	}
	if ( col >= 0 && (event.AltDown() ) )
	{
		// continue for sort event
		if (IsSort()) {
			sqlResultTable *t=(sqlResultTable *)GetTable();
			t->setSortColumn(col);
			return;
		}
	}
	// add support for (de)selecting multiple rows and cols with Control pressed
	else if ( row >= 0 && (event.ControlDown() || event.CmdDown()) )
	{
		if (GetSelectedRows().Index(row) == wxNOT_FOUND)
			SelectRow(row, true);
		else
			DeselectRow(row);
	}
	else if ( col >= 0 && (event.ControlDown() || event.CmdDown()) )
	{
		if (GetSelectedCols().Index(col) == wxNOT_FOUND)
			SelectCol(col, true);
		else
			DeselectCol(col);
		event.Skip();
	}
	else
		event.Skip();
}

void ctlSQLGrid::AutoSizeColumn(int col, bool setAsMin, bool doLimit)
{
	ColKeySizeHashMap::iterator it = colSizes.find(GetColKeyValue(col));
	if (it != colSizes.end()) // Restore user-specified size
		SetColSize(col, it->second);
	else
		wxGrid::AutoSizeColumn(col, setAsMin);

	if (doLimit)
	{
		int newSize, oldSize;
		int maxSize, totalSize = 0, availSize;

		oldSize = GetColSize(col);
		availSize = GetClientSize().GetWidth() - GetRowLabelSize();
		maxSize = availSize / 2;
		for (int i = 0 ; i < GetNumberCols() ; i++)
			totalSize += GetColSize(i);

		if (oldSize > maxSize && totalSize > availSize)
		{
			totalSize -= oldSize;
			/* Shrink wide column to maxSize.
			 * If the rest of the columns are short, make sure to use all the remaining space,
			 *   but no more than oldSize (which is enough according to AutoSizeColumns())
			 */
			newSize = wxMin(oldSize, wxMax(maxSize, availSize - totalSize));
			SetColSize(col, newSize);
		}
	}
}

void ctlSQLGrid::AutoSizeColumns(bool setAsMin)
{
	wxCoord newSize, oldSize;
	wxCoord maxSize, totalSize = 0, availSize;
	int col, nCols = GetNumberCols();
	int row, nRows = GetNumberRows();
	colMaxSizes.Empty();

	/* We need to check each cell's width to choose best. wxGrid::AutoSizeColumns()
	 * is good, but looping through long result sets gives a noticeable slowdown.
	 * Thus we'll check every first 500 cells for each column.
	 */

	// First pass: auto-size columns
	for (col = 0 ; col < nCols; col++)
	{
		ColKeySizeHashMap::iterator it = colSizes.find(GetColKeyValue(col));
		if (it != colSizes.end()) // Restore user-specified size
		{
			newSize = it->second;
			colMaxSizes.Add(-1);
		}
		else
		{
			wxClientDC dc(GetGridWindow());
			newSize = 0;
			// get cells's width
			for (row = 0 ; row < wxMin(nRows, 500) ; row++)
			{
				wxSize size = GetBestSize(row, col);
				if ( size.x > newSize )
					newSize = size.x;
			}
			// get column's label width
			wxCoord w, h;
			dc.SetFont( GetLabelFont() );
			dc.GetMultiLineTextExtent( GetColLabelValue(col), &w, &h );
			if ( GetColLabelTextOrientation() == wxVERTICAL )
				w = h;

			if ( w > newSize )
				newSize = w;

			if (!newSize)
				newSize = GetRowLabelSize();
			else
				// leave some space around text
				newSize += 6;

			colMaxSizes.Add(newSize);
		}
		SetColSize(col, newSize);
		totalSize += newSize;
	}

	availSize = GetClientSize().GetWidth() - GetRowLabelSize();

	// Second pass: shrink wide columns if exceeded available width
	if (totalSize > availSize)
	{
		// A wide column shouldn't take up more than 50% of the visible space
		maxSize = availSize / 2;
		for (col = 0 ; col < nCols ; col++)
		{
			oldSize = GetColSize(col);
			// Is too wide and no user-specified size
			if (oldSize > maxSize && !(col < (int)colMaxSizes.GetCount() && colMaxSizes[col] == -1))
			{
				totalSize -= oldSize;
				/* Shrink wide column to maxSize.
				 * If the rest of the columns are short, make sure to use all the remaining space,
				 *   but no more than oldSize (which is enough according to first pass)
				 */
				newSize = wxMin(oldSize, wxMax(maxSize, availSize - totalSize));
				SetColSize(col, newSize);
				totalSize += newSize;
			}
		}
	}
}

wxString ctlSQLGrid::GetColKeyValue(int col)
{
	wxString colKey = wxString::Format(wxT("%d:"), col) + GetColLabelValue(col);
	return colKey;
}

wxSize ctlSQLGrid::GetBestSize(int row, int col)
{
	wxSize size;

	wxGridCellAttr *attr = GetCellAttr(row, col);
	wxGridCellRenderer *renderer = attr->GetRenderer(this, row, col);
	if ( renderer )
	{
		wxClientDC dc(GetGridWindow());
		size = renderer->GetBestSize(*this, *attr, dc, row, col);
		int h =	renderer->GetBestHeight(*this, *attr, dc, row, col, size.GetWidth());
		size.SetHeight(h);
		renderer->DecRef();
	}

	attr->DecRef();

	return size;
}



int recurse(ctlSQLGrid *g, int pos,int row, double &transfer) {
	wxString text;
	double leveltime=0; //actual time level
	double lastnode=0;
	while (row<g->GetNumberRows()) {
		text = g->GetCellValue(row, 0);
		int p=0;
		while (text.at(p)==' ' )
		{
			p++;
		} 
		if (p==pos) {
			//
			lastnode=0;
			if (text.at(p)=='-') {
				// посчитаем время работы узла
					double m=1;
					// ->  Nested Loop  (cost=205.13..273.44 rows=4 width=188) (actual time=13.157..13.157 rows=0 loops=1)
					wxRegEx foundstr(wxT("actual time=.*?\\.\\.([0-9.]+).*?loops=([0-9]+)\\)"),wxRE_ADVANCED);
					if (foundstr.Matches(text)) {
							wxString v=foundstr.GetMatch(text,1);
							v.ToCDouble(&lastnode);
							v=foundstr.GetMatch(text,2);
							v.ToDouble(&m);
							lastnode=lastnode*m;
							leveltime=leveltime+lastnode;
					} 
					g->grp->ColoriseRow(row,wxColour(248,240,130));

			} else
			{
				g->grp->ColoriseRow(row,wxColour(224,255,224));
				g->GetTable()->SetRowLabelValue(row,wxEmptyString);
			}
			row++;
			continue;
		} if (p<pos) {
			// end level
			// leveltime содержит время сумаррное время работы узлов этого уровня
			transfer=leveltime;
			return row;
		} if (p>pos) {
			// nested level
			//g->SetRowGroup(row-1);
			wxString s;
			int newrow=recurse(g,p,row,transfer);
			// 
			//leveltime=leveltime+transfer;
			//GroupRows *u=g->getgroup();
			double tt=lastnode-transfer;
			text = g->GetCellValue(row-1, 0);
			if ((text.Find("Append")>0)
				||(text.Find("Gather")>0)) 
				tt=lastnode;
			g->grp->AddGroup(row-1,newrow-1,tt);
			s << (tt) ;
			s=s+wxT("-");
			g->GetTable()->SetRowLabelValue(row-1,s);

			row=newrow;
		}

	}
	return row;
}
void ctlSQLGrid::SetRowGroup(int row) { };
bool ctlSQLGrid::FullArrayCollapseRowsPlan(bool clear)
{
	//wxString colKey = wxString::Format(wxT("%d:"), col) + GetColLabelValue(col);
	wxString text;
	//for(int row = 0; row < GetNumberRows(); ++row) 
	
	if (grp) { delete grp;}
	if (clear) { grp=NULL; return true;}
    grp = new GroupRows(this);
	double transfersum=0;
	int r= recurse(this,0,0,transfersum);
	grp->CalcTime();
	//for(int row = 0; row < GetNumberRows(); ++row) {
	//	text = GetCellValue(row, 0);
	//    //if (row%2==0) SetRowAttr(row,pAttr);

	//}

	return true;
}


