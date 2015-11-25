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

#include "stdafx.h"
#include "GaussianDistribution.h"
#include "Renderer.h"
namespace fastbird
{
	void GaussianDist::Calc(unsigned width, unsigned height, float devi, float multiplier)
	{
		auto& renderer = Renderer::GetInstance();
		float afSampleOffsets[15];
		renderer.GetSampleOffsets_Bloom(width, afSampleOffsets, mGaussianDistWeightX, devi, multiplier);
		for (int i = 0; i < 15; i++)
		{
			mGaussianDistOffsetX[i] = Vec4(afSampleOffsets[i], 0.0f, 0.0f, 0.0f);
		}
		renderer.GetSampleOffsets_Bloom(height, afSampleOffsets, mGaussianDistWeightY, devi, multiplier);
		for (int i = 0; i < 15; i++)
		{
			mGaussianDistOffsetY[i] = Vec4(afSampleOffsets[i], 0.0f, 0.0f, 0.0f);
		}
	}
}