//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// pgPublication.cpp - Subscription class
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>

// App headers
#include "pgAdmin3.h"
#include "utils/misc.h"
#include "schema/pgSubscription.h"


pgSubscription::pgSubscription(const wxString &newName)
	: pgDatabaseObject(subscriptionFactory, newName)
{
}

wxString pgSubscription::GetTranslatedMessage(int kindOfMessage) const
{
	wxString message = wxEmptyString;

	switch (kindOfMessage)
	{
		case RETRIEVINGDETAILS:
			message = _("Retrieving details on subscription");
			message += wxT(" ") + GetName();
			break;
		case REFRESHINGDETAILS:
			message = _("Refreshing subscription");
			message += wxT(" ") + GetName();
			break;
		case DROPINCLUDINGDEPS:
			message = wxString::Format(_("Are you sure you wish to drop subscription \"%s\" including all objects that depend on it?"),
			                           GetFullIdentifier().c_str());
			break;
		case DROPEXCLUDINGDEPS:
			message = wxString::Format(_("Are you sure you wish to drop subscription \"%s\"?"),
			                           GetFullIdentifier().c_str());
			break;
		case DROPCASCADETITLE:
			message = _("Drop subscription cascaded?");
			break;
		case DROPTITLE:
			message = _("Drop subscription?");
			break;
		case PROPERTIESREPORT:
			message = _("Subscription properties report");
			message += wxT(" - ") + GetName();
			break;
		case PROPERTIES:
			message = _("Subscription properties");
			break;
		case DDLREPORT:
			message = _("Subscription DDL report");
			message += wxT(" - ") + GetName();
			break;
		case DDL:
			message = _("Subscription DDL");
			break;
		case DEPENDENCIESREPORT:
			message = _("Subscription dependencies report");
			message += wxT(" - ") + GetName();
			break;
		case DEPENDENCIES:
			message = _("Subscription dependencies");
			break;
		case DEPENDENTSREPORT:
			message = _("Subscription dependents report");
			message += wxT(" - ") + GetName();
			break;
		case DEPENDENTS:
			message = _("Subscription dependents");
			break;
	}

	return message;
}

bool pgSubscription::DropObject(wxFrame *frame, ctlTree *browser, bool cascaded)
{
	wxString sql = wxT("DROP SUBSCRIPTION ") + GetQuotedIdentifier();
	if (cascaded)
		sql += wxT(" CASCADE");
	return GetDatabase()->ExecuteVoid(sql);
}


wxString pgSubscription::GetSql(ctlTree *browser)
{
	if (sql.IsNull())
	{
		sql = wxT("-- Subscription: ") + GetQuotedIdentifier() + wxT("\n\n")
		      + wxT("-- DROP SUBSCRIPTION ") + GetQuotedIdentifier() + wxT(";")
		      + wxT("\n\n CREATE SUBSCRIPTION ") + GetName()+wxT("\n")
			  + wxT("                       CONNECTION  E") + qtConnString(GetConnInfo())+wxT("\n")
			  + wxT("                       PUBLICATION ") + GetPubStr()+wxT("");
		sql += wxT("\n  WITH (") + GetStrOper() + wxT(")");

		sql += wxT(";\n");
	}
	return sql;
}


void pgSubscription::ShowTreeDetail(ctlTree *browser, frmMain *form, ctlListView *properties, ctlSQLBox *sqlPane)
{
	if (properties)
	{
		CreateListColumns(properties);

		properties->AppendItem(_("Name"), GetName());
		properties->AppendItem(_("OID"), GetOid());
		properties->AppendItem(_("Owner"), GetOwner());
		properties->AppendItem(_("Publications"), GetPubStr());
		//properties->AppendYesNoItem(_("Relocatable?"), GetIsRelocatable());
		properties->AppendItem(_("Options"), GetStrOper());
		properties->AppendItem(_("Comment"), firstLineOnly(GetComment()));
	}
}



