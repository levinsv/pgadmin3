#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include "utils/FormatterSQL.h"

using namespace FSQL;

TEST_CASE( "FormatterSQL build autocomplite", "[fmtsql]" ) {
			wxString s = "select a.* from a aa,b";
			FormatterSQL f(s);
			wxString act1;
			wxString act,cl,t,exp;
SECTION( "Простой запрос" ) {
			int e = f.ParseSql(0);	
			act1=f.BuildAutoComplite(0, 0);
			wxString act2 = f.GetListTable(0);
			exp = "[ a,aa] \n[ b,] \n";
			CHECK(act2==exp);
}
SECTION( "GetColsList cols" ) {
			int e = f.ParseSql(0);
			act1=f.BuildAutoComplite(0, 0);
			act1 = f.GetColsList("aa.f1",cl, t);
			CHECK(act1.ToStdWstring().length()==0  );
}
SECTION( "GetColsList table" ) {
			int e = f.ParseSql(0);
	        act1=f.BuildAutoComplite(0, 0);
            act1 = f.GetColsList("aa.f1", cl, t);
			CHECK( t=="a");
}
SECTION( "GetColsList cols 2" ) {
			int e = f.ParseSql(0);
	        act1=f.BuildAutoComplite(0, 0);
			act1 = f.GetColsList("b.*", cl, t);
			CHECK(act1.ToStdWstring().length()==0);
}
SECTION( "GetColsList table 2" ) {
			int e = f.ParseSql(0);
	        act1 = f.BuildAutoComplite(0, 0);
            act1 = f.GetColsList("b.*", cl, t);
			CHECK(t=="b");
}
SECTION( "Функция 1" ) {
			int e;
			f.SetSql("select 2 from f() a");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ @,a] \n";
			CHECK(act==exp);
}
SECTION( "func2" ) {
			int e;
			FSQL::FormatterSQL f3(L"select now() from f() a(f1,\"ф2\")");
			wxString act3;
			e = f3.ParseSql(0);
			CHECK(e==0);	
			act = f3.BuildAutoComplite(0, 0); act = f3.GetListTable(0);
			exp=R"([ @,a] f1,"ф2"
			)";
			CHECK(act3==exp);
			act3 = f3.GetListTable(1);
			//act3=f3.printParseArray();
			//act3=wxString::Format(" len: %d",act3.Len());
			exp = "[ now,] \n[ f,] \n[ a,] \n";
			//std::cerr << act3;
			CHECK(act3==exp);
}
SECTION( "Функция 2a" ) {
			int e;
			f.SetSql("select 2 from f(p1,(1+2)) a(f1 int,\"ф2\" text)");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ @,a] f1,\"ф2\"\n";
			CHECK(act==exp);
}
SECTION( "Функция 2b" ) {
			int e;
			f.SetSql("select 2 from f(p1,(1+2)) a(f1 int,\"ф2\" text), t2");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ @,a] f1,\"ф2\"\n[ t2,] \n";
			CHECK(act==exp);
}
SECTION( "Подзапрос 1" ) {
			int e;
			f.SetSql("select 2 from (select it.* from it) t1, t2");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ it,] \n[ @,t1] it.*\n[ t2,] \n";
			CHECK(act==exp);
}
SECTION( "Подзапрос 2" ) {
			int e;
			f.SetSql("select 2 from (select it.* from it) t1(f1,f2), t2 al(f3,f4)");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ it,] \n[ @,t1] f1,f2\n[ t2,al] f3,f4\n";
			CHECK(act==exp);
			act = f.GetListTable(1);
			exp = "[ t1,] \n[ al,] \n";
			CHECK(act==exp);

}
SECTION( "Подзапрос 3" ) {
			int e;
			f.SetSql("select 2 from (select it.dt::time at zone Ndt,it.col2 from it) t1, t2 al(f3,f4)");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ it,] \n[ @,t1] Ndt,it.col2\n[ t2,al] f3,f4\n";
			CHECK(act==exp);
}
SECTION( "GetColsList cols 3" ) {			
			int e;
			f.SetSql("select 2 from (select it.dt::time at zone Ndt,it.col2 from it) t1, t2 al(f3,f4)");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ it,] \n[ @,t1] Ndt,it.col2\n[ t2,al] f3,f4\n";
			act = f.GetColsList("t1.n", cl, t);
			exp = "Ndt";
			CHECK(act==exp);
}
SECTION( "GetColsList table" ) {
			int e;
			f.SetSql("select 2 from (select it.dt::time at zone Ndt,it.col2 from it) t1, t2 al(f3,f4)");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			act1 = f.GetColsList("t1.n", cl, t);
			exp = "t1";
			CHECK(t==exp);
}
SECTION( "GetColsList cols 4" ) {			
			int e;
			f.SetSql("select 2 from (select it.dt::time at zone Ndt,it.col2 from it) t1, t2 al(f3,f4)");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ it,] \n[ @,t1] Ndt,it.col2\n[ t2,al] f3,f4\n";
			CHECK(act==exp);
			act1 = f.GetColsList("t1.*", cl, t);
			exp = "Ndt\tcol2";
			CHECK(act1==exp);
}
SECTION( "Join 1" ) {
			int e;
			// join
			f.SetSql("select * from def join param on def.id=param.def join inv_type c on c.id=param.type limit 5");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ def,] \n[ param,] \n[ inv_type,c] \n";
			CHECK(act==exp);
}
SECTION( "Join 1.1" ) {
			int e;
			f.SetSql("select * from def d join param p on d.id=p.def join inv_type c on c.id=p.type where");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = L"[ def,d] \n[ param,p] \n[ inv_type,c] \n";
			CHECK(act==exp);
}
SECTION( "Join 2" ) {
			int e;
			f.SetSql("select def.*,param.* from def join ( select f2,f3 from a) param on def.id=param.def join inv_type c on c.id=param.type");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ def,] \n[ a,] \n[ @,param] f2,f3\n[ inv_type,c] \n";
			CHECK(act==exp);
}
SECTION( "with 1" ) {			
			int e;
			// with
			f.SetSql("with a as (select in1,i2 from def), b(b1,b2) as (select t3 from z2) select * from b");
			e = f.ParseSql(0);
			act = f.BuildAutoComplite(0, 0); act = f.GetListTable(0);
			exp = "[ def,] \n[ @,a] in1,i2\n[ z2,] \n[ @,b] b1,b2\n[ b,] \n";
			CHECK(act==exp);
}
}

