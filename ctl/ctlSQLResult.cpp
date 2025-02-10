//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// ctlSQLResult.cpp - SQL Query result window
//
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include <wx/clipbrd.h>

#include "db/pgConn.h"
#include "db/pgQueryThread.h"
#include "ctl/ctlSQLResult.h"
#include "utils/sysSettings.h"
#include "frm/frmExport.h"
#include "frm/mathplot.h"
#include "frm/frmPlot.h"
#include "ctl/SourceViewDialog.h"


ctlSQLResult::ctlSQLResult(wxWindow *parent, pgConn *_conn, wxWindowID id, const wxPoint &pos, const wxSize &size)
	: ctlSQLGrid(parent, id, pos, size)
{
	conn = _conn;
	thread = NULL;

	SetTable(new sqlResultTable(), true);
	
	EnableEditing(false);
	SetSizer(new wxBoxSizer(wxVERTICAL));
	cg=GetGridLineColour();
	Connect(wxID_ANY, wxEVT_GRID_RANGE_SELECT, wxGridRangeSelectEventHandler(ctlSQLResult::OnGridSelect));
	Connect(wxID_ANY, wxEVT_GRID_COL_SORT, wxGridEventHandler(ctlSQLResult::OnGridColSort));
	Connect(wxID_ANY, wxEVT_KEY_UP, wxCharEventHandler(ctlSQLResult::OnKeyUp));
	Connect(wxID_ANY, wxEVT_CHAR, wxCharEventHandler(ctlSQLResult::OnKeyChar));
	m_win_s = NULL;
}



ctlSQLResult::~ctlSQLResult()
{
	Abort();

	if (thread)
	{
		if (thread->IsRunning())
		{
			thread->CancelExecution();
			thread->Wait();
		}

		delete thread;
		thread = NULL;
	}
}


void ctlSQLResult::SetConnection(pgConn *_conn)
{
	conn = _conn;
}


bool ctlSQLResult::Export()
{
	if (NumRows() > 0)
	{
		frmExport dlg(this);
		if (dlg.ShowModal() == wxID_OK)
			return dlg.Export(NULL);
	}
	return false;
}

bool ctlSQLResult::ToFile()
{
	if (NumRows() > 0)
	{
		frmExport dlg(this);
		if (dlg.ShowModal() == wxID_OK)
			return dlg.Export(thread->DataSet());
	}
	return false;
}

bool ctlSQLResult::ToFile(frmExport *frm)
{
	if (NumRows() > 0)
	{
		return frm->Export(thread->DataSet());
	}
	return false;
}



bool ctlSQLResult::IsColText(int col)
{
	switch (colTypClasses.Item(col))
	{
		case PGTYPCLASS_NUMERIC:
		case PGTYPCLASS_BOOL:
			return false;
	}

	return true;
}


int ctlSQLResult::Execute(const wxString &query, int resultToRetrieve, wxWindow *caller, long eventId, void *data)
{
	wxGridTableMessage *msg;
	sqlResultTable *table = (sqlResultTable *)GetTable();

	msg = new wxGridTableMessage(table, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, GetNumberRows());
	ProcessTableMessage(*msg);
	delete msg;
	msg = new wxGridTableMessage(table, wxGRIDTABLE_NOTIFY_COLS_DELETED, 0, GetNumberCols());
	ProcessTableMessage(*msg);
	delete msg;

	Abort();

	colNames.Empty();
	colTypes.Empty();
	colTypClasses.Empty();
	thread = new pgQueryThread(conn, query, resultToRetrieve, caller, eventId, data);

	if (thread->Create() != wxTHREAD_NO_ERROR)
	{
		Abort();
		return -1;
	}

	((sqlResultTable *)GetTable())->SetThread(thread);
	sqlquerytext= wxString(query);
	thread->Run();
	return RunStatus();
}


int ctlSQLResult::Abort()
{
	if (thread)
	{
		((sqlResultTable *)GetTable())->SetThread(0);

		if (thread->IsRunning())
		{
			thread->CancelExecution();
			thread->Wait();
		}

		delete thread;
		thread = NULL;
	}
	return 0;
}



