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

"D:\wxWidgets-2.8.12\utils\wxrc\vc_msw\wxrc" -c -o xrcDialogs.cpp *.xrc

