/*
Copyright (C) 2015 Felipe Izzo
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include "errno.h"

#include <3ds.h>
#include <dirent.h>
#include "ctr.h"
#include "touch_ctr.h"

u8 isN3DS;

#define TICKS_PER_SEC 268123480.0

qboolean		isDedicated;

u64 initialTime = 0;
int hostInitialized = 0;

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int             findhandle (void)
{
	int             i;

	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int             pos;
	int             end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE    *f;
	int             i;

	i = findhandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;

	return filelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE    *f;
	int             i;

	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;

	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int     Sys_FileTime (char *path)
{
	FILE    *f;

	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}

	return -1;
}

void Sys_mkdir (char *path)
{
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}


void Sys_Error (char *error, ...)
{
	va_list         argptr;

	printf ("Sys_Error: ");
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");
	printf("Press START to exit");
	while(1){
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break;
	}
	Host_Shutdown();
	exit(1);
}

void Sys_Printf (char *fmt, ...)
{
	if(hostInitialized)
		return;

	va_list         argptr;

	va_start (argptr,fmt);
	vprintf (fmt,argptr);
	va_end (argptr);
}

void Sys_Quit (void)
{
	Host_Shutdown();
	gfxExit();
	exit (0);
}

double Sys_FloatTime (void)
{
	if(!initialTime){
		initialTime = svcGetSystemTick();
	}
	u64 curTime = svcGetSystemTick();
	return (curTime - initialTime)/TICKS_PER_SEC;
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
}

void CTR_SetKeys(u32 keys, u32 state){
	if( keys & KEY_SELECT)
		Key_Event(K_ESCAPE, state);
	if( keys & KEY_START)
		Key_Event(K_ENTER, state);
	if( keys & KEY_DUP)
		Key_Event(K_UPARROW, state);
	if( keys & KEY_DDOWN)
		Key_Event(K_DOWNARROW, state);
	if( keys & KEY_DLEFT)
		Key_Event(K_LEFTARROW, state);
	if( keys & KEY_DRIGHT)
		Key_Event(K_RIGHTARROW, state);
	if( keys & KEY_Y)
		Key_Event('y', state);
	if( keys & KEY_X)
		Key_Event('x', state);
	if( keys & KEY_B)
		Key_Event('b', state);
	if( keys & KEY_A)
		Key_Event('a', state);
	if( keys & KEY_L)
		Key_Event('l', state);
	if( keys & KEY_R)
		Key_Event('r', state);
	if( keys & KEY_ZL)
		Key_Event('k', state);
	if( keys & KEY_ZR)
		Key_Event('t', state);
}


void Sys_SendKeyEvents (void)
{
	hidScanInput();
	u32 kDown = hidKeysDown();
	u32 kUp = hidKeysUp();
	if(kDown)
		CTR_SetKeys(kDown, true);
	if(kUp)
		CTR_SetKeys(kUp, false);

	Touch_Update();
}

void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}

//=============================================================================

int main (int argc, char **argv)
{
	float		time, oldtime;

	APT_CheckNew3DS(&isN3DS);
	if(isN3DS)
		osSetSpeedupEnable(true);

	gfxInit(GSP_RGB565_OES,GSP_RGB565_OES,false);
	hidInit();
	gfxSetDoubleBuffering(GFX_TOP, false);
	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	gfxSet3D(false);
	consoleInit(GFX_BOTTOM, NULL);

	char *qargv[3];
	int   qargc = 1;

	qargv[0] = "";
	if(argc>1){
		if(strlen(argv[1]) != 0){
			qargv[1] = "-game";
			qargv[2] = argv[1];
			qargc += 2;
		}
	}

	static quakeparms_t    parms;

	parms.memsize = 24*1024*1024;
	parms.membase = malloc (parms.memsize);
	parms.basedir = ".";

	COM_InitArgv (qargc, qargv);

	parms.argc = com_argc;
	parms.argv = com_argv;
	Host_Init (&parms);
	hostInitialized = true;
	Touch_Init();
	Touch_DrawOverlay();
	//Sys_Init();
	oldtime = Sys_FloatTime() -0.1;
	while (aptMainLoop())
	{
		time = Sys_FloatTime();
		Host_Frame (time - oldtime);
		oldtime = time;
	}
	gfxExit();
	return 0;
}
