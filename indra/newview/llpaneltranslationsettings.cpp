/** 
 * @file llpaneltranslationsettings.cpp
 * @brief Machine translation settings for chat
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpaneltranslationsettings.h"

// Viewer includes
#include "llfloaterchat.h"
#include "llimpanel.h"
#include "llimview.h"
#include "lltranslate.h"
#include "llviewercontrol.h" // for gSavedSettings

// Linden library includes
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"
#include "lltexteditor.h"
#include "lluictrlfactory.h"

class EnteredKeyVerifier : public LLTranslate::KeyVerificationReceiver
{
public:
	EnteredKeyVerifier(LLTranslate::EService service, bool alert)
	:	LLTranslate::KeyVerificationReceiver(service)
	,	mAlert(alert)
	{
	}

private:
	/*virtual*/ void setVerificationStatus(bool ok)
	{
		LLPanelTranslationSettings* panel = LLPanelTranslationSettings::getInstance();
		if (!panel)
		{
			llwarns << "The translation settings panel has disappeared!" << llendl;
			return;
		}

		switch (getService())
		{
		case LLTranslate::SERVICE_BING:
			panel->setBingVerified(ok, mAlert);
			break;
		case LLTranslate::SERVICE_GOOGLE:
			panel->setGoogleVerified(ok, mAlert);
			break;
		}
	}
	/*virtual*/ const char* getName() const { return "EnteredKeyVerifier"; }

	bool mAlert;
};

LLPanelTranslationSettings::LLPanelTranslationSettings()
:	mMachineTranslationCB(NULL)
,	mLanguageCombo(NULL)
,	mTranslationServiceRadioGroup(NULL)
,	mBingAPIKeyEditor(NULL)
,	mGoogleAPIKeyEditor(NULL)
,	mBingVerifyBtn(NULL)
,	mGoogleVerifyBtn(NULL)
,	mBingKeyVerified(false)
,	mGoogleKeyVerified(false)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_translation_settings.xml");

	mMachineTranslationCB->setValue(gSavedSettings.getBOOL("TranslateChat"));
	mLanguageCombo->setSelectedByValue(gSavedSettings.getString("TranslateLanguage"), TRUE);
	mTranslationServiceRadioGroup->setSelectedByValue(gSavedSettings.getString("TranslationService"), TRUE);

	std::string bing_key = gSavedSettings.getString("BingTranslateAPIKey");
	if (!bing_key.empty())
	{
		mBingAPIKeyEditor->setText(bing_key);
		mBingAPIKeyEditor->setTentative(FALSE);
		verifyKey(LLTranslate::SERVICE_BING, bing_key, false);
	}
	else
	{
		mBingAPIKeyEditor->setTentative(TRUE);
		mBingKeyVerified = FALSE;
	}

	std::string google_key = gSavedSettings.getString("GoogleTranslateAPIKey");
	if (!google_key.empty())
	{
		mGoogleAPIKeyEditor->setText(google_key);
		mGoogleAPIKeyEditor->setTentative(FALSE);
		verifyKey(LLTranslate::SERVICE_GOOGLE, google_key, false);
	}
	else
	{
		mGoogleAPIKeyEditor->setTentative(TRUE);
		mGoogleKeyVerified = FALSE;
	}

	updateControlsEnabledState();
}

