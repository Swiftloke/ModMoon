#include "main.hpp"
#include "config.hpp"
#include "utils.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <string.h>

//OK, so when the next update is released, we'll do config migration this way-
//The config file version will be determined, then based on that, we'll figure out what options need to be added.
//Like this. The numbering is odd as it is based on the config file version, not the ModMoon version;
//A version can change without adding config settings.
/*
	unordered_map<int, string> newoptions;
	newoptions[17] = R"raw(
	ThisOptionThatIsNewIn31{0}
	AnotherOption{1, 5})raw";
	Then based on that we'll figure out what needs to be appended to the file.
	for(int i = this->read("ConfigFileVersion", 0); i <= CONFIG_FILE_VERSION; i++)
	{
		this->configfile.append(newoptions[i]);
	}
*/
using namespace std;

void _mkdir(const char *dir) { //http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
        char tmp[256];
        char *p = NULL;
        size_t len;

        snprintf(tmp, sizeof(tmp),"%s",dir);
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
                if(*p == '/') {
                        *p = 0;
                        mkdir(tmp, S_IRWXU);
                        *p = '/';
                }
        mkdir(tmp, S_IRWXU);
}
/*void Rmkdir(string path) //My own attempt, but I'm not going to bother writing my own when there's already one out there
{
	char d = '/';
	unsigned int posa = 0, posb = 0;
	while(1)
	{
		posa = path.find(d, posb);
		if(posa == string::npos) break;
		posb = path.find(d, posa);
		if(posb == string::npos) break;
		mkdir(path.substr(posa, posb).c_str(), 0); //ctrulib ignores create mode
	}
}*/

void Config::createfile()
{
	/*ofstream out(filepath, ofstream::trunc | ofstream::binary);
	//out << "Enabled{True}\r\nGameType{Cia}\r\nGameRegion{Usa}\r\nLastSSDHash{}\r\nModsFolder{/saltysdMODS/}\r\nSmashSelectorVersion{3.0}\r\nConfigFileVersion{008}\r\nSelectedModSlot{1}\r\n\r\nMainCodeFolder{/luma/titles/}\r\nSplitCodeTextFolder{/corbenik/exe/text/}\r\nSplitCodeRoFolder{/corbenik/exe/ro/}\r\nSplitCodeDataFolder{/corbenik/exe/data/}\r\n\r\nConfigWorking{TRUE}\r\nInitialSetup{NotDone}\r\nHitboxDisplayActive{False}\r\n#When putting in the path make sure you use / and NOT \\ Also make sure the path ends in a /. \r\n#If you made a change to this file and smash selector no longer works, just delete this file.";
	out << configfiledefault;
	out.close();*/

	//This just... doesn't work as a global. So I guess it'll be a local until I get a better solution. Yay
	string configfiledefault = 
R"raw(ModMoonVersion{)raw" + to_string(MODMOON_VERSION) + R"raw(}
ModsEnabled{False}
ConfigFileVersion{)raw" + to_string(CONFIG_FILE_VERSION) + R"raw(}
InitialSetupDone{False}
ModsFolder{/3ds/ModMoon/}
ActiveTitleIDs{0}
TitleIDSlots{0}
SelectedTitleIDPos{0}
DarkModeEnabled{False}
DisableErrors{False}
DisableUpdater{False}
MainMenuHighlightColors{255, 0, 0}
ErrorHighlightColors{255, 0, 0}
TitleSelectHighlightColors{255, 0, 0}
ToolsMenuHighlightColors{133, 46, 165}
EnableFlexibleCartridgeSystem{False}
;This file saves config info for ModMoon.
;If you change things in this file and ModMoon breaks, just delete it.
;By enabling DisableErrors, you are disqualifying yourself from
;recieving aid with ModMoon; these functions are critical in troubleshooting.
)raw";

	this->configfile = configfiledefault;
	this->isflushed = false;
	this->flush();
}

Config::Config(string path, string filename)
{
	if(!(pathExist(path))){_mkdir(path.c_str());}
	filepath = path + filename;
	ifstream in(filepath.c_str(), ifstream::binary);
	if(!in)
	{
		in.close(); 
		createfile();
	}
	else
	{in.seekg(0, in.end);
	int size = in.tellg();
	in.seekg(0);
	char* buf = new char[size];
	in.read(buf, size);
	configfile = string(buf);
	delete[] buf;
	in.close();
	}
	if(this->read("ModMoonVersion", 0) < MODMOON_VERSION)
		this->write("ModMoonVersion", MODMOON_VERSION);
	//if(read("ConfigFileVersion", 0) < CONFIG_FILE_VERSION) updateconfig();
}

