
#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include <wx/regex.h>
#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/textbuf.h>
#include <wx/sysopt.h>

// wxAUI
#include <wx/aui/aui.h>

// App headers
#include "frm/frmMain.h"
#include "frm/frmLog.h"
#include "db/pgConn.h"
#include "utils/pgfeatures.h"
#include "schema/pgServer.h"
#include "schema/pgUser.h"
#include "ctl/ctlMenuToolbar.h"
#include "ctl/ctlAuiNotebook.h"
#include "utils/csvfiles.h"
#include "log/StorageModel.h"
#include "utils/utffile.h"
#include "utils/misc.h"

#include <wx/arrimpl.cpp>
#ifdef WIN32
#include <wx/msw/ole/automtn.h>
#endif


WX_DEFINE_OBJARRAY(RemoteConnArray2);


wxBEGIN_EVENT_TABLE(frmLog, pgFrame)
// test
EVT_MENU(MNU_FIND_TEXT, frmLog::OnFind)
EVT_MENU(MNU_SEND_MAIL, frmLog::OnSendMail)
EVT_CHECKBOX(ID_SET_GROUP, frmLog::OnSetGroup)
EVT_CHECKBOX(ID_SET_DETAILGROUP, frmLog::OnSetDetailGroup)
EVT_BUTTON(ID_CLEAR_ALL_FILTER, frmLog::OnClearAllFilter)
EVT_BUTTON(ID_ADD_FILTER, frmLog::OnAddFilterIgnore)
EVT_BUTTON(ID_ADD_UFilter, frmLog::OnAddUFilter)
EVT_BUTTON(ID_DEL_UFilter, frmLog::OnDelUFilter)
EVT_BUTTON(ID_HELP_LOG, frmLog::OnHelp)
EVT_COMBOBOX(ID_CBOX_UFilter, frmLog::OnChangeUFilter)
EVT_COMBOBOX(ID_CBOX_SMART, frmLog::OnChangeSmart)
EVT_SET_FOCUS(frmLog::OnSetFocus)
EVT_CLOSE(frmLog::OnClose)
EVT_KILL_FOCUS(frmLog::OnKillFocus)
EVT_ACTIVATE(frmLog::OnActivate)
wxEND_EVENT_TABLE()

//#include <wx/arrimpl.cpp>
//WX_DEFINE_OBJARRAY(RemoteConnArray);


void frmLog::OnActivate(wxActivateEvent& event) {
	m_storage_model->getStorage()->SetErrMsgFlag(false);
	seticon(false);
	event.Skip();
}
void frmLog::OnSetFocus(wxFocusEvent& event) {
	m_storage_model->getStorage()->SetErrMsgFlag(false);
	seticon(false);
}
void frmLog::OnKillFocus(wxFocusEvent& event) {
	m_storage_model->getStorage()->SetErrMsgFlag(false);
	seticon(false);

}
void frmLog::OnClose(wxCloseEvent& event) {
	if (event.CanVeto()&& detail->IsChecked())
	{
		my_view->setGroupMode(true);
		detail->SetValue(false);
	//detail->Enable(event.IsChecked());
		event.Veto();
		return;
	}
	event.Skip();
}

