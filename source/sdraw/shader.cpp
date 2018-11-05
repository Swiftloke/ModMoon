#include "shader.hpp"

//Bunches and bunches of planning...

/*
As a construction argument, input the names of all the uniforms the shader has as an std::vector<string>.
Then the constructor will get those uniform locations and put them in an
std::unordered_map<const char*, int>. That will allow it to efficiently set uniforms.

Actually, never mind. It probably makes more sense to do it at runtime.
if uniform not in uniftable
	uniftable.append(shaderInstanceGetUniformLocation(this->shader, uniform))

This is meant to be an internal-only system; sdraw_fs is what's externally configurable to the user.

Vertex handling cannot be handled by using a single class; this is due to requiring AttrInfo/BufInfo 
to be properly configured with the correct vertex attributes. Therefore, each shader will derive

maybe instead of the constructor, provide an std::function as a construction argument to configure vertices?
It's not something I haven't done before, WorkerFunction does the same thing. 

In the base class, have a template<typename T> add_vert_internal for adding vertices. Then in the derived class
set the actual add_vert function = add_vert_internal<this::vertex>.

What does it need to do?
-Configure the shader itself, buffers, and attributes
-Handle uniform setting
-bind() function to set the vertex shader and configure the projection matrix
-Handle adding a vertex struct *SPECIFIC TO THE OBJECT* to the buffer array:
Needs a function that can provide the necessary info, then the function will add it to the buffer array.
Done by passing a class, or raw values? A class. MyFunc({values}) is totally fine in C++.
This works totally fine, so this is how it will be done:

#include <functional>
struct MyVert {float position[3]; float tc[2];};
void* vertlist;
template<typename T> T testfunc(T vert)
{
	T list = static_cast<T*>(vertlist)[0];
	list = vert;
	return vert;
}
std::function<MyVert(MyVert)> func = testfunc<MyVert>;
int main()
{
	func({3, 3, 3, 3, 3});
	return 0;
}

So vshader<ThisVertexType> seems to make the most sense.
Example definition:
struct {float pos[3]; float tc[2];} myvert;
sdraw::vshader<myvert> myvertShader;

Next up... Setting up the VAO. This is where dynamic typing would be REALLY HELPFUL.
Getting the name of a variable can be done with:
#define NAMEOFVAR(var) (#var)
The simplest thing to do here is probably just provide the types/sizes of each attribute.
On 3DS, the enum is GPU_FORMATS, in OpenGL it's GLenum.
But, with the name of the type, and the offset of the type, how hard could it be to deduce everything?

^^^^ That's a fun idea. That way, it doesn't matter where it is.

... After quite a bit of time looking at variadic templates, I hate to say that I think they're too complicated
for me to work with, given my current knowledge of C++. Should future me come back with sufficient knowledge, he
can fix this to make it better, but I like OpenEQ's idea that there should just be a single attach() function with one
type per call. I can then use std::extent and std::remove_extent to calculate the final glVertexAttribFormat.

Test 1:
*/

