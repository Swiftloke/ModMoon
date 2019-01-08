#include "fragment.hpp"

std::unordered_map<string, C3D_TexEnv> fragmentmap;
C3D_Tex* boundtex[3] = {0};

//Most TexEnv structs are generated here.

void initmodmoontevkeys()
{
	C3D_TexEnv* tev = new C3D_TexEnv; //Would rather not spam "&" in every TexEnv* call
	{
		C3D_TexEnvInit(tev);
		sdraw::assign("blank", tev);
	}
	{
		C3D_TexEnvInit(tev);
		C3D_TexEnvSrc(tev, C3D_Both, GPU_TEXTURE0);
		C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
		C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA);
		C3D_TexEnvFunc(tev, C3D_Both, GPU_REPLACE);
		
		sdraw::assign("texture", tev);
	}
	{
		C3D_TexEnvInit(tev);
		C3D_TexEnvSrc(tev, C3D_RGB, GPU_CONSTANT);
		C3D_TexEnvSrc(tev, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT);
		C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
		C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA);
		C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
		C3D_TexEnvFunc(tev, C3D_Alpha, GPU_MODULATE);
		
		sdraw::assign("textColor", tev);
	}
	{
		C3D_TexEnvInit(tev);
		C3D_TexEnvSrc(tev, C3D_Both, GPU_CONSTANT, GPU_CONSTANT);
		C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
		C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
		C3D_TexEnvFunc(tev, C3D_Both, GPU_REPLACE);
		
		sdraw::assign("constColor", tev);
	}
	{
		C3D_TexEnvInit(tev);
		//Citro3D port of this https://www.khronos.org/opengl/wiki/Texture_Combiners#Example_:_Blend_tex0_and_tex1_based_on_a_blending_factor_you_supply
		//*Lack of fragment shader intensifies*
		//Configure the fragment shader to blend texture0 with texture1 based on the alpha of the constant
		C3D_TexEnvSrc(tev, C3D_Both, GPU_TEXTURE0, GPU_TEXTURE1, GPU_CONSTANT);
		//One minus alpha to get it to be 0 -> all texture 0, 256 -> all texture1, whereas it would be the opposite otherwise
		C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA);
		C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA);
		//C3D_TexEnvColor(&tev, RGBA8(0, 0, 0, blendfactor));
		C3D_TexEnvFunc(tev, C3D_Both, GPU_INTERPOLATE);
		
		sdraw::assign("blendTex0Tex1", tev);
	}
	{
		C3D_TexEnvInit(tev);
		C3D_TexEnvSrc(tev, C3D_RGB, GPU_CONSTANT);
		C3D_TexEnvSrc(tev, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT);
		C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
		C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA);
		C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
		C3D_TexEnvFunc(tev, C3D_Alpha, GPU_MODULATE);
		//C3D_TexEnvColor(&tev, (color & 0xFFFFFF) | ((alpha & 0xFF) << 24));
		
		sdraw::assign("highlighter", tev);
	}
	{
		//Configure TexEnv stage 1 to "blink" the texture by making it all blue
		C3D_TexEnvInit(tev);
		C3D_TexEnvSrc(tev, C3D_RGB, GPU_CONSTANT);
		C3D_TexEnvSrc(tev, C3D_Alpha, GPU_PREVIOUS);
		C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
		C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA);
		C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
		C3D_TexEnvFunc(tev, C3D_Alpha, GPU_REPLACE);
		C3D_TexEnvColor(tev, RGBA8(0, 0, 255, 0));
		sdraw::assign("titleSelectBlink", tev);
	}
	{
		C3D_TexEnvInit(tev);
		C3D_TexEnvSrc(tev, C3D_RGB, GPU_TEXTURE0, GPU_TEXTURE1, GPU_CONSTANT);
		C3D_TexEnvSrc(tev, C3D_Alpha, GPU_TEXTURE0, GPU_TEXTURE1);
		C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA);
		C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
		C3D_TexEnvFunc(tev, C3D_RGB, GPU_INTERPOLATE);
		C3D_TexEnvFunc(tev, C3D_Alpha, GPU_MODULATE);
		sdraw::assign("launchButtonMoon", tev);
	}
	{
		C3D_TexEnvInit(tev);
		C3D_TexEnvSrc(tev, C3D_RGB, GPU_TEXTURE1);
		C3D_TexEnvSrc(tev, C3D_Alpha, GPU_TEXTURE0);
		C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
		C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA);
		C3D_TexEnvFunc(tev, C3D_Both, GPU_REPLACE);
		sdraw::assign("topScreenMoon", tev);
	}
	{
		C3D_TexEnvInit(tev);
		C3D_TexEnvSrc(tev, C3D_RGB, GPU_CONSTANT);
		C3D_TexEnvSrc(tev, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT);
		C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
		C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
		C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
		C3D_TexEnvFunc(tev, C3D_Alpha, GPU_ADD);
		C3D_TexEnvColor(tev, RGBA8(255, 255, 0, 254));
		sdraw::assign("secretCode", tev);
	}
	delete tev;
}

void sdraw::assign(const char* key, C3D_TexEnv* tev)
{
	fragmentmap.insert({ key, *tev });
}

/*void sdraw::assign(const char* key, std::function<C3D_TexEnv(void)> generator)
{
	//fragmentmap.insert({ key, generator() });
}*/

void sdraw::setfs(const char* key, unsigned int stage, u32 color1 /*, u32 color2*/)
{
	drawCall(); //Flush state

	C3D_TexEnvColor(&fragmentmap[key], color1);
	//C3D_TexEnvBufColor(color2);
	C3D_SetTexEnv(stage, &fragmentmap[key]);
	//Error checking code
	/*	std::unordered_map<string, C3D_TexEnv>::iterator find = fragmentmap.find(key);
	if (find != fragmentmap.end())
	{
		C3D_TexEnvColor(&(find->second), color1);
		//C3D_TexEnvBufColor(color2);
		C3D_SetTexEnv(stage, &(find->second));
	}
	else //Error checking
	{
		C3D_TexEnv tev;
		C3D_TexEnvInit(&tev);
		C3D_SetTexEnv(stage, &tev);
	}*/
}

void sdraw::setfs(C3D_TexEnv tev, unsigned int stage, u32 color1 /*, u32 color2 */)
{
	drawCall(); //Flush state
	C3D_TexEnvColor(&tev, color1);
	C3D_SetTexEnv(stage, &tev);
}

void sdraw::bindtex(unsigned int texunit, sdraw_stex tex)
{
	if (tex.spritesheet != boundtex[texunit])
	{
		drawCall(); //Flush state
		boundtex[texunit] = tex.spritesheet;
		C3D_TexBind(texunit, tex.spritesheet);
	}
}

void sdraw::bindtex(unsigned int texunit, C3D_Tex* tex)
{
	if (tex->data != boundtex[texunit]->data)
	{
		drawCall(); //Flush state
		boundtex[texunit] = tex;
		C3D_TexBind(texunit, tex);
	}
}