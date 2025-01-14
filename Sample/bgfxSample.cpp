#include <bgfx/bgfx.h>

#include <Gwen/Gwen.h>
#include <Gwen/Skins/Simple.h>
#include <Gwen/Skins/TexturedBase.h>
#include <Gwen/UnitTest/UnitTest.h>
#define BX_CONFIG_DEBUG 0
#if defined(_WIN32)
#include <Gwen/Input/Windows.h>
#else
#define ENTRY_CONFIG_USE_SDL 1
// #include <Gwen/Input/SDL13.h>
#endif


#include "bgfxRenderer.h"

#include "Gwen/Renderers/OpenGL_DebugFont.h"

// #include <common/entry.h>
// #include <common/dbg.h>
// #include <common/math.h>
// #include <common/processevents.h>

#include "TrueTypeFont.h"
#include "GlyphStash.h"

#include <stdio.h>
#include <string.h>


static const char* s_shaderPath = NULL;

static void shaderFilePath(char* _out, const char* _name)
{
	strcpy(_out, s_shaderPath);
	strcat(_out, _name);
	strcat(_out, ".bin");
}

long int fsize(FILE* _file)
{
	long int pos = ftell(_file);
	fseek(_file, 0L, SEEK_END);
	long int size = ftell(_file);
	fseek(_file, pos, SEEK_SET);
	return size;
}

static const bgfx::Memory* load(const char* _filePath)
{
	FILE* file = fopen(_filePath, "rb");
	if (NULL != file)
	{
		uint32_t size = (uint32_t)fsize(file);
		const bgfx::Memory* mem = bgfx::alloc(size+1);
		size_t ignore = fread(mem->data, 1, size, file);
		BX_UNUSED(ignore);
		fclose(file);
		mem->data[mem->size-1] = '\0';
		return mem;
	}

	return NULL;
}

static const bgfx::Memory* loadShader(const char* _name)
{
	char filePath[512];
	shaderFilePath(filePath, _name);
	return load(filePath);
}

static const bgfx::Memory* loadTexture(const char* _name)
{
	char filePath[512];
	strcpy(filePath, "textures/");
	strcat(filePath, _name);
	return load(filePath);
}


//engine abstraction
class TextureProvider_bgfx : public bgfx_font::ITextureProvider
{
public:
    TextureProvider_bgfx(uint16_t width, uint16_t height, uint32_t depth): m_width(width), m_height(height), m_depth(depth)
    {
		const bgfx::Memory* mem = NULL;// bgfx::alloc(width*height*depth);
		//memset(mem->data, 0, width*height*depth);
		// XXX This needs review
		uint32_t flags = BGFX_TEXTURE_NONE; // BGFX_TEXTURE_MIN_POINT|BGFX_TEXTURE_MAG_POINT|BGFX_TEXTURE_U_CLAMP|BGFX_TEXTURE_V_CLAMP;
		/*
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		*/
		//uint32_t flags = BGFX_TEXTURE_NONE;
		if(depth==1)
			// XXX This needs a review
			// bgfx::createTexture2D(uint16_t(m_viewState.m_width), uint16_t(m_viewState.m_height), false, 1, bgfx::TextureFormat::BGRA8, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_TEXTURE_RT),
			m_handle = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGB8, flags, mem);
		else
			// XXX This needs a review
			m_handle = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::BGRA8, flags, mem);		
    }

	~TextureProvider_bgfx()
	{
		bgfx::destroy(m_handle);
	}

    uint16_t getWidth() { return m_width; }
    uint16_t getHeight() { return m_height; }
    uint32_t getDepth() { return m_depth; }
    
    void update(bgfx_font::Rect16 rect, const uint8_t* data)
    {	
		const bgfx::Memory* mem = bgfx::alloc(rect.w*rect.h*m_depth);
		
		/*
		const bgfx::Memory* mem = bgfx::alloc(m_width*m_height*m_depth);
		size_t lineSize = m_width*m_depth;
		for(int i = 0; i < m_height; ++i)
		{
			if(i&1)			
				memset(mem->data+i*lineSize, 255, lineSize);			
			else
				memset(mem->data+i*lineSize, 0, lineSize);
		}*/


		memcpy(mem->data, data, rect.w*rect.h*m_depth);
		// XXX This needs a review
		// bgfx::updateTexture2D(m_handle, 0, rect.x, rect.y, rect.w, rect.h, mem);
		// void bgfx::updateTexture2D(TextureHandle _handle, uint16_t _layer, uint8_t _mip, uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const Memory *_mem, uint16_t _pitch = UINT16_MAX)¶
		bgfx::updateTexture2D(m_handle, 0, 0, rect.x, rect.y, rect.w, rect.h, mem, 0);
		
		//bgfx::updateTexture2D(m_handle, 0, 0, 0, m_width,m_height, mem);
    }

    void clear()
    {

    }
	
    uint16_t m_width;
    uint16_t m_height;
    uint32_t m_depth;
    bgfx::TextureHandle m_handle;
};

