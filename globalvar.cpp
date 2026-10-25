#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include "utils/misc.h"
#include "utils/sysSettings.h"
#include "schema/pgServer.h"
#include "frm/frmMain.h"
#include "dlg/dlgSelectConnection.h"
#include "db/pgConn.h"
#include "utils/sysLogger.h"
#include "utils/registry.h"
#include <wx/stdpaths.h>

// Globals
frmMain *winMain = 0;
wxThread *updateThread = 0;
bool iswayland=false;
bool isdark=false;
wxBrush selectFindBrush;

sysSettings *settings;
wxArrayInt existingLangs;
wxArrayString existingLangNames;
wxLocale *locale = 0;
pgAppearanceFactory *appearanceFactory = 0;

wxString pgBackupExecutable;       // complete filename of PostgreSQL's pg_dump, pg_dumpall and pg_restore, if available
wxString pgBackupAllExecutable;
wxString pgRestoreExecutable;

wxString edbBackupExecutable;      // complete filename of EnterpriseDB's pg_dump, pg_dumpall and pg_restore, if available
wxString edbBackupAllExecutable;
wxString edbRestoreExecutable;

wxString gpBackupExecutable;      // complete filename of Greenplum's pg_dump, pg_dumpall and pg_restore, if available
wxString gpBackupAllExecutable;
wxString gpRestoreExecutable;

wxString loadPath;              // Where the program is loaded from
wxString dataDir;               // The program data directory
wxString docPath;               // Where docs are stored
wxString uiPath;                // Where ui data is stored
wxString i18nPath;              // Where i18n data is stored
wxString brandingPath;          // Where branding data is stored
wxString pluginsDir;            // The plugins ini file directory
wxString svgPath;              //  Where svg icons data is stored
wxString defPath;              //  Where default main data is stored
wxString settingsIni;           // The settings.ini file


wxLog *logger;

#include "images/pgAdmin3-16.pngc"
#include "images/pgAdmin3-32.pngc"
#include "images/splash.pngc"

pgAppearanceFactory::pgAppearanceFactory()
{
	is_branded = false;

	// Setup the default branding options
#ifdef __WIN32__
	splash_font_size = 8;
#else
#ifdef __WXMAC__
	splash_font_size = 11;
#else
	splash_font_size = 9;
#endif
#endif

	splash_pos_x = 128;
	splash_pos_y = 281;
	splash_pos_offset = 15;

	large_icon = *pgAdmin3_32_png_img;
	small_icon = *pgAdmin3_16_png_img;
	splash_image = *splash_png_img;

	splash_text_colour = wxColour(255, 255, 255);
	report_key_colour = wxColour(0, 154, 206);

	long_appname = wxT("pgAdmin III");
	short_appname = wxT("pgadmin3");
	website_url = wxT("http://www.pgadmin.org/");

	hide_enterprisedb_help = false;
	hide_greenplum_help = false;

	// Attempt to overload branding information
	wxFileName brIni(brandingPath + wxT("/branding.ini"));
	if (brIni.FileExists())
	{
		wxString brCfg = FileRead(brIni.GetFullPath());

		wxStringTokenizer tkz(brCfg, wxT("\r\n"));

		while(tkz.HasMoreTokens())
		{
			wxString token = tkz.GetNextToken();

			if (token.Trim() == wxEmptyString || token.StartsWith(wxT(";")))
				continue;

			if (token.Lower().StartsWith(wxT("largeicon=")))
			{
				large_icon = wxImage(brandingPath + wxT("/") + token.AfterFirst('=').Trim());
				if (!large_icon.IsOk())
					large_icon = *pgAdmin3_32_png_img;
				else
					is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("smallicon=")))
			{
				small_icon = wxImage(brandingPath + wxT("/") + token.AfterFirst('=').Trim());
				if (!small_icon.IsOk())
					small_icon = *pgAdmin3_16_png_img;
				else
					is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("splashimage=")))
			{
				splash_image = wxImage(brandingPath + wxT("/") + token.AfterFirst('=').Trim());
				if (!splash_image.IsOk())
					splash_image = *splash_png_img;
				else
					is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("icon=")))
			{
				icon = token.AfterFirst('=').Trim();
				is_branded = true;
			}
#ifdef __WIN32__
			else if (token.Lower().StartsWith(wxT("splashfontsizewin=")))
