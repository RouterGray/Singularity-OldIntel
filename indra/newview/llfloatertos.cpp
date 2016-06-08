/** 
 * @file llfloatertos.cpp
 * @brief Terms of Service Agreement dialog
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llfloatertos.h"

// viewer includes
#include "llagent.h"
#include "llappviewer.h"
#include "llstartup.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"

// linden library includes
#include "llbutton.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llvfile.h"
#include "message.h"
#include "llcorehttputil.h"

// static 
LLFloaterTOS* LLFloaterTOS::sInstance = NULL;

// static
LLFloaterTOS* LLFloaterTOS::show(ETOSType type, const std::string & message)
{
	if( !LLFloaterTOS::sInstance )
	{
		LLFloaterTOS::sInstance = new LLFloaterTOS(type, message);
	}

	if (type == TOS_TOS)
	{
		LLUICtrlFactory::getInstance()->buildFloater(LLFloaterTOS::sInstance, "floater_tos.xml");
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildFloater(LLFloaterTOS::sInstance, "floater_critical.xml");
	}

	return LLFloaterTOS::sInstance;
}


LLFloaterTOS::LLFloaterTOS(ETOSType type, const std::string & message)
:	LLModalDialog( std::string(" "), 100, 100 ),
	mType(type),
	mMessage(message),
	mLoadCompleteCount( 0 )
{
}

BOOL LLFloaterTOS::postBuild()
{	
	childSetAction("Continue", onContinue, this);
	childSetAction("Cancel", onCancel, this);
	childSetCommitCallback("agree_chk", updateAgree, this);

	if ( mType != TOS_TOS )
	{
		// this displays the critical message
		LLTextEditor *editor = getChild<LLTextEditor>("tos_text");
		editor->setHandleEditKeysDirectly( TRUE );
		editor->setEnabled( FALSE );
		editor->setWordWrap(TRUE);
		editor->setFocus(TRUE);
		editor->setValue(LLSD(mMessage));

		return TRUE;
	}

	// disable Agree to TOS radio button until the page has fully loaded
	LLCheckBoxCtrl* tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
	tos_agreement->setEnabled( false );

	// hide the SL text widget if we're displaying TOS with using a browser widget.
	LLTextEditor *editor = getChild<LLTextEditor>("tos_text");
	editor->setVisible( FALSE );

	LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("tos_html");
	if ( web_browser )
	{
		web_browser->addObserver(this);


        std::string url(getString("real_url"));

        LLHandle<LLFloater> handle = getHandle();

        LLCoros::instance().launch("LLFloaterTOS::testSiteIsAliveCoro",
            boost::bind(&LLFloaterTOS::testSiteIsAliveCoro, handle, url));

	}

	return TRUE;
}

void LLFloaterTOS::setSiteIsAlive( bool alive )
{
	// only do this for TOS pages
	if ( mType == TOS_TOS )
	{
		LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("tos_html");
		// if the contents of the site was retrieved
		if ( alive )
		{
			if ( web_browser )
			{
				// navigate to the "real" page 
				web_browser->navigateTo( getString( "real_url" ) );
			};
		}
		else
		{
			LL_INFOS("TOS") << "ToS page: ToS page unavailable!" << LL_ENDL;
			// normally this is set when navigation to TOS page navigation completes (so you can't accept before TOS loads)
			// but if the page is unavailable, we need to do this now
			LLCheckBoxCtrl* tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
			tos_agreement->setEnabled( true );
		}
	}
}

LLFloaterTOS::~LLFloaterTOS()
{
	sInstance = NULL;
}

// virtual
void LLFloaterTOS::draw()
{
	// draw children
	LLModalDialog::draw();
}

// static
void LLFloaterTOS::updateAgree(LLUICtrl*, void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	bool agree = self->childGetValue("agree_chk").asBoolean(); 
	self->childSetEnabled("Continue", agree);
}

// static
void LLFloaterTOS::onContinue( void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	LL_INFOS() << "User agrees with TOS." << LL_ENDL;
	if (self->mType == TOS_TOS)
	{
		gAcceptTOS = TRUE;
	}
	else
	{
		gAcceptCriticalMessage = TRUE;
	}

	// Testing TOS dialog
	#if ! LL_RELEASE_FOR_DOWNLOAD		
	if ( LLStartUp::getStartupState() == STATE_LOGIN_WAIT )
	{
		LLStartUp::setStartupState( STATE_LOGIN_SHOW );
	}
	else 
	#endif

	LLStartUp::setStartupState( STATE_LOGIN_AUTH_INIT );			// Go back and finish authentication
	self->close(); // destroys this object
}

// static
void LLFloaterTOS::onCancel( void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	LL_INFOS() << "User disagrees with TOS." << LL_ENDL;
	LLNotificationsUtil::add("MustAgreeToLogIn", LLSD(), LLSD(), login_alert_done);
	LLStartUp::setStartupState( STATE_LOGIN_SHOW );
	self->mLoadCompleteCount = 0;  // reset counter for next time we come to TOS
	self->close(); // destroys this object
}

//virtual 
void LLFloaterTOS::handleMediaEvent(LLPluginClassMedia* /*self*/, EMediaEvent event)
{
	if(event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{
		// skip past the loading screen navigate complete
		if ( ++mLoadCompleteCount == 2 )
		{
			LL_INFOS("TOS") << "TOS: NAVIGATE COMPLETE" << LL_ENDL;
			// enable Agree to TOS radio button now that page has loaded
			LLCheckBoxCtrl * tos_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
			tos_agreement->setEnabled( true );
		}
	}
}

void LLFloaterTOS::testSiteIsAliveCoro(LLHandle<LLFloater> handle, std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);


    httpOpts->setWantHeaders(true);


    LL_INFOS("testSiteIsAliveCoro") << "Generic POST for " << url << LL_ENDL;

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (handle.isDead())
    {
        LL_WARNS("testSiteIsAliveCoro") << "Dialog canceled before response." << LL_ENDL;
        return;
    }

    LLFloaterTOS *that = dynamic_cast<LLFloaterTOS *>(handle.get());
    
    if (that)
        that->setSiteIsAlive(static_cast<bool>(status)); 
    else
    {
        LL_WARNS("testSiteIsAliveCoro") << "Handle was not a TOS floater." << LL_ENDL;
    }
}


