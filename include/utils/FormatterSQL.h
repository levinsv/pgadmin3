
#pragma once
#include <wx/wx.h>
namespace FSQL {
    /// <summary>
    ///  Formatter SQL
    /// </summary>
    enum type_item {
        unknown
        , spaces
        , keyword
        , literal
        , separation
        , bracket
        , identifier
        , name
        , numeric
        , bindarg
        , comment
    };
    struct view_item {
        type_item type = unknown;
        int srcpos = -1; // start position sql text -1 needed ignore
        int width = 0;   // 
        int height = 0;
        int endlevel = -1; // end bracet or home
        bool newline = false;
        int x = 0, y = 0, flags = 0;
        wxString txt;
    };
    enum f_key {
        none,
        new_line_align_no_pad = 1,
        new_line_align_next = 2,
        newLineComma = 4,
        newLineXPrevValue = 8,
        isCASE = 16,
        special = 32,
        newLineBracet = 64,
        HorizontalAlign = 128,
        isFUNCTION = 256,
        end_from = 256 << 1,
        limit = 256 << 2,
        fetch = 256 << 3,
        for_up = 256 << 4,
        fix_position_new_line = 256 << 5,
    };
    const struct KEYWORDEntity
    {
        const char* name;
        int len;            // == strlen(name)
        int flags;
    } keyEntities[] =
    {
        { "as",     2,  none },
        { "is",     2,  none },
        { "by",     2,  none },
        { "in",     2,  none },
        { "on",     2,  HorizontalAlign },
        { "of",     2,  none },
        { "or",     2,  newLineXPrevValue },
        { "and",    3,  newLineXPrevValue},
        { "not",    3,  none},
        { "asc",    3,  none},
        { "set",    3,  none},
        { "all",    3,  none},
        { "end",    3,  special},
        { "desc",   4,  none },
        { "like",   4,  none},
        { "null",   4,  none},
        { "from",   4,  new_line_align_no_pad },
        { "case",   4,  isCASE },
        { "when",   4,  new_line_align_no_pad },
        { "else",   4,  new_line_align_no_pad },
        { "with",   4,  special },
        { "join",   4,  new_line_align_next },
        { "left",   4,  new_line_align_next },
        { "full",   4,  new_line_align_next },
        { "then",  4,  none},
        { "into",  4,  none},
        { "last", 4,  none},
        { "next", 4,  none},
        { "only", 4,  none},
        { "cube", 4,  none},
        { "rows", 4,  none},
        { "sets", 4,  none},
        { "where",  5,  new_line_align_no_pad | end_from},
        { "outer",  5,  none},
        { "union",  5,  new_line_align_no_pad | end_from},
        { "order",  5,  new_line_align_no_pad | end_from},
        { "limit",  5,  new_line_align_no_pad | end_from},
        { "group",  5,  new_line_align_no_pad | end_from},
        { "count",  5,  none},
        { "using",  5,  none},
        { "first", 5,  none},
        { "inner", 5,  none},
        { "nulls", 5,  none},
        { "cross", 5,  none},

        { "select", 6,  newLineComma | new_line_align_no_pad},
        { "update",  6, newLineComma | new_line_align_no_pad},
        { "insert",  6, newLineComma | new_line_align_no_pad},
        { "values",  6, newLineComma | new_line_align_no_pad},
        { "window",  6, new_line_align_no_pad | end_from},
        { "having",  6, new_line_align_no_pad | end_from},
        { "except",  6, new_line_align_no_pad | end_from},
        { "offset", 6,  none | end_from},
        { "nothing", 7,  none},
        { "lateral", 7,  none},
        { "between", 7,  none},

        { "nothing", 7,  none},
        { "default", 7,  none},
        { "current", 7,  none},
        { "distinct", 8,  none},
        { "conflict", 8,  none},
        { "recursive", 9,  none},
        { "intersect", 9,  new_line_align_no_pad | end_from},
        { "returning", 9,  none},
        { "ordinality", 10,  none},
        { "materialized", 12,  none},
    };
    struct complite_element {
        wxString table;
        wxString alias;
        wxString columnList;
        int startIndex = -1;
        int endIndex = -1;
    };
    struct bits {
        unsigned int from : 1;
        unsigned int select_list : 1;
        unsigned int with : 1;
        unsigned int skip : 1;
    };
    union Byte {
        unsigned char byte;
        struct bits b;
    };

    class FormatterSQL {
    public:
        FormatterSQL(const wxString& sqlsrc) {
            sql = sqlsrc;
        }
        void Formating(wxDC& d, wxRect re, bool isTest = false); // draw 
        wxString Formating(wxRect re);                          // 
        // 
        wxString BuildAutoComplite(int startIndex, int level);
        wxString GetListTable(int cursorPos);
        wxString GetColsList(wxString what, wxString& listfieldOut, wxString& nameTableOut);
        //
        int ParseSql(int flags);
        wxString printParseArray();
        void SetSql(const wxString& sqlsrc) { sql = sqlsrc; }
        int  GetIndexItemNextSqlPosition(int sqlPosition);
        bool GetItem(int index, FSQL::view_item& item);
    private:
        wxString get_list_columns(int startindex, union FSQL::Byte zone);

        wxPoint align_level(int start_i, int level, int Xpos, int Ypos, int flag);
        int check_bracket(int index);
        int get_prev_value(int indx, wxString keyword);
        int next_item_no_space(int& index, int direction = 1);
        wxSize max_width_size(int index); // index bracet
        wxSize best_sizeAndDraw(wxDC& dc, wxPoint& pos, FSQL::view_item& vi, int mode);
        int maxYheightLine = 0;
        int max_exp_bracet_width = 200;
        int pad = 16;
        int casepad = 8;
        wxPoint neededNewLine; // добавляет новую строку перед первым встреченным не пробельным символом
        wxRect rect;
        wxString sql;
        std::vector<FSQL::view_item> items;
        std::vector<FSQL::complite_element> listTable; // перечень таблиц синонимов, подзапросов и функций с колонками
        //int recurse(int level);
    };
}