#include <imgui.h>
#ifdef USE_FREETYPE
#include <imgui_freetype.h>
#endif

#include "ImguiManager.h"

#include <OgreMaterialManager.h>
#include <OgreMesh.h>
#include <OgreMeshManager.h>
#include <OgreSubMesh.h>
#include <OgreTextureGpu.h>
#include <OgreTextureGpuManager.h>
#include <OgreString.h>
#include <OgreStringConverter.h>
#include <OgreViewport.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreHighLevelGpuProgram.h>
#include <OgreUnifiedHighLevelGpuProgram.h>
#include <OgreRoot.h>
#include <OgreTechnique.h>
#include <OgreTextureUnitState.h>
#include <OgreViewport.h>
#include "OgreHardwareBufferManager.h"
#include "OgreHlmsDatablock.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreStagingTexture.h"
#include "OgreTextureBox.h"
#include "OgreLogManager.h"
#include "OgreWindow.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"
#include "Compositor/OgreCompositorManager2.h"
#include <Compositor/OgreCompositorNodeDef.h>
#include <Compositor/Pass/OgreCompositorPassDef.h>
#include <Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h>
#include <Compositor/OgreCompositorWorkspaceDef.h>
#ifdef OGRE_BUILD_COMPONENT_OVERLAY
#include <Overlay/OgreFontManager.h>
#endif

using namespace Ogre;

// map sdl2 mouse buttons to imgui
static int sdl2imgui(int b)
{
    switch(b) {
    case 2:
        return 2;
    case 3:
        return 1;
    default:
        return b - 1;
    }
}

struct ImguiInputListener : public InputListener
{
    ImguiInputListener()
    {
        ImGuiIO& io = ImGui::GetIO();
        // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
        io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
        io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
        io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
        io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
        io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
        io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
		io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
		io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
        io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
        io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
        io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
        io.KeyMap[ImGuiKey_A] = 'a';
        io.KeyMap[ImGuiKey_C] = 'c';
        io.KeyMap[ImGuiKey_V] = 'v';
        io.KeyMap[ImGuiKey_X] = 'x';
        io.KeyMap[ImGuiKey_Y] = 'y';
        io.KeyMap[ImGuiKey_Z] = 'z';
    }

    void mouseMoved( const SDL_Event&arg )
    {

        ImGuiIO& io = ImGui::GetIO();
		if(arg.type == SDL_MOUSEWHEEL)
			io.MouseWheel += Ogre::Math::Sign(arg.wheel.y);
		else
		{
			io.MousePos.x = arg.motion.x;
			io.MousePos.y = arg.motion.y;
		}

    }

    void mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id)
    {
        ImGuiIO& io = ImGui::GetIO();
        int b = sdl2imgui(arg.button);
        if(b<5)
        {
            io.MouseDown[b] = true;
        }
    }
    void mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id)
    {
        ImGuiIO& io = ImGui::GetIO();
        int b = sdl2imgui(arg.button);
        if(b<5)
        {
            io.MouseDown[b] = false;
        }
    }
    void keyPressed( const SDL_KeyboardEvent&arg )
    {

        ImGuiIO& io = ImGui::GetIO();
        
        // ignore
        if(arg.keysym.sym == SDLK_LSHIFT) return;

        io.KeyCtrl = arg.keysym.mod & KMOD_CTRL;
        io.KeyShift = arg.keysym.mod & KMOD_SHIFT;
		io.KeyAlt = arg.keysym.mod & KMOD_ALT;
		io.KeySuper = false;
        int key = arg.keysym.scancode;

      if(key > 0 && key < 512)
		io.KeysDown[key] = true;
    }
	void keyReleased( const SDL_KeyboardEvent &arg )
    {
        int key = (arg.keysym.scancode);
        if(key < 0 || key >= 512)
            return;

        ImGuiIO& io = ImGui::GetIO();

        io.KeyCtrl = arg.keysym.mod & KMOD_CTRL;
        io.KeyShift = arg.keysym.mod & KMOD_SHIFT;
		io.KeyAlt = arg.keysym.mod & KMOD_ALT;
		io.KeySuper = false;
        io.KeysDown[key] = false;
    }
	void textInput(const SDL_TextInputEvent&arg)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.AddInputCharactersUTF8(arg.text);
	}
};

template<> ImguiManager* Singleton<ImguiManager>::msSingleton = 0;

