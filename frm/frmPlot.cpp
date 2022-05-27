
#include "pgAdmin3.h"
#include "frm/frmPlot.h"

enum {
	ID_QUIT = 108,
	ID_ABOUT,
	ID_PRINT,
	ID_PRINT_PREVIEW,
	ID_ALIGN_X_AXIS,
	ID_ALIGN_Y_AXIS,
	ID_TOGGLE_GRID,
	ID_COPY_SCREEN,
	ID_TOGGLE_INFO,
	ID_SAVE_SCREENSHOT,
	ID_TOGGLE_SHOW_SERIES,
	ID_TOGGLE_SHOW_SIMBOL,
	ID_TOGGLE_COSINE,
	ID_BLACK_THEME
};
//IMPLEMENT_DYNAMIC_CLASS(frmPlot, pgFrame)
static const struct wxColourDesc
{
    const wxChar* name;
    //        unsigned char r, g, b;
}
colors[] =
{
     wxT("#ED7D31")
    ,wxT("#997300")
    ,wxT("#A580D5")
    ,wxT("#70AD47")
    ,wxT("#FFC000")
    ,wxT("#4472C4")
    ,wxT("#70AD47")
    ,wxT("#CA682A")
    ,wxT("#FFE533")
    ,wxT("#FF5933")
    ,wxT("#FF59B9")

    ,wxT("#5A5FFF")
    ,wxT("#5A9F8A")
    ,wxT("#5A9F50")
    ,wxT("#1CD749")
    ,wxT("#6ABB49")
    ,wxT("#BEBB49")


    ,wxT("#4797D8")
    ,wxT("#4713D8")
    ,wxT("#7413D8")
    ,wxT("#B389D8")
    ,wxT("#9CDA89")
    ,wxT("#E7D900")

    ,wxT("#FF9A00")
    ,wxT("#00FF00")
    ,wxT("#8A7DFF")
    ,wxT("#250DFF")
    ,wxT("#DA0D8F")
    ,wxT("#17C3FF")


};

wxBEGIN_EVENT_TABLE(frmPlot, pgFrame)
// test
EVT_MENU(ID_ABOUT, frmPlot::OnAbout)
EVT_MENU(ID_QUIT, frmPlot::OnQuit)
EVT_MENU(ID_PRINT_PREVIEW, frmPlot::OnPrintPreview)
EVT_MENU(ID_PRINT, frmPlot::OnPrint)
EVT_MENU(mpID_FIT, frmPlot::OnFit)
EVT_MENU(ID_ALIGN_X_AXIS, frmPlot::OnAlignXAxis)
EVT_MENU(ID_ALIGN_Y_AXIS, frmPlot::OnAlignYAxis)
EVT_MENU(ID_TOGGLE_GRID, frmPlot::OnToggleGrid)
EVT_MENU(ID_COPY_SCREEN, frmPlot::OnCopy)
EVT_MENU(ID_TOGGLE_INFO, frmPlot::OnToggleInfoLayer)
EVT_MENU(ID_SAVE_SCREENSHOT, frmPlot::OnSaveScreenshot)
EVT_MENU(ID_BLACK_THEME, frmPlot::OnBlackTheme)
EVT_MENU(ID_TOGGLE_SHOW_SERIES, frmPlot::OnToggleShowSeries)
EVT_MENU(ID_TOGGLE_SHOW_SIMBOL, frmPlot::OnToggleShowSimbol)
EVT_MENU(ID_TOGGLE_COSINE, frmPlot::OnToggleCosine)
EVT_MENU(mpID_CENTER, frmPlot::OnCenter)
EVT_MENU(mpID_ZOOM_IN, frmPlot::OnZoomIn)
EVT_MENU(mpID_ZOOM_INX, frmPlot::OnZoomInX)
EVT_MENU(mpID_ZOOM_INY, frmPlot::OnZoomInY)
EVT_MENU(mpID_ZOOM_OUTX, frmPlot::OnZoomOutX)
EVT_MENU(mpID_ZOOM_OUTY, frmPlot::OnZoomOutY)
EVT_MENU(mpID_ZOOM_OUT, frmPlot::OnZoomOut)
EVT_MENU(mpID_LOCKASPECT, frmPlot::OnLockAspect)
EVT_KEY_DOWN(frmPlot::OnKEY_DOWN)
EVT_CLOSE(frmPlot::OnClose)
wxEND_EVENT_TABLE()



