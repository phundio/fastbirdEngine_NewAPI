/*
 -----------------------------------------------------------------------------
 This source file is part of fastbird engine
 For the latest info, see http://www.jungwan.net/
 
 Copyright (c) 2013-2015 Jungwan Byun
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 -----------------------------------------------------------------------------
*/

//----------------------------------------------------------------------------
// File: font.hlsl
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Constant Buffer
//----------------------------------------------------------------------------
#include "Constants.h"

//----------------------------------------------------------------------------
// Textures
//----------------------------------------------------------------------------
Texture2D  gFontTexture : register(t0);
//{
	//Filter = MIN_MAG_MIP_POINT;
//};

//----------------------------------------------------------------------------
// Vertex Shader
//----------------------------------------------------------------------------
struct a2v
{
	float4 pos		: POSITION;
	float4 color	: COLOR;
	float2  uv		: TEXCOORD;       //  8
	int4  channel   : BLENDINDICES;    //  4
};

//----------------------------------------------------------------------------
struct v2p
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;
	float2 uv		: TEXCOORD0;
    int4   channel  : TEXCOORD1;  
};

//----------------------------------------------------------------------------
v2p font_VertexShader( in a2v IN )
{
    v2p OUT;

	OUT.pos = mul(gWorldViewProj, IN.pos);
	OUT.color = IN.color;
	OUT.uv = IN.uv;
	OUT.channel = IN.channel;

	return OUT;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 font_PixelShader( in v2p IN ) : SV_Target
{
	float4 color = gFontTexture.Sample(gLinearSampler, IN.uv);
	if( dot(vector<int, 4>(1,1,1,1), IN.channel) )
    {
        // Get the pixel value
		float val = dot(color, IN.channel);
		
        // A value above .5 is part of the actual character glyph
        // A value below .5 is part of the glyph outline
		color.rgb = val > 0.5 ? 2*val-1 : 0;
		color.a   = val > 0.5 ? 1 : 2*val;		
    }
	color *= IN.color;
	return color;
}