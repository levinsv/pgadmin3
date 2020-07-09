@echo off
REM #######################################################################
REM #
REM # pgAdmin III - PostgreSQL Tools
REM # Copyright (C) 2002 - 2016, The pgAdmin Development Team
REM # This software is released under the PostgreSQL Licence
REM #
REM # embed-xrc.bat - convert xrc files to c++ files
REM #
REM #######################################################################

"D:\PostgreSQL\pgadmin3\pgadmin\Debug_(3.0)\wxrc.exe" -c -o xrcDialogs.cpp *.xrc

