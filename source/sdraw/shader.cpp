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

#include "shader.hpp"

#include "vshader_shbin.h"
#include "eventualvertex_shbin.h"
#include "twocoordsinterp_shbin.h"
#include "threetextures_shbin.h"

using namespace sdraw;

Shader<MM::vertex_basic>* sdraw::MM::shader_basic;
Shader<MM::vertex_basic>* sdraw::MM::shader_eventual;
Shader<MM::vertex_twocoords>* sdraw::MM::shader_twocoords;
Shader<MM::vertex_threetextures>* sdraw::MM::shader_threetextures;

void MM::initmodmoonshaders()
{
	shader_basic = new Shader<MM::vertex_basic>(vshader_shbin, vshader_shbin_size);
	shader_basic->appendVAO<float[3]>();
	shader_basic->appendVAO<float[2]>();
	shader_basic->finalizeVAO();

	shader_eventual = new Shader<MM::vertex_basic>(eventualvertex_shbin, eventualvertex_shbin_size);
	shader_eventual->appendVAO<float[3]>();
	shader_eventual->appendVAO<float[2]>();
	shader_eventual->finalizeVAO();

	shader_twocoords = new Shader<MM::vertex_twocoords> (twocoordsinterp_shbin, twocoordsinterp_shbin_size);
	shader_twocoords->appendVAO<float[3]>();
	shader_twocoords->appendVAO<float[3]>();
	shader_twocoords->appendVAO<float[2]>();
	shader_twocoords->finalizeVAO();

	shader_threetextures = new Shader<MM::vertex_threetextures> (threetextures_shbin, threetextures_shbin_size);
	shader_threetextures->appendVAO<float[3]>();
	shader_threetextures->appendVAO<float[2]>();
	shader_threetextures->appendVAO<float[2]>();
	shader_threetextures->appendVAO<float[2]>();
	shader_threetextures->finalizeVAO();
}

void MM::destroymodmoonshaders()
{
	delete shader_basic;
	delete shader_eventual;
	delete shader_twocoords;
	delete shader_threetextures;
}