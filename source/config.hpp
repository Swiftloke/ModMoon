/*
*   This file is part of ModMoon
*   Copyright (C) 2018-2019 Swiftloke
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#pragma once
#include <string>
#include <vector>
#include <3ds/types.h>

#define CONFIG_FILE_VERSION 18
#define MODMOON_VERSION 310
using namespace std;

class Config
{
	public:
	Config(string path, string filename);
	~Config();
	void flush();
	string read(string configsetting);
	bool read(string configsetting, bool null);
	int read(string conigsetting, int null);
	u64 read(string configsetting, u64 null, u64 nullb);
	vector<u64> u64multiread(string configsetting, bool parseashex);
	vector<int> intmultiread(string configsetting);
	void write(string configsetting, string newvalue);
	void write(string configsetting, const char* newvalue);
	void write(string configsetting, int newvalue);
	void write(string configseting, bool newvalue);
	void write(string configsetting, u64 newvalue, u64 null);
	void u64multiwrite(string configsetting, vector<u64>& newvalue, bool parseashex);
	void intmultiwrite(string configsetting, vector<int>& newvalue);
	bool isflushed = true;
	
	private:
	string filepath;
	string configfile;
	void createfile();
	void updateconfig();
};

void _mkdir(const char* path);
void reloadglobalvalues();
void regenerateconfig();