// Class declarations
void frmLog::OnClearAllFilter(wxCommandEvent& event) {

	my_view->ClearAllFilter(false);
}
void frmLog::OnAddFilterIgnore(wxCommandEvent& event) {
	wxString Fname = "LoadSkip";
	my_view->ModUserFilter(Fname, "AddUserFilter", listUserFilter, contentFilter);
	//my_view->AddFilterIgnore(Fname);
}
void frmLog::OnAddUFilter(wxCommandEvent& event) {
	wxString txt = listUserFilter->GetValue();
	my_view->ModUserFilter(txt, "AddUserFilter", listUserFilter, contentFilter);

}
void frmLog::OnDelUFilter(wxCommandEvent& event) {
	my_view->ModUserFilter("", "RemoveFilter", listUserFilter, contentFilter);
}
void frmLog::OnChangeUFilter(wxCommandEvent& event) {
	if (event.GetSelection() >= 0) {
		my_view->ModUserFilter("", "ChangeFilter", listUserFilter, contentFilter);
	}
}
void frmLog::OnChangeSmart(wxCommandEvent& event) {
	int n = event.GetSelection();
	if (n >= 0) {
		listUserFilter->SetSelection(n);
		my_view->ModUserFilter("", "ChangeFilter", listUserFilter, contentFilter);
	}
}
void frmLog::OnSendMail(wxCommandEvent& event) {
	//wxMessageBox("send mail");
	wxDataViewItem item;
	item = my_view->GetCurrentItem();
	if (!item.IsOk() ) {
		return;
	}
	wxVariant v, t;
	StorageModel* m = dynamic_cast<StorageModel*>(my_view->GetModel());
	Storage* st = m->getStorage();
	wxArrayString a;
	int r = m->GetRow(item);
	if (r != -1) {
		a=st->GetAllFields(r, st->IsFilter());
	}
	else return;

	wxString str = "";
#ifdef DEBUG
	wxString fn = "C:\\Users\\lsv\\Source\\Repos\\pgadmin64\\pgadmin\\x64\\Debug_(3.0)\\testhtml.txt" ;
#else
	wxString fn = "mail.template";
#endif // DEBUG
	wxUtfFile file(fn, wxFile::read, wxFONTENCODING_UTF8);
	if (file.IsOpened())
	{
		file.Read(str);
		file.Close();
		wxStringTokenizer tk(str, "\n", wxTOKEN_DEFAULT);
		wxString cc;
		wxString to;
		wxString html;
		wxString templat;
		while (tk.HasMoreTokens())
		{
			wxString l = tk.GetNextToken();
			if (l.StartsWith("Cc=")) { cc = l.After('='); continue; }
			if (l.StartsWith("To=")) { to = l.After('='); continue; }
			if (l.StartsWith("<tr>")) {
				wxString le;
				wxString r;
				for (int i = 0; i < a.Count(); i++) {
					templat = l;
					le = escapeHtml(a[i++],false);
					r = escapeHtml(a[i],false);
					int co = templat.Replace("$1", le);
					co += templat.Replace("$2", r);
					html.Append(templat);
				}
				continue;
			}
			html.Append(l);
		}
		#ifdef WIN32
		wxAutomationObject oObject;
		if (oObject.GetInstance("Outlook.Application")) {
			wxAutomationObject msg;
			wxVariant n[1];
			wxString strI;
			int i = 0;
			strI << i;
			n[0] = wxVariant(strI);
			bool rez = oObject.GetObject(msg, "CreateItem", 1, n);
			if (rez) {
				//oObject.PutProperty("Visible", true);
				msg.PutProperty("Subject", "Error ");
				msg.PutProperty("BodyFormat", 2);
				msg.PutProperty("To", to);
				//
				msg.PutProperty("Cc", cc);
				msg.PutProperty("HTMLBody", html);
				msg.CallMethod("Display");
			}
			//oObject.PutProperty("ActiveCell.Font.Bold", @true);

		}
		#endif
	}
}
void frmLog::OnFind(wxCommandEvent& event) {

}
void frmLog::OnHelp(wxCommandEvent& event) {

	wxMessageBox(wxString::FromUTF8("Для включения фильтра нужно:\n"
		" Щелкнуть правой кнопкой мыши по полю.Для инверсии фильтра нужно удерживать Ctrl.\n"
		" Выбрать значение в контекстном меню заголовка колонки.\n"
		" Там отображаются 20 самых частых значения в колонке с указанием количества этих значений.\n"
		" Ввести в поле значения для фильтра, выделить это значение и нажать Enter.\n"
		" Для фильтра используется только выделенный текст.\n"
		" Такой фильтр будет работать на поиск выделенного вхождения в поле.\n"
		" Если в выделенной строке первым символом будет \"!\" то фильтр инверсируется.\n"
		"\n"
		"Shift+KeyUP,Shift+KeyDOWN - переход на запись с тем же sql_state.\n"
		"Alt+KeyUP,Alt+KeyDOWN     - переход на запись с другим sql_state.\n"
		"Ctrl + S                  - отправка строки лога по почте Outlook. \n"
		"                          Шаблон письма в файле mail.template в первых двух строках шаблона можно указать адреса которые будут подставляться в письмо.\n"
	));
}

