#include <3ds.h>
#include <citro3d.h>

#include <type_traits>

namespace sdraw
{

	struct internal_vertex
	{
		float x1, y1, z1;
		float x2, y2, z2;
		float tc0x, tc0y;
		float tc1x, tc1y;
		float tc2x, tc2y;
	};

	template<typename T>
	struct type2enum { static constexpr unsigned int value = 0; };

	//template<> struct type2enum<int> {static constexpr unsigned int value = 0;};
	template<> struct type2enum<float> { static constexpr GPU_FORMATS value = GPU_FLOAT; };
	template<> struct type2enum<bool> { static constexpr GPU_FORMATS value = GPU_BYTE; };

	//Abstract class used for storage of a pointer to a real templatized class
	class ShaderBase
	{
	public:
		virtual unsigned int getArrayPos() = 0;
		virtual void resetArrayPos() = 0;
		virtual void appendVertex(internal_vertex& vert) = 0;
		virtual void setUniformF(const char* uniform, float x, float y = 0, float z = 0, float w = 0) = 0;
		virtual void setUniformMtx4x4(const char* uniform, C3D_Mtx* matrix) = 0;
		virtual void setUniformI(const char* uniform, int x, int y = 0, int z = 0, int w = 0) = 0;
		virtual void setUniformB(const char* uniform, bool value) = 0;
		virtual ~ShaderBase() {};
	};

	template<typename T> class Shader : ShaderBase
	{
	public:
		T* vertexlist;
		Shader() {} //Allows for defining shaders as extern
		Shader(const u8* vshaderbin, u32 vshadersize) : arraypos(0)
		{
			vertexlist = (T*)linearAlloc(sizeof(T) * 1024);
			AttrInfo_Init(&attrinfo);
			BufInfo_Init(&bufinfo);

			DVLB_s* DVLB = DVLB_ParseFile((u32*)vshaderbin, vshadersize);
			shaderProgramInit(&program);
			shaderProgramSetVsh(&program, &DVLB->DVLE[0]);
		}
		~Shader() { linearFree(vertexlist); }
		void appendVertex(internal_vertex& vert)
		{
			vertexlist[arraypos++] = T(vert);
		}

		inline void resetArrayPos() { arraypos = 0; }
		inline unsigned int getArrayPos() { return arraypos; }

		template<typename M> void appendVAO()
		{
			unsigned int count = std::extent<M>::value;
			GPU_FORMATS enumtype =
				type2enum<typename std::remove_extent<M>::type>::value;
			//glVertexAttribFormat(attrcount++, count, enumtype, false, attroffset);
			AttrInfo_AddLoader(&attrinfo, attrcount++, enumtype, count);
			attroffset += sizeof(M);
		}

		void finalizeVAO()
		{
			//Permutation- the order in which vertices are arranged in the buffer.
			u32 permutation = 0x76543210;
			BufInfo_Add(&bufinfo, vertexlist, sizeof(T), attrcount, permutation);
		}

		void bind()
		{
			C3D_BindProgram(&program);
			C3D_SetBufInfo(&bufinfo);
			C3D_SetAttrInfo(&attrinfo);
			//In namespace sdraw
			updateshaderstate(this);
		}

		void setUniformF(const char* uniform, float x, float y = 0, float z = 0, float w = 0)
		{
			int id = shaderInstanceGetUniformLocation(program.vertexShader, uniform);
			C3D_FVUnifSet(GPU_VERTEX_SHADER, id, x, y, z, w);
		}
		void setUniformMtx4x4(const char* uniform, C3D_Mtx* matrix)
		{
			int id = shaderInstanceGetUniformLocation(program.vertexShader, uniform);
			C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, id, matrix);
		}
		void setUniformI(const char* uniform, int x, int y = 0, int z = 0, int w = 0)
		{
			int id = shaderInstanceGetUniformLocation(program.vertexShader, uniform);
			C3D_IVUnifSet(GPU_VERTEX_SHADER, id, x, y, z, w);
		}
		void setUniformB(const char* uniform, bool value)
		{
			int id = shaderInstanceGetUniformLocation(program.vertexShader, uniform);
			C3D_BoolUnifSet(GPU_VERTEX_SHADER, id, value);
		}

	private:
		unsigned int attroffset = 0;
		unsigned int arraypos = 0;
		unsigned int attrcount = 0;

		C3D_AttrInfo attrinfo;
		C3D_BufInfo bufinfo;
		shaderProgram_s program;

		//Would probably harm performance more than it would help.
		//Otherwise, idea would be to save gotten uniform locations into a table.
		//std::unordered_map<string, int> uniformtable;
	};

	//User code goes here.
	namespace MM
	{

	struct vertex_basic
	{
		float x, y, z; float tcx, tcy;
		vertex_basic(internal_vertex& vert) :
		x(vert.x1), y(vert.y1), z(vert.z1), tcx(vert.tc0x), tcy(vert.tc0y) {}
	};

	extern Shader<vertex_basic>* shader_basic;
	extern Shader<vertex_basic>* shader_eventual;

	struct vertex_twocoords
	{
		float x1, y1, z1;
		float x2, y2, z2;
		float tcx, tcy;
		vertex_twocoords(internal_vertex& vert) :
		x1(vert.x1), y1(vert.y1), z1(vert.z1), x2(vert.x2), y2(vert.y2), z2(vert.z2),
		tcx(vert.tc0x), tcy(vert.tc0y) {}
	};

	extern Shader<vertex_twocoords>* shader_twocoords;

	struct vertex_threetextures
	{
		float x, y, z;
		float tc0x, tc0y;
		float tc1x, tc1y;
		float tc2x, tc2y;
		vertex_threetextures(internal_vertex& vert) :
		x(vert.x1), y(vert.y1), z(vert.z1), tc0x(vert.tc0x), tc0y(vert.tc0y),
		tc1x(vert.tc1x), tc1y(vert.tc1y), tc2x(vert.tc2x), tc2y(vert.tc2y) {}
	};
	extern Shader<vertex_threetextures>* shader_threetextures;

	void initmodmoonshaders();

	void destroymodmoonshaders();

	} //namespace MM


} //namespace sdraw