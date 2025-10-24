#include "pgAdmin3.h"
#include "ctl/ctlTreeJSON.h"
#include <wx/wx.h>
#include <wx/event.h>
#include <map>
#include <wx/colordlg.h>



BEGIN_EVENT_TABLE(ctlTreeJSON, wxTreeCtrl)
EVT_CHAR(ctlTreeJSON::OnChar)
//EVT_MOUSE_EVENTS(ctlTreeJSON::OnMouse)
EVT_TREE_BEGIN_LABEL_EDIT(wxID_ANY, ctlTreeJSON::OnBeginEdit)
EVT_TREE_END_LABEL_EDIT(wxID_ANY, ctlTreeJSON::OnEndEdit)
EVT_LEFT_DCLICK(ctlTreeJSON::OnDoubleClick)
EVT_TREE_DELETE_ITEM(wxID_ANY, ctlTreeJSON::OnDeleteItem)
END_EVENT_TABLE()
IMPLEMENT_DYNAMIC_CLASS(ctlTreeJSON, wxTreeCtrl)

void ctlTreeJSON::OnChar(wxKeyEvent& event) {
	wxTreeItemId id = GetSelection();
	if (!id.IsOk()) return;
	if (event.GetKeyCode() == WXK_INSERT) {
		CopyNode(id);
		return;
	}
	if (event.GetKeyCode() == WXK_DELETE) {
		if (HasChildren(id)) DeleteChildren(id);
		Delete(id);
		return;
	}
	if (event.GetKeyCode() == WXK_CONTROL_Z) {
		if (GetItemBackgroundColour(id) != GetBackgroundColour()) {
			if (orig.find(id) != orig.end()) {
				conf[id] = orig[id];
				wxString t = GetItemText(id);
				wxString newtext = orig[id].AsString();
				wxString key = t.BeforeFirst(':');
				if (key !=t) {
					newtext = key + ":" + newtext;
				}
				SetItemText(id, newtext);
				SetItemBackgroundColour(id, GetBackgroundColour());
				wxColour empty;
				int rez = 0;
				if (newtext != t && newtext.length() > 0) 
					rez=RefreshImages(id, empty, newtext);
				if (rez!=1) orig.erase(id);

				return;
			}

		}

	}
	if (event.GetKeyCode() == WXK_CONTROL_F) {
		wxTextEntryDialog dialog(this,
			wxT("Please enter find string\n")
			,
			wxT("Find"),
			m_FindString,
			wxOK | wxCANCEL);		//setName( dlg.GetValue().wc_str() );
		if (dialog.ShowModal() == wxID_OK) {
			m_FindString = dialog.GetValue();
			BuildFind();
		}
		return;
	}
	int find_next = 0;
	if (event.GetKeyCode() == WXK_F3) {
		find_next = 1;
	}
	if (event.GetKeyCode() == WXK_F3 && event.ShiftDown()) {
		find_next = -1;
	}
	if ((find_next == 1 || find_next == -1) && findsId.size() > 0) {
		wxTreeItemId sel, next, item = GetRootItem();
		while (item.IsOk()) {
			item = nextTreeItem(item);
			if (item.IsOk()) {
				if (id == item) {
					sel = id;
					if (find_next == -1 && next.IsOk()) {
						SelectItem(next);
						return;
					}
				}
				if (findsId.count(item) > 0) {
					next = item;
					if (sel.IsOk() && find_next == 1 && id != item) {
						SelectItem(next);
						return;
					}

				}
			}
		}
		return;
	}
	event.Skip();
};
void ctlTreeJSON::BuildFind() {
	for (const auto& [id, tmp] : findsId) {
		SetItemBackgroundColour(id, GetBackgroundColour());
	}
	findsId.clear();
	wxTreeItemId item;
	wxTreeItemId sel;
	wxTreeItemIdValue cookie;
	if (m_FindString.IsEmpty()) return;
	sel = GetSelection();
	item = GetFirstChild(GetRootItem(), cookie);
	if (!GetCount()) return; // empty tree control - i.e. just cleared it?
	wxString searchtext = m_FindString.Lower();
	wxTreeItemId last;
	bool flag = false;
	while (item.IsOk()) {
		wxString itemtext = GetItemText(item).Lower();
		if (sel == item) flag = true;
		if (itemtext.Contains(searchtext)) {
			//findsId.push_back(item);
			findsId.emplace(item, 0);
			SetItemBackgroundColour(item, wxColour("#FFFF00"));
			if (flag)
			{
				last = item;
				flag = false;
			}
		}
		item = nextTreeItem(item);
	}
	if (findsId.size() > 0)
	{
		if (last.IsOk())
			SelectItem(last);
		else
			wxBell();
	}
}
wxTreeItemId ctlTreeJSON::nextTreeItem(const wxTreeItemId& item) {
	wxTreeItemId child;
	wxTreeItemIdValue cookie;
	if (HasChildren(item))
		child = GetFirstChild(item, cookie);
	else
	{
		// Try a sibling of this or ancestor instead
		wxTreeItemId p = item;
		wxTreeItemId toFind;
		do
		{
			toFind = GetNextSibling(p);
			p = GetItemParent(p);
			if (GetRootItem() == p && !toFind.IsOk()) {
				return child;
			}
		} while (p.IsOk() && !toFind.IsOk());
		child = toFind;
	}
	return child;
}
wxTreeItemId ctlTreeJSON::findTreeItem(const wxTreeItemId& root, const wxString& text, bool bCaseSensitive, bool bExactMatch)
{
	wxTreeItemId item = root, child;
	wxTreeItemIdValue cookie;
	wxString findtext(text), itemtext;
	bool bFound;
	if (!bCaseSensitive) findtext.MakeLower();
	while (item.IsOk())
	{
		itemtext = GetItemText(item);
		if (!bCaseSensitive) itemtext.MakeLower();
		bFound = bExactMatch ? (itemtext == findtext) : itemtext.Contains(findtext);
		if (bFound) return item;
		child = GetFirstChild(item, cookie);
		if (child.IsOk()) child = findTreeItem(child, text, bCaseSensitive, bExactMatch);
		if (child.IsOk()) return child;
		item = GetNextSibling(item);

	} // while(item.IsOk())

	return item;
}
void ctlTreeJSON::OnDeleteItem(wxTreeEvent& event) {
	wxTreeItemId id = event.GetItem();
	if (colors.find(id) != colors.end()) {
		colors.erase(id);
	}
	if (conf.find(id) != conf.end()) {
		conf.erase(id);
		m_change = true;
	}

};
wxColour getColorFromString(const wxString& str) {
	wxString strcolor = str.AfterFirst('#');
	wxColour empty;
	if (!strcolor.IsEmpty() && strcolor.length() > 5) {
		wxString strc = "#" + strcolor.Mid(0, 6);
		unsigned long tmp;
		int scanned = wxSscanf(strcolor, "%lx", &tmp);
		if (scanned == 1) {
			wxColour c;
			c.Set((tmp>>16) & 0xFF | (tmp  & 0x00FF00)| (tmp & 0xFF)<<16);
			if (c.IsOk()) return c;
		}
	}
	return empty;
}
int ctlTreeJSON::RefreshImages(const wxTreeItemId& id, wxColour newColour, wxString textLabel) {
	//SetIma
	int rez = 0;
	if (colors.find(id) != colors.end()) {
		rez = -1;
		auto old_color = colors.at(id);
		if (textLabel.length() > 0) {
			// change label
			newColour = getColorFromString(textLabel);
			if (!newColour.IsOk()) return rez;
		}
		if (old_color.GetRGB() != newColour.GetRGB()) {
			colors.erase(id);
			colors[id] = newColour;
			wxString oldt = GetItemText(id);
			wxString newtextcolor = newColour.GetAsString(wxC2S_HTML_SYNTAX);
			int pos = oldt.Find('#');
			if (pos < 0) {
				pos = 0;
			}
			wxString newlabel = oldt.Left(pos) + newtextcolor;
			SetItemText(id, newlabel);
			wxJSONValue v = conf[id];
			conf[id] = newtextcolor;
			m_change = true;
			if (orig.find(id) != orig.end()) {
				if (conf[id].AsString() == orig[id].AsString()) {
					SetItemBackgroundColour(id, GetBackgroundColour());
					orig.erase(id);
				}
			}
			else {
				// save original value
				orig[id] = v;
				SetItemBackgroundColour(id, wxColour("#c0ffff"));
			}

			RefreshImageList();
			rez = 1;
		}
	}
	return rez;
}
void ctlTreeJSON::OnDoubleClick(wxMouseEvent& event) {
	wxPoint p = event.GetPosition();
	int flags;
	wxTreeItemId id = HitTest(p, flags);
	wxString s;
	wxJSONValue v;
	if (id.IsOk()) {
		s = GetItemText(id);

		if (conf.count(id) > 0) v = conf[id];
	}

	if (flags & wxTREE_HITTEST_ONITEMLABEL) {
		wxString s = GetItemText(id);
		wxString str = s.AfterFirst(':');
		wxString key = s.BeforeFirst(':');
		if (str.IsEmpty()) str = s;
		int im = GetItemImage(id);
		s = wxString::Format("val=%s n_image=%d", str, im);
		if (key!=s) {
			if (v.GetType() == wxJSONTYPE_BOOL) {
				bool b = !v.AsBool();
				wxString newstr = "true";
				if (!b) newstr = "false";
				wxTreeEvent e;
				wxString nv = key + ":" + newstr;
				e.SetLabel(nv);
				e.SetItem(id);
				e.Allow();
				OnEndEdit(e);
				if (e.IsAllowed()) {
					SetItemText(id, nv);
				}
			}
		}
		//wxMessageBox(s);
	}
	else if (flags & wxTREE_HITTEST_ONITEMICON) {
		//if (v.AsString())

		if (colors.find(id) != colors.end()) {
			wxColourData data;
			wxColour initialColourToUse = colors.at(id);
			data.SetColour(initialColourToUse);
			wxColourDialog dlg(this, &data);
			dlg.Bind(wxEVT_COLOUR_CHANGED, [](wxColourDialogEvent& event) {
				//Redraw(event.GetColour());
				});
			if (dlg.ShowModal() == wxID_OK) {
				// Colour did change.
				RefreshImages(id, dlg.GetColourData().GetColour(), "");
			}
			else {
				// Colour didn't change.
			}
		}
	}
};
void ctlTreeJSON::OnBeginEdit(wxTreeEvent& event) {
	//wxTrap();
};
void ctlTreeJSON::OnEndEdit(wxTreeEvent& event) {
	if (event.IsEditCancelled()) return;
	wxString newtext = event.GetLabel();
	auto id = event.GetItem();
	wxString oldtext = GetItemText(id);
	auto parent_id = GetItemParent(id);
	wxString name;
	wxJSONValue v = conf[id];
	if (v.IsArray() || v.IsObject()) {
		event.Veto();
		return;
	}
	if (parent_id.IsOk()) {
		wxJSONValue p = conf[parent_id];
		wxString newvaluetext = newtext;
		wxString n; // name parameter
		int idx = -1; // index array
		if (p.IsObject()) {
			n = oldtext.BeforeFirst(':');
			if (!(n.length() > 0 && p.HasMember(n))) {
				event.Veto();
				return;
			}
			// 
			if (!(n.length() > 0 && newtext.StartsWith(n + ":"))) {
				// name need equals
				event.Veto();
				return;
			}
			newvaluetext = newtext.Mid(n.length() + 1);
		}
		else if (p.IsArray()) {
			for (int j = 0; j < conf[parent_id].Size(); ++j) {
				if (conf[parent_id][j].GetRefData() == conf[id].GetRefData()) { idx = j; break; }
			}
			if (idx == -1) {
				//	wxTrap();
			}
		}

		//change jsonvalue
		if (v.IsBool()) {
			bool bb = false;
			if (newvaluetext == "true") bb = true;
			//if (n.IsEmpty()) p[idx] = bb;  else p[n] = bb;
			conf[id] = bb;
			m_change = true;
		}
		else if (v.IsInt()) {
			int ii = 0;
			int scaned = wxSscanf(newvaluetext, "%d", &ii);
			if (scaned != 1) {
				event.Veto();
				return;
			}
			//if (n.IsEmpty()) p[idx] = ii;  else p[n] = ii;
			conf[id] = ii;
			m_change = true;
		}
		else {
			// string value
			//if (n.IsEmpty()) p[idx] = newvaluetext;  else p[n] = newvaluetext;
			conf[id] = newvaluetext;
			m_change = true;
			//if (n.IsEmpty()) conf[parent_id][idx] = conf[id];  else conf[parent_id][n] = conf[id];
		}
		//p[n] = newtext.Mid(n.length());
		if (orig.find(id) != orig.end()) {
			if (conf[id].AsString() == orig[id].AsString()) {
				SetItemBackgroundColour(id, GetBackgroundColour());
				orig.erase(id);
			}
		}
		else {
			// save original value
			orig[id] = v;
			SetItemBackgroundColour(id, wxColour("#c0ffff"));
		}




	}

	wxColour empty;
	if (newtext != oldtext && newtext.length() > 0) {
		int rez = RefreshImages(id, empty, newtext);
		if (rez > 0) return;
		if (rez < 0)event.Veto(); // bad colour
		else
		{
			// no colour

		}
	}
};
wxTreeItemId ctlTreeJSON::NodeToJSON(const wxTreeItemId& id, wxJSONValue& newjson) {
	wxTreeItemId item = id, child;
	wxTreeItemIdValue cookie;
	wxString itemtext;
	int i = 0;
	while (item.IsOk())
	{
		itemtext = GetItemText(item);
		wxJSONValue j = conf.at(item);
		wxJSONValue n;
		n.SetType(j.GetType());
		//		wxJSONValue v = conf.at(item);
		//		j = v;
				//wxJSONType t = v.GetType();
		child = GetFirstChild(item, cookie);

		if (child.IsOk()) child = NodeToJSON(child, n);
		if (newjson.IsObject()) {
			wxString key = itemtext.BeforeFirst(':');
			if (key.Right(2) == "{}" || key.Right(2) == "[]") key = key.Left(key.length() - 2);
			wxString vl = itemtext.AfterFirst(':');
			if (n.IsArray() || n.IsObject()) newjson[key] = n;
			else
				newjson[key] = j;
		}
		else if (newjson.IsArray()) {
			if (n.IsArray() || n.IsObject()) newjson[i] = n;
			else
				newjson[i] = j;
			i++;
		}

		//if (child.IsOk()) return child;
		item = GetNextSibling(item);
	} // while(item.IsOk())

	return item;
}
wxJSONValue ctlTreeJSON::copyjson(wxJSONValue& src) {
	wxJSONValue v;
	v.SetType(src.GetType());
	if (src.IsArray()) {
		for (int j = 0; j < src.Size(); ++j) {
			wxJSONValue m = copyjson(src[j]);
			v.Append(m);
		}
	}
	else if (src.IsObject()) {
		wxArrayString arr = src.GetMemberNames();
		for (int j = 0; j < arr.Count(); ++j) {
			v[arr[j]] = copyjson(src[arr[j]]);
		}
	}
	else {
		bool b = false;
		int i = 0;
		wxString s;
		src.AsBool(b);
		src.AsInt(i);
		src.AsString(s);
		if (src.IsBool()) v = b;
		if (src.IsInt())
			v = i;
		if (src.IsString()) v = s;
	}
	return v;
}
void ctlTreeJSON::CopyNode(const wxTreeItemId& idSource) {
	wxJSONValue v = conf[idSource];
	wxTreeItemId pid = GetItemParent(idSource);
	wxJSONValue pv = conf.at(pid);
	wxJSONValue n = copyjson(conf[idSource]);

	wxTreeItemId nid;
	if (pv.IsArray()) {
		//if (v.IsObject()) nid = NodeToJSON(idSource, n);
		int vsize = pv.Size();
		wxString label = wxString::Format("[%d]", vsize);
		if (n.IsObject() || n.IsArray()) {
			nid = addtree(pid, label, &n);
			conf[nid] = n;
			LoadInTree(n, nid);
			SelectItem(nid);
			//EnsureVisible(nid);
			m_change = true;
		}
		else {
			//conf[pid][vsize]=n;
			//LoadInTree(conf[pid], pid);
			nid = addtree(pid, n.AsString(), &n);
			conf[nid] = n;

			m_change = true;
		}


	}
	else if (pv.IsObject()) {
		wxString label = GetItemText(idSource);
		wxString suf = label.Right(2);
		label = label.Mid(0, label.length() - 2);
		int vsize = v.Size();
		label = wxString::Format("%s%d", label, vsize);
		label += suf;
		if (v.IsObject() || v.IsArray()) {
			//nid = NodeToJSON(idSource, n);
			if (vsize == 0) {
				//copy empty array 
				wxJSONValue newString;
				wxString text = "new string value";
				newString = text;
				newString = conf[idSource].Append(newString);
				nid = addtree(idSource, text, &newString);
				conf[nid] = newString;
				m_change = true;
				return;
			}
			else {
				if (v.IsObject() || v.IsArray()) return;
				nid = addtree(pid, label, &n);
				conf[nid] = n;
				LoadInTree(n, nid);
				m_change = true;
			}
		}
		else {
			//wxString key = GetItemText(idSource).BeforeFirst(':');

			return;
		}
		// copy only Object or Array
		nid = addtree(pid, label, &n);
		LoadInTree(n, nid);
	}
	else {
		return;
	}
	RefreshImageList();
}
//void ctlTreeJSON::AddJSONValue(const wxJSONValue& parent, const wxJSONValue& jval);
wxTreeItemId ctlTreeJSON::addtree(const wxTreeItemId& idParent, wxString text, wxJSONValue* jval) {
	wxTreeItemId item;
	if (idParent.IsOk()) {
		item = AppendItem(idParent, text, -1, -1, 0);
	}
	else {
		item = AddRoot(text, -1, -1, 0);
	}
	if (m_root == idParent) {
		SetItemBackgroundColour(item, *wxLIGHT_GREY);
	}
	if (text.length() >= 7) {
		wxColour c = getColorFromString(text);
		if (c.IsOk()) {
			colors[item] = c;
		}
	}
	return item;
};
void ctlTreeJSON::RefreshImageList() {
	auto it = GetFirstVisibleItem();
	wxRect r;
	wxSize sz(15, 15);
	if (GetBoundingRect(it, r)) {
	#ifdef __WXGTK__
		sz.x=r.height;
		sz.y=r.height;
	#else
		sz.x = r.height - 2;
		sz.y = r.height - 2;
	#endif // __WXMSW__
	}
	//else return;
	wxVector<wxBitmapBundle> images;
	int n = 0;
	for (const auto& [item, color] : colors) {
		wxMemoryDC dc;
		wxBitmap bmp(sz.x, sz.y);
		wxMask* mask = new wxMask();
		int w = sz.x, h = sz.y;
		bmp.SetMask(mask);
		dc.SelectObject(bmp);
		if (color == wxNullColour)
			dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
		else
			dc.SetBrush(wxBrush(color));

		dc.DrawRectangle(0, 0, w, h);
		images.push_back(wxBitmapBundle(bmp));
		//SetItemImage(item, n);
		//wxString s = GetItemText(item);
		n++;
	}
	if (images.size() > 0) {
		SetImages(images);
		n = 0;
		for (const auto& [item, color] : colors) {
			SetItemImage(item, n);
			n++;
		}

	}
}
void ctlTreeJSON::LoadInTree(wxJSONValue& jval, const wxTreeItemId& idParent) {
	wxJSONValue def;
	if (jval.AsArray()) {
		for (int i = 0; i < jval.Size(); ++i) {
			wxString key = "";
			wxJSONValue jv = jval.Item(i);
			wxTreeItemId item;
			if (!(jv.IsObject() || jv.IsArray())) key += "" + jv.AsString();
			else
				key = wxString::Format("[%d]", i);
			item = addtree(idParent, key, &jv);
			conf[item] = jv;
			if ((jv.IsObject() || jv.IsArray()))
				LoadInTree(jv, item);
		}
	}
	else
		if (jval.IsObject()) {
			wxArrayString arr = jval.GetMemberNames();
			// sort
			struct ss {
				int sort_id;
				wxString key;
			};
			std::vector<ss> ord;
			for (int i = 0; i < arr.Count(); ++i) {
				const wxJSONValue& jv = jval.Get(arr[i], def);
				if (jv.AsArray()) ord.push_back({ 5000,arr[i] });
				else if (jv.IsObject())  ord.push_back({ 4000,arr[i] });
				else ord.push_back({ 3000,arr[i] });
			}
			std::sort(ord.begin(), ord.end(),
				[](const ss& lhs, const ss& rhs) {return std::tie(lhs.sort_id, lhs.key) < std::tie(rhs.sort_id, rhs.key); });
			// append tree
			for (const auto& j : ord) {
				wxJSONValue jv = jval.Item(j.key);
				wxTreeItemId item;
				wxString key = j.key;
				if (!(jv.IsObject() || jv.IsArray())) key += ":" + jv.AsString();
				if ((jv.IsArray())) key += "[]";
				if ((jv.IsObject())) key += "{}";
				item = addtree(idParent, key, &jv);
				conf[item] = jv;
				if ((jv.IsObject() || jv.IsArray())) LoadInTree(jv, item);
			}
		}
		else {
			wxTreeItemId item;
			wxJSONValue* vv = &jval;
			wxString text = jval.AsString();
			item = addtree(idParent, text, vv);
			conf[item] = jval;
		}
};

