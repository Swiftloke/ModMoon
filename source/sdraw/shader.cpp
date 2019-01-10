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