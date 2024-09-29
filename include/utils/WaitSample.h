#ifndef __WAITSAMPLE_H
#define __WAITSAMPLE_H


#include <map>
//#include <frm/frmStatus.h>
class frmStatus;
struct Sample
{
	int btime;
	int etime;
	int wait_id;
	int pid;
	int btype;	// backend_type = 0 Client backend
	long long qid;
	int samples;
};
enum iswhat  {
	WAIT_FULL,
	WAIT_GRP,
	WAIT_NAME
} ;
/// <summary>
/// qid,wait_id,pid
/// </summary>
struct key3 {
	long long qid;
	int w, pid;
	int sum;

	bool const operator==(const key3& o) const {
		return qid == o.qid && w == o.w && pid==o.pid;
	}
	bool const cmp2field(const key3& o) const {
		return qid == o.qid && w == o.w;
	}

	bool const operator<(const key3& o) const {
		return qid < o.qid || (qid == o.qid && w < o.w)|| (qid == o.qid && w == o.w && pid<o.pid);
	}
};
/// <summary>
/// compare pid,wait,qid
/// </summary>
struct key3p {
	long long qid;
	int w, pid;
	int sum;

	bool const operator==(const key3p& o) const {
		return qid == o.qid && w == o.w && pid == o.pid;
	}
	bool const cmp2field(const key3p& o) const {
		return qid == o.qid && w == o.w;
	}

	bool const operator<(const key3p& o) const {
		return pid < o.pid || (pid == o.pid && w < o.w) || (pid == o.pid && w == o.w && qid < o.qid);
	}
};

typedef std::vector<int> vec_int;
enum class Color_GUI{
	BG,
	CURSOR_LINE,
	GRID_LINE,
	LABEL
};
class WaitSample
{
private:
	int periodms = 10;
	int history_size;
	frmStatus *m_frmStatus;
	wxJSONValue opt;
	std::map<int, int> pids;
	std::map<int, int> chkpids;
// 
	std::vector<wxString> btype = { "client backend","background writer","checkpointer"
		,"walreceiver","walsender","walwriter","autovacuum launcher","autovacuum worker"
		,"logical replication launcher","logical replication worker","parallel worker"
		,"archiver","startup" };
	std::vector<wxString> btypeshort = { "CL","BGW","CKP"
		,"walR","walS","walW","avL","avW"
		,"lrL","lrW","pW"
		,"ARC","startup" };
	std::map<long long, wxString> qid_sql;
	std::map<wxString, long> wait_idx; // mapping wait_name -> color rgb
	std::map<wxString, int> wait_disable;
	std::map<wxString, int> wait_id;
	std::map<int, wxString> wait_id_name; // number wait -> full name wait
	std::vector<int> colors; // mapping colors[wait_id] -> color rgb
	std::vector<wxString> group;
	std::vector<Sample> smp; // 
	std::vector<Sample> tmp_smp; // 
	long long basetime = 0;
	int timebegiserios = 0, timeendserios = 0;
	int start_index_serios;
	std::vector<wxColour> color_gui;
	/// <summary>
	/// Получение номера группы по имени группы.
	/// </summary>
	/// <param name="wclass"></param>
	/// <returns></returns>
	int wait_class(wxString wclass);
	wxString m_file_cache_sql;
public:
	void SaveFileSamples();
	void LoadFileSamples();
	bool AddQuery(long long qid, wxString sql);
	wxString GetQueryByQid(long long qid);
	/// <summary>
	/// Получить Id backend_type
	/// Если неизвестный backend_type то он получит новый ID 
	/// </summary>
	/// <param name="backend_type"></param>
	/// <returns></returns>
	int GetBackendTypeId(wxString backend_type);
	wxString GetBackendTypeName(int backend_id) {
		return btype[backend_id];
	};
	wxString GetBackendTypeNameShort(int backend_id) {
		return backend_id>=btypeshort.size() ? GetBackendTypeName(backend_id): btypeshort[backend_id];
	};
	/// <summary>
	/// Получение индекса цвета по полному имени ожидания
	/// Если для ожидание нет цвета то используется цвет группы
	/// </summary>
	/// <param name="wait_name">Полное имя ожидания</param>
	/// <returns>Индекс в массиве colors</returns>
	long GetColorByWaitName(wxString wait_name) {
		auto clr = wait_idx.find(wait_name);
		if (clr == wait_idx.end()) {
			wxString grp_name = wait_name.BeforeFirst(':');
			clr = wait_idx.find(grp_name);
		}

		return clr->second;
	};
	/// <summary>
	/// Получение Текстового имени ожидания 
	/// </summary>
	/// <param name="wait_id">Ид ожидания</param>
	/// <param name="isGrp">WAIT_FULL - полное имя,WAIT_GRP - группа, WAIT_NAME - имя ожидания </param>
	/// <returns></returns>
	wxString GetName(int wait_id, iswhat isGrp) {
		wxString n = wait_id_name[wait_id];
		if (isGrp == WAIT_FULL) return n;
		wxString grp_name = n.BeforeFirst(':');
		if (grp_name.IsEmpty()) return n;
		if (isGrp==WAIT_GRP) return grp_name;
		else 
				return n.AfterFirst(':');
	};
	int GetCountWaits() { return wait_id.size(); }
	inline int d_time(long long ms) {
		return ms - basetime;
	}
	std::vector<Sample>* GetSamples();
	std::vector<int> GetColors();
	/// <summary>
	/// Получение начальной границы интервала для переданного времени.
	/// </summary>
	/// <param name="timeEnd">-1 текущая левая граница, или время</param>
	/// <param name="AggrigateInterval"> Агрегирующий интервал</param>
	/// <returns>Время начала интервала.</returns>
	int GetHomeInterval(int timeEnd, int AggrigateInterval);
	int GetInterval(int posStart, int posEnd);
	wxArrayString GetWaitIdNameArray(bool onlyGroup);
	int GetGroupingData(int timeEnd, // -1 для правой границы
		int needCountGroup,		     // сколько интервалов получить
		int groupInterval,			 // 1 min, 5 min, 10 min, 20 min, 30 min, 1 hour
		wxString groupRule,			 // как проводить групировку 
		wxArrayString& nameGroup,    // эаполняет именами ожиданий или групп ожиданий
		std::vector<wxDateTime>& xAxisValue,// границы интервалов
		std::vector<wxTimeSpan>& yAxisValue,
		std::vector<vec_int>& Values,
		std::vector<long>& clr
	);
	/// <summary>
	/// Получение позиции в массиве по указанному времени
	/// </summary>
	/// <param name="time"></param>
	/// <returns></returns>
	int getPositionByTime(int time);
	inline int getPeriod() { return periodms; }
	void SetConfig(long periodmills, long history_size,frmStatus *parent) {
		periodms = periodmills;
		this->history_size = history_size;
		m_frmStatus = parent;
	};
	void Init();
	
	WaitSample() {
		//c2.FromString("#3644ff");
		Init();
		m_frmStatus = NULL;
	};
		void BeginSeriosSample(long long timeserios) {
		chkpids.clear();
		if (basetime == 0) basetime = timeserios;
		timebegiserios = timeendserios;
		timeendserios = d_time(timeserios);
		start_index_serios = smp.size();

	}
	void EndSeriosSample();
	void AddSample(int pid, bool isXidTransation, wxString& active, const wxString& sample);
	wxColour & GetColorGui(Color_GUI gui_index) { return color_gui[(int)gui_index]; }
	bool RemoveFiles();
};

#endif