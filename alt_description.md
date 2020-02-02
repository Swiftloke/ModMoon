![](https://preview.ibb.co/heKvWp/banner.png)


 ...I must be crazy; especially given that this was originally just a recreation of Smash Selector.

**Latest Release: Version 3.0.1.**

**So, what exactly is ModMoon?**

This software is a LayeredFS/SaltySD manager for the 3DS. It supports every title in existence, even homebrew! It also includes many awesome easter eggs and features for the user to discover, but core functionality is its main goal. Switching mods is painless, and the user-interface is beautifully simple.
What makes ModMoon especially unique is its UI, built off of the sDraw rendering engine- an original, immensely flexible graphics core created for ModMoon. This is much, much more impressive than it sounds. For end users, the result will be a UI experience superior and better looking (in my humble opinion :) ) to nearly all homebrew on the 3DS. sDraw is the reason ModMoon started development - I wanted Smash Selector to have an excellent UI, instead of boring and confusing console text.

**Functionality**
ModMoon has a ton of stuff packed in to it. Here's a quick overview of what everything does throughout the various menus.

**Main Menu:
Launch:** Applies and launches mods for the selected game.
**Tools:** Opens up the tools menu, which provides access to more functionality.
**Selector Bar:** Changes the currently selected mod for the current game.

**Tools Menu:
Active Title Selection:** This menu allows you to change the titles currently "active" (which titles are available in the title selection menu). This is to minimize clutter by only displaying the titles the user actually uses.
**Smash Controls Modifier:** This code is taken straight out of Smash Selector 2.4 with no changes. It allows you, like Smash Selector, to modify the controls of the game with much greater customization than the game itself allows- including mapping two buttons to the same action, changing the controls of the D-Pad, and even changing the actions of the New 3DS buttons/C-Stick!
**Tutorial:** This replays the tutorial that was shown at the first start of ModMoon.
**Migrate Mods:** This allows you to migrate mods that were used in Smash Selector. This action is done automatically at first start.
**Dark/Light Mode:** This option allows you to change between two "themes"- dark mode and light mode! To you dark theme master racers out there, this one is for you.

**Major Features:
Error Checking:** ModMoon supports checking, displaying, and sometimes even automatically fixing errors when moving mods around. This is a significant source of information for users and helpers, as earlier mod tools were very opaque about what went wrong (and even if something went wrong at all), thus breaking quite a bit. Anyone who has ever used Smash mods knows this aggravation- this feature alone makes Smash Selector obsolete.
**Help Pop-up:** At any time, you can press X while hovering over a button or in a menu to get some helpful information about that feature- what it is, how to use it, and helpful tips!
**Title Selection Menu:** Press Y to open this from any menu. Allows you to select the current game from your list of active titles.
**Built-in SaltySD files:** For Smash players, ModMoon contains a set of built in IPS files for SaltySD, just like its predecessor Smash Selector. These files will be copied when you select Smash from the Active Title Selection menu. (If Smash on an SD card is automatically selected for you, these files will also be copied automatically. If you have Smash on a cartridge, just tap it like you would any other title to trigger the copy.)
**Background title loader:** All titles, both active and non-active, installed on the system will be loaded immediately in the background, with no lag to the rest of the system.
**Auto-updater (ModMoon):** If you're connected to the internet, ModMoon will check for updates for itself. This system runs in the background, which means you'll experience no lag while it does its thing. If an update is available, it will prompt you to update (and give you an opportunity to skip it) and install the update.
**Auto-updater (SaltySD):** This is a big one for Smash players. Smash Selector, like ModMoon had built in SaltySD IPS files. However, those files were very old, and there was never a system in place to update them. This is, of course, solved with ModMoon. Alongside updating itself, it will check for updates to the aforementioned SaltySD files used to run Smash mods, and download them seamlessly without rebooting.
**Not a fan of auto-updaters?** Understandable. Just open the config file (/3ds/ModMoon/settings.txt) and set "ShouldDisableUpdater" to "True", but keep in mind that you'll miss out on any improvements I ever make until you either update manually or re-enable the updater. I highly recommend against this, but the choice is there.
**Flexible Cartridge System:** This system automatically detects if you've inserted/ejected a cartridge, and reacts based on that, just like the HOME menu. Unfortunately, there seems to be a bug with either the 3DS homebrew development system (libctru) or Luma3DS which makes this code, when enabled, take upwards of 30 seconds to exit ModMoon. It is disabled by default, and you can try it out by, in the config system, setting "EnableFlexibleCartridgeSystem" to "True". When a solution is found, I will release an update with the fix and enable this for all users.
**Configurable Highlighter Colors:** ModMoon, as a modding tool, has some customization tools for itself as well. Alongside the Dark Mode functionality, a user can change the highlighter colors used in each of the menus! Open the config file (/3ds/ModMoon/settings.txt), and inside the "HighlighterColors" option (where * is the menu you want to change) write the red, blue and green values (in 0-255 format) with commas separating them. Google has a nice tool to do this for you (use the rgb(\*, \*, \*) output).
**Custom SaltySD:** For Smash mod creators, this one is pretty nice. It allows you to provide a custom code.ips file within the /codes folder of your modpack, and it will automatically be moved to the correct location. LayeredFS games need not worry about this since the file structure will allow you to do this anyway.

