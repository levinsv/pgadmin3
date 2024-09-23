//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// xh_styledtext.h - wxStyledText handler
//
//////////////////////////////////////////////////////////////////////////


#ifndef _WX_XH_STYLEDTEXT_H_
#define _WX_XH_STYLEDTEXT_H_


#include "wx/xrc/xmlres.h"

//class WXDLLIMPEXP_XRC
class ctlStyledTextXmlHandler : public wxXmlResourceHandler
{
	DECLARE_DYNAMIC_CLASS(ctlStyledTextXmlHandler)
public:
	ctlStyledTextXmlHandler();
	virtual wxObject *DoCreateResource();
	virtual bool CanHandle(wxXmlNode *node);
};


#endif // _WX_XH_STYLEDTEXT_H_
