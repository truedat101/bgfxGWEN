#include "bgfxRenderer.h"

#include "Gwen/Utility.h"
#include "Gwen/Font.h"
#include "Gwen/Texture.h"
#include "Gwen/WindowProvider.h"
#include <bx/file.h>

#ifndef ENTRY_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR
#	define ENTRY_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR 1
#endif // ENTRY_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR

// #include "entry/entry.h"
/*
#define D3DFVF_VERTEXFORMAT2D ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 )

struct FontData
{
	ID3DXFont*	pFont;
	int			iSpaceWidth;
};
*/

namespace Gwen
{
	namespace Renderer
	{
        
        long int fsize(FILE* _file)
        {
	        long int pos = ftell(_file);
	        fseek(_file, 0L, SEEK_END);
	        long int size = ftell(_file);
	        fseek(_file, pos, SEEK_SET);
	        return size;
        }

		#if ENTRY_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR
			bx::AllocatorI* getDefaultAllocator()
			{
		BX_PRAGMA_DIAGNOSTIC_PUSH();
		BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4459); // warning C4459: declaration of 's_allocator' hides global declaration
		BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wshadow");
				static bx::DefaultAllocator s_allocator;
				return &s_allocator;
		BX_PRAGMA_DIAGNOSTIC_POP();
			}
		#endif // ENTRY_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR

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

