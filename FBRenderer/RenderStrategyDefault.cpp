#include "stdafx.h"
#include "RenderStrategyDefault.h"
#include "Texture.h"
#include "GaussianDistribution.h"
#include "RenderEventMarker.h"
#include "RenderTarget.h"
#include "FBSceneManager/Scene.h"
#include "Renderer.h"
#include "RenderParam.h"
#include "IRenderable.h"
#include "DirectionalLight.h"
#include "GaussianDistribution.h"
#include "StarDef.h"
#include "RenderOptions.h"
#include "SystemTextures.h"
#include "FBStringLib/StringLib.h"
#include "FBSceneManager/SceneObjectType.h"
#include "FBSceneManager/Camera.h"
#include "FBMathLib/BoundingVolume.h"
using namespace fastbird;

static const int FB_NUM_BLOOM_TEXTURES = 3;
static const int FB_NUM_STAR_TEXTURES = 12;
static const int starGlareMaxPasses = 3;
static const int starGlareSamples = 8;
static const Real starGlareChromaticAberration = 0.5f;
static const Real starGlareInclination = HALF_PI;
static StarDef* starGlareDef = 0;

//IRenderStrategyPtr RenderStrategyDefault::Create(){
//	return IRenderStrategyPtr(FB_NEW(RenderStrategyDefault), [](RenderStrategyDefault* obj){FB_DELETE(obj); });
//}
IMPLEMENT_STATIC_CREATE(RenderStrategyDefault);

class RenderStrategyDefault::Impl{
public:
	Impl()
		:mGlowSet(false), mFrameLuminanceCalced(0), mLuminance(0.5f)
		, mRenderingFace(0)
	{

	}

	SceneWeakPtr mScene;
	RenderTargetWeakPtr mRenderTarget;
	Vec2I mSize; // Render target size.
	RenderTargetId mId;
	TexturePtr mDepthTarget;
	TexturePtr mGlowTarget;
	TexturePtr mGlowTexture[2];
	bool mGlowSet;
	CameraPtr mLightCamera;
	Vec2 mLightCamSize;
	TexturePtr mShadowMap;
	TexturePtr mCloudVolumeDepth;
	ShaderPtr mCloudDepthWriteShader;
	TexturePtr mGodRayTarget[2]; // half resolution; could be shared.
	TexturePtr mNoMSDepthStencil;
	TexturePtr mHDRTarget;
	TexturePtr mSmallSilouetteBuffer;
	TexturePtr mBigSilouetteBuffer;
	unsigned mFrameLuminanceCalced;
	Real mLuminance;
	GaussianDistPtr mGaussianDistBlendGlow;
	GaussianDistPtr mGaussianDistBloom;
	TexturePtr mBrightPassTexture;
	TexturePtr mStarSourceTex;
	TexturePtr mBloomSourceTex;
	TexturePtr mBloomTexture[FB_NUM_BLOOM_TEXTURES];
	TexturePtr mStarTextures[FB_NUM_STAR_TEXTURES];
	size_t mRenderingFace;

	//-------------------------------------------------------------------
	// IRenderStrategy
	//-------------------------------------------------------------------
	void SetScene(ScenePtr scene){
		mScene = scene;
	}

	void SetRenderTarget(RenderTargetPtr renderTarget){
		mRenderTarget = renderTarget;
		mSize = renderTarget->GetSize();
		mId = renderTarget->GetId();
	}	

	void Render(size_t face){		
		auto scene = mScene.lock();
		auto renderTarget = mRenderTarget.lock();
		auto renderer = Renderer::GetInstance();
		if (!scene || !renderTarget || !renderer)
			return;

		RenderEventMarker marker("Rendering with default strategy.");
		RenderParam renderParam;
		RenderParamOut renderParamOut;
		memset(&renderParam, 0, sizeof(RenderParam));
		memset(&renderParamOut, 0, sizeof(RenderParamOut));
		ClearSilouetteBuffer();
		DepthTexture(false);
		CloudVolumeTexture(false);
		ShadowMap(false);
		renderTarget->Bind(face);
		RenderParam param;
		memset(&param, 0, sizeof(RenderParam));
		scene->PreRender(param, 0);

		GlowTarget(true);
		renderer->Clear(0., 0., 0., 1.);
		GlowTarget(false);		
		{
			RenderEventMarker marker("Shadow Pass");
			memset(&renderParam, 0, sizeof(RenderParam));
			memset(&renderParamOut, 0, sizeof(RenderParamOut));
			renderParam.mRenderPass = PASS_SHADOW;
			ShadowMap(false);
			ShadowTarget(true);
			scene->Render(renderParam, 0);
			ShadowTarget(false);
		}
	{
		RenderEventMarker marker("Depth Pass");
		memset(&renderParam, 0, sizeof(RenderParam));
		memset(&renderParamOut, 0, sizeof(RenderParamOut));
		renderParam.mRenderPass = PASS_DEPTH;
		DepthTarget(true, true);
		scene->Render(renderParam, 0);
		DepthTarget(false, false);
		DepthTexture(true);
	}
	{
		RenderEventMarker marker("Cloud Volume Pass");
		memset(&renderParam, 0, sizeof(RenderParam));
		memset(&renderParamOut, 0, sizeof(RenderParamOut));
		renderParam.mRenderPass = PASS_DEPTH;
		CloudVolumeTarget(true);
		renderer->LockDepthStencilState();
		renderer->SetBlueMask();
		scene->Render(renderParam, 0);
		renderer->UnlockDepthStencilState();
		DepthWriteShaderCloud();
		renderer->SetFrontFaceCullRS();
		renderer->SetNoDepthWriteLessEqual();
		renderer->SetGreenAlphaMaskAddMinusBlend(); // replace very first far volume face, and next will be added if it is not culled.
		renderParam.mSpecialRenderables = true;
		renderParam.mFilter = SceneObjectType::CloudVolumes;
		scene->PreRender(renderParam, 0);
		scene->Render(renderParam, 0);
		renderer->RestoreRasterizerState();
		renderer->SetRedAlphaMaskAddAddBlend();
		scene->Render(renderParam, 0);
		renderer->RestoreRasterizerState();
		renderer->RestoreBlendState();
		renderer->RestoreDepthStencilState();
		CloudVolumeTarget(false);
	}
	{
		RenderEventMarker marker("GodRay Pre-occlusion Pass");
		memset(&renderParam, 0, sizeof(RenderParam));
		memset(&renderParamOut, 0, sizeof(RenderParamOut));
		GodRayTarget(true);
		renderParam.mRenderPass = PASS_GODRAY_OCC_PRE;
		scene->Render(renderParam, 0);
		GodRay();
	}
	{
		RenderEventMarker marker("Main Render Pass");
		memset(&renderParam, 0, sizeof(RenderParam));
		memset(&renderParamOut, 0, sizeof(RenderParamOut));
		renderParam.mRenderPass = PASS_NORMAL;
		HDRTarget(true);
		renderer->Clear(0., 0., 0., 1.);
		DepthTexture(true);
		CloudVolumeTexture(true);
		ShadowMap(true);
		scene->Render(renderParam, &renderParamOut);
		if (renderParamOut.mSilouetteRendered)
			Silouette();
	}

	{
		RenderEventMarker marker("Blend Glow and God Ray");
		BlendGlow();
		BlendGodRay();
		HDRTarget(false);
	}
	{
		RenderEventMarker marker("HDR and Bloom");
		MeasureLuminanceOfHDRTarget();
		BrightPass();
		BrightPassToStarSource();
		StarSourceToBloomSource();
		Bloom();
		RenderStarGlare();
		ToneMapping();
	}
	}