**Usage**
Usage of ModMoon is pretty easy. Here's how everything works:

ModMoon works with mods that it finds in your "ModsFolder" config setting (by default, it's /3ds/ModMoon, but you can change it to whatever you'd like by opening the file (/3ds/ModMoon/settings.txt) and changing the setting). It then looks into the folder of the title ID of the game you want to load mods for (created automatically when you select a title from the Active Title Selection menu) and reads mods from the "Slot_X" folder, where X is a number starting at 1. Here's a quick visual example:
[SPOILER="Folder Structure"]
The following is an example for Smash 3DS (00040000000EDF00) and Pokemon Sun (0004000000164800).

```
G:\3ds\ModMoon
|
|
+---00040000000EDF00
|    \---Slot_1
|    |   |   desc.txt
|    |   |
|    |   \---animcmd
|    |       \---fighter
|    |           \---captain
|    |                \--game.bin
|    |
|    \---Slot_2
|    |    \--desc.txt
|    |
|    \---Slot_3
|    |    \--desc.txt
|    |
|    \---Slot_4
|    |    \--desc.txt
|    |
|    \---Slot_5
|    |    \--desc.txt
|    |
|    \---Slot_6
|    |    \--desc.txt
|    |
|    \---Slot_7
|         \--desc.txt
|
+---0004000000164800
|    \---Slot_1
|    |    |    desc.txt
|    |    |
|    |    \---romfs
|    |        \--Shop.cro
|    |    \---a
|    |        \---0
|    |            \---1
|    |                \--3
```

[/SPOILER]

When you first start up ModMoon, it will migrate your mods from Smash Selector, check for updates, then play a quick tutorial. It will show you how to set your active titles in the Active Title Selection menu, then how to select which game to run mods for. Because the Active Title Selection menu makes the folders for the games you select, I'd recommend going through this tutorial before adding your mods.

On a regular use, you'll select the game you want to use, then change what mod you want to run by scrolling through the list, tapping on the selector bar (or moving the Circle Pad left and right with the selector bar highlighted). To disable mods, simply select "Disabled" from the mods list. You'll then press the Launch button, and ModMoon will fade into the game!

Note that you *must* exit ModMoon by either pressing Start or launching a game. Shutting down the 3DS, or closing ModMoon by pressing "close" in the Home Menu, will, on top of hanging the system, not give it the chance to save configuration information for itself and the mods it handles. On top of the aforementioned hang, you will receive several error messages the next time ModMoon starts due to this failure to save.

As for using the many tools available, check the help popup for each option for instructions.

**Errors**
As previously mentioned, ModMoon has quite a bit of built in error checking. Here are some of the things it will warn the user about, and how you should solve them:

**Warning: Failed to find mods for this game!** ModMoon couldn't find any mod slots for the game you just selected. You should make some!

**Failed to enable/disable mods! This may resolve itself through normal usage.** Try the inverse of the action you just performed (as in, if you just enabled mods and that error message occurred, try disabling them; if you tried to disable mods, try enabling them) and this error message will probably disappear. You can then move on with your modding experience :)

**Congrats! You have gained 30 extra lives!** Hmmmm... Where does this appear, and what does it indicate? ( ͡° ͜ʖ ͡° )

**Failed to move slot file from X to X! Error code:** What could be going wrong here depends on the error code. See below.

In the Smash Controls Modifier: **The currently selected title is not Smash. Please select Smash and try again.** ModMoon will not attempt to modify the save data of a game other than Smash- this would probably cause save corruption. Set Smash as your active title and try to open it again.

**Custom SaltySD code.ips move failed! (original move) Error Code:** ModMoon failed to move the normal SaltySD file out of, or in to, the /luma/titles folder.

**Custom SaltySD code.ips move failed! (custom move) Error Code:** ModMoon failed to move the custom code.ips file from or to the mod slot it originated from.