frmPlot::frmPlot(frmQuery* parent, const wxString& _title) : pgFrame(parent, _title)
{
    
	dlgName = wxT("frmPlot");
	RestorePosition(-1, -1, 700, 500, 700, 500);
    indexColor = 0;
	//SetIcon(wxIcon(log_xpm));
	wxMenu* file_menu = new wxMenu();
	wxMenu* view_menu = new wxMenu();
	wxMenu* show_menu = new wxMenu();

	file_menu->Append(ID_PRINT_PREVIEW, wxT("Print Pre&view..."));
	file_menu->Append(ID_PRINT, wxT("&Print..."));
	file_menu->Append(ID_SAVE_SCREENSHOT, wxT("Save screenshot"));
    file_menu->Append(ID_COPY_SCREEN, wxT("Copy clipboard screenshot\tCtrl+C"));
    
	file_menu->AppendSeparator();
	file_menu->Append(ID_ABOUT, wxT("&About..."));
	file_menu->Append(ID_QUIT, wxT("E&xit\tAlt-X"));

	view_menu->Append(mpID_FIT, wxT("&Fit bounding box"), wxT("Set plot view to show all items"));
	view_menu->Append(mpID_ZOOM_IN, wxT("Zoom in"), wxT("Zoom in plot view."));
	view_menu->Append(mpID_ZOOM_OUT, wxT("Zoom out"), wxT("Zoom out plot view."));
    view_menu->AppendCheckItem(mpID_LOCKASPECT, _("Lock aspect"),_("Lock horizontal and vertical zoom aspect."));
    view_menu->AppendSeparator();

    view_menu->Append(mpID_ZOOM_INX, wxT("Zoom in X axis\tKP_Add"), wxT("Zoom in plot view X axis."));
    view_menu->Append(mpID_ZOOM_OUTX, wxT("Zoom out X axis\tKP_Subtract"), wxT("Zoom out plot view X axis."));
    view_menu->Append(mpID_ZOOM_INY, wxT("Zoom in Y axis\tCtrl+KP_Add"), wxT("Zoom in plot view Y axis."));
    view_menu->Append(mpID_ZOOM_OUTY, wxT("Zoom out Y axis\tCtrl+KP_Subtract"), wxT("Zoom out plot view Y axis."));

	view_menu->AppendSeparator();
	view_menu->Append(ID_ALIGN_X_AXIS, wxT("Switch &X axis align"));
	view_menu->Append(ID_ALIGN_Y_AXIS, wxT("Switch &Y axis align"));
	view_menu->AppendCheckItem(ID_TOGGLE_GRID, wxT("Toggle grid/ticks"));
	//view_menu->AppendCheckItem( ID_COPY_SCREEN, wxT("Show Scroll Bars"));
	view_menu->AppendCheckItem(ID_TOGGLE_INFO, wxT("Show overlay info box"));
    view_menu->Check(ID_TOGGLE_INFO, true);
	view_menu->AppendCheckItem(ID_BLACK_THEME, wxT("Switch to black background theme"));

	show_menu->AppendCheckItem(ID_TOGGLE_SHOW_SERIES, wxT("Show name series"));
	show_menu->AppendCheckItem(ID_TOGGLE_SHOW_SIMBOL, wxT("Show Simbol"));
	//show_menu->AppendCheckItem(ID_TOGGLE_COSINE, wxT("Cosine"));
	// Start with all plots visible
	show_menu->Check(ID_TOGGLE_SHOW_SERIES, false);
	show_menu->Check(ID_TOGGLE_SHOW_SIMBOL, true);
	//show_menu->Check(ID_TOGGLE_COSINE, true);

	wxMenuBar* menu_bar = new wxMenuBar();
	menu_bar->Append(file_menu, wxT("&File"));
	menu_bar->Append(view_menu, wxT("&View"));
	menu_bar->Append(show_menu, wxT("&Show"));

	SetMenuBar(menu_bar);
//	CreateStatusBar(1);
	mpLayer* l;

	m_plot = new mpWindow(this, -1, wxPoint(0, 0), wxSize(100, 100), wxSUNKEN_BORDER);
    
	m_plot->SetMargins(30, 100, 50, 100);
	wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    m_plot->Bind(wxEVT_KEY_DOWN, &frmPlot::OnKEY_DOWN, this);

	topsizer->Add(m_plot, 1, wxEXPAND);
//	topsizer->Add(m_log, 0, wxEXPAND);

	SetAutoLayout(TRUE);
	SetSizer(topsizer);
	axesPos[0] = 0;
	axesPos[1] = 0;
	ticks = true;
	m_plot->Fit();
}
frmPlot::~frmPlot()
{
	// If the status window wasn't launched in standalone mode...
	//if (mainForm) 		mainForm->RemoveFrame(this);


	// If connection is still available, delete it
	SavePosition();
}
void frmPlot::Go()
{
	// Show the window
    m_plot->Fit();
	Show(true);
}