//ctlTreeJSON::ctlTreeJSON(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style): wxTreeCtrl(parent, id, pos, size, style) {};

void ctlTreeJSON::InitMy() {
	//ReadJSON();
	settings->ReloadJsonFileIfNeed();

	wxJSONValue r(wxJSONTYPE_NULL);
	DeleteAllItems();
	colors.clear();
	conf.clear();
	orig.clear();
	findsId.clear();
	m_FindString = "";
	m_tree =settings->jsoncfg;
	if (m_tree.IsNull())  {
	} else {
		m_root = addtree(m_root, "root", &r);
		LoadInTree(m_tree, m_root);
		RefreshImageList();
#ifdef __WXMSW__
	// GTK 3 bug visualization
        ExpandAll();
#endif // __WXMSW__
	}
	m_change = false;
}
void ctlTreeJSON::Save() {
	if (m_change) {
		wxTreeItemId item = m_root, child;
		wxTreeItemIdValue cookie, cookie2;
		wxJSONValue v(wxJSONTYPE_OBJECT);
		wxString itemtext;
		int i = 0;
		child = GetFirstChild(m_root, cookie);
		while (child.IsOk())
		{
			wxString name = GetItemText(child);
			name = name.Left(name.length() - 2);
			wxJSONValue v2(wxJSONTYPE_OBJECT);
			child = GetFirstChild(child, cookie2);
			NodeToJSON(child, v2);
			v[name] = v2;
			settings->WriteJsonObect(name, v2);
			child = GetNextChild(m_root, cookie);
		}
		settings->WriteJsonFile();
		m_change = false;
	}

}
ctlTreeJSON::~ctlTreeJSON()
{
//	Save();
}

