#pragma once

#ifndef __FRMPLOT_H
#define __FRMPLOT_H
// wxWindows headers
#include "pgAdmin3.h"
#include "frmQuery.h"
#include "mathplot.h"

class frmPlot : public pgFrame
{
public:
    frmPlot(frmQuery* parent, const wxString& _title);
    ~frmPlot();
    void Go();
    void AddSeries(const wxString legend, const std::vector<double>& x, const std::vector<double>& y);
    void ClearAndSetAxis(wxString XtextAxis, unsigned int X_labelType, wxString YtextAxis);
    void OnAbout(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnPrintPreview(wxCommandEvent& event);
    void OnPrint(wxCommandEvent& event);
    void OnFit(wxCommandEvent& event);
    void OnAlignXAxis(wxCommandEvent& event);
    void OnAlignYAxis(wxCommandEvent& event);
    void OnToggleGrid(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnToggleInfoLayer(wxCommandEvent& event);
    void OnSaveScreenshot(wxCommandEvent& event);
    void OnToggleLissajoux(wxCommandEvent& event);
    void OnToggleShowSimbol(wxCommandEvent& event);
    void OnToggleCosine(wxCommandEvent& event);
    void OnBlackTheme(wxCommandEvent& event);

    mpWindow* m_plot;

private:
    void OnClose(wxCloseEvent& event);
    int axesPos[2];
    bool ticks;
    mpInfoCoords* nfo; // mpInfoLayer* nfo;
    //DECLARE_DYNAMIC_CLASS(frmPlot)
    int indexColor;
    DECLARE_EVENT_TABLE()
};



#endif