void frmPlot::OnClose(wxCloseEvent& event) {
    event.Skip();
	
}
void frmPlot::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(TRUE);
}
void frmPlot::ClearAndSetAxis(wxString XtextAxis, unsigned int X_labelType, wxString YtextAxis)
{
    m_plot->DelAllLayers(true, false);

    mpScaleX* xaxis = new mpScaleX(XtextAxis, mpALIGN_BOTTOM, true, X_labelType);
    mpScaleY* yaxis = new mpScaleY(YtextAxis, mpALIGN_LEFT, true);
    wxFont graphFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    xaxis->SetFont(graphFont);
    yaxis->SetFont(graphFont);
    xaxis->SetDrawOutsideMargins(false);
    yaxis->SetDrawOutsideMargins(false);
    m_plot->AddLayer(xaxis); // 0
    m_plot->AddLayer(yaxis); // 1
    m_plot->AddLayer(nfo = new mpInfoCoords(wxRect(80, 80, 10, 10), wxTRANSPARENT_BRUSH)); //&hatch));
    nfo->SetVisible(true); // 2
    //wxBrush hatch2(wxColour(163,208,212), wxSOLID);
    mpInfoLegend* leg;
    m_plot->AddLayer(leg = new mpInfoLegend(wxRect(20, 20, 40, 40), wxTRANSPARENT_BRUSH)); //&hatch2));
    leg->SetVisible(true); // 3
    leg->SetDrawOutsideMargins(false);
    indexColor = 0;
    //m_plot->Fit();
}

