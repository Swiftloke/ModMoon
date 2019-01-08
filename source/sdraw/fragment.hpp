#pragma once
#include "sdraw.hpp"
#include <unordered_map>
#include <functional>
#include <string>
using std::string;

void initmodmoontevkeys();

namespace sdraw
{
	//void assign(const char* key, std::function<C3D_TexEnv(void)> generator);
	void assign(const char* key, C3D_TexEnv* tev);

	//Note: These functions triggers a draw call in order to change state.
	void setfs(const char* key, unsigned int stage = 0, u32 color1 = 0 /*, u32 color2 = 0*/);
	void setfs(C3D_TexEnv tev, unsigned int stage = 0, u32 color1 = 0 /*, u32 color2 = 0*/);

	void bindtex(unsigned int texunit, sdraw_stex tex);
	void bindtex(unsigned int texunit, C3D_Tex* tex);
} //namespace sdraw