	bool SetSmallSilouetteBuffer(){
		auto renderer = Renderer::GetInstance();
		auto rt = mRenderTarget.lock();
		const auto& size = rt->GetSize();
		Vec2I halfSize((int)(size.x * 0.5f), (int)(size.y * 0.5f));
		if (!mSmallSilouetteBuffer)
		{
			mSmallSilouetteBuffer = renderer->CreateTexture(0, halfSize.x, halfSize.y, PIXEL_FORMAT_R16_FLOAT, BUFFER_USAGE_DEFAULT,
				BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
		}
		auto halfDepthBuffer = renderer->GetTemporalDepthBuffer(halfSize);
		TexturePtr rts[] = { mSmallSilouetteBuffer };
		unsigned index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, halfDepthBuffer, 0);
		Viewport vps = { 0, 0, (Real)halfSize.x, (Real)halfSize.y, 0.0f, 1.0f };
		renderer->SetViewports(&vps, 1);
		return true;
	}

	bool SetBigSilouetteBuffer(){
		auto renderer = Renderer::GetInstance();
		auto rt = mRenderTarget.lock();
		const auto& size = rt->GetSize();
		if (!mBigSilouetteBuffer)
		{
			mBigSilouetteBuffer = renderer->CreateTexture(0, size.x, size.y, PIXEL_FORMAT_R16_FLOAT, BUFFER_USAGE_DEFAULT,
				BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
		}
		auto depthBuffer = renderer->GetTemporalDepthBuffer(size);
		TexturePtr rts[] = { mBigSilouetteBuffer };
		unsigned index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, depthBuffer, 0);
		Viewport vps = { 0, 0, (Real)size.x, (Real)size.y, 0.0f, 1.0f };
		renderer->SetViewports(&vps, 1);
		return true;
	}