void ctlSQLResult::DisplayData(bool single)
{
	if (!thread || !thread->DataValid())
		return;

	if (thread->ReturnCode() != PGRES_TUPLES_OK)
		return;

	rowcountSuppressed = single;
	Freeze();

	/*
	 * Resize and repopulate by informing it to delete all the rows and
	 * columns, then append the correct number of them. Probably is a
	 * better way to do this.
	 */
	wxGridTableMessage *msg;
	sqlResultTable *table = (sqlResultTable *)GetTable();
	
	msg = new wxGridTableMessage(table, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, GetNumberRows());
	ProcessTableMessage(*msg);
	delete msg;
	msg = new wxGridTableMessage(table, wxGRIDTABLE_NOTIFY_COLS_DELETED, 0, GetNumberCols());
	ProcessTableMessage(*msg);
	delete msg;
	msg = new wxGridTableMessage(table, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, NumRows());
	ProcessTableMessage(*msg);
	delete msg;
	msg = new wxGridTableMessage(table, wxGRIDTABLE_NOTIFY_COLS_APPENDED, thread->DataSet()->NumCols());
	ProcessTableMessage(*msg);
	delete msg;
	table->initSort();
	SetSort(true);
	if (NumRows()<1000) {
		int h, v;
		GetDefaultCellAlignment(&h, &v);
	for(int row = 0; row < NumRows(); ++row) {
	    if (row%2==0) {
			    wxGridCellAttr* pAttr = new wxGridCellAttr;
			    pAttr->SetBackgroundColour(wxColour(224,255,224));
				
				pAttr->SetAlignment(h,v);
				SetRowAttr(row,pAttr);
		}
		
	}
	}
	table->isplan=false;
	wxString c=thread->DataSet()->ColName(0);
	if (c==wxT("QUERY PLAN")) {
		//
		table->isplan=true;
		FullArrayCollapseRowsPlan(false);
	} else FullArrayCollapseRowsPlan(true);

	if (single)
	{
		colNames.Add(thread->DataSet()->ColName(0));
		colTypes.Add(wxT(""));
		colTypClasses.Add(0L);

		AutoSizeColumn(0, false, false);
	}
	else
	{
		long col, nCols = thread->DataSet()->NumCols();

		AutoSizeColumns(false);

		for (col = 0 ; col < nCols ; col++)
		{
			colNames.Add(thread->DataSet()->ColName(col));
			colTypes.Add(thread->DataSet()->ColFullType(col));
			colTypClasses.Add(thread->DataSet()->ColTypClass(col));
			if (thread->DataSet()->ColTypClass(col) == PGTYPCLASS_NUMERIC)
			{
				/*
				 * For numeric columns, set alignment to right.
				 */
				wxGridCellAttr *attr = new wxGridCellAttr();
				attr->SetAlignment(wxALIGN_RIGHT, wxALIGN_TOP);
				SetColAttr(col, attr);
			}
		}
	}
	Thaw();
}



wxString ctlSQLResult::GetMessagesAndClear()
{
	if (thread)
		return thread->GetMessagesAndClear();
	return wxEmptyString;
}


wxString ctlSQLResult::GetErrorMessage()
{
	return conn->GetLastError();
}

pgError ctlSQLResult::GetResultError()
{
	if (thread)
		return thread->GetResultError();

	pgError dummy;
	memset(&dummy, 0, sizeof(dummy));
	return dummy;
}

long ctlSQLResult::NumRows() const
{
	if (thread && thread->DataValid())
		return thread->DataSet()->NumRows();
	return 0;
}


long ctlSQLResult::InsertedCount() const
{
	if (thread)
		return thread->RowsInserted();
	return -1;
}


OID ctlSQLResult::InsertedOid() const
{
	if (thread)
		return thread->InsertedOid();
	return (OID) - 1;
}


int ctlSQLResult::RunStatus()
{
	if (!thread)
		return -1;

	if (thread->IsRunning())
		return CTLSQL_RUNNING;

	return thread->ReturnCode();
}


