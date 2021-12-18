#pragma once

#include <imgui/imgui.h>
#include <OgrePrerequisites.h>

#include <OgreFrameListener.h>
#include <OgreSingleton.h>
#include <OgreTextureGpu.h>
#include <OgreResourceGroupManager.h>
#include <OgreRenderable.h>
#include <OgreRenderOperation.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreMovableObject.h>
#include <OgreRenderQueueListener.h>
#include "SDL.h"

#define MAX_NUM_RENDERABLES 30

class InputListener
{
	public:
		//Receives SDL_MOUSEMOTION and SDL_MOUSEWHEEL events
		virtual void mouseMoved(const SDL_Event &arg) {}
		virtual void mousePressed(const SDL_MouseButtonEvent &arg, Ogre::uint8 id) {}
		virtual void mouseReleased(const SDL_MouseButtonEvent &arg, Ogre::uint8 id) {}
		virtual void textEditing(const SDL_TextEditingEvent& arg) {}
		virtual void textInput(const SDL_TextInputEvent& arg) {}
		virtual void keyPressed(const SDL_KeyboardEvent &arg) {}
		virtual void keyReleased(const SDL_KeyboardEvent &arg) {}
		virtual void joyButtonPressed(const SDL_JoyButtonEvent &evt, int button) {}
		virtual void joyButtonReleased(const SDL_JoyButtonEvent &evt, int button) {}
		virtual void joyAxisMoved(const SDL_JoyAxisEvent &arg, int axis) {}
		virtual void joyPovMoved(const SDL_JoyHatEvent &arg, int index) {}

};

namespace Ogre
{
    class SceneManager;

    class ImguiManager : public FrameListener, public Singleton<ImguiManager>, public Ogre::RenderQueueListener
    {
    public:
        static void createSingleton();

        ImguiManager();
        ~ImguiManager();

        /// add font from ogre .fontdef file
        /// must be called before init()
        ImFont* addFont(const String& name, const String& group = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        virtual void init(Window* win, SceneManager* mgr, void(*fn)(bool*));

        virtual void newFrame(float deltaTime,const Ogre::Rect & windowRect);

		virtual void render();

        //inherited from FrameListener
		virtual bool frameStarted(const FrameEvent& evt);

		/// @see RenderQueueListener
		virtual void renderQueueStarted(RenderQueue *rq, uint8 queueGroupId, const String& invocation, bool& skipThisInvocation);

		InputListener* getInputListener();
		void setDisplayFunction(void(*df)(bool*)) { mDisplayFunction = df; };

        static ImguiManager& getSingleton(void);
        static ImguiManager* getSingletonPtr(void);

    protected:

        class ImGUIRenderable : public MovableObject, public Renderable
        {
        protected:
            void initImGUIRenderable(void);

        public:
            ImGUIRenderable(IdType id, ObjectMemoryManager *objectMemoryManager,
				SceneManager* manager, uint8 renderQueueId);
            ~ImGUIRenderable();

            void updateVertexData(Ogre::VertexBufferPacked *vertexBuffer, const ImVector<ImDrawIdx>& idxBuf);
            Real getSquaredViewDepth(const Camera* cam) const   { (void)cam; return 0; }

            virtual const MaterialPtr& getMaterial(void) const { return mMaterial; }

            virtual void getWorldTransforms( Matrix4* xform ) const { *xform = mXform; }
			virtual void getRenderOperation(v1::RenderOperation& op, bool casterPass);
            virtual const LightList& getLights(void) const;
			virtual const String& getMovableType(void) const;

            MaterialPtr              mMaterial;
			HlmsUnlitDatablock		 *mUnlitDatablock;
            Matrix4                  mXform;
			bool					 mInitialized;
			VertexElement2Vec mVertexElements;

			const String ImguiMovableType = "IMGUI";
        };

        void createFontTexture();
        void createMaterial();

		SceneManager*				mSceneMgr;
		uint32 mScreenWidth, mScreenHeight;
        ImGUIRenderable             *mRenderables[MAX_NUM_RENDERABLES] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
        TextureGpu*                  mFontTex;
		void(*mDisplayFunction)(bool*);

        bool                        mFrameEnded;
		Ogre::VertexBufferPacked *vertexBuffer[MAX_NUM_RENDERABLES] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

        typedef std::vector<ImWchar> CodePointRange;
        std::vector<CodePointRange> mCodePointRanges;
    };
}
