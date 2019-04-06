//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// frmExport.cpp - The export file dialogue
//
//////////////////////////////////////////////////////////////////////////



// App headers
#include "pgAdmin3.h"
#include <wx/file.h>
#include "frm/frmExport.h"
#include "utils/sysSettings.h"
#include "utils/misc.h"
#include "ctl/ctlSQLResult.h"
#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/msw/ole/automtn.h>

#define txtFilename     CTRL_TEXT("txtFilename")
#define btnOK           CTRL_BUTTON("wxID_OK")
#define rbUnicode       CTRL_RADIOBUTTON("rbUnicode")
#define rbLocal         CTRL_RADIOBUTTON("rbLocal")
#define rbCRLF          CTRL_RADIOBUTTON("rbCRLF")
#define rbLF            CTRL_RADIOBUTTON("rbLF")
#define rbQuoteStrings  CTRL_RADIOBUTTON("rbQuoteStrings")
#define rbQuoteAll      CTRL_RADIOBUTTON("rbQuoteAll")
#define rbQuoteNone     CTRL_RADIOBUTTON("rbQuoteNone")
#define chkColnames     CTRL_CHECKBOX("chkColnames")
#define chkXlsCopy      CTRL_CHECKBOX("chkXlsCopy")
#define cbColSeparator  CTRL_COMBOBOX("cbColSeparator")
#define cbQuoteChar     CTRL_COMBOBOX("cbQuoteChar")


BEGIN_EVENT_TABLE(frmExport, pgDialog)
	EVT_TEXT(XRCID("txtFilename"),          frmExport::OnChange)
	EVT_RADIOBUTTON(XRCID("rbQuoteNone"),   frmExport::OnChange)
	EVT_RADIOBUTTON(XRCID("rbQuoteStrings"), frmExport::OnChange)
	EVT_RADIOBUTTON(XRCID("rbQuoteAll"),    frmExport::OnChange)
	EVT_BUTTON(XRCID("btnFilename"),        frmExport::OnBrowseFile)
	EVT_BUTTON(wxID_HELP,                   frmExport::OnHelp)
	EVT_BUTTON(wxID_OK,                     frmExport::OnOK)
	EVT_BUTTON(wxID_CANCEL,                 frmExport::OnCancel)
END_EVENT_TABLE()

class XMLDataFormat : public wxDataFormat
{
public:
    XMLDataFormat() : wxDataFormat(wxT("XML Spreadsheet")) {}
};
class XMLDataObject : public wxDataObjectSimple
{

public:
    XMLDataObject(const wxString& xmlstring = wxEmptyString) : wxDataObjectSimple(), m_XMLString(xmlstring)
    {
        SetFormat(XMLDataFormat());
    }


    size_t GetLength() const { return m_XMLString.Len()*2 + 1; }
    wxString GetXML() const { return m_XMLString; }
    void SetXML(const wxString& xml) { m_XMLString = xml; }

    size_t GetDataSize() const
    {
        return GetLength();
    }

    bool GetDataHere(void *buf) const
    {
        //char* c = _strdup(m_XMLString.c_str()); not needed as per suggestion

        //memcpy(buf, m_XMLString.c_str(), m_XMLString.Len()*2+1);
		
		memcpy(buf,m_XMLString.mb_str(wxConvUTF8),m_XMLString.Len()*2+1);
        return true;
    }

    bool SetData(size_t len, const void* buf)
    {
        //char* c = new char[len + 1]; not needed as per suggestion
        //memcpy(c, buf, len); not needed as per suggestion
        m_XMLString = wxString::FromUTF8((const char*)buf); //changed as per suggestion

        //delete c;

        return true;
    }

    virtual size_t GetDataSize(const wxDataFormat&) const 
    {
        return GetDataSize();
    }
    virtual bool GetDataHere(const wxDataFormat&, void *buf) const 
    {
        return GetDataHere(buf);
    }
    virtual bool SetData(const wxDataFormat&, size_t len, const void *buf) 
    {
        return SetData(len, buf);
    }


private:
    wxString m_XMLString;
};