void frmPlot::AddSeries(const wxString legend, const std::vector<double>& x, const std::vector<double>& y, const std::vector<wxString>& l)
{
    mpFXYVector* vectorLayer = new mpFXYVector(legend);
    vectorLayer->SetData(x, y);
    if (l.size()>0) vectorLayer->SetLegendBar(l);
    vectorLayer->SetContinuity(true);
    size_t n;
    wxColourDesc cc;
    for (n = indexColor; n < WXSIZEOF(colors); n++)
    {
        cc = colors[n];
        indexColor++;
        break;
         //= new wxColour(cc.name);
    }
    if (indexColor >= WXSIZEOF(colors)) cc.name = L"#000000";
    vectorLayer->SetDrawOutsideMargins(false);
    vectorLayer->ShowSimbol(true);
    if (vectorLayer->IsLegendBar()) {
        wxPen vectorpen(wxColour(cc.name), 1, wxPENSTYLE_SOLID);
        wxBrush br(wxColour(cc.name));
        vectorLayer->SetPen(vectorpen);
        vectorLayer->ShowName(false);
        vectorLayer->SetBrush(br);
        vectorLayer->SetContinuity(false);
    }
    else
    {
        wxPen vectorpen(wxColour(cc.name), 2, wxPENSTYLE_SOLID);
        vectorLayer->SetPen(vectorpen);
        vectorLayer->ShowName(false);
    }
    m_plot->AddLayer(vectorLayer);
    //m_plot->Fit();
}
void frmPlot::OnKEY_DOWN(wxKeyEvent& event) {
    //void OnScrollThumbTrack(wxScrollWinEvent & event);
    bool modctrl;
    if ((event.GetModifiers() & wxMOD_CONTROL) == wxMOD_CONTROL) {
        modctrl = true;
    }
    else modctrl = false;
    int go = 0;
    int orient = wxHORIZONTAL;
    if (event.GetKeyCode() == WXK_DOWN) {
        go =-( m_plot->GetScrY() - m_plot->GetMarginTop() - m_plot->GetMarginBottom());
        orient = wxVERTICAL;
    }
    if (event.GetKeyCode() == WXK_UP) {
        go = (m_plot->GetScrY() - m_plot->GetMarginTop() - m_plot->GetMarginBottom());
        orient = wxVERTICAL;
    }
    if (event.GetKeyCode() == WXK_LEFT) {
        go = -(m_plot->GetScrX()-m_plot->GetMarginRight() - m_plot->GetMarginLeft());
    }
    if (event.GetKeyCode() == WXK_RIGHT) {
        go = m_plot->GetScrX()-m_plot->GetMarginRight() - m_plot->GetMarginLeft();
    }

    if ((go != 0)) {

        wxScrollWinEvent evt(wxEVT_NULL, go ,orient);
        double npos;
        if (orient == wxHORIZONTAL) {
            npos = m_plot->GetPosX() + (double)go / m_plot->GetScaleX();
                //m_plot->OnScrollThumbTrack(evt);
                m_plot->SetPosX(npos);
        }
        else {
            npos = m_plot->GetPosY() + (double)go / m_plot->GetScaleY();
            //m_plot->OnScrollThumbTrack(evt);
            m_plot->SetPosY(npos);

        }
        event.Skip(false);
        return;
    }

    event.Skip(true);

}
void frmPlot::OnFit(wxCommandEvent& WXUNUSED(event))
{
    m_plot->Fit();
}

void frmPlot::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxT("wxWidgets mathplot sample\n(c) 2003 David Schalig\n(c) 2007-2009 Davide Rondini and wxMathPlot team"));
}

void frmPlot::OnAlignXAxis(wxCommandEvent& WXUNUSED(event))
{
    axesPos[0] = (int)(axesPos[0] + 1) % 5;
    mpScaleX* xaxis = ((mpScaleX*)(m_plot->GetLayer(0)));
    mpScaleY* yaxis = ((mpScaleY*)(m_plot->GetLayer(1)));
    if (axesPos[0] == 0) {
        xaxis->SetAlign(mpALIGN_BORDER_BOTTOM);
        m_plot->SetMarginTop(0);
        m_plot->SetMarginBottom(0);
    }
    if (axesPos[0] == 1) {
        //((mpScaleX*)(m_plot->GetLayer(0)))->SetAlign(mpALIGN_BOTTOM);
        xaxis->SetAlign(mpALIGN_BOTTOM);
        m_plot->SetMarginTop(0);
        m_plot->SetMarginBottom(50);
    }
    if (axesPos[0] == 2) {
        //((mpScaleX*)(m_plot->GetLayer(0)))->SetAlign(mpALIGN_CENTER);
        xaxis->SetAlign(mpALIGN_CENTER);
        m_plot->SetMarginTop(0);
        m_plot->SetMarginBottom(0);
    }
    if (axesPos[0] == 3) {
        //((mpScaleX*)(m_plot->GetLayer(0)))->SetAlign(mpALIGN_TOP);
        xaxis->SetAlign(mpALIGN_TOP);
        m_plot->SetMarginTop(50);
        m_plot->SetMarginBottom(0);
    }
    if (axesPos[0] == 4) {
        ((mpScaleX*)(m_plot->GetLayer(0)))->SetAlign(mpALIGN_BORDER_TOP);
        xaxis->SetAlign(mpALIGN_BORDER_TOP);
        m_plot->SetMarginTop(0);
        m_plot->SetMarginBottom(0);
    }
    m_plot->UpdateAll();
}

