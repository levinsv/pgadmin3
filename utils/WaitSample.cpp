#include "pgAdmin3.h"
#include "utils/WaitSample.h"
#include "wx/stdpaths.h"
#include "utils/utffile.h"
#include "utils/json/jsonval.h"

void WaitSample::Init() {
	wxString clr = "h1 { \
		region: \"IO:DataFileRead\", #2132bd;\n\
region: \"IO:DataFileWrite\", #2132bd;\n\
region: \"IO:BufFileRead\", #16658d;\n\
region: \"IO:BufFileWrite\", #16658d;\n\
region: \"IO:DataFileExtend\", #9720ba;\n\
region: \"IO:DataFilePrefetch\", #2132bd;\n\
region: \"IO:DataFileFlush\", #2132bd;\n\
region: \"IO:DataFileSync\", #2132bd;\n\
region: \"IO:ReplicationSlotSync\", #b609ea;\n\
region: \"IO:WALSync\", #ff6a00;\n\
region: \"IO:WALWrite\", #ff6a00;\n\
region: \"IO:WALInitWrite\", #ff6a00;\n\
region: \"IO:WALRead\", #ff6a00;\n\
region: \"IO\", #2132bd;\n\
region: \"IPC\", #908b3b;\n\
region: \"Lock\", #ff0000;\n\
region: \"Activity\", #9fde72;\n\
region: \"Client\", #0b6222;\n\
region: \"BufferPin\", #a4a3a0;\n\
region: \"Client:ClientWrite\", #76bb88;\n\
region: \"IPC:ArchiveCommand\", #908b3b;\n\
region: \"IPC:MessageQueueReceive\", #aaae4f;\n\
region: \"IPC:MessageQueueSend\", #aaae4f;\n\
region: \"LWLock\", #87f566;\n\
region: \"LWLock:WALWrite\", #87f566;\n\
region: \"LWLock:WALInsert\", #87f566;\n\
region: \"LWLock:Autovacuum\", #87f566;\n\
region: \"LWLock:BufferContent\", #87f566;\n\
region: \"CPU\", #FFFA8A;\n\
region: \"Timeout\", #6ce4c6;\n\
region: \"Timeout:VacuumDelay\", #6ce4c6;\n\
region: \"Timeout:PgSleep\", #6ce4c6;\n\
}";
	wxStringTokenizer tk(clr, "\n", wxTOKEN_DEFAULT);
	wxString cc;
	wxString cnt;
	wxString ee;
	wxString l;
	wxJSONValue events(wxJSONType::wxJSONTYPE_ARRAY);
	while (tk.HasMoreTokens())
	{
		wxString l = tk.GetNextToken();
		wxString w = l.AfterFirst('"').BeforeFirst('"');
		if (w.IsEmpty()) continue;
		wxString c = l.AfterFirst('#').BeforeFirst(';');
		unsigned long tmp=0;
		wxSscanf(c, "%lx", &tmp);
		tmp = (tmp >> 16) & 0xFF | (tmp & 0x00FF00) | (tmp & 0xFF) << 16;
		wait_idx.emplace(w, tmp);
		wxJSONValue e(wxJSONType::wxJSONTYPE_OBJECT);
		wxColour cc3;
		cc3.Set(tmp);
		bool en = true;
		if (w == "Timeout:PgSleep" ||
			w == "Timeout:VacuumDelay" ||
			w == "IPC:ArchiveCommand" ||
			w == "Activity:RecoveryWalStream" ||
			w == "Activity:WalReceiverMain" ||
			w == "Activity:WalSenderMain" ||
			w == "Timeout:PgSleep")
			en = false; // disable for example
		e["enable"] = en;
		e["color"] = "#" + c;
		e["name"] = w;
		events.Append(e);
	}
	wxJSONValue def(wxJSONType::wxJSONTYPE_OBJECT);
	def["events"] = events;
	wxString cstr;
	cstr = "#808080";
	wxJSONValue c1=wxString(cstr); //BG
	wxJSONValue c2 = wxString("#000000"); //CURSOR_LINE
	cstr = "#c0c0c0";
	wxJSONValue c3 = wxString(cstr); //GRID_LINE
	wxJSONValue c4 = wxString("#000000"); //LABEL
	def["bg"] = c1;
	def["cursorline"] = c2;
	def["gridline"] = c3;
	def["label"] = c4;
	//def["autoloadcache_sql"] = true;
	group.push_back("BufferPin");
	group.push_back("Client");
	group.push_back("IO");
	group.push_back("IPC");
	group.push_back("Lock");
	group.push_back("Activity");
	group.push_back("LWLock");
	group.push_back("Timeout");
	group.push_back("CPU");
	settings->ReloadJsonFileIfNeed();
	settings->ReadJsonObect("WaitEvents", opt, def);
	//    settings->WriteJsonFile();
	if (!opt.IsNull()) {
		wait_idx.clear();
		wxJSONValue r = opt["events"];
		for (int j = 0; j < r.Size(); j++) {
			wxJSONValue e = r[j];
			bool enable = e["enable"].AsBool();
			wxString c= e["color"].AsString();
			unsigned long tmp;
			wxSscanf(c, "#%lx", &tmp);
			tmp = (tmp >> 16) & 0xFF | (tmp & 0x00FF00) | (tmp & 0xFF) << 16;
			wxColour cc(tmp);
			if (!cc.IsOk()) cc = *wxBLACK;
			//wxSscanf(c, "%lx", &tmp);
			wxString w = e["name"].AsString();
			wait_idx.emplace(w, tmp);
			if (!enable) wait_disable.emplace(w,0);
		}
	}
	else opt = def;
	// color gui
	wxColour cc1(opt["bg"].AsString());
	if (cc1.IsOk()) color_gui.push_back(wxColour(cc1)); else
		color_gui.push_back(wxColour(def["bg"].AsString()));
	cc1.Set(opt["cursorline"].AsString());
	if (cc1.IsOk()) color_gui.push_back(wxColour(cc1)); else
		color_gui.push_back(wxColour(def["cursor_line"].AsString()));
	cc1.Set(opt["gridline"].AsString());
	if (cc1.IsOk()) color_gui.push_back(wxColour(cc1)); else
		color_gui.push_back(wxColour(def["gridline"].AsString()));
	cc1.Set(opt["label"].AsString());
	if (cc1.IsOk()) color_gui.push_back(wxColour(cc1)); else
		color_gui.push_back(wxColour(def["label"].AsString()));

	wxString tempDir = wxStandardPaths::Get().GetUserConfigDir() + wxFileName::GetPathSeparator() + "postgresql" + wxFileName::GetPathSeparator() + "cache_sql.txt";
	m_file_cache_sql = tempDir;
	wxTextFile file(tempDir);
	if (file.Exists()) file.Open();
	if (file.IsOpened())
	{
		wxString str;
		wxString sql;
		long long qid;
		for (str = file.GetFirstLine(); !file.Eof(); str = file.GetNextLine())
		{
			if (str.length() == 16) {
				unsigned long long temp_qid = 0;
				int cntbits = 0;
				for (int i = str.length() - 1; i >= 0; i--, cntbits = cntbits + 4)
				{
					unsigned char h = str[i];
					unsigned char n09 = h - '0';
					unsigned char nAF = (h | 0x20) - 'a';
					if ((n09 <= (9 - 0)) || (nAF <= (0xf - 0xa))) {
						// is hex digits 
					}
					else break;
					unsigned long long tetra = ((h & 0xf) + (h >> 6) * 9);
					temp_qid |= tetra << cntbits;
				}
				if (cntbits == 64) {
					//wxString tmp = sql.BeforeLast('\n');
					//if (tmp.IsEmpty()) tmp=sql;
					if (sql.length() > 0) AddQuery(qid, sql); // previos sql
					qid = temp_qid;
					sql = wxEmptyString;
					continue;
				}
			}
			if (!sql.IsEmpty()) sql.Append('\n');
			sql.Append(str);
		}
		//file.ReadAll(&str);
		file.Close();
		if (sql.length() > 0)
		{
			//wxString tmp = sql.BeforeLast('\n');
			//if (tmp.IsEmpty()) tmp = sql;
			AddQuery(qid, sql); // previos sql

		}

	}
}
std::vector<Sample>* WaitSample::GetSamples() { return &smp; }
std::vector<int> WaitSample::GetColors() { return colors; }