pgObject *pgSubscription::Refresh(ctlTree *browser, const wxTreeItemId item)
{
	pgObject *language = 0;
	pgCollection *coll = browser->GetParentCollection(item);
	if (coll)
		language = subscriptionFactory.CreateObjects(coll, 0, wxT("\n  and oid=") + GetOidStr());

	return language;
}



pgObject *pgSubscriptionFactory::CreateObjects(pgCollection *collection, ctlTree *browser, const wxString &restriction)
{
	wxString sql;
	pgSubscription *subscription = 0;
	bool superu=collection->GetDatabase()->GetConnection()->IsSuperuser();
	//wxString dbname=collection->GetDatabase()->GetConnection()->GetDbOid;
	OID db=collection->GetDatabase()->GetConnection()->GetDbOid();
	sql = wxT("select  oid,subname,pg_get_userbyid(subowner) AS \"owner\",subenabled,subconninfo,subslotname,subsynccommit,subpublications,obj_description(oid,'pg_subscription') as comment from pg_subscription  where subdbid=")
		  +NumToStr(db)+ wxT("\n")
	      + restriction + wxT("\n");
	pgSet *subscriptions = collection->GetDatabase()->ExecuteSet(sql);

	if (subscriptions)
	{
		while (!subscriptions->Eof())
		{
			wxString tmp;
			tmp = subscriptions->GetVal(wxT("subpublications"));
			tmp.Replace(wxT("{"), wxT(""));
			tmp.Replace(wxT("}"), wxT(""));

			subscription = new pgSubscription(subscriptions->GetVal(wxT("subname")));
			subscription->iSetDatabase(collection->GetDatabase());
			subscription->iSetOid(subscriptions->GetOid(wxT("oid")));
			subscription->iSetOwner(subscriptions->GetVal(wxT("owner")));
			subscription->iSetPubStr(tmp);
			subscription->iSetConnInfo(subscriptions->GetVal(wxT("subconninfo")));
		    subscription->iSetSlotName(subscriptions->GetVal(wxT("subslotname")));
			//subscription->iSetName(subscriptions->GetVal(wxT("subname"));
			subscription->iSetIsEnabled(subscriptions->GetBool(wxT("subenabled")));
			subscription->iSetIsSyncCommit(subscriptions->GetVal(wxT("subsynccommit")));
			subscription->iSetComment(subscriptions->GetVal(wxT("comment")));

			if (browser)
			{
				browser->AppendObject(collection, subscription);

				subscriptions->MoveNext();
			}
			else
				break;
		}

		delete subscriptions;
	}
	return subscription;
}


/////////////////////////////

pgSubscriptionCollection::pgSubscriptionCollection(pgaFactory *factory, pgDatabase *db)
	: pgDatabaseObjCollection(factory, db)
{
}


wxString pgSubscriptionCollection::GetTranslatedMessage(int kindOfMessage) const
{
	wxString message = wxEmptyString;

	switch (kindOfMessage)
	{
		case RETRIEVINGDETAILS:
			message = _("Retrieving details on subscriptions");
			break;
		case REFRESHINGDETAILS:
			message = _("Refreshing subscriptions");
			break;
		case OBJECTSLISTREPORT:
			message = _("Subscriptions list report");
			break;
	}

	return message;
}

///////////////////////////////////////////////////

#include "images/extension.pngc"
#include "images/extension-sm.pngc"
#include "images/extensions.pngc"
#include "images/slsubscription.pngc"
#include "images/slsubscriptions.pngc"

pgSubscriptionFactory::pgSubscriptionFactory()
	: pgDatabaseObjFactory(__("Subscription"), __("New Subscription..."), __("Create a new Subscription."), extension_png_img, extension_sm_png_img)
{
}


pgCollection *pgSubscriptionFactory::CreateCollection(pgObject *obj)
{
	return new pgSubscriptionCollection(GetCollectionFactory(), (pgDatabase *)obj);
}
dlgProperty *pgSubscriptionFactory::CreateDialog(frmMain *frame, pgObject *node, pgObject *parent)
{
	return NULL;
}

pgSubscriptionFactory subscriptionFactory;
static pgaCollectionFactory cf(&subscriptionFactory, __("Subscriptions"), slsubscriptions_png_img);
