//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// ctlSQLResult.h - SQL Query result window
//
//////////////////////////////////////////////////////////////////////////

#ifndef CTLSQLRESULT_H
#define CTLSQLRESULT_H

// wxWindows headers
#include <wx/thread.h>

#include "db/pgSet.h"
#include "db/pgConn.h"
#include "ctlSQLGrid.h"
#include "frm/frmExport.h"
#include "frm/frmQuery.h"

#define CTLSQL_RUNNING 100  // must be greater than ExecStatusType PGRES_xxx values

class ctlSQLResult : public ctlSQLGrid
{
public:
	ctlSQLResult(wxWindow *parent, pgConn *conn, wxWindowID id, const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize);
	~ctlSQLResult();


	int Execute(const wxString &query, int resultToDisplay = 0, wxWindow *caller = 0, long eventId = 0, void *data = 0); // > 0: resultset to display, <=0: last result
	void SetConnection(pgConn *conn);
	long NumRows() const;
	long InsertedCount() const;
	OID  InsertedOid() const;
	wxColor cg;
	int Abort();

	bool Export();
	bool ToFile();
	bool ToFile(frmExport *frm);
	bool CanExport()
	{
		return NumRows() > 0 && colNames.GetCount() > 0;
	}

	wxString OnGetItemText(long item, long col) const;
	wxString SummaryColumn();
	wxString AutoColsPlot(int flags,frmQuery *parent);
	wxString CheckSelColumnDate();
	void ClearFilter();
	bool IsColText(int col);
	bool hasRowNumber()
	{
		return !rowcountSuppressed;
	}

	int RunStatus();
	wxString GetMessagesAndClear();
	wxString GetErrorMessage();
	pgError GetResultError();

	void DisplayData(bool single = false);

	bool GetRowCountSuppressed()
	{
		return rowcountSuppressed;
	};

	void SetMaxRows(int rows);
	void ResultsFinished();
	void OnGridSelect(wxGridRangeSelectEvent &event);
	void OnGridColSort(wxGridEvent& event);
	wxString SetFilter(int row,int col,bool reverse);

	wxArrayString colNames;
	wxArrayString colTypes;
	wxArrayLong colTypClasses;
private:
	pgQueryThread *thread;
	pgConn *conn;
	bool rowcountSuppressed;
};

class sqlResultTable : public wxGridStringTable//wxGridTableBase
{
public:
	sqlResultTable();
	wxString GetValue(int row, int col);
    wxString GetValueFast(int row, int col);
	wxString GetRowLabelValue( int row ) ;
	int GetNumberRows();
	int GetNumberCols();
	bool isplan;
	bool use_map;    // use maplines for GetValue
	int *maplines;	// maplines[visible_row]=real_row
	int *colorder;  // array order type -1 desc , 0 no sort, 1 asc
#define MAX_COL_SORT 5
	int colsortnumber[MAX_COL_SORT];
	int sortColumns(); // 
	int setSortColumn(int col); // set order for column (cycle change value -1,0,1)
	int getSortColumn(int col);
	int initSort(); // allocate memory
	bool IsEmptyCell(int row, int col)
	{
		return false;
	}
	wxString GetColLabelValue(int col);
	void SetValue(int row, int col, const wxString &value)
	{
		return;
	}
	void SetThread(pgQueryThread *t)
	{
		thread = t;
	}
	bool DeleteRows(size_t pos = 0, size_t numRows = 1)
	{
		return true;
	}
	bool DeleteCols(size_t pos = 0, size_t numCols = 1)
	{
		return true;
	}
	
private:
	pgQueryThread *thread;
};

#endif
