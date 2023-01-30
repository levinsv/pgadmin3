
// 
#include "pgAdmin3.h"

#include "frm/frmMain.h"
#include "ctl/ctlGitPanel.h"
#include "ctl/ctlSQLBox.h"
#include "wx/notebook.h"
#include "wx/artprov.h"
#include "wx/creddlg.h"
#include "wx/webrequest.h"
#include "wx/filedlg.h"
#include "wx/image.h"
#include "schema/pgObject.h"
#include "schema/pgServer.h"
#include "utils/json/jsonval.h"
#include "utils/json/jsonwriter.h"
#include "utils/json/jsonreader.h"
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/imaglist.h>
#include "images/gqbAdd.pngc"
#include "images/gqbRemove.pngc"
#include "images/conversion.pngc"
#include "../utils/diff_match_patch.h"

enum
{
    MARGIN_LINE_NUMBERS
};
#if wxUSE_WEBREQUEST


class SourceViewDialog : public wxFrame
{
    ctlSQLBox* m_text1;
    ctlSQLBox* m_text2;
    bool m_changing_values;
    wxCheckBox* m_visibleSpace;
    wxCheckBox* m_showNumber;

public:

    void onClose(wxCloseEvent& evt)
    {
        //EndModal(GetReturnCode());
        evt.Skip();
        
    }
    void showNumber(ctlSQLBox* text, bool visible)
    {
        if (visible) {
            long int width = text->TextWidth(wxSTC_STYLE_LINENUMBER,
                wxT(" ") + NumToStr((long int)text->GetLineCount()) + wxT(" "));
            if (width != text->GetMarginWidth(0))
            {
                text->SetMarginWidth(0, width);
                text->Update();
            }
        }
        else {
            text->SetMarginWidth(0, 0);
            text->Update();
        }
    }
    ctlSQLBox* createSTC()
    {
        ctlSQLBox* text = new ctlSQLBox(this, wxID_ANY);

//        text->SetMarginWidth(MARGIN_LINE_NUMBERS, 20);
//        text->StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColour(75, 75, 75));
//        text->StyleSetBackground(wxSTC_STYLE_LINENUMBER, wxColour(220, 220, 220));
//        text->SetMarginType(MARGIN_LINE_NUMBERS, wxSTC_MARGIN_NUMBER);
        text->SetEOLMode(2);

        showNumber(text,true);
        
        //text->StyleSetForeground(wxSTC_H_DOUBLESTRING, wxColour(255, 0, 0));
        //text->StyleSetForeground(wxSTC_H_SINGLESTRING, wxColour(255, 0, 0));
        //text->StyleSetForeground(wxSTC_H_ENTITY, wxColour(255, 0, 0));
        //text->StyleSetForeground(wxSTC_H_TAG, wxColour(0, 150, 0));
        //text->StyleSetForeground(wxSTC_H_TAGUNKNOWN, wxColour(0, 150, 0));
        //text->StyleSetForeground(wxSTC_H_ATTRIBUTE, wxColour(0, 0, 150));
        //text->StyleSetForeground(wxSTC_H_ATTRIBUTEUNKNOWN, wxColour(0, 0, 150));
        //text->StyleSetForeground(wxSTC_H_COMMENT, wxColour(150, 150, 150));

        return text;
    }
    void addIndicText(ctlSQLBox* ctl,std::wstring tex,int indic) {
        wxString t(tex);

        ctl->AddText(t);
        if (indic > 0) {
            ctl->IndicatorFillRange(ctl->GetLength()-t.Len(), t.Len());
        }
    };
    void difftext(ctlSQLBox* ctlL, ctlSQLBox* ctlR, wxString sL,wxString sR) {
        //Diff_EditCost = 4;
        //Match_Threshold = 0.5;
        //Match_Distance = 1000;
        diff_match_patch dmp(4, 0.5, 1000);
        std::list<Diff> diffs;
        if (sL == sR) {
            return ;
        }
        diffs = dmp.diff_main(sL.wc_str(), sR.wc_str(), true);
        int nstart = 0;
        int pos = 0;
        std::wstring cur_l;
        std::wstring ncur_l;// std::wstring p_ncur_l; std::wstring p_ncur_r;
        std::wstring cur_r;
        std::wstring ncur_r;
        std::wstring tex;
        std::wstring t;
        std::wstring tableline;
        int rline = 1, lline = 1;
        std::list<Diff>::const_iterator it; // ��������� ��������
        it = diffs.begin(); // ����������� ��� ������ ������
        Diff aDiff;
        bool modify = false;
        nstart = 0;
        int s_indicHighlight=20;
        ctlL->IndicatorSetForeground(s_indicHighlight, wxColour(246, 185, 100));
        ctlL->IndicatorSetAlpha(s_indicHighlight, 50);
        ctlL->IndicatorSetStyle(s_indicHighlight, wxSTC_INDIC_ROUNDBOX);
        ctlL->SetIndicatorCurrent(s_indicHighlight);

        ctlR->IndicatorSetForeground(s_indicHighlight, wxColour(246, 185, 100));
        ctlR->IndicatorSetAlpha(s_indicHighlight, 50);
        ctlR->IndicatorSetStyle(s_indicHighlight, wxSTC_INDIC_ROUNDBOX);
        ctlR->SetIndicatorCurrent(s_indicHighlight);

        while (it != diffs.end()) // ���� �������� �� ��������� �����
        {
            aDiff = *it;
            tex = aDiff.text;
            nstart = 0;
            while (nstart < tex.length()) {
                pos = tex.find('\n', nstart);
                if (pos == -1) { t.assign(tex, nstart, tex.length()); nstart = tex.length(); }
                else { t.assign(tex, nstart, pos - nstart); nstart = pos; }
                if (t.length() > 0) {
                    // ��� �� ��� ���� ������
                    if (aDiff.operation == Operation::INSERT) { 
                        cur_r += L"<span class=\"differencei\">" + t + L"</span>"; addIndicText(ctlR, t, s_indicHighlight);
                    }
                    if (aDiff.operation == 0) {
                        cur_l += L"<span class=\"differenced\">" + t + L"</span>"; addIndicText(ctlL,t, s_indicHighlight);
                    }
                    if (aDiff.operation == Operation::EQUAL) {
                        cur_r += t;
                        cur_l += t;
                        addIndicText(ctlL, t, 0);
                        addIndicText(ctlR, t, 0);
                    }
                    else modify = true;
                    // ���� �� �������� ������� ������ ������� ��� ��� �� ���� ������
                }
                else
                {
                    // ����� �� �������� \n
                    nstart = pos + 1;
                    ncur_l = std::to_wstring(lline);
                    ncur_l = L""; ncur_r = L"";
                    //				if (p_ncur_r==ncur_r) { ncur_r=L""; modify=true;}
                    //				if (p_ncur_l==ncur_l) { ncur_l=L""; modify=true;}

                    std::wstring t_cur_l = cur_l;
                    std::wstring t_cur_r = cur_r;
                    if (aDiff.operation == 0) {
                        t_cur_r = L"";
                        addIndicText(ctlL, L"\n", s_indicHighlight);
                        ncur_l = std::to_wstring(lline);
                        modify = true;
                        lline++;
                    }
                    else  if (aDiff.operation == Operation::INSERT) {
                        t_cur_l = L"";
                        addIndicText(ctlR, L"\n", s_indicHighlight);
                        ncur_r = std::to_wstring(rline);
                        modify = true;
                        rline++;
                    }
                    else if (aDiff.operation == Operation::EQUAL) {
                        ncur_r = std::to_wstring(rline);
                        ncur_l = std::to_wstring(lline);
                        rline++; lline++;
                        addIndicText(ctlL, L"\n", 0);
                        addIndicText(ctlR, L"\n", 0);
                    }
                   // if (modify) countdiffline++;
                    //				if (( (ncur_r.empty()&&(!ncur_l.empty()))
                    //					||(ncur_l.empty()&&(!ncur_r.empty()))
                    //					)&&(!modify)) modify=true;
                                    // create columns
                                    //left
                    tableline += L"<tr><td class=\"diff_next\" onclick=\"c(this)\"></td>";
                    tableline += modify ? L"<td class=\"has_difference\" onclick=\"d(this)\">" + ncur_l + "</td>" : L"<td class=\"diff_header\" onclick=\"d(this)\">" + ncur_l + "</td>";
                    tableline += L"<td class=\"lineContent\"><pre>" + t_cur_l + "</pre></td>";
                    // right
                    tableline += L"<td class=\"diff_next\" onclick=\"c(this)\"></td>";
                    tableline += modify ? L"<td class=\"has_difference\" onclick=\"d(this)\">" + ncur_r + "</td>" : L"<td class=\"diff_header\" onclick=\"d(this)\">" + ncur_r + "</td>";
                    tableline += L"<td class=\"lineContent\"><pre>" + t_cur_r + "</pre></td>";
                    tableline += L"</tr>";

                    if (aDiff.operation == 0) {
                        cur_l = L"";
                    }
                    else  if (aDiff.operation == Operation::INSERT) {
                        cur_r = L"";
                    }
                    else if (aDiff.operation == Operation::EQUAL) {
                        cur_r = L""; cur_l = L"";
                    }

                    //
                    modify = false;
                }
            } // ���� �� ������� ������ ������ Diff
            ++it;
        }

    }
    SourceViewDialog(wxWindow* parent, wxString sqlL, wxString sqlR) :
        wxFrame(parent, wxID_ANY, L"Compare git to DB", wxDefaultPosition, wxSize(900, 600))
    {
        m_changing_values = false;
        m_text1 = createSTC();
        //m_text1->SetText(sqlL);

        m_text2 = createSTC();
        //m_text2->SetText(sqlR);

        difftext(m_text1, m_text2, sqlL, sqlR);



        wxBoxSizer* bSizer1;
        bSizer1 = new wxBoxSizer(wxVERTICAL);

        wxPanel* m_panelSql = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
        wxBoxSizer* bSizer2;
        bSizer2 = new wxBoxSizer(wxHORIZONTAL);

        bSizer2->Add(m_text1, 1, wxEXPAND, 5);

        bSizer2->Add(m_text2, 1, wxEXPAND, 5);


        m_panelSql->SetSizer(bSizer2);
        m_panelSql->Layout();
        bSizer2->Fit(m_panelSql);
        bSizer1->Add(m_panelSql, 1, wxEXPAND, 2);

        wxPanel* m_panelOpt = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
        wxBoxSizer* bSizer4;
        bSizer4 = new wxBoxSizer(wxHORIZONTAL);

        m_visibleSpace = new wxCheckBox(m_panelOpt, wxID_ANY, wxT("Show space simbol"), wxDefaultPosition, wxDefaultSize, 0);
        bSizer4->Add(m_visibleSpace, 0, wxALL, 5);

        m_showNumber = new wxCheckBox(m_panelOpt, wxID_ANY, wxT("Show number line"), wxDefaultPosition, wxDefaultSize, 0);
        bSizer4->Add(m_showNumber, 0, wxALL, 5);


        bSizer4->Add(0, 0, 1, wxEXPAND, 5);

        wxButton* m_btn_close = new wxButton(m_panelOpt, wxID_ANY, wxT("Close"), wxDefaultPosition, wxDefaultSize, 0);
        bSizer4->Add(m_btn_close, 0, wxALL, 5);

//        wxButton *m_btn_cancel = new wxButton(m_panelOpt, wxID_ANY, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
//        bSizer4->Add(m_btn_cancel, 0, wxALL, 5);


        m_panelOpt->SetSizer(bSizer4);
        m_panelOpt->Layout();
        bSizer4->Fit(m_panelOpt);
        bSizer1->Add(m_panelOpt, 0, wxALL | wxEXPAND, 5);


        this->SetSizer(bSizer1);
        this->Layout();

        this->Centre(wxBOTH);

        m_visibleSpace->Bind(wxEVT_CHECKBOX, &SourceViewDialog::OnShowCheckBoxSpace, this);
        m_showNumber->Bind(wxEVT_CHECKBOX, &SourceViewDialog::OnShowCheckShowNumber, this);
        m_btn_close->Bind(wxEVT_BUTTON, &SourceViewDialog::onClose2, this);
        //Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(SourceViewDialog::onClose), NULL, this);
        //m_text1->Connect(wxEVT_STC_PAINTED, wxStyledTextEventHandler(SourceViewDialog::onScrollLeft), NULL, this);
        //m_text2->Connect(wxEVT_STC_PAINTED, wxStyledTextEventHandler(SourceViewDialog::onScrollRight), NULL, this);
        Bind(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(SourceViewDialog::onClose), this);
        m_text1->Bind(wxEVT_STC_PAINTED, wxStyledTextEventHandler(SourceViewDialog::onScrollLeft), this);
        m_text2->Bind(wxEVT_STC_PAINTED, wxStyledTextEventHandler(SourceViewDialog::onScrollRight), this);

    }
    void onClose2(wxCommandEvent& evt) {
       //onClose(wxCloseEvent(NULL));
       Close();
    }
    void OnShowCheckBoxSpace(wxCommandEvent& evt) {
        bool bVal= m_visibleSpace->IsChecked();
        m_text1->SetViewWhiteSpace(bVal ? wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);
        m_text1->SetViewEOL(bVal ? 1: 0);
        m_text2->SetViewWhiteSpace(bVal ? wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);
        m_text2->SetViewEOL(bVal ? 1 : 0);

    }
    void OnShowCheckShowNumber(wxCommandEvent& evt) {
        bool bVal = m_showNumber->IsChecked();
        showNumber(m_text1, bVal);
        showNumber(m_text2, bVal);
    }
    