wxArrayString WaitSample::GetWaitIdNameArray(bool onlyGroup) {
	typedef std::pair<wxString, int> pair;
	// sort map by value;
	std::vector<pair> vec;
	std::copy(wait_id.begin(),
		wait_id.end(),
		std::back_inserter<std::vector<pair>>(vec));
	std::sort(vec.begin(), vec.end(),
		[](const pair& l, const pair& r)
		{
			if (l.second != r.second) {
				return l.second < r.second;
			}

			return l.first < r.first;
		});
	wxArrayString a;
	for (auto const& pair : vec) {
		//std::cout << '{' << pair.first << "," << pair.second << '}' << std::endl;
		wxString n = pair.first;
		if (onlyGroup) {
			if (!n.BeforeFirst(':').IsEmpty()) n = n.BeforeFirst(':');
		}
		a.Add(n);

	}
	return a;
}
// return position in smp vector
int WaitSample::GetHomeInterval(int timeEnd, int AggrigateInterval) {
	Sample sa;
	if (timeEnd == -1) {
		// end smp
		sa = smp.back();
		timeEnd = sa.btime;
	}
	else {

	}
	long long th = ((basetime + timeEnd) / AggrigateInterval) * AggrigateInterval;
	int home_int = th - basetime;
	if (th > (basetime + timeEnd)) {
		//long long te = th - AggrigateInterval;
		home_int = th - AggrigateInterval - basetime;
	}
	return home_int;

	//wxLongLong l(((basetime + timeEnd)/AggrigateInterval)* AggrigateInterval);
	//wxDateTime t(l);
}
int WaitSample::getPositionByTime(int time) {

	int max = smp.size() - 1;
	int min = 0;
	int i = (max - min) / 2;
	while (i != -1) {
		int t = smp[i].btime;
		if (t < time) {
			min = i;
			i = min + (max - min) / 2;
		}
		else if (t > time) {
			max = i;
			i = min + (max - min) / 2;
		}
		else {
			while (i > 0 && smp[i - 1].btime == t) {
				i--;
			}
			break;
		}
		if (max - min < 2) {
			i = max;
			break;
		}
	}
	return i;

}