I have come across two prominent error codes in development:
**Error Code 2: "No such file or directory"**. ModMoon itself indicates that this is probably the result of you not properly closing it out. ModMoon requires that you close it either by pressing Start to exit or launching a game- otherwise, it will not be able to properly save mod configurations.
**Error Code C82044BE: "Destination already exists"**. ModMoon will attempt to handle this error automatically, but will not attempt to destroy any files. This error code means that ModMoon cannot move a folder somewhere because that folder already exists. Check the destination folder it provided you, and if necessary manually move it back to where it should be (this error may occur if you broke its record-keeping by, say, shutting it off improperly (see "Usage") and ModMoon won't fix it automatically if the attempt will destroy files in the process. It's like asking a robot to move records around file cabinets, except you already had them do it but you wiped their memory, and now they can't try doing that same movement again without intervention from someone who can analyze the situation.) If this error occurred while disabling Smash mods, check the luma/titles/ folder. There will be a Disabled(smash title ID) folder and a (smash title ID) folder. Remove one or the other to resolve the situation.

**Development Time**
Smash Selector 2.4 is just over a year old at the time of writing. Why did it take so long to make its successor?
Soon-ish (within the next few weeks) I intend on making a full article about what goes into homebrew development, and why that caused ModMoon to release rather late. Check back soon, I'll post a link to it when it's done.

**Credits**
ModMoon was my first from-the-ground-up, full-scale, completely polished program. Along the course of its development there are many, many people I want to thank for making development of this program possible.

The USM-eM Team, composed of [USER=369787]@LinkSoraZelda[/USER], [USER=387920]@Dannyo15[/USER], [USER=416320]@DewTek[/USER], [USER=366937]@Yudowat[/USER], Gryz, Karma, and M-1: Fostering the scene in which I developed ModMoon. Encouraging me to work on Smash-Selector, which is what taught me to program, and many more small pieces of encouragement that have made a significant impact.
[USER=366937]@Yudowat[/USER], [USER=369787]@LinkSoraZelda[/USER] and [USER=402948]@xGhostBoyx[/USER] for the amazing design of the UI!
[USER=372791]@Cydget[/USER], for originally developing Smash Selector and allowing me to work on it with him. Also for nerd-sniping me late in development into making several interesting graphics effects (namely, the animated banner and launch button).
[USER=274292]@realWinterMute[/USER] and fincs, for giving me early access to the new versions of citro3d and early access to citro2d, and for a very particular event.
You, for reading the credits! As a reward, here's a hint- try entering the Konami Code in ModMoon.
fincs, for a TON of things. Inspiring me to learn graphics programming, without which ModMoon in its present maturity would have been impossible. Making citro3d, without which ModMoon would also have been impossible. Countless hours of help with citro3d, without which ModMoon would have been (guess what!) impossible. Helping me track down many different bugs within ModMoon's codebase. Just being awesome. Props to this guy.
[USER=38848]@smealum[/USER], [USER=274292]@realWinterMute[/USER], mtheall, fincs, yellows8, Lectem, and every other contributor to libctru, for building such an awesome library for 3DS development.
[USER=46970]@Aurora Wright[/USER] and [USER=373953]@TuxSH[/USER] for making Luma3DS, and more importantly LayeredFS and code.ips loading.
[USER=318030]@shinyquagsire23[/USER], for developing SaltySD.
[USER=369787]@LinkSoraZelda[/USER] and Cloud Road Music for the banner music (taken from USM-eM)
M-1 for shooting me the code.bin files to build SaltySD with at the last minute.
[USER=370603]@ih8ih8sn02[/USER] for the reverse engineering work that made the Smash Controls Modifier possible.
The USM-eM beta testers, specifically, [USER=376636]@NoThisIsStupider[/USER], Solid, and Lil-G.
mtheall, for his incredible work in two areas. The first: resolving a ridiculous crash that left ModMoon in an unreleaseable state for weeks on end. This guy put several days of his own time in to use his significant expertise to solve my mistake. The release of 3.0 would have been delayed indefinitely without him. The second: Creating Tex3DS (a fancy graphics tool for building textures), and helping me migrate ModMoon's codebase to use it instead of a nasty hack.

**Builds and Source**
The latest release can be found here: [URL]https://github.com/Swiftloke/ModMoon/releases[/URL]
The source code can be found here: [URL]https://github.com/Swiftloke/ModMoon[/URL]
Did I forget something in this release writeup? Got an idea for a new feature for me to add? Don't hesitate to reply and mention it or contact me on Discord at Swiftloke#3647. (I will not respond to DMs on GBATemp.)

I hope you enjoy this thing. All the effort that's gone in to it pays off, I hope :)