void frmPlot::OnAlignYAxis(wxCommandEvent& WXUNUSED(event))
{
    axesPos[1] = (int)(axesPos[1] + 1) % 5;
    mpScaleX* xaxis = ((mpScaleX*)(m_plot->GetLayer(0)));
    mpScaleY* yaxis = ((mpScaleY*)(m_plot->GetLayer(1)));
    if (axesPos[1] == 0) {
        //((mpScaleY*)(m_plot->GetLayer(1)))->SetAlign(mpALIGN_BORDER_LEFT);
        yaxis->SetAlign(mpALIGN_BORDER_LEFT);
        m_plot->SetMarginLeft(0);
        m_plot->SetMarginRight(0);
    }
    if (axesPos[1] == 1) {
        //((mpScaleY*)(m_plot->GetLayer(1)))->SetAlign(mpALIGN_LEFT);
        yaxis->SetAlign(mpALIGN_LEFT);
        m_plot->SetMarginLeft(70);
        m_plot->SetMarginRight(0);
    }
    if (axesPos[1] == 2) {
        //((mpScaleY*)(m_plot->GetLayer(1)))->SetAlign(mpALIGN_CENTER);
        yaxis->SetAlign(mpALIGN_CENTER);
        m_plot->SetMarginLeft(0);
        m_plot->SetMarginRight(0);
    }
    if (axesPos[1] == 3) {
        //((mpScaleY*)(m_plot->GetLayer(1)))->SetAlign(mpALIGN_RIGHT);
        yaxis->SetAlign(mpALIGN_RIGHT);
        m_plot->SetMarginLeft(0);
        m_plot->SetMarginRight(70);
    }
    if (axesPos[1] == 4) {
        //((mpScaleY*)(m_plot->GetLayer(1)))->SetAlign(mpALIGN_BORDER_RIGHT);
        yaxis->SetAlign(mpALIGN_BORDER_RIGHT);
        m_plot->SetMarginLeft(0);
        m_plot->SetMarginRight(0);
    }
    m_plot->UpdateAll();
}

void frmPlot::OnToggleGrid(wxCommandEvent& event)
{
    //ticks = !ticks;
    ticks = !event.IsChecked();
    ((mpScaleX*)(m_plot->GetLayer(0)))->SetTicks(ticks);
    ((mpScaleY*)(m_plot->GetLayer(1)))->SetTicks(ticks);
    m_plot->UpdateAll();
}

void frmPlot::OnCopy(wxCommandEvent& event)
{
    // Write a bitmap to the clipboard wxImage image(wxT("splash.png"), wxBITMAP_TYPE_PNG);
    wxBitmapType fileType = wxBITMAP_TYPE_PNG;
//    wxSize imgSize(500, 500);
    m_plot->SaveScreenshot("", fileType, wxDefaultSize, false,true);
}

void frmPlot::OnToggleInfoLayer(wxCommandEvent& event)
{
    if (event.IsChecked())
        nfo->SetVisible(true);
    else
        nfo->SetVisible(false);
    m_plot->UpdateAll();
    event.Skip();
}