    void onScrollLeft(wxStyledTextEvent& evt)
    {
        if (m_changing_values) return;
        m_changing_values = true;

        int fvl = m_text1->GetFirstVisibleLine();
        if (m_text2->GetFirstVisibleLine() != fvl)
        {
            m_text2->ScrollToLine(fvl);
            // ShowLines
        }

        m_changing_values = false;
    }

    void onScrollRight(wxStyledTextEvent& evt)
    {
        if (m_changing_values) return;
        m_changing_values = true;

        int fvl = m_text2->GetFirstVisibleLine();
        if (m_text1->GetFirstVisibleLine() != fvl)
        {
            m_text1->ScrollToLine(fvl);
            // ShowLines
        }

        m_changing_values = false;
    }
};

ctlGitPanel::ctlGitPanel(wxWindow* parent, frmMain* form, wxJSONValue cf) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
    formMain = form;
    cfg = cf;

    //, wxPoint(0, 0), wxDefaultSize, wxSUNKEN_BORDER | wxCLIP_CHILDREN

//        Bind(wxEVT_CLOSE_WINDOW, &WebRequestFrame::OnClose, this);

        // Prepare UI controls
    wxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    mainSizer->Add(new wxStaticText(this, wxID_ANY, "Request URL:"),
        wxSizerFlags().Border());
    m_urlTextCtrl = new wxTextCtrl(this, wxID_ANY,
        "https://www.wxwidgets.org/downloads/logos/blocks.png",
        wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER);
    mainSizer->Add(m_urlTextCtrl,
        wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT));
    //m_urlTextCtrl->Bind(wxEVT_TEXT_ENTER, &ctlGitPanel::OnLoadGitButton, this);

    m_notebook = new wxNotebook(this, wxID_ANY);
    m_notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &ctlGitPanel::OnNotebookPageChanged, this);

    // Image page
    //wxPanel* imagePanel = new wxPanel(m_notebook);
    //wxSizer* imageSizer = new wxBoxSizer(wxVERTICAL);

    //m_imageStaticBitmap = new wxStaticBitmap(imagePanel,
    //    wxID_ANY, wxArtProvider::GetBitmap(wxART_MISSING_IMAGE));
    //imageSizer->Add(m_imageStaticBitmap, wxSizerFlags(1).Expand());

    //imagePanel->SetSizer(imageSizer);
    //m_notebook->AddPage(imagePanel, "Image", true);

    // Text page
    wxPanel* textPanel = new wxPanel(m_notebook);
    wxSizer* textSizer = new wxBoxSizer(wxVERTICAL);

    m_postCheckBox = new wxCheckBox(textPanel, wxID_ANY, "Post request body");
    textSizer->Add(m_postCheckBox, wxSizerFlags().Border());
    m_postCheckBox->Bind(wxEVT_CHECKBOX, &ctlGitPanel::OnPostCheckBox, this);

    m_postRequestTextCtrl = new wxTextCtrl(textPanel, wxID_ANY,
        "app=WebRequestSample&version=1",
        wxDefaultPosition, wxSize(-1, FromDIP(60)), wxTE_MULTILINE);
    textSizer->Add(m_postRequestTextCtrl,
        wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT));

    textSizer->Add(new wxStaticText(textPanel, wxID_ANY, "Request body content type:"),
        wxSizerFlags().Border());
    m_postContentTypeTextCtrl = new wxTextCtrl(textPanel, wxID_ANY,
        "application/x-www-form-urlencoded");
    textSizer->Add(m_postContentTypeTextCtrl,
        wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT));

    textSizer->Add(new wxStaticText(textPanel, wxID_ANY, "Response body:"),
        wxSizerFlags().Border());
    m_textResponseTextCtrl = new wxTextCtrl(textPanel, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    m_textResponseTextCtrl->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    textSizer->Add(m_textResponseTextCtrl,
        wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));

    textPanel->SetSizer(textSizer);
    m_notebook->AddPage(textPanel, "Debug");

    // Download page
    //wxPanel* downloadPanel = new wxPanel(m_notebook);
    //wxSizer* downloadSizer = new wxBoxSizer(wxVERTICAL);
    //wxStaticText* downloadHeader = new wxStaticText(downloadPanel, wxID_ANY,
    //    "The URL will be downloaded to a file.\n"
    //    "Progress will be shown and you will be asked, where\n"
    //    "to save the file when the download completed.");
    //downloadSizer->Add(downloadHeader, wxSizerFlags().Expand().Border());
    //downloadSizer->AddStretchSpacer();
    //m_downloadGauge = new wxGauge(downloadPanel, wxID_ANY, 100);
    //downloadSizer->Add(m_downloadGauge, wxSizerFlags().Expand().Border());
    //m_downloadStaticText = new wxStaticText(downloadPanel, wxID_ANY, "-1-");
    //downloadSizer->Add(m_downloadStaticText, wxSizerFlags().Expand().Border());

    //downloadSizer->AddStretchSpacer();

    //downloadPanel->SetSizer(downloadSizer);
    //m_notebook->AddPage(downloadPanel, "Download");

    // Advanced page
    //wxPanel* advancedPanel = new wxPanel(m_notebook);
    //wxSizer* advSizer = new wxBoxSizer(wxVERTICAL);
    //wxStaticText* advHeader = new wxStaticText(advancedPanel, wxID_ANY,
    //    "As an example of processing data while\n"
    //    "it's being received from the server, every\n"
    //    "zero byte in the response will be counted below.");
    //advSizer->Add(advHeader, wxSizerFlags().Expand().Border());

    //advSizer->AddStretchSpacer();
    //m_advCountStaticText = new wxStaticText(advancedPanel, wxID_ANY, "0",
    //    wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
    //m_advCountStaticText->SetFont(m_advCountStaticText->GetFont()
    //    .MakeBold().MakeLarger().MakeLarger());
    //advSizer->Add(m_advCountStaticText, wxSizerFlags().Expand().Border());
    //advSizer->AddStretchSpacer();

    //advancedPanel->SetSizer(advSizer);

    //m_notebook->AddPage(advancedPanel, "Advanced");
