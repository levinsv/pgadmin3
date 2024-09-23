#ifndef CTLTREEJSON_H
#define CTLTREEJSON_H



#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/timer.h>
#include "utils/json/jsonval.h"
#include <map>
class ctlTreeJSON : public wxTreeCtrl
{
public:
	ctlTreeJSON() :wxTreeCtrl(){};
//	ctlTreeJSON(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS);
	virtual ~ctlTreeJSON();
	void ReadJSON();
	void AddJSONValue(const wxJSONValue& parent, const wxJSONValue& jval);
	void LoadInTree(wxJSONValue& jval, const wxTreeItemId& idParent);
	void Save();
	void InitMy();
	DECLARE_DYNAMIC_CLASS(ctlTreeJSON)
		DECLARE_EVENT_TABLE()
private:
	wxTreeItemId addtree(const wxTreeItemId& idParent, wxString text, wxJSONValue* jval);
	void RefreshImageList();
	/// <summary>
	/// 
	/// -1 bad color
	/// 0 other element
	/// 1 - colour change
	/// </summary>
	/// <param name="newColour"> new color or empty</param>
	/// <param name="textLabel"> new text label or empty</param>
	int RefreshImages(const wxTreeItemId& id, wxColour newColour, wxString textLabel);
	void OnChar(wxKeyEvent& event);
	void OnDeleteItem(wxTreeEvent& event);
	void OnBeginEdit(wxTreeEvent& event);
	void OnEndEdit(wxTreeEvent& event);
	void OnDoubleClick(wxMouseEvent& event);
	void CopyNode(const wxTreeItemId& idSource);
	void BuildFind();
	wxTreeItemId NodeToJSON(const wxTreeItemId& id, wxJSONValue& newjson);
	wxTreeItemId findTreeItem(const wxTreeItemId& root, const wxString& text, bool bCaseSensitive, bool bExactMatch);
	wxTreeItemId nextTreeItem(const wxTreeItemId& item);
	wxJSONValue copyjson(wxJSONValue& src);
	std::map<wxTreeItemId, wxJSONValue> conf;
	std::map<wxTreeItemId, wxJSONValue> orig;
	std::map<wxTreeItemId, wxColour> colors;
	wxJSONValue m_tree;
	wxTreeItemId m_root;
	wxString m_FindString;
	std::map<wxTreeItemId, int> findsId;
	bool m_change = false;
};

#endif