TEST_CASE( "FormatterSQL parse", "[parsesql]" ) {
	int e;
	FormatterSQL f2("()");
	wxString exp,o,s;
	SECTION( "simple 1" ) {
			wxString s = "select * from table t1,table t2 where t1.f=t2.f order by 1 desc limit 2";
			FormatterSQL f(s);
			int e=f.ParseSql(0);
			CHECK(e==0);
	}
	SECTION( "empty braket" ) {
			e = f2.ParseSql(0);
			CHECK(e==0);
			o = f2.printParseArray();
			exp = "Index: 0 Jump 1 Type: 5 widt: 1 Flag: 0 Val : (\nIndex: 1 Jump 0 Type: 5 widt: 1 Flag: 0 Val : )\n";
			CHECK(o==exp);
	}
	SECTION( "variant1" ) {
			f2.SetSql("a.* 0:1 b::t (m).c 2~!@6");	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 7 widt: 1 Flag: 0 Val : a\nIndex: 1 Type: 4 widt: 2 Flag: 0 Val : .*\nIndex: 2 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 3 Type: 8 widt: 1 Flag: 0 Val : 0\nIndex: 4 Type: 4 widt: 1 Flag: 0 Val : :\nIndex: 5 Type: 8 widt: 1 Flag: 0 Val : 1\nIndex: 6 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 7 Type: 7 widt: 1 Flag: 0 Val : b\nIndex: 8 Type: 4 widt: 2 Flag: 0 Val : ::\nIndex: 9 Type: 7 widt: 1 Flag: 256 Val : t\nIndex: 10 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 11 Jump 13 Type: 5 widt: 1 Flag: 0 Val : (\nIndex: 12 Type: 7 widt: 1 Flag: 0 Val : m\nIndex: 13 Jump 11 Type: 5 widt: 1 Flag: 0 Val : )\nIndex: 14 Type: 4 widt: 1 Flag: 0 Val : .\nIndex: 15 Type: 7 widt: 1 Flag: 0 Val : c\nIndex: 16 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 17 Type: 8 widt: 1 Flag: 0 Val : 2\nIndex: 18 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 19 Type: 4 widt: 3 Flag: 0 Val : ~!@\nIndex: 20 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 21 Type: 8 widt: 1 Flag: 0 Val : 6\n";
			CHECK(o==exp);
	}

	SECTION( "many variantns" ) {
			f2.SetSql( "--1" );	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 10 widt: 3 Flag: 0 Val : --1\n";
			CHECK(o==exp);

			f2.SetSql("$$ fsdf ");	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 3 widt: 8 Flag: 0 Val : $$ fsdf \n";
			CHECK(o==exp);

			f2.SetSql("E' ");	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "";
			CHECK(o==exp);

			f2.SetSql("/*");	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 10 widt: 2 Flag: 0 Val : /*\n";
			CHECK(o==exp);
			
			s = "a.b"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 6 widt: 3 Flag: 0 Val : a.b\n";
			CHECK(o==exp);
			s = "a::b"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 7 widt: 1 Flag: 0 Val : a\nIndex: 1 Type: 4 widt: 2 Flag: 0 Val : ::\nIndex: 2 Type: 7 widt: 1 Flag: 0 Val : b\n";
			CHECK(o==exp);
			s = "is not null"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 2 widt: 11 Flag: 0 Val : is not null\n";
			CHECK(o==exp);
			s = "*,+:%@!~"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 4 widt: 1 Flag: 0 Val : *\nIndex: 1 Type: 4 widt: 1 Flag: 0 Val : ,\nIndex: 2 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 3 Type: 4 widt: 6 Flag: 0 Val : +:%@!~\n";
			CHECK(o==exp);

			s = "1||2"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 8 widt: 1 Flag: 0 Val : 1\nIndex: 1 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 2 Type: 4 widt: 2 Flag: 0 Val : ||\nIndex: 3 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 4 Type: 8 widt: 1 Flag: 0 Val : 2\n";
			CHECK(o==exp);

			s = "1+2"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 8 widt: 1 Flag: 0 Val : 1\nIndex: 1 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 2 Type: 4 widt: 1 Flag: 0 Val : +\nIndex: 3 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 4 Type: 8 widt: 1 Flag: 0 Val : 2\n";
			CHECK(o==exp);
			s = "e'\\n'"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 3 widt: 5 Flag: 0 Val : e'\\n'\n";
			CHECK(o==exp);

			s = "u&'d\\0061t\\+000061'"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 3 widt: 19 Flag: 0 Val : u&'d\\0061t\\+000061'\n";
			CHECK(o==exp);
			s = "$$ $$"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 3 widt: 5 Flag: 0 Val : $$ $$\n";
			CHECK(o==exp);
			s = "$a$ $ $a$"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 3 widt: 9 Flag: 0 Val : $a$ $ $a$\n";
			CHECK(o==exp);
			s = "$0"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "\n";
			//Assert::AreEqual(exp.ToStdWstring(), o.ToStdWstring(), s);
			s = "f(a,b)"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 7 widt: 1 Flag: 256 Val : f\nIndex: 1 Jump 6 Type: 5 widt: 1 Flag: 0 Val : (\nIndex: 2 Type: 7 widt: 1 Flag: 0 Val : a\nIndex: 3 Type: 4 widt: 1 Flag: 0 Val : ,\nIndex: 4 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 5 Type: 7 widt: 1 Flag: 0 Val : b\nIndex: 6 Jump 1 Type: 5 widt: 1 Flag: 0 Val : )\n";
			//Assert::AreEqual(exp.ToStdWstring(), o.ToStdWstring(), s);
			s = ")"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "\n";
			CHECK(e==-1);
			s = "("; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			CHECK(e==-1);
			exp = "\n";
			//Assert::AreEqual(exp.ToStdWstring(), o.ToStdWstring(), s);
			s = "like"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 2 widt: 4 Flag: 0 Val : like\n";
			CHECK(o==exp);

			s = "("; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "\n";
			//Assert::AreEqual(exp.ToStdWstring(), o.ToStdWstring(), s);
			
			s = "x=ANY([1,2,3])"; f2.SetSql(s);	e = f2.ParseSql(0);	o = f2.printParseArray();
			exp = "Index: 0 Type: 7 widt: 1 Flag: 0 Val : x\nIndex: 1 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 2 Type: 4 widt: 1 Flag: 0 Val : =\nIndex: 3 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 4 Type: 7 widt: 3 Flag: 256 Val : ANY\nIndex: 5 Jump 15 Type: 5 widt: 1 Flag: 0 Val : (\nIndex: 6 Jump 14 Type: 5 widt: 1 Flag: 0 Val : [\nIndex: 7 Type: 8 widt: 1 Flag: 0 Val : 1\nIndex: 8 Type: 4 widt: 1 Flag: 0 Val : ,\nIndex: 9 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 10 Type: 8 widt: 1 Flag: 0 Val : 2\nIndex: 11 Type: 4 widt: 1 Flag: 0 Val : ,\nIndex: 12 Type: 1 widt: 1 Flag: 0 Val : \nIndex: 13 Type: 8 widt: 1 Flag: 0 Val : 3\nIndex: 14 Jump 6 Type: 5 widt: 1 Flag: 0 Val : ]\nIndex: 15 Jump 5 Type: 5 widt: 1 Flag: 0 Val : )\n";
			CHECK(o==exp);
	}
}
TEST_CASE( "FormatterSQL parsePlpgsql", "[parsesql]" ) {
	int e;
	wxString exp,o,s;
	SECTION( "variant1" ) {
		FormatterSQL f2(R"(
			declare
			begin
			end
		 )");
		 std::vector<complite_element> list = f2.ParsePLpgsql(); o=f2.GetListTable(list);
		 exp = "";
		 CHECK(o==exp);
	}		 
SECTION( "plpgsql 1" ) {		 
		 FormatterSQL f2(R"(
			declare
			 x record;
			 iobj varchar(50) :=add_part(now(),'1 month');
			begin
			for i in 11..22
				loop
				end loop;
			end
		 )");
		 std::vector<complite_element> list = f2.ParsePLpgsql(); o=f2.GetListTable(list);
 		 exp = "[ add_part,] \n[ now,] \n";
		 CHECK(o==exp);
}
SECTION( "plpgsql 2" ) {		 
		 FormatterSQL f2(R"(
			declare
			 x record;
			begin
			for i in 11..22
				loop
				  case f1() when f2() else
select a into ii from tab1 t where f3() and f4() in ('a');
case end;
delete from t2 using t4,t5 where t4.id=t5 and t2.id=t4.id returning t2.id;
				end loop;
			end
		 )");
		 std::vector<complite_element> list = f2.ParsePLpgsql(); o=f2.GetListTable(list);
 		 exp = "[ f1,] \n[ f2,] \n[ tab1,] \n[ f3,] \n[ f4,] \n[ t2,] \n[ t5,] \n";
 	 	CHECK(o==exp);
}

}