		bgfxRenderer::bgfxRenderer(int viewID, const char* shaderPath, const char* texturePath)
		{   
			m_viewID = viewID;
            m_shaderPath = shaderPath;
            m_texturePath = texturePath;

            m_verticesCount = 0;
            m_width = 1280;
	        m_height = 720;
            m_currentTexture.idx = BGFX_INVALID_HANDLE;// bgfx::kInvalidHandle;           
			g_allocator = getDefaultAllocator();
			s_fileReader = BX_NEW(g_allocator, bx::FileReader);
			s_fileWriter = BX_NEW(g_allocator, bx::FileWriter);

            // Load vertex shader.
            const bgfx::Memory* mem;
	        mem = loadShaderMem("vs_gwen_flat");
	        bgfx::ShaderHandle vs_gwen_flat = bgfx::createShader(mem);

			mem = loadShaderMem("vs_gwen_textured");
	        bgfx::ShaderHandle vs_gwen_textured = bgfx::createShader(mem);

	        // Load fragment shader.
	        mem = loadShaderMem("fs_gwen_flat");
	        bgfx::ShaderHandle fs_gwen_flat = bgfx::createShader(mem);

            mem = loadShaderMem("fs_gwen_textured");
	        bgfx::ShaderHandle fs_gwen_textured = bgfx::createShader(mem);

			//bgfx::UniformHandle u_color0 = bgfx::createUniform("u_color0", bgfx::UniformType::Uniform4fv);
            
            // Create program from shaders.
        	m_flatProgram = bgfx::createProgram(vs_gwen_flat, fs_gwen_flat);			
            m_texturedProgram = bgfx::createProgram(vs_gwen_textured, fs_gwen_textured);
            
            // We can destroy vertex and fragment shader here since
	        // their reference is kept inside bgfx after calling createProgram.
	        // Vertex and fragment shader will be destroyed once program is
	        // destroyed.
            bgfx::destroy(vs_gwen_flat);
			bgfx::destroy(vs_gwen_textured);
	        bgfx::destroy(fs_gwen_flat);
            bgfx::destroy(fs_gwen_textured);

            //declare vertex format
            m_posUVColorDecl.begin();
	        m_posUVColorDecl.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float);
            m_posUVColorDecl.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
	        m_posUVColorDecl.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true);
	        m_posUVColorDecl.end();

			m_posDecl.begin();
	        m_posDecl.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float);
			m_posDecl.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true);
	        m_posDecl.end();
		}

		bgfxRenderer::~bgfxRenderer()
		{
			bx::deleteObject(g_allocator, s_fileReader);
			s_fileReader = NULL;

			bx::deleteObject(g_allocator, s_fileWriter);
			s_fileWriter = NULL;
            bgfx::destroy(m_flatProgram);
            bgfx::destroy(m_texturedProgram);
		}

        void bgfxRenderer::Init()
        {
        }

		void bgfxRenderer::Begin()
		{
            bgfx::setViewRect(0, 0, 0, m_width, m_height);         
			bgfx::setState( 
				BGFX_STATE_DEFAULT // BGFX_STATE_RGB_WRITE
				|BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
				//|BGFX_STATE_ALPHA_TEST
				// |BGFX_STATE_DEPTH_WRITE
				|BGFX_STATE_DEPTH_TEST_LESS
				);

			//TODO clarify depth sorting in regard to gwen
			//seems to be decreasing.
			m_depth = 5000;

            //better kept outside ...
            //float view[16];
		    //float proj[16];
		    // mtxLookAt(view, eye, at);
		    //mtxProj(proj, 60.0f, 16.0f/9.0f, 0.1f, 100.0f);

		    // Set view and projection matrix for view 0.
		    //bgfx::setViewTransform(0, view, proj);

		}

		void bgfxRenderer::End()
		{
			// Submit primitive for rendering to view 0.
			Flush();
		}

        void bgfxRenderer::SetDrawColor(Gwen::Color color)
		{
           memcpy(&m_color, &color, 4);			
		}

		void bgfxRenderer::DrawFilledRect( Gwen::Rect rect )
		{
			if ( m_currentTexture.idx != bgfx::kInvalidHandle )
			{
				Flush();
				m_currentTexture.idx = bgfx::kInvalidHandle;
			}

			//m_color = rand() | 0x44000044;


			Translate( rect );

			AddVert( rect.x, rect.y );
			AddVert( rect.x+rect.w, rect.y );
			AddVert( rect.x, rect.y + rect.h );

			AddVert( rect.x+rect.w, rect.y );
			AddVert( rect.x+rect.w, rect.y+rect.h );
			AddVert( rect.x, rect.y + rect.h );
        }

        void bgfxRenderer::StartClip()
		{
            //state change
			Flush();
			/*
            const Gwen::Rect& rect = ClipRegion();
            uint16_t x = rect.x * Scale();
            uint16_t y = rect.y * Scale();
            uint16_t w = rect.w * Scale();
            uint16_t h = rect.h * Scale();
            bgfx::setViewRect(m_viewID, x, y, w, h);
			*/
		}

		void bgfxRenderer::EndClip()
		{

            //state change
			Flush();
            //bgfx::setViewRect(m_viewID, 0, 0, m_width, m_height);
		}

        void bgfxRenderer::LoadTexture( Gwen::Texture* pTexture )
		{
			return;
            //const wchar_t *wFileName = pTexture->name.GetUnicode().c_str();
            const char* fileName = pTexture->name.c_str();

            char filePath[512];
	        strcpy(filePath, m_texturePath);
	        strcat(filePath, fileName);
            
            const bgfx::Memory* mem;
            mem = load(filePath);

            //TODO support non DDS files

            bgfx::TextureInfo info;
			// void* createTexture(TextureHandle /*_handle*/, const Memory* /*_mem*/, uint64_t /*_flags*/, uint8_t /*_skip*/) override
			// TextureHandle bgfx::createTexture(const Memory *_mem, uint64_t _flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, uint8_t _skip = 0, TextureInfo *_info = NULL)¶
	        // rci->createTexture(_handle, mem, _flags, 0);
			/*
			TextureHandle createTexture(
				const Memory* _mem
				, uint64_t _flags = BGFX_TEXTURE_NONE|BGFX_SAMPLER_NONE
				, uint8_t _skip = 0
				, TextureInfo* _info = NULL
				);
			*/
			// bgfx::TextureHandle handle = bgfx::createTexture(mem, 0, &info);
			bgfx::TextureHandle handle = bgfx::createTexture(mem, 0, 0, &info);
            memcpy(&pTexture->data, &handle, 2);

            pTexture->width = info.width;
            pTexture->height = info.height;     
		}

		void bgfxRenderer::FreeTexture( Gwen::Texture* pTexture )
		{
            bgfx::TextureHandle handle;
            memcpy(&handle, &pTexture->data, 2);
            bgfx::destroy(handle);
            pTexture->data = NULL;
		}

		void bgfxRenderer::DrawTexturedRect( Gwen::Texture* pTexture, Gwen::Rect rect, float u1, float v1, float u2, float v2 )
		{

            if ( m_currentTexture.idx != bgfx::kInvalidHandle )
			{
				Flush();
				m_currentTexture.idx = bgfx::kInvalidHandle;
			}

            bgfx::TextureHandle handle;
            memcpy(&handle, &pTexture->data, 2);

            // Missing image, not loaded properly?
            if(handle.idx == bgfx::kInvalidHandle)
            {
                return DrawMissingImage( rect );
            }
             
			Translate( rect );
            
            if ( m_currentTexture.idx != handle.idx )
			{
				Flush();
                m_currentTexture.idx = handle.idx;
			}

			//m_color = rand();// | 0x44000044;

			AddVert( rect.x, rect.y,			u1, v1 );
			AddVert( rect.x+rect.w, rect.y,		u2, v1 );
			AddVert( rect.x, rect.y + rect.h,	u1, v2 );

			AddVert( rect.x+rect.w, rect.y,		u2, v1 );
			AddVert( rect.x+rect.w, rect.y+rect.h, u2, v2 );
			AddVert( rect.x, rect.y + rect.h, u1, v2 );
		}

        //void bgfxRenderer::DrawMissingImage( Gwen::Rect pTargetRect )
        //{
        //}

        Gwen::Color bgfxRenderer::PixelColour( Gwen::Texture* pTexture, unsigned int x, unsigned int y, const Gwen::Color& col_default )
		{
            //TODO implement
            /*
			IDirect3DTexture9* pImage = (IDirect3DTexture9*) pTexture->data;
			if ( !pImage ) return col_default;

			IDirect3DSurface9* pSurface = NULL;

			if ( pImage->GetSurfaceLevel( 0, &pSurface ) != S_OK ) return col_default;
			if ( !pSurface ) return col_default;

			D3DLOCKED_RECT lockedRect;
			pSurface->LockRect( &lockedRect, NULL, D3DLOCK_READONLY );
			DWORD* pixels = (DWORD*)lockedRect.pBits;
			D3DXCOLOR color = pixels[lockedRect.Pitch / sizeof(DWORD) * y + x];
			pSurface->UnlockRect();

			pSurface->Release();

			return Gwen::Color( color.r*255, color.g*255, color.b*255, color.a*255 );
            */
             return Gwen::Color();
		}

		void bgfxRenderer::LoadFont( Gwen::Font* font )
		{
            /*
			m_FontList.push_back( font );
			// Scale the font according to canvas
			font->realsize = font->size * Scale();

			D3DXFONT_DESC fd;

			memset( &fd, 0, sizeof(fd) );

			wcscpy_s( fd.FaceName, LF_FACESIZE, font->facename.c_str() );

			fd.Width = 0;
			fd.MipLevels = 1;
			fd.CharSet = DEFAULT_CHARSET;
			fd.Height = font->realsize * -1.0f;
			fd.OutputPrecision = OUT_DEFAULT_PRECIS;
			fd.Italic = 0;
			fd.Weight = font->bold ? FW_BOLD : FW_NORMAL;
#ifdef CLEARTYPE_QUALITY
			fd.Quality = font->realsize < 14 ? DEFAULT_QUALITY : CLEARTYPE_QUALITY;
#else 
			fd.Quality = PROOF_QUALITY;
#endif 
			fd.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

			LPD3DXFONT pD3DXFont;
			HRESULT hr = D3DXCreateFontIndirect( m_pDevice, &fd, &pD3DXFont );

			FontData*	pFontData = new FontData();
			pFontData->pFont = pD3DXFont;

			// ID3DXFont doesn't measure trailing spaces, so we measure the width of a space here and store it
			// in the font data - then we can add it to the width when we measure text with trailing spaces.
			{
				RECT rctA = {0,0,0,0};
				pFontData->pFont->DrawTextW( NULL, L"A", -1, &rctA, DT_CALCRECT | DT_LEFT | DT_TOP | DT_SINGLELINE, 0 );

				RECT rctSpc = {0,0,0,0};
				pFontData->pFont->DrawTextW( NULL, L"A A", -1, &rctSpc, DT_CALCRECT | DT_LEFT | DT_TOP | DT_SINGLELINE, 0 );

				pFontData->iSpaceWidth = rctSpc.right - rctA.right * 2;
			}

			font->data = pFontData;
            */
		}

		void bgfxRenderer::FreeFont( Gwen::Font* pFont )
		{
            /*
			m_FontList.remove( pFont );

			if ( !pFont->data ) return;

			FontData* pFontData = (FontData*) pFont->data;

			if ( pFontData->pFont )
			{
				pFontData->pFont->Release();
				pFontData->pFont = NULL;
			}

			delete pFontData;
			pFont->data = NULL;
            */

		}

		void bgfxRenderer::RenderText( Gwen::Font* pFont, Gwen::Point pos, const Gwen::UnicodeString& text )
		{
            /*
			Flush();

			// If the font doesn't exist, or the font size should be changed
			if ( !pFont->data || fabs( pFont->realsize - pFont->size * Scale() ) > 2 )
			{
				FreeFont( pFont );
				LoadFont( pFont );
			}

			FontData* pFontData = (FontData*) pFont->data;
			
			Translate( pos.x, pos.y );

			RECT ClipRect = { pos.x, pos.y, 0, 0 };
			pFontData->pFont->DrawTextW( NULL, text.c_str(), -1, &ClipRect, DT_LEFT | DT_TOP | DT_NOCLIP | DT_SINGLELINE, m_Color );
            */
		}

		Gwen::Point bgfxRenderer::MeasureText( Gwen::Font* pFont, const Gwen::UnicodeString& text )
		{
            /*
			// If the font doesn't exist, or the font size should be changed
			if ( !pFont->data || fabs( pFont->realsize - pFont->size * Scale() ) > 2 )
			{
				FreeFont( pFont );
				LoadFont( pFont );
			}

			FontData* pFontData = (FontData*) pFont->data;

			Gwen::Point size;

			if ( text.empty() )
			{
				RECT rct = {0,0,0,0};
				pFontData->pFont->DrawTextW( NULL, L"W", -1, &rct, DT_CALCRECT, 0 );

				return Gwen::Point( 0, rct.bottom );
			}

			RECT rct = {0,0,0,0};
			pFontData->pFont->DrawTextW( NULL, text.c_str(), -1, &rct, DT_CALCRECT | DT_LEFT | DT_TOP | DT_SINGLELINE, 0 );

			for (int i=text.length()-1; i>=0 && text[i] == L' '; i-- )
			{
				rct.right += pFontData->iSpaceWidth;
			}

			return Gwen::Point( rct.right / Scale(), rct.bottom / Scale() );
            */
            return Gwen::Point();
		}     

