#include "stdafx.h"
#include "TextureAtlas.h"
#include "Renderer.h"
#include "Texture.h"
#include "TinyXmlLib/tinyxml2.h"

namespace fastbird{
	void TextureAtlasRegion::GetQuadUV(Vec2 uv[])
	{
		uv[0] = Vec2(mUVStart.x, mUVEnd.y);
		uv[1] = mUVStart;
		uv[2] = mUVEnd;
		uv[3] = Vec2(mUVEnd.x, mUVStart.y);
	}

	const Vec2& TextureAtlasRegion::GetStartUV() const { 
		return mUVStart; 
	}

	const Vec2 TextureAtlasRegion::GetUVSize() const { 
		return mUVEnd - mUVStart; 
	}
	const Vec2I& TextureAtlasRegion::GetSize() const {
		return mSize;
	}

	//-----------------------------------------------------------------------
	TextureAtlasRegion* TextureAtlas::AddRegion(const char* name)
	{
		if (!name || strlen(name) == 0){
			Logger::Log(FB_DEFAULT_LOG_ARG, "invalid arg");
			return 0;
		}
		auto it = mRegions.Find(name);
		if (it != mRegions.end()){
			Logger::Log(FB_DEFAULT_LOG_ARG, "already existing. returning it.");
			return it->second;
		}

		TextureAtlasRegion* region = FB_NEW(TextureAtlasRegion);
		region->mName = name;
		mRegions.Insert(std::make_pair(region->mName, region));
	}

	TextureAtlasRegion* TextureAtlas::GetRegion(const char* name)
	{
		if (!name || strlen(name) == 0){
			Logger::Log(FB_DEFAULT_LOG_ARG, "invalid arg");
			return 0;
		}

		REGION_MAP::iterator it = mRegions.Find(name);
		if (it != mRegions.end())
		{
			return it->second;
		}
		else
		{
			Logger::Log(FB_DEFAULT_LOG_ARG, "region name(%s) is not found", name);
			return 0;
		}
	}

	bool TextureAtlas::ReloadTextureAtlas()
	{
		auto renderer = Renderer::GetRenderer();
		if (!renderer){
			Logger::Log(FB_DEFAULT_LOG_ARG, "no renderer.");
		}
		tinyxml2::XMLDocument doc;
		doc.LoadFile(mPath.c_str());
		if (doc.Error()) {
			const char* errMsg = doc.GetErrorStr1();
			if (errMsg)
				Logger::Log(FB_ERROR_LOG_ARG, FormatString("texture atlas error : \t%s", errMsg));
			else
				Logger::Log(FB_ERROR_LOG_ARG, "ReloadTextureAtlas Failed");
			return false;
		}
		tinyxml2::XMLElement* pRoot = doc.FirstChildElement("TextureAtlas");
		if (!pRoot) {
			Logger::Log(FB_ERROR_LOG_ARG, "Invalid TextureAtlas format!");
			return false;
		}

		const char* szBuffer = pRoot->Attribute("file");
		TexturePtr texture;
		if (szBuffer) {
			mTexture = renderer->CreateTexture(szBuffer, true);
			if (!mTexture)
			{
				Logger::Log(FB_ERROR_LOG_ARG, FormatString("Texture %s not found.", szBuffer));
			}
		}
		else {
			Logger::Log(FB_ERROR_LOG_ARG, "Invalid TextureAtlas format! No Texture Defined.");
			return false;
		}

		Vec2I textureSize = mTexture->GetSize();
		if (textureSize.x != 0 && textureSize.y != 0) {
			tinyxml2::XMLElement* pRegionElem = pRoot->FirstChildElement("region");
			while (pRegionElem) {
				szBuffer = pRegionElem->Attribute("name");
				if (!szBuffer)
				{
					Logger::Log(FB_ERROR_LOG_ARG, "No name for texture atlas region");
					continue;
				}
				TextureAtlasRegion* region = AddRegion(szBuffer);
				if (!region)
				{
					continue;					
				}
				region->mID = pRegionElem->UnsignedAttribute("id");
				region->mStart.x = pRegionElem->IntAttribute("x");
				region->mStart.y = pRegionElem->IntAttribute("y");
				region->mSize.x = pRegionElem->IntAttribute("width");
				region->mSize.y = pRegionElem->IntAttribute("height");
				Vec2 start((float)region->mStart.x, (float)region->mStart.y);
				Vec2 end(start.x + region->mSize.x, start.y + region->mSize.y);
				region->mUVStart = start / textureSize;
				region->mUVEnd = end / textureSize;
				pRegionElem = pRegionElem->NextSiblingElement();
			}
			return true;
		}
		else {
			Logger::Log(FB_ERROR_LOG_ARG, "Texture size is 0,0");
			return false;
		}
	}
}