frmExport::frmExport(wxWindow *p)
{
	parent = p;

	SetFont(settings->GetSystemFont());
	LoadResource(p, wxT("frmExport"));
	RestorePosition();

	// Icon
	appearanceFactory->SetIcons(this);
	cbQuoteChar->Disable();
	btnOK->Disable();


	bool uc = settings->GetExportUnicode();
	rbUnicode->SetValue(uc);
	rbLocal->SetValue(!uc);

	bool isCrLf = settings->GetExportRowSeparator() == wxT("\r\n");
	rbCRLF->SetValue(isCrLf);
	rbLF->SetValue(!isCrLf);

	int qt = settings->GetExportQuoting();

	rbQuoteNone->SetValue(qt == 0);
	rbQuoteStrings->SetValue(qt == 1);
	rbQuoteAll->SetValue(qt == 2);

	cbColSeparator->SetValue(settings->GetExportColSeparator());


	cbQuoteChar->SetValue(settings->GetExportQuoteChar());

	wxString val;
	settings->Read(wxT("Export/LastFile"), &val, wxEmptyString);
	txtFilename->SetValue(val);

	wxCommandEvent ev;
	OnChange(ev);
}


frmExport::~frmExport()
{
	SavePosition();
}


void frmExport::OnHelp(wxCommandEvent &ev)
{
	DisplayHelp(wxT("export"), HELP_PGADMIN);
}


void frmExport::OnChange(wxCommandEvent &ev)
{
	cbQuoteChar->Enable(rbQuoteStrings->GetValue() || rbQuoteAll->GetValue());
	btnOK->Enable(!txtFilename->GetValue().IsEmpty() && !cbColSeparator->GetValue().IsEmpty());
}


void frmExport::OnOK(wxCommandEvent &ev)
{
	settings->SetExportUnicode(rbUnicode->GetValue());
	settings->SetExportRowSeparator(rbCRLF->GetValue() ? wxT("\r\n") : wxT("\n"));
	settings->SetExportColSeparator(cbColSeparator->GetValue());

	if (rbQuoteAll->GetValue())
		settings->SetExportQuoting(2);
	else if (rbQuoteStrings->GetValue())
		settings->SetExportQuoting(1);
	else
		settings->SetExportQuoting(0);

	settings->SetExportQuoteChar(cbQuoteChar->GetValue());

	settings->Write(wxT("Export/LastFile"), txtFilename->GetValue());


	if (IsModal())
		EndModal(wxID_OK);
	else
		Destroy();
}

