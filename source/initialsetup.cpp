#include "main.hpp"
#include "error.hpp"
#include "utils.hpp"
#include "migrators/migrators.hpp"
#include "download.hpp"
#include "titleselects.hpp"

string progresstext;
unsigned int progress;
unsigned int total;
float expandpos;
C3D_Tex dummy; //We need a texture for the framebuffer, but we're against a black background.
bool progressrunning = true;
bool doprogressdraw = true;
Handle progressthreaddone;

void threadfunc_drawinitialsetupprogress(void* args)
{
	//insert the progress and total into the string?
	while (progressrunning)
	{
		if(!doprogressdraw)
			continue;
		drawprogresserror(progresstext, expandpos, (float)progress / total, dummy, dummy);
	}
	svcSignalEvent(progressthreaddone);
}

void progresspopup()
{
	while (expandpos < 1) //Let it stay at max once we're done
	{
		expandpos += .06f;
		if (expandpos >= 1) expandpos = 1; //Prevent it from going overboard
	}
}

void progresspopdown()
{
	while (expandpos > 0)
	{
		expandpos -= .06f;
		if (expandpos <= 1) expandpos = 0;
	}
}

void initialsetup()
{
	error("Welcome! Let's get you set up.");
	C3D_TexInit(&dummy, 32, 32, GPU_RGBA8);
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	svcCreateEvent(&progressthreaddone, RESET_ONESHOT);
	threadCreate(threadfunc_drawinitialsetupprogress, NULL, 20000, mainthreadpriority + 1, -2, true);
	if (pathExist("/saltysd/smash/card.txt"))
	{
		progresstext = "Moving Smash Selector 1.0 mods...";
		progress = 1; //It'll move too fast to be worth the animation
		total = 1;
		progresspopup();
		ss1xMigrate();
		progresspopdown();
	}
	if (pathExist("/3ds/data/smash_selector/settings.txt"))
	{
		progresstext = "Moving Smash Selector 2.x mods...";
		progress = 1; //Ditto. If I had some smoothing code for the progress bar it would be fine, but I don't.
		total = 1;
		progresspopup();
		//ss2xMigrate();
		progresspopdown();
	}
	//Lasagna migrator? Maybe I should poll GBATemp to find out if it's worth my time to make it.
	//It seems like it's going to be quite annoying to pull off.
	//
	//

	initupdatechecker();
	svcWaitSynchronization(event_downloadthreadfinished, U64_MAX);
	if (isupdateavailable())
	{
		doprogressdraw = false;
		error("An update is available.\nIt will be installed now.");
		doprogressdraw = true; //This may cause issues...
		initdownloadandinstallupdate();
		progress = 0;
		total = 100;
		progresstext = "Downloading update...\n[progress]% complete";
		progresspopup();
		do
		{
			progress = retrievedownloadprogress();
			if (progress == 101)
			{
				progresstext = "Installing update...";
				progress = 100;
			}
		} while(progress != 102);
		progresspopdown();
		progressrunning = false; //Another point of conflict?
		svcWaitSynchronization(progressthreaddone, U64_MAX);
		error("Update complete. The system\nwill now reboot.");
		//nsrebootsystemclean();
	}
	//A quick tutorial?
	mainmenushiftin();
	//threadfunc_fade(colorvalues);
	errorsetmode(MODE_FADE);
	error("Let's get you acquainted\nwith ModMoon.");
	error("First, let's select the\ngames you want to use.");
	error("Touch the tools button.");
	//Most of the same logic as the real main menu, but only the tools button is selectable
	touchPosition tpos, opos;
	u32 kDown, kHeld;
	int alphapos = 0;
	bool alphaplus = true;
	while (aptMainLoop())
	{
		hidScanInput();
		kDown = hidKeysDown();
		kHeld = hidKeysHeld();
		hidTouchRead(&tpos);
		if (buttonpressed(rightbutton, 169, 13, opos, kHeld) || kDown & KEY_A)
		{
			mainmenushiftout();
			break;
		}
		draw.framestart();
		drawtopscreen();
		draw.drawon(GFX_BOTTOM);
		highlighterhandle(alphapos, alphaplus);
		mainmenudraw(1, tpos, alphapos, false);
		draw.frameend();
		opos = tpos;
	}
	//Navigate through the tools menu that doesn't yet exist...
	//
	//
	error("Select the titles you\nwant to use by tapping on them\nor using the Circle Pad and .");
	error("The cartridge is always selected.\nSelected titles will glow blue.");
	errorsetmode(MODE_POPUP); //activetitleselect can call errors
	activetitleselect();
	errorsetmode(MODE_FADE);
	error("You can always select new titles\nor deactivate existing ones by\nentering this menu.");
	//Go back through the tools menu...
	//
	//
	mainmenushiftin();
	error("Now press  to enter the title\nselection menu.");
	//Blah, duplicated code that I can't do anything about.
	while (aptMainLoop())
	{
		hidScanInput();
		kDown = hidKeysDown();
		kHeld = hidKeysHeld();
		if (kDown & KEY_Y)
			break;
		draw.framestart();
		drawtopscreen();
		draw.drawon(GFX_BOTTOM);
		highlighterhandle(alphapos, alphaplus);
		mainmenudraw(1, tpos, alphapos, false);
		draw.frameend();
	}
	error("To select the title currently in use,\nyou can use this menu and\ntap it or use the Circle Pad and .");
	errorsetmode(MODE_POPUP);
	titleselect();
	errorsetmode(MODE_FADE);
	//Fix a small issue with the tools menu and the main menu clashing together in a copied framebuffer
	draw.framestart();
	drawtopscreen();
	draw.drawon(GFX_BOTTOM);
	highlighterhandle(alphapos, alphaplus);
	mainmenudraw(0, tpos, 0, false);
	draw.frameend();
	error("You're all set! Have fun,\nand happy modding!");
	config.write("InitialSetupDone", true);
}