void frmLog::OnSetGroup(wxCommandEvent& event)
{
    //wxDataViewColumn* const col = m_ctrl[Page_List]->GetColumn(0);
    if (event.IsChecked())
    {
        //wxLogMessage("Group set check");
        my_view->setGroupMode(true);
        detail->SetValue(false);
    }
    else {
        //wxLogMessage("Group unset check");
        my_view->setGroupMode(false);
    }
    detail->Enable(event.IsChecked());
}
bool frmLog::CheckConn(wxString host,int port) {
	for (size_t i = 0; i < conArray.GetCount(); i++)
	{
		if ((conArray[i].conn->GetHostName() == host)&&(conArray[i].conn->GetPort() == port))
			return true;
	}
	return false;
}
pgConn* frmLog::createConn(pgServer* srv) {
	pgConn* conn;
	
	if (!srv->GetConnected()) mainForm->ReconnectServer(srv, false);
	conn = srv->CreateConn(wxEmptyString, 0, "Log conn");

	return conn;
}
void frmLog::AddNewConn(pgConn* con) {
	if (con != NULL) {
		if (!con->HasFeature(FEATURE_CSVLOG)) return;
		logfileName.Add("");
		savedPartialLine.Add("");
		logfileLength.Add(0);
		len.Add(0);
		conArray.Add(new RemoteConn2(con));
	}
}
void frmLog::OnSetDetailGroup(wxCommandEvent& event)
{
    //wxDataViewColumn* const col = m_ctrl[Page_List]->GetColumn(0);
    if (event.IsChecked())
    {
        my_view->ViewGroup(true);

    }
    else {
        my_view->ViewGroup(false);
    }
}