// branch
    wxPanel* branchPanel = new wxPanel(m_notebook);
    wxSizer* brnSizer = new wxBoxSizer(wxVERTICAL);
    brnSizer->Add(new wxStaticText(branchPanel, wxID_ANY, "List branch:"),
        wxSizerFlags().Border());
//    brnSizer->Add(brnHeader, wxSizerFlags().Expand().Border());

    //brnSizer->AddStretchSpacer();
    m_Branch_List_Ctrl = new wxComboBox(branchPanel, wxID_ANY,
        "");
    brnSizer->Add(m_Branch_List_Ctrl, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT));

    m_branchListButton = new wxButton(branchPanel, wxID_ANY, "Branch List");
    m_branchListButton->Bind(wxEVT_BUTTON, &ctlGitPanel::OnBranchListButton, this);
    //m_cancelButton->Disable();
    brnSizer->Add(m_branchListButton, wxSizerFlags().Border());

    m_branchDeleteButton = new wxButton(branchPanel, wxID_ANY, "Branch Delete");
    m_branchDeleteButton->Bind(wxEVT_BUTTON, &ctlGitPanel::OnBranchDeleteButton, this);
    brnSizer->Add(m_branchDeleteButton, wxSizerFlags().Border());
    branchPanel->SetSizer(brnSizer);
    m_notebook->AddPage(branchPanel, "Branch");

