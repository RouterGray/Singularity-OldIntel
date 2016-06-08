/**
* @file floatervoicelicense.cpp
* @brief prompts user to agree to the Vivox license in order to enable voice
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
*
* Copyright (c) 2010, McCabe Maxsted
*
* Imprudence Viewer Source Code
* The source code in this file ("Source Code") is provided to you
* under the terms of the GNU General Public License, version 2.0
* ("GPL"). Terms of the GPL can be found in doc/GPL-license.txt in
* this distribution, or online at
* http://secondlifegrid.net/programs/open_source/licensing/gplv2
*
* There are special exceptions to the terms and conditions of the GPL as
* it is applied to this Source Code. View the full text of the exception
* in the file doc/FLOSS-exception.txt in this software distribution, or
* online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
*
* By copying, modifying or distributing this software, you acknowledge
* that you have read and understood your obligations described above,
* and agree to abide by those obligations.
*
* ALL SOURCE CODE IS PROVIDED "AS IS." THE AUTHOR MAKES NO
* WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
* COMPLETENESS OR PERFORMANCE.
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h"

#include "floatervoicelicense.h"

// viewer includes
#include "llagent.h"
#include "llappviewer.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"

// linden library includes
#include "llbutton.h"
#include "llcorehttputil.h"
#include "llradiogroup.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llvfile.h"
#include "message.h"

FloaterVoiceLicense::FloaterVoiceLicense(const LLSD& key)
:	LLModalDialog( std::string(" "), 100, 100 ),
	mLoadCompleteCount( 0 )
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_voice_license.xml");
}

BOOL FloaterVoiceLicense::postBuild()
{	
	childSetAction("Continue", onContinue, this);
	childSetAction("Cancel", onCancel, this);
	childSetCommitCallback("agree_chk", updateAgree, this);

	// disable Agree to License radio button until the page has fully loaded
	LLCheckBoxCtrl* license_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
	license_agreement->setEnabled( false );

	// hide the SL text widget if we're displaying license with using a browser widget.
	LLTextEditor *editor = getChild<LLTextEditor>("license_text");
	editor->setVisible( FALSE );

	LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("license_html");
	if ( web_browser )
	{
		// start to observe it so we see navigate complete events
		web_browser->addObserver( this );
		std::string url = getString( "real_url" );
		if (url.substr(0, 4) == "http")
		{
			LLHandle<LLFloater> handle = getHandle();

			LLCoros::instance().launch("LLFloaterTOS::testSiteIsAliveCoro",
				boost::bind(&FloaterVoiceLicense::testSiteIsAliveCoro, handle, url));
		}
		else
			setSiteIsAlive(false);
	}

	return TRUE;
}

void FloaterVoiceLicense::setSiteIsAlive( bool alive )
{
	LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("license_html");
	// if the contents of the site was retrieved
	if ( alive )
	{
		if ( web_browser )
		{
			// navigate to the "real" page 
			std::string real_url = getString( "real_url" );
			web_browser->navigateTo(real_url);
		}
	}
	else
	{
		web_browser->navigateToLocalPage("license", getString("fallback_html"));
		// normally this is set when navigation to license page completes (so you can't accept before it loads)
		// but if the page is unavailable, we need to do this now
		LLCheckBoxCtrl* license_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
		license_agreement->setEnabled( true );
	}
}

FloaterVoiceLicense::~FloaterVoiceLicense()
{
}

// virtual
void FloaterVoiceLicense::draw()
{
	// draw children
	LLModalDialog::draw();
}

// static
void FloaterVoiceLicense::updateAgree(LLUICtrl*, void* userdata )
{
	FloaterVoiceLicense* self = (FloaterVoiceLicense*) userdata;
	bool agree = self->childGetValue("agree_chk").asBoolean(); 
	self->childSetEnabled("Continue", agree);
}

// static
void FloaterVoiceLicense::onContinue( void* userdata )
{
	FloaterVoiceLicense* self = (FloaterVoiceLicense*) userdata;
	LL_INFOS() << "User agreed to the Vivox personal license" << LL_ENDL;

	// enabling voice by default here seems like the best behavior
	gSavedSettings.setBOOL("EnableVoiceChat", TRUE);
	gSavedSettings.setBOOL("VivoxLicenseAccepted", TRUE);

	// save these settings in case something bad happens later
	gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);

	if (LLStartUp::getStartupState() == STATE_LOGIN_VOICE_LICENSE)
	{
		LLStartUp::setStartupState( STATE_LOGIN_AUTH_INIT );			// Go back and finish authentication
	}
	self->close(); // destroys this object
}

// static
void FloaterVoiceLicense::onCancel( void* userdata )
{
	FloaterVoiceLicense* self = (FloaterVoiceLicense*) userdata;
	LL_INFOS() << "User disagreed with the vivox personal license" << LL_ENDL;
	gSavedSettings.setBOOL("EnableVoiceChat", FALSE);
	gSavedSettings.setBOOL("VivoxLicenseAccepted", FALSE);
	
	if (LLStartUp::getStartupState() == STATE_LOGIN_VOICE_LICENSE)
	{
		LLStartUp::setStartupState( STATE_LOGIN_AUTH_INIT );			// Go back and finish authentication
	}
	self->mLoadCompleteCount = 0;  // reset counter for next time we come here
	self->close(); // destroys this object
}

//virtual 
void FloaterVoiceLicense::handleMediaEvent(LLPluginClassMedia* /*self*/, EMediaEvent event)
{
	if(event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{
		// skip past the loading screen navigate complete
		if ( ++mLoadCompleteCount == 2 )
		{
			LL_INFOS() << "NAVIGATE COMPLETE" << LL_ENDL;
			// enable Agree to License radio button now that page has loaded
			LLCheckBoxCtrl * license_agreement = getChild<LLCheckBoxCtrl>("agree_chk");
			license_agreement->setEnabled( true );
		}
	}
}

void FloaterVoiceLicense::testSiteIsAliveCoro(LLHandle<LLFloater> handle, std::string url)
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

	FloaterVoiceLicense *that = dynamic_cast<FloaterVoiceLicense *>(handle.get());

	if (that)
		that->setSiteIsAlive(static_cast<bool>(status));
	else
	{
		LL_WARNS("testSiteIsAliveCoro") << "Handle was not a TOS floater." << LL_ENDL;
	}
}
