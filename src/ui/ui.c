/**
* ui.c -- ͼ�ν������ģ��
* ��Ȩ���� (C) 2016 ������ ���� <root@lc-soft.io>
*/

#include <LCUI_Build.h>
#include <LCUI/LCUI.h>
#include <LCUI/display.h>
#include <LCUI/font/charset.h>
#include <LCUI/gui/builder.h>
#include <stdlib.h>
#include "ui.h"
#include "finder.h"

#define XML_PATH "res/ui.xml"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>

static void InitConsoleWindow( void )
{
	int hCrt;
	FILE *hf;
	AllocConsole();
	hCrt = _open_osfhandle( (long)GetStdHandle( STD_OUTPUT_HANDLE ), _O_TEXT );
	hf = _fdopen( hCrt, "w" );
	*stdout = *hf;
	setvbuf( stdout, NULL, _IONBF, 0 );
	printf( "InitConsoleWindow OK!\n" );
}

#endif

static void onTimer( void *arg )
{
	Widget_PrintTree( NULL );
}

void UI_Init(void)
{
	LCUI_Widget box;
#ifdef LCUI_BUILD_IN_WIN32
	InitConsoleWindow();
#endif
	LCUI_Init();
	LCUIDisplay_SetMode( LDM_WINDOWED );
	LCUIDisplay_SetSize( 960, 540 );
	//LCUIDisplay_ShowRectBorder();
	box = LCUIBuilder_LoadFile( XML_PATH );
	if( box ) {
		Widget_Top( box );
		Widget_Unwrap( &box );
	}
	Widget_UpdateStyle( LCUIWidget_GetRoot(), TRUE );
	UI_InitSidebar();
	UI_InitSettingsView();
	UI_InitFoldersView();
	LCUITimer_Set( 2000, onTimer, NULL, FALSE );
}

int UI_Run(void)
{
	return LCUI_Main();
}