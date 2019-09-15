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
	int Copy(bool gensql);

	virtual bool CheckRowPresent(int row)
	{
		return true;
	};
	wxSize GetBestSize(int row, int col);
	void OnLabelDoubleClick(wxGridEvent &event);
	void OnLabelClick(wxGridEvent &event);
	void OnCellRightClick(wxGridEvent &event);
	bool FullArrayCollapseRowsPlan(bool clear);
	void AutoSizeColumn(int col, bool setAsMin = false, bool doLimit = true);
	void AutoSizeColumns(bool setAsMin);
	wxString GetRowLabelValue( int row );
	void SetRowGroup(int row);
	GroupRows *grp;
	bool generatesql;
	WX_DECLARE_STRING_HASH_MAP( int, ColKeySizeHashMap );

	DECLARE_DYNAMIC_CLASS(ctlSQLGrid)
	DECLARE_EVENT_TABLE()
	
private:
	void OnCopy(wxCommandEvent &event);
	void OnMouseWheel(wxMouseEvent &event);
	void OnGridColSize(wxGridSizeEvent &event);
	wxString GetColumnName(int colNum);
	wxString GetColKeyValue(int col);
	void AppendColumnHeader(wxString &str, int start, int end);
	void AppendColumnHeader(wxString &str, wxArrayInt columns);
	// Stores sizes of colums explicitly resized by user
	ColKeySizeHashMap colSizes;
	// Max size for each column
	wxArrayInt colMaxSizes;
	
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
		wxASSERT(lastrowgroup>end.Count()," out of bounds");
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

class CursorCellRenderer : public wxGridCellStringRenderer
      {
      public:
         virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc,
               const wxRect& rect, int row, int col, bool isSelected)
         {
			int hAlign, vAlign;
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
			if (text.Find(wxT('\n'))!=wxNOT_FOUND ) 
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
			grid.DrawTextRectangle(dc, text,
                           rect, hAlign, vAlign);
         }

      };

#endif