#else
#ifdef __WXMAC__
			else if (token.Lower().StartsWith(wxT("splashfontsizemac=")))
#else
			else if (token.Lower().StartsWith(wxT("splashfontsizegtk=")))
#endif
#endif
			{
				token.AfterFirst('=').Trim().ToLong(&splash_font_size);
				is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("splashposx=")))
			{
				token.AfterFirst('=').Trim().ToLong(&splash_pos_x);
				is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("splashposy=")))
			{
				token.AfterFirst('=').Trim().ToLong(&splash_pos_y);
				is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("splashposoffset=")))
			{
				token.AfterFirst('=').Trim().ToLong(&splash_pos_offset);
				is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("splashtextcolour=")))
			{
				splash_text_colour = wxColor(token.AfterFirst('=').Trim());
				is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("shortappname=")))
			{
				short_appname = token.AfterFirst('=').Trim();
				is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("longappname=")))
			{
				long_appname = token.AfterFirst('=').Trim();
				is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("websiteurl=")))
			{
				website_url = token.AfterFirst('=').Trim();
				is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("reportkeycolour=")))
			{
				report_key_colour = wxColor(token.AfterFirst('=').Trim());
				is_branded = true;
			}
			else if (token.Lower().StartsWith(wxT("hideenterprisedbhelp=")))
			{
				if (token.AfterFirst('=').Trim() == wxT("1"))
				{
					hide_enterprisedb_help = true;
					is_branded = true;
				}
			}
			else if (token.Lower().StartsWith(wxT("hidegreenplumhelp=")))
			{
				if (token.AfterFirst('=').Trim() == wxT("1"))
				{
					hide_greenplum_help = true;
					is_branded = true;
				}
			}
		}
	}

#ifdef __WXMSW__

	// Set the MUI cache value for the grouped task bar title,
	// otherwise we get the value from the resources which is
	// definitely not what we want in branded mode!
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\ShellNoRoam\\MUICache"));
	if(!pRegKey->Exists())
		pRegKey->Create();

#if wxCHECK_VERSION(2, 9, 5)
	wxStandardPaths &paths = wxStandardPaths::Get();
#else
	wxStandardPaths paths;
#endif
	//wxString tmp;
	//tmp.Printf(wxT("%s"), long_appname);
	pRegKey->SetValue(paths.GetExecutablePath(), GetLongAppName());
	delete pRegKey;

	// Reset the image for the task bar group. This can only by
	// set per-exe name unfortunately. If we don't find an icon,
	// remove the registry value.
	wxString icon_path = brandingPath + wxT("\\") + icon;


	pRegKey = new wxRegKey(wxT("HKEY_CURRENT_USER\\Software\\Classes\\Applications\\") + wxFileName(paths.GetExecutablePath()).GetFullName());
	if(!pRegKey->Exists())
		pRegKey->Create();

	if (wxFile::Exists(icon_path))
	{
		pRegKey->SetValue(wxT("TaskbarGroupIcon"), icon_path);
	}
	else
	{
		if (pRegKey->HasValue(wxT("TaskbarGroupIcon")))
			pRegKey->DeleteValue(wxT("TaskbarGroupIcon"));
	}
	delete pRegKey;
#endif
}

void pgAppearanceFactory::SetIcons(wxDialog *dlg)
{
	wxIconBundle icons;
	icons.AddIcon(GetSmallIconImage());
	icons.AddIcon(GetBigIconImage());
	dlg->SetIcons(icons);
}

void pgAppearanceFactory::SetIcons(wxTopLevelWindow *dlg)
{
	wxIconBundle icons;
	icons.AddIcon(GetSmallIconImage());
	icons.AddIcon(GetBigIconImage());
	dlg->SetIcons(icons);
}

wxIcon pgAppearanceFactory::GetSmallIconImage()
{
	wxIcon icon;
	icon.CopyFromBitmap(wxBitmap(small_icon));
	return icon;
}

wxIcon pgAppearanceFactory::GetBigIconImage()
{
	wxIcon icon;
	icon.CopyFromBitmap(wxBitmap(large_icon));
	return icon;
}

wxFont pgAppearanceFactory::GetSplashTextFont()
{
	wxFont fnt(*wxNORMAL_FONT);
	fnt.SetPointSize(splash_font_size);
	return fnt;
}