//**************************************************************************************************
//**************************************************************************************************
//**************************************************************************************************

        bool bgfxRenderer::InitializeContext( Gwen::WindowProvider* pWindow )
		{
            uint32_t width = 1280;
	        uint32_t height = 720;
	        uint32_t debug = BGFX_DEBUG_TEXT;

	        bgfx::init();
	        bgfx::reset(width, height);

	        // Enable debug text.
	        bgfx::setDebug(debug);

	        // Set view 0 clear state.
	        bgfx::setViewClear(0
		    //     , BGFX_CLEAR_COLOR_BIT|BGFX_CLEAR_DEPTH_BIT
			    , BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
		        , 0x303030ff
		        , 1.0f
		        , 0
		        );
            /*

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

	        case bgfx::RendererType::OpenGLES2:
	        case bgfx::RendererType::OpenGLES3:
		        s_shaderPath = "shaders/gles/";
		        break;
	        }*/

			return true;
		}

		bool bgfxRenderer::ShutdownContext( Gwen::WindowProvider* pWindow )
		{
			// Shutdown bgfx.
	        bgfx::shutdown();
			return true;
		}

		bool bgfxRenderer::PresentContext( Gwen::WindowProvider* pWindow )
		{
			//m_pDevice->Present( NULL, NULL, NULL, NULL );
			return true;
		}

		bool bgfxRenderer::ResizedContext( Gwen::WindowProvider* pWindow, int w, int h )
		{
            
            // Force setting the current texture again
            m_currentTexture.idx = bgfx::kInvalidHandle;

            // Free any unmanaged resources (fonts) ??
			//Release();
           
	        bgfx::reset((uint32_t ) w, (uint32_t ) h, BGFX_RESET_NONE);
            bgfx::setViewRect(0, 0, 0, w, h);
			return true;
		}

		bool bgfxRenderer::BeginContext( Gwen::WindowProvider* pWindow )
		{	
            // Set view 0 default viewport.
		    //bgfx::setViewRect(0, 0, 0, width, height);

		    // This dummy draw call is here to make sure that view 0 is cleared
		    // if no other draw calls are submitted to view 0.
		    // bgfx::submit(0);
			/*
			bgfx::ProgramHandle m_flatProgram;
            bgfx::ProgramHandle m_texturedProgram;
			*/
			// ??? is this correct?
			bgfx::submit(0, m_flatProgram);
			return true;
		}

		bool bgfxRenderer::EndContext( Gwen::WindowProvider* pWindow )
		{
			// Advance to next frame. Rendering thread will be kicked to 
		    // process submitted rendering primitives.
		    bgfx::frame();
			return true;
		}

