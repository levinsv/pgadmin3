//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// ctlSQLGrid.h - SQL Data Display Grid
//
//////////////////////////////////////////////////////////////////////////

#ifndef CTLSQLGRID_H
#define CTLSQLGRID_H

// wxWindows headers
#include <wx/grid.h>

class GroupRows;

class ctlSQLGrid : public wxGrid
{
public:
	ctlSQLGrid(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size);
	ctlSQLGrid();

	wxString GetExportLine(int row);
	wxString GetExportLine(int row, wxArrayInt cols);
	wxString GetExportLine(int row, int col1, int col2);
	virtual bool IsColText(int col)
	{
		return false;
	}
	int Copy(int gensql);

	virtual bool CheckRowPresent(int row)
	{
		return true;
	};
	bool IsSort()
	{
		return isSort;
	};
	void SetSort(bool flag)
	{
		isSort=flag;
	};

	wxString GetColumnName(int colNum);
	wxSize GetBestSize(int row, int col);
	void OnLabelDoubleClick(wxGridEvent &event);
	void OnGridSelectCell(wxGridEvent& evt);
	void OnLabelClick(wxGridEvent &event);
	void OnCellRightClick(wxGridEvent &event);
	bool FullArrayCollapseRowsPlan(bool clear);
	void AutoSizeColumn(int col, bool setAsMin = false, bool doLimit = true);
	void AutoSizeColumns(bool setAsMin);
	wxString GetRowLabelValue( int row );
	void SetRowGroup(int row);
	GroupRows *grp;
	int generatesql; // 0 -���, 1 - insert , 2 - in_list
	wxString sqlquerytext;
	// Fast searh
	wxString searchStr;

	WX_DECLARE_STRING_HASH_MAP( int, ColKeySizeHashMap );

	DECLARE_DYNAMIC_CLASS(ctlSQLGrid)
	DECLARE_EVENT_TABLE()
	
private:
	void OnCopy(wxCommandEvent &event);
	void OnMouseWheel(wxMouseEvent &event);
	void OnGridColSize(wxGridSizeEvent &event);
	void DrawColLabel( wxDC& dc, int col );
	void DrawRowLabel(wxDC& dc, int row);
	wxString GetColKeyValue(int col);
	void AppendColumnHeader(wxString &str, int start, int end);
	void AppendColumnHeader(wxString &str, wxArrayInt columns);
	// Stores sizes of colums explicitly resized by user
	ColKeySizeHashMap colSizes;
	// Max size for each column
	wxArrayInt colMaxSizes;
	bool isSort;
	
};