// commits
    wxPanel* commitPanel = new wxPanel(m_notebook);
    wxSizer* cmtSizer = new wxBoxSizer(wxVERTICAL);
    cmtSizer->Add(new wxStaticText(commitPanel, wxID_ANY, "List commit files:"),
        wxSizerFlags().Border());
    //    brnSizer->Add(brnHeader, wxSizerFlags().Expand().Border());

        //brnSizer->AddStretchSpacer();
    
    m_commit_List_View = new wxListView(commitPanel, wxID_ANY, wxDefaultPosition, wxSize(-1,120), wxLC_LIST| wxSIMPLE_BORDER| wxLC_NO_HEADER);
    wxImageList *imaget = new wxImageList(19, 16,false,3);
    imaget->Add(*gqbAdd_png_img);
    imaget->Add(*gqbRemove_png_img);
    //imaget->Add(*conversion_png_img);
    wxBitmap image1 = wxBitmap(conversion_png_img->Scale(19, 16));
    imaget->Add(image1);
    m_commit_List_View->SetImageList(imaget, wxIMAGE_LIST_SMALL);
    m_commit_List_View->Bind(wxEVT_KEY_UP, &ctlGitPanel::OnKEY_UP, this);
    m_commit_List_View->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &ctlGitPanel::OnListRClick, this);
    
    cmtSizer->Add(m_commit_List_View, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT));

    cmtSizer->Add(new wxStaticText(commitPanel, wxID_ANY, "Message commit:"),
        wxSizerFlags().Border());
    m_CommentTextCtrl = new wxTextCtrl(commitPanel, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    //m_CommentTextCtrl->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    cmtSizer->Add(m_CommentTextCtrl,
        wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));

    m_commitButton = new wxButton(commitPanel, wxID_ANY, "commit");
    m_commitButton->Bind(wxEVT_BUTTON, &ctlGitPanel::OnCommitButton, this);
    //m_cancelButton->Disable();
    cmtSizer->Add(m_commitButton, wxSizerFlags().Border());

//    m_commitDeleteButton = new wxButton(commitPanel, wxID_ANY, "commit Delete");
//   m_commitDeleteButton->Bind(wxEVT_BUTTON, &ctlGitPanel::OncommitDeleteButton, this);
 //   cmtSizer->Add(m_commitDeleteButton, wxSizerFlags().Border());


    commitPanel->SetSizer(cmtSizer);
    m_notebook->AddPage(commitPanel, "Commit");

///////////////////////////

    mainSizer->Add(m_notebook, wxSizerFlags(1).Expand().Border());

    wxStdDialogButtonSizer* btn2Sizer = new wxStdDialogButtonSizer();
    m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    m_cancelButton->Bind(wxEVT_BUTTON, &ctlGitPanel::OnCancelButton, this);
    m_cancelButton->Disable();
    btn2Sizer->AddButton(m_cancelButton);

    m_startButton = new wxButton(this, wxID_OK, "&Load Git");
    m_startButton->Bind(wxEVT_BUTTON, &ctlGitPanel::OnLoadGitButton, this);
    btn2Sizer->AddButton(m_startButton);
    m_link= new wxHyperlinkCtrl(this, wxID_ANY, "","",wxDefaultPosition,wxSize(-1,-1));
   // btn2Sizer->AddStretchSpacer();
    btn2Sizer->Add(m_link, wxSizerFlags(1).Expand().Border(wxLEFT ).Top());
    

    btn2Sizer->Realize();
    mainSizer->Add(btn2Sizer, wxSizerFlags().Expand().Border());

    wxCommandEvent evt;
    OnPostCheckBox(evt);

    //SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    SetSizer(mainSizer);

    SetSize(FromDIP(wxSize(540, 500)));

    wxLogStatus(this, "%s", wxWebSession::GetDefault().GetLibraryVersionInfo().ToString());

    m_downloadProgressTimer.Bind(wxEVT_TIMER,
        &ctlGitPanel::OnProgressTimer, this);

    GetBranchList(true);
    if (!error_msg.IsEmpty()) {
        // error connect
        wxLogError("GitLab connect error.\n%s", error_msg);
        return;
    }