//**************************************************************************************************
//**************************************************************************************************
//**************************************************************************************************

        void bgfxRenderer::Flush()
		{
			if ( m_verticesCount > 0 )
			{
                if (bgfx::getAvailTransientVertexBuffer(m_verticesCount, m_posDecl))
	            {
                    bgfx::TransientVertexBuffer tvb;
		            bgfx::allocTransientVertexBuffer(&tvb, m_verticesCount, m_posDecl);

					memcpy(tvb.data, m_vertices, m_verticesCount * sizeof(PosVF));
                    if(m_currentTexture.idx != bgfx::kInvalidHandle){
                        //bgfx::createProgram(m_texturedProgram);
						// XXX This needs a review
						m_flatProgram = bgfx::createProgram(loadShader("vs_gwen_flat"), true);
                    }else{
						// XXX This needs a review
						m_flatProgram = bgfx::createProgram(loadShader("vs_gwen_flat"), true);
                        // bgfx::createProgram(m_flatProgram);
                    }
				    // Set vertex and index buffer.                    
				    // bgfx::setVertexBuffer(&tvb, m_verticesCount);
					// XXX Need to review what the right method signature is
					bgfx::setVertexBuffer(0, &tvb, 0, m_verticesCount);
			
				    //bgfx::setIndexBuffer(ibh);
                    // bgfx::submit((uint8_t) m_viewID, m_depth--);
					// void bgfx::submit(ViewId _id, ProgramHandle _program, uint32_t _depth = 0, uint8_t _flags = BGFX_DISCARD_ALL)¶
					bgfx::submit(m_viewID, m_flatProgram, m_depth--);
	            }else
                {
                    //assert(false
                    int break_point = 0;
                }

				m_verticesCount = 0;
			}
		}
        
		void bgfxRenderer::AddVert( int x, int y )
		{    
			if ( m_verticesCount >= MAX_VERTICES-1 )
			{
				Flush();
			}

			m_vertices[ m_verticesCount ].x = (float)x;
			m_vertices[ m_verticesCount ].y = (float)y;
			m_vertices[ m_verticesCount ].color = m_color;

			m_verticesCount++;    
		}

		void bgfxRenderer::AddVert( int x, int y, float u, float v )
		{
			if ( m_verticesCount >= MAX_VERTICES-1 )
			{
				Flush();
			}

			m_vertices[ m_verticesCount ].x = (float)x;
			m_vertices[ m_verticesCount ].y = (float)y;
            //m_vertices[ m_verticesCount ].x = -0.5f + (float)x;
			//m_vertices[ m_verticesCount ].y = -0.5f + (float)y;
			//m_vertices[ m_verticesCount ].u = u;
			//m_vertices[ m_verticesCount ].v = v;

			m_vertices[ m_verticesCount ].color = m_color;

			m_verticesCount++;
		}   

        const bgfx::Memory* bgfxRenderer::loadShaderMem(const char* _name)
        {
             char filePath[512];
             strcpy(filePath, m_shaderPath);
             strcat(filePath, _name);
			 strcat(filePath, ".bin");
	        return load(filePath);
        }


		const bgfx::Memory* bgfxRenderer::loadMem(bx::FileReaderI* _reader, const char* _filePath)
		{
			if (bx::open(_reader, _filePath) )
			{
				uint32_t size = (uint32_t)bx::getSize(_reader);
				const bgfx::Memory* mem = bgfx::alloc(size+1);
				bx::read(_reader, mem->data, size, bx::ErrorAssert{});
				bx::close(_reader);
				mem->data[mem->size-1] = '\0';
				return mem;
			}

			// XXX? where is this DBG
			// DBG("Failed to load %s.", _filePath);
			return NULL;
		}

		// These are borrowed from bgfx examples common bgfx_utils.cpp
		const void* bgfxRenderer::loadMem(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const char* _filePath, uint32_t* _size)
		{
			if (bx::open(_reader, _filePath) )
			{
				uint32_t size = (uint32_t)bx::getSize(_reader);
				void* data = bx::alloc(_allocator, size);
				bx::read(_reader, data, size, bx::ErrorAssert{});
				bx::close(_reader);

				if (NULL != _size)
				{
					*_size = size;
				}
				return data;
			}

			// XXX where is DBG from
			// DBG("Failed to load %s.", _filePath);
			return NULL;
		}


		// These are borrowed from bgfx examples common bgfx_utils.cpp
		const bgfx::ShaderHandle bgfxRenderer::loadShader(bx::FileReaderI* _reader, const char* _name)
		{
			char filePath[512];

			const char* shaderPath = "???";

			switch (bgfx::getRendererType() )
			{
			case bgfx::RendererType::Noop:
			case bgfx::RendererType::Direct3D9:  shaderPath = "shaders/dx9/";   break;
			case bgfx::RendererType::Direct3D11:
			case bgfx::RendererType::Direct3D12: shaderPath = "shaders/dx11/";  break;
			case bgfx::RendererType::Agc:
			case bgfx::RendererType::Gnm:        shaderPath = "shaders/pssl/";  break;
			case bgfx::RendererType::Metal:      shaderPath = "shaders/metal/"; break;
			case bgfx::RendererType::Nvn:        shaderPath = "shaders/nvn/";   break;
			case bgfx::RendererType::OpenGL:     shaderPath = "shaders/glsl/";  break;
			case bgfx::RendererType::OpenGLES:   shaderPath = "shaders/essl/";  break;
			case bgfx::RendererType::Vulkan:     shaderPath = "shaders/spirv/"; break;
			case bgfx::RendererType::WebGPU:     shaderPath = "shaders/spirv/"; break;

			case bgfx::RendererType::Count:
				BX_ASSERT(false, "You should not be here!");
				break;
			}

			bx::strCopy(filePath, BX_COUNTOF(filePath), shaderPath);
			bx::strCat(filePath, BX_COUNTOF(filePath), _name);
			bx::strCat(filePath, BX_COUNTOF(filePath), ".bin");

			bgfx::ShaderHandle handle = bgfx::createShader(loadMem(_reader, filePath) );
			bgfx::setName(handle, _name);

			return handle;
		}

		// These are borrowed from bgfx examples common bgfx_utils.cpp
		const bgfx::ShaderHandle bgfxRenderer::loadShader(const char* _name)
		{
			return loadShader(s_fileReader, _name);
		}

        const bgfx::Memory* bgfxRenderer::loadTexture(const char* _name)
        {
            char filePath[512];
	        strcpy(filePath, "textures/");
	        strcat(filePath, _name);
	        return load(filePath);
        }
        
		//void bgfxRenderer::Release()
		//{
            /*
			Font::List::iterator it = m_FontList.begin();

			while ( it != m_FontList.end() )
			{
				FreeFont( *it );
				it = m_FontList.begin();
			}
            */
		//}

	}
}
