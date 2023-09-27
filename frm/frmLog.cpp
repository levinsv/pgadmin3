
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
	DBthread->DoTerminate();
	while (!DBthread->isReadyRows()) {
		wxThread::Sleep(50);
		wxYield();
	}
	DBthread->GoReadRows();
	
	wxThread::Sleep(50);
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
void* MyThread::Entry() {

	while (!m_exit)
	{
		{
			wxMutexLocker lock(s_mutexDBReading);
			m_addNewRows.Clear();
			m_serversName.Clear();
			m_startRowsSevers.Clear();
			getFilename();
		}
		
		s_goRead.Wait();
		if (m_exit) break;
	}

	return NULL;
}
wxString MyThread::AppendNewRows(MyDataViewCtrl* my_view, Storage* st) {
	//Storage* st = m_storage_model->getStorage();
	//int rows = st->getCountStore();
	int ra, ri;
	st->ClearRowsStat();
	my_view->Freeze();
	for (size_t i = 0; i < m_startRowsSevers.GetCount(); i++) {
		if (m_exit) return "exit";
		int sr = m_startRowsSevers[i];
		int er = m_addNewRows.GetCount();
		if ((i+1)< m_startRowsSevers.GetCount()) er= m_startRowsSevers[i+1];
		if (sr == er) continue;
		st->SetHost(m_serversName[i]);
		for (int k = sr; k < er; k++) {
			if (m_exit) return "exit";
			wxString ss = m_addNewRows[k];
			if (ss.IsEmpty())
				continue;
			my_view->AddRow(ss);
		}
	}
	st->GetRowsStat(ra, ri);
	my_view->Thaw();
	//int newrows = st->getCountStore();
	//if (loglen !=logfileLength) 
	if (st->GetErrMsgFlag()) {
		m_seticon=true;
	}
	return wxString::Format("Add rows %d ignore %d. View rows ", ra, ri);

}
void MyThread::getFilename() {
	pgSet* set;
	namepage = "";
	m_seticon = false;
	for (size_t i = 0; i < m_conArray.GetCount(); i++) {
		if (m_exit) break;
		RemoteConn2* po = (RemoteConn2*) m_conArray[i];
		wxString dbname = po->conn->GetDbname();
		if (!namepage.IsEmpty()) namepage += ",";
		m_serversName.Add(po->conn->GetHostName());
		m_startRowsSevers.Add(m_addNewRows.GetCount());
		if (!po->conn->IsAlive()) {
			wxDateTime n = wxDateTime::Now();
			m_seticon = true;
			if (po->nextrun < n) {
				if (!po->conn->Reconnect(false))
				{
					wxTimeSpan sp(0, 2);
					po->nextrun = wxDateTime::Now() + sp;
					namepage += " " + dbname;
					continue;
				}
			}
			else {
				namepage += " " + dbname;
				continue;
			}


		}
		set = po->conn->ExecuteSet(
			wxT("select current_setting('log_directory')||'/'||name filename,modification filetime,size len\n")
			wxT("  FROM pg_ls_logdir()  where name ~ '.csv' ORDER BY modification DESC"));
		if (set)
		{

			//logfileTimestamp = set->GetDateTime(wxT("filetime"));
			len[i] = set->GetLong(wxT("len"));
			namepage += dbname;
			//m_storage_model->getStorage()->SetHost(m_conArray[i].conn->GetHostName());
			//m_startRowsSevers[i] = m_addNewRows.GetCount();
			wxString fn = set->GetVal(wxT("filename"));
			if (fn != logfileName[i]) {
				logfileLength[i] = 0;
				logfileName[i] = fn;

			}
			/// addLogFile(logfileName, logfileTimestamp, len, logfileLength, skipFirst);

			delete set;
			readLogFile(logfileName[i], len[i], logfileLength[i], savedPartialLine[i], po->conn);
			//m_startRowsSevers[i] = m_addNewRows.GetCount();
			//if (m_startRowsSevers[i] == m_addNewRows.GetCount()) m_startRowsSevers[i] = FLAG_MAX_LINE;
		}
	}
	//if (namepage.IsEmpty()) namepage = "not connect";
	//if (m_notebook->GetPageText(0) != namepage) m_notebook->SetPageText(0, namepage);
}
void MyThread::readLogFile(wxString logfileName, long& lenfile, long& logfileLength, wxString& savedPartialLine, pgConn* conn) {
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
		if (m_exit) return;
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
		if (true) {

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
		int l = strlen(raw);
		logfileLength += l;
		wxString host = conn->GetHostName();
		//status->SetLabelText(wxString::Format("%s Load bytes %ld", host, logfileLength));
		sendText(wxString::Format("%s Load bytes %ld", host, logfileLength));
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
			wxString msgstr = host+" The server log contains entries in multiple encodings and cannot be displayed by pgAdmin.";
			//wxMessageBox(msgstr);
			sendText(msgstr);
			return;
		}

		if (csv_log_format)
		{
			// This will work for any DB using CSV format logs


			CSVLineTokenizer tk(str);


			//my_view->Freeze();
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


				//my_view->AddRow(str.Trim());
				m_addNewRows.Add(str.Trim());
			}

			//my_view->Thaw();
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
			//my_view->AddRow(line.Trim());
			m_addNewRows.Add(line.Trim());
	}


}