// load pgadmin3.json
    wxString jsonText = GetRepositoryFile("main", "pgadmin3.json");
    if (!jsonText.IsEmpty()) {
        wxJSONReader reader;
        wxJSONValue c;
        int errnum = reader.Parse(jsonText, &c);
        if (errnum > 0) {
            wxLogError("Branch 'main'. Parse json file pgadmin3.json errors. Number errors %d", errnum);
        }
        else
        {
            if ((!c["ignore_schema"].IsNull()) && cfg["ignore_schema"].IsNull()) cfg["ignore_schema"] = c["ignore_schema"];
            if ((!c["control_objects"].IsNull()) && cfg["control_objects"].IsNull()) cfg["control_objects"] = c["control_objects"];
            if ((!c["maps_branch_to_dbname"].IsNull()) && cfg["maps_branch_to_dbname"].IsNull()) cfg["maps_branch_to_dbname"] = c["maps_branch_to_dbname"];
        }

    }

}
void ctlGitPanel::OnListRClick(wxListEvent& evt) {
    int i = evt.GetIndex();
    wxListItem it=evt.GetItem();
    wxString pat = it.GetText();
    if (it) {
    }
    wxString sql_git = m_git_content[pat];
    wxString sql_db = m_base_content[pat];
    SourceViewDialog* dlg = new SourceViewDialog(NULL, sql_git,sql_db);
    dlg->Show();

}
void ctlGitPanel::OnKEY_UP(wxKeyEvent& event) {
    bool ctrl = false;
    if ((event.GetModifiers() & wxMOD_CONTROL) == wxMOD_CONTROL) {
        ctrl = true;
    }
    if (event.GetKeyCode() == WXK_DELETE && ctrl) {
        wxArrayInt num;
        for (int i = 0; i < m_commit_List_View->GetItemCount(); i++) {
            if (!m_commit_List_View->IsSelected(i)) continue;
            num.Add(i);
        }
        for (int j = num.Count() - 1; j >= 0; j--) {
            m_commit_List_View->DeleteItem(num[j]);
        }
        event.Skip(true);
    }
    if (event.GetKeyCode() == 'A' && ctrl) {
        for (int i = 0; i < m_commit_List_View->GetItemCount(); i++) {
            m_commit_List_View->Select(i);
        }
        event.Skip(true);
    }
    

}
ctlGitPanel::~ctlGitPanel()
{
    // We have to block until the web request completes, but we need to
    // process events while doing it.
    //Hide();
    if (m_currentRequest.IsOk())
        m_currentRequest.Cancel();

    while (m_currentRequest.IsOk())
    {
        wxYield();
    }
}
wxJSONValue ctlGitPanel::GetConfig() {
    wxString c;
    wxJSONValue cfg;
    wxString p = wxStandardPaths::Get().GetUserConfigDir() + wxFileName::GetPathSeparator() + "postgresql"+ wxFileName::GetPathSeparator() +"gitlab.json";
    if (!wxFileExists(p)) return cfg;
    wxFileInputStream input(p);
    if (input.IsOk()) {
        wxJSONReader reader;
        int errnum=reader.Parse(input, &cfg);
        if (errnum > 0) {
            wxLogError("Parse json file %s errors. Number errors %d",p,errnum);
        }
        else {
            
        }

    }
    return cfg;
}
wxJSONValue ctlGitPanel::execRequest(wxString url, wxJSONValue args,wxString cmd) {
    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
        this,
        url
    );
    wxJSONValue r;
    if (!request.IsOk()) {
        // This is not expected, but handle the error somehow.
        return r;
    }
    wxString pt;
    cfg["private_token"].AsString(pt);
    request.SetHeader("PRIVATE-TOKEN", pt);
    if (cmd != "GET") request.SetMethod(cmd);
    if (!args.IsNull()) {
        wxJSONWriter writer(wxJSONWRITER_NONE);
        wxString     str;
        writer.Write(args, str);
//        str = "{}";
        request.SetData(str, "application/json");
        m_postContentTypeTextCtrl->SetValue(str);
    }
    wxString rez;
    wxString answer;
    wxString link;
    response_link = "";
    // Bind state event
    Bind(wxEVT_WEBREQUEST_STATE, [&link, &rez, &answer](wxWebRequestEvent& evt) {
        switch (evt.GetState())
        {
            // Request completed
        case wxWebRequest::State_Completed:
        {
            answer = evt.GetResponse().AsString();
            rez.Printf("Loaded %lld bytes text data (Status: %d %s)",
                evt.GetResponse().GetContentLength(),
                evt.GetResponse().GetStatus(),
                evt.GetResponse().GetStatusText());
            link = evt.GetResponse().GetHeader("Link");
            break;
        }
        // Request failed
        case wxWebRequest::State_Unauthorized:
        {
            rez.Printf("%s", evt.GetErrorDescription());
            break;
        }
        case wxWebRequest::State_Failed:
            rez.Printf("%s", evt.GetErrorDescription());
            break;
        }
        });
    error_msg = "";
    request.Start();
    while (request.IsOk() && rez.Len() == 0)
    {
        wxYield();
    }
    m_textResponseTextCtrl->SetLabelText(rez);
    if (rez.Len() > 0 && !rez.StartsWith("Loaded")) {
        //m_textResponseTextCtrl->SetValue(answer);
        error_msg = rez;
    }
    wxJSONReader reader;
    wxJSONValue root;
    // now read the JSON text and store it in the 'root' structure
    // check for errors before retreiving values...
    if (!answer.IsEmpty()) {
        response_link = link;

        int numErrors = reader.Parse(answer, &root);
        if (numErrors > 0) {
           // m_downloadStaticText->SetLabelText("Error parser JSON text response");
        }
    }
    return root;
}
void ctlGitPanel::CommandBranch(wxString branchName,wxString cmd) {
    wxJSONValue r;
    wxString ur;
    cfg["url"].AsString(ur);
    wxString pr;
    cfg["project_id"].AsString(pr);
    ur = ur + "projects/" + pr + "/repository/branches";
    wxJSONValue rez;
    r["branch"] = branchName;
    if (cmd == "create") {
        r["ref"] = wxString("main");
        rez = execRequest(ur, r, "POST");
    }
    else {
        rez = execRequest(ur, r, "DELETE");

    }

}
void ctlGitPanel::GetBranchList(bool refresh) {
    wxJSONValue r;
    wxString ur;
    cfg["url"].AsString(ur);
    wxString pr;
    cfg["project_id"].AsString(pr);
    ur = ur+"projects/"+pr+"/repository/branches";
    
    if (m_Branch_List_Ctrl->GetCount()==0 || refresh) {
        m_Branch_List_Ctrl->Clear();
        wxJSONValue rez = execRequest(ur, r,"GET");
        for (int i = 0; i < rez.Size(); i++) {
            wxJSONValue b = rez[i];
            wxString name;
            b["name"].AsString(name);
            m_Branch_List_Ctrl->Append(name);
        }
    }
    int i;
    for (i = 0; i < m_Branch_List_Ctrl->GetCount(); i++) {
        if (m_Branch_List_Ctrl->GetString(i) == currentDBname) {
            m_Branch_List_Ctrl->SetSelection(i);
            break;
        }
    }
    if (currentDBname.IsEmpty()) return;

    if (i >= m_Branch_List_Ctrl->GetCount()) {
        CommandBranch(currentDBname, "create");
        m_Branch_List_Ctrl->Append(currentDBname);
        m_Branch_List_Ctrl->SetSelection(i);
    }
    

}
bool ctlGitPanel::CheckValidObject(pgObject *o) {
    if (o ) {
        if (o->GetMetaType() == PGM_VIEW
            || o->GetMetaType() == PGM_FOREIGNTABLE
            || o->GetMetaType() == PGM_FUNCTION
            || o->GetMetaType() == PGM_TABLE
            || o->GetMetaType() == PGM_TRIGGER
            || o->GetMetaType() == PGM_SCHEMA
            )
            return true;
    }
    if (o && !o->IsCollection()) {

    }
    return false;
}
void ctlGitPanel::ShowPage(pgObject* data) {
    m_textResponseTextCtrl->Clear();
   // wxString path(formMain->GetNodePath(data->GetId()));

    wxString path;
    wxString msg = "not support";
    ctlTree* browser= formMain->GetBrowser();
    wxTreeItemId node= data->GetId();
    path = browser->GetItemText(node).BeforeFirst('(').Trim();
    if (data->GetTypeName() == "Procedure" 
        || data->GetTypeName() == "Function"
        || data->GetTypeName() == "Trigger Function"
        || data->GetTypeName() == "Event Trigger"
        )
        path = browser->GetItemText(node).Trim();
    wxJSONValue r;
    wxString typenam= data->GetTypeName();
    bool ex = false;
    bool valid = true;
    r = cfg["control_objects"];
    if (!data->IsCollection()) {
        typenam = data->GetFactory()->GetCollectionFactory()->GetTypeName();
    } 
    if (!r.IsNull()) {
        bool nx = true;
        for (int j = 0; j < r.Size(); j++) {
            wxString ss = r[j].AsString();
            if (ss == typenam) { nx=false; break; };
        }
        if (nx) { valid = false; ex = true; msg = wxString::Format("'%s'not \"control_objects\"", typenam); };
    }
    wxTreeItemId parent = browser->GetItemParent(node);
    
    
    while (parent.IsOk())
    {
        pgObject* o = browser->GetObject(parent);
        if (o) typenam = o->GetTypeName();

        if (o && (typenam == "Schemas"
            || typenam == "Schema"
            )) {
            //valid = CheckValidObject(data);
            if (o && typenam == "Schema") {
                r = cfg["ignore_schema"];
                if (!r.IsNull()) {
                    for (int j = 0; j < r.Size(); j++) {
                        wxString ss = r[j].AsString();
                        if (ss == o->GetName()) {
                            valid = false;
                            msg = "schema ignore";
                            ex = true;
                        }
                    }
                    
                }
            }
        }
        if (typenam == "Database") {
            ex = true;
            currentDBname = o->GetName();
            nodeDB = o->GetId();
        }
        if (o && o->IsCollection()) {
            r = cfg["control_objects"];
            if (!r.IsNull()) {
                bool nx = true;
                for (int j = 0; j < r.Size(); j++) {
                    wxString ss = r[j].AsString();
                    if (ss == typenam) { nx = false; break; };
                }
                if (nx) { valid = false; ex = true; msg = wxString::Format("'%s'not \"control_objects\"", typenam); };
            }

        }
        path = browser->GetItemText(parent).BeforeFirst('(').Trim() + wxT("/") + path;
        o = browser->GetObject(parent);
        if (o && o->GetMetaType() == PGM_TABLE && !o->IsCollection()) {
            // without partitions
            path = browser->GetItemText(parent).BeforeFirst('(').Trim();
            node = parent;
            data = o;
        }
        parent = browser->GetItemParent(parent);
        if (ex) break;
    }
    if (syncDBname != currentDBname) {
        valid = false;
        msg = wxString::Format("diiference db name and load git db '%s'<>'%s'", currentDBname, syncDBname);
    }
    if (!valid) {
        m_link->SetLabel(msg);
        return;
    }
    wxString stype = data->GetTypeName();
    m_link->SetLabel(path+" - "+ stype);
    if (stype=="Schema") return;
    if (data->IsCollection()) { 

        return;
    }
    // need load gitlad
    if (syncDBname!=currentDBname || currentDBname.IsEmpty()) return;

    wxString sq;
    
    wxStringToStringHashMap::iterator it;
        it = m_treeName.find(path);
        if (it == m_treeName.end()) {
            m_treeName[path] = "new";
            it = m_treeName.find(path);
        }
            wxString v = it->second;
            sq = data->GetSql(browser);
            sq.Replace("\r\n", "\n");
            if (v == "new") {
                // add commit
                ReplaceItem(path, Files::Add_File);
                m_base_content[path] = sq;

            }
            else {
                wxString s2 = m_git_content[path];
                if (s2 != sq) {
                    m_treeName[path] = "update";
                    m_base_content[path] = sq;
                    ReplaceItem(path, Files::Update_File);
                }
                else {
                    m_treeName[path] = "equal";
                    ReplaceItem(path, -1);
                }

            }
}
int ctlGitPanel::ReplaceItem(wxString path,int image) {
    int i = m_commit_List_View->FindItem(-1, path);
    if (i != wxNOT_FOUND) m_commit_List_View->DeleteItem(i);
        else          i = m_commit_List_View->GetItemCount();
    if (image == -1) return i;
    return m_commit_List_View->InsertItem(i, path, image);
}
wxString url_encode(const wxString param) {
    
    wxCharBuffer  utf8CB = param.utf8_str();
    char *readBuff = utf8CB.data();
    size_t len = strlen(readBuff);
    wxString encode;
    for (int i = 0; i < len;i++) {
        char c = readBuff[i];
        if ((c>0) && ((isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~'|| c=='='))) {
            encode.Append(c);
            continue;
        }
        encode.Append(wxString::Format("%%%02X", (unsigned char)c));
    }
    
    return encode;

}
wxString base64_decode(const wxString enc) {
    wxString str;
    wxMemoryBuffer buf;
    buf = wxBase64Decode(enc);
    str = wxString::FromUTF8((char*)buf.GetData(), buf.GetDataLen());
    return str;
}
wxString base64_encode(const wxString dec) {
    wxString str;
    wxCharBuffer cb;
    cb=dec.ToUTF8();
    str = wxBase64Encode(cb.data(),cb.length());
    //str = wxString::FromUTF8((char*)buf.GetData(), buf.GetDataLen());
    return str;
}
wxString ctlGitPanel::getCurBranch(wxString dbname) {
    wxJSONValue r = cfg["maps_branch_to_dbname"];
    wxString br;
    if (!r.IsNull()) {
        for (int j = 0; j < r.Size(); j++) {
            wxJSONValue r2 = r[j];
            wxString  b= r2["branch"].AsString();
            wxJSONValue r3 = r2["list_db"];
            for (int k = 0; k < r3.Size(); k++) {
                if (dbname == r3[k].AsString()) {
                    br= b;
                    goto ex_for;
                }
            }
        }

    }
    br = dbname;
ex_for:
    int i=m_Branch_List_Ctrl->FindString(br);
    if (i < 0) {
        wxMessageBox("Not found branch for db name = " + dbname);
        br = "";
    }
    return br;
}
void ctlGitPanel::GetRepositoryTree(wxString branchName, wxString path, wxString typeElement, wxString value) {
    wxJSONValue r;
    wxString ur;
    //m_git_tree[path] = "need load";

    cfg["url"].AsString(ur);
    wxString pr;
    cfg["project_id"].AsString(pr);
    ur = ur + "projects/" + pr + "/repository/tree";
    wxJSONValue rez;
    wxJSONValue prrr;
    r["ref"] = getCurBranch(branchName);
    wxString rpath = path.AfterFirst('/');
    r["pagination"] = wxString("keyset");
    r["recursive"] = wxString("true");
    r["per_page"] = 90;
    r["path"] = wxString("");
    ur = ur + ArgsForGet(r);
    m_urlTextCtrl->SetValue(ur);
    while (true)
    {

    
    rez=execRequest(ur, prrr, "GET");
    if (rez.AsArray()) {
        wxString prefix = path.BeforeFirst('/');
        for (int j = 0; j < rez.Size(); j++) {
            wxJSONValue ss = rez[j];
            wxString name = branchName + "/"+ss["path"].AsString();
            if (ss["type"].AsString() != "tree") { 
                m_treeName[name] = "git";
                m_count_git++;
            }
            //if (name==path) m_treeName[path] = "both";
        }
        if (!response_link.IsEmpty()) {
            int st = 0;
            wxString tmp;
            while (true) {
                int p3 = response_link.find(',',st);
                if (p3 < 0) p3 = response_link.Len();
                if (p3 > 0) {
                    tmp = response_link.substr(st, p3 - st);
                    int p1 = tmp.Find("rel=\"next");
                    if (p1 < 0 && p3 == response_link.Len()) {
                        return;
                    };
                    if (p1 > 0) {
                        int e = tmp.Index('>');
                        int s = tmp.Index('<');
                        ur = tmp.substr(s + 1, e - s - 1);
                        break;
                    }
                    st = p3 + 1;

                }
                else break;
            }
            //ur = response_link;
            continue;
        }
    }
    break;
    }
    return;
}
wxString ctlGitPanel::ArgsForGet(wxJSONValue r) {
    wxArrayString key = r.GetMemberNames();
    wxString params;
    wxJSONValue rez;
    for (int i = 0; i < key.Count(); i++) {
        //if (key[i]=="path") 
            wxString p = key[i] + "=" + r[key[i]].AsString();
            if (params.IsEmpty()) params = "?";
            else params += "&";
            params = params +  url_encode(p);
    }

    return params;
}

void ctlGitPanel::OnCommitButton(wxCommandEvent& WXUNUSED(evt)) {
    wxJSONValue p,act,f;
    wxString br = getCurBranch(currentDBname);
    if (br != currentDBname) {
        wxMessageBox("Branch name not equals DB name. Commit denied.");
        return;
    }
    p["branch"] = br;
    int cnt = 0;
    wxString comment = m_CommentTextCtrl->GetValue();
    if (comment.IsEmpty()) {
        wxMessageBox("Commit name is Empty");
        return;
    }
    
    p["commit_message"] = comment;
    wxArrayInt num;
    for (int i = 0; i < m_commit_List_View->GetItemCount(); i++) {
        if (!m_commit_List_View->IsSelected(i)) continue;
        wxString path = m_commit_List_View->GetItemText(i);
        wxString rpath = path.AfterFirst('/');
        if (m_treeName[path] == "update") f["action"] = wxString("update");
        if (m_treeName[path] == "new") f["action"] = wxString("create");
        if (m_treeName[path] == "git") f["action"] = wxString("delete");
        f["file_path"] = rpath;
        if (f["action"].AsString() != "delete") {
            f["encoding"] = wxString("base64");
            wxString cont = m_base_content[path];
            wxString enc = base64_encode(cont);
            f["content"] = enc;
        }
        act.Append(f);
        num.Add(i);
        cnt++;

    }
    if (cnt > 0) {
        p["actions"] = act;
        bool re=ApplyCommit(currentDBname, p);
        if (re) {
            for (int j = num.Count() - 1; j >= 0; j--) {
                wxString path = m_commit_List_View->GetItemText(num[j]);
                m_commit_List_View->DeleteItem(num[j]);
                if (m_treeName[path] == "update") {
                    m_treeName[path] = "equal";
                    m_git_content[path]= m_base_content[path];
                }
                else if (m_treeName[path] == "new") {
                    m_treeName[path] = "equal";
                    m_git_content[path] = m_base_content[path];
                }
                else if (m_treeName[path] == "git") {
                    wxStringToStringHashMap::iterator it;
                    it=m_treeName.find(path);
                    m_treeName.erase(it);
                }



            }
        }
    }
}
bool ctlGitPanel::ApplyCommit(wxString branchName, wxJSONValue params) {
    wxJSONValue r;
    wxString ur;
    cfg["url"].AsString(ur);
    wxString pr;
    cfg["project_id"].AsString(pr);
    ur = ur + "projects/" + pr + "/repository/commits";
    wxJSONValue rez;
    //    r["pagination"] = wxString("keyset");
    //    r["recursive"] = wxString("true");
    //    r["path"] = wxString("");
    ur = ur + ArgsForGet(r);
    m_urlTextCtrl->SetValue(ur);
    rez = execRequest(ur, params, "POST");
    if (!rez.IsNull()) {
        wxString u = rez["web_url"].AsString();
        m_link->SetURL(u);
        return true;
    }
    return false;
}
wxString ctlGitPanel::GetRepositoryFile(wxString branchName, wxString path) {
    wxJSONValue r;
    wxString ur;
    //m_git_tree[path] = "need load";

    cfg["url"].AsString(ur);
    wxString pr;
    cfg["project_id"].AsString(pr);
    wxString rpath = path.AfterFirst('/');
    if (rpath.IsEmpty()) rpath = path;
    ur = ur + "projects/" + pr + "/repository/files/"+url_encode(rpath);
    wxJSONValue rez;
    r["ref"] = getCurBranch(branchName);
//    r["pagination"] = wxString("keyset");
//    r["recursive"] = wxString("true");
//    r["path"] = wxString("");
    ur = ur + ArgsForGet(r);
    m_urlTextCtrl->SetValue(ur);
    rez = execRequest(ur, rez, "GET");
    if (!rez.IsNull()) {
        wxString cont = rez["content"].AsString();
        wxString encod= rez["encoding"].AsString();
        wxString str;
        if (encod == "base64") {
            cont = base64_decode(cont);
        }
        m_git_content[path] = cont;
        return cont;
    }
    return "";
}
void ctlGitPanel::GetExpandedChildNodes(wxTreeItemId node, wxArrayString& expandedNodes, wxString pat,  int lvl)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId child;
    ctlTree* browser = formMain->GetBrowser();
    if (lvl == 0) child = node;
    else child = browser->GetFirstChild(node, cookie);
    pgObject* obj;
    wxString path;
    time_t tmp;
    int size = expandedNodes.Count();
    while (child.IsOk())
    {
        obj = browser->GetObject(child);
        wxString typenam;
        wxString s;
        if (obj) {
            typenam = obj->GetTypeName();
            wxJSONValue r;

            if (typenam == "Schema") {
                r = cfg["ignore_schema"];
                if (!r.IsNull()) {
                    for (int j = 0; j < r.Size(); j++) {
                        wxString ss = r[j].AsString();
                        if (ss == obj->GetName())  goto nex;
                    }

                }
            }

            if (obj->GetMetaType() == PGM_CATALOG
                || obj->GetMetaType() == PGM_COLUMN
                || obj->GetMetaType() == PGM_RULE
                || obj->GetMetaType() == PGM_CATALOG
                ) {
                child = browser->GetNextChild(node, cookie);
                continue;
            }
            r = cfg["control_objects"];
            if (!r.IsNull()&& obj->IsCollection()) {
                bool nx = true;
                for (int j = 0; j < r.Size(); j++) {
                    wxString ss = r[j].AsString();
                    if (ss == typenam) { nx = false; break; };
                }
                if (nx) goto nex;
            }

            if ((obj->GetMetaType() == PGM_SCHEMA
                || obj->GetMetaType() == PGM_DATABASE
                || obj->GetMetaType() == PGM_TABLE
                || obj->GetMetaType() == PGM_FOREIGNTABLE
                ) && !obj->IsCollection()) {
                obj->ShowTreeDetail(browser);
                //obj->ShowTree(parent,browser);
            }
            else
            {
                if (obj->GetMetaType() == PGM_VIEW) obj->ShowTreeDetail(browser); // ������ ��� ���� ����� �������� ���� � ���������
                if (obj->GetMetaType() == PGM_EVENTTRIGGER)  // �������� ���� � ��������� �� ��������
                    obj->ShowTreeDetail(browser);
            }

        }
        if (browser->HasChildren(child))
        {
            bool rec = true;
            if (obj && (obj->GetMetaType() == PGM_TABLE
                //||obj->GetMetaType()==PGM_VIEW
                )) {
                wxTreeItemId Item = browser->GetItemParent(child);
                obj = browser->GetObject(Item); // Tables
                wxTreeItemId Item2 = browser->GetItemParent(obj->GetId());
                obj = browser->GetObject(Item2); // Schemes
                if (obj && obj->GetMetaType() == PGM_SCHEMA && !obj->IsCollection()) {
                    rec = false; // �� �������� ���� �� ���. �������� � �������, � �� ������ �� �������
                    obj = browser->GetObject(child);
                    obj->ShowTreeDetail(browser);
                }
                else obj = browser->GetObject(child);
            }
            if (obj && (obj->GetMetaType() == PGM_VIEW && !obj->IsCollection())) rec = false;
            if (rec) {
                GetExpandedChildNodes(child, expandedNodes, pat + '/' + browser->GetItemText(child).BeforeFirst('(').Trim(), lvl + 1);
                //expandedNodes.Add(parent->GetNodePath(child));
                obj = browser->GetObject(child);
            }

        }
        if (obj) {
            
            s = obj->GetSql(browser);
            if ((typenam != "Schema" && typenam != "Database")&& !obj->IsCollection()) {
                //����� SQL ��� ����� �� ���������, ��� ��� ��� ��������� ������� � �� ���� � Git
                wxString p = pat + '/' + browser->GetItemText(child).Trim();
                m_link->SetLabel(p);
                m_count_db++;
                expandedNodes.Add(p);
                s.Replace("\r\n", "\n");
                m_base_content[p] = s;
                wxStringToStringHashMap::iterator it;
                it = m_treeName.find(p);
                if (it != m_treeName.end()) {
                    wxString v = it->second;
                    wxString g_path= it->first;
                    if (v == "git") {
                        wxString s2 = GetRepositoryFile(currentDBname, p);
                        if (!s2.IsSameAs(s)) {
                            m_treeName[p] = "update";
                            ReplaceItem(p, Files::Update_File);
                        }
                        else
                            m_treeName[p] = "equal";
                    }
                }
                else {
                    // not found git
                    m_treeName[p] = "new";
                    ReplaceItem(p, Files::Add_File);

                }

            }
        }
        
nex:
        child = browser->GetNextChild(node, cookie);
    }
}
void ctlGitPanel::OnLoadGitButton(wxCommandEvent& WXUNUSED(evt))
{
    wxTreeItemId node;
    wxArrayString expandedNodes;
    m_startButton->Disable();
    wxString msg = "Load gitlab db name = " + currentDBname;
    syncDBname = "";
    if (nodeDB.IsOk()) {
        m_git_content.clear();
        m_base_content.clear();
        m_treeName.clear();
        m_base_tree.clear();
        m_count_git = 0; m_count_db = 0;
        //m_commit_List_View
        m_commit_List_View->DeleteAllItems();
        if (getCurBranch(currentDBname).IsEmpty()) {
            goto ex;
        };

        wxString path = '/';
        GetRepositoryTree(currentDBname,path,"", "");
        if (!error_msg.IsEmpty()) { msg = error_msg;  goto ex; }
        GetExpandedChildNodes(nodeDB, expandedNodes, currentDBname, 1);
        wxStringToStringHashMap::iterator it= m_treeName.begin();
        // only git
        for (wxStringToStringHashMap::iterator en = m_treeName.end(); it != en; ++it)
        {
            path = it->first;
            if ((it->second) == "git") {
                ReplaceItem(path,Files::Remove_File);
            }
        }

        for (int i = 0; i < expandedNodes.GetCount(); i++) {
            wxString s = expandedNodes[i];
            //m_commit_List_View->InsertItem(m_commit_List_View->GetItemCount(), s, 2);
        }

        msg = wxString::Format("%s (git obj %d; db obj %d)",msg, m_count_git, m_count_db);
        syncDBname = currentDBname;
    }
ex:
    m_link->SetLabel(msg);
    m_startButton->Enable();
}

void ctlGitPanel::OnCancelButton(wxCommandEvent& WXUNUSED(evt))
{
    if (m_currentRequest.IsOk())
        m_currentRequest.Cancel();
}


void ctlGitPanel::OnProgressTimer(wxTimerEvent& WXUNUSED(evt))
{
}

void ctlGitPanel::OnPostCheckBox(wxCommandEvent& WXUNUSED(evt))
{
    m_postContentTypeTextCtrl->Enable(m_postCheckBox->IsChecked());
    m_postRequestTextCtrl->Enable(m_postCheckBox->IsChecked());
    wxColour textBg = wxSystemSettings::GetColour(
        (m_postCheckBox->IsChecked()) ? wxSYS_COLOUR_WINDOW : wxSYS_COLOUR_BTNFACE);

    m_postContentTypeTextCtrl->SetBackgroundColour(textBg);
    m_postRequestTextCtrl->SetBackgroundColour(textBg);
}


void ctlGitPanel::OnClose(wxCloseEvent& event)
{
    if (m_currentRequest.IsOk())
    {
        if (event.CanVeto())
        {
            wxMessageDialog dialog
            (
                this,
                "A web request is in progress, "
                "closing the window will cancel it.",
                "Please confirm",
                wxYES_NO
            );
            dialog.SetYesNoLabels("Cancel and close", "Don't close");

            if (dialog.ShowModal() != wxID_YES)
            {
                event.Veto();
                return;
            }
        }

        m_currentRequest.Cancel();
    }

    event.Skip();
}

void ctlGitPanel::OnNotebookPageChanged(wxBookCtrlEvent& event)
{
    //SourceViewDialog* dlg = new SourceViewDialog(NULL, "helo\nworld\n", "helo\nworld right\n3 line\n4 line\n");
    //dlg->Show();

}
#endif
