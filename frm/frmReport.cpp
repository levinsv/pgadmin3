//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// frmReport.cpp - The report file dialogue
//
//////////////////////////////////////////////////////////////////////////



// App headers
#include "pgAdmin3.h"
#include <wx/file.h>
#include <wx/regex.h>

#include "frm/frmMain.h"
#include "frm/frmReport.h"
#include "utils/sysSettings.h"
#include "utils/misc.h"
#include "ctl/ctlListView.h"
#include "schema/pgObject.h"
#include "schema/pgCollection.h"
#include "schema/pgServer.h"
#include "schema/pgDatabase.h"
#include "schema/pgSchema.h"
#include "schema/pgTable.h"
#include "schema/pgColumn.h"
#include "schema/pgConstraints.h"
#include "agent/pgaJob.h"
#include "schema/pgForeignKey.h"
#include "schema/pgIndexConstraint.h"
#include "schema/pgCheck.h"
#include "utils/utffile.h"
#include <wx/stdpaths.h>
#include "../utils/diff_match_patch.h"
// XML2/XSLT headers
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#define txtTitle          CTRL_TEXT("txtTitle")
#define txtNotes          CTRL_TEXT("txtNotes")
#define txtHtmlFile       CTRL_TEXT("txtHtmlFile")
#define txtXmlFile        CTRL_TEXT("txtXmlFile")
#define txtHtmlStylesheet CTRL_TEXT("txtHtmlStylesheet")
#define txtXmlStylesheet  CTRL_TEXT("txtXmlStylesheet")
#define btnOK             CTRL_BUTTON("wxID_OK")
#define btnFile           CTRL_BUTTON("btnFile")
#define btnStylesheet     CTRL_BUTTON("btnStylesheet")
#define rbHtml            CTRL_RADIOBUTTON("rbHtml")
#define rbXml             CTRL_RADIOBUTTON("rbXml")
#define rbHtmlBuiltin     CTRL_RADIOBUTTON("rbHtmlBuiltin")
#define rbHtmlEmbed       CTRL_RADIOBUTTON("rbHtmlEmbed")
#define rbHtmlLink        CTRL_RADIOBUTTON("rbHtmlLink")
#define rbXmlPlain        CTRL_RADIOBUTTON("rbXmlPlain")
#define rbXmlLink         CTRL_RADIOBUTTON("rbXmlLink")
#define rbXmlProcess      CTRL_RADIOBUTTON("rbXmlProcess")
#define chkSql            CTRL_CHECKBOX("chkSql")
#define chkBrowser        CTRL_CHECKBOX("chkBrowser")

BEGIN_EVENT_TABLE(frmReport, pgDialog)
	EVT_RADIOBUTTON(XRCID("rbHtml"),        frmReport::OnChange)
	EVT_RADIOBUTTON(XRCID("rbXml"),         frmReport::OnChange)
	EVT_RADIOBUTTON(XRCID("rbHtmlBuiltin"), frmReport::OnChange)
	EVT_RADIOBUTTON(XRCID("rbHtmlEmbed"),   frmReport::OnChange)
	EVT_RADIOBUTTON(XRCID("rbHtmlLink"),    frmReport::OnChange)
	EVT_RADIOBUTTON(XRCID("rbXmlPlain"),    frmReport::OnChange)
	EVT_RADIOBUTTON(XRCID("rbXmlLink"),     frmReport::OnChange)
	EVT_RADIOBUTTON(XRCID("rbXmlProcess"),  frmReport::OnChange)
	EVT_TEXT(XRCID("txtHtmlFile"),          frmReport::OnChange)
	EVT_TEXT(XRCID("txtXmlFile"),           frmReport::OnChange)
	EVT_TEXT(XRCID("txtHtmlStylesheet"),    frmReport::OnChange)
	EVT_TEXT(XRCID("txtXmlStylesheet"),     frmReport::OnChange)
	EVT_BUTTON(XRCID("btnFile"),            frmReport::OnBrowseFile)
	EVT_BUTTON(XRCID("btnStylesheet"),      frmReport::OnBrowseStylesheet)
	EVT_BUTTON(wxID_HELP,                   frmReport::OnHelp)
	EVT_BUTTON(wxID_OK,                     frmReport::OnOK)
	EVT_BUTTON(wxID_CANCEL,                 frmReport::OnCancel)
END_EVENT_TABLE()

frmReport::frmReport(wxWindow *p)
{
	parent = p;
	header = wxT("");

	SetFont(settings->GetSystemFont());
	LoadResource(p, wxT("frmReport"));

	// Icon
	appearanceFactory->SetIcons(this);
	RestorePosition();
	btnOK->Disable();

	wxString val;
	bool bVal;

	// Output format
	settings->Read(wxT("Reports/ReportFormat"), &val, wxT("h"));
	if (val == wxT("x"))
	{
		rbHtml->SetValue(false);
		rbXml->SetValue(true);
	}
	else
	{
		rbHtml->SetValue(true);
		rbXml->SetValue(false);
	}

	// HTML Stylesheet
	settings->Read(wxT("Reports/HtmlStylesheetMode"), &val, wxT("b"));
	if (val == wxT("e"))
	{
		rbHtmlBuiltin->SetValue(false);
		rbHtmlEmbed->SetValue(true);
		rbHtmlLink->SetValue(false);
	}
	else if (val == wxT("l"))
	{
		rbHtmlBuiltin->SetValue(false);
		rbHtmlEmbed->SetValue(false);
		rbHtmlLink->SetValue(true);
	}
	else
	{
		rbHtmlBuiltin->SetValue(true);
		rbHtmlEmbed->SetValue(false);
		rbHtmlLink->SetValue(false);
	}

	// XML Stylesheet
	settings->Read(wxT("Reports/XmlStylesheetMode"), &val, wxT("p"));
	if (val == wxT("l"))
	{
		rbXmlPlain->SetValue(false);
		rbXmlLink->SetValue(true);
		rbXmlProcess->SetValue(false);
	}
	else if (val == wxT("r"))
	{
		rbXmlPlain->SetValue(false);
		rbXmlLink->SetValue(false);
		rbXmlProcess->SetValue(true);
	}
	else
	{
		rbXmlPlain->SetValue(true);
		rbXmlLink->SetValue(false);
		rbXmlProcess->SetValue(false);
	}

	// Default values
	settings->Read(wxT("Reports/LastNotes"), &val, wxT(""));
	txtNotes->SetValue(val);

	settings->Read(wxT("Reports/LastHtmlStylesheet"), &val, wxEmptyString);
	txtHtmlStylesheet->SetValue(val);

	settings->Read(wxT("Reports/LastXmlStylesheet"), &val, wxEmptyString);
	txtXmlStylesheet->SetValue(val);

	settings->Read(wxT("Reports/LastHtmlFile"), &val, wxEmptyString);
	txtHtmlFile->SetValue(val);

	settings->Read(wxT("Reports/LastXmlFile"), &val, wxEmptyString);
	txtXmlFile->SetValue(val);

	settings->Read(wxT("Reports/IncludeSQL"), &bVal, true);
	chkSql->SetValue(bVal);
	chkSql->Disable();

	settings->Read(wxT("Reports/OpenInBrowser"), &bVal, true);
	chkBrowser->SetValue(bVal);

	wxCommandEvent ev;
	OnChange(ev);

	txtTitle->SetFocus();
}


frmReport::~frmReport()
{
	SavePosition();
}


void frmReport::OnHelp(wxCommandEvent &ev)
{
	DisplayHelp(wxT("reports"), HELP_PGADMIN);
}


void frmReport::OnChange(wxCommandEvent &ev)
{
	bool enable = true;

	if (rbHtml->GetValue())
	{
		// Show/hide the appropriate controls
		rbHtmlBuiltin->Show(true);
		rbHtmlEmbed->Show(true);
		rbHtmlLink->Show(true);
		txtHtmlStylesheet->Show(true);
		txtHtmlFile->Show(true);

		rbXmlPlain->Show(false);
		rbXmlLink->Show(false);
		rbXmlProcess->Show(false);
		txtXmlStylesheet->Show(false);
		txtXmlFile->Show(false);

		// Enable/disable as appropriate
		if (txtHtmlFile->GetValue().IsEmpty())
			enable = false;

		if (rbHtmlBuiltin->GetValue())
		{
			txtHtmlStylesheet->Disable();
			btnStylesheet->Disable();
		}
		else
		{
			txtHtmlStylesheet->Enable();
			btnStylesheet->Enable();
		}

		if (rbHtmlEmbed->GetValue())
		{
			if (!wxFile::Exists(txtHtmlStylesheet->GetValue()))
				enable = false;
		}
	}
	else
	{
		// Show/hide the appropriate controls
		rbHtmlBuiltin->Show(false);
		rbHtmlEmbed->Show(false);
		rbHtmlLink->Show(false);
		txtHtmlStylesheet->Show(false);
		txtHtmlFile->Show(false);

		rbXmlPlain->Show(true);
		rbXmlLink->Show(true);
		rbXmlProcess->Show(true);
		txtXmlStylesheet->Show(true);
		txtXmlFile->Show(true);

		// Enable/disable as appropriate
		if (txtXmlFile->GetValue().IsEmpty())
			enable = false;

		if (rbXmlPlain->GetValue())
		{
			txtXmlStylesheet->Disable();
			btnStylesheet->Disable();
		}
		else
		{
			txtXmlStylesheet->Enable();
			btnStylesheet->Enable();
		}

		if (rbXmlProcess->GetValue())
		{
			if (!wxFile::Exists(txtXmlStylesheet->GetValue()))
				enable = false;
		}
	}

	btnOK->Enable(enable);
}


