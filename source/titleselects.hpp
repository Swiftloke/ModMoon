#pragma once
#include <3ds/types.h>
//Both the regular title selection menu and the active title selection menu
void titleselect();
void activetitleselect();
//This is used during initial setup. This use case is based around when a title
//needs to be activated, but all the title icons need to finish loading
//before that can occur.
void queuetitleforactivationwithinmenu(u64 titleid, int mediatype);

//Allow this to be done manually. This use case is for modpack downloading.
void triggeractivationqueue();
