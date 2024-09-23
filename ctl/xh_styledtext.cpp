//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// xh_styledtext.cpp - ctlStyledText handler
//
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"

#include "wx/wx.h"
#include "ctl/xh_styledtext.h"
#include "ctl/ctlStyledText.h"


IMPLEMENT_DYNAMIC_CLASS(ctlStyledTextXmlHandler, wxXmlResourceHandler)

ctlStyledTextXmlHandler::ctlStyledTextXmlHandler()
	: wxXmlResourceHandler()
{
	XRC_ADD_STYLE(wxTE_MULTILINE);
	XRC_ADD_STYLE(wxSIMPLE_BORDER);
	XRC_ADD_STYLE(wxSUNKEN_BORDER);
	XRC_ADD_STYLE(wxTE_RICH2);

	AddWindowStyles();
}


wxObject *ctlStyledTextXmlHandler::DoCreateResource()
{
	ctlStyledText *StyledText = new ctlStyledText(m_parentAsWindow, GetID(), GetPosition(), GetSize(), GetStyle());

	SetupWindow(StyledText);

	return StyledText;
}

bool ctlStyledTextXmlHandler::CanHandle(wxXmlNode *node)
{
	return IsOfClass(node, wxT("ctlStyledText"));
}