void ImguiManager::createSingleton()
{
    if(!msSingleton)
    {
        msSingleton = new ImguiManager();
    }
}
ImguiManager* ImguiManager::getSingletonPtr(void)
{
    createSingleton();
    return msSingleton;
}
ImguiManager& ImguiManager::getSingleton(void)
{  
    createSingleton();
    return ( *msSingleton );  
}

ImguiManager::ImguiManager()
:mSceneMgr(0)
{
    ImGui::CreateContext();
	mSceneMgr = NULL;
	mScreenWidth = 0;
	mScreenHeight = 0;
}
ImguiManager::~ImguiManager()
{
    ImGui::DestroyContext();
	Ogre::Root::getSingletonPtr()->removeFrameListener(this);

	Ogre::HlmsManager *hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
	Ogre::HlmsUnlit *hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));
	Ogre::TextureGpuManager *textureMgr = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
	
	if(mFontTex != NULL)
		textureMgr->destroyTexture(mFontTex);
	mFontTex = NULL;
	for (size_t i = 0; i < MAX_NUM_RENDERABLES; i++)
	{
		if (mRenderables[i] != NULL)
		{
			mRenderables[i]->_setNullDatablock();
			if (mRenderables[i]->mUnlitDatablock != NULL)
				hlmsUnlit->destroyDatablock(mRenderables[i]->mUnlitDatablock->getName());
		}
	}
}

void ImguiManager::init(Ogre::Window* win, Ogre::SceneManager* mgr)
{
	mSceneMgr = mgr;
	mScreenWidth = win->getWidth();
	mScreenHeight = win->getHeight();

	Ogre::Root::getSingletonPtr()->addFrameListener(this);
	mSceneMgr->addRenderQueueListener(this);

	createFontTexture();
	createMaterial();
	
	SceneNode* dummyNode = OGRE_NEW SceneNode(0, 0, new NodeMemoryManager(), 0);
	dummyNode->_getFullTransformUpdated();
	for (size_t i = 0; i < MAX_NUM_RENDERABLES; i++)
	{
		IdType oid = Ogre::Id::generateNewId<Ogre::MovableObject>();
		mRenderables[i] = new ImguiManager::ImGUIRenderable(oid, &mSceneMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), mSceneMgr, 224U);
		mRenderables[i]->setUseIdentityProjection(true);
		mRenderables[i]->setUseIdentityView(true);
		mRenderables[i]->setVisibilityFlags((1u<<i));
		dummyNode->attachObject(mRenderables[i]);
	}
}