// virtual
BOOL LLPanelTranslationSettings::postBuild()
{
	mMachineTranslationCB = getChild<LLCheckBoxCtrl>("translate_chat_checkbox");
	mLanguageCombo = getChild<LLComboBox>("translate_language_combo");
	mTranslationServiceRadioGroup = getChild<LLRadioGroup>("translation_service_rg");
	mBingAPIKeyEditor = getChild<LLLineEditor>("bing_api_key");
	mGoogleAPIKeyEditor = getChild<LLLineEditor>("google_api_key");
	mBingVerifyBtn = getChild<LLButton>("verify_bing_api_key_btn");
	mGoogleVerifyBtn = getChild<LLButton>("verify_google_api_key_btn");

	mMachineTranslationCB->setCommitCallback(boost::bind(&LLPanelTranslationSettings::updateControlsEnabledState, this));
	mTranslationServiceRadioGroup->setCommitCallback(boost::bind(&LLPanelTranslationSettings::updateControlsEnabledState, this));
	mBingVerifyBtn->setClickedCallback(boost::bind(&LLPanelTranslationSettings::onBtnBingVerify, this));
	mGoogleVerifyBtn->setClickedCallback(boost::bind(&LLPanelTranslationSettings::onBtnGoogleVerify, this));

	mBingAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLPanelTranslationSettings::onEditorFocused, this, _1));
	mBingAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLPanelTranslationSettings::onBingKeyEdited, this, _1));
	mGoogleAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLPanelTranslationSettings::onEditorFocused, this, _1));
	mGoogleAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLPanelTranslationSettings::onGoogleKeyEdited, this, _1));

	// Now set the links, because v3 has special xml linking and we do not.
	LLTextEditor* bing_api_key = getChild<LLTextEditor>("bing_api_key_label");
	LLTextEditor* google_api_key = getChild<LLTextEditor>("google_api_key_label");
	LLTextEditor* google_links = getChild<LLTextEditor>("google_links_text");
	google_api_key->setParseHTML(true);
	bing_api_key->setParseHTML(true);
	google_links->setParseHTML(true);

	LLStyleSP link_style(new LLStyle);
	link_style->setColor(gSavedSettings.getColor4("HTMLLinkColor"));
	std::string link_text;
	std::string link_url;

	link_text = " AppId:";
	link_url = "https://datamarket.azure.com/dataset/1899a118-d202-492c-aa16-ba21c33c06cb"; //Microsoft changed their policy, no longer link to "http://www.bing.com/developers/createapp.aspx";
	link_style->setLinkHREF(link_url);
	bing_api_key->appendStyledText(link_text, false, false, link_style);

	link_text = " API key:";
	link_url = "https://developers.google.com/translate/v2/getting_started#auth";
	link_style->setLinkHREF(link_url);
	google_api_key->appendStyledText(link_text, false, false, link_style);

	link_text = getString("Pricing");
	link_url = "https://developers.google.com/translate/v2/pricing";
	link_style->setLinkHREF(link_url);
	google_links->appendStyledText(link_text, false, false, link_style);
	google_links->appendColoredText(std::string(" | "), false, false, gColors.getColor("TextFgReadOnlyColor"));
	link_text = getString("Stats");
	link_url = "https://code.google.com/apis/console";
	link_style->setLinkHREF(link_url);
	google_links->appendStyledText(link_text, false, false, link_style);

	//center();
	return TRUE;
}

void LLPanelTranslationSettings::setBingVerified(bool ok, bool alert)
{
	if (alert)
	{
		showAlert(ok ? "bing_api_key_verified" : "bing_api_key_not_verified");
	}

	mBingKeyVerified = ok;
	updateControlsEnabledState();
}

void LLPanelTranslationSettings::setGoogleVerified(bool ok, bool alert)
{
	if (alert)
	{
		showAlert(ok ? "google_api_key_verified" : "google_api_key_not_verified");
	}

	mGoogleKeyVerified = ok;
	updateControlsEnabledState();
}

std::string LLPanelTranslationSettings::getSelectedService() const
{
	return mTranslationServiceRadioGroup->getSelectedValue().asString();
}

std::string LLPanelTranslationSettings::getEnteredBingKey() const
{
	return mBingAPIKeyEditor->getTentative() ? LLStringUtil::null : mBingAPIKeyEditor->getText();
}

std::string LLPanelTranslationSettings::getEnteredGoogleKey() const
{
	return mGoogleAPIKeyEditor->getTentative() ? LLStringUtil::null : mGoogleAPIKeyEditor->getText();
}

void LLPanelTranslationSettings::showAlert(const std::string& msg_name) const
{
	LLSD args;
	args["MESSAGE"] = getString(msg_name);
	LLNotificationsUtil::add("GenericAlert", args);
}