void frmLog::OnAddLabelTextThread(wxThreadEvent& event) {
	wxString s=event.GetString();
	if (!msgtext.IsEmpty() && !s.IsEmpty()) s += " : ";
	s += msgtext;
	status->SetLabelText(s);
}

void frmLog::OnTimer(wxTimerEvent& event) {

	if (!DBthread->isReadyRows()) return;

	wxString rez=DBthread->AppendNewRows(my_view, m_storage_model->getStorage());
	msgtext = wxString::Format("%s%d", rez, m_storage_model->GetRowCount());
	status->SetLabelText(msgtext);
	if (DBthread->IsIconError()) {
		seticon(true);
	}
	wxString namepage= DBthread->getNamePage();

	if (namepage.IsEmpty()) namepage = "not connect";
	if (m_notebook->GetPageText(0) != namepage) m_notebook->SetPageText(0, namepage);
	DBthread->GoReadRows();

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
wxSize MywxAuiDefaultTabArt::GetTabSize(wxDC& dc,
	wxWindow* wnd,
	const wxString& caption,
	const wxBitmapBundle& bitmap,
	bool WXUNUSED(active),
	int close_button_state,
	int* x_extent)
{
	wxCoord measured_textx, measured_texty, tmp;

	dc.SetFont(m_normalFont);
	dc.GetTextExtent(caption, &measured_textx, &measured_texty);

	dc.GetTextExtent(wxT("ABCDEFXj"), &tmp, &measured_texty);

	// add padding around the text
	wxCoord tab_width = measured_textx;
	wxCoord tab_height = measured_texty;

	// if the close button is showing, add space for it
	if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
	{
		// increase by button size plus the padding
		tab_width += m_activeCloseBmp.GetBitmapFor(wnd).GetLogicalWidth() + wnd->FromDIP(3);
	}

	// if there's a bitmap, add space for it
	if (bitmap.IsOk())
	{
		// we need the correct size of the bitmap to be used on this window in
		// logical dimensions for drawing
		const wxSize bitmapSize = bitmap.GetPreferredLogicalSizeFor(wnd);

		// increase by bitmap plus right side bitmap padding
		tab_width += bitmapSize.x + wnd->FromDIP(3);
		tab_height = wxMax(tab_height, bitmapSize.y);
	}

	// add padding
	wxSize padding = wnd->FromDIP(wxSize(16, 10));
	tab_width += padding.x;
	tab_height += padding.y;

	if (m_flags & wxAUI_NB_TAB_FIXED_WIDTH)
	{
		tab_width = m_fixedTabWidth;
	}

	*x_extent = tab_width;

	return wxSize(tab_width, tab_height);
}

void MywxAuiDefaultTabArt::DrawTab(wxDC& dc,
	wxWindow* wnd,
	const wxAuiNotebookPage& page,
	const wxRect& in_rect,
	int close_button_state,
	wxRect* out_tab_rect,
	wxRect* out_button_rect,
	int* x_extent)
{
	wxCoord normal_textx, normal_texty;
	wxCoord selected_textx, selected_texty;
	wxCoord texty;

	// if the caption is empty, measure some temporary text
	wxString caption = page.caption;
	if (caption.empty())
		caption = wxT("Xj");

	dc.SetFont(m_normalFont);
	dc.GetTextExtent(caption, &selected_textx, &selected_texty);

	dc.SetFont(m_normalFont);
	dc.GetTextExtent(caption, &normal_textx, &normal_texty);

	// figure out the size of the tab
	wxSize tab_size = GetTabSize(dc,
		wnd,
		page.caption,
		page.bitmap,
		page.active,
		close_button_state,
		x_extent);

	wxCoord tab_height = m_tabCtrlHeight - 3;
	wxCoord tab_width = tab_size.x;
	wxCoord tab_x = in_rect.x;
	wxCoord tab_y = in_rect.y + in_rect.height - tab_height;


	caption = page.caption;


	// select pen, brush and font for the tab to be drawn

	if (page.active)
	{
		dc.SetFont(m_normalFont);
		texty = selected_texty;
	}
	else
	{
		dc.SetFont(m_normalFont);
		texty = normal_texty;
	}


	// create points that will make the tab outline

	int clip_width = tab_width;
	if (tab_x + clip_width > in_rect.x + in_rect.width)
		clip_width = (in_rect.x + in_rect.width) - tab_x;

	// since the above code above doesn't play well with WXDFB or WXCOCOA,
	// we'll just use a rectangle for the clipping region for now --
	dc.SetClippingRegion(tab_x, tab_y, clip_width + 1, tab_height - 3);


	wxPoint border_points[6];
	
		border_points[0] = wxPoint(tab_x, tab_y + tab_height - 4);
		border_points[1] = wxPoint(tab_x, tab_y + 2);
		border_points[2] = wxPoint(tab_x + 2, tab_y);
		border_points[3] = wxPoint(tab_x + tab_width - 2, tab_y);
		border_points[4] = wxPoint(tab_x + tab_width, tab_y + 2);
		border_points[5] = wxPoint(tab_x + tab_width, tab_y + tab_height - 4);
	
	// TODO: else if (m_flags &wxAUI_NB_LEFT) {}
	// TODO: else if (m_flags &wxAUI_NB_RIGHT) {}

	int drawn_tab_yoff = border_points[1].y;
	int drawn_tab_height = border_points[0].y - border_points[1].y;

	bool isdark = wxSystemSettings::GetAppearance().IsUsingDarkBackground();

	wxColor back_color = m_baseColour;
	if (page.active)
	{
		// draw active tab

		// draw base background color
		wxRect r(tab_x, tab_y, tab_width, tab_height);
		dc.SetPen(wxPen(m_activeColour));
		dc.SetBrush(wxBrush(m_activeColour));
		dc.DrawRectangle(r.x + 1, r.y + 1, r.width - 1, r.height - 4);

		// this white helps fill out the gradient at the top of the tab
		wxColor gradient = *wxWHITE;
		if (isdark)
		{
			//dark mode, we go darker
			gradient = m_activeColour.ChangeLightness(70);
		}
		back_color = gradient;

		dc.SetPen(wxPen(gradient));
		dc.SetBrush(wxBrush(gradient));
		dc.DrawRectangle(r.x + 2, r.y + 1, r.width - 3, r.height - 4);

		// these two points help the rounded corners appear more antialiased
		dc.SetPen(wxPen(m_activeColour));
		dc.DrawPoint(r.x + 2, r.y + 1);
		dc.DrawPoint(r.x + r.width - 2, r.y + 1);

		// set rectangle down a bit for gradient drawing
		r.SetHeight(r.GetHeight() / 2);
		r.x += 2;
		r.width -= 3;
		r.y += r.height;
		r.y -= 2;

		// draw gradient background
		wxColor top_color = gradient;
		wxColor bottom_color = m_activeColour;
		dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);
	}
	else
	{
		// draw inactive tab

		wxRect r(tab_x, tab_y + 1, tab_width, tab_height - 3);

		// start the gradient up a bit and leave the inside border inset
		// by a pixel for a 3D look.  Only the top half of the inactive
		// tab will have a slight gradient
		r.x += 3;
		r.y++;
		r.width -= 4;
		r.height /= 2;
		r.height--;

		// -- draw top gradient fill for glossy look
		wxColor top_color = m_baseColour;
		wxColor bottom_color = top_color.ChangeLightness(160);
		if (isdark)
		{
			//dark mode, we go darker
			top_color = m_activeColour.ChangeLightness(70);
			bottom_color = m_baseColour;
		}

		dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);

		r.y += r.height;
		r.y--;

		// -- draw bottom fill for glossy look
		top_color = m_baseColour;
		bottom_color = m_baseColour;
		dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);
	}

	// draw tab outline
	dc.SetPen(m_borderPen);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawPolygon(WXSIZEOF(border_points), border_points);

	// there are two horizontal grey lines at the bottom of the tab control,
	// this gets rid of the top one of those lines in the tab control
	if (page.active)
	{
		if (m_flags & wxAUI_NB_BOTTOM)
			dc.SetPen(wxPen(m_baseColour.ChangeLightness(170)));
		// TODO: else if (m_flags &wxAUI_NB_LEFT) {}
		// TODO: else if (m_flags &wxAUI_NB_RIGHT) {}
		else //for wxAUI_NB_TOP
			dc.SetPen(m_baseColourPen);
		dc.DrawLine(border_points[0].x + 1,
			border_points[0].y,
			border_points[5].x,
			border_points[5].y);
	}


	int text_offset;
	int bitmap_offset = 0;
	if (page.bitmap.IsOk())
	{
		bitmap_offset = tab_x + wnd->FromDIP(8);

		const wxBitmap bitmap = page.bitmap.GetBitmapFor(wnd);

		// draw bitmap
		dc.DrawBitmap(bitmap,
			bitmap_offset,
			drawn_tab_yoff + (drawn_tab_height / 2) - (bitmap.GetLogicalHeight() / 2),
			true);

		text_offset = bitmap_offset + bitmap.GetLogicalWidth();
		text_offset += wnd->FromDIP(3); // bitmap padding
	}
	else
	{
		text_offset = tab_x + wnd->FromDIP(8);
	}

	// draw close button if necessary
	int close_button_width = 0;

	wxString draw_text = caption;

	// 
	size_t pos = 0; size_t poss = 0;
	int x = text_offset;
	int ttx, tty;
	int y = drawn_tab_yoff + (drawn_tab_height) / 2 - (texty / 2) - 1;
	{
		wxDCBrushChanger setBrush(dc, *wxYELLOW_BRUSH);
		while ((pos = draw_text.find(' ', poss)) != wxString::npos) {
			dc.GetTextExtent(draw_text.SubString(poss, pos), &ttx, &tty);
			x += ttx;
			size_t en=draw_text.find(',', pos + 1);
			if (en == wxString::npos) en = draw_text.Len();
			dc.GetTextExtent(draw_text.SubString(pos+1, en-1), &ttx, &tty);
			wxRect r(x, y, ttx, tty);
			dc.DrawRoundedRectangle(r, 2);
			x += ttx;
			poss = en;

		}
		//
	}
	// draw tab text
	wxColor sys_color = wxSystemSettings::GetColour(
		page.active ? wxSYS_COLOUR_CAPTIONTEXT : wxSYS_COLOUR_INACTIVECAPTIONTEXT);
	dc.SetTextForeground(sys_color);
	dc.DrawText(draw_text,
		text_offset,
		drawn_tab_yoff + (drawn_tab_height) / 2 - (texty / 2) - 1);

	// draw focus rectangle except under macOS where it looks out of place