class GroupRows
{
public:
	GroupRows( ctlSQLGrid *grid) {
		g=grid;
		rowsGroup.Clear();
		rowsGroup.Add(0,g->GetNumberRows());
		end.Clear();
		end.Add(-1,g->GetNumberRows());
		run.Clear();
		run.Add(0.0,g->GetNumberRows());
	};
	void AddGroup( int rowgroup, int lastrowgroup, double actualtime) {
		// default group open
		rowsGroup[rowgroup]=-rowgroup;
		//beg[rowgroup]=rowgroup;
		wxASSERT_MSG(lastrowgroup>end.Count()," out of bounds");
		end[rowgroup]=lastrowgroup;
		run[rowgroup]=actualtime;
	};
	void VisibleGroup(int row, bool visible) {
		int endg=end[row];
		int grp=IsGroupRow(row);
		int gg;
		int r=row+1;
		if (grp!=0) {

			if (!visible) {
				// hide group
				rowsGroup[row]*=-1;
				for(int i=r;i<(endg+1);i++) {
					g->HideRow(i);
				}
				wxGridCellAttr* pAttrg = new wxGridCellAttr;
				pAttrg->SetBackgroundColour(wxColour(200,191,232));
				g->SetRowAttr(row,pAttrg);
			} else
			{
				// show group 
				rowsGroup[row]*=-1;
				int sizerow=g->GetDefaultRowSize();
				//pAttr->SetBackgroundColour(wxColour(0,162,232));
				wxGridCellAttr* pAttrg = new wxGridCellAttr;
				pAttrg->SetBackgroundColour(wxColour(248,240,130));
				g->SetRowAttr(row,pAttrg);
				for(int i=r;i<(endg+1);i++) {
					gg=IsGroupRow(i);
					if (gg<=0) {
						g->SetRowSize(i,sizerow);
					}
					else 
					{
						g->SetRowSize(i,sizerow);
						i=end[i];
					}

				}

			}
		}
	};
	// 0 - no group, -int - open group ,+int - close gruop
	int IsGroupRow(int row) {
		if (end[row]!=-1) {
			// is group
			return rowsGroup[row];
		} else return 0;
	};
	void ColoriseRow(int row,const wxColour& color) {
		//g->GetTable()->SetRowLabelValue(row-1,s);
						wxGridCellAttr* pAttr = new wxGridCellAttr;
						pAttr->SetBackgroundColour(color);
						g->SetRowAttr(row,pAttr);
	};
	void CalcTime() {
		//g->GetTable()->SetRowLabelValue(row-1,s);
			double sum=0;
			double total=1;
			for(int i=0;i<run.Count();i++) {
				double t=run[i];
				if (i==0) {
					total=abs(t);
					g->GetTable()->SetRowLabelValue(i,"100");
				} else
				{
					wxString str;
					if (total==0||t==0) {
						str="";
					} else
					{
					
					t=(t/total)*100;
					str.Printf(wxT("%5.2f"), t);
					}
					g->GetTable()->SetRowLabelValue(i,str);
				}

			}
			

	};

private:
	ctlSQLGrid *g;
	wxArrayInt rowsGroup, end;
	wxArrayDouble run;
};
//#define TEST_wxGridCellAutoWrapStringRenderer
#ifndef TEST_wxGridCellAutoWrapStringRenderer
class CursorCellRenderer : public wxGridCellStringRenderer
      {
      public:
         virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc,
               const wxRect& rect, int row, int col, bool isSelected)
         {
			int hAlign, vAlign;
			int sPos=-1;
			attr.GetAlignment(&hAlign, &vAlign);
            //////////////////////////////////////////////////////////////////////////////
            //CursorCellRenderer::Draw(grid, attr, dc, rect, row, col, isSelected); //
			dc.SetBackgroundMode( wxSOLID );
			wxString text=grid.GetCellValue(row, col);
			// grey out fields if the grid is disabled
			if ( grid.IsEnabled() )
			{
				if ( isSelected )
				{
					wxColour clr;
					if ( wxWindow::FindFocus() == grid.GetGridWindow() )
						clr = grid.GetSelectionBackground();
					else
						clr = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
				
				
					dc.SetBrush( wxBrush(clr, wxSOLID) );
				}
				else
				{
					wxColor color;
					color.Set(239, 228, 176);
					if (sPos=text.Find(wxT('\n'))!=wxNOT_FOUND ) 
						dc.SetBrush( wxBrush(color, wxSOLID) );
					else
						dc.SetBrush( wxBrush(attr.GetBackgroundColour(), wxSOLID) );
				}
			}
			else
			{
				dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE), wxSOLID));
			}

			dc.SetPen( *wxTRANSPARENT_PEN );
			dc.DrawRectangle(rect);
            //////////////////////////////////////////////////////////////////////////////
			SetTextColoursAndFont(grid, attr, dc, isSelected);
			ctlSQLGrid* ctrl = static_cast<ctlSQLGrid*>(&grid);
			if (!ctrl->searchStr.IsEmpty()) {
				if (sPos == -1) sPos = text.Len();
				int pp;

				pp = text.Find(ctrl->searchStr);
				if (pp >= 0) {
					wxArrayString lines;
					grid.StringToLines(text, lines);
					wxRect r;
					r.y = rect.y;
					dc.SetBrush(*wxYELLOW_BRUSH);
					size_t nLines = lines.GetCount();
					for (size_t l = 0; l < nLines; l++)
					{
						const wxString& line = lines[l];
						pp = line.Find(ctrl->searchStr);
						if (line.empty() || (pp==-1))
						{
							r.y += dc.GetCharHeight();
							continue;
						}
					
					int lineWidth, lineWidthP, lineHeight,start=0;
					wxString pref;
					if (hAlign == wxALIGN_RIGHT) {
						start = pp + ctrl->searchStr.Len();
						pref = line.substr(start);
					} else 
						pref = line.substr(start, pp);

					dc.GetTextExtent(pref, &lineWidthP, &lineHeight);
					r.x=rect.x+lineWidthP;
					pref= line.substr(pp, ctrl->searchStr.Len());
					dc.GetTextExtent(pref, &lineWidth, &lineHeight);
					r.width = lineWidth;
					r.height = lineHeight;
					if (hAlign == wxALIGN_RIGHT) r.x = rect.x + (rect.width - lineWidth- lineWidthP);
					if (!(r.y < (rect.y + rect.height)))  { r.y = rect.y + rect.height - 5; r.height = 5; }
					if (!((r.x < (rect.x + rect.width)))) { r.x = rect.x + rect.width - 5; r.width = 5; }
					dc.DrawRoundedRectangle(r, 3);
					break;
					}
				}
			}

			grid.DrawTextRectangle(dc, text,
                           rect, hAlign, vAlign);
         }
#else
class CursorCellRenderer : public wxGridCellAutoWrapStringRenderer
{
public:
	virtual void Draw(wxGrid& grid,
			 wxGridCellAttr& attr,
			 wxDC& dc,
			 const wxRect& rectCell,
			 int row, int col,
			 bool isSelected) {
			 bool f = false;
			 wxString text = grid.GetCellValue(row, col);
			 wxColor prev;
			 if (!isSelected) {
				 wxColor color;
				 color.Set(239, 228, 176);
				 if (text.Find(wxT('\n')) != wxNOT_FOUND)
				 {
					 //dc.SetBrush(wxBrush(color, wxSOLID));
					 prev = attr.GetBackgroundColour();
					 attr.SetBackgroundColour(color);
					 f = true;
				 }
				 
			 }

			 wxGridCellAutoWrapStringRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);
			 if (f) attr.SetBackgroundColour(prev);
		 }
#endif
      };

#endif