// Type your code here, or load an example.
/*#include <cstddef>
#include <cstdio>
#include <type_traits>
#include <functional>

#include "sdraw.hpp"

struct internal_vertex
{
    float pos[3]; float tc[2];
};

template<typename T> 
struct type2enum {static constexpr unsigned int value = 0;};

//template<> struct type2enum<int> {static constexpr unsigned int value = 0;};
template<> struct type2enum<float> {static constexpr GPU_FORMATS value = GPU_FLOAT;};
template<> struct type2enum<bool> {static constexpr GPU_FORMATS value = GPU_BYTE;};

template<typename T> class Shader
{
    public:
    typedef T (*VertexConverterFunc)(internal_vertex);
    T* vertexlist;
    Shader(VertexConverterFunc vertexconversion) : vertexconverter(vertexconversion)
    {
        vertexlist = (T*)linearAlloc(sizeof(T) * 1024);
        resetarraypos();
		AttrInfo_Init(&attrinfo);
		BufInfo_Init(&bufinfo);
    }
    ~Shader() {linearFree(vertexlist);}
    void appendvertex(internal_vertex& vert)
    {
        vertexlist[arraypos++] = *(vertexconverter(vert));
    }
    inline void resetarraypos() {arraypos = 0;}
    
    template<typename M> void appendVAO()
    {
        unsigned int count = std::extent<M>::value;
        GPU_FORMATS enumtype = 
            type2enum<typename std::remove_extent<M>::type>::value;
        //glVertexAttribFormat(attrcount++, count, enumtype, false, attroffset);
		AttrInfo_AddLoader(&attrinfo, attrcount++, enumtype, count);
        attroffset += sizeof(M);
    }

	void VAOfinalize()
	{
		//Permutation- the order in which vertices are arranged in the buffer.
		u32 permutation = 0x76543210;
		BufInfo_Add(&bufinfo, vertexlist, sizeof(T), attrcount, permutation);
	}

	void bind()
	{
		C3D_BindProgram(program);
		C3D_SetBufInfo(&bufinfo);
		C3D_SetAttrInfo(&attrinfo);
	}

	void setUniformF(const char* uniform, float x = 0, float y = 0, float z = 0, float w = 0)
	{
		int id = shaderInstanceGetUniformLocation(program, uniform);
		C3D_FVUnifSet(GPU_VERTEX_SHADER, id, x, y, z, w);
	}
	void setUniformI(const char* uniform, int x = 0, int y = 0, int z = 0, int w = 0)
	{
		int id = shaderInstanceGetUniformLocation(program, uniform);
		C3D_IVUnifSet(GPU_VERTEX_SHADER, id, x, y, z, w);
	}
	void setUniformB(const char* uniform, bool value)
	{
		int id = shaderInstanceGetUniformLocation(program, uniform);
		C3D_BoolUnifSet(GPU_VERTEX_SHADER, id, value);
	}

    private:
    VertexConverterFunc vertexconverter;
    unsigned int attroffset;
    unsigned int arraypos;
    unsigned int attrcount;

	C3D_AttrInfo attrinfo;
	C3D_BufInfo bufinfo;
	shaderProgram_s program;

	//Would probably harm performance more than it would help.
	//Otherwise, idea would be to save gotten uniform locations into a table.
	//std::unordered_map<string, int> uniformtable;
};*/

/*int main()
{
    struct f {float pos[3]; float tc[2];};
    Shader<f> basic
    ([] (internal_vertex vert) -> f {
        return {vert.pos[0], vert.pos[1], vert.pos[2], vert.tc[0], vert.tc[1]};
    });
    basic.appendVAO<float[3]>();
    basic.appendVAO<bool>();
}*/

#include "vshader_shbin.h"
#include "eventualvertex_shbin.h"
#include "twocoordsinterp_shbin.h"
#include "threetextures_shbin.h"

using namespace sdraw;

Shader<MM::vertex_basic> sdraw::MM::shader_basic;
Shader<MM::vertex_basic> sdraw::MM::shader_eventual;
Shader<MM::vertex_twocoords> sdraw::MM::shader_twocoords;
Shader<MM::vertex_threetextures> sdraw::MM::shader_threetextures;

void MM::initmodmoonshaders()
{
	shader_basic = Shader<MM::vertex_basic>(vshader_shbin, vshader_shbin_size);
	shader_basic.appendVAO<float[3]>();
	shader_basic.appendVAO<float[2]>();
	shader_basic.finalizeVAO();

	shader_eventual.appendVAO<float[3]>();
	shader_eventual.appendVAO<float[2]>();
	shader_eventual.finalizeVAO();

	shader_twocoords = Shader<MM::vertex_twocoords> (twocoordsinterp_shbin, twocoordsinterp_shbin_size);
	shader_twocoords.appendVAO<float[3]>();
	shader_twocoords.appendVAO<float[3]>();
	shader_twocoords.appendVAO<float[2]>();
	shader_twocoords.finalizeVAO();

	shader_threetextures = Shader<MM::vertex_threetextures> (threetextures_shbin, threetextures_shbin_size);
	shader_threetextures.appendVAO<float[3]>();
	shader_threetextures.appendVAO<float[2]>();
	shader_threetextures.appendVAO<float[2]>();
	shader_threetextures.appendVAO<float[2]>();
	shader_threetextures.finalizeVAO();
}