#pragma once

class SourceViewDialog : public wxFrame
{
    ctlSQLBox* m_text1;
    ctlSQLBox* m_text2;
    bool m_changing_values;
    wxCheckBox* m_visibleSpace;
    wxCheckBox* m_showNumber;
    int s_indicHighlight;
    int pos = 0;
    int prev_line = -1;
public:
    SourceViewDialog::~SourceViewDialog() {
        //delete m_text1;
        //m_btn_close->UnBind();
    }
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

        showNumber(text, true);

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
    void addIndicText(ctlSQLBox* ctl, std::wstring tex, int indic) {
        wxString t(tex);

        ctl->AddText(t);
        if (indic > 0) {
            ctl->IndicatorFillRange(ctl->GetLength() - t.Len(), t.Len());
        }
    };
    void difftext(ctlSQLBox* ctlL, ctlSQLBox* ctlR, wxString sL, wxString sR) {
        //Diff_EditCost = 4;
        //Match_Threshold = 0.5;
        //Match_Distance = 1000;
        diff_match_patch dmp(4, 0.5, 1000);
        std::list<Diff> diffs;
        if (sL == sR) {
            return;
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
        s_indicHighlight = 20;
        ctlL->IndicatorSetForeground(s_indicHighlight, wxColour(246, 85, 100));
        
        ctlL->IndicatorSetAlpha(s_indicHighlight, 50);
        ctlL->IndicatorSetStyle(s_indicHighlight, wxSTC_INDIC_ROUNDBOX);
        ctlL->SetIndicatorCurrent(s_indicHighlight);

        //ctlR->IndicatorSetForeground(s_indicHighlight, wxColour(246, 185, 100));
        ctlR->IndicatorSetForeground(s_indicHighlight, wxColour(246, 85, 100));
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
                        cur_l += L"<span class=\"differenced\">" + t + L"</span>"; addIndicText(ctlL, t, s_indicHighlight);
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
    SourceViewDialog(wxWindow* parent, wxString sqlL, wxString sqlR, wxString title) :
        wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(1200, 700))
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
        wxButton* m_btn_next = new wxButton(m_panelOpt, wxID_ANY, wxT("Next"), wxDefaultPosition, wxDefaultSize, 0);

        bSizer4->Add(m_btn_next, 0, wxALL, 5);

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
        m_btn_next->Bind(wxEVT_BUTTON, &SourceViewDialog::onNext, this);
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
    void onNext(wxCommandEvent& evt) {
        //onClose(wxCloseEvent(NULL));

        int v = 0; 
        int l = -1;
        ctlSQLBox* t=NULL;
        int l1 = m_text1->GetLength();
        int l2 = m_text2->GetLength();
        m_text1->SelectNone();
        m_text2->SelectNone();
        while (v == 0 && pos < wxMax(l1, l2)) {
            if (pos < m_text1->GetLength()) v = m_text1->IndicatorAllOnFor(pos);
            if (v != 0) {
                t=m_text1;
            }
            else {
                if (pos < m_text2->GetLength()) v = m_text2->IndicatorAllOnFor(pos);
                if (v != 0) {
                    t = m_text2;
                }
            }
            if (t == NULL) {
                pos++;
                continue;
            }
            l = t->LineFromPosition(pos);
            pos++;
            if (l > prev_line) {
                t->SetCurrentPos(pos - 1);
                int start = pos - 1;
                prev_line = l;
                bool f = false;
                // find end fragment
                while (pos<t->GetLength()) { 
                    if (t->IndicatorAllOnFor(pos++) == 0) {
                        f = true;
                        break;
                    }
                }
                t->SetSelection(start,pos-1);
                t->SetFirstVisibleLine(l);
                break;
            }
            else {
                t = NULL;
                v = 0;
                continue;
            }
            
        }
        if (t == NULL) {
            pos = 0;
            prev_line = -1;
        }
    }
    void OnShowCheckBoxSpace(wxCommandEvent& evt) {
        bool bVal = m_visibleSpace->IsChecked();
        m_text1->SetViewWhiteSpace(bVal ? wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);
        m_text1->SetViewEOL(bVal ? 1 : 0);
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
