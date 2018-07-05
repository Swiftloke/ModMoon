#include "main.hpp"
#include "titleselects.hpp"
#include "utils.hpp"
#include "initialsetup.hpp"
#include "error.hpp"
#include "toolsmenu.hpp"
/*Ah. It's great to finally see this in action. This button hasn't had a true implementation
since the beginning of the project, WAY back in September 2017.
There are five options to work with, they should just call various parts of the project.
-Smash Controls Modifier
-Active Title Selection
-Tutorial
-Mods migrator
-Dark Mode toggle (this may later be expanded into a full settings menu)
*/


//Unlike the main menu shift in logic, which was developed at the very beginning of the project,
//this logic uses sDraw's significant developments since then. Instead of being weird and playing
//with pixel positions, we'll just use the vertex interpolation functionality.
//Final x position should be 18.
//Y values- 6, 49, 93, 138, 184 (a 43-45 pixel change)
//Coming in from the left, -290 (the width of the button + 1)
//Coming in from the right, 321 (screen width + 1)

bool toolsmenushift(float& interpfactor, bool plus)
{
	if (plus)
	{
		interpfactor += 0.05;
		if (interpfactor > 1)
		{
			interpfactor = 1;
			return true;
		}
	}
	else
	{
		interpfactor -= 0.05;
		if (interpfactor < 0)
		{
			interpfactor = 0;
			return true;
		}
	}
	return false;
}

void toolsmenudraw(float interpfactor, int position, int highlighteralpha, bool shouldblink)
{
	const int initialxvals[] = { -290, 401, -290, 401, -290 };
	const int toolsyvals[] = { 6, 49, 93, 138, 184 };
	draw.framestart();
	if (shouldblink)
	{
		draw.drawrectangle(0, 0, 400, 240, RGBA8(255, 255, 255, 255));
		draw.drawon(GFX_BOTTOM);
		draw.drawrectangle(0, 0, 320, 240, RGBA8(255, 255, 255, 255));
	}
	else
	{
		drawtopscreen();
		draw.drawon(GFX_BOTTOM);
		draw.drawtexture(backgroundbot, 0, 0);
		draw.drawhighlighter(toolsmenuhighlighter, initialxvals[position] - 13, toolsyvals[position] - 7,\
			highlighteralpha, 18 - 13, toolsyvals[position] - 7, interpfactor);
		draw.drawtexture(controlsmodifierbutton, initialxvals[0], toolsyvals[0], 18, toolsyvals[0], interpfactor);
		draw.drawtexture(controlsmodifierbutton, initialxvals[1], toolsyvals[1], 18, toolsyvals[1], interpfactor);
		draw.drawtexture(controlsmodifierbutton, initialxvals[2], toolsyvals[2], 18, toolsyvals[2], interpfactor);
		draw.drawtexture(controlsmodifierbutton, initialxvals[3], toolsyvals[3], 18, toolsyvals[3], interpfactor);
		draw.drawtexture(controlsmodifierbutton, initialxvals[4], toolsyvals[4], 18, toolsyvals[4], interpfactor);
	}
	draw.frameend();
}

void toolsmenushiftout()
{
	float shift = 1;
	while (shift > 0)
	{
		toolsmenushift(shift, false);
		toolsmenudraw(shift, 0, 0, false);
	}
}

void toolsmenushiftin()
{
	float shift = 0;
	while (shift < 1)
	{
		toolsmenushift(shift, true);
		toolsmenudraw(shift, 0, 0, false);
	}
}

//Temporary forward declaration
void controlsmodifier();

void toolsmenu()
{
	float shift = 0;
	bool shiftin = true;
	static int position = 0;
	int highlighteralpha = 0;
	bool highlighterplus = true;
	touchPosition tpos, opos;
	bool shouldblink = false; //Toggling dark mode needs this
	while (aptMainLoop())
	{
		toolsmenushift(shift, shiftin);
		highlighterhandle(highlighteralpha, highlighterplus);
		toolsmenudraw(shift, position, highlighteralpha, shouldblink);
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		hidTouchRead(&tpos);
		if(kDown || kHeld)
			highlighteralpha = 255;
		if (kDown & KEY_A)
		{
			switch (position)
			{
				case 0:
				{
					activetitleselect();
					break;
				}
				case 1: 
				{
					toolsmenushiftout();
					controlsmodifier();
					toolsmenushiftin();
					break;
				}
				case 2:
				{
					toolsmenushiftout();
					tutorial(false);
					toolsmenushiftin();
					break;
				}
				case 3:
				{
					bool migrationwasdone = doallmigration();
					if (!migrationwasdone)
					{
						error("No mods needing to be\nmigrated were found.");
					}
					break;
				}
				case 4: 
				{
					shouldblink = true;
					while (kHeld & KEY_A)
					{
						hidScanInput();
						kHeld = hidKeysHeld();
						toolsmenudraw(shift, position, highlighteralpha, shouldblink);
					}
					shouldblink = false;
					toggledarkmode();
					break;
				}
			}
		}
		if(kDown & KEY_UP)
		{
			position--;
			if(position < 0)
				position = 4;
		}
		if (kDown & KEY_DOWN)
		{
			position++;
			if(position > 4)
				position = 0;
		}
		if (kDown & KEY_B)
		{
			shiftin = false;
			while (shift > 0)
			{
				toolsmenushift(shift, shiftin);
				highlighterhandle(highlighteralpha, highlighterplus);
				toolsmenudraw(shift, position, highlighteralpha, false);
			}
			break;
		}
		//Get the highlighter in position
		if(touched(controlsmodifierbutton, 18, 6, tpos))
			position = 0;
		//Actually activate it
		else if (buttonpressed(controlsmodifierbutton, 18, 6, opos, kHeld))
		{
			activetitleselect();
		}

		if (touched(controlsmodifierbutton, 18, 49, tpos))
			position = 1;
		else if (buttonpressed(controlsmodifierbutton, 18, 49, opos, kHeld))
		{
			toolsmenushiftout();
			controlsmodifier();
			toolsmenushiftin();
		}

		if (touched(controlsmodifierbutton, 18, 93, tpos))
			position = 2;
		else if (buttonpressed(controlsmodifierbutton, 18, 93, opos, kHeld))
		{
			toolsmenushiftout();
			tutorial(false);
			toolsmenushiftin();
		}

		if (touched(controlsmodifierbutton, 18, 138, tpos))
			position = 3;
		else if (buttonpressed(controlsmodifierbutton, 18, 138, opos, kHeld))
		{
			bool migrationwasdone = doallmigration();
			if (!migrationwasdone)
			{
				error("No mods needing to be\nmigrated were found.");
			}
		}
		
		if (touched(controlsmodifierbutton, 18, 184, tpos))
			shouldblink = true;
		else if (buttonpressed(controlsmodifierbutton, 18, 184, opos, kHeld))
			toggledarkmode();
		else
			shouldblink = false;

		opos = tpos;
	}
}

void toggledarkmode()
{
	bool currentdarkmodestatus = draw.darkmodeshouldactivate;
	draw.darkmodeshouldactivate = !currentdarkmodestatus;
	config.write("DarkModeEnabled", !currentdarkmodestatus);
}