wxString frmExport::InitXml(int cols, int rows)
{
	wxString strc;
	wxString strr;
	
	strc.Printf(wxT("%d"),cols);
	strr.Printf(wxT("%d"),rows);

	wxString s= wxT("<?xml version=\"1.0\"?>\n")
		wxT("<?mso-application progid=\"Excel.Sheet\"?>\n")
		wxT("<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n")
		wxT(" xmlns:o=\"urn:schemas-microsoft-com:office:office\"\n")
		wxT("xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n")
		wxT("xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n")
		wxT("xmlns:html=\"http://www.w3.org/TR/REC-html40\">\n")
		wxT("<Styles>\n")
		wxT("<Style ss:ID=\"def\">\n")
		wxT("<Borders>\n")
		wxT("<Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("</Borders></Style>\n")
		wxT("<Style ss:ID=\"hdr\">\n")
		wxT("<Borders>\n")
		wxT("<Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("</Borders><Font ss:FontName=\"Arial Cyr\" x:CharSet=\"204\" ss:Bold=\"1\"/></Style>\n")
		wxT("<Style ss:ID=\"dt\">\n")
		wxT("<Borders>\n")
		wxT("<Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("</Borders><NumberFormat ss:Format=\"General Date\"/></Style>\n")
		wxT("<Style ss:ID=\"null\">\n")
		wxT("<Borders>\n")
		wxT("<Border ss:Position=\"Bottom\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Left\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Right\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("<Border ss:Position=\"Top\" ss:LineStyle=\"Continuous\" ss:Weight=\"1\"/>\n")
		wxT("</Borders><Interior ss:Color=\"#CCFFCC\" ss:Pattern=\"Solid\"/></Style>\n")
		wxT("</Styles>\n")
		wxT("<Worksheet ss:Name=\"data\">\n")
		wxT("<Table ss:ExpandedColumnCount=\"")+strc+wxT("\" ss:ExpandedRowCount=\"")+strr+wxT("\">\n")
		wxT("\n")
		;
	for (int c = 0 ; c < cols ; c++)
	{
		s+=wxT("<Column ss:AutoFitWidth=\"1\"/>\n");
	}
	return s;
}
wxString textEscapeXml(wxString &text) 
{
						wxString::const_iterator i;
						wxString xmltext=wxEmptyString;
					for ( i = text.begin(); i != text.end(); ++i )
					{
#if wxCHECK_VERSION(3, 0, 0)
					char c=*i;
					switch (c)
#else
					switch (*i)
#endif
					{
						case '&': xmltext+= wxT("&amp;"); break;
						case '\'': xmltext+= wxT("&apos;"); break;
						case '"': xmltext+= wxT("&quot;"); break;
						case '<': xmltext+= wxT("&lt;"); break;
						case '>': xmltext+= wxT("&gt;"); break;
						case 13: break;
						case 10: xmltext+= wxT("&#10;"); break;
						default: xmltext+= *i; break;
					}
					}
return xmltext;
}
bool frmExport::ExportXls(ctlSQLResult *grid)
{
	int colCount, rowCount;
	int col;
	wxString line;
	long skipped = 0;
	wxWX2MBbuf buf;
	colCount = grid->GetNumberCols();
	rowCount = grid->NumRows();
	line=InitXml(colCount,rowCount+1);
    wxAutomationObject excelObject; 
		line += wxT("<Row>\n");
		for (col = 0 ; col < colCount ; col++)
		{
			line += wxT("<Cell ss:StyleID=\"hdr\"><Data ss:Type=\"String\">")+grid->OnGetItemText(-1, col + 1).BeforeFirst('\n')+wxT("</Data></Cell>");
		//wxString adr(wxT(""));
		//adr.Printf(wxT("%s%d"), name,1);
		//rng[0] = wxVariant(adr); 
		//excelObject.GetObject(range, wxT("Range"), 1,rng); 
		//range.CallMethod(wxT("Activate")); 
		//excelObject.PutProperty(wxT("ActiveCell.Font.Bold"), true);
		//excelObject.PutProperty(wxT("ActiveCell.Value"), grid->OnGetItemText(-1, col + 1).BeforeFirst('\n')); 
		//name[0]++;
		}
		line += wxT("</Row>\n");
	int row;
	wxString text;
	wxString type;
	OID typOid;
	wxString fn=txtFilename->GetValue().BeforeLast('.');
	if (fn.IsEmpty()) fn=txtFilename->GetValue()+wxT(".xml"); 
	else fn+=wxT(".xml");

		wxFile file(fn, wxFile::write);
	if (!file.IsOpened())
	{
		wxLogError(__("Failed to open file %s."), fn.c_str());
		return false;
	}
	file.Write(line, wxConvUTF8);
	wxString xmltext=wxEmptyString;
	for (row = 0 ; row < rowCount ; row++)
	{
		
		line = wxT("<Row>\n");
		for (col = 0 ; col < colCount ; col++)
		{
			wxString style(wxT("null"));
			//adr.Printf(wxT("%s%d"), name,row+2);
			//rng[0] = wxVariant(adr); 
			//excelObject.GetObject(range, wxT("Range"), 1,rng); 
			//range.CallMethod(wxT("Activate")); 
			text=grid->OnGetItemText(row, col + 1);
			xmltext=wxEmptyString;
			typOid = grid->colTypClasses[col];
			if (!text.IsEmpty()) {
				style=wxT("def");
				xmltext=textEscapeXml(text);
	// find out if string
				switch (typOid)
				{
					case PGTYPCLASS_NUMERIC:
						//type=wxT("0.00");
						if (xmltext.IsNumber())
								xmltext=wxT("<Cell ss:StyleID=\"")+style+wxT("\"><Data ss:Type=\"Number\">")+xmltext+wxT("</Data></Cell>");
							else
								xmltext=wxT("<Cell ss:StyleID=\"")+style+wxT("\"><Data ss:Type=\"String\">")+xmltext+wxT("</Data></Cell>");
						break;
					case PGTYPCLASS_DATE:
						//type=wxT("ÃÃÃÃ-ÌÌ-ÄÄ ÷÷:ìì:ññ");
						xmltext=xmltext.BeforeFirst('+');
						if (xmltext.Replace(wxT(" "),wxT("T"))==0) {
							xmltext=wxT("<Cell ss:StyleID=\"")+style+wxT("\"><Data ss:Type=\"String\">")+xmltext+wxT("</Data></Cell>");
						} else 
						{
							style=wxT("dt");
							xmltext=wxT("<Cell ss:StyleID=\"")+style+wxT("\"><Data ss:Type=\"DateTime\">")+xmltext+wxT("</Data></Cell>");
						}
						break;
					case PGTYPCLASS_BOOL:
					default:
						//type=wxT("@");
						xmltext=wxT("<Cell ss:StyleID=\"")+style+wxT("\"><Data ss:Type=\"String\">")+xmltext+wxT("</Data></Cell>");
						break;
				}

			}	else xmltext=wxT("<Cell ss:StyleID=\"null\"/>");
		//	excelObject.PutProperty(wxT("ActiveCell.Borders.LineStyle"), 1);
		//		excelObject.PutProperty(wxT("ActiveCell.NumberFormat"),type);
		//	excelObject.PutProperty(wxT("ActiveCell.Value"), text); 
		//	name[0]++;
			line += xmltext+wxT("\n");
		}
		line += wxT("</Row>\n");
		file.Write(line, wxConvUTF8);
	}
	line = wxT("</Table></Worksheet>");
	file.Write(line, wxConvUTF8);
	text=textEscapeXml(grid->sqlquerytext);
	line= wxT("<Worksheet ss:Name=\"SQL text\">\n")
		  wxT("<Table ss:ExpandedColumnCount=\"1\" ss:ExpandedRowCount=\"1\">\n")
		  wxT("<Column ss:Width=\"300\"/><Row ss:AutoFitHeight=\"1\">\n")
		  wxT("<Cell><Data ss:Type=\"String\">")+text+
		  wxT("\n</Data></Cell></Row></Table></Worksheet>")
		  wxT("</Workbook>\n")
		  ;
	file.Write(line, wxConvUTF8);
	file.Close();
	if (excelObject.CreateInstance(wxT("Excel.Application"))) {
		 //excelObject.GetObject(xlbook,wxT("Workbooks.Add"));
		excelObject.CallMethod(wxT("Workbooks.Open"), fn);
	excelObject.PutProperty(_T("VISIBLE"), true);
	return true;
	}
	return false;
	 
}

bool frmExport::Export(pgSet *set)
{
	ctlSQLResult *grid = 0;
	if (!set)
	{
		wxLogInfo(wxT("Exporting data from the grid"));
		grid = (ctlSQLResult *)parent;
		if (chkXlsCopy->GetValue()) {
			return ExportXls(grid);
		}
		
	}
	else
		wxLogInfo(wxT("Exporting data from a resultset"));

	wxFile file(txtFilename->GetValue(), wxFile::write);
	if (!file.IsOpened())
	{
		wxLogError(__("Failed to open file %s."), txtFilename->GetValue().c_str());
		return false;
	}

	wxString line;
	long skipped = 0;
	wxWX2MBbuf buf;

	int colCount, rowCount;

	if (set)
	{
		colCount = set->NumCols();
		rowCount = set->NumRows();
	}
	else
	{
		colCount = grid->GetNumberCols();
		rowCount = grid->NumRows();
	}

	int col;
	if (chkColnames->GetValue())
	{
		for (col = 0 ; col < colCount ; col++)
		{
			if (!col)
				line = wxEmptyString;
			else
				line += cbColSeparator->GetValue();

			if (rbQuoteStrings->GetValue() || rbQuoteAll->GetValue())
			{
				wxString qc = cbQuoteChar->GetValue();

				wxString hdr;
				if (set)
					hdr = set->ColName(col);
				else
					hdr = grid->OnGetItemText(-1, col + 1).BeforeFirst('\n');

				hdr.Replace(qc, qc + qc);
				line += qc + hdr + qc;
			}
			else
			{
				if (set)
					line += set->ColName(col);
				else
					line += grid->OnGetItemText(-1, col + 1).BeforeFirst('\n');
			}
		}
		if (rbCRLF->GetValue())
			line += wxT("\r\n");
		else
			line += wxT("\n");

		if (rbUnicode->GetValue())
			file.Write(line, wxConvUTF8);
		else
		{
			buf = line.mb_str(wxConvLibc);
			if (!buf)
				skipped++;
			else
				file.Write(line, wxConvLibc);
		}
	}


	wxString text;
	OID typOid;

	int row;
	for (row = 0 ; row < rowCount ; row++)
	{
		for (col = 0 ; col < colCount ; col++)
		{
			if (!col)
				line = wxEmptyString;
			else
				line += cbColSeparator->GetValue();

			bool needQuote = rbQuoteAll->GetValue();

			if (set)
			{
				text = set->GetVal(col);
				typOid = set->ColTypClass(col);
			}
			else
			{
				text = grid->OnGetItemText(row, col + 1);
				typOid = grid->colTypClasses[col];
			}

			if (!needQuote && rbQuoteStrings->GetValue())
			{
				// find out if string
				switch (typOid)
				{
					case PGTYPCLASS_NUMERIC:
					case PGTYPCLASS_BOOL:
						break;
					default:
						needQuote = true;
						break;
				}
			}
			if (needQuote)
			{
				wxString qc = cbQuoteChar->GetValue();
				text.Replace(qc, qc + qc);
				line += qc + text + qc;
			}
			else
				line += text;
		}
		if (rbCRLF->GetValue())
			line += wxT("\r\n");
		else
			line += wxT("\n");

		if (rbUnicode->GetValue())
			file.Write(line, wxConvUTF8);
		else
		{
			buf = line.mb_str(wxConvLibc);
			if (!buf)
				skipped++;
			else
				file.Write(line, wxConvLibc);
		}

		if (set)
			set->MoveNext();
	}
	file.Close();

	if (skipped)
		wxLogError(wxPLURAL(
		               "Data export incomplete.\n\n%d row contained characters that could not be converted to the local charset.\n\nPlease correct the data or try using UTF8 instead.",
		               "Data export incomplete.\n\n%d rows contained characters that could not be converted to the local charset.\n\nPlease correct the data or try using UTF8 instead.",
		               skipped), skipped);
	else
		wxMessageBox(_("Data export completed successfully."), _("Export data"), wxICON_INFORMATION | wxOK);

	return true;
}


void frmExport::OnCancel(wxCommandEvent &ev)
{
	if (IsModal())
		EndModal(wxID_CANCEL);
	else
		Destroy();
}

void frmExport::OnBrowseFile(wxCommandEvent &ev)
{
	wxString directory;
	wxString filename;

	if (txtFilename->GetValue().IsEmpty())
		directory = wxGetHomeDir();
	else
	{
		directory = wxFileName(txtFilename->GetValue()).GetPath();
		filename = wxFileName(txtFilename->GetValue()).GetFullName();
	}

#ifdef __WXMSW__
	wxFileDialog file(this, _("Select export filename"), directory, filename,
	                  _("CSV files (*.csv)|*.csv|Data files (*.dat)|*.dat|All files (*.*)|*.*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
#else
	wxFileDialog file(this, _("Select export filename"), directory, filename,
	                  _("CSV files (*.csv)|*.csv|Data files (*.dat)|*.dat|All files (*)|*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
#endif

	if (file.ShowModal() == wxID_OK)
	{
		txtFilename->SetValue(file.GetPath());
		OnChange(ev);
	}
}