void frmLog::getFilename() {
    pgSet* set;
	wxString namepage;
	for (size_t i = 0;i< conArray.GetCount(); i++) {

		if (!namepage.IsEmpty()) namepage += ",";

		if (!conArray[i].conn->IsAlive()) {
			wxDateTime n = wxDateTime::Now();
			if (conArray[i].nextrun < n) {
				if (!conArray[i].conn->Reconnect())
				{
					wxTimeSpan sp(0, 5);
					conArray[i].nextrun = wxDateTime::Now() + sp;
					namepage += "-" + conArray[i].conn->GetDbname();
					continue;
				}
			}
			else {
				namepage += "?" + conArray[i].conn->GetDbname();
				continue;
			}

			
		}
		set = conArray[i].conn->ExecuteSet(
			wxT("select current_setting('log_directory')||'/'||name filename,modification filetime,size len\n")
			wxT("  FROM pg_ls_logdir()  where name ~ '.csv' ORDER BY modification DESC"));
		if (set)
		{

			//logfileTimestamp = set->GetDateTime(wxT("filetime"));
			len[i] = set->GetLong(wxT("len"));
			namepage += conArray[i].conn->GetDbname();
			m_storage_model->getStorage()->SetHost(conArray[i].conn->GetHostName());

			wxString fn = set->GetVal(wxT("filename"));
			if (fn != logfileName[i]) {
				logfileLength[i] = 0;
				logfileName[i] = fn;
				
			}
			/// addLogFile(logfileName, logfileTimestamp, len, logfileLength, skipFirst);

			delete set;
			readLogFile(logfileName[i],len[i],logfileLength[i],savedPartialLine[i],conArray[i].conn);
		}
	}
	if (namepage.IsEmpty()) namepage = "not connect";
	if (m_notebook->GetPageText(0) != namepage) m_notebook->SetPageText(0, namepage);
}
void frmLog::readLogFile(wxString logfileName,long& lenfile,long& logfileLength,wxString& savedPartialLine,pgConn* conn) {
	wxString line;

	// If GPDB 3.3 and later, log is normally in CSV format.  Let's get a whole log line before calling addLogLine,
	// so we can do things smarter.

	// PostgreSQL can log in CSV format, as well as regular format.  Normally, we'd only see
	// the regular format logs here, because pg_logdir_ls only returns those.  But if pg_logdir_ls is
	// changed to return the csv format log files, we should handle it.

	bool csv_log_format = logfileName.Right(4) == wxT(".csv");

	if (csv_log_format && savedPartialLine.length() > 0)
	{
		if (logfileLength == 0)  // Starting at beginning of log file
			savedPartialLine.clear();
		else
			line = savedPartialLine;
	}
	wxString funcname = "pg_read_binary_file(";
	while (lenfile > logfileLength)
	{
		//statusBar->SetStatusText(_("Reading log from server..."));
		pgSet* set = conn->ExecuteSet(wxT("SELECT ") + funcname +
			conn->qtDbString(logfileName) + wxT(", ") + NumToStr(logfileLength) + wxT(", 50000)"));
		if (!set)
		{
			conn->IsAlive();
			return;
		}
		char* raw1 = set->GetCharPtr(0);

		if (!raw1 || !*raw1)
		{
			delete set;
			break;
		}
		char* raw;
		unsigned char m[50001];
		if (settings->GetASUTPstyle()||true) {

			raw = (char*)&m[0];
			unsigned char c;
			unsigned char* startChar;
			int pos = 0;
			raw1 = raw1 + 2;
			int utf8charLen = 0;
			while (*raw1 != 0) {
				c = *raw1;
				c = c - '0';
				if (c > 9) c = *raw1 - 'a' + 10;
				raw1++;
				m[pos] = c << 4;
				c = *raw1 - '0';
				if (c > 9) c = *raw1 - 'a' + 10;
				c = c | m[pos];
				m[pos] = c;
				// check utf-8 char
				if (utf8charLen == 0) {
					startChar = &m[pos];
					if (c >> 7 == 0)
						utf8charLen = 1;
					else if (c >> 5 == 0x6)
						utf8charLen = 2;
					else if (c >> 4 == 0xE)
						utf8charLen = 3;
					else if (c >> 5 == 0x1E)
						utf8charLen = 4;
					else
						utf8charLen = 0;
					// bad utf8 format
				}
				pos++;
				raw1++;
				utf8charLen--;
			}
			// 
			if (utf8charLen != 0) {
				//read = startChar - &m[0];
				// remove bad utf-8 char
				*startChar = 0;
			}
			else
				m[pos] = 0;
		}
		else {
			raw = raw1;
		}
		int l= strlen(raw);
		logfileLength += l;
		wxString host = conn->GetHostName();
		status->SetLabelText(wxString::Format("%s Load bytes %ld", host,logfileLength));
		wxString str;
		str = line + wxTextBuffer::Translate(wxString(raw, set->GetConversion()), wxTextFileType_Unix);
		//if (wxString(wxString(raw, wxConvLibc).wx_str(), wxConvUTF8).Len() > 0)
		//	str = line + wxString(wxString(raw, wxConvLibc).wx_str(), wxConvUTF8);
		//else {
		//	str = line + wxTextBuffer::Translate(wxString(raw, set->GetConversion()), wxTextFileType_Unix);
		//}


		delete set;

		if (str.Len() == 0)
		{
			wxString msgstr = _("The server log contains entries in multiple encodings and cannot be displayed by pgAdmin.");
			wxMessageBox(msgstr);
			return;
		}

		if (csv_log_format)
		{
			// This will work for any DB using CSV format logs


			CSVLineTokenizer tk(str);

			
			my_view->Freeze();
			while (tk.HasMoreLines())
			{
				line.Clear();

				bool partial;
				str = tk.GetNextLine(partial);
				if (partial)
				{
					line = str; // Start of a log line, but not complete.  Loop back, Read more data.
					break;
				}


				// Looks like we have a good complete CSV log record.
				
				//addLogLine(str.Trim(), true, true);
				my_view->AddRow(str.Trim());
			}

			my_view->Thaw();
		}
		else
		{
		}
	}

	savedPartialLine.clear();

	if (!line.IsEmpty())
	{
		// We finished reading to the end of the log file, but still have some data left
		if (csv_log_format)
		{
			savedPartialLine = line;    // Save partial log line for next read of the data file.
			line.Clear();
		}
		else
			my_view->AddRow(line.Trim());
	}


}
void frmLog::OnTimer(wxTimerEvent& event) {
	Storage* st = m_storage_model->getStorage();
	//int rows = st->getCountStore();
	int ra, ri;
	st->ClearRowsStat();
	getFilename();
	st->GetRowsStat(ra, ri);
	//int newrows = st->getCountStore();
	//if (loglen !=logfileLength) 
	if (m_storage_model->getStorage()->GetErrMsgFlag()) {
		seticon(true);
	}
	status->SetLabelText(wxString::Format("Add rows %d ignore %d. View rows %d", ra,ri, m_storage_model->GetRowCount()) );

}
#include "log/log_xpm.xpm"
#include "log/log_red_xpm.xpm"
void frmLog::seticon(bool errflag) {
	//wxImage img = *sql_32_png_img;
	if (errflag) {
		SetIcon(idefRed);
	}
	else {
		SetIcon(idef);
	}
	return;

	wxBitmap* b = new wxBitmap(log_xpm);
	
	//wxIcon ico=img.
	wxMemoryDC dc(*b);
	dc.SetBrush(*wxYELLOW_BRUSH);
	
	//dc.SetBackground(*wxYELLOW_BRUSH);
	dc.SetBackground(*wxRED_BRUSH);
	dc.SetBrush(*wxRED_BRUSH);

	//dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetTextForeground(*wxRED);
	dc.SetPen(*wxRED_PEN);
	wxRect rect(7,4,7, 7);

	if (errflag) dc.DrawRoundedRectangle(rect, 0);

//	wxFont font = wxFont(5, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
//	wxFontStyle s;
//	font.SetStyle(wxFONTFLAG_ANTIALIASED);
//	dc.SetFont(font);

	wxImage img = b->ConvertToImage();
	dc.SelectObject(wxNullBitmap);
	int w = img.GetWidth();
	int h = img.GetHeight();
	wxColor p;
	wxColor c= wxColor(0,255,0);
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			p = wxColour(img.GetRed(x, y),
				img.GetGreen(x, y),
				img.GetBlue(x, y));
			//if (img.IsTransparent(x, y)) img.SetRGB(x, y, 255, 255, 255);
			if (p == c) {
				//img.SetTra
				img.SetAlpha(x, y, 255);
				img.SetRGB(x, y, img.GetMaskRed(), img.GetMaskGreen(), img.GetMaskBlue());
				;
			};
		}

	//wxIcon* ico = new wxIcon();
	wxIcon ico=GetIcon();
	wxBitmap* bmp = new wxBitmap(img);
	ico.CopyFromBitmap(*bmp);
	//SetIcon(*sql_32_png_ico);

	SetIcon(ico);
	

}
frmLog::frmLog(frmMain *form, const wxString &_title, pgServer *srv) : pgFrame(NULL, _title)
{

	dlgName = wxT("frmLog");
	RestorePosition(-1, -1, 700, 500, 700, 500);
	//SetIcon(wxIcon(log_xpm));
	idef = wxIcon(log_xpm);
	idefRed = wxIcon(log_red_xpm);
	seticon(false);
	mainForm = form;
	// Setup accelerators
	wxAcceleratorEntry entries[2];
	entries[0].Set(wxACCEL_CTRL, (int)'F', MNU_FIND);
	entries[1].Set(wxACCEL_CTRL, (int)'S', MNU_SEND_MAIL);
	wxAcceleratorTable accel(2, entries);
	SetAcceleratorTable(accel);
	SetAcceleratorTable(accel);

    m_notebook = new wxNotebook( this, wxID_ANY );

    wxPanel* testPanel = new wxPanel(m_notebook, wxID_ANY);
    //BuildDataViewCtrl(testPanel, Page_Test);
    my_view = new MyDataViewCtrl(testPanel, wxID_ANY, wxDefaultPosition,
        wxDefaultSize,  wxDV_VARIABLE_LINE_HEIGHT | wxDV_HORIZ_RULES | wxDV_VERT_RULES);
    my_view->GetMainWindow()->Bind(wxEVT_MOTION, &MyDataViewCtrl::OnMouseMove, my_view);
    my_view->GetMainWindow()->Bind(wxEVT_KEY_DOWN, &MyDataViewCtrl::OnKEY_DOWN, my_view);
    my_view->GetMainWindow()->Bind(wxEVT_KEY_UP, &MyDataViewCtrl::OnKEY_UP, my_view);
    my_view->Bind(wxEVT_DATAVIEW_COLUMN_HEADER_CLICK, &MyDataViewCtrl::OnEVT_DATAVIEW_COLUMN_HEADER_CLICK, my_view);
    my_view->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &MyDataViewCtrl::OnEVT_DATAVIEW_SELECTION_CHANGED, my_view);
    my_view->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &MyDataViewCtrl::OnContextMenu, my_view);

    // my_view->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &MyDataViewCtrl::OnContextMenu, my_view);

    m_timer.Bind(wxEVT_TIMER, &frmLog::OnTimer, this);
    



    my_view->Bind(wxEVT_MENU, &MyDataViewCtrl::OnEVT_DATAVIEW_CONTEXT_MENU, my_view);
    m_storage_model = new StorageModel(my_view);
    my_view->AssociateModel(m_storage_model.get());
    
    m_storage_model->BuildColumns(my_view);
	wxString s;
	s=settings->Read(dlgName + "/ColsWidth","");
	if (s.Len()>0) my_view->setSettingString(s);
	//settings->Write(dlgName + "/ColsWidth", s);

    wxSizer* zeroPanelSz = new wxBoxSizer(wxVERTICAL);
    my_view->SetMinSize(wxSize(-1, 200));
    zeroPanelSz->Add(my_view, 1, wxGROW | wxALL, 5);
    
    status = new wxStaticText(testPanel, wxID_ANY, "status text");
    zeroPanelSz->Add(
            status,
            0, wxGROW | wxALL, 5);
    //zeroPanelSz->Add(button_sizer);
    //zeroPanelSz->Add(sizerCurrent);
    my_view->setStatusObj(status);
    wxBoxSizer* sSizer = new wxBoxSizer(wxHORIZONTAL);
    group = new wxCheckBox(testPanel, ID_SET_GROUP, "Mode group");
    sSizer->Add(group,
        wxSizerFlags().Centre().DoubleBorder());
    detail = new wxCheckBox(testPanel, ID_SET_DETAILGROUP, "View detail group");
    sSizer->Add(detail,
        wxSizerFlags().Centre().DoubleBorder());
    const wxSizerFlags border1 = wxSizerFlags().DoubleBorder();

    //wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    sSizer->Add(new wxButton(testPanel, ID_CLEAR_ALL_FILTER, "Clear All Filter"), border1);
    //sSizer->Add(new wxButton(testPanel, ID_DELETE_SEL, "Delete selected"), border);
	sSizer->Add(new wxButton(testPanel, ID_ADD_FILTER, "Add Filter Ignore"), border1);
	sSizer->Add(new wxButton(testPanel, ID_HELP_LOG, "Help"), border1);
	smart = new wxComboBox(testPanel, ID_CBOX_SMART, wxT(""), wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
	//smart->SetWindowStyle(smart->GetWindowStyle()|wxCB_READONLY);
	
	smart->SetMinSize(wxSize(300,-1));
	//listUserFilter = new wxComboBox(testPanel, ID_CBOX_UFilter, wxT("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
	sSizer->Add(smart, border1);
	my_view->smart = smart;

    zeroPanelSz->Add(sSizer);
    testPanel->SetSizerAndFit(zeroPanelSz);

    m_notebook->AddPage(testPanel, "Log");
	wxBoxSizer* setingSZ = new wxBoxSizer(wxHORIZONTAL);
	setingSZ->Add(m_notebook);

	wxPanel* settingPanel = new wxPanel(m_notebook, wxID_ANY);
	lb = new wxCheckListBox(settingPanel,wxID_ANY);
	wxTreeItemIdValue foldercookie, servercookie;
	wxTreeItemId folderitem, serveritem;
	pgObject* object;
	pgServer* server;
	std::vector<wxString> vec;
	folderitem = mainForm->GetBrowser()->GetFirstChild(mainForm->GetBrowser()->GetRootItem(), foldercookie);
	while (folderitem)
	{
		if (mainForm->GetBrowser()->ItemHasChildren(folderitem))
		{
			serveritem = mainForm->GetBrowser()->GetFirstChild(folderitem, servercookie);
			while (serveritem)
			{
				object = mainForm->GetBrowser()->GetObject(serveritem);
				if (object && object->IsCreatedBy(serverFactory))
				{
					server = (pgServer*)object;
					wxString srvname = wxString::Format("%s:%d", server->GetName(), server->GetPort());
					vec.push_back(srvname);
					
				}
				serveritem = mainForm->GetBrowser()->GetNextChild(folderitem, servercookie);
			}
		}
		folderitem = mainForm->GetBrowser()->GetNextChild(mainForm->GetBrowser()->GetRootItem(), foldercookie);
	}
	lb->Append(vec);
	wxString srvs;
	wxString srvname = wxString::Format("%s:%d", srv->GetName(), srv->GetPort());
	settings->Read(dlgName + "/AutoConnect", &srvs, "");
	if (!srvs.IsEmpty()) srvs += ";";
	srvs += srvname;
	wxStringTokenizer tk(srvs, ";", wxTOKEN_RET_EMPTY_ALL);
	while (tk.HasMoreTokens())
	{
		wxString l = tk.GetNextToken();
		pgServer* s = getServer(l);
		if (s != NULL) {
			if (!CheckConn(s->GetName(), s->GetPort())) {
				pgConn* conn = createConn(s);
				AddNewConn(conn);
				for (unsigned int x = 0; x < lb->GetCount(); x++)
					if (l==lb->GetString(x)) lb->Check(x, true);

			}
		}
		
	}

	wxSizer* zeroPanelSz2 = new wxBoxSizer(wxHORIZONTAL);
	lb->SetMinSize(wxSize(-1, 200));
	zeroPanelSz2->Add(lb, 1, wxGROW | wxALL, 5);

	wxPanel *m_panel2 = new wxPanel(settingPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	zeroPanelSz2->Add(m_panel2, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer(wxHORIZONTAL);
	listUserFilter = new wxComboBox(m_panel2, ID_CBOX_UFilter, wxT("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0);
	bSizer2->Add(listUserFilter, 1, wxALL | wxEXPAND, 2);
	wxButton *m_button1 = new wxButton(m_panel2, ID_ADD_UFilter, wxT("Add"), wxDefaultPosition, wxDefaultSize, 0);
	m_button1->SetMaxSize(wxSize(30, -1));
	bSizer2->Add(m_button1, 0, wxALL, 2);
	wxButton *m_button2 = new wxButton(m_panel2, ID_DEL_UFilter, wxT("Del"), wxDefaultPosition, wxDefaultSize, 0);
	m_button2->SetMaxSize(wxSize(30, -1));
	bSizer2->Add(m_button2, 0, wxALL, 2);
	bSizer3->Add(bSizer2, 0, wxEXPAND, 5);

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer(wxVERTICAL);

	contentFilter = new wxTextCtrl(m_panel2, ID_TEXT_UFilter, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	bSizer5->Add(contentFilter, 1, wxALL | wxEXPAND, 2);
	bSizer3->Add(bSizer5, 1, wxEXPAND, 2);
	m_panel2->SetSizer(bSizer3);

	//listUserFilter->Bind();
	
	settingPanel->SetSizerAndFit(zeroPanelSz2);
	m_notebook->AddPage(settingPanel, "Settings");

	bool b=true;
	settings->Read(dlgName + "/Mode",&b, false);
	group->SetValue(b);
	my_view->setGroupMode(b);
	my_view->ModUserFilter("","Init", listUserFilter, contentFilter);
//	if (mainForm) getFilename();
	m_timer.Start(timerInterval);
}
pgServer* frmLog::getServer(wxString& strserver) {
	wxTreeItemIdValue foldercookie, servercookie;
	wxTreeItemId folderitem, serveritem;
	pgObject* object;
	pgServer* server;
	folderitem = mainForm->GetBrowser()->GetFirstChild(mainForm->GetBrowser()->GetRootItem(), foldercookie);
	while (folderitem)
	{
		if (mainForm->GetBrowser()->ItemHasChildren(folderitem))
		{
			serveritem = mainForm->GetBrowser()->GetFirstChild(folderitem, servercookie);
			while (serveritem)
			{
				object = mainForm->GetBrowser()->GetObject(serveritem);
				if (object && object->IsCreatedBy(serverFactory))
				{
					server = (pgServer*)object;
					wxString srvname = wxString::Format("%s:%d", server->GetName(), server->GetPort());
					if (srvname == strserver) {
						return server;
					}
				}
				serveritem = mainForm->GetBrowser()->GetNextChild(folderitem, servercookie);
			}
		}
		folderitem = mainForm->GetBrowser()->GetNextChild(mainForm->GetBrowser()->GetRootItem(), foldercookie);
	}
	return 0;
}

frmLog::~frmLog()
{
	// If the status window wasn't launched in standalone mode...
	if (mainForm)
		mainForm->RemoveFrame(this);


    // If connection is still available, delete it
	SavePosition();
	wxString srvs;
	for (unsigned int x = 0; x < lb->GetCount(); x++)
		if (lb->IsChecked(x)) {
			if (!srvs.IsEmpty()) srvs += ";";
			srvs += lb->GetString(x);
		}
	settings->Write(dlgName + "/AutoConnect", srvs);
	wxString s = my_view->getSettingString();
	settings->Write(dlgName+"/ColsWidth",s);
	settings->WriteBool(dlgName + "/Mode", group->IsChecked());
	Storage *st=m_storage_model->getStorage();
	st->saveFilters();
	mainForm->Logfrm = NULL;
}
void frmLog::Go()
{
    // Show the window
    Show(true);
}
LogFactory::LogFactory(menuFactoryList* list, wxMenu* mnu, ctlMenuToolbar* toolbar) : contextActionFactory(list)
{
	mnu->Append(id, _("Log view..."), _("Log view CSV format"));
}


wxWindow* LogFactory::StartDialog(frmMain* form, pgObject* obj)
{
	pgServer* srv = (pgServer *) obj;
	wxString txt = "";
	if (form->Logfrm != NULL) {
		if (!form->Logfrm->CheckConn(srv->GetName(), srv->GetPort())) {
			pgConn* conn = form->Logfrm->createConn(srv);
			form->Logfrm->AddNewConn(conn);
		}
	}
	else {
		form->Logfrm = new frmLog(form, txt, srv);
		if (form->Logfrm!=NULL)
			form->AddFrame(form->Logfrm);

	}
	//frmLog* frm = new frmLog(form, obj);

	form->Logfrm->Go();
	return 0;
}


bool LogFactory::CheckEnable(pgObject* obj)
{
	if (!obj)
		return false;

	if (obj->GetMetaType() == PGM_SERVER) {
//		if (!((pgServer*)obj)->GetConnected())
//			return false;
		return true;
	}
	return false;
}