int WaitSample::wait_class(wxString wclass) {
	int i = 0;
	for (const auto& name : group) {
		if (name == wclass) return i;
		i++;
	}
	wxFAIL_MSG("Unknown wait class " + wclass);
	return -1000;
}
// return count group
int WaitSample::GetGroupingData(int timeEnd, int needCountGroup, int groupInterval,
	wxString groupRule, wxArrayString& nameGroup,
	std::vector<wxDateTime>& xAxisValue,
	std::vector<wxTimeSpan>& yAxisValue,
	std::vector<vec_int>& Values, // массив столбцов справа на лево
	std::vector<long>& clr	      // цета слоёв размерностью groupRule.Count()
)
{
	int idx_grp = wait_id.size();
	std::vector<int> filter_map(wait_id.size() + group.size());
	std::vector<int> summary(wait_id.size() + group.size());
	for (size_t i = 0; i < filter_map.size(); i++) filter_map[i] = -1;
	nameGroup.Clear();
	//std::map<int, wxString> wait_id_name;
	long long qidfilter = -1;
	for (const auto& kv : wait_id) {
		//std::cout << kv.first << " has value " << kv.second << std::endl;
		wxString w_name = kv.first;
		int w_id = kv.second;
		//wait_id_name.emplace(w_id, w_name);
		bool enable = true;
		wxString grp_name = w_name.BeforeFirst(':');
		if (grp_name.IsEmpty()) grp_name = w_name;
		int cl = wait_class(grp_name);
		bool isF = false;
		if (!groupRule.IsEmpty()) {
			wxStringTokenizer tk(groupRule, ";", wxTOKEN_DEFAULT);
			wxString l;
			bool isDis = true;
			bool isGrp = true;

			while (tk.HasMoreTokens())
			{
				l = tk.GetNextToken();
				if (l[0] == '@') {
					if (qidfilter == -1) {
						int cntbits = 0;
						unsigned long long temp_qid = 0;
						for (int i = l.length() - 1; i > 0; i--, cntbits = cntbits + 4)
						{
							unsigned char h = l[i];
							unsigned char n09 = h - '0';
							unsigned char nAF = (h | 0x20) - 'a';
							if ((n09 <= (9 - 0)) || (nAF <= (0xf - 0xa))) {
								// is hex digits 
							}
							else break;
							unsigned long long tetra = ((h & 0xf) + (h >> 6) * 9);
							temp_qid |= tetra << cntbits;
						}
						qidfilter = temp_qid;
					}
					continue;
				}
				isDis = l[0] == '-';
				if (isDis) l = l.substr(1);
				isGrp = l.find(':') == -1;
				if (w_name == l) {
					if (isDis) {
						filter_map[w_id] = w_id; //disable in Grp
						if (filter_map[idx_grp + cl] < 0) filter_map[idx_grp + cl] = -2;
					}
					else {
						//filter_map[w_id] = w_id;
						isF = false;	//default 
						break;
					}
					isF = true;
					break;
				}
				if (isGrp) {

					if (grp_name == l) {
						if (isDis) {
							// Groups equal and disable
							filter_map[w_id] = w_id; // disable w_id Grp ALL disable
							filter_map[idx_grp + cl] = -2; // disable group
							isF = true;
							break;
						}
						// default rule
						break;
					}
					continue;	// next rule
				}
			}
		}
		if (!isF) {
			// no found in Rule
			filter_map[w_id] = idx_grp + cl; // wait_id -> [idx_grp + cl]
			filter_map[idx_grp + cl] = 999;	// visible
			//filter_map[idx_grp+cl]=
		}


	}
	// nameGroup
	// begin Group
	wxString n, g;
	clr.clear();
	clr.resize(filter_map.size());
	for (size_t i = idx_grp; i < filter_map.size(); i++)
	{
		int wait_id = filter_map[i];
		if (wait_id == -1) continue;
		n = group[i - idx_grp];
		g = n;
		clr[i] = GetColorByWaitName(n);
		if (wait_id == -2) n = '-' + n;
		nameGroup.Add(n);
		// individual wait
		for (size_t j = 0; j < idx_grp; j++)
		{
			int wait_id = filter_map[j];
			n = wait_id_name[j];
			wxString grp_name = n.BeforeFirst(':');
			if (grp_name.IsEmpty()) grp_name = n;
			if (grp_name != g) continue;
			if (!n.AfterFirst(':').IsEmpty()) n = n.AfterFirst(':');
			if (wait_id == j) n = "--" + n; else n = "  " + n;
			nameGroup.Add(n);
			clr[j] = GetColorByWaitName(wait_id_name[j]);
		}


	}


	int homeInt = GetHomeInterval(timeEnd, groupInterval);
	Sample sa;
	if (timeEnd == -1) {
		sa = smp.back();
		timeEnd = sa.btime;
	}
	int idx = getPositionByTime(timeEnd);
	for (size_t i = 0; i < summary.size(); i++) summary[i] = 0;
	Values.clear();
	xAxisValue.clear();
	//for (size_t i = 0; i < maxS.size(); i++) maxS[i] = 0;
	long long  maxsum = 0;
	int minsum = 0;
	int nGrp = needCountGroup;
	while (idx >= 0) {
		sa = smp[idx--];
		if (sa.btime >= homeInt && idx >= 0) {
		}
		else {
			// new interval
			wxLongLong l(basetime + homeInt);
			wxDateTime t(l);
			xAxisValue.push_back(t);
			std::vector<int> itog(summary.size());
			int ni = idx_grp;
			int Ymax = 0;
			for (size_t i = 0; i < itog.size(); i++)
			{

				int wait_id = filter_map[i];
				if (wait_id < 0) { itog[i] = wait_id; continue; }
				int a = summary[i];
				itog[i] = a;
				if (i >= idx_grp) Ymax += a;
				//if (minsum > a) minsum = a;
			}
			if (maxsum < Ymax) maxsum = Ymax;
			Values.push_back(itog);
			homeInt -= groupInterval;
			nGrp--;
			if (nGrp == 0) break;
			for (size_t i = 0; i < summary.size(); i++) summary[i] = 0;
		}
		if (qidfilter != -1 && sa.qid != qidfilter) continue;
		int w_id = sa.wait_id;
		if (filter_map[w_id] < 0) continue;
		int ii = filter_map[w_id];
		// grp
		int s = sa.samples * periodms;
		if (ii != w_id) summary[ii] += s;
		// details
		summary[w_id] += s;
	}
	yAxisValue.clear();
	yAxisValue.push_back(wxTimeSpan(0, 0, 0, maxsum));
	return Values.size();
}
int WaitSample::GetInterval(int posStart, int posEnd) {
	Sample sa1, sa2;
	if (smp.size() == 0) return 0;
	if (posEnd == -1) {
		sa2 = smp.back();
	}
	else sa2 = smp.at(posEnd);
	sa1 = smp.at(posStart);
	return sa2.btime - sa1.btime;
}
bool WaitSample::AddQuery(long long qid, wxString sql) {
	if (qid_sql.find(qid) == qid_sql.end()) {
		qid_sql.emplace(qid, sql);
		return true;
	}
	return false;
}
wxString WaitSample::GetQueryByQid(long long qid) {
	if (qid_sql.find(qid) != qid_sql.end()) {
		wxString sql = qid_sql[qid];
		return sql;
	}
	else {
		// try get text sql
		if (m_frmStatus && m_frmStatus->getTextSqlbyQid(qid)) {
			wxString sql = qid_sql[qid];
			return sql;
		};
	}
	return wxEmptyString;

}
void WaitSample::LoadFileSamples() {
	wxString tempDir = wxStandardPaths::Get().GetUserConfigDir() + wxFileName::GetPathSeparator() + "postgresql" + wxFileName::GetPathSeparator() + "sample.dat";
	wxTextFile file(tempDir);
	file.Open();
	if (file.IsOpened())
	{
		wxString str;
		for (str = file.GetFirstLine() + '\n'; !file.Eof(); str += file.GetNextLine() + '\n')
		{

		}
		//file.ReadAll(&str);
		file.Close();

		wxStringTokenizer tk(str, "\n", wxTOKEN_DEFAULT);
		wxString cc;
		wxString cnt;
		wxString ee;
		wxString l;
		int nl = 0;
		while (tk.HasMoreTokens())
		{
			wxString l = tk.GetNextToken();
			if (l.IsEmpty()) continue;
			if (nl == 0) {
				wxSscanf(l, "%lld", &basetime);
				nl++;
				continue;
			}
			if (nl == 4) {
				//smp
					//wxString w_name = tk.GetNextToken();
					//wxString w_id = tk2.GetNextToken();
				int tmp = 0;
				Sample sa;
				if (wxSscanf(l, "%d %d %d %d %lld %d %d", &sa.btime, &sa.etime, &sa.wait_id, &sa.pid, &sa.qid, &sa.btype, &sa.samples) != 7) {
					wxLogError("Invalid count parameters '%s'.", l);
				}
				/*wxSscanf(tk2.GetNextToken(), "%d", &sa.btime);
				wxSscanf(tk2.GetNextToken(), "%d", &sa.etime);
				wxSscanf(tk2.GetNextToken(), "%d", &sa.wait_id);
				wxSscanf(tk2.GetNextToken(), "%d", &sa.pid);
				wxSscanf(tk2.GetNextToken(), "%lld", &sa.qid);
				wxSscanf(tk2.GetNextToken(), "%d", &sa.btype);
				wxSscanf(tk2.GetNextToken(), "%d", &sa.samples);*/
				smp.push_back(sa);
				continue;
			}

			wxStringTokenizer tk2(l, " ", wxTOKEN_DEFAULT);
			if (nl == 1) {
				//btype

				while (tk2.HasMoreTokens())
				{
					wxString b_name, b_id;
					while (b_id.IsEmpty()) {
						wxString str = tk2.GetNextToken();
						if (!wxIsdigit(str[0]))
							b_name += b_name.IsEmpty() ? str : " " + str;
						else b_id = str;
					}
					unsigned long tmp;
					wxSscanf(b_id, "%ld", &tmp);
					int i = GetBackendTypeId(b_name);
					if (i != tmp) {

					}
				}
				nl++;
				continue;
			}
			if (nl == 2) {
				//wait_id
				while (tk2.HasMoreTokens())
				{
					wxString w_name, w_id;
					while (w_id.IsEmpty()) {
						wxString str = tk2.GetNextToken();
						if (!wxIsdigit(str[0]))
							w_name += w_name.IsEmpty() ? str : " " + str;
						else w_id = str;
					}
					unsigned long tmp;
					wxSscanf(w_id, "%ld", &tmp);
					wait_id.emplace(w_name, tmp);
					wait_id_name.emplace(tmp, w_name);
				}
				nl++;
				continue;
			}
			if (nl == 3) {
				//colors
				int sz = wait_id.size();
				colors.resize(sz);
				int i = 0;
				while (tk2.HasMoreTokens())
				{
					wxString c = tk2.GetNextToken();
					unsigned long tmp;
					wxSscanf(c, "%ld", &tmp);
					colors[i++] = tmp;
				}
				nl++;
				continue;
			}
		}

	}
}
void WaitSample::SaveFileSamples() {
	wxString tempDir = wxStandardPaths::Get().GetUserConfigDir() + wxFileName::GetPathSeparator() + "postgresql" + wxFileName::GetPathSeparator() + "sample.dat";
	wxUtfFile file(tempDir, wxFile::write, wxFONTENCODING_UTF8);
	if (file.IsOpened())
	{
		wxString strnl;
		strnl = wxString::Format("%lld\n", basetime);
		file.Write(strnl);
		strnl.Clear();
		//btype
		int i = 0;
		for (const auto& e : btype) {
			if (!strnl.IsEmpty()) strnl += ' ';
			strnl += wxString::Format("%s %d", e, i);
			i++;
		}
		strnl += '\n';
		file.Write(strnl);
		// wait_id
		strnl.Clear();
		for (const auto& e : wait_id) {
			if (!strnl.IsEmpty()) strnl += ' ';
			strnl += wxString::Format("%s %d", e.first, e.second);
		}
		strnl += '\n';
		file.Write(strnl);
		strnl.Clear();
		for (const auto& e : colors) {
			if (!strnl.IsEmpty()) strnl += ' ';
			strnl += wxString::Format("%d", e);
		}
		strnl += '\n';
		file.Write(strnl);
		strnl.Clear();
		for (const auto& e : smp) {
			strnl.Append(wxString::Format("%d %d %d %d %lld %d %d\n", e.btime, e.etime, e.wait_id, e.pid, e.qid, e.btype, e.samples));
		}
		file.Write(strnl);
		file.Close();
	}
	
	tempDir = m_file_cache_sql;
	wxUtfFile file1(tempDir, wxFile::write, wxFONTENCODING_UTF8);
	if (file1.IsOpened())
	{
		wxString str;

		for (const auto& e : qid_sql) {
			wxLongLong l(e.first);

			str = wxString::Format("%0x%0x\n%s\n", l.GetHi(), l.GetLo(), e.second);
			file1.Write(str);
		}
		file1.Close();
	}
}
bool WaitSample::RemoveFiles() {
	bool rez = false;
	if (wxFileExists(m_file_cache_sql)) {
		wxRemoveFile(m_file_cache_sql);
		rez = true;
	}
	return rez;
}
void WaitSample::AddSample(int pid, bool isXidTransation, wxString& btype, const wxString& sample) {
	//PidWait pw(pid, basetime);;
	// поиск
	int pw;
	auto iter = pids.find(pid);
	if (iter != pids.end()) {
		//std::cout << "Found the key " << iter->first << " with the value " << iter->second << "\n";
		pw = iter->second;
	}
	else {
		pids.emplace(pid, pw);
	}
	++chkpids[pid];
	int bt = GetBackendTypeId(btype);
	//pw.AddSample(sample, active, timebegiserios, timeendserios);
	wxStringTokenizer tk(sample, ";", wxTOKEN_DEFAULT);
	wxString cnt;
	wxString w;
	wxString et;
	while (tk.HasMoreTokens())
	{
		wxString et = tk.GetNextToken();
		wxString cnt = et.AfterLast(':');
		et = et.BeforeLast(':');
		wxString tqid = et.AfterLast(':');
		wxString w = et.BeforeLast(':');
		// ignore ClientRead out transaction
		if (w == "Client:ClientRead" && !isXidTransation)
			continue;
		int smp_cnt;
		long long qid;
		wxSscanf(cnt, "%d", &smp_cnt);
		wxSscanf(tqid, "%lld", &qid);
		auto dis_it = wait_disable.find(w);
		if (dis_it != wait_disable.end()) {
			continue;
		}
		auto iter = wait_id.find(w);
		int id;
		if (iter == wait_id.end()) {
			id = wait_id.size();
			wait_id.emplace(w, id);
			wait_id_name.emplace(id, w);
			//map color
			long c = 0;
			auto it2 = wait_idx.find(w);
			if (it2 == wait_idx.end()) {
				it2 = wait_idx.find(w.BeforeFirst(':'));
				if (it2 != wait_idx.end()) {
					c = it2->second;
				}
			}
			else c = it2->second;
			colors.push_back(c);
			//colors
		}
		else id = iter->second;

		Sample sa;
		sa.btime = timebegiserios;
		sa.etime = timeendserios;
		sa.wait_id = id;
		sa.samples = smp_cnt;
		sa.pid = pid;
		sa.qid = qid;
		sa.btype = bt;
		smp.push_back(sa);
		//pw.AddSample();
	}

}
int WaitSample::GetBackendTypeId(wxString backend_type) {
	int idx = 0;
	auto it = std::find_if(btype.begin(), btype.end(), [&](const wxString& s) {
		idx++;
		return s == backend_type;
		}
	);
	if (it != btype.end()) return idx - 1;
	btype.push_back(backend_type);
	return  idx;
}
std::vector<int> tmp_idx_colors;
bool compareBySample(const Sample& a, const Sample& b)
{
	if (tmp_idx_colors[a.wait_id] < tmp_idx_colors[b.wait_id])
		return true;
	if (tmp_idx_colors[b.wait_id] < tmp_idx_colors[a.wait_id])
		return false;
	return a.samples < b.samples;
}

void WaitSample::EndSeriosSample() {
	int end_index = smp.size();
	if ((end_index - start_index_serios) < 2) return;
	// sort
	//(std::list<int>::iterator it = C.begin(), end = C.end(); it != end; ++it)
	std::vector<Sample>::iterator it = smp.end();
	it = it - (end_index - start_index_serios);
	tmp_idx_colors = colors;
	std::sort(it, smp.end(), compareBySample);
	return;
}