void LLPanelTranslationSettings::updateControlsEnabledState()
{
	// Enable/disable controls based on the checkbox value.
	bool on = mMachineTranslationCB->getValue().asBoolean();
	std::string service = getSelectedService();
	bool bing_selected = service == "bing";
	bool google_selected = service == "google";

	mTranslationServiceRadioGroup->setEnabled(on);
	mLanguageCombo->setEnabled(on);

	getChild<LLTextEditor>("bing_api_key_label")->setVisible(bing_selected);
	mBingAPIKeyEditor->setVisible(bing_selected);
	mBingVerifyBtn->setVisible(bing_selected);

	getChild<LLTextEditor>("google_api_key_label")->setVisible(google_selected);
	getChild<LLTextEditor>("google_links_text")->setVisible(google_selected);
	mGoogleAPIKeyEditor->setVisible(google_selected);
	mGoogleVerifyBtn->setVisible(google_selected);

	mBingAPIKeyEditor->setEnabled(on);
	mGoogleAPIKeyEditor->setEnabled(on);

	mBingVerifyBtn->setEnabled(on &&
		!mBingKeyVerified && !getEnteredBingKey().empty());
	mGoogleVerifyBtn->setEnabled(on &&
		!mGoogleKeyVerified && !getEnteredGoogleKey().empty());
}

void LLPanelTranslationSettings::verifyKey(int service, const std::string& key, bool alert)
{
	LLTranslate::KeyVerificationReceiverPtr receiver =
		new EnteredKeyVerifier((LLTranslate::EService) service, alert);
	LLTranslate::verifyKey(receiver, key);
}

void LLPanelTranslationSettings::onEditorFocused(LLFocusableElement* control)
{
	LLLineEditor* editor = dynamic_cast<LLLineEditor*>(control);
	if (editor && editor->hasTabStop()) // if enabled. getEnabled() doesn't work
	{
		if (editor->getTentative())
		{
			editor->setText(LLStringUtil::null);
			editor->setTentative(FALSE);
		}
	}
}

void LLPanelTranslationSettings::onBingKeyEdited(LLLineEditor* caller)
{
	if (caller->isDirty())
	{
		setBingVerified(false, false);
	}
}

void LLPanelTranslationSettings::onGoogleKeyEdited(LLLineEditor* caller)
{
	if (caller->isDirty())
	{
		setGoogleVerified(false, false);
	}
}

void LLPanelTranslationSettings::onBtnBingVerify()
{
	std::string key = getEnteredBingKey();
	if (!key.empty())
	{
		verifyKey(LLTranslate::SERVICE_BING, key);
	}
}

void LLPanelTranslationSettings::onBtnGoogleVerify()
{
	std::string key = getEnteredGoogleKey();
	if (!key.empty())
	{
		verifyKey(LLTranslate::SERVICE_GOOGLE, key);
	}
}

void LLPanelTranslationSettings::apply()
{
	gSavedSettings.setBOOL("TranslateChat", mMachineTranslationCB->getValue().asBoolean());
	gSavedSettings.setString("TranslateLanguage", mLanguageCombo->getSelectedValue().asString());
	gSavedSettings.setString("TranslationService", getSelectedService());
	gSavedSettings.setString("BingTranslateAPIKey", getEnteredBingKey());
	gSavedSettings.setString("GoogleTranslateAPIKey", getEnteredGoogleKey());
	LLFloaterChat::getInstance()->showTranslationCheckbox();
	std::set<LLHandle<LLFloater> > floaters(LLIMMgr::instance().getIMFloaterHandles());
	for(std::set<LLHandle<LLFloater> >::iterator i = floaters.begin(); i != floaters.end(); ++i)
	{
		LLFloaterIMPanel& floater = static_cast<LLFloaterIMPanel&>(*i->get());
		if (floater.getSessionType() == LLFloaterIMPanel::P2P_SESSION)
			floater.rebuildDynamics();
	}
}