void frmReport::OnOK(wxCommandEvent &ev)
{
	wxString filename;
	if (rbHtml->GetValue())
		filename = txtHtmlFile->GetValue();
	else
		filename = txtXmlFile->GetValue();

	wxFileName fn(filename);
	fn.MakeAbsolute();

	// Check if the file exsits, and if so, whether to overwrite it
	if (wxFileExists(fn.GetFullPath()))
	{
		wxString msg;
		msg.Printf(_("The file: \n\n%s\n\nalready exists. Do you want to overwrite it?"), fn.GetFullPath().c_str());

		if (wxMessageBox(msg, _("Overwrite file?"), wxYES_NO | wxICON_QUESTION) != wxYES)
		{
			if (rbHtml->GetValue())
				txtHtmlFile->SetFocus();
			else
				txtXmlFile->SetFocus();
			return;
		}
	}

	// Yield, to allow the messagebox to go
	wxTheApp->Yield(true);

	// Add the title, notes and SQL
	if (txtTitle->GetValue() != wxT(""))
		XmlAddHeaderValue(wxT("title"), txtTitle->GetValue());

	if (txtNotes->GetValue() != wxT(""))
		XmlAddHeaderValue(wxT("notes"), txtNotes->GetValue());

	// Generate the report data
	wxString report;

	if (rbHtml->GetValue())
	{
		if (rbHtmlBuiltin->GetValue())
		{
			wxString xml = GetXmlReport(wxEmptyString);
			wxString xsl = GetDefaultXsl(GetEmbeddedCss(GetDefaultCss()));
			report = XslProcessReport(xml, xsl);
		}
		else if (rbHtmlEmbed->GetValue())
		{
			wxString xml = GetXmlReport(wxEmptyString);
			wxString css = FileRead(txtHtmlStylesheet->GetValue());
			if (css.IsEmpty())
			{
				wxLogError(_("No stylesheet data could be read from the file %s: Errcode=%d."), txtHtmlStylesheet->GetValue().c_str(), wxSysErrorCode());
				return;
			}
			wxString xsl = GetDefaultXsl(GetEmbeddedCss(css));
			report = XslProcessReport(xml, xsl);
		}
		else
		{
			wxString xml = GetXmlReport(wxEmptyString);
			wxString xsl = GetDefaultXsl(GetCssLink(txtHtmlStylesheet->GetValue()));
			report = XslProcessReport(xml, xsl);
		}
	}
	else
	{
		if (rbXmlPlain->GetValue())
		{
			report = GetXmlReport(wxEmptyString);
		}
		else if (rbXmlLink->GetValue())
		{
			report = GetXmlReport(txtXmlStylesheet->GetValue());
		}
		else
		{
			wxString xml = GetXmlReport(wxEmptyString);
			wxString xsl = FileRead(txtXmlStylesheet->GetValue());
			if (xsl.IsEmpty())
			{
				wxLogError(_("No stylesheet data could be read from the file %s: Errcode=%d."), txtXmlStylesheet->GetValue().c_str(), wxSysErrorCode());
				return;
			}
			report = XslProcessReport(xml, xsl);
		}
	}

	// If report is empty, an error must have occurred
	if (report.IsEmpty())
		return;

	// Save it to disk
	wxFile file(fn.GetFullPath(), wxFile::write);
	if (!file.IsOpened())
	{
		wxLogError(_("Failed to open file %s."), fn.GetFullPath().c_str());
		return;
	}
	file.Write(report, wxConvUTF8);
	file.Close();

	// Open the file in the default browser if required
	if (chkBrowser->GetValue())
#ifdef __WXMSW__
		wxLaunchDefaultBrowser(fn.GetFullPath());
#else
		wxLaunchDefaultBrowser(wxT("file://") + fn.GetFullPath());
#endif

	// Save the settings for next time round
	settings->Write(wxT("Reports/LastNotes"), txtNotes->GetValue());

	if (rbHtml->GetValue())
		settings->Write(wxT("Reports/ReportFormat"), wxT("h"));
	else
		settings->Write(wxT("Reports/ReportFormat"), wxT("x"));

	if (rbHtmlBuiltin->GetValue())
		settings->Write(wxT("Reports/HtmlStylesheetMode"), wxT("b"));
	else if (rbHtmlEmbed->GetValue())
		settings->Write(wxT("Reports/HtmlStylesheetMode"), wxT("e"));
	else
		settings->Write(wxT("Reports/HtmlStylesheetMode"), wxT("l"));

	if (rbXmlPlain->GetValue())
		settings->Write(wxT("Reports/XmlStylesheetMode"), wxT("p"));
	else if (rbXmlLink->GetValue())
		settings->Write(wxT("Reports/XmlStylesheetMode"), wxT("l"));
	else
		settings->Write(wxT("Reports/XmlStylesheetMode"), wxT("r"));

	settings->Write(wxT("Reports/LastHtmlStylesheet"), txtHtmlStylesheet->GetValue());
	settings->Write(wxT("Reports/LastXmlStylesheet"), txtXmlStylesheet->GetValue());


	settings->Write(wxT("Reports/LastHtmlFile"), txtHtmlFile->GetValue());
	settings->Write(wxT("Reports/LastXmlFile"), txtXmlFile->GetValue());

	settings->WriteBool(wxT("Reports/IncludeSQL"), chkSql->GetValue());

	settings->WriteBool(wxT("Reports/OpenInBrowser"), chkBrowser->GetValue());

	// Now go away
	if (IsModal())
		EndModal(wxID_OK);
	else
		Destroy();
}

void frmReport::OnCancel(wxCommandEvent &ev)
{
	if (IsModal())
		EndModal(wxID_CANCEL);
	else
		Destroy();
}

void frmReport::OnBrowseStylesheet(wxCommandEvent &ev)
{
	if (rbHtml->GetValue())
	{
		wxString def = txtHtmlStylesheet->GetValue();

		if (!wxFile::Exists(def))
			def.Empty();

#ifdef __WXMSW__
		wxFileDialog file(this, _("Select stylesheet filename"), wxGetHomeDir(), def, _("HTML Stylesheet files (*.css)|*.css|All files (*.*)|*.*"), wxFD_OPEN);
#else
		wxFileDialog file(this, _("Select stylesheet filename"), wxGetHomeDir(), def, _("HTML Stylesheet files (*.css)|*.css|All files (*)|*"), wxFD_OPEN);
#endif

		if (file.ShowModal() == wxID_OK)
		{
			txtHtmlStylesheet->SetValue(file.GetPath());
			OnChange(ev);
		}
	}
	else
	{
		wxString def = txtXmlStylesheet->GetValue();

		if (!wxFile::Exists(def))
			def.Empty();

#ifdef __WXMSW__
		wxFileDialog file(this, _("Select stylesheet filename"), wxGetHomeDir(), def, _("XML Stylesheet files (*.xsl)|*.xsl|All files (*.*)|*.*"), wxFD_OPEN);
#else
		wxFileDialog file(this, _("Select stylesheet filename"), wxGetHomeDir(), def, _("XML Stylesheet files (*.xsl)|*.xsl|All files (*)|*"), wxFD_OPEN);
#endif

		if (file.ShowModal() == wxID_OK)
		{
			txtXmlStylesheet->SetValue(file.GetPath());
			OnChange(ev);
		}
	}
}

