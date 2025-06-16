#pragma once

class Table;

class Cols
{
public:
    wxString name;
    int num;
    // Для v - ссылка view/table текущего уровня (самый нижний уровень).
    // Для r - сыылка на FK таблицу
    Table *linkTable=NULL;
    int linknumcol;
    wxString linkOid;
    bool pk = false;
    //(самый нижний уровень).
    Table* relTable = NULL;
    int relcol = 0;
    bool operator< (const Cols& e) const
    {
        return (name < e.name);
    }
    bool operator==(const Cols& otherPoint) const
    {
        if (this->name == otherPoint.name ) return true;
        else return false;
    }

    struct HashFunction
    {
        size_t operator()(const Cols& point) const
        {
            size_t xHash = std::hash<wxString>()(point.name);
            //size_t yHash = std::hash<wxString>()(point.y) << 1;
            return xHash;
        }
    };
};
class Table
{
public:
    Table(const wxString &tablename);
    Table() {};
    const wxString& GetName(){ return name; };
    const wxString& GetSchema() { return schema; };
    const wxString& GetAlias() { return alias; };
    void SetAlias(const wxString str) { alias=str; };
    const wxString& GetOID() { return oid; };
    const int GetColsCount() { return cols.size(); };
    Cols& operator[](unsigned index) { return cols[index]; }
    void SetCol(int index,Cols & valcol){ cols[index]=valcol; }
    char GetType() { return type; };
    void Set(wxString kind,wxString sch,wxString toid, wxString tname);
    void AddColumn(wxString ncol, wxString colname, wxString oidTable, wxString ncolTable);
    bool operator< (const Table& e) const
    {
        return  (schema < e.schema)||(schema == e.schema && name < e.name);
    }
    bool operator==(const Table& otherPoint) const
    {
        if (this->name == otherPoint.name && this->schema== otherPoint.schema) return true;
        else return false;
    }

    struct HashFunction
    {
        size_t operator()(const Table& point) const
        {
            size_t xHash = std::hash<wxString>()(point.name);
            size_t yHash = std::hash<wxString>()(point.schema) << 1;
            return xHash ^ yHash;
        }
    };

private:
    void make_name(const wxString& tablename);
    wxString name;
    wxString schema;
    wxString alias;
    char type=0; // pg_class.relkind
    wxString oid;
    //std::unordered_set<Cols,Cols::HashFunction> cols;
    std::vector<Cols> cols;
    
};
/// <summary>
/// Связка двух таблиц по колонкам
/// </summary>
class LinkTableFK
{
public:
    Table *parent;
    Table *child;
    std::vector<int> colsp;
    std::vector<int> colsc;
};
typedef struct tab_col_struct {
    Table* t; int column;
    bool operator< (const tab_col_struct& e) const
    {
        return  (t < e.t) || (t == e.t && column < e.column);
    }
    bool operator==(const tab_col_struct& otherPoint) const
    {
        if (t == otherPoint.t && column == otherPoint.column) return true;
        else return false;
    }

} tab_col_struct;
typedef struct tab_tab_struct {
    Table* t;
    Table* t2;
    bool operator< (const tab_tab_struct& e) const
    {
        return  (t < e.t) || (t == e.t && t2 < e.t2);
    }

} tab_tab_struct;
typedef struct fk_full_struct {
    tab_col_struct left;
    tab_col_struct right;
    bool operator< (const fk_full_struct& e) const
    {
        return  (left < e.left) || (left == e.left && right < e.right);
    }
} fk_full_struct;
class TableColsMap
{
public:
    enum Flag {
        ALL_LEFT_TO_RIGHT = 1,
        SEQUENCE_LIST_TABLE = 2,
        USE_TRANSIT_FK = 4,
        NOT_ADD_FIRST_SPACE=8
    };
#define CHKFLAG(va,par) (((va) & (par))==(par))

#ifdef DEBUG
    static void TEST() {
        Table v1("\"schema\".table");
        Table v2("\"schema\".\"table\"");
        Table v3("schema.table");
        Table v4("schema.\"table\"");
        Table v5("\"table\"");
        Table v6("table");
        Table e1("sc.table.t");
        Table e2("\"\".table");
        Table e3("\".\".table");
    }

#endif // DEBUG
    // Рекурсивное поисковое выражения для разбора внутреннего представления view 
    // Получаем ссылку на таблицу и колонку связанную со столбцом view.
    TableColsMap() { regaction.Compile(
        R"((?x)
\{TARGETENTRY.*?
(\{ ([^{}]++|(?1))* \})
\s+:resno\s(\d+)\s
:resname\s(\S+)\s.*?
:resorigtbl\s(\d+)\s.*?
:resorigcol\s(\d+)(\s:)
)",wxRE_EXTENDED);
    };
    void checkDBconn(pgConn *dbconn);
    /// <summary>
    /// Добавить таблицы для поиска ссылок.
    /// </summary>
    /// <param name="dbconn">Соединие к БД</param>
    /// <param name="tables">список таблиц</param>
    /// <param name="alias">список синонимов</param>
    /// <returns>true если успешно добавлены</returns>
    wxString AddTableList(pgConn* dbconn, const wxArrayString& tables, const wxArrayString& alias, const TableColsMap::Flag flag, const wxString& leftexp);
    void MapViewColToRelCol(const TableColsMap::Flag flag);
    Table* GetRelTableForViewCol(const wxString oid, int ncolview, int& outrelcol);
    Table* GetTablebyOID(const wxString oid);
    Table* GetTableByName(const wxString& sch, const wxString& tname);
    void BuildMapTableColumnsToSQLexp(Table* reltab, const wxString& alias, std::map<tab_col_struct, wxString>& map);
    wxString MapTableToTable(std::map<tab_col_struct, wxString>& maplefttables, std::map<tab_col_struct, wxString>& maprighttable, const TableColsMap::Flag flag, const wxString& leftexp);
    ~TableColsMap();
private:
    void getDatabaseViews();
    void getDatabaseTables(const TableColsMap::Flag flag);
    tab_col_struct search_link(tab_col_struct start, tab_col_struct chknode, std::set<fk_full_struct>& traz, int level);
    void Clear();
    pgConn* db = NULL;
    //std::map<Table *, std::vector<LinkTableFK>> tablechild_fk;
    std::multimap<tab_col_struct, tab_col_struct> all_fk_index; // {parent_t, n_col}={ftable, fcol}
    //std::map<Table *, wxString> alias;
    wxRegEx regaction;
    std::map<wxString, Table*> oids;
    wxString whereexp;
};