InputListener* ImguiManager::getInputListener()
{
    static ImguiInputListener listener;
    return &listener;
}
Ogre::RenderPassDescriptor * ImguiManager::renderPassDesc = NULL;
//-----------------------------------------------------------------------------------
bool ImguiManager::frameStarted(const FrameEvent& evt)
{
	if (mScreenWidth > 0)
	{
		Ogre::ImguiManager::getSingleton().newFrame(
			evt.timeSinceLastFrame,
			Ogre::Rect(0, 0, mScreenWidth, mScreenHeight));
		ImGui::ShowDemoWindow();
		render();
	}
	return true;
}
//-----------------------------------------------------------------------------------
void ImguiManager::render()
{
	Ogre::RenderSystem* renderSys = Ogre::Root::getSingletonPtr()->getRenderSystem();
	if (mFrameEnded)
	{
		return;
	}

	mFrameEnded = true;
	ImGuiIO& io = ImGui::GetIO();

	// Instruct ImGui to Render() and process the resulting CmdList-s
	/// Adopted from https://bitbucket.org/ChaosCreator/imgui-ogre2.1-binding
	/// ... Commentary on OGRE forums: http://www.ogre3d.org/forums/viewtopic.php?f=5&t=89081#p531059
	
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImVector<ImDrawVert> vtxBuf = ImVector<ImDrawVert>();
	ImVector<ImDrawIdx> idxBuf = ImVector<ImDrawIdx>();

	Ogre::CompositorManager2 *compositorManager = Root::getSingletonPtr()->getCompositorManager2();
	Ogre::CompositorNodeDef *nodedef = compositorManager->getNodeDefinitionNonConst("TestMaskWorkspace_Node");
	CompositorTargetDef* target = nodedef->getTargetPass(0);
	Ogre::CompositorPassDefVec vec = target->getCompositorPassesNonConst();

	uint32 rend_offset = 0;
	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		const ImDrawList* draw_list = draw_data->CmdLists[i];
		uint32 startIdx = 0;
		uint32 startVtx = 0;
		for (int j = 0; j < draw_list->CmdBuffer.Size; ++j)
		{
			const ImDrawCmd *drawCmd = &draw_list->CmdBuffer[j];
			Ogre::TextureGpuManager *textureMgr = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
			TextureGpu* tex = NULL;
			if (drawCmd->TextureId != 0)
				tex = (TextureGpu*)drawCmd->TextureId;
			else
				tex = mFontTex;

			startVtx = std::min(draw_list->IdxBuffer[startIdx], draw_list->IdxBuffer[startIdx+1]);
			for (uint32 k = 0; k < (drawCmd->ElemCount); k++)
			{
				idxBuf.push_back(draw_list->IdxBuffer[startIdx+k]-startVtx);
				for (int l = vtxBuf.size(); l <= idxBuf.back(); l++)
				{
					vtxBuf.push_back(draw_list->VtxBuffer[l+startVtx]);
					vtxBuf.back().pos.x = (vtxBuf.back().pos.x / (Real)mScreenWidth)*2.0f - 1.0f;
					vtxBuf.back().pos.y = -((vtxBuf.back().pos.y / (Real)mScreenHeight)*2.0f - 1.0f);
				}
			}
			startIdx += drawCmd->ElemCount;

			uint32 rend_idx = (rend_offset + j) % MAX_NUM_RENDERABLES;
			uint32 pass_idx = 2 + rend_idx;

			vec[pass_idx]->mVpRect[0].mVpLeft = 0;
			vec[pass_idx]->mVpRect[0].mVpTop = 0;
			vec[pass_idx]->mVpRect[0].mVpWidth = 1;
			vec[pass_idx]->mVpRect[0].mVpHeight = 1;
			vec[pass_idx]->mVpRect[0].mVpScissorLeft = ((Real)drawCmd->ClipRect.x / (Real)mScreenWidth);
			vec[pass_idx]->mVpRect[0].mVpScissorTop = ((Real)(drawCmd->ClipRect.y) / (Real)mScreenHeight);
			vec[pass_idx]->mVpRect[0].mVpScissorWidth = ((Real)(drawCmd->ClipRect.z - drawCmd->ClipRect.x) / (Real)mScreenWidth);
			vec[pass_idx]->mVpRect[0].mVpScissorHeight = ((Real)(drawCmd->ClipRect.w - drawCmd->ClipRect.y) / (Real)mScreenHeight);
			
			mRenderables[rend_idx]->updateVertexData(vtxBuf, idxBuf);
			
			Ogre::HlmsManager *hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
			Ogre::HlmsUnlit *hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

			Ogre::String name = "imgui_";
			name.append(StringConverter::toString(rend_idx));
			HlmsUnlitDatablock* datablock =(HlmsUnlitDatablock*) hlmsUnlit->getDatablock(name);
			HlmsSamplerblock sb;
			sb.setFiltering(Ogre::TFO_TRILINEAR);
			datablock->setTexture(0, tex, &sb);
			mRenderables[rend_idx]->setDatablock(datablock);
			mRenderables[rend_idx]->mUnlitDatablock = datablock;
			mRenderables[rend_idx]->setVisible(true);
			idxBuf.clear();
			vtxBuf.clear();
			mRenderables[rend_idx]->mInitialized = true;
		}
		rend_offset += draw_list->CmdBuffer.Size;
	}
	for (int l = rend_offset; l < MAX_NUM_RENDERABLES; l++)
	{
		if(mRenderables[l]->mInitialized)
			mRenderables[l]->setVisible(false);
	}
}
//-----------------------------------------------------------------------------------
void ImguiManager::createMaterial()
{
	HlmsMacroblock mb;
	mb.mCullMode = CULL_NONE;
	mb.mDepthFunc = Ogre::CMPF_ALWAYS_PASS;
	mb.mScissorTestEnabled = true;
	HlmsBlendblock bb;
	bb.mIsTransparent = true;
	bb.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA);
	bb.mSeparateBlend = true;
	bb.mSourceBlendFactor = Ogre::SBF_SOURCE_ALPHA;
	bb.mDestBlendFactor = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;
	bb.mSourceBlendFactorAlpha = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;
	bb.mDestBlendFactorAlpha = Ogre::SBF_ZERO;
	bb.mBlendOperation = Ogre::SBO_ADD;
	bb.mBlendOperationAlpha = Ogre::SBO_ADD;
	
	for (int i = 0; i < MAX_NUM_RENDERABLES; i++)
	{
		Ogre::HlmsManager *hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
		Ogre::HlmsUnlit *hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

		Ogre::String datablockName = "imgui_";
		datablockName.append(StringConverter::toString(i));
		Ogre::HlmsUnlitDatablock *datablock = static_cast<Ogre::HlmsUnlitDatablock*>(
			hlmsUnlit->createDatablock(datablockName,
				datablockName,
				mb,
				bb,
				Ogre::HlmsParamVec()));
	}
}