	//-------------------------------------------------------------------
	void ClearSilouetteBuffer(){
		auto renderer = Renderer::GetInstance();
		auto rt = mRenderTarget.lock();
		{
			Vec2I halfSize((int)(mSize.x * 0.5f), (int)(mSize.y * 0.5f));
			if (!mSmallSilouetteBuffer)
			{
				mSmallSilouetteBuffer = renderer->CreateTexture(0, halfSize.x, halfSize.y, PIXEL_FORMAT_R16_FLOAT, BUFFER_USAGE_DEFAULT,
					BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
			}
			TexturePtr rts[] = { mSmallSilouetteBuffer };
			unsigned index[] = { 0 };
			renderer->SetRenderTarget(rts, index, 1, 0, 0);
			Viewport vps = { 0, 0, (Real)halfSize.x, (Real)halfSize.y, 0.0f, 1.0f };
			renderer->SetViewports(&vps, 1);
			renderer->Clear(1, 1, 1, 1, 1, 0);
		}
		{
			if (!mBigSilouetteBuffer)
			{
				mBigSilouetteBuffer = renderer->CreateTexture(0, mSize.x, mSize.y, PIXEL_FORMAT_R16_FLOAT, BUFFER_USAGE_DEFAULT,
					BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
			}
			TexturePtr rts[] = { mBigSilouetteBuffer };
			unsigned index[] = { 0 };
			renderer->SetRenderTarget(rts, index, 1, 0, 0);
			Viewport vps = { 0, 0, (Real)mSize.x, (Real)mSize.y, 0.0f, 1.0f };
			renderer->SetViewports(&vps, 1);
			renderer->Clear(1, 1, 1, 1, 1, 0);
		}
	}

	void DepthTarget(bool bind, bool clear)
	{
		auto renderer = Renderer::GetInstance();
		auto rt = mRenderTarget.lock();
		if (bind){
			int width = int(mSize.x);
			int height = int(mSize.y);
			if (!mDepthTarget)
			{
				mDepthTarget = renderer->CreateTexture(0, width, height, PIXEL_FORMAT_R16_FLOAT, BUFFER_USAGE_DEFAULT,
					BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
			}
			TexturePtr rts[] = { mDepthTarget };
			size_t rtViewIndex[] = { 0 };

			auto depthBuffer = renderer->GetTemporalDepthBuffer(Vec2I(width, height));
			renderer->SetRenderTarget(rts, rtViewIndex, 1, depthBuffer, 0);
			Viewport vp = { 0, 0, (Real)width, (Real)height, 0, 1 };
			renderer->SetViewports(&vp, 1);
			if (clear)
				renderer->Clear(1, 1, 1, 1, 1, 0);
			renderer->LockBlendState();
		}
		else{
			rt->BindTargetOnly(false);
			renderer->UnlockBlendState();
		}
	}

	void DepthTexture(bool bind) {
		auto renderer = Renderer::GetInstance();
		if (bind){
			renderer->SetSystemTexture(SystemTextures::Depth, mDepthTarget);			
		}
		else{
			renderer->SetSystemTexture(SystemTextures::Depth, 0);
		}
	}

	void GlowTarget(bool bind)
	{
		auto renderer = Renderer::GetInstance();
		auto rt = mRenderTarget.lock();
		if (bind){
			if (!renderer->GetOptions()->r_Glow){
				rt->BindTargetOnly(true);
			}

			if (mGlowSet)
				return;

			if (!mGlowTarget)
			{
				/*
				A pixel shader can be used to render to at least 8 separate render targets,
				all of which must be the same type (buffer, Texture1D, Texture1DArray, etc...).
				Furthermore, all render targets must have the same size in all dimensions (width, height, depth, array size, sample counts).
				Each render target may have a different data format.

				You may use any combination of render targets slots (up to 8). However, a resource view cannot be bound to
				multiple render-target-slots simultaneously. A view may be reused as long as the resources are not used simultaneously.

				*/				
				mGlowTarget = renderer->CreateTexture(0, mSize.x, mSize.y, PIXEL_FORMAT_R16G16B16A16_FLOAT, BUFFER_USAGE_DEFAULT,
					BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV | TEXTURE_TYPE_MULTISAMPLE);				
				mGlowTarget->SetDebugName(FormatString("rt%u_%u_%u_GlowTarget", rt->GetId(), mSize.x, mSize.y).c_str());

				mGlowTexture[0] = renderer->CreateTexture(0, (int)(mSize.x * 0.25f), (int)(mSize.y * 0.25f), PIXEL_FORMAT_R16G16B16A16_FLOAT, BUFFER_USAGE_DEFAULT,
					BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV | TEXTURE_TYPE_MULTISAMPLE);
				mGlowTexture[0]->SetDebugName(FormatString("rt%u_%u_%u_GlowTexture0", rt->GetId(), mSize.x, mSize.y).c_str());

				mGlowTexture[1] = renderer->CreateTexture(0, (int)(mSize.x * 0.25f), (int)(mSize.y * 0.25f), PIXEL_FORMAT_R16G16B16A16_FLOAT, BUFFER_USAGE_DEFAULT,
					BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV | TEXTURE_TYPE_MULTISAMPLE);
				mGlowTexture[1]->SetDebugName(FormatString("rt%u_%u_%u_GlowTexture1", rt->GetId(), mSize.x, mSize.y).c_str());
			}

			TexturePtr rts[] = { rt->GetRenderTargetTexture(), mGlowTarget };
			if (mHDRTarget && renderer->GetOptions()->r_HDR)
			{
				rts[0] = mHDRTarget;
			}

			assert(mRenderingFace == 0);
			size_t rtViewIndex[] = { mRenderingFace, 0 };
			renderer->SetRenderTarget(rts, rtViewIndex, 2, rt->GetDepthStencilTexture(), mRenderingFace);

			auto viewPort = rt->GetViewport();
			Viewport viewports[] = { viewPort, viewPort };
			renderer->SetViewports(viewports, 2);
			mGlowSet = true;
		}
		else{
			if (!mGlowSet)
				return;
			mGlowSet = false;
			rt->BindTargetOnly(true);
		}

	}

	void UpdateLightCamera()
	{
		auto renderer = Renderer::GetInstance();
		auto cam = renderer->GetCamera();
		if (!mLightCamera)
		{
			mLightCamera = Camera::Create();				
			mLightCamera->SetOrthogonal(true);
			auto cmd = renderer->GetOptions();
			float width = std::min(cmd->r_ShadowCamWidth, mSize.x * (cmd->r_ShadowCamWidth / 1600.f));
			float height = std::min(cmd->r_ShadowCamHeight, mSize.y * (cmd->r_ShadowCamHeight / 900.f));
			width = std::max(16.f, width);
			height = std::max(16.f, height);
			mLightCamSize = Vec2(width, height);

			Vec2 shadowMapSize((Real)std::min((Real)cmd->r_ShadowMapWidth, mSize.x / 1600.0 * cmd->r_ShadowMapWidth),
				(Real)std::min((Real)cmd->r_ShadowMapHeight, mSize.y / 900.0 * cmd->r_ShadowMapHeight));
			shadowMapSize.x = (Real)CropSize8((int)shadowMapSize.x);
			shadowMapSize.y = (Real)CropSize8((int)shadowMapSize.y);
			shadowMapSize.x = std::max(16.0, shadowMapSize.x);
			shadowMapSize.y = std::max(16.0, shadowMapSize.y);

			Vec2 vWorldUnitsPerTexel;
			vWorldUnitsPerTexel = Vec2(mLightCamSize.x, mLightCamSize.y);
			vWorldUnitsPerTexel *= Vec2(1.0f / shadowMapSize.x, 1.0f / shadowMapSize.y);
			mLightCamSize.x = std::floor(mLightCamSize.x / vWorldUnitsPerTexel.x);
			mLightCamSize.x *= vWorldUnitsPerTexel.x;
			mLightCamSize.y = std::floor(mLightCamSize.y / vWorldUnitsPerTexel.y);
			mLightCamSize.y *= vWorldUnitsPerTexel.y;

			mLightCamera->SetWidth(mLightCamSize.x);
			mLightCamera->SetHeight(mLightCamSize.y);
			mLightCamera->SetNearFar(cmd->r_ShadowNear, cmd->r_ShadowFar);
		}


		auto target = cam->GetTarget();
		float shadowCamDist = renderer->GetOptions()->r_ShadowCamDist;
		auto light = mScene.lock()->GetLight(0);
		assert(light);
		if (target && target->GetBoundingVolume() && target->GetBoundingVolume()->GetRadius() < shadowCamDist)
		{
			mLightCamera->SetPosition(target->GetPos() + cam->GetDirection() * 10.0f + light->GetPosition() *shadowCamDist);
		}
		else
		{
			mLightCamera->SetPosition(cam->GetPosition() + cam->GetDirection() * 10.0f + light->GetPosition() * shadowCamDist);
		}
		mLightCamera->SetDirection(-light->GetPosition());
	}

	void ShadowTarget(bool bind)
	{
		auto renderer = Renderer::GetInstance();
		auto rt = mRenderTarget.lock();
		if (bind){

			auto cmd = renderer->GetOptions();
			if (!mShadowMap)
			{
				const auto& size = mSize;
				int width = (int)std::min(cmd->r_ShadowMapWidth, (int)(size.x / 1600 * cmd->r_ShadowMapWidth));
				int height = (int)std::min(cmd->r_ShadowMapHeight, (int)(size.y / 900 * cmd->r_ShadowMapHeight));
				width = CropSize8(width);
				height = CropSize8(height);

				width = std::max(16, width);
				height = std::max(16, height);

				mShadowMap = renderer->CreateTexture(0,
					width, height,
					PIXEL_FORMAT_D32_FLOAT, BUFFER_USAGE_DEFAULT,
					BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_DEPTH_STENCIL_SRV);
				assert(mShadowMap);				
				mShadowMap->SetDebugName(FormatString("rt%u_%u_%u_ShadowMap", rt->GetId(), width, height).c_str());
			}

			TexturePtr rts[] = { 0 };
			size_t index[] = { 0 };
			renderer->SetRenderTarget(rts, index, 1, mShadowMap, 0);
			renderer->SetShadowMapShader();
			const auto& size = mShadowMap->GetSize();
			Viewport vp = { 0, 0,
				(Real)size.x,
				(Real)size.y,
				0, 1 };
			renderer->SetViewports(&vp, 1);
			renderer->SetCamera(mLightCamera);
			renderer->Clear(0, 0, 0, 0, 1.0f, 0);
		}
		else{
			assert(mShadowMap);
			rt->BindTargetOnly(false);
			renderer->SetCamera(rt->GetCamera());
		}
	}

	void ShadowMap(bool bind)
	{
		auto renderer = Renderer::GetInstance();
		if (bind && mShadowMap)
			renderer->SetSystemTexture(SystemTextures::ShadowMap, mShadowMap);
		else
			renderer->SetSystemTexture(SystemTextures::ShadowMap, 0);
	}

	void DepthWriteShaderCloud(){

	}

	void CloudVolumeTarget(bool bind)
	{
		auto renderer = Renderer::GetInstance();		
		if (bind){

			const auto& size = mSize;
			if (!mCloudVolumeDepth)
			{
				mCloudVolumeDepth = renderer->CreateTexture(0, size.x / 2, size.y / 2, PIXEL_FORMAT_R16G16B16A16_FLOAT, BUFFER_USAGE_DEFAULT,
					BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
			}
			auto depthBuffer = renderer->GetTemporalDepthBuffer(Vec2I(size.x / 2, size.y / 2));
			assert(depthBuffer);
			TexturePtr rts[] = { mCloudVolumeDepth };
			size_t index[] = { 0 };
			// mTempDepthBufferHalf already filled with scene objects. while writing the depth texture;
			renderer->SetRenderTarget(rts, index, 1, depthBuffer, 0);
			Viewport vp = { 0, 0, size.x * .5f, size.y * .5f, 0, 1 };
			renderer->SetViewports(&vp, 1);
			renderer->Clear(0.0f, 0.0f, 0.0f, 0, 1, 0);
		}
		else{
			auto rt = mRenderTarget.lock();
			rt->BindTargetOnly(false);
		}
	}

	void CloudVolumeTexture(bool set)
	{
		auto renderer = Renderer::GetInstance();
		if (set)
			renderer->SetSystemTexture(SystemTextures::CloudVolume, mCloudVolumeDepth);
		else
			renderer->SetSystemTexture(SystemTextures::CloudVolume, 0);
	}


	void GodRayTarget(bool bind)
	{
		auto renderer = Renderer::GetInstance();
		if (bind){			
			const auto& size = mSize;
			if (!mGodRayTarget[0])
			{
				for (int i = 0; i < 2; i++)
				{
					mGodRayTarget[i] = renderer->CreateTexture(0, size.x / 2, size.y / 2, PIXEL_FORMAT_R8G8B8A8_UNORM,
						BUFFER_USAGE_DEFAULT, BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
				}
				mNoMSDepthStencil = renderer->CreateTexture(0, size.x / 2, size.y / 2, PIXEL_FORMAT_D24_UNORM_S8_UINT,
					BUFFER_USAGE_DEFAULT, BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_DEPTH_STENCIL);
				renderer->GetGodRayPS();
			}


			TexturePtr rts[] = { mGodRayTarget[1] };
			size_t index[] = { 0 };
			renderer->SetRenderTarget(rts, index, 1, mNoMSDepthStencil, 0);
			Viewport vp = { 0, 0, (Real)size.x*.5f, (Real)size.y*.5f, 0, 1 };
			renderer->SetViewports(&vp, 1);
			renderer->Clear(0, 0, 0, 1, 1, 0);
		}
		else{

		}
	}

	void GodRay()
	{
		auto renderer = Renderer::GetInstance();
		auto rt = mRenderTarget.lock();
		DirectionalLightPtr light = renderer->GetDirectionalLight(0);
		assert(light);
		auto camera = rt->GetCamera();
		Vec4 lightPos(camera->GetPosition() + light->GetPosition(), 1);
		lightPos = camera->GetMatrix(Camera::ViewProj) * lightPos; // only x,y nee
		lightPos.x = lightPos.x*.5f + .5f;
		lightPos.y = .5f - lightPos.y*.5f;

		TexturePtr rts[] = { mGodRayTarget[0] };
		size_t index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, 0, 0);
		renderer->SetTexture(mGodRayTarget[1], BINDING_SHADER_PS, 0);
		Vec4* pData = (Vec4*)renderer->MapMaterialParameterBuffer();
		if (pData)
		{
			*pData = camera->GetMatrix(Camera::ViewProj) * lightPos; // only x,y needed.
			pData->x = lightPos.x;
			pData->y = lightPos.y;
			auto pEC = renderer->GetOptions();

			pData->z = pEC->r_GodRayDensity; // density
			pData->w = pEC->r_GodRayDecay; // decay
			++pData;
			pData->x = pEC->r_GodRayWeight; // weight
			pData->y = pEC->r_GodRayExposure; // exposure
			renderer->UnmapMaterialParameterBuffer();
		}
		renderer->DrawFullscreenQuad(renderer->GetGodRayPS(), false);

		rt->BindTargetOnly(false);
	}


	//---------------------------------------------------------------------------
	void HDRTarget(bool bind)
	{
		auto renderer = Renderer::GetInstance();		
		const auto& size = mSize;
		if (!mHDRTarget)
		{
			mHDRTarget = renderer->CreateTexture(0, size.x, size.y, PIXEL_FORMAT_R16G16B16A16_FLOAT, BUFFER_USAGE_DEFAULT, BUFFER_CPU_ACCESS_NONE,
				TEXTURE_TYPE_RENDER_TARGET_SRV | TEXTURE_TYPE_MULTISAMPLE);
			if (!mHDRTarget)
			{
				Logger::Log(FB_ERROR_LOG_ARG, "Cannot create HDR RenderTarget.");
				return;
			}			
			mHDRTarget->SetDebugName(FormatString("rt%u_%u_%u_HDRTargetTexture", mId, size.x, size.y).c_str());
		}

		TexturePtr rts[] = { mHDRTarget };
		size_t index[] = { 0 };
		auto rt = mRenderTarget.lock();
		renderer->SetRenderTarget(rts, index, 1, rt->GetDepthStencilTexture(), 0);
		renderer->SetViewports(&rt->GetViewport(), 1);
	}

	void Silouette()
	{
		auto rt = mRenderTarget.lock();
		rt->BindTargetOnly(true);
		auto renderer = Renderer::GetInstance();

		TexturePtr ts[] = { mSmallSilouetteBuffer, mBigSilouetteBuffer, mDepthTarget };
		renderer->SetTextures(ts, 3, BINDING_SHADER_PS, 0);
		renderer->DrawFullscreenQuad(renderer->GetSilouetteShader(), false);
	}

	//---------------------------------------------------------------------------
	void BlendGlow()
	{
		if (!mGlowTexture[0])
		{
			return;
		}

		auto const renderer = Renderer::GetInstance();
		auto rt = mRenderTarget.lock();
		const auto& oriSize = mSize;
		Vec2I size((int)(oriSize.x*0.25f), (int)(oriSize.y*0.25f));
		{
			RenderEventMarker marker("Glowing");
			assert(mGlowTarget);
			TexturePtr rt[] = { mGlowTexture[1] };
			size_t index[] = { 0 };
			renderer->SetRenderTarget(rt, index, 1, 0, 0);
			renderer->SetTexture(mGlowTarget, BINDING_SHADER_PS, 0);
			renderer->RestoreBlendState();
			Vec2I resol = mGlowTexture[0]->GetSize();
			Viewport vp = { 0, 0, (Real)resol.x, (Real)resol.y, 0, 1 };
			renderer->SetViewports(&vp, 1);

			if (!mGaussianDistBlendGlow)
			{
				mGaussianDistBlendGlow = GaussianDistPtr(FB_NEW(GaussianDist), [](GaussianDist* obj){ delete obj; });
				mGaussianDistBlendGlow->Calc(oriSize.x, oriSize.y, 3, 1.25f);
			}

			// Horizontal Blur		
			BIG_BUFFER* pData = (BIG_BUFFER*)renderer->MapBigBuffer();
			Vec4* avSampleOffsets = pData->gSampleOffsets;
			Vec4* avSampleWeights = pData->gSampleWeights;
			memcpy(avSampleOffsets, mGaussianDistBlendGlow->mGaussianDistOffsetX, sizeof(Vec4) * 15);
			memcpy(avSampleWeights, mGaussianDistBlendGlow->mGaussianDistWeightX, sizeof(Vec4) * 15);
			renderer->UnmapBigBuffer();
			// mGlowPS is same with BloomPS except it has the _MULTI_SAMPLE shader define.
			renderer->DrawFullscreenQuad(renderer->GetGlowShader(), false);

			// Vertical Blur
			pData = (BIG_BUFFER*)renderer->MapBigBuffer();
			avSampleOffsets = pData->gSampleOffsets;
			avSampleWeights = pData->gSampleWeights;
			memcpy(avSampleOffsets, mGaussianDistBlendGlow->mGaussianDistOffsetY, sizeof(Vec4) * 15);
			memcpy(avSampleWeights, mGaussianDistBlendGlow->mGaussianDistWeightY, sizeof(Vec4) * 15);
			renderer->UnmapBigBuffer();
			rt[0] = mGlowTexture[0];
			renderer->SetRenderTarget(rt, index, 1, 0, 0);
			renderer->SetTexture(mGlowTexture[1], BINDING_SHADER_PS, 0);
			renderer->DrawFullscreenQuad(renderer->GetGlowShader(), false);
		}

	{
		RenderEventMarker marker("Glow Blend");
		rt->BindTargetOnly(true);
		renderer->SetTexture(mGlowTexture[0], BINDING_SHADER_PS, 0);
		renderer->SetAdditiveBlendState();
		renderer->SetNoDepthStencil();
		if (renderer->GetMultiSampleCount() == 1)
			renderer->DrawFullscreenQuad(renderer->GetCopyPS(), false);
		else
			renderer->DrawFullscreenQuad(renderer->GetCopyPSMS(), false);
		renderer->RestoreBlendState();
	}
	}

	////---------------------------------------------------------------------------
	void BlendGodRay()
	{
		RenderEventMarker marker("GodRay Blending");
		assert(mGodRayTarget[0]);
		auto const renderer = Renderer::GetInstance();
		renderer->SetTexture(mGodRayTarget[0], BINDING_SHADER_PS, 0);
		renderer->SetAdditiveBlendState();
		renderer->SetNoDepthStencil();
		renderer->DrawFullscreenQuad(renderer->GetCopyPS(), false);
		renderer->RestoreBlendState();
	}

	////---------------------------------------------------------------------------
	void MeasureLuminanceOfHDRTarget()
	{
		auto const renderer = Renderer::GetInstance();
		renderer->RestoreBlendState();
		renderer->SetNoDepthStencil();

		RenderEventMarker mark("Luminance");
		assert(mHDRTarget);
		int dwCurTexture = Renderer::FB_NUM_TONEMAP_TEXTURES_NEW - 1;
		TexturePtr renderTarget = renderer->GetToneMap(dwCurTexture);
		TexturePtr rts[] = { renderTarget };
		size_t index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, 0, 0); // no depth buffer;
		renderer->SetTexture(mHDRTarget, BINDING_SHADER_PS, 0);
		bool msaa = renderer->GetMultiSampleCount() > 1;
		if (msaa)
		{
			Vec4* pDest = (Vec4*)renderer->MapMaterialParameterBuffer();
			if (pDest)
			{
				pDest->x = (Real)mHDRTarget->GetWidth();
				pDest->y = (Real)mHDRTarget->GetHeight();
				renderer->UnmapMaterialParameterBuffer();
			}
		}

		const Vec2I& resol = renderTarget->GetSize();
		Viewport vp = { 0, 0, (Real)resol.x, (Real)resol.y, 0, 1 };
		renderer->SetViewports(&vp, 1);
		renderer->DrawFullscreenQuad(renderer->GetSampleLumInitialShader(), false);
		--dwCurTexture;

		while (dwCurTexture>0)
		{
			TexturePtr src = renderer->GetToneMap(dwCurTexture + 1);
			TexturePtr renderTarget = renderer->GetToneMap(dwCurTexture);

			TexturePtr rts[] = { renderTarget };
			size_t index[] = { 0 };
			renderer->SetRenderTarget(rts, index, 1, 0, 0); // no depth buffer;
			renderer->SetTexture(src, BINDING_SHADER_PS, 0);

			const Vec2I& resol = renderTarget->GetSize();
			Viewport vp = { 0, 0, (Real)resol.x, (Real)resol.y, 0, 1 };
			renderer->SetViewports(&vp, 1);
			renderer->DrawFullscreenQuad(renderer->GetSampleLumIterativeShader(), false);
			--dwCurTexture;
		}

		// Perform the final pass of the average luminance calculation.
	{
		TexturePtr src = renderer->GetToneMap(dwCurTexture + 1);
		TexturePtr renderTarget = renderer->GetToneMap(dwCurTexture);
		TexturePtr rts[] = { renderTarget };
		size_t index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, 0, 0); // no depth buffer;
		renderer->SetTexture(src, BINDING_SHADER_PS, 0);
		const Vec2I& resol = renderTarget->GetSize();
		{Viewport vp = { 0, 0, (Real)resol.x, (Real)resol.y, 0, 1 };
		renderer->SetViewports(&vp, 1); }

		renderer->DrawFullscreenQuad(renderer->GetSampleLumFinalShader(), false);
	}

	// AdaptedLum
	{
		renderer->SwapLuminanceMap();
		auto luminanceMap0 = renderer->GetLuminanceMap(0);
		TexturePtr rts[] = { luminanceMap0 };
		size_t index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, 0, 0); // no depth buffer;
		renderer->SetTexture(renderer->GetLuminanceMap(1), BINDING_SHADER_PS, 0);
		renderer->SetTexture(renderer->GetToneMap(0), BINDING_SHADER_PS, 1);
		Viewport vp = { 0, 0, (Real)luminanceMap0->GetWidth(), (Real)luminanceMap0->GetHeight(), 0, 1 };
		renderer->SetViewports(&vp, 1);
		renderer->DrawFullscreenQuad(renderer->GetCalcAdapedLumShader(), false);
	}
	}

	//---------------------------------------------------------------------------
	void BrightPass()
	{
		auto const renderer = Renderer::GetInstance();
		auto rt = mRenderTarget.lock();
		const auto& size = mSize;
		if (!mBrightPassTexture)
		{
			Vec2I size(
				CropSize8((int)(size.x* 0.25f)),
				CropSize8((int)(size.y * 0.25f))
				);
			mBrightPassTexture = renderer->CreateTexture(0, size.x, size.y, PIXEL_FORMAT_R8G8B8A8_UNORM,
				BUFFER_USAGE_DEFAULT, BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);			
			mBrightPassTexture->SetDebugName(FormatString("rt%u_%u_%u_BrightPass", rt->GetId(), size.x, size.y).c_str());
		}

		const Vec2I& resol = mBrightPassTexture->GetSize();
		{
			RenderEventMarker mark("Bloom - BrightPass");
			// brightpass
			TexturePtr rts[] = { mBrightPassTexture };
			size_t index[] = { 0 };
			renderer->SetRenderTarget(rts, index, 1, 0, 0); // no depth buffer;

			TexturePtr rvs[] = { mHDRTarget, renderer->GetLuminanceMap(0) };
			renderer->SetTextures(rvs, 2, BINDING_SHADER_PS, 0);

			Viewport vp = { 0, 0, (Real)resol.x, (Real)resol.y, 0, 1 };
			renderer->SetViewports(&vp, 1);

			if (renderer->GetMultiSampleCount() != 1)
			{
				Vec4* pDest = (Vec4*)renderer->MapMaterialParameterBuffer();
				if (pDest)
				{
					pDest->x = (Real)mHDRTarget->GetWidth();
					pDest->y = (Real)mHDRTarget->GetHeight();
					renderer->UnmapMaterialParameterBuffer();
				}
			}

			renderer->DrawFullscreenQuad(renderer->GetBrightPassPS(), false);
		}
	}

	//---------------------------------------------------------------------------
	void BrightPassToStarSource()
	{
		auto const renderer = Renderer::GetInstance();		
		const auto& size = mSize;
		if (!mStarSourceTex)
		{
			Vec2I starSourceSize(CropSize8((int)(size.x*0.25f)),
				CropSize8((int)(size.y*0.25f))
				);
			mStarSourceTex = renderer->CreateTexture(0, starSourceSize.x, starSourceSize.y, PIXEL_FORMAT_R8G8B8A8_UNORM,
				BUFFER_USAGE_DEFAULT, BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
		}
		RenderEventMarker mark("BrightPassToStarSource");

		Vec4* pOffsets = 0;
		Vec4* pWeights = 0;
		renderer->GetSampleOffsets_GaussBlur5x5(mGlowTarget->GetWidth(), mGlowTarget->GetHeight(),
			&pOffsets, &pWeights, 1.0f);
		assert(pOffsets && pWeights);

		BIG_BUFFER* pData = (BIG_BUFFER*)renderer->MapBigBuffer();
		memcpy(pData->gSampleOffsets, pOffsets, sizeof(Vec4) * 13);
		memcpy(pData->gSampleWeights, pWeights, sizeof(Vec4) * 13);
		renderer->UnmapBigBuffer();

		TexturePtr rts[] = { mStarSourceTex };
		size_t index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, 0, 0);
		Viewport vp = { 0, 0, (Real)mStarSourceTex->GetWidth(), (Real)mStarSourceTex->GetHeight(), 0, 1 };
		renderer->SetViewports(&vp, 1);
		renderer->Clear(0, 0, 0, 1);
		renderer->SetTexture(mGlowTarget, BINDING_SHADER_PS, 0);
		if (renderer->GetMultiSampleCount() != 1)
		{
			Vec4* pDest = (Vec4*)renderer->MapMaterialParameterBuffer();
			if (pDest)
			{
				pDest->x = (Real)mGlowTarget->GetWidth();
				pDest->y = (Real)mGlowTarget->GetHeight();
				renderer->UnmapMaterialParameterBuffer();
			}
		}

		renderer->DrawFullscreenQuad(renderer->GetBlur5x5PS(), false);
	}

	//---------------------------------------------------------------------------
	void StarSourceToBloomSource()
	{
		RenderEventMarker mark("StarSourceToBloomSource");
		auto const renderer = Renderer::GetInstance();		
		const auto& size = mSize;
		if (!mBloomSourceTex)
		{
			Vec2I bloomSourceSize(CropSize8(size.x / 8), CropSize8(size.y / 8));
			mBloomSourceTex = renderer->CreateTexture(0, bloomSourceSize.x, bloomSourceSize.y, PIXEL_FORMAT_R8G8B8A8_UNORM, BUFFER_USAGE_DEFAULT,
				BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
		}
		auto blur5x5 = renderer->GetBlur5x5PS();
		assert(blur5x5);
		Vec4* pOffsets = 0;
		Vec4* pWeights = 0;
		renderer->GetSampleOffsets_GaussBlur5x5(mBrightPassTexture->GetWidth(), mBrightPassTexture->GetHeight(),
			&pOffsets, &pWeights, 1.0f);
		assert(pOffsets && pWeights);

		BIG_BUFFER* pData = (BIG_BUFFER*)renderer->MapBigBuffer();
		memcpy(pData->gSampleOffsets, pOffsets, sizeof(Vec4) * 13);
		memcpy(pData->gSampleWeights, pWeights, sizeof(Vec4) * 13);
		renderer->UnmapBigBuffer();

		TexturePtr rts[] = { mBloomSourceTex };
		size_t index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, 0, 0);

		Viewport vp = { 0, 0, (Real)mBloomSourceTex->GetWidth(), (Real)mBloomSourceTex->GetHeight(), 0, 1 };
		renderer->SetViewports(&vp, 1);

		renderer->SetTexture(mBrightPassTexture, BINDING_SHADER_PS, 0);

		if (renderer->GetMultiSampleCount() != 1)
		{
			Vec4* pDest = (Vec4*)renderer->MapMaterialParameterBuffer();
			if (pDest)
			{
				pDest->x = (Real)mBrightPassTexture->GetWidth();
				pDest->y = (Real)mBrightPassTexture->GetHeight();
				renderer->UnmapMaterialParameterBuffer();
			}
		}

		renderer->DrawFullscreenQuad(blur5x5, false);
	}

	//---------------------------------------------------------------------------
	void Bloom()
	{
		auto const renderer = Renderer::GetInstance();
		if (!mBloomTexture[0])
		{
			Vec2I size(CropSize8(mSize.x / 8),
				CropSize8(mSize.y / 8));
			for (int i = 0; i < FB_NUM_BLOOM_TEXTURES; i++)
			{
				mBloomTexture[i] = renderer->CreateTexture(0, size.x, size.y, PIXEL_FORMAT_R8G8B8A8_UNORM,
					BUFFER_USAGE_DEFAULT, BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
				char buff[255];
				sprintf_s(buff, "rt%u_%u_%u_Bloom(%d) %u", mId, size.x, size.y, i);
				mBloomTexture[i]->SetDebugName(buff);
			}
		}

		// blur
		RenderEventMarker mark("Bloom - Blur");
		assert(mBloomSourceTex);
		TexturePtr rts[] = { mBloomTexture[2] };
		size_t index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, 0, 0);
		Viewport vp = { 0, 0, (Real)mBloomTexture[2]->GetWidth(), (Real)mBloomTexture[2]->GetHeight(), 0, 1 };
		renderer->SetViewports(&vp, 1);

		renderer->SetTexture(mBloomSourceTex, BINDING_SHADER_PS, 0);

		Vec4* pOffsets = 0;
		Vec4* pWeights = 0;
		renderer->GetSampleOffsets_GaussBlur5x5(mBrightPassTexture->GetWidth(), mBrightPassTexture->GetHeight(),
			&pOffsets, &pWeights, 1.0f);
		assert(pOffsets && pWeights);

		BIG_BUFFER* pData = (BIG_BUFFER*)renderer->MapBigBuffer();
		memcpy(pData->gSampleOffsets, pOffsets, sizeof(Vec4) * 13);
		memcpy(pData->gSampleWeights, pWeights, sizeof(Vec4) * 13);
		renderer->UnmapBigBuffer();

		if (renderer->GetMultiSampleCount() != 1)
		{
			Vec4* pDest = (Vec4*)renderer->MapMaterialParameterBuffer();
			if (pDest)
			{
				pDest->x = (Real)mBloomSourceTex->GetWidth();
				pDest->y = (Real)mBloomSourceTex->GetHeight();
				renderer->UnmapMaterialParameterBuffer();
			}
		}

		renderer->DrawFullscreenQuad(renderer->GetBlur5x5PS(), false);

		const Vec2I& resol = mBloomTexture[2]->GetSize();
		// Horizontal Blur
		{
			RenderEventMarker mark("Bloom - Apply hori gaussian filter");
			BIG_BUFFER* pData = (BIG_BUFFER*)renderer->MapBigBuffer();
			Vec4* avSampleOffsets = pData->gSampleOffsets;
			Vec4* avSampleWeights = pData->gSampleWeights;
			if (!mGaussianDistBloom)
			{
				mGaussianDistBloom = GaussianDistPtr(FB_NEW(GaussianDist), [](GaussianDist* obj){ delete obj; });
				mGaussianDistBloom->Calc(resol.x, resol.y, 3, 1.25f);
			}
			memcpy(avSampleOffsets, mGaussianDistBloom->mGaussianDistOffsetX, sizeof(Vec4) * 15);
			memcpy(avSampleWeights, mGaussianDistBloom->mGaussianDistWeightX, sizeof(Vec4) * 15);
			renderer->UnmapBigBuffer();

			TexturePtr rts[] = { mBloomTexture[1] };
			size_t index[] = { 0 };
			renderer->SetRenderTarget(rts, index, 1, 0, 0);
			renderer->SetTexture(mBloomTexture[2], BINDING_SHADER_PS, 0);
			renderer->DrawFullscreenQuad(renderer->GetBloomPS(), false);
		}

		// Vertical Blur
	{
		RenderEventMarker mark("Bloom - Apply verti gaussian filter");
		BIG_BUFFER* pData = (BIG_BUFFER*)renderer->MapBigBuffer();
		Vec4* avSampleOffsets = pData->gSampleOffsets;
		Vec4* avSampleWeights = pData->gSampleWeights;
		memcpy(avSampleOffsets, mGaussianDistBloom->mGaussianDistOffsetY, sizeof(Vec4) * 15);
		memcpy(avSampleWeights, mGaussianDistBloom->mGaussianDistWeightY, sizeof(Vec4) * 15);
		renderer->UnmapBigBuffer();

		renderer->SetTexture(0, BINDING_SHADER_PS, 0);
		renderer->SetTexture(0, BINDING_SHADER_PS, 1);
		renderer->SetTexture(0, BINDING_SHADER_PS, 2);
		rts[0] = mBloomTexture[0];
		renderer->SetRenderTarget(rts, index, 1, 0, 0);
		renderer->SetTexture(mBloomTexture[1], BINDING_SHADER_PS, 0);

		renderer->DrawFullscreenQuad(renderer->GetBloomPS(), false);
	}
	}
	
	
	//static  function
	void ReleaseStarDef()
	{
		FB_DELETE(starGlareDef);
	}
	void CalcStarGlareConst(Vec4 s_aaColor[starGlareMaxPasses][starGlareSamples])
	{
		static const Color s_colorWhite(0.63f, 0.63f, 0.63f, 0.0f);
		for (int p = 0; p < starGlareMaxPasses; ++p)
		{
			Real ratio;
			ratio = (Real)(p + 1) / (Real)starGlareMaxPasses;

			for (int s = 0; s < starGlareSamples; s++)
			{
				Color chromaticAberrColor = Lerp(StarDef::GetChromaticAberrationColor(s), s_colorWhite, ratio);
				s_aaColor[p][s] = Lerp(s_colorWhite, chromaticAberrColor, starGlareChromaticAberration).GetVec4();
			}
		}
		StarDef::InitializeStatic();
		starGlareDef = FB_NEW(StarDef);
		starGlareDef->Initialize(STLT_VERTICAL);
	}

	//---------------------------------------------------------------------------
	void RenderStarGlare()
	{
		auto const renderer = Renderer::GetInstance();
		if (mStarTextures[0] == 0)
		{
			Vec2I size(CropSize8((int)(mSize.x * 0.25f)), CropSize8((int)(mSize.y*0.25f)));
			for (int i = 0; i < FB_NUM_STAR_TEXTURES; ++i)
			{
				mStarTextures[i] = renderer->CreateTexture(0, size.x, size.y, PIXEL_FORMAT_R16G16B16A16_FLOAT, BUFFER_USAGE_DEFAULT,
					BUFFER_CPU_ACCESS_NONE, TEXTURE_TYPE_RENDER_TARGET_SRV);
			}
		}

		TexturePtr rts[] = { mStarTextures[0] };
		size_t index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, 0, 0);
		Viewport vp = { 0, 0, (Real)mStarTextures[0]->GetWidth(), (Real)mStarTextures[0]->GetHeight(), 0, 1 };
		renderer->SetViewports(&vp, 1);
		renderer->Clear(0, 0, 0, 1);

		auto rt = mRenderTarget.lock();
		const Real fTanFoV = rt->GetCamera()->GetFOV();
		const Color vWhite(1.0f, 1.0f, 1.0f, 1.0f);
		static Vec4 s_aaColor[starGlareMaxPasses][starGlareSamples];
		static bool s_aaColorCalced = false;
		if (!s_aaColorCalced)
		{
			s_aaColorCalced = true;
			CalcStarGlareConst(s_aaColor);

		}

		Vec4 avSampleWeights[Renderer::FB_MAX_SAMPLES];
		Vec4 avSampleOffsets[Renderer::FB_MAX_SAMPLES];

		//mAdditiveBlendState->Bind();

		Real srcW = (Real)mStarSourceTex->GetWidth();
		Real srcH = (Real)mStarSourceTex->GetHeight();


		Real radOffset;
		radOffset = starGlareInclination + starGlareDef->m_fInclination;

		TexturePtr pSrcTexture = 0;
		TexturePtr pDestTexture = 0;

		// Direction loop
		for (int d = 0; d < starGlareDef->m_nStarLines; ++d)
		{
			const STARLINE& starLine = starGlareDef->m_pStarLine[d];

			pSrcTexture = mStarSourceTex;

			Real rad = radOffset + starLine.fInclination;
			Real sn = std::sin(rad), cs = std::cos(rad);
			Vec2 vtStepUV(sn / srcW * starLine.fSampleLength,
				cs / srcH * starLine.fSampleLength);

			Real attnPowScale;
			attnPowScale = (fTanFoV + 0.1f) * 1.0f *
				(160.0f + 120.0f) / (srcW + srcH) * 1.2f;

			// 1 direction expansion loop
			int iWorkTexture;
			iWorkTexture = 1;
			for (int p = 0; p < starLine.nPasses; p++)
			{
				if (p == starLine.nPasses - 1)
				{
					// Last pass move to other work buffer
					pDestTexture = mStarTextures[d + 4];
				}
				else
				{
					pDestTexture = mStarTextures[iWorkTexture];
				}

				// Sampling configration for each stage
				for (int i = 0; i < starGlareSamples; ++i)
				{
					Real lum;
					lum = std::pow(starLine.fAttenuation, attnPowScale * i);

					avSampleWeights[i] = s_aaColor[starLine.nPasses - 1 - p][i] *
						lum * (p + 1.0f) * 0.5f;

					// Offset of sampling coordinate
					avSampleOffsets[i].x = vtStepUV.x * i;
					avSampleOffsets[i].y = vtStepUV.y * i;
					if (fabs(avSampleOffsets[i].x) >= 0.9f ||
						fabs(avSampleOffsets[i].y) >= 0.9f)
					{
						avSampleOffsets[i].x = 0.0f;
						avSampleOffsets[i].y = 0.0f;
						avSampleWeights[i] *= 0.0f;
					}

				}
				BIG_BUFFER* pData = (BIG_BUFFER*)renderer->MapBigBuffer();
				memcpy(pData->gSampleOffsets, avSampleOffsets, sizeof(Vec4)* Renderer::FB_MAX_SAMPLES);
				memcpy(pData->gSampleWeights, avSampleWeights, sizeof(Vec4)* Renderer::FB_MAX_SAMPLES);
				renderer->UnmapBigBuffer();

				TexturePtr rts[] = { pDestTexture };
				size_t index[] = { 0 };
				renderer->SetRenderTarget(rts, index, 1, 0, 0);
				renderer->SetTexture(pSrcTexture, BINDING_SHADER_PS, 0);
				renderer->DrawFullscreenQuad(renderer->GetStarGlareShader(), false);

				// Setup next expansion
				vtStepUV *= starGlareSamples;
				attnPowScale *= starGlareSamples;

				// Set the work drawn just before to next texture source.
				pSrcTexture = mStarTextures[iWorkTexture];

				iWorkTexture += 1;
				if (iWorkTexture > 2)
				{
					iWorkTexture = 1;
				}
			}
		}

		pDestTexture = mStarTextures[0];

		std::vector<TexturePtr> textures;
		textures.reserve(starGlareDef->m_nStarLines);
		for (int i = 0; i < starGlareDef->m_nStarLines; i++)
		{
			textures.push_back(mStarTextures[i + 4]);
			avSampleWeights[i] = vWhite.GetVec4() * (1.0f / (Real)starGlareDef->m_nStarLines);
		}

	{
		TexturePtr rts[] = { pDestTexture };
		size_t index[] = { 0 };
		renderer->SetRenderTarget(rts, index, 1, 0, 0);
	}
	renderer->SetTextures(&textures[0], textures.size(), BINDING_SHADER_PS, 0);

	switch (starGlareDef->m_nStarLines)
	{
	case 2:
		renderer->DrawFullscreenQuad(renderer->GetMergeTexturePS(), false);
		break;

	default:
		assert(0);
	}

	textures.clear();
	for (int i = 0; i < starGlareDef->m_nStarLines; i++)
	{
		textures.push_back(0);
	}
	renderer->SetTextures(&textures[0], starGlareDef->m_nStarLines, BINDING_SHADER_PS, 0);
	}