void frmReport::OnBrowseFile(wxCommandEvent &ev)
{
	if (rbHtml->GetValue())
	{
#ifdef __WXMSW__
		wxFileDialog file(this, _("Select output filename"), wxGetHomeDir(), txtHtmlFile->GetValue(),
		                  _("HTML files (*.html)|*.html|All files (*.*)|*.*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
#else
		wxFileDialog file(this, _("Select output filename"), wxGetHomeDir(), txtHtmlFile->GetValue(),
		                  _("HTML files (*.html)|*.html|All files (*)|*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
#endif

		if (file.ShowModal() == wxID_OK)
		{
			txtHtmlFile->SetValue(file.GetPath());
			OnChange(ev);
		}
	}
	else
	{
#ifdef __WXMSW__
		wxFileDialog file(this, _("Select output filename"), wxGetHomeDir(), txtXmlFile->GetValue(),
		                  _("XML files (*.xml)|*.xml|All files (*.*)|*.*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
#else
		wxFileDialog file(this, _("Select output filename"), wxGetHomeDir(), txtXmlFile->GetValue(),
		                  _("XML files (*.xml)|*.xml|All files (*)|*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
#endif

		if (file.ShowModal() == wxID_OK)
		{
			txtXmlFile->SetValue(file.GetPath());
			OnChange(ev);
		}
	}
}

void frmReport::SetReportTitle(const wxString &t)
{
	txtTitle->SetValue(t);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// START STYLESHEET FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

wxString frmReport::GetCssLink(const wxString &file)
{
	wxString data;

	data = wxT("    <link rel=\"stylesheet\" href=\"") + file + wxT("\" type=\"text/css\" />\n");

	return data;
}

wxString frmReport::GetEmbeddedCss(const wxString &css)
{
	wxString data;

	data  = wxT("    <style type=\"text/css\">\n");
	data += css;
	data += wxT("    </style>\n");

	return data;
}

const wxString frmReport::GetDefaultCss()
{
	wxString data;

	data = wxT("      body {  font-family: verdana, helvetica, sans-serif; margin: 0px; padding: 0; }\n")
	       wxT("      h1 { font-weight: bold; font-size: 150%; border-bottom-style: solid; border-bottom-width: 2px; margin-top: 0px; padding-bottom: 0.5ex; color: #eeeeee; overflow: hidden; text-overflow: ellipsis; }\n")
	       wxT("      h2 { font-size: 130%; padding-bottom: 0.5ex; color: ") + appearanceFactory->GetReportKeyColour().GetAsString(wxC2S_HTML_SYNTAX) + wxT("; border-bottom-style: solid; border-bottom-width: 2px; }\n")
	       wxT("      h3 { font-size: 110%; padding-bottom: 0.5ex; color: #000000; }\n")
	       wxT("      th { text-align: left; background-color: ") + appearanceFactory->GetReportKeyColour().GetAsString(wxC2S_HTML_SYNTAX) + wxT("; color: #eeeeee; }\n")
	       wxT("      #ReportHeader { padding: 10px; background-color: ") + appearanceFactory->GetReportKeyColour().GetAsString(wxC2S_HTML_SYNTAX) + wxT("; color: #eeeeee; border-bottom-style: solid; border-bottom-width: 2px; border-color: #999999; }\n")
	       wxT("      #ReportHeader th { width: 25%; white-space: nowrap; vertical-align: top; }\n")
	       wxT("      #ReportHeader td { vertical-align: top; color: #eeeeee; }\n")
	       wxT("      #ReportNotes { padding: 10px; background-color: #eeeeee; font-size: 80%; border-bottom-style: solid; border-bottom-width: 2px; border-color: #999999; }\n")
	       wxT("      .ReportSQL { margin-bottom: 10px; padding: 10px; display: block; background-color: #eeeeee; font-family: monospace; }\n")
	       wxT("      #ReportDetails { margin-left: 10px; margin-right: 10px; margin-bottom: 10px; }\n")
	       wxT("      #ReportDetails td, th { font-size: 80%; margin-left: 2px; margin-right: 2px; }\n")
	       wxT("      #ReportDetails th { border-bottom-color: #777777; border-bottom-style: solid; border-bottom-width: 2px; }\n")
	       wxT("      .ReportDetailsOddDataRow { background-color: #dddddd; }\n")
	       wxT("      .ReportDetailsEvenDataRow { background-color: #eeeeee; }\n")
	       wxT("      .ReportTableHeaderCell { background-color: #dddddd; color: ") + appearanceFactory->GetReportKeyColour().GetAsString(wxC2S_HTML_SYNTAX) + wxT("; vertical-align: top; font-size: 80%; white-space: nowrap; }\n")
	       wxT("      .ReportTableValueCell { vertical-align: top; font-size: 80%; white-space: nowrap; }\n")
	       wxT("      .ReportTableInfo { font-size: 80%; font-style: italic; }\n")
	       wxT("      #ReportFooter { font-weight: bold; font-size: 80%; text-align: right; background-color: ") + appearanceFactory->GetReportKeyColour().GetAsString(wxC2S_HTML_SYNTAX) + wxT("; color: #eeeeee; margin-top: 10px; padding: 2px; border-bottom-style: solid; border-bottom-width: 2px; border-top-style: solid; border-top-width: 2px; border-color: #999999; }\n")
	       wxT("      #ReportFooter a { color: #ffffff; text-decoration: none; }\n");

	return data;
}

wxString frmReport::GetDefaultXsl(const wxString &css)
{
	wxString data;

	data = wxT("<?xml version=\"1.0\"?>\n")
	       wxT("<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n")
	       wxT("<xsl:output method=\"xml\" doctype-system=\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\" doctype-public=\"-//W3C//DTD XHTML 1.0 Transitional//EN\" indent=\"yes\" encoding=\"utf-8\" />\n")
	       wxT("<xsl:template match=\"/report\">\n")
	       wxT("\n")
	       wxT("<html>\n")
	       wxT("  <head>\n")
	       wxT("    <xsl:if test=\"header/title != ''\">\n")
	       wxT("      <title><xsl:value-of select=\"header/title\" /></title>\n")
	       wxT("    </xsl:if>\n")
	       wxT("    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
	data += css;
	data += wxT("  </head>\n")
	        wxT("\n")
	        wxT("  <body>\n")
	        wxT("    <div id=\"ReportHeader\">\n")
	        wxT("\n")
	        wxT("    <xsl:if test=\"header/title != ''\">\n")
	        wxT("      <h1><xsl:value-of select=\"header/title\" /></h1>\n")
	        wxT("    </xsl:if>\n")
	        wxT("\n")
	        wxT("    <xsl:if test=\"header/generated != ''\">\n")
	        wxT("      <b>");
	data +=   _("Generated");
	data += wxT(": </b><xsl:value-of select=\"header/generated\" /><br />\n")
	        wxT("    </xsl:if>\n")
	        wxT("\n")
	        wxT("    <xsl:if test=\"header/server != ''\">\n")
	        wxT("      <b>");
	data +=   _("Server");
	data += wxT(": </b><xsl:value-of select=\"header/server\" /><br />\n")
	        wxT("    </xsl:if>\n")
	        wxT("\n")
	        wxT("    <xsl:if test=\"header/database != ''\">\n")
	        wxT("      <b>");
	data +=   _("Database");
	data += wxT(": </b><xsl:value-of select=\"header/database\" /><br />\n")
	        wxT("    </xsl:if>\n")
	        wxT("\n")
	        wxT("    <xsl:if test=\"header/catalog != ''\">\n")
	        wxT("      <b>");
	data +=   _("Catalog");
	data += wxT(": </b><xsl:value-of select=\"header/catalog\" /><br />\n")
	        wxT("    </xsl:if>\n")
	        wxT("\n")
	        wxT("    <xsl:if test=\"header/schema != ''\">\n")
	        wxT("      <b>");
	data +=   _("Schema");
	data += wxT(": </b><xsl:value-of select=\"header/schema\" /><br />\n")
	        wxT("    </xsl:if>\n")
	        wxT("\n")
	        wxT("    <xsl:if test=\"header/table != ''\">\n")
	        wxT("      <b>");
	data +=   _("Table");
	data += wxT(": </b><xsl:value-of select=\"header/table\" /><br />\n")
	        wxT("    </xsl:if>\n")
	        wxT("\n")
	        wxT("    <xsl:if test=\"header/job != ''\">\n")
	        wxT("      <b>");
	data +=   _("Job");
	data += wxT(": </b><xsl:value-of select=\"header/job\" /><br />\n")
	        wxT("    </xsl:if>\n")
	        wxT("\n")
	        wxT("    </div>\n")
	        wxT("\n")
	        wxT("    <xsl:if test=\"header/notes != ''\">\n")
	        wxT("      <div id=\"ReportNotes\">\n")
	        wxT("      <b>");
	data +=   _("Notes");
	data += wxT(": </b><br /><br />\n")
	        wxT("      <xsl:call-template name=\"substitute\">\n")
	        wxT("         <xsl:with-param name=\"string\" select=\"header/notes\" />\n")
	        wxT("      </xsl:call-template>\n")
	        wxT("      </div>\n")
	        wxT("    </xsl:if>\n")
	        wxT("\n")
	        wxT("    <div id=\"ReportDetails\">\n")
	        wxT("      <xsl:apply-templates select=\"section\" >\n")
	        wxT("        <xsl:sort select=\"@number\" data-type=\"number\" order=\"ascending\" />\n")
	        wxT("      </xsl:apply-templates>\n")
	        wxT("    </div>\n")
	        wxT("\n")
	        wxT("    <div id=\"ReportFooter\">\n");
	data +=   _("Report generated by");
	data += wxT(" <a href=\"");
	data += HtmlEntities(appearanceFactory->GetWebsiteUrl());
	data += wxT("\">");
	data += HtmlEntities(appearanceFactory->GetLongAppName());
	data += wxT("</a>\n")
	        wxT("    </div>\n")
	        wxT("\n")
	        wxT("    <br />\n")
	        wxT("  </body>\n")
	        wxT("</html>\n")
	        wxT("\n")
	        wxT("</xsl:template>\n")
	        wxT("\n")
	        wxT("<xsl:template match=\"section\">\n")
	        wxT("  <xsl:if test=\"../section[@id = current()/@id]/@name != ''\">\n")
	        wxT("    <h2><xsl:value-of select=\"../section[@id = current()/@id]/@name\" /></h2>\n")
	        wxT("  </xsl:if>\n")
	        wxT("\n")
	        wxT("  <xsl:if test=\"count(../section[@id = current()/@id]/table/columns/column) > 0\">\n")
	        wxT("    <div style=\"overflow:auto;\">\n")
	        wxT("      <table>\n")
	        wxT("        <tr>\n")
	        wxT("          <xsl:apply-templates select=\"../section[@id = current()/@id]/table/columns/column\">\n")
	        wxT("            <xsl:sort select=\"@number\" data-type=\"number\" order=\"ascending\" />\n")
	        wxT("            <xsl:with-param name=\"count\" select=\"count(../section[@id = current()/@id]/table/columns/column)\" />\n")
	        wxT("          </xsl:apply-templates>\n")
	        wxT("        </tr>\n")
	        wxT("        <xsl:apply-templates select=\"../section[@id = current()/@id]/table/rows/*\" mode=\"rows\">\n")
	        wxT("          <xsl:sort select=\"@number\" data-type=\"number\" order=\"ascending\" />\n")
	        wxT("          <xsl:with-param name=\"column-meta\" select=\"../section[@id = current()/@id]/table/columns/column\" />\n")
	        wxT("        </xsl:apply-templates>\n")
	        wxT("      </table>\n")
	        wxT("    </div>\n")
	        wxT("    <br />\n")
	        wxT("    <xsl:if test=\"../section[@id = current()/@id]/table/info != ''\">\n")
	        wxT("      <p class=\"ReportTableInfo\"><xsl:value-of select=\"../section[@id = current()/@id]/table/info\" /></p>\n")
	        wxT("    </xsl:if>\n")
	        wxT("  </xsl:if>\n")
	        wxT("\n")
	        wxT("  <xsl:if test=\"../section[@id = current()/@id]/sql != ''\">\n")
	        wxT("    <pre class=\"ReportSQL\">\n")
	        wxT("      <xsl:call-template name=\"substitute\">\n")
	        wxT("         <xsl:with-param name=\"string\" select=\"../section[@id = current()/@id]/sql\" />\n")
	        wxT("      </xsl:call-template>\n")
	        wxT("    </pre>\n")
	        wxT("  </xsl:if>\n")
	        wxT("</xsl:template>\n")
	        wxT("\n")
	        wxT("<xsl:template match=\"column\">\n")
	        wxT("  <xsl:param name=\"count\" />\n")
	        wxT("  <th class=\"ReportTableHeaderCell\">\n")
	        wxT("  <xsl:attribute name=\"width\"><xsl:value-of select=\"100 div $count\" />%</xsl:attribute>\n")
	        wxT("    <xsl:call-template name=\"substitute\">\n")
	        wxT("       <xsl:with-param name=\"string\" select=\"@name\" />\n")
	        wxT("    </xsl:call-template>\n")
	        wxT("  </th>\n")
	        wxT("</xsl:template>\n")
	        wxT("\n")
	        wxT("<xsl:template match=\"*\" mode=\"rows\">\n")
	        wxT("  <xsl:param name=\"column-meta\" />\n")
	        wxT("  <tr>\n")
	        wxT("  <xsl:choose>\n")
	        wxT("  <xsl:when test=\"position() mod 2 != 1\">\n")
	        wxT("    <xsl:attribute name=\"class\">ReportDetailsOddDataRow</xsl:attribute>\n")
	        wxT("  </xsl:when>\n")
	        wxT("  <xsl:otherwise>\n")
	        wxT("    <xsl:attribute name=\"class\">ReportDetailsEvenDataRow</xsl:attribute>\n")
	        wxT("  </xsl:otherwise>\n")
	        wxT("  </xsl:choose>\n")
	        wxT("    <xsl:apply-templates select=\"$column-meta\" mode=\"cells\">\n")
	        wxT("      <xsl:with-param name=\"row\" select=\".\" />\n")
	        wxT("    </xsl:apply-templates>\n")
	        wxT("  </tr>\n")
	        wxT("</xsl:template>\n")
	        wxT("\n")
	        wxT("<xsl:template match=\"*\" mode=\"cells\">\n")
	        wxT("  <xsl:param name=\"row\" />\n")
	        wxT("    <td class=\"ReportTableValueCell\">\n")
	        wxT("    <xsl:choose>\n")
	        wxT("      <xsl:when test=\"$row/@*[name() = current()/@id]|$row/*[name() = current()/@id] != ''\">\n")
	        wxT("        <xsl:call-template name=\"substitute\">\n")
	        wxT("          <xsl:with-param name=\"string\" select=\"$row/@*[name() = current()/@id]|$row/*[name() = current()/@id]\" />\n")
	        wxT("        </xsl:call-template>\n")
	        wxT("      </xsl:when>\n")
	        wxT("      <xsl:otherwise>\n")
	        wxT("        <xsl:text> </xsl:text>\n")
	        wxT("      </xsl:otherwise>\n")
	        wxT("    </xsl:choose>\n")
	        wxT("  </td>\n")
	        wxT("</xsl:template>\n")
	        wxT("\n")
	        wxT("<xsl:template name=\"substitute\">\n")
	        wxT("   <xsl:param name=\"string\" />\n")
	        wxT("   <xsl:param name=\"from\" select=\"'&#xA;'\" />\n")
	        wxT("   <xsl:param name=\"to\">\n")
	        wxT("      <br />\n")
	        wxT("   </xsl:param>\n")
	        wxT("   <xsl:choose>\n")
	        wxT("      <xsl:when test=\"contains($string, $from)\">\n")
	        wxT("         <xsl:value-of select=\"substring-before($string, $from)\" />\n")
	        wxT("         <xsl:copy-of select=\"$to\" />\n")
	        wxT("         <xsl:call-template name=\"substitute\">\n")
	        wxT("            <xsl:with-param name=\"string\" select=\"substring-after($string, $from)\" />\n")
	        wxT("            <xsl:with-param name=\"from\" select=\"$from\" />\n")
	        wxT("            <xsl:with-param name=\"to\" select=\"$to\" />\n")
	        wxT("         </xsl:call-template>\n")
	        wxT("      </xsl:when>\n")
	        wxT("      <xsl:otherwise>\n")
	        wxT("         <xsl:value-of select=\"$string\" />\n")
	        wxT("      </xsl:otherwise>\n")
	        wxT("   </xsl:choose>\n")
	        wxT("</xsl:template>\n")
	        wxT("\n")
	        wxT("</xsl:stylesheet>\n");

	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// END STYLESHEET FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// START XML FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void frmReport::XmlAddHeaderValue(const wxString &name, const wxString &value)
{
	header += wxT("    <") + HtmlEntities(name) + wxT(">") + HtmlEntities(value) + wxT("</") + HtmlEntities(name) + wxT(">\n");
}

int frmReport::XmlCreateSection(const wxString &name)
{
	int ind = sectionName.Add(HtmlEntities(name));
	sectionData.Add(wxT(""));
	sectionTableHeader.Add(wxT(""));
	sectionTableRows.Add(wxT(""));
	sectionTableInfo.Add(wxT(""));
	sectionSql.Add(wxT(""));
	return ind + 1;
}

void frmReport::XmlSetSectionTableHeader(const int section, int columns, const wxChar *name, ...)
{
	va_list ap;
	const wxChar *p = name;
	wxString data;

	va_start(ap, name);

	for (int x = 0; x < columns; x++)
	{
		data += wxT("        <column id=\"c");
		data += NumToStr((long)(x + 1));
		data += wxT("\" number=\"");
		data += NumToStr((long)(x + 1));
		data += wxT("\" name=\"");
		data += HtmlEntities(p);
		data += wxT("\"/>\n");
		p = va_arg(ap, wxChar *);
	}

	va_end(ap);

	sectionTableHeader[section - 1] = data;
}

void frmReport::XmlAddSectionTableRow(const int section, int number, int columns, const wxChar *value, ...)
{
	va_list ap;
	const wxChar *p = value;
	wxString data;

	va_start(ap, value);

	data = wxT("        <row id=\"r");
	data += NumToStr((long)number);
	data += wxT("\" number=\"");
	data += NumToStr((long)number);
	data += wxT("\"");

	for (int x = 0; x < columns; x++)
	{
		data += wxT(" c");
		data += NumToStr((long)(x + 1));
		data += wxT("=\"");
		data += HtmlEntities(p);
		data += wxT("\"");
		p = va_arg(ap, wxChar *);
	}


	data += wxT(" />\n");

	va_end(ap);

	sectionTableRows[section - 1] += data;
}

void frmReport::XmlAddSectionTableFromListView(const int section, ctlListView *list)
{
	// Get the column headers
	int cols = list->GetColumnCount();

	wxString data;
	wxListItem itm;

	// Build the columns
	for (int x = 0; x < cols; x++)
	{
		itm.SetMask(wxLIST_MASK_TEXT);
		list->GetColumn(x, itm);
		wxString label = itm.GetText();
		data += wxT("        <column id=\"c");
		data += NumToStr((long)(x + 1));
		data += wxT("\" number=\"");
		data += NumToStr((long)(x + 1));
		data += wxT("\" name=\"");
		data += HtmlEntities(label);
		data += wxT("\" />\n");
	}
	sectionTableHeader[section - 1] = data;

	// Build the rows
	int rows = list->GetItemCount();

	for (int y = 0; y < rows; y++)
	{
		data = wxT("        <row id=\"r");
		data += NumToStr((long)(y + 1));
		data += wxT("\" number=\"");
		data += NumToStr((long)(y + 1));
		data += wxT("\"");

		for (int x = 0; x < cols; x++)
		{
			data += wxT(" c");
			data += NumToStr((long)(x + 1));
			data += wxT("=\"");
			data += HtmlEntities(list->GetText(y, x));
			data += wxT("\"");
		}
		data += wxT(" />\n");
		sectionTableRows[section - 1] += data;
	}
}

void frmReport::XmlAddSectionTableFromGrid(const int section, ctlSQLResult *grid)
{
	// Get the column headers
	int cols = grid->GetNumberCols();
	int shift = 0;

	wxString data;
	wxListItem itm;

	if (grid->GetRowCountSuppressed())
		shift = 1;

	for (int x = 1; x <= cols; x++)
	{
		wxString label = grid->OnGetItemText(-1, x - shift);
		data += wxT("        <column id=\"c");
		data += NumToStr((long)(x));
		data += wxT("\" number=\"");
		data += NumToStr((long)(x));
		data += wxT("\" name=\"");
		data += HtmlEntities(label);
		data += wxT("\" />\n");
	}
	sectionTableHeader[section - 1] = data;

	// Build the rows
	int rows = grid->NumRows();

	for (int y = 0; y < rows; y++)
	{
		data = wxT("        <row id=\"r");
		data += NumToStr((long)(y + 1));
		data += wxT("\" number=\"");
		data += NumToStr((long)(y + 1));
		data += wxT("\"");

		for (int x = 1; x <= cols; x++)
		{
			data += wxT(" c");
			data += NumToStr((long)(x));
			data += wxT("=\"");
			data += HtmlEntities(grid->OnGetItemText(y, x - shift));
			data += wxT("\"");
		}
		data += wxT(" />\n");
		sectionTableRows[section - 1] += data;
	}
}

void frmReport::XmlSetSectionSql(int section, const wxString &sql)
{
	sectionSql[section - 1] = HtmlEntities(sql);

	if (!sectionSql[section - 1].IsEmpty())
		chkSql->Enable();
	else
		chkSql->Disable();
}

void frmReport::XmlAddSectionValue(const int section, const wxString &name, const wxString &value)
{
	sectionData[section - 1] += wxT("    <") + HtmlEntities(name) + wxT(">") + HtmlEntities(value) + wxT("</") + HtmlEntities(name) + wxT(">\n");
}

wxString frmReport::GetSectionTableColumns(const int section)
{
	wxString data;

	data  = wxT("      <columns>\n");
	data += sectionTableHeader[section - 1];
	data += wxT("      </columns>\n");

	return data;
}

wxString frmReport::GetSectionTableRows(const int section)
{
	wxString data;

	data  = wxT("      <rows>\n");
	data += sectionTableRows[section - 1];
	data += wxT("      </rows>\n");

	return data;
}

wxString frmReport::GetSectionTable(const int section)
{
	wxString data;

	data  = wxT("    <table>\n");
	data += GetSectionTableColumns(section);
	data += GetSectionTableRows(section);

	if (!sectionTableInfo[section - 1].IsEmpty())
	{
		data += wxT("      <info>");
		data += sectionTableInfo[section - 1];
		data += wxT("</info>\n");
	}

	data += wxT("    </table>\n");

	return data;
}

wxString frmReport::GetSection(const int section)
{
	wxString data;

	data  = wxT("  <section id=\"s");
	data += NumToStr((long)section);
	data += wxT("\" number=\"");
	data += NumToStr((long)section);
	data += wxT("\" name=\"");
	data += sectionName[section - 1];
	data += wxT("\">\n");
	data += GetSectionTable(section);


	if (chkSql->GetValue() && !sectionSql[section - 1].IsEmpty())
	{
		data += wxT("    <sql>");
		data += sectionSql[section - 1];
		data += wxT("</sql>\n");
	}

	data += sectionData[section - 1];
	data += wxT("  </section>\n");

	return data;
}

wxString frmReport::GetXmlReport(const wxString &stylesheet = wxT(""))
{
	wxString data;

	data  = wxT("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

	if (!stylesheet.IsEmpty())
	{
		data += wxT("<?xml-stylesheet type=\"text/xsl\" href=\"");
		data += stylesheet;
		data += wxT("\" ?>\n");
	}

	data += wxT("\n");
	data += wxT("<report>\n\n");

	data += wxT("  <header>\n");
	data += header;
	data += wxT("  </header>\n\n");

	for (unsigned int x = 1; x <= sectionName.GetCount(); x++ )
	{
		data += GetSection(x);
		data += wxT("\n");
	}

	data += wxT("</report>\n");

	return data;
}

//
// libxml convenience macros
//
#define XML_FROM_WXSTRING(s) ((xmlChar *)(const char *)s.mb_str(wxConvUTF8))
#define WXSTRING_FROM_XML(s) wxString((char *)s, wxConvUTF8)

wxString frmReport::XslProcessReport(const wxString &xml, const wxString &xsl)
{
	xmlChar *output = 0;
	xmlDocPtr ssDoc = 0, xmlDoc = 0, resDoc = 0;
	xsltStylesheetPtr ssPtr = 0;
	int length;

	wxBeginBusyCursor();

	// Apply the stylesheet
	xmlSubstituteEntitiesDefault (1); // Substitute entities
	xmlLoadExtDtdDefaultValue = 1; // Load external entities

	// Parse the stylesheet
	ssDoc = xmlParseDoc(XML_FROM_WXSTRING(xsl));
	if (!ssDoc)
	{
		wxEndBusyCursor();
		wxLogError(_("Failed to parse the XML stylesheet!"));
		goto cleanup;
	}

	ssPtr = xsltParseStylesheetDoc(ssDoc);
	if (!ssPtr)
	{
		wxEndBusyCursor();
		wxLogError(_("Failed to parse the XSL stylesheet!"));
		goto cleanup;
	}

	// Parse the data
	xmlDoc = xmlParseDoc(XML_FROM_WXSTRING(xml));
	if (!xmlDoc)
	{
		wxEndBusyCursor();
		wxLogError(_("Failed to parse the XML document!"));
		goto cleanup;
	}

	// Apply the stylesheet
	resDoc = xsltApplyStylesheet(ssPtr, xmlDoc, NULL);
	if (!resDoc)
	{
		wxEndBusyCursor();
		wxLogError(_("Failed to apply the XSL stylesheet to the XML document!"));
		goto cleanup;
	}

	// Get the result
	xsltSaveResultToString (&output, &length, resDoc, ssPtr);
	if (!resDoc)
	{
		wxEndBusyCursor();
		wxLogError(_("Failed to read the processed document!"));
		goto cleanup;
	}

cleanup:

	// Cleanup
	if (resDoc)
		xmlFreeDoc(resDoc);

	if (xmlDoc)
		xmlFreeDoc(xmlDoc);

	if (ssPtr)
		xsltFreeStylesheet(ssPtr);

	// This crashes - dunno why :-(
	// if (ssDoc)
	//  xmlFreeDoc(ssDoc);

	xsltCleanupGlobals();

	wxEndBusyCursor();

	if (output)
		return WXSTRING_FROM_XML(output);
	else
		return wxEmptyString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// END XML FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SQL::SQL(const wxString &_sql, const wxString &_pathtree) {
  // Construct a SQL with the specified operation and text.
	sql=wxString(_sql);
	pathtree=wxString(_pathtree) ;
	countchild=0;
	mode=0;
}

SQL::SQL() {
}

/**
 * Display a human-readable version of this SQL.
 * @return text version
 */
wxString SQL::toString() const {
  wxString prettyText = sql;
  // Replace linebreaks with Pilcrow signs.
  //prettyText.replace('\n', L'\u00b6');
  	wxString c=wxEmptyString;
	wxString cmp=wxEmptyString;
	if (countchild>0) c.Printf(wxT("(%d)"), countchild);
	bool r=false;
	if (sql.Cmp(sql2)!=0) {
		cmp="ne EQ ";
		cmp.Printf(wxT("(ne EQ (%d, %d))"), sql.Len(),sql2.Len());
	}
	if (mode==__Remove) cmp=cmp+" - ";
	if (mode==__Insert) cmp=cmp+" + ";
	if (mode==__Equal) cmp=cmp+" = ";
  return wxString(pathtree+c+"\n"+cmp+"SQL(" + wxString("\""))
      + prettyText + wxString("\")");
}

/**
 * Is this SQL equivalent to another SQL?
 * @param d Another SQL to compare against
 * @return true or false
 */
bool SQL::operator==(const SQL &d) const {
  return (d.sql == this->sql);
}

bool SQL::operator!=(const SQL &d) const {
  return !(operator == (d));
}
	#include <wx/listimpl.cpp>
	WX_DEFINE_LIST(MyListSql);
#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(ArraySQL)
wxString reportCompareFactory::GetNodePath(wxTreeItemId node) {
	wxString path;
	path = parent->GetBrowser()->GetItemText(node).Trim();

	wxTreeItemId parent_id = parent->GetBrowser()->GetItemParent(node);
	while (parent_id.IsOk())
	{
		path = parent->GetBrowser()->GetItemText(parent_id).BeforeFirst('(').Trim() + wxT("/") + path;
		parent_id = parent->GetBrowser()->GetItemParent(parent_id);
	}

	return path;

}
wxString reportCompareFactory::ApplyCompareOpts(wxString sql,int metatype) {
	if (sql.IsEmpty()) return "";
	bool no_comment = parent->getChoiceSelect(1);
	bool no_priv = parent->getChoiceSelect(0);
	if (no_comment) {
		int last = 0;
		while (last!=-1) {
			last = sql.Find("\nCOMMENT ON");
			int start = last;
			int end = -1;
			while (last!=-1) {
				last++;
				if (sql.Length() == last) { last = -1; continue; }
				wxChar c = sql.GetChar(last);
				if (c == '\'') {
					last++;
					if (sql.GetChar(last) == '\'') { continue; }
					if (sql.GetChar(last) == ';') { end = last; last = -1; continue; }
					
				}
			};
			if (end != -1) {
				sql=sql.Remove(start, end - start+1);
				last = 0;
			}
		}
		
	}
	if (no_priv &&(metatype != PGM_ROLE)) {
		int last = 0;
		while (last != -1) {
			last = sql.Find("\nGRANT ");
			if (last==-1) last = sql.Find("\nREVOKE ");
			int start = last;
			int end = -1;
			while (last != -1) {
				last++;
				if (sql.Length() == last) { last = -1; continue; }
				wxChar c = sql.GetChar(last);
				if (c == ';') {
					 end = last; last = -1;
				}
			};
			if (end != -1) {
				sql = sql.Remove(start, end - start + 1);
				last = 0;
			}
		}

	}
	return sql;

}
void reportCompareFactory::GetExpandedChildNodes(wxTreeItemId node, wxArrayString &expandedNodes, ArraySQL &list, time_t *t, wxBusyInfo *w, MyHashSQL &h_path,int lvl)
{
	wxTreeItemIdValue cookie;
	ctlTree *browser=parent->GetBrowser();
	wxTreeItemId child;
	if (lvl==0) child = node;
		else child = browser->GetFirstChild(node, cookie);
	pgObject *obj;
	wxString path;
	time_t tmp;
	int size=expandedNodes.Count();
	while (child.IsOk())
	{
		obj=browser->GetObject(child);
		if (obj && obj->GetMetaType()==PGM_FUNCTION && !browser->HasChildren(child))
			path=GetNodePath(child);
		else
											  		path=parent->GetNodePath(child);
		
	//	tmp=wxDateTime::GetTimeNow();
	//	if (difftime(tmp,*t)>2.0) {
			//w->~wxBusyInfo();
			//wxSafeYield();
			//wxMilliSleep(50);
			//wxSafeYield();
			//w = new wxBusyInfo(wxString::Format("Path = %s ,GetName() = %s, isCollection = %d",
			//                                    path.c_str(), obj->GetName().c_str(), obj->IsCollection()),parent);
			//wxSafeYield();
			//wxMilliSleep(50);
			//wxSafeYield();
	//		*t=tmp;
	//	}
		if (obj) {
//		  OutputDebugString(wxString::Format("Path = %s ,GetName() = %s, isCollection = %d\n",
//			                                    path.c_str(), obj->GetTypeName().c_str(), obj->IsCollection()));
		  if (obj->GetMetaType()==PGM_CATALOG
			  ||obj->GetMetaType()==PGM_COLUMN
			  ||obj->GetMetaType()==PGM_RULE
			  ||obj->GetMetaType()==PGM_CATALOG
			  ) {
				  child = browser->GetNextChild(node, cookie);
				  continue;
		  }
		  //obj->ShowTreeDetail(browser);
		  //obj->ShowTree(parent,browser);
		  // если надо объекто сложный и сас состоит из коллекций
		if ((obj->GetMetaType()==PGM_SCHEMA
			||obj->GetMetaType()==PGM_DATABASE
			||obj->GetMetaType()==PGM_TABLE
			||obj->GetMetaType()==PGM_FOREIGNTABLE
			)&&!obj->IsCollection()) {
						obj->ShowTreeDetail(browser);
						//obj->ShowTree(parent,browser);
		} else
		{
			if (obj->GetMetaType() == PGM_VIEW) obj->ShowTreeDetail(browser); // только для того чтобы получить инфу о триггерах
			if (obj->GetMetaType() == PGM_EVENTTRIGGER)  // получаем инфу о тригеррах по событиям
				obj->ShowTreeDetail(browser);
		}
		}
		
		//if (browser->IsExpanded(child))
		if (browser->HasChildren(child))
		{
			bool rec=true;
		  if (obj && (obj->GetMetaType()==PGM_TABLE
			  //||obj->GetMetaType()==PGM_VIEW
			  )) {
				  wxTreeItemId Item = browser->GetItemParent(child);
				  obj=browser->GetObject(Item); // Tables
				  wxTreeItemId Item2 = browser->GetItemParent(obj->GetId());
				  obj=browser->GetObject(Item2); // Schemes
				  if (obj && obj->GetMetaType()==PGM_SCHEMA&& !obj->IsCollection()) {
					  rec=false; // не собираем инфу по сек. таблицам и секциям, и во внутрь не заходим
					  obj=browser->GetObject(child);
					  obj->ShowTreeDetail(browser);
				  } else obj=browser->GetObject(child);
		   }
		  if (obj && (obj->GetMetaType()==PGM_VIEW && !obj->IsCollection())) rec=false;
		  if (rec) {
			GetExpandedChildNodes(child, expandedNodes,list,t,w,h_path,lvl+1);
			//expandedNodes.Add(parent->GetNodePath(child));
		    obj=browser->GetObject(child);
		  }
		  
		}
		if (obj ) {
				wxString s=obj->GetSql(browser);
				if (obj->GetMetaType()==PGM_SEQUENCE) s="";
				s = ApplyCompareOpts(s, obj->GetMetaType());
				int c=browser->GetChildrenCount(child,false);
				if (size>0) {
					wxString srcpath(path);
					SQL *sq;
					srcpath.Replace(expandedNodes[0],expandedNodes[2],false);
					srcpath.Replace(expandedNodes[1],expandedNodes[3],false);
					MyHashSQL::iterator it=h_path.find(srcpath);
					if (h_path.end()==it) {
						// не найдено в первой БД

						if (s!=wxEmptyString) {
							sq =new SQL(wxEmptyString,srcpath);
							sq->sql2=s;
							sq->mode=__Remove;
							list.Add(sq);

						} else
						{
							sq =new SQL(wxEmptyString,srcpath);
							sq->sql2=wxEmptyString;
							sq->countchild=c;
							sq->mode=__Remove;
							list.Add(sq);
							
						}
						
						h_path[srcpath]=list.GetCount()-1;
					}
					else {
						int i=it->second;
						SQL& sql=list.Item(i);
						sql.mode=__Equal;
						sql.sql2=s;
					}
				} else {
					if (s!=wxEmptyString) {
						SQL *sq =new SQL(s,path);
						sq->mode=__Insert;
						list.Add(sq);
					} else
					{
						SQL *sq =new SQL(s,path);
						sq->countchild=c;
						sq->mode=__Insert;
						list.Add(sq);
					}
					h_path[path]=list.GetCount()-1;
				}
		}
		
		child = browser->GetNextChild(node, cookie);
	}

}

wxWindow *reportCompareFactory::StartDialog(frmMain *form, pgObject *obj)
{
	parent = form;
//	std::wstring r;
//	std::wstring str1=L"";
//	std::wstring str2=L"line#\nline2\nADD line 3\nline4\n";
//	r=printdiff(str1,str2);
//	return 0;

//	wxBeginBusyCursor();
	//frmReport *report = new frmReport(GetFrmMain());
	//wxBusyInfo *waiting;
	wxString msg;
	// Generate the report header
	wxDateTime now = wxDateTime::Now();
	
		ctlTree *browser=form->GetBrowser();
	wxTreeItemIdValue foldercookie;
	wxTreeItemId folderitem = browser->GetFirstChild(browser->GetRootItem(), foldercookie);
	wxString path(form->GetNodePath(obj->GetId()));
	// ������ ��������/�������/serverN/Datebases/dbname
	//                p1      p2      p3
	wxString p_db;
	int p1=path.Find('/');
	if (p1<0) return 0;
	int p2=path.find('/',p1+1);
	if (p2<0) return 0;
	startpathpos=p2;
	int p3=path.find('/',p2+1);
	wxString p_pref=path.substr(0,p2);
	wxString p_server_obj;
	if (!obj->GetConnection()) return 0;
	if (obj && obj->GetDatabase() && obj->GetDatabase()->GetConnected()) 
			p_db=obj->GetDatabase()->GetName();
	else
			p_db=obj->GetServer()->GetDatabaseName();

	if (p3<0) {
		// select server
		//if (wxMessageBox(wxString::Format("Path = %s ,GetName() = %s, isCollection = %d",
		//	                                    path.c_str(), obj->GetTypeName().c_str(), obj->IsCollection()), _("Close"), wxYES_NO | wxICON_QUESTION) != wxYES)
		//{
		//	return 0;
		//}
//Группы серверов/Серверы/PostgreSQL 9.6
// используем первую попавшуюся открытую БД
		p_server_obj = path.substr(p2); // с /серверN/

	}
	else
	{
		//	Группы серверов/Серверы/PostgreSQL 9.6/Базы данных/postgres
		p_server_obj=path.substr(p2,p3-p2); // � /������N/
	}
	wxString p_db_replace=_("Databases")+"/"+p_db+"/";
	wxString p_server_replace=_("Servers")+p_server_obj;
	pgServer *server;
	pgDatabase *db=NULL,*lastdb=NULL;
wxString trg_db_replace;
wxString trg_server_replace;

	wxTreeItemId srvitem = obj->GetServer()->GetId();

	while (folderitem)
	{
		if (browser->ItemHasChildren(folderitem))
		{
			wxTreeItemIdValue servercookie;
			wxTreeItemId serveritem = browser->GetFirstChild(folderitem, servercookie);
			while (serveritem)
			{
				server = (pgServer *)browser->GetItemData(serveritem);
				if (server != NULL && server->IsCreatedBy(serverFactory))
				{
					trg_server_replace=browser->GetItemText(server->GetId()).BeforeFirst('(').Trim();
					if (srvitem!=server->GetId() && server->GetConnected()) {
						// наше соединение не нужно нужно другое и активное
						pgCollection *coll = browser->FindCollection(databaseFactory, server->GetId());
						if (coll)
						{
							treeObjectIterator dbs(browser, coll);
							while ((db = (pgDatabase *)dbs.GetNextObject()) != 0)
							{
								// есть открытая БД
								lastdb=db;
								if (db->GetConnected()) {

									//if (db->GetName()!=wxT("postgres")) 	goto ex_out;
									goto ex_out;
								}
							}
							
						}
					}
				}
				serveritem = browser->GetNextChild(folderitem, servercookie);
			}
		}
		folderitem = browser->GetNextChild(browser->GetRootItem(), foldercookie);
	}
ex_out:
pgObject *trgobj;
wxString newpath;
if (lastdb!=NULL) {
			p_db=browser->GetItemText(lastdb->GetId()).BeforeFirst('(').Trim();
			trg_db_replace=_("Databases")+"/"+p_db+"/";
			trg_server_replace=_("Servers")+"/"+trg_server_replace;

			newpath=path;
			
			//if (newpath.Replace(p_server_replace,trg_server_replace,false)==0) 
			newpath.Replace(p_server_replace,trg_server_replace,false);
			newpath.Replace(p_db_replace,trg_db_replace,false);
			if (!parent->SetCurrentNode(parent->GetBrowser()->GetRootItem(),newpath)) {
				msg.Printf("Not found object %s in other DB.",newpath);
				wxMessageBox(msg, _("Error"), wxOK | wxICON_INFORMATION);
				return 0;
			}
			trgobj=browser->GetObject(browser->GetSelection());
} else
{
		msg="Not open connecting, compare stop.";
		//		msg.Printf("В установленном соединении %s, нет подходящих БД.",browser->GetItemText(lastdb->GetServer()->GetId()).BeforeFirst('(').Trim());
	wxMessageBox(msg, _("Error"), wxOK | wxICON_INFORMATION);
	return 0;
}
if (!parent->StartChoiceDialog()) return 0;
time_t timer=wxDateTime::GetTimeNow();
	wxArrayString expandedNodes;
	ArraySQL list;
	MyHashSQL h_path;

wxWindowDisabler disableAll;
{
#ifndef DEBUG
			wxBusyInfo waiting(wxString::Format(" Reading source DB Path = %s ,Start object = %s",
			                                    browser->GetItemText(obj->GetServer()->GetId()).c_str(), obj->GetName().c_str(),parent));
			// Give the UI a chance to redraw
			wxSafeYield();
			wxMilliSleep(50);
			wxSafeYield();
#endif
//			waiting->~wxBusyInfo();
	GetExpandedChildNodes(obj->GetId(),expandedNodes,list,&timer, NULL,h_path,0);
}
//waiting->~wxBusyInfo();

	wxFileName fn("");
	fn.MakeAbsolute();
wxSafeYield();
wxMilliSleep(50);
wxSafeYield();
//	return 0;

{
			wxBusyInfo waiting(wxString::Format(" Reading target DB Path = %s\n, Start object = %s",
			                                    browser->GetItemText(trgobj->GetId()).c_str(), trgobj->GetName().c_str(),parent));
			// Give the UI a chance to redraw
			wxSafeYield();
			wxMilliSleep(50);
			wxSafeYield();
timer=wxDateTime::GetTimeNow();
	wxArrayString expandedNodes2;
	expandedNodes2.Add(trg_server_replace);
	expandedNodes2.Add(trg_db_replace);
	expandedNodes2.Add(p_server_replace);
	expandedNodes2.Add(p_db_replace);
	GetExpandedChildNodes(trgobj->GetId(),expandedNodes2,list,&timer, NULL,h_path,0);
}
//waiting->~wxBusyInfo();

int e_count=list.GetCount();
int linecount=0;
int nstart=0,lastpos=0;
int c,minlen=100000,root=0;
wxArrayInt* child;
wxArrayPtrVoid level(15);
wxHashTable htab(wxKEY_STRING);
wxString key;
int npos=p2;
    for (int i = 0; i < e_count; ++i)
    {
        SQL& sq = list.Item(i);
		c=0;
		nstart=0;
		if ((nstart=sq.pathtree.rfind('/'))!=wxNOT_FOUND ) { 
			key=sq.pathtree.SubString(npos,nstart-1);
			if (minlen>key.Len()) {minlen=key.Len(); root=i;}
		}
		child=(wxArrayInt *)htab.Get(key);
		if (child==NULL) {
			child=new wxArrayInt;
			htab.Put(key,(wxObject *)child);
		}
		child->Add(i);
	
		linecount++;
	}
	
MyListSql::iterator iter2;
//e_count=list.GetCount();
//for (int i = 0; i < e_count; ++i)
//{
//		SQL& sq = list.Item(i);
//}
//	file2.Write(report, wxConvUTF8);
//	file2.Close();


	//return 0;
	titleline=wxEmptyString;
	list_head=wxEmptyString;
	rowlist=wxEmptyString;
	list_end=wxEmptyString;
	tableheader=wxEmptyString;
	head=wxEmptyString;
	tableheader2=wxEmptyString;
	tableshtml=wxEmptyString;

// �������� �������
#ifndef _DEBUG
	wxString fDir=wxStandardPaths::Get().GetExecutablePath().BeforeLast('\\')+wxT("\\");
#else
	wxString fDir=wxStandardPaths::Get().GetExecutablePath().BeforeLast('\\')+wxT("\\");
#endif
	wxString f=fDir+"textcompare_report.template";
	wxString buffer;
	
	wxUtfFile file3(f, wxFile::read, wxFONTENCODING_UTF8);
	if (file3.IsOpened())
	{
		file3.Read(buffer);
		file3.Close();
	} else
	{
		wxLogError(_("Failed to open file %s."), f.c_str());
		return 0;
	}
	buffer.Replace('\r',wxEmptyString,true);
	wxStringTokenizer lines(buffer, wxT("\n"));
	Diff_EditCost=4;
	Match_Threshold=0.5;
	Match_Distance=1000;
	
	bool mline=false;
	while (lines.HasMoreTokens())
	{
		wxString tmp = lines.GetNextToken();
		
		//if (tmp.length()>0 && tmp[tmp.length()-1]=='\r') tmp.RemoveLast();
		wxString line = tmp.Strip(wxString::both);
		if (tmp.StartsWith("@end@")) break;
		int l=wxString("@titleline").Len();
		if (tmp.StartsWith("@titleline")) {titleline=tmp.Mid(l)+"\n"; continue;}
		l=wxString("@[list").Len();
		if (tmp.StartsWith("@[list")) {list_head=tmp.Mid(l)+"\n"; continue;}
		l=wxString("@]list").Len();
		if (tmp.StartsWith("@]list")) {list_end=tmp.Mid(l)+"\n"; continue;}
		l=wxString("@rowlist").Len();
		if (tmp.StartsWith("@rowlist")) {rowlist=tmp.Mid(l)+"\n"; continue;}
		l=wxString("@tableheader2").Len();
		if (tmp.StartsWith("@tableheader2")) {tableheader2=tmp.Mid(l)+"\n"; continue;}
		
		if (tmp==wxString("@tableheader@")) {mline=!mline; continue;}
		if (mline) { tableheader+=tmp+"\n";continue;}
		// diff config
		if (tmp.StartsWith("@Match_Distance=")) {Match_Distance=(int)StrToDouble(tmp.After('=')); continue;}
		if (tmp.StartsWith("@Diff_EditCost=")) {Diff_EditCost=StrToDouble(tmp.After('=')); continue;}
		if (tmp.StartsWith("@Match_Threshold=")) {Match_Threshold=(float)StrToDouble(tmp.After('=')); continue;}
		
		head+=tmp+"\n";
	}
	//
{
			wxBusyInfo waiting(wxString::Format(" Search for differences ...",0));
			// Give the UI a chance to redraw
			wxSafeYield();
			wxMilliSleep(50);
			wxSafeYield();

	wxString content=printlvl(root,0,list,htab);
	wxString tmp(titleline);
	tmp.Replace("$titleline$","Left  : "+path);
	wxString tmp2(titleline);
	tmp2.Replace("$titleline$","Right: "+newpath);
	head.Replace("$titleline$",tmp+tmp2);

	head.Replace("$list$",content);
	head.Replace("$tables$",tableshtml);
	tableshtml=wxEmptyString;
	content=wxEmptyString;
}

	//head+="</div></body></html>";
    fDir=wxStandardPaths::Get().GetTempDir()+wxT("\\cmp.html");
	//fn="D:\\PostgreSQL\\cmp.html";
	fn=fDir;
	fn.MakeAbsolute();

	wxString html(head);
	wxFile file4(fn.GetFullPath(), wxFile::write);
	if (!file4.IsOpened())
	{
		wxLogError(_("Failed to open file %s."), fn.GetFullPath().c_str());
		return 0;
	}
	file4.Write(html, wxConvUTF8);
	file4.Close();
	head=wxEmptyString;

list.RemoveAt(0,list.GetCount());
e_count=list.GetCount();
for (int i = 0; i < e_count; ++i)
{
		//SQL *p=&sq;
	//list.RemoveAt(0,list.GetCount());
//		SQL *p = list.Detach(i);
//		delete p;
}
//htab.DeleteContents(true);
h_path.clear();
//MyHashSQL::iterator it, en;
//for( it = h_path.begin(), en = h_path.end(); it != en; ++it ) 
//{
//    h_path.erase(it);
//}
//htab.Clear();
//list.Empty();

#ifdef __WXMSW__
		wxLaunchDefaultBrowser(fn.GetFullPath());
#else
		wxLaunchDefaultBrowser(wxT("file://") + fn.GetFullPath());
#endif

	return 0;
}
#if defined(DELETE)
#undef DELETE
#endif // DUMMYSTRUCTNAME

std::wstring reportCompareFactory::printdiff(std::wstring str1, std::wstring str2 )
{
	  diff_match_patch dmp(Diff_EditCost,Match_Threshold,Match_Distance);
	  std::list<Diff> diffs;
	  if (str1==str2) {
		  return L"";
	  }
	  diffs=dmp.diff_main(str1,str2);
	  int nstart=0;
	  int pos=0;
	  countdiffline=0;
	  std::wstring cur_l;
	  std::wstring ncur_l; std::wstring p_ncur_l;std::wstring p_ncur_r;
	  std::wstring cur_r;
	  std::wstring ncur_r;
	  std::wstring tex;
	  std::wstring t;
	  std::wstring tableline;
	  int rline=1,lline=1;
	  std::list<Diff>::const_iterator it; // объявляем итератор
	  it = diffs.begin(); // присваиваем ему начало списка
	  Diff aDiff;
	  bool modify = false;
	  bool oneline = false;
	  nstart = 0;
	  while (it != diffs.end()) // пока итератор не достигнет конца
	  {
		aDiff=*it;
	    tex=aDiff.text;
		nstart=0;
	    while (nstart<tex.length()) {
			pos=tex.find('\n',nstart);
			if (pos==-1) {t.assign(tex,nstart,tex.length());nstart=tex.length();} else {t.assign(tex,nstart,pos-nstart);nstart=pos;}
			if (t.length()>0) {
				// это всё ещё одна строка
				if (aDiff.operation == Operation::INSERT) cur_r += L"<span class=\"differencei\">" + t + L"</span>";
				if (aDiff.operation == Operation::DELETE) cur_l += L"<span class=\"differenced\">" + t + L"</span>";
				if (aDiff.operation == Operation::EQUAL) {
					cur_r += t;
					cur_l += t;
				}
				else modify = true;
				// пока не встретим перевод строки считаем что это всё одна строка
				oneline = true;
			}
			else
			{
				// дошли до перевода \n
				nstart=pos+1;
				ncur_l=std::to_wstring(lline);
				ncur_l=L""; ncur_r=L"";
//				if (p_ncur_r==ncur_r) { ncur_r=L""; modify=true;}
//				if (p_ncur_l==ncur_l) { ncur_l=L""; modify=true;}
				
				std::wstring t_cur_l=cur_l;
				std::wstring t_cur_r=cur_r;
				if (aDiff.operation==Operation::DELETE) {
					t_cur_r=L"";
					ncur_l=std::to_wstring(lline);
					modify=true;
					lline++;
				} else  if (aDiff.operation==Operation::INSERT) {
					t_cur_l=L"";
					ncur_r=std::to_wstring(rline);
					modify=true;
					rline++;
				} else if (aDiff.operation==Operation::EQUAL) {
					ncur_r=std::to_wstring(rline);
					ncur_l=std::to_wstring(lline);
					rline++; lline++;
				}
				if (modify) countdiffline++;
//				if (( (ncur_r.empty()&&(!ncur_l.empty()))
//					||(ncur_l.empty()&&(!ncur_r.empty()))
//					)&&(!modify)) modify=true;
				// create columns
				//left
					tableline+=L"<tr><td class=\"diff_next\" onclick=\"c(this)\"></td>";
					 tableline+= modify ? L"<td class=\"has_difference\" onclick=\"d(this)\">"+ncur_l+"</td>" : L"<td class=\"diff_header\" onclick=\"d(this)\">"+ncur_l+"</td>";
					tableline+=L"<td class=\"lineContent\"><pre>"+t_cur_l+"</pre></td>";
				// right
				tableline+=L"<td class=\"diff_next\" onclick=\"c(this)\"></td>";
				tableline+= modify ? L"<td class=\"has_difference\" onclick=\"d(this)\">"+ncur_r+"</td>" : L"<td class=\"diff_header\" onclick=\"d(this)\">"+ncur_r+"</td>";
				tableline+=L"<td class=\"lineContent\"><pre>"+t_cur_r+"</pre></td>";
				tableline+=L"</tr>";



				if (ncur_r.length()>0) p_ncur_r=ncur_r;
				if (ncur_l.length()>0) p_ncur_l=ncur_l;
				
				if (aDiff.operation==Operation::DELETE) {
					cur_l=L"";
				} else  if (aDiff.operation==Operation::INSERT) {
					cur_r=L"";
				} else if (aDiff.operation==Operation::EQUAL) {
					cur_r=L"";cur_l=L"";
				}
				
				//
				modify=false;
				oneline=false;
			}
		} // цикл по строкам внутри одного Diff
	  ++it;
	  }
#ifdef _DEBUG
	wxFileName fn("cmp_debug.txt");
	fn.MakeAbsolute();

	wxFile file(fn.GetFullPath(), wxFile::write);
	if (!file.IsOpened())
	{
		wxLogError(_("Failed to open file %s."), fn.GetFullPath().c_str());
	}

wxString report;
	report.Append(wxString::Format(" Table\n%s\n",tableline));
	file.Write(report, wxConvUTF8);
	file.Close();

#endif

return tableline;
}
wxString reportCompareFactory::printlvl(int element,int lvl,ArraySQL &list, wxHashTable &htab)
{
	wxString l(list_head);

	wxString r=wxEmptyString;
	wxArrayInt* child;
	wxString key,name;
	int e=element;
	int nstart;
		SQL& sq=list.Item(e);
		if ((nstart=sq.pathtree.rfind('/'))!=wxNOT_FOUND ) { 
			name=sq.pathtree.AfterLast('/');
			key=sq.pathtree.Mid(startpathpos);
			
		}
		wxString tid=wxString::Format("id%d",e);
		wxString rlist=HtmlEntities(name);
		wxString cdiff=wxEmptyString;
		// таблица различий
		countdiffline = 0;
		if (sq.sql.length() + sq.sql2.length() > 0) {
			// для одинаковых не будем таблицу формировать
			std::wstring t1=sq.sql.wc_str();
			std::wstring t2(sq.sql2.wc_str());
			t1.erase(std::remove(t1.begin(), t1.end(), '\r'), t1.end());
			t2.erase(std::remove(t2.begin(), t2.end(), '\r'), t2.end());
			std::wstring rez=printdiff(t1,t2);
			if (rez.length()>0) {
				//L"</tbody></table>"
				//tableshtml
				wxString tmp(tableheader);
				tmp.Replace("@idtablecmp@",tid);
				tmp.Replace("$rowlist$",rlist);
				tableshtml+=tmp+"\n";
				tmp=tableheader2;
				tmp.Replace("@idtablecmp@",tid);
				tmp.Replace("$rowlist$",rlist);
				tableshtml+=tmp;
				tableshtml+=rez;
				tableshtml+="</tbody></table>\n";
				cdiff=wxString::Format(" ( %d )",countdiffline);
			}
		}

		// список объектов
		child=(wxArrayInt *)htab.Get(key);
		r=rowlist;
		r.Replace("$rowlist$",rlist+cdiff);
		if (sq.mode==__Insert) r.Replace("@color@","insert");
				else if (sq.mode==__Remove) r.Replace("@color@","remove");
				else if (countdiffline>0) r.Replace("@color@","ne"); else {
					if (child!=NULL) r.Replace("@color@","eq"); else r.Replace("@color@","eqhidden");
				}
		r.Replace("@idtablecmp@",tid);	
		l+=r;
		if (child!=NULL) {
			for (int j=0;j<child->GetCount();j++) {
				l+=printlvl(child->Item(j),lvl+1,list,htab);
			}
		}
	l+=list_end;
	return l;
}

///////////////////////////////////////////////////////
// Report base
///////////////////////////////////////////////////////
wxWindow *reportBaseFactory::StartDialog(frmMain *form, pgObject *obj)
{
	parent = form;

	wxBeginBusyCursor();
	frmReport *report = new frmReport(GetFrmMain());

	// Generate the report header
	wxDateTime now = wxDateTime::Now();
	report->XmlAddHeaderValue(wxT("generated"), now.Format(wxT("%c")));
	if (obj->GetServer())
		report->XmlAddHeaderValue(wxT("server"), obj->GetServer()->GetFullIdentifier());
	if (obj->GetDatabase())
		report->XmlAddHeaderValue(wxT("database"), obj->GetDatabase()->GetName());
	if (obj->GetSchema())
	{
		if (obj->GetSchema()->GetMetaType() == PGM_CATALOG)
			report->XmlAddHeaderValue(wxT("catalog"), obj->GetSchema()->GetDisplayName());
		else
			report->XmlAddHeaderValue(wxT("schema"), obj->GetSchema()->GetName());
	}
	if (obj->GetJob())
		report->XmlAddHeaderValue(wxT("job"), obj->GetJob()->GetName());
	if (obj->GetTable())
		report->XmlAddHeaderValue(wxT("table"), obj->GetTable()->GetName());

	GenerateReport(report, obj);
	wxEndBusyCursor();

	report->ShowModal();
	return 0;
}

///////////////////////////////////////////////////////
// Properties report
///////////////////////////////////////////////////////
reportObjectPropertiesFactory::reportObjectPropertiesFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar)
	: reportBaseFactory(list)
{
	mnu->Append(id, _("&Properties Report"), _("Generate a Properties report for this object."));
}

bool reportObjectPropertiesFactory::CheckEnable(pgObject *obj)
{
	if (obj)
	{
		if (obj->IsCollection())
			return false;
		else
			return true;
	}
	return false;
}

void reportObjectPropertiesFactory::GenerateReport(frmReport *report, pgObject *object)
{
	report->SetReportTitle(object->GetTranslatedMessage(PROPERTIESREPORT));

	int section = report->XmlCreateSection(object->GetTranslatedMessage(PROPERTIES));

	ctlListView *list = GetFrmMain()->GetProperties();
	object->ShowProperties();

	report->XmlAddSectionTableFromListView(section, list);

	report->XmlSetSectionSql(section, object->GetSql(NULL));
}

///////////////////////////////////////////////////////
// DDL report
///////////////////////////////////////////////////////
reportObjectDdlFactory::reportObjectDdlFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar)
	: reportBaseFactory(list)
{
	mnu->Append(id, _("&DDL Report"), _("Generate a DDL report for this object."));
}

bool reportObjectDdlFactory::CheckEnable(pgObject *obj)
{
	if (obj)
	{
		if (obj->GetMetaType() == PGM_SERVER || obj->GetMetaType() == PGM_CATALOGOBJECT || obj->IsCollection())
			return false;
		else
			return true;
	}
	return false;
}

void reportObjectDdlFactory::GenerateReport(frmReport *report, pgObject *object)
{
	report->SetReportTitle(object->GetTranslatedMessage(DDLREPORT));

	int section = report->XmlCreateSection(object->GetTranslatedMessage(DDL));

	report->XmlSetSectionSql(section, object->GetSql(NULL));
}

///////////////////////////////////////////////////////
// Data dictionary report
///////////////////////////////////////////////////////
reportObjectDataDictionaryFactory::reportObjectDataDictionaryFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar)
	: reportBaseFactory(list)
{
	mnu->Append(id, _("&Data Dictionary Report"), _("Generate a Data Dictionary report for this object."));
}

bool reportObjectDataDictionaryFactory::CheckEnable(pgObject *obj)
{
	if (obj)
	{
		if ((obj->GetMetaType() == PGM_TABLE || obj->GetMetaType() == GP_PARTITION) && !obj->IsCollection())
			return true;
		else
			return false;
	}
	return false;
}

void reportObjectDataDictionaryFactory::GenerateReport(frmReport *report, pgObject *object)
{

	pgTable *table = (pgTable *)object;

	report->SetReportTitle(object->GetTranslatedMessage(DATADICTIONNARYREPORT));

	// Columns
	int section = report->XmlCreateSection(_("Columns"));

	report->XmlSetSectionTableHeader(section, 6, (const wxChar *) _("Name"), (const wxChar *) _("Data type"), (const wxChar *) _("Not Null?"), (const wxChar *) _("Primary key?"), (const wxChar *) _("Default"), (const wxChar *) _("Comment"));

	ctlTree *browser = GetFrmMain()->GetBrowser();
	pgCollection *columns = table->GetColumnCollection(browser);

	treeObjectIterator colIt(browser, columns);

	pgColumn *column;
	bool haveInherit = false;
	wxString colName;
	while ((column = (pgColumn *)colIt.GetNextObject()) != 0)
	{
		column->ShowTreeDetail(browser);
		if (column->GetColNumber() > 0)
		{
			colName = column->GetName();
			if (column->GetInheritedCount() > 0)
			{
				colName += _("*");
				haveInherit = true;
			}

			report->XmlAddSectionTableRow(section,
			                              column->GetColNumber(),
			                              6,
			                              (const wxChar *) colName,
			                              (const wxChar *) column->GetVarTypename(),
			                              (const wxChar *) BoolToYesNo(column->GetNotNull()),
			                              (const wxChar *) BoolToYesNo(column->GetIsPK()),
			                              (const wxChar *) column->GetDefault(),
			                              (const wxChar *) column->GetComment());
		}
	}
	if (haveInherit)
	{
		wxString info;
		info.Printf(_("* Inherited columns from %s."), table->GetInheritedTables().c_str());
		report->XmlSetSectionTableInfo(section, info);
	}

	// Constraints
	pgCollection *constraints = table->GetConstraintCollection(browser);

	treeObjectIterator conIt(browser, constraints);

	pgObject *constraint;
	long x = 1;
	wxString definition, type;
	while ((constraint = (pgObject *)conIt.GetNextObject()) != 0)
	{
		if (x == 1)
		{
			section = report->XmlCreateSection(_("Constraints"));
			report->XmlSetSectionTableHeader(section, 4, (const wxChar *) _("Name"), (const wxChar *) _("Type"), (const wxChar *) _("Definition"), (const wxChar *) _("Comment"));
		}

		constraint->ShowTreeDetail(browser);

		switch (constraint->GetMetaType())
		{
			case PGM_PRIMARYKEY:
				type = _("Primary key");
				definition = ((pgIndexConstraint *)constraint)->GetDefinition();
				break;
			case PGM_UNIQUE:
				type = _("Unique");
				definition = ((pgIndexConstraint *)constraint)->GetDefinition();
				break;
			case PGM_FOREIGNKEY:
				type = _("Foreign key");
				definition = ((pgForeignKey *)constraint)->GetDefinition();
				break;
			case PGM_EXCLUDE:
				type = _("Exclude");
				definition = ((pgIndexConstraint *)constraint)->GetDefinition();
				break;
			case PGM_CHECK:
				type = _("Check");
				definition = wxT("(") + ((pgCheck *)constraint)->GetDefinition() + wxT(")");
				break;
		}

		report->XmlAddSectionTableRow(section,
		                              x,
		                              4,
		                              (const wxChar *) constraint->GetName(),
		                              (const wxChar *) type,
		                              (const wxChar *) definition,
		                              (const wxChar *) constraint->GetComment());
		x++;
	}
}

///////////////////////////////////////////////////////
// Statistics report
///////////////////////////////////////////////////////
reportObjectStatisticsFactory::reportObjectStatisticsFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar)
	: reportBaseFactory(list)
{
	mnu->Append(id, _("&Statistics Report"), _("Generate a Statistics report for this object."));
}

bool reportObjectStatisticsFactory::CheckEnable(pgObject *obj)
{
	if (obj)
	{
		if (!obj->HasStats())
		{
			if (obj->IsCollection())
			{
				pgaFactory *f = obj->GetFactory();

				if (f)
				{
					if (f->GetMetaType() == PGM_TABLE ||
					        f->GetMetaType() == GP_PARTITION ||
					        f->GetMetaType() == PGM_TABLESPACE ||
					        f->GetMetaType() == PGM_DATABASE)
						return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}

		}
		else
		{
			return true;
		}
	}

	return false;
}

void reportObjectStatisticsFactory::GenerateReport(frmReport *report, pgObject *object)
{
	report->SetReportTitle(object->GetTranslatedMessage(STATISTICSREPORT));

	int section = report->XmlCreateSection(object->GetTranslatedMessage(OBJSTATISTICS));

	ctlListView *list = GetFrmMain()->GetStatistics();
	object->ShowStatistics(GetFrmMain(), list);

	report->XmlAddSectionTableFromListView(section, list);
}

///////////////////////////////////////////////////////
// Dependencies report
///////////////////////////////////////////////////////
reportObjectDependenciesFactory::reportObjectDependenciesFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar)
	: reportBaseFactory(list)
{
	mnu->Append(id, _("&Dependencies Report"), _("Generate a Dependencies report for this object."));
}

bool reportObjectDependenciesFactory::CheckEnable(pgObject *obj)
{
	if (obj)
	{
		if (!obj->HasDepends())
			return false;
		else
			return true;
	}
	return false;
}

void reportObjectDependenciesFactory::GenerateReport(frmReport *report, pgObject *object)
{
	report->SetReportTitle(object->GetTranslatedMessage(DEPENDENCIESREPORT));

	int section = report->XmlCreateSection(object->GetTranslatedMessage(DEPENDENCIES));

	ctlListView *list = GetFrmMain()->GetDependencies();
	object->ShowDependencies(parent, list);

	report->XmlAddSectionTableFromListView(section, list);
}

///////////////////////////////////////////////////////
// Dependents report
///////////////////////////////////////////////////////
reportObjectDependentsFactory::reportObjectDependentsFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar)
	: reportBaseFactory(list)
{
	mnu->Append(id, _("&Dependents Report"), _("Generate a Dependents report for this object."));
}

bool reportObjectDependentsFactory::CheckEnable(pgObject *obj)
{
	if (obj)
	{
		if (!obj->HasReferences())
			return false;
		else
			return true;
	}
	return false;
}

void reportObjectDependentsFactory::GenerateReport(frmReport *report, pgObject *object)
{
	report->SetReportTitle(object->GetTranslatedMessage(DEPENDENTSREPORT));

	int section = report->XmlCreateSection(object->GetTranslatedMessage(DEPENDENTS));

	ctlListView *list = GetFrmMain()->GetReferencedBy();
	object->ShowDependents(parent, list);

	report->XmlAddSectionTableFromListView(section, list);
}

///////////////////////////////////////////////////////
// Object list report
///////////////////////////////////////////////////////
reportObjectListFactory::reportObjectListFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar)
	: reportBaseFactory(list)
{
	mnu->Append(id, _("&Object List Report"), _("Generate an Object List report for this collection."));
}

bool reportObjectListFactory::CheckEnable(pgObject *obj)
{
	if (obj)
	{
		if (!obj->IsCollection())
		{
			return false;
		}
		else
		{
			pgaFactory *f = obj->GetFactory();
			if (obj)
			{
				if (f->GetMetaType() == PGM_SERVER)
					return false;
			}
			else
				return false;

			return true;
		}
	}
	return false;
}

void reportObjectListFactory::GenerateReport(frmReport *report, pgObject *object)
{
	report->SetReportTitle(object->GetTranslatedMessage(OBJECTSLISTREPORT));

	int section = report->XmlCreateSection(object->GetFullIdentifier());

	ctlListView *list = GetFrmMain()->GetProperties();
	object->ShowProperties();

	report->XmlAddSectionTableFromListView(section, list);
}