wxString ctlSQLResult::OnGetItemText(long item, long col) const
{
	if (thread && thread->DataValid())
	{
		if (!rowcountSuppressed)
		{
			if (col)
				col--;
			else
				return NumToStr(item + 1L);
		}
		if (item >= 0)
		{
			//thread->DataSet()->Locate(item + 1);
			//return thread->DataSet()->GetVal(col);
			sqlResultTable *t=(sqlResultTable *)GetTable();
			return t->GetValueFast(item,col);
		}
		else
			return thread->DataSet()->ColName(col);
	}
	return wxEmptyString;
}
wxString ctlSQLResult::CopySelColumnNameType(bool onlyname)
{
	wxString ss = wxEmptyString;
	if (GetSelectedCols().GetCount()) {
		wxArrayInt cols = GetSelectedCols();
		int err = 0;
		int noformat = 0;
		int dtType = -1;
		//AppendColumnHeader(str, cols);
			//str.Append(GetExportLine(i, cols));
		for (size_t col = 0; col < cols.Count(); col++)
		{
			int cl = cols[col];
			bool isDt = false;
			wxString colName = colNames.Item(cl);
			wxString colType = colTypes.Item(cl);
			if (!ss.IsEmpty()) ss.Append(", ");
			ss.Append(colName);
			if (!onlyname) ss.Append(" " + colType);

		}
	}
	if (wxTheClipboard->Open())
	{
		wxTheClipboard->SetData(new wxTextDataObject(ss));
		wxTheClipboard->Close();
	}

	return ss;
}
wxString ctlSQLResult::CheckSelColumnDate()
{
	size_t i;
	wxString ss = wxEmptyString;
	if (GetSelectedCols().GetCount()) {
		wxArrayInt cols = GetSelectedCols();
		size_t numRows = GetNumberRows();
		int err = 0;
		int noformat = 0;
		int dtType = -1;
		//AppendColumnHeader(str, cols);
			//str.Append(GetExportLine(i, cols));
			for (size_t col = 0; col < cols.Count(); col++)
			{
				int cl = cols[col];
				bool isDt = false;
				switch (colTypClasses.Item(cl))
				{
				case PGTYPCLASS_DATE:
					isDt= true;
				}
				wxDateTime dt((time_t)-1);
				wxDateTime dt_prev;
				wxTimeSpan sp,tmp;
				bool parseDT = false;
				int k = 0;
				for (i = 0; i < numRows; i++)
				{
					if (!isDt) break;
					wxString text = GetCellValue(i, cl);
					if (GetRowSize(i) > 0) {

						if (dtType == -1) {
							if (dt.ParseISOCombined(text, ' ')) {
								dtType = 0;
							}
							else if (dt.ParseDateTime(text)) {
								dtType = 1;
							}


						}
						if (dtType>=0) {
							parseDT = false;
							if (dtType == 0 && dt.ParseISOCombined(text, ' ')) parseDT = true; 
							if (dtType == 1 && dt.ParseDateTime(text)) parseDT = true;
							if (parseDT) {
								if (k == 0) { dt_prev = dt; k++; continue; }
								tmp = dt - dt_prev;
								if (k == 1) { sp = tmp; dt_prev = dt; k++; continue; }
								if (tmp.GetMilliseconds() != sp.GetMilliseconds()) {
									wxGridCellAttr* pAttr = new wxGridCellAttr;
									
									pAttr->SetBackgroundColour(*wxYELLOW);
									SetRowAttr(i, pAttr);
									err++;
								}
								dt_prev = dt;
							}
							else noformat++;
						};
						k++;
					};
					if (tmp.GetDays()>0) ss = tmp.Format("delta Days %D Hours:%H Min:%M Sec:%S");
					else if (tmp.GetHours() > 0) ss = tmp.Format("delta Hours:%H Min:%M Sec:%S");
						else if (tmp.GetMinutes() > 0) ss = tmp.Format("delta Min:%M Sec:%S");
							else if (tmp.GetSeconds() > 0) ss = tmp.Format("delta Sec:%S");
							else ss = tmp.Format("delta MSec:%l");

					
				}
			}
			
			if (dtType==-1) ss= wxString::Format("No timestamp(date) column type",err);
			else {
				ss = wxString::Format("Unsequence rows: %d,bad format rows: %d,%s", err, noformat,ss);

			}
			
	}
	return ss;
}
wxDateTime parseDT(int &fmttype,const wxString dtStr) {
	wxDateTime dt((time_t)-1);
	if (fmttype == -1) {
		if (dt.ParseISOCombined(dtStr, ' ')) {
			fmttype = 0;
		}
		else if (dt.ParseDateTime(dtStr)) {
			fmttype = 1;
		} else fmttype = -1;
		return dt;
	}
	else if (fmttype == 0) {
		if (dt.ParseISOCombined(dtStr, ' ')) return dt;
		else {
			fmttype = -1;
			return parseDT(fmttype,dtStr);
		}
		
	}
	else if (fmttype == 1) {
		if (dt.ParseDateTime(dtStr)) return dt;
		else {
			fmttype = -1;
			return parseDT(fmttype, dtStr);
		}
	}
	return dt;
}
wxString ctlSQLResult::CompareSelectCells() {
	if (IsSelection()) {
		unsigned int i;
		int row, col;
		wxGridCellCoordsArray cells = GetSelectedCells();
		wxString sql_1;
		wxString sql_2;
		wxString cname;
		if (GetSelectionBlockTopLeft().GetCount() > 0 &&
			GetSelectionBlockBottomRight().GetCount() > 0)
		{
			unsigned int x1, x2, y1, y2;
			int count = GetSelectionBlockTopLeft().GetCount();

			x1 = GetSelectionBlockTopLeft()[0].GetCol();
			x2 = GetSelectionBlockBottomRight()[0].GetCol();
			y1 = GetSelectionBlockTopLeft()[0].GetRow();
			y2 = GetSelectionBlockBottomRight()[0].GetRow();
			for (size_t n = 0; n < count; n++)
			{
				x1 = GetSelectionBlockTopLeft()[n].GetCol();
				x2 = GetSelectionBlockBottomRight()[n].GetCol();
				y1 = GetSelectionBlockTopLeft()[n].GetRow();
				y2 = GetSelectionBlockBottomRight()[n].GetRow();
				for (i = y1; i <= y2; i++)
				{
					//clearCellData(row, col);
					row = i;
					col = x1;
					if (sql_1.IsEmpty()) { sql_1 = GetCellValue(row, col); cname = wxString::Format("Row: %d col:%s", row + 1, ctlSQLGrid::GetColumnName(col)); }
					else {
						sql_2 = GetCellValue(row, col); cname += wxString::Format(" # Row: %d col:%s", row + 1, ctlSQLGrid::GetColumnName(col));
					}
					if (!sql_2.IsEmpty() && sql_1 == sql_2) {
						sql_2 = wxEmptyString;
						cname = cname.BeforeFirst('#');
						continue;
					}
					if (!sql_2.IsEmpty()) {
						SourceViewDialog* dlg = new SourceViewDialog(NULL, sql_1, sql_2, cname);
						dlg->Show();
						return "Diff cells";
					}
				}
			}
		}
		else
		{
		}
	}
	return "Not diff cells.";
}
wxString ctlSQLResult::AutoColsPlot(int flags,frmQuery* parent) {
	wxArrayInt cols;
	wxArrayString leg;
	wxArrayInt colsY;
	wxString rez="Draw plot";
	wxString sss;
	std::vector<long> typeCols;
	if (IsSelection()) {
			unsigned int i;
			int col;
			wxGridCellCoordsArray cells = GetSelectedCells();
			for (i = 0; i < cells.Count(); i++) {
				//clearCellData(cells[i].GetRow(), cells[i].GetCol());
				//cols.Add(cells[i].GetCol());
			}
			wxGridCellCoordsArray topLeft = GetSelectionBlockTopLeft();
			wxGridCellCoordsArray bottomRight = GetSelectionBlockBottomRight();
			for (i = 0; i < topLeft.Count(); i++) {
				//for (row = topLeft[i].GetRow(); row <= bottomRight[i].GetRow(); row++)
				{
					for (col = topLeft[i].GetCol(); col <= bottomRight[i].GetCol(); col++) {
						//clearCellData(row, col);
						wxString aa = wxString::Format(",%d,", col);
						if (sss.Find(aa) != -1) continue;
						sss += aa;
						cols.Add(col);
						typeCols.push_back(colTypClasses.Item(col));
					}
				}
			}
			wxArrayInt cls = GetSelectedCols();
			for (i = 0; i < cls.Count(); i++) {
				if (cols.Index(cls[i]) == wxNOT_FOUND) {
					cols.Add(cls[i]);
					typeCols.push_back(colTypClasses.Item(cls[i]));
				}
//				for (row = 1; row < GetNumberRows(); row++) {
//					clearCellData(row, cols[i]);
//				}
			}
	}
	else { 
		return "not select cells or columns";
	}
	std::vector<wxString> lbar;
	size_t numRows = GetNumberRows();

	if (cols.Count()>0) {
		int idx = 0;
		int cl1 = cols[idx];
		int legC = -1;
		int xC = -1;
		//cols = GetSelectedCols();
		if (cols.Count() == 2 && typeCols[idx]!= PGTYPCLASS_NUMERIC) {
			legC = cl1; idx++; // 1 cols = legend
			cl1 = cols[idx];
			if (typeCols[idx] != PGTYPCLASS_NUMERIC) {
				wxMessageBox("The number of selected column 2 needed Number type\nExample: LY", "Plot");
				return "";
			}
			std::vector<double> x, y;
			double xv=0, yv;
			for (int i = 0; i < numRows; i++)
			{
				if (GetRowSize(i) == 0) continue;
				//str.Append(GetExportLine(i, cols));
				wxString yVal = GetCellValue(i, cl1);
				if (!yVal.ToCDouble(&yv)) { yVal.ToDouble(&yv); /* error! */ }
				x.push_back(xv);
				y.push_back(yv);
				xv++;
				lbar.push_back(GetCellValue(i, legC));
			}
			frmPlot* frame = new frmPlot(parent, "Plot Bar");
			frame->ClearAndSetAxis(ctlSQLGrid::GetColumnName(legC), mpX_NORMAL, ctlSQLGrid::GetColumnName(cl1));
			frame->AddSeries("Bar", x, y, lbar);
			frame->Go();
			return rez;

		}
		if (cols.Count() < 3) {

//			wxMessageBox("The number of selected columns must be more than 1\nExample: LXY or XYYYY...", "Plot");
//			return "";
		}
	}
	else {
					wxMessageBox("The number of selected columns must be more than 0\nExample: LXY or Y or XYYYY...", "Plot");
					return "";

	}
	int idx = 0;
	int cl1 = cols[idx];
	int legC = -1;
	int xC = -1;
	int typeAxisX = mpX_DATETIME;
	// Leg col
	if (typeCols[idx] == PGTYPCLASS_STRING) {
		legC = cl1;
		idx++;
		if (cols.Count() < 2) {
			wxMessageBox("Column 2 must be numeric\nExample: LY or LXY", "Plot");
			return "";
		}
	}
	cl1 = cols[idx];
	// X col
	wxString xA, yA;
	xA = ctlSQLGrid::GetColumnName(cl1);
	if (colTypClasses.Item(cl1) == PGTYPCLASS_DATE) { xC = cl1; idx++; }
		else if (colTypClasses.Item(cl1) == PGTYPCLASS_NUMERIC) { xC = cl1; typeAxisX = mpX_NORMAL; idx++; }
	if (xC == -1) { wxMessageBox("The value type of column X must be a date or a number", "Plot"); return ""; }
	if (cols.GetCount() == 1 && typeCols[0]!= PGTYPCLASS_NUMERIC) {
		wxMessageBox("The value type of column Y must be a number", "Plot");
		return "";
	}
	if (cols.GetCount() == 1 && typeCols[0] == PGTYPCLASS_NUMERIC) {
		// only Y
		xC = -1;
		idx = 0;
		xA = "NumRow";
	}
	// Y cols
	for (size_t col = idx; col < cols.Count(); col++)
	{
		int cl = cols[col];
		if (colTypClasses.Item(cl) == PGTYPCLASS_NUMERIC) {
			if (legC == -1) {
				
			}
			colsY.Add(cl);
			leg.Add(ctlSQLGrid::GetColumnName(cl));
		}
		else {
			wxMessageBox("The values of column Y must be numeric", "Plot");
			return "";
		}
	}
	if (legC != -1)  yA = leg[0]; else yA = "Value";
	//frmQuery* q = &parent;
	frmPlot* frame = new frmPlot(parent,"Plot");
	frame->ClearAndSetAxis(xA, typeAxisX, yA);
	// Rows read
	
	wxString lg,currLg;
	
	if (legC !=-1) {
		// 3 cols
		std::vector<double> x,y;
		int fmttype = -1;
		wxDateTime dt;
		for (int i = 0; i < numRows; i++)
		{
			if (GetRowSize(i) == 0) continue;
			currLg= GetCellValue(i, legC);
			if (lg != currLg && !lg.IsEmpty()) {
				if (x.size()>0) frame->AddSeries(lg, x, y,lbar);
				lg = wxEmptyString;
			}
			if (lg.IsEmpty()) {
				x.clear();
				y.clear();
				lg = currLg;
			}
			//str.Append(GetExportLine(i, cols));
			wxString xVal = GetCellValue(i, xC);
			wxString yVal = GetCellValue(i, colsY[0]);
			double xv, yv;
			//yv=wxAtof(yVal);
			if (!yVal.ToCDouble(&yv)) { yVal.ToDouble(&yv); /* error! */ }
			if (typeAxisX == mpX_DATETIME) {
				dt = parseDT(fmttype, xVal);
				if (fmttype == -1) {
					wxString temp = wxString::Format("The values of column X must be timestamp [row: %d,col: %d]",i+1, colsY[0]);
					rez = temp;
					//wxMessageBox(temp, "Plot");
					continue;
				}
				xv = (double)dt.GetTicks();
			}
			else
			{
				if (!xVal.ToCDouble(&xv)) { xVal.ToDouble(&xv); /* error! */ }
			}
			x.push_back(xv);
			y.push_back(yv);
		}
		frame->AddSeries(lg, x, y,lbar);

	}
	else {
		std::vector<double> x, y;
		int fmttype = -1;
		wxDateTime dt;
		bool first = true;
		if (cols.GetCount() == 1) first = false;
		for (size_t col = 0; col < colsY.Count(); col++)
		{
			lg = leg[col];
			int ccol = colsY[col];
			for (int i = 0; i < numRows; i++)
			{
				if (GetRowSize(i) == 0) continue;
				//str.Append(GetExportLine(i, cols));
				wxString yVal = GetCellValue(i, ccol);
				double xv, yv;
				if (!yVal.ToCDouble(&yv)) { yVal.ToDouble(&yv); /* error! */ }
				if (first) {
					wxString xVal = GetCellValue(i, xC);
					if (typeAxisX == mpX_DATETIME) {
						dt = parseDT(fmttype, xVal);
						if (fmttype == -1) {
							wxString temp = wxString::Format("The values of column X must be timestamp [row: %d,col: %d]", i, colsY[0]);
							wxMessageBox(temp, "Plot");
							return "";
						}
						xv = (double)dt.GetTicks();
					}
					else
					{
						if (!xVal.ToCDouble(&xv)) { xVal.ToDouble(&xv); /* error! */ }
					}
					x.push_back(xv);
				}
				y.push_back(yv);

			}
			if (cols.GetCount() == 1) {
				double nrows = 1;
				for (int i = 0; i < numRows; i++) x.push_back(nrows++);
			}
			frame->AddSeries(lg, x, y,lbar);
			y.clear();
			first = false;
		}
	}
	frame->Go();
	return rez;

}
wxString ctlSQLResult::SummaryColumn()
{
			//ce=cells.Item(0);
		double sum=0;
		size_t i;
	if (GetSelectedRows().GetCount())
	{
		//AppendColumnHeader(str, 0, (GetNumberCols() - 1));

		wxArrayInt rows = GetSelectedRows();

		for (i = 0 ; i < rows.GetCount() ; i++)
		{
			//str.Append(GetExportLine(rows.Item(i)));
			//if (rows.GetCount() > 1)
			//	str.Append(END_OF_LINE);
		}

		//copied = rows.GetCount();
	}
	else if (GetSelectedCols().GetCount())
	{
		wxArrayInt cols = GetSelectedCols();
		size_t numRows = GetNumberRows();

		//AppendColumnHeader(str, cols);
		for (i = 0 ; i < numRows ; i++)
		{
			//str.Append(GetExportLine(i, cols));
				for (size_t col = 0 ; col < cols.Count() ; col++)
			    {
				wxString text = GetCellValue(i, cols[col]);
				if (GetRowSize(i)>0) sum=sum+wxAtof(text);
				}
		
		}

		//copied = numRows;
	}
	else if (GetSelectionBlockTopLeft().GetCount() > 0 &&
	         GetSelectionBlockBottomRight().GetCount() > 0)
	{
		unsigned int x1, x2, y1, y2;

		x1 = GetSelectionBlockTopLeft()[0].GetCol();
		x2 = GetSelectionBlockBottomRight()[0].GetCol();
		y1 = GetSelectionBlockTopLeft()[0].GetRow();
		y2 = GetSelectionBlockBottomRight()[0].GetRow();

		//AppendColumnHeader(str, x1, x2);

		for (i = y1; i <= y2; i++)
	{
//			str.Append(GetExportLine(i, x1, x2));
				wxString text = GetCellValue(i, x1);
				if (GetRowSize(i)>0) sum=sum+wxAtof(text);

		//copied = y2 - y1 + 1;
		}
	}
	else
	{
		int row, col;

		row = GetGridCursorRow();
		col = GetGridCursorCol();

		//AppendColumnHeader(str, col, col);

//		str.Append(GetExportLine(row, col, col));
		//copied = 1;
	}


		wxString result;
	    result.Printf(wxT("%f"), sum);
		return result;
}
void ctlSQLResult::OnKeyUp(wxKeyEvent& event) {
	if (event.GetKeyCode() == WXK_ESCAPE && this->searchStr.Len() > 0) {
		OnKeyChar(event);
		event.Skip(false);
		return;
	}
	if (event.GetKeyCode() == WXK_SPACE && this->searchStr.Len()>0) {
		OnKeyChar(event);
		event.Skip(false);
		return;
	}
	if (event.GetKeyCode() == WXK_RETURN && this->searchStr.Len() > 0) {
		OnKeyChar(event);
		event.Skip(false);
		return;
	}

	event.Skip();
}
void ctlSQLResult::OnKeyChar(wxKeyEvent& event) {
	if (event.GetKeyCode() == WXK_RETURN) {
		sqlResultTable* t = (sqlResultTable*)GetTable();
		if (!(thread && thread->DataValid())) {  return ; }
		//wxString fltval=GetCellValue(row,col);
		wxString text;
		int colS = GetGridCursorCol();
		int rowS= GetGridCursorRow();
		wxString fltval = this->searchStr;
		size_t numRows = GetNumberRows();
		size_t numCols = GetNumberCols();
		int all = 0, show = 0, hide = 0;
		for (size_t i = rowS; i < numRows; i++)
		{
			//str.Append(GetExportLine(i, cols));
			//SetRowSize(i,sizerow);
			if (GetRowSize(i) == 0) continue;
			for (int col = 0; col < numCols; col++)
			{
				if ((t->GetValueFast(i, col).Find(fltval) != wxNOT_FOUND)) {
					MakeCellVisible(i, col);
					SetGridCursor(i, col);
					return;
				}

			}
		}
	}
	if (event.GetKeyCode() == WXK_BACK) {
		this->searchStr=this->searchStr.RemoveLast();
		m_win_s->SetValue(this->searchStr);
		Refresh();
	}
	if (event.GetKeyCode() == WXK_ESCAPE) {
		this->searchStr = "";
		if (m_win_s != NULL) {
			m_win_s->Destroy();
			m_win_s = NULL;
		}
		Refresh();

	}
	else
	{
		wxChar uc = event.GetUnicodeKey();
		if (uc != WXK_NONE) {
			if (uc >= 32) {
				this->searchStr.Append(uc);
				if (this->searchStr.Len() == 1) {
					if (m_win_s == NULL) {
						int colS = GetGridCursorCol();
						wxWindow* t = this->GetGridColLabelWindow();

						wxRect r = CellToRect(0, colS);
						r.y = 0;
						m_win_s = new wxTextCtrl(t, wxID_ANY, "",
							r.GetPosition(),
							r.GetSize(),
							wxTE_PROCESS_ENTER);
						//|wxTE_READONLY
						m_win_s->SetInsertionPointEnd();

					}
				}
				m_win_s->SetValue(this->searchStr);
				Refresh();
			}
			
		}
	}
	event.Skip();
}
void ctlSQLResult::OnGridColSort(wxGridEvent& event)
    {
        const int col = event.GetCol();
        //m_table->Sort(col, !(m_grid->IsSortingBy(col) && m_grid->IsSortOrderAscending()));
    }