struct Vertex
{
	float x,y;
	float u,v;
	float r,g,b,a;
};

/*
(-1 -1) (1 -1)
(-1  1) (1  1) 
*/

//1 - 0
const float TW = 512.0f;
static Vertex s_quadVertices[4] =
{
	{ 0,  TW, 0.0f, 0.0f, 1.0f, 0.0 ,0.0, 1.0f},
	{ TW, TW, 1.0f, 0.0f, 1.0f, 1.0 ,0.0, 1.0f},
	{ 0,  0, 0.0f, 1.0f, 1.0f, 0.0 ,1.0, 1.0f},
	{ TW, 0, 1.0f, 1.0f, 1.0f, 1.0 ,0.0, 1.0f}
};

static uint16_t s_quadVerticesIdx[6] = { 0,2,1, 1,2,3 };

int _main_(int _argc, char** _argv)
{
    uint32_t width = 1280;
	uint32_t height = 720;
	uint32_t debug = BGFX_DEBUG_TEXT;

	bgfx::init();
	bgfx::reset(width, height);

	// Enable debug text.
	bgfx::setDebug(debug);

	// Set view 0 clear state.
	// XXX This needs review
	bgfx::setViewClear(0
		, BGFX_CLEAR_NONE// BGFX_CLEAR_COLOR_BIT|BGFX_CLEAR_DEPTH_BIT
		, 0x303030ff
		, 1.0f
		, 0
		);

    // Setup root path for binary shaders. Shader binaries are different 
	// for each renderer.
	switch (bgfx::getRendererType() )
	{
	default:
	case bgfx::RendererType::Direct3D9:
		s_shaderPath = "shaders/dx9/";
		break;

	case bgfx::RendererType::Direct3D11:
		s_shaderPath = "shaders/dx11/";
		break;

	case bgfx::RendererType::OpenGL:
		s_shaderPath = "shaders/glsl/";
		break;

	case bgfx::RendererType::OpenGLES:
		s_shaderPath = "shaders/gles/";
		break;
	}

	// XXX TODO: Missing Vulkan, Metal, etc.

   
    Gwen::Renderer::bgfxRenderer * pRenderer = new Gwen::Renderer::bgfxRenderer(0,s_shaderPath, "textures/");
	pRenderer->Init();

	//
	// Create a GWEN skin
	//
	//Gwen::Skin::TexturedBase* pSkin = new Gwen::Skin::TexturedBase( pRenderer );
	//pSkin->Init("DefaultSkin.png");

	Gwen::Skin::Simple* pSkin = new Gwen::Skin::Simple();
	pSkin->SetRender(pRenderer);
	//pSkin->Init("DefaultSkin.png");

	//
	// Create a Canvas (it's root, on which all other GWEN panels are created)
	//
	Gwen::Controls::Canvas* pCanvas = new Gwen::Controls::Canvas( pSkin );
	pCanvas->SetSize( 998, 650 - 24 );
	pCanvas->SetDrawBackground( true );
	pCanvas->SetBackgroundColor( Gwen::Color( 150, 170, 170, 255 ) );

	//
	// Create our unittest control (which is a Window with controls in it)
	//
	UnitTest* pUnit = new UnitTest( pCanvas );
	pUnit->SetPos( 10, 10 );

	//
	// Create a Windows Control helper 
	// (Processes Windows MSG's and fires input at GWEN)
	//
	#if defined(_WIN32)
	Gwen::Input::Windows GwenInput;
	#else
	Gwen::Input::SDL13 GwenInput;
	#endif
	GwenInput.Initialize( pCanvas );
    



    bgfx_font::TrueTypeFont* font = new  bgfx_font::TrueTypeFont();
    font->loadFont("c:/windows/fonts/times.ttf");

    TextureProvider_bgfx* text_provider = new TextureProvider_bgfx(512, 512, 1);
    bgfx_font::GlyphStash stash(text_provider);
	std::vector<uint8_t> buffer;
	float emSize = 18.0f;
	bgfx_font::GlyphSize glyphSize = font->getGlyphSize('a',emSize);
    bgfx_font::Rect16 rect = stash.allocateRegion(glyphSize.width, glyphSize.height);
	buffer.resize(glyphSize.width*glyphSize.height);
    font->bakeGlyphAlpha('a', emSize, &buffer[0]);
	stash.updateRegion(rect, &buffer[0]);

	glyphSize = font->getGlyphSize('M', 192.0f);
    rect = stash.allocateRegion(glyphSize.width, glyphSize.height);
	buffer.resize(glyphSize.width*glyphSize.height);
    font->bakeGlyphAlpha('M', 192.0f, &buffer[0]);
	stash.updateRegion(rect, &buffer[0]);
	
	// Create vertex stream declaration.
	bgfx::VertexLayout vertexDecl;
	vertexDecl.begin();
		vertexDecl.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float);
		vertexDecl.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
		vertexDecl.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float);
	vertexDecl.end();
	
	const bgfx::Memory* mem;

	// Create static vertex buffer.
	mem = bgfx::makeRef(s_quadVertices, sizeof(s_quadVertices) );
	bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(mem, vertexDecl);

	// Create static index buffer.
	mem = bgfx::makeRef(s_quadVerticesIdx, sizeof(s_quadVerticesIdx) );
	bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(mem);

	// Create texture sampler uniforms.
	bgfx::UniformHandle u_texColor = bgfx::createUniform("u_texColor", bgfx::UniformType::Uniform1iv);	
	//bgfx::UniformHandle u_color0 = bgfx::createUniform("v_color0", bgfx::UniformType::Uniform4fv);

	// Load vertex shader.
	mem = loadShader("vs_font_basic");
	bgfx::ShaderHandle vsh = bgfx::createShader(mem);

	// Load fragment shader.
	mem = loadShader("fs_font_basic");
	bgfx::ShaderHandle fsh = bgfx::createShader(mem);

	// Create program from shaders.
	bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh);

	// We can destroy vertex and fragment shader here since
	// their reference is kept inside bgfx after calling createProgram.
	// Vertex and fragment shader will be destroyed once program is^
	// destroyed.
	bgfx::destroy(vsh);
	bgfx::destroy(fsh);
	

    //void bakeGlyphAlpha(uint32_t codePoint, float size, uint8_t* outBuffer);

    while (!processEvents(width, height, debug) )
	{
		// Set view 0 default viewport.
		bgfx::setViewRect(0, 0, 0, width, height);

		// This dummy draw call is here to make sure that view 0 is cleared
		// if no other draw calls are submitted to view 0.
		bgfx::submit(0);

		// Use debug font to print information about this example.
		bgfx::dbgTextClear();
		bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/00-helloworld");
		bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: Initialization and debug text.");

		
		float at[3] = { -10.0f, -10.0f, 0.0f };
		float eye[3] = {-10.0f, -10.0f, -1.0f };
		
		float view[16];
		float proj[16];
		mtxLookAt(view, eye, at);
		//mtxProj(proj, 60.0f, 16.0f/9.0f, 0.1f, 1000.0f);
		mtxOrtho(proj, 0,width,0,height,0.1f, 1000.0f);
		// Set view and projection matrix for view 0.
		bgfx::setViewTransform(0, view, proj);
		
		//float color[4] = { 1.0f, 0.5f,0.5f,1.0f};		
		//bgfx::setUniform(u_color0, color);

		// Set vertex and fragment shaders.
		bgfx::createProgram(program);

		// Set vertex and index buffer.
		// bgfx::setVertexBuffer(vbh);
		bgfx::setVertexBuffer(0, vbh);
		bgfx::setIndexBuffer(ibh);

		bgfx::setTexture(0, u_texColor, text_provider->m_handle);

		bgfx::setState( BGFX_STATE_RGB_WRITE
				|BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
				//|BGFX_STATE_ALPHA_TEST
				|BGFX_STATE_DEPTH_WRITE
				|BGFX_STATE_DEPTH_TEST_LESS
				);

		// Submit primitive for rendering to view 0.
		bgfx::submit(0);

		pCanvas->RenderCanvas();


        // Advance to next frame. Rendering thread will be kicked to 
		// process submitted rendering primitives.
		bgfx::frame();
	}


	bgfx::destroyIndexBuffer(ibh);
	bgfx::destroyVertexBuffer(vbh);
	bgfx::destroyUniform(u_texColor);
	//bgfx::destroyUniform(u_color0);

	delete font;
    delete text_provider;
	
	bgfx::destroy(program);

	// Shutdown bgfx.
    bgfx::shutdown();

	return 0;

    /*
	//
	// Begin the main game loop
	//
	MSG msg;
	while( true )
	{
		// Skip out if the window is closed
		if ( !IsWindowVisible( g_pHWND ) )
			break;

		// If we have a message from windows..
		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			// .. give it to the input handler to process
			GwenInput.ProcessMessage( msg );

			// if it's QUIT then quit..
			if ( msg.message == WM_QUIT )
				break;


			// Handle the regular window stuff..
			TranslateMessage(&msg);
			DispatchMessage(&msg);

		}

		// Main OpenGL Render Loop
		{
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			pCanvas->RenderCanvas();

			SwapBuffers( GetDC( g_pHWND ) );
		}
	}

	// Clean up OpenGL
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( OpenGLContext );

	delete pCanvas;
	delete pSkin;
	delete pRenderer;
    */

}