string Config::read(string configsetting)
{
	int pos = configfile.find(configsetting, 0);
	if(pos == string::npos)
		return "";
	pos += configsetting.size() + 1; //Now pos is after the '{' of the config setting
	int posb = configfile.find('}', pos);
	return configfile.substr(pos, posb-pos);
}

int Config::read(string configsetting, int null) //Extra argument because overloading can't be done with only a change in return type
{
	string out = read(configsetting);
	return atoi(out.c_str());
}

bool Config::read(string configsetting, bool null)
{
	string out = read(configsetting);
	return out == "True";
}

u64 Config::read(string configsetting, u64 null, u64 nullb)
{
	string out = read(configsetting);
	return strtoull(out.c_str(), NULL, 16);
}

vector<u64> Config::u64multiread(string configsetting, bool parseashex)
{
	string rawin = read(configsetting);
    vector<u64> out;
    size_t pos = 0, lastpos = 0;
    bool exit = false;
    while(!exit)
    {
        pos = rawin.find(", ", pos);
        if(pos == string::npos)
        {
            exit = true;
            pos = rawin.length();
        }
		string strout = rawin.substr(lastpos, pos - lastpos);
		out.push_back(strtoull(strout.c_str(), NULL, parseashex ? 16 : 10));
        pos += 2;
        lastpos = pos;
    }
	return out;
}

void Config::u64multiwrite(string configsetting, vector<u64>& newvalue, bool parseashex)
{
	string rawout;
	for (vector<u64>::iterator iter = newvalue.begin(); iter < newvalue.end(); iter++)
	{
		stringstream out;
		if(parseashex) 
			out << std::hex;
		out << *iter;
		rawout.append(out.str());
		if (iter + 1 != newvalue.end()) //Not the last element
			rawout.append(", ");
	}
	write(configsetting, rawout);
}

//Same thing but signed ints
vector<int> Config::intmultiread(string configsetting)
{
	string rawin = read(configsetting);
    vector<int> out;
    size_t pos = 0, lastpos = 0;
    bool exit = false;
    while(!exit)
    {
        pos = rawin.find(", ", pos);
        if(pos == string::npos)
        {
            exit = true;
            pos = rawin.length();
        }
		string strout = rawin.substr(lastpos, pos - lastpos);
		out.push_back(stoi(strout.c_str(), NULL, 10));
        pos += 2;
        lastpos = pos;
    }
	return out;
}

void Config::intmultiwrite(string configsetting, vector<int>& newvalue)
{
	string rawout;
	for (vector<int>::iterator iter = newvalue.begin(); iter < newvalue.end(); iter++)
	{
		stringstream out;
		out << *iter;
		rawout.append(out.str());
		if (iter + 1 != newvalue.end()) //Not the last element
			rawout.append(", ");
	}
	write(configsetting, rawout);
}

void Config::write(string configsetting, string newvalue)
{
	isflushed = false; //A change has been made to the file in ram, so it is no longer synced with the file on disc
	int pos = configfile.find(configsetting, 0);
	pos += configsetting.size() + 1; //Now pos is after the '{' of the config setting
	int posb = configfile.find_first_of('}', pos);
	configfile.erase(pos, posb - pos);
	configfile.insert(pos, newvalue);
}

//The compiler gets confused about overloading and accidentally uses the bool overload for a const char* without this. lmao
//Then the bool overload provided a const char* to the write function, calling the bool overload, providing a const char*...
//Resulting in a stack overflow.
//Quite the fun bug to spend 3 hours figuring out.
void Config::write(string configsetting, const char* newvalue)
{
	write(configsetting, string(newvalue));
}

void Config::write(string configsetting, int newvalue)
{
	write(configsetting, to_string(newvalue));
}

void Config::write(string configsetting, bool newvalue)
{
	write(configsetting, newvalue ? "True" : "False");
}

void Config::write(string configsetting, u64 newvalue, u64 null)
{
	stringstream out;
	out << std::hex << newvalue;
	write(configsetting, out.str());
}

void Config::flush()
{
	if(isflushed) return; //No need for the lengthy write time if the file hasn't been changed
	isflushed = true;
	ofstream out(filepath.c_str(), ios::trunc | ofstream::binary);
	out.write(configfile.c_str(), configfile.size());
	out.close();
}

Config::~Config()
{
	if(!isflushed) flush();
}

void reloadconfigvalues()
{
	//
}

void Config::updateconfig() //Migrate old stuff
{
	//Stubbed in initial release. Fleshed out fully; see top comment.
}