	//---------------------------------------------------------------------------
	void ToneMapping()
	{
		RenderEventMarker mark("ToneMapping");
		auto renderer = Renderer::GetInstance();
		TexturePtr textures[] = { mHDRTarget, renderer->GetLuminanceMap(0), mBloomTexture[0], mStarTextures[0] };
		auto rt = mRenderTarget.lock();
		rt->BindTargetOnly(false);
		renderer->SetTextures(textures, 4, BINDING_SHADER_PS, 0);
		if (renderer->IsLuminanceOnCpu())
		{
			Vec4* pData = (Vec4*)renderer->MapMaterialParameterBuffer();
			if (pData)
			{
				*pData = float4(mLuminance, 0, 0, 0);
				renderer->UnmapMaterialParameterBuffer();
			}
		}

		renderer->DrawFullscreenQuad(renderer->GetToneMappingPS(), false);
		renderer->RestoreDepthStencilState();
		renderer->SetTexture(0, BINDING_SHADER_PS, 3);
	}

	TexturePtr GetShadowMap() const{
		return mShadowMap;
	}

	void DeleteShadowMap()
	{
		mShadowMap = 0;
	}
};

//---------------------------------------------------------------------------
RenderStrategyDefault::RenderStrategyDefault()
: mImpl(new Impl){

}
RenderStrategyDefault::~RenderStrategyDefault(){
}

void RenderStrategyDefault::Render(size_t face){
	mImpl->Render(face);
}

bool RenderStrategyDefault::IsHDR() const{
	return true;
}

bool RenderStrategyDefault::IsGlowSupported(){
	return true;
}

CameraPtr RenderStrategyDefault::GetLightCamera() const{
	
	return mImpl->mLightCamera;
}

bool RenderStrategyDefault::SetHDRTarget(){
	mImpl->HDRTarget(true);
	return true;
}

bool RenderStrategyDefault::SetSmallSilouetteBuffer(){
	return mImpl->SetSmallSilouetteBuffer();
}

bool RenderStrategyDefault::SetBigSilouetteBuffer(){
	return mImpl->SetBigSilouetteBuffer();
}

void RenderStrategyDefault::GlowRenderTarget(bool bind){
	return mImpl->GlowTarget(bind);
}

void RenderStrategyDefault::DepthTexture(bool bind){
	return mImpl->DepthTexture(bind);
}