ImFont* ImguiManager::addFont(const String& name, const String& group)
{
#ifdef OGRE_BUILD_COMPONENT_OVERLAY
    FontPtr font = FontManager::getSingleton().getByName(name, group);
    OgreAssert(font, "font does not exist");
    OgreAssert(font->getType() == FT_TRUETYPE, "font must be of FT_TRUETYPE");
    DataStreamPtr dataStreamPtr =
        ResourceGroupManager::getSingleton().openResource(font->getSource(), font->getGroup());
    MemoryDataStream ttfchunk(dataStreamPtr, false); // transfer ownership to imgui

    // convert codepoint ranges for imgui
    CodePointRange cprange;
    for(const auto& r : font->getCodePointRangeList())
    {
        cprange.push_back(r.first);
        cprange.push_back(r.second);
    }

    ImGuiIO& io = ImGui::GetIO();
    const ImWchar* cprangePtr = io.Fonts->GetGlyphRangesDefault();
    if(!cprange.empty())
    {
        cprange.push_back(0); // terminate
        mCodePointRanges.push_back(cprange);
        // ptr must persist until createFontTexture
        cprangePtr = mCodePointRanges.back().data();
    }

    ImFontConfig cfg;
    strncpy(cfg.Name, name.c_str(), 40);
    return io.Fonts->AddFontFromMemoryTTF(ttfchunk.getPtr(), ttfchunk.size(),
                                          font->getTrueTypeSize(), &cfg, cprangePtr);
#else
    OGRE_EXCEPT(Exception::ERR_INVALID_CALL, "Ogre Overlay Component required");
    return NULL;
#endif
}

void ImguiManager::createFontTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    if(io.Fonts->Fonts.empty())
        io.Fonts->AddFontDefault();
#ifdef USE_FREETYPE
    ImGuiFreeType::BuildFontAtlas(io.Fonts, 0);
#endif

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	Ogre::TextureGpuManager *textureMgr = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
	mFontTex = textureMgr->createOrRetrieveTexture("ImguiFontTex", GpuPageOutStrategy::Discard, Ogre::TextureFlags::ManualTexture,
		Ogre::TextureTypes::Type2DArray,
		Ogre::BLANKSTRING, 0u);
	mFontTex->setPixelFormat(PFG_RGBA8_UNORM);
	mFontTex->setTextureType(TextureTypes::Type2DArray);
	mFontTex->setNumMipmaps(1u);
	mFontTex->setResolution(width, height);
	StagingTexture *stagingTexture = textureMgr->getStagingTexture(mFontTex->getWidth(),
		mFontTex->getHeight(),
		mFontTex->getDepth(),
		mFontTex->getNumSlices(),
		mFontTex->getPixelFormat());
	mFontTex->_transitionTo(GpuResidency::Resident, NULL);
	mFontTex->_setNextResidencyStatus(GpuResidency::Resident);

	stagingTexture->startMapRegion();
	TextureBox texBox = stagingTexture->mapRegion(mFontTex->getWidth(), mFontTex->getHeight(),
		mFontTex->getDepth(), mFontTex->getNumSlices(),
		mFontTex->getPixelFormat());
	const size_t bytesPerRow = mFontTex->_getSysRamCopyBytesPerRow(0);
	texBox.copyFrom(pixels, mFontTex->getWidth(), mFontTex->getHeight(), bytesPerRow);
	stagingTexture->stopMapRegion();
	stagingTexture->upload(texBox, mFontTex, 0, 0, 0, true);
	textureMgr->removeStagingTexture(stagingTexture);
	mFontTex->notifyDataIsReady();
    mCodePointRanges.clear(); 
}
void ImguiManager::newFrame(float deltaTime,const Ogre::Rect & windowRect)
{
    mFrameEnded=false;
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = std::max(deltaTime, 1e-4f); // see https://github.com/ocornut/imgui/commit/3c07ec6a6126fb6b98523a9685d1f0f78ca3c40c

     // Read keyboard modifiers inputs
    io.KeyAlt = false;// mKeyInput->isKeyDown(OIS::KC_LMENU);
    io.KeySuper = false;

    // Setup display size (every frame to accommodate for window resizing)
     io.DisplaySize = ImVec2((float)(windowRect.right - windowRect.left), (float)(windowRect.bottom - windowRect.top));
	 

    // Start the frame
    ImGui::NewFrame();
}