#ifndef __WXOSX__
	if (page.active && (wnd->FindFocus() == wnd))
	{
		wxRect focusRectText(text_offset, (drawn_tab_yoff + (drawn_tab_height) / 2 - (texty / 2) - 1),
			selected_textx, selected_texty);

		wxRect focusRect;
		wxRect focusRectBitmap;

		if (page.bitmap.IsOk())
		{
			const wxBitmap bitmap = page.bitmap.GetBitmapFor(wnd);

			focusRectBitmap = wxRect(bitmap_offset, drawn_tab_yoff + (drawn_tab_height / 2) - (bitmap.GetLogicalHeight() / 2),
				bitmap.GetLogicalWidth(), bitmap.GetLogicalHeight());
		}

		if (page.bitmap.IsOk() && draw_text.IsEmpty())
			focusRect = focusRectBitmap;
		else if (!page.bitmap.IsOk() && !draw_text.IsEmpty())
			focusRect = focusRectText;
		else if (page.bitmap.IsOk() && !draw_text.IsEmpty())
			focusRect = focusRectText.Union(focusRectBitmap);

		focusRect.Inflate(2, 2);

		wxRendererNative::Get().DrawFocusRect(wnd, dc, focusRect, 0);
	}
#endif // !__WXOSX__

	* out_tab_rect = wxRect(tab_x, tab_y, tab_width, tab_height);

	dc.DestroyClippingRegion();

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

    m_notebook = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,0 );
	MywxAuiDefaultTabArt* art = new MywxAuiDefaultTabArt();
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
	m_notebook->SetArtProvider(art);
	//m_notebook->SetSelectedFont(settings->GetSystemFont());
	//m_notebook->SetNormalFont(settings->GetSystemFont());
	m_notebook->SetFont(settings->GetSystemFont());
	bool b=true;
	settings->Read(dlgName + "/Mode",&b, false);
	group->SetValue(b);
	my_view->setGroupMode(b);
	my_view->ModUserFilter("","Init", listUserFilter, contentFilter);
//	if (mainForm) getFilename();
	Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(frmLog::OnAddLabelTextThread), NULL, this);
	DBthread = new MyThread(conArray,this);
	if (DBthread->Create() != wxTHREAD_NO_ERROR)
	{
		wxLogError(wxT("Can’t create thread!"));
	}
	DBthread->Run();
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