void frmPlot::OnBlackTheme(wxCommandEvent& event)
{
    //wxColor black(0,0,0);
    //wxColor white(255,255,255);
    wxColour grey(96, 96, 96);
    /*wxBrush* brush = new wxBrush(*wxTRANSPARENT_BRUSH)*/;
    m_plot->SetColourTheme(*wxBLACK, *wxWHITE, grey);
    m_plot->UpdateAll();
}

void frmPlot::OnPrintPreview(wxCommandEvent& WXUNUSED(event))
{
    // Pass two printout objects: for preview, and possible printing.
    mpPrintout* plotPrint = new mpPrintout(m_plot);
    mpPrintout* plotPrintPreview = new mpPrintout(m_plot);
    wxPrintPreview* preview = new wxPrintPreview(plotPrintPreview, plotPrint);
    wxPreviewFrame* frame = new wxPreviewFrame(preview, this, wxT("Print Plot"), wxPoint(100, 100), wxSize(600, 650));
    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);
}

void frmPlot::OnPrint(wxCommandEvent& WXUNUSED(event))
{
    wxPrinter printer;
    mpPrintout printout(m_plot, wxT("Plot print"));
    printer.Print(this, &printout, true);
}

void frmPlot::OnSaveScreenshot(wxCommandEvent& event)
{
    wxFileDialog fileDialog(this, _("Save a screenshot"), wxT(""), wxT(""), wxT("BMP image (*.bmp) | *.bmp|JPEG image (*.jpg) | *.jpeg;*.jpg|PNG image (*.png) | *.png"), wxFD_SAVE);
    if (fileDialog.ShowModal() == wxID_OK) {
        wxFileName namePath(fileDialog.GetPath());
        wxBitmapType fileType = wxBITMAP_TYPE_BMP;
        if (namePath.GetExt().CmpNoCase(wxT("jpeg")) == 0) fileType = wxBITMAP_TYPE_JPEG;
        if (namePath.GetExt().CmpNoCase(wxT("jpg")) == 0)  fileType = wxBITMAP_TYPE_JPEG;
        if (namePath.GetExt().CmpNoCase(wxT("png")) == 0)  fileType = wxBITMAP_TYPE_PNG;
        wxSize imgSize(500, 500);
        m_plot->SaveScreenshot(fileDialog.GetPath(), fileType, imgSize, false);
    }
    event.Skip();
}

void frmPlot::OnToggleShowSeries(wxCommandEvent& event)
{
    bool v = event.IsChecked();
    for (unsigned int p = 4; p < m_plot->CountAllLayers(); p++) {
        mpFXYVector* ly = dynamic_cast<mpFXYVector*>(m_plot->GetLayer(p));
        if (ly == NULL) continue;
        if ((ly->GetLayerType() == mpLAYER_PLOT) && (ly->IsVisible())) {
            ly->ShowName(v);
        }
    }
    //m_plot->SetLayerVisible(wxT("Lissajoux"), event.IsChecked());
    m_plot->UpdateAll();
}

void frmPlot::OnToggleShowSimbol(wxCommandEvent& event)
{
    //m_plot->SetLayerVisible(wxT("f(x) = SIN(x)"), event.IsChecked());
    mpLayer* ly = NULL;
    wxPen lpen;
    wxString label;
    bool show = event.IsChecked();
    for (unsigned int p = 0; p < m_plot->CountAllLayers(); p++) {
        ly = m_plot->GetLayer(p);
        if ((ly->GetLayerType() == mpLAYER_PLOT)) {
            mpFXYVector* ly = reinterpret_cast<mpFXYVector*>(m_plot->GetLayer(p));
            ly->ShowSimbol(show);
        }
    }
    m_plot->UpdateAll();
}

void frmPlot::OnToggleCosine(wxCommandEvent& event)
{
    m_plot->SetLayerVisible(wxT("g(y) = COS(y)"), event.IsChecked());
}