void ctlSQLResult::OnGridSelect(wxGridRangeSelectEvent &event)
{
	SetFocus();
}
void ctlSQLResult::ClearFilter()
{
	size_t numRows = GetNumberRows();
	int sizerow=GetDefaultRowSize();
		for (size_t i = 0 ; i <  numRows; i++)
		{
				if (GetRowSize(i)>0) continue;
				//SetRowSize(i,sizerow);
				ShowRow(i);
		
		}
	SetGridLineColour(cg);
}
wxString ctlSQLResult::SetFilter(int row,int col,bool reverse)
{
	wxString result=wxEmptyString;
	sqlResultTable *t=(sqlResultTable *)GetTable();
	if (!(thread && thread->DataValid())) {result.Printf(wxT("Error thread not valid"));return result; }
	//wxString fltval=GetCellValue(row,col);
	wxString fltval=t->GetValueFast(row,col);
	wxString text;
	
	bool eq;
	size_t numRows = GetNumberRows();
	int all=0,show=0,hide=0;
		for (size_t i = 0 ; i <  numRows; i++)
		{
			//str.Append(GetExportLine(i, cols));
			//SetRowSize(i,sizerow);
				if (GetRowSize(i)==0) continue;
				
				eq=(fltval==t->GetValueFast(i,col));
				if (reverse) eq=!eq;
				if (!eq) { 
					HideRow(i);
					hide++;
				} else show++;
				all++;
		
		}
	SetGridLineColour(wxColor(0,0,255));
	result.Printf(wxT("Show rows:%d hide:%d all:%d"), show,hide,all);
	return result;

}
#include <map>
int sqlResultTable::sortColumns()
{
    bool no_sort=colsortnumber[0]==-1;
	if (!maplines) {
		maplines = new int[GetNumberRows()];
		for(int i=0;i<GetNumberRows();i++) maplines[i]=i;
	}
	if (no_sort) {
		for(int i=0;i<GetNumberRows();i++) maplines[i]=i;
		use_map=true;
		return 0;
	}
	use_map=false;
	int *cols[MAX_COL_SORT];
	for (int i=0;i<MAX_COL_SORT;i++) cols[i]=NULL;
	int col;

			 for (int k=0;k<MAX_COL_SORT;k++) {
				 col=colsortnumber[k];
				 if (col!=-1) {
						if (thread && thread->DataSet()->ColTypClass(col) == PGTYPCLASS_NUMERIC)
						{
							//sort numeric column
								 std::multimap<double, int> mp;
								 double d;
								  for (int i=0;i<GetNumberRows();i++) {
									  thread->DataSet()->Locate(i+1);
									  d=thread->DataSet()->GetDouble(col);
									  mp.insert(std::pair<double, int>(d, i));
								  }
								 std::multimap<double, int>::iterator it = mp.begin();
								 double prev;
								 int row;
								 int minval=0;
								 cols[k] = new int[GetNumberRows()];
								   for (int i = 0; it != mp.end(); it++, i++) {  // выводим их
									   row=it->second; // row
									   if (i>0) if (prev!=it->first) minval++;
									   cols[k][row]=minval;
									   prev=it->first;
									}
						} else {
							//no numeric sort as string
								 std::multimap<wxString, int> mp;
								 wxString s;
								  for (int i=0;i<GetNumberRows();i++) {
									  thread->DataSet()->Locate(i+1);
									  s=thread->DataSet()->GetVal(col);
									  mp.insert(std::pair<wxString, int>(s, i));
								  }
								 std::multimap<wxString, int>::iterator it = mp.begin();
								 wxString prev;
								 int row;
								 int minval=0;
								 cols[k] = new int[GetNumberRows()];
								   for (int i = 0; it != mp.end(); it++, i++) {  
									   row=it->second; // row
									   if (i>0) if (prev!=it->first) minval++;
									   cols[k][row]=minval;
									   prev=it->first;
									}
						}
				 } else  break;
			 }
    
	class sorter {
		 int **cols;
		 sqlResultTable *m;
	public:
		 sorter(int *_cols[], sqlResultTable *my_obj) { cols=_cols;m=my_obj;}
		 bool operator() (int i,int j) { 
			 bool rez=true;
			 int a;
			 int b;
			 int col,ord;
			 for (int k=0;k<MAX_COL_SORT;k++) {
				 col=m->colsortnumber[k];
				 if (col!=-1) {
					 ord=m->colorder[col];
					 if (ord==0) continue;
					 a=cols[k][i];
					 b=cols[k][j];
					 if (a!=b) {
						 rez=a<b;
					 } else
					 {
						 rez=false;
						 continue;
					 }
					 if (ord==-1) rez=!rez;
				 }
				 break;
			 }
			 return rez;
		 }
	};
	std::sort(maplines,maplines+GetNumberRows(),sorter(cols,this));
	for (int i=0;i<MAX_COL_SORT;i++) if (cols[i]!=NULL) delete cols[i];
	use_map=true;
	return 0;
}
int sqlResultTable::getSortColumn(int col)
{
	return colorder[col];
}
int sqlResultTable::setSortColumn(int col)
{

	if (col<GetNumberCols()) {
	  if (colorder[col]==0) colorder[col]=1;
		 else if (colorder[col]==1) colorder[col]=-1;
		  else if (colorder[col]==-1) colorder[col]=0; // no sort
	  int pos=-1,poszero=-1;
	  for (int i=0;i<MAX_COL_SORT;i++)  if (colsortnumber[i]==col) {pos=i; break;}
	                                    else if (colsortnumber[i]==-1) { poszero=i; break;}  
      if (pos==-1 && poszero==-1) { colorder[col]=0; return 0;} // limit sort columns 
	  if (pos==-1 && poszero!=-1) colsortnumber[poszero]=col;  // add sort column
	  if (colorder[col]==0 && pos!=-1) {
		  // shift array (remove sort for column)
		  for (int i=pos;i<MAX_COL_SORT-1;i++) colsortnumber[i]=colsortnumber[i+1];
		  colsortnumber[MAX_COL_SORT-1]=-1;
	  }
	  sortColumns();
	  return colorder[col];
	}
	return 0;
}
int sqlResultTable::initSort()
{
	if (colorder) delete [] colorder;
	if (maplines) delete [] maplines;
	maplines=NULL;
	colorder = new int[GetNumberCols()];
	for (int i=0;i<GetNumberCols();i++) colorder[i]=0;
	for (int i=0;i<MAX_COL_SORT;i++) colsortnumber[i]=-1;
	//maplines = new int[GetNumberRows()];
	use_map=false;
	return 0;
}
wxString sqlResultTable::GetValueFast(int row, int col)
{
	wxString s;
	if (thread && thread->DataValid())
	{
		if (col >= 0)
		{
			if (use_map) row=maplines[row];
			thread->DataSet()->Locate(row + 1);
			wxString s = thread->DataSet()->GetVal(col);
			return s;
		}

	}
	return "";
}
wxString sqlResultTable::GetValue(int row, int col)
{
	if (thread && thread->DataValid())
	{
		if (col >= 0)
		{
			if (use_map) row=maplines[row];
			thread->DataSet()->Locate(row + 1);
			if (c_IndicateNull && thread->DataSet()->IsNull(col))
				return wxT("<NULL>");
			else
			{

				wxString decimalMark = wxT(".");
				wxString s = thread->DataSet()->GetVal(col);

				if(thread->DataSet()->ColTypClass(col) == PGTYPCLASS_NUMERIC &&
				        !c_DecimalMark.IsEmpty())
				{
					decimalMark = settings->GetDecimalMark();
					s.Replace(wxT("."), decimalMark);

				}
				if (thread->DataSet()->ColTypClass(col) == PGTYPCLASS_NUMERIC &&
				        !c_ThousandsSeparator.IsEmpty())
				{
					/* Add thousands separator */
					size_t pos = s.find(decimalMark);
					if (pos == wxString::npos)
						pos = s.length();
					while (pos > 3)
					{
						pos -= 3;
						if (pos > 1 || !s.StartsWith(wxT("-")))
							s.insert(pos, c_ThousandsSeparator);
					}
					return s;
				}
				else
				{
					wxString data = thread->DataSet()->GetVal(col);

					if (data.Length() > (size_t) c_MaxColSize)
						return thread->DataSet()->GetVal(col).Left(c_MaxColSize) + wxT(" (...)");
					else
						return thread->DataSet()->GetVal(col);
				}
			}
		}
		else
			return thread->DataSet()->ColName(col);
	}
	return wxEmptyString;
}

sqlResultTable::sqlResultTable()
{
	thread = NULL;
	colorder=NULL;
	maplines=NULL;
	use_map=false;
	c_MaxColSize = settings->GetMaxColSize();
	c_IndicateNull=settings->GetIndicateNull();
	c_ThousandsSeparator = settings->GetThousandsSeparator();
	c_DecimalMark = settings->GetDecimalMark();
}

int sqlResultTable::GetNumberRows()
{
	if (thread && thread->DataValid())
		return thread->DataSet()->NumRows();
	return 0;
}

wxString sqlResultTable::GetRowLabelValue( int row )
{
    if ( isplan )
    {
        // using default label
        //
        return wxGridStringTable::GetRowLabelValue(row);
    }
    else
    {
		    wxString s;
			s << row + 1;
		return s;
    }
}

wxString sqlResultTable::GetColLabelValue(int col)
{
	if (thread && thread->DataValid())
		return thread->DataSet()->ColName(col) + wxT("\n") +
		       thread->DataSet()->ColFullType(col);
	return wxEmptyString;
}

int sqlResultTable::GetNumberCols()
{
	if (thread && thread->DataValid())
		return thread->DataSet()->NumCols();
	return 0;
}

