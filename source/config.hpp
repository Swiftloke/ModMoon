#pragma once
#include <string>
#include <vector>
#include <3ds/types.h>

#define CONFIG_FILE_VERSION 16
#define MODMOON_VERSION 30
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