ImguiManager::ImGUIRenderable::ImGUIRenderable(IdType id, ObjectMemoryManager *objectMemoryManager,
	SceneManager* manager, uint8 renderQueueId) :
	MovableObject(id, objectMemoryManager,manager,renderQueueId)
{
    initImGUIRenderable();

    //By default we want ImGUIRenderables to still work in wireframe mode
    setPolygonModeOverrideable( false );
}
//-----------------------------------------------------------------------------------
void ImguiManager::ImGUIRenderable::initImGUIRenderable(void)
{
	mInitialized = false;
	mUnlitDatablock = NULL;
	mVertexElements.push_back(Ogre::VertexElement2(Ogre::VertexElementType::VET_FLOAT2, Ogre::VertexElementSemantic::VES_POSITION));
	mVertexElements.push_back(Ogre::VertexElement2(Ogre::VertexElementType::VET_FLOAT2, Ogre::VertexElementSemantic::VES_TEXTURE_COORDINATES));
	mVertexElements.push_back(Ogre::VertexElement2(Ogre::VertexElementType::VET_COLOUR, Ogre::VertexElementSemantic::VES_DIFFUSE));
}
//-----------------------------------------------------------------------------------
ImguiManager::ImGUIRenderable::~ImGUIRenderable()
{
    
}
void ImguiManager::ImGUIRenderable::getRenderOperation(v1::RenderOperation& op, bool casterPass)
{
	OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
		"ImGUIRenderable does not implement getRenderOperation."
		" You've put a v2 object in "
		"the wrong RenderQueue ID (which is set to be compatible with "
		"v1::Entity). Do not mix v2 and v1 objects",
		"ImGUIRenderable::getRenderOperation");
}
//-----------------------------------------------------------------------------------
void ImguiManager::ImGUIRenderable::updateVertexData(const ImVector<ImDrawVert>& vtxBuf, const ImVector<ImDrawIdx>& idxBuf)
{
	VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();
	if (!mVaoPerLod[0].empty())
	{		
		vaoManager->destroyVertexArrayObject(mVaoPerLod[0].back());
		mVaoPerLod[0].clear();
		mVaoPerLod[1].clear();
	}
	Ogre::IndexBufferPacked *indexBuffer = 0;

	try
	{
		indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_16BIT,
			idxBuf.size(),
			Ogre::BT_IMMUTABLE,
			idxBuf.Data, false);
	}
	catch (Ogre::Exception &e)
	{
		OGRE_FREE_SIMD(indexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
		indexBuffer = 0;
		throw e;
	}
	Ogre::VertexBufferPacked *vertexBuffer = 0;
	try
	{
		vertexBuffer = vaoManager->createVertexBuffer(mVertexElements, vtxBuf.size(),
			BT_IMMUTABLE,
			vtxBuf.Data, false);
	}
	catch (Ogre::Exception &e)
	{
		OGRE_FREE_SIMD(vertexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
		vertexBuffer = 0;
		throw e;
	}
	VertexBufferPackedVec vertexBuffers;
	vertexBuffers.push_back(vertexBuffer);
	Ogre::VertexArrayObject *vao = vaoManager->createVertexArrayObject(
		vertexBuffers, indexBuffer, OT_TRIANGLE_LIST);

	mVaoPerLod[0].push_back(vao);
	mVaoPerLod[1].push_back(vao);
	Ogre::HlmsManager *hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
	Ogre::HlmsUnlit *hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

	//mRenderables.push_back(this);
}
//-----------------------------------------------------------------------------------
const LightList& ImguiManager::ImGUIRenderable::getLights(void) const
{
    static const LightList l;
    return l;
}
const String& ImguiManager::ImGUIRenderable::getMovableType(void) const
{
	return ImguiMovableType;
}

void ImguiManager::renderQueueStarted(RenderQueue *rq, uint8 queueGroupId,
	const String& invocation, bool& skipThisInvocation)
{
	if (queueGroupId == 224U)
	{
		for (int i = 0; i < MAX_NUM_RENDERABLES; i++)
		{
			if ( mRenderables[i]->mInitialized && mRenderables[i]->isVisible())
				rq->addRenderableV2(0, queueGroupId, false, mRenderables[i], mRenderables[i]);
		}
		
	}
}

