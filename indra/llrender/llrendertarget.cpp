/** 
 * @file llrendertarget.cpp
 * @brief LLRenderTarget implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llrendertarget.h"
#include "llrender.h"
#include "llgl.h"

LLRenderTarget* LLRenderTarget::sBoundTarget = NULL;



void check_framebuffer_status()
{
	if (gDebugGL)
	{
		GLenum status = glCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT);
		switch (status)
		{
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			break;
		default:
			llwarns << "check_framebuffer_status failed -- " << std::hex << status << llendl;
			ll_fail("check_framebuffer_status failed");	
			break;
		}
	}
}

bool LLRenderTarget::sUseFBO = false;

LLRenderTarget::LLRenderTarget() :
	mResX(0),
	mResY(0),
	mTex(0),
	mFBO(0),
	mDepth(0),
	mStencil(0),
	mUseDepth(false),
	mRenderDepth(false),
	mUsage(LLTexUnit::TT_TEXTURE),
	mSamples(0),
	mSampleBuffer(NULL)
{
}

LLRenderTarget::~LLRenderTarget()
{
	release();
}


void LLRenderTarget::setSampleBuffer(LLMultisampleBuffer* buffer)
{
	mSampleBuffer = buffer;
}

bool LLRenderTarget::allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil, LLTexUnit::eTextureType usage, bool use_fbo)
{
	stop_glerror();
	release();
	stop_glerror();

	mResX = resx;
	mResY = resy;

	mStencil = stencil;
	mUsage = usage;
	mUseDepth = depth;


	if ((sUseFBO || use_fbo) && gGLManager.mHasFramebufferObject)
	{
		if (depth)
		{
			if (!allocateDepth())
			{
				llwarns << "Failed to allocate depth buffer for render target." << llendl;
				return false;
			}
		}

		glGenFramebuffersEXT(1, (GLuint *) &mFBO);

		if (mDepth)
		{
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
			if (mStencil)
			{
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);
				stop_glerror();
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);
				stop_glerror();
			}
			else
			{
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
				stop_glerror();
			}
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		}
		
		stop_glerror();
	}

	return addColorAttachment(color_fmt);
}

bool LLRenderTarget::addColorAttachment(U32 color_fmt)
{
	if (color_fmt == 0)
	{
		return true;
	}

	U32 offset = mTex.size();
	if (offset >= 4 ||
		(offset > 0 && (mFBO == 0 || !gGLManager.mHasDrawBuffers)))
	{
		llerrs << "Too many color attachments!" << llendl;
	}

	U32 tex;
	LLImageGL::generateTextures(1, &tex);
	gGL.getTexUnit(0)->bindManual(mUsage, tex);

	stop_glerror();

	{
		clear_glerror();
		LLImageGL::setManualImage(LLTexUnit::getInternalType(mUsage), 0, color_fmt, mResX, mResY, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		if (glGetError() != GL_NO_ERROR)
		{
			llwarns << "Could not allocate color buffer for render target." << llendl;
			return false;
		}
	}

	stop_glerror();

	{
		if (offset == 0)
		{ //use bilinear filtering on single texture render targets that aren't multisampled
			gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
			stop_glerror();
		}
		else
		{ //don't filter data attachments
			gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
			stop_glerror();
		}

		if (mUsage != LLTexUnit::TT_RECT_TEXTURE)
		{
			gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_MIRROR);
			stop_glerror();
		}
		else
		{
			// ATI doesn't support mirrored repeat for rectangular textures.
			gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
			stop_glerror();
		}
	}

	if (mFBO)
	{
		stop_glerror();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+offset,
			LLTexUnit::getInternalType(mUsage), tex, 0);
			stop_glerror();

		check_framebuffer_status();
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	mTex.push_back(tex);

	if (gDebugGL)
	{ //bind and unbind to validate target
		bindTarget();
		flush();
	}

	return true;
}

bool LLRenderTarget::allocateDepth()
{
	if (mStencil)
	{
		//use render buffers where stencil buffers are in play
		glGenRenderbuffersEXT(1, (GLuint *) &mDepth);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mDepth);
		stop_glerror();
		clear_glerror();
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, mResX, mResY);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	}
	else
	{
		LLImageGL::generateTextures(1, &mDepth);
		gGL.getTexUnit(0)->bindManual(mUsage, mDepth);
		U32 internal_type = LLTexUnit::getInternalType(mUsage);
		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
		stop_glerror();
		clear_glerror();
		LLImageGL::setManualImage(internal_type, 0, GL_DEPTH_COMPONENT32_ARB, mResX, mResY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
	}

	if (glGetError() != GL_NO_ERROR)
	{
		llwarns << "Unable to allocate depth buffer for render target." << llendl;
		return false;
	}

	return true;
}

void LLRenderTarget::shareDepthBuffer(LLRenderTarget& target)
{
	if (!mFBO || !target.mFBO)
	{
		llerrs << "Cannot share depth buffer between non FBO render targets." << llendl;
	}

	if (target.mDepth)
	{
		llerrs << "Attempting to override existing depth buffer.  Detach existing buffer first." << llendl;
	}

	if (target.mUseDepth)
	{
		llerrs << "Attempting to override existing shared depth buffer. Detach existing buffer first." << llendl;
	}

	if (mDepth)
	{
		stop_glerror();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, target.mFBO);
		stop_glerror();

		if (mStencil)
		{
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);
			stop_glerror();
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);			
			stop_glerror();
			target.mStencil = true;
		}
		else
		{
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, LLTexUnit::getInternalType(mUsage), mDepth, 0);
			stop_glerror();
		}
		check_framebuffer_status();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		target.mUseDepth = true;
	}
}

void LLRenderTarget::release()
{
	if (mDepth)
	{
		if (mStencil)
		{
			glDeleteRenderbuffersEXT(1, (GLuint*) &mDepth);
			stop_glerror();
		}
		else
		{
			LLImageGL::deleteTextures(1, &mDepth, true);
			stop_glerror();
		}
		mDepth = 0;
	}
	else if (mUseDepth && mFBO)
	{ //detach shared depth buffer
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
		if (mStencil)
		{ //attached as a renderbuffer
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
			mStencil = false;
		}
		else
		{ //attached as a texture
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, LLTexUnit::getInternalType(mUsage), 0, 0);
		}
		mUseDepth = false;
	}

	if (mFBO)
	{
		glDeleteFramebuffersEXT(1, (GLuint *) &mFBO);
		mFBO = 0;
	}

	if (mTex.size() > 0)
	{
		LLImageGL::deleteTextures(mTex.size(), &mTex[0], true);
		mTex.clear();
	}

	mResX = mResY = 0;

	mSampleBuffer = NULL;
	sBoundTarget = NULL;
}

void LLRenderTarget::bindTarget()
{
	if (mFBO)
	{
		stop_glerror();
		if (mSampleBuffer)
		{
			mSampleBuffer->bindTarget(this);
			stop_glerror();
		}
		else
		{
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
			stop_glerror();
			if (gGLManager.mHasDrawBuffers)
			{ //setup multiple render targets
				GLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0_EXT,
										GL_COLOR_ATTACHMENT1_EXT,
										GL_COLOR_ATTACHMENT2_EXT,
										GL_COLOR_ATTACHMENT3_EXT};
				glDrawBuffersARB(mTex.size(), drawbuffers);
			}
			
			if (mTex.empty())
			{ //no color buffer to draw to
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			}

			check_framebuffer_status();

			stop_glerror();
		}
	}

	glViewport(0, 0, mResX, mResY);
	sBoundTarget = this;
}

// static
void LLRenderTarget::unbindTarget()
{
	if (gGLManager.mHasFramebufferObject)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	sBoundTarget = NULL;
}

void LLRenderTarget::clear(U32 mask_in)
{
	U32 mask = GL_COLOR_BUFFER_BIT;
	if (mUseDepth)
	{
		mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	}
	if (mFBO)
	{
		check_framebuffer_status();
		stop_glerror();
		glClear(mask & mask_in);
		stop_glerror();
	}
	else
	{
		LLGLEnable scissor(GL_SCISSOR_TEST);
		glScissor(0, 0, mResX, mResY);
		stop_glerror();
		glClear(mask & mask_in);
	}
}

U32 LLRenderTarget::getTexture(U32 attachment) const
{
	if (attachment > mTex.size()-1)
	{
		llerrs << "Invalid attachment index." << llendl;
	}
	if (mTex.empty())
	{
		return 0;
	}
	return mTex[attachment];
}

void LLRenderTarget::bindTexture(U32 index, S32 channel)
{
	gGL.getTexUnit(channel)->bindManual(mUsage, getTexture(index));
}

void LLRenderTarget::flush(bool fetch_depth)
{
	gGL.flush();
	if (!mFBO)
	{
		gGL.getTexUnit(0)->bind(this);
		glCopyTexSubImage2D(LLTexUnit::getInternalType(mUsage), 0, 0, 0, 0, 0, mResX, mResY);

		if (fetch_depth)
		{
			if (!mDepth)
			{
				allocateDepth();
			}

			gGL.getTexUnit(0)->bind(this);
			glCopyTexImage2D(LLTexUnit::getInternalType(mUsage), 0, GL_DEPTH24_STENCIL8_EXT, 0, 0, mResX, mResY, 0);
		}

		gGL.getTexUnit(0)->disable();
	}
	else
	{
		stop_glerror();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		stop_glerror();
	
		if (mSampleBuffer)
		{
			LLGLEnable multisample(GL_MULTISAMPLE_ARB);
			stop_glerror();
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
			stop_glerror();
			check_framebuffer_status();
			glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, mSampleBuffer->mFBO);
			check_framebuffer_status();
			
			stop_glerror();
			if(gGLManager.mIsATI)
			{
				glBlitFramebufferEXT(0, 0, mResX, mResY, 0, 0, mResX, mResY, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
				glBlitFramebufferEXT(0, 0, mResX, mResY, 0, 0, mResX, mResY, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
			}
			else
			{
				glBlitFramebufferEXT(0, 0, mResX, mResY, 0, 0, mResX, mResY, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
			}
			stop_glerror();		

			if (mTex.size() > 1)
			{		
				for (U32 i = 1; i < mTex.size(); ++i)
				{
					glFramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
										LLTexUnit::getInternalType(mUsage), mTex[i], 0);
					stop_glerror();
					glFramebufferRenderbufferEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, mSampleBuffer->mTex[i]);
					stop_glerror();
					glBlitFramebufferEXT(0, 0, mResX, mResY, 0, 0, mResX, mResY, GL_COLOR_BUFFER_BIT, GL_NEAREST);		
					stop_glerror();
				}

				for (U32 i = 0; i < mTex.size(); ++i)
				{
					glFramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+i,
										LLTexUnit::getInternalType(mUsage), mTex[i], 0);
					stop_glerror();
					glFramebufferRenderbufferEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+i, GL_RENDERBUFFER_EXT, mSampleBuffer->mTex[i]);
					stop_glerror();
				}
			}
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		}
	}
}

void LLRenderTarget::copyContents(LLRenderTarget& source, S32 srcX0, S32 srcY0, S32 srcX1, S32 srcY1,
						S32 dstX0, S32 dstY0, S32 dstX1, S32 dstY1, U32 mask, U32 filter)
{
	GLboolean write_depth = mask & GL_DEPTH_BUFFER_BIT ? TRUE : FALSE;

	LLGLDepthTest depth(write_depth, write_depth);

	gGL.flush();
	if (!source.mFBO || !mFBO)
	{
		llerrs << "Cannot copy framebuffer contents for non FBO render targets." << llendl;
	}

	if (mSampleBuffer)
	{
		mSampleBuffer->copyContents(source, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	}
	else
	{
		if (mask == GL_DEPTH_BUFFER_BIT && source.mStencil != mStencil)
		{
			stop_glerror();
		
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, source.mFBO);
			check_framebuffer_status();
			gGL.getTexUnit(0)->bind(this, true);
			stop_glerror();
			glCopyTexSubImage2D(LLTexUnit::getInternalType(mUsage), 0, srcX0, srcY0, dstX0, dstY0, dstX1, dstY1);
			stop_glerror();
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			stop_glerror();
		}
		else
		{
			glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, source.mFBO);
			stop_glerror();
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, mFBO);
			stop_glerror();
			check_framebuffer_status();
			stop_glerror();
			if(gGLManager.mIsATI && mask & GL_STENCIL_BUFFER_BIT)
			{
				mask &= ~GL_STENCIL_BUFFER_BIT;
				glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GL_STENCIL_BUFFER_BIT, filter);
			}
			if(mask)
				glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
			stop_glerror();
			glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
			stop_glerror();
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
			stop_glerror();
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			stop_glerror();
		}
	}
}

//static
void LLRenderTarget::copyContentsToFramebuffer(LLRenderTarget& source, S32 srcX0, S32 srcY0, S32 srcX1, S32 srcY1,
						S32 dstX0, S32 dstY0, S32 dstX1, S32 dstY1, U32 mask, U32 filter)
{
	if (!source.mFBO)
	{
		llerrs << "Cannot copy framebuffer contents for non FBO render targets." << llendl;
	}
	{
		GLboolean write_depth = mask & GL_DEPTH_BUFFER_BIT ? TRUE : FALSE;

		LLGLDepthTest depth(write_depth, write_depth);
		
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, source.mSampleBuffer ? source.mSampleBuffer->mFBO : source.mFBO);
		stop_glerror();
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
		stop_glerror();
		check_framebuffer_status();
		stop_glerror();
		if(gGLManager.mIsATI && mask & GL_STENCIL_BUFFER_BIT)
		{
			mask &= ~GL_STENCIL_BUFFER_BIT;
			glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GL_STENCIL_BUFFER_BIT, filter);
		}
		if(mask)
			glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
		stop_glerror();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		stop_glerror();
	}
}

bool LLRenderTarget::isComplete() const
{
	return (!mTex.empty() || mDepth) ? true : false;
}

void LLRenderTarget::getViewport(S32* viewport)
{
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = mResX;
	viewport[3] = mResY;
}

//==================================================
// LLMultisampleBuffer implementation
//==================================================
LLMultisampleBuffer::LLMultisampleBuffer()
{

}

LLMultisampleBuffer::~LLMultisampleBuffer()
{
	release();
}

void LLMultisampleBuffer::release()
{
	if (mFBO)
	{
		glDeleteFramebuffersEXT(1, (GLuint *) &mFBO);
		mFBO = 0;
	}

	if (mTex.size() > 0)
	{
		glDeleteRenderbuffersEXT(mTex.size(), (GLuint *) &mTex[0]);
		mTex.clear();
	}

	if (mDepth)
	{
		glDeleteRenderbuffersEXT(1, (GLuint *) &mDepth);
		mDepth = 0;
	}
}

void LLMultisampleBuffer::bindTarget()
{
	bindTarget(this);
}

void LLMultisampleBuffer::bindTarget(LLRenderTarget* ref)
{
	if (!ref)
	{
		ref = this;
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
	if (gGLManager.mHasDrawBuffers)
	{ //setup multiple render targets
		GLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0_EXT,
								GL_COLOR_ATTACHMENT1_EXT,
								GL_COLOR_ATTACHMENT2_EXT,
								GL_COLOR_ATTACHMENT3_EXT};
		glDrawBuffersARB(ref->mTex.size(), drawbuffers);
	}

	check_framebuffer_status();

	glViewport(0, 0, mResX, mResY);

	sBoundTarget = this;
}

bool LLMultisampleBuffer::allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil,  LLTexUnit::eTextureType usage, bool use_fbo )
{
	return allocate(resx,resy,color_fmt,depth,stencil,usage,use_fbo,2);
}

bool LLMultisampleBuffer::allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, bool stencil,  LLTexUnit::eTextureType usage, bool use_fbo, U32 samples )
{
	release();
	stop_glerror();

	mResX = resx;
	mResY = resy;

	mUsage = usage;
	mUseDepth = depth;
	mStencil = stencil;

	if (!gGLManager.mHasFramebufferMultisample)
	{
		llerrs << "Attempting to allocate unsupported render target type!" << llendl;
		return false;
	}

	mSamples = samples;
	
	if (mSamples <= 1)
	{
		llerrs << "Cannot create a multisample buffer with less than 2 samples." << llendl;
		return false;
	}

	stop_glerror();

	if ((sUseFBO || use_fbo) && gGLManager.mHasFramebufferObject)
	{

		if (depth)
		{
			stop_glerror();
			if(!allocateDepth())
				return false;
			stop_glerror();
		}
		glGenFramebuffersEXT(1, (GLuint *) &mFBO);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
		stop_glerror();
		clear_glerror();
		if (mDepth)
		{
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);
			if (mStencil)
			{
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepth);			
			}
			check_framebuffer_status();
		}
		
		stop_glerror();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		stop_glerror();
	}

	return addColorAttachment(color_fmt);
}

bool LLMultisampleBuffer::addColorAttachment(U32 color_fmt)
{
	if (color_fmt == 0)
	{
		return true;
	}

	U32 offset = mTex.size();
	if (offset >= 4 ||
		(offset > 0 && (mFBO == 0 || !gGLManager.mHasDrawBuffers)))
	{
		llerrs << "Too many color attachments!" << llendl;
	}

	U32 tex;
	glGenRenderbuffersEXT(1, &tex);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, tex);
	stop_glerror();
	clear_glerror();
	glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, mSamples, color_fmt, mResX, mResY);
	if (glGetError() != GL_NO_ERROR)
	{
		llwarns << "Unable to allocate color buffer for multisample render target." << llendl;
		return false;
	}
	if (mFBO)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBO);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+offset, GL_RENDERBUFFER_EXT, tex);
		check_framebuffer_status();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	mTex.push_back(tex);
	return true;
}

bool LLMultisampleBuffer::allocateDepth()
{
	glGenRenderbuffersEXT(1, (GLuint* ) &mDepth);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mDepth);
	stop_glerror();
	clear_glerror();
	if (mStencil)
	{
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, mSamples, GL_DEPTH24_STENCIL8_EXT, mResX, mResY);	
	}
	else
	{
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, mSamples, GL_DEPTH_COMPONENT16_ARB, mResX, mResY);	
	}
	if (glGetError() != GL_NO_ERROR)
	{
		llwarns << "Unable to allocate depth buffer for multisample render target." << llendl;
		return false;
	}
	return true;
}

