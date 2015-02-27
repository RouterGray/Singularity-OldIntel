/**
* @file lltranslate.cpp
* @brief Functions for translating text via Google Translate.
*
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "lltranslate.h"

#include <curl/curl.h>

#include "llbufferstream.h"
#include "llpaneltranslationsettings.h"
#include "lltrans.h"
#include "llui.h"
#include "sgversion.h"
#include "llviewercontrol.h"

#include "json/reader.h"

const LLStringExplicit GOOGLE_URL_BASE("https://www.googleapis.com/language/translate/v2");
const LLStringExplicit BING_URL_BASE("http://api.microsofttranslator.com/v2/Http.svc/");

static std::string allowed_chars()
{
	std::string allowed("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_~");
	std::sort(allowed.begin(), allowed.end());
	return allowed;
}

// virtual
void LLGoogleTranslationHandler::getTranslateURL(
	std::string &url,
	const std::string &from_lang,
	const std::string &to_lang,
	const std::string &text) const
{
	url = std::string(GOOGLE_URL_BASE + "?key=")
		+ getAPIKey() + "&q=" + LLURI::escape(text) + "&target=" + to_lang;
	if (!from_lang.empty())
	{
		url += "&source=" + from_lang;
	}
}

// virtual
void LLGoogleTranslationHandler::getKeyVerificationURL(
	std::string& url,
	const std::string& key) const
{
	url = std::string(GOOGLE_URL_BASE + "/languages?key=")
		+ key + "&target=en";
}

// virtual
bool LLGoogleTranslationHandler::parseResponse(
	int& status,
	const std::string& body,
	std::string& translation,
	std::string& detected_lang,
	std::string& err_msg) const
{
	Json::Value root;
	Json::Reader reader;

	if (!reader.parse(body, root))
	{
		err_msg = reader.getFormatedErrorMessages();
		return false;
	}

	if (!root.isObject()) // empty response? should not happen
	{
		return false;
	}

	if (status != HTTP_OK)
	{
		// Request failed. Extract error message from the response.
		parseErrorResponse(root, status, err_msg);
		return false;
	}

	// Request succeeded, extract translation from the response.
	return parseTranslation(root, translation, detected_lang);
}

// virtual
bool LLGoogleTranslationHandler::isConfigured() const
{
	return !getAPIKey().empty();
}

// static
void LLGoogleTranslationHandler::parseErrorResponse(
	const Json::Value& root,
	int& status,
	std::string& err_msg)
{
	const Json::Value& error = root.get("error", 0);
	if (!error.isObject() || !error.isMember("message") || !error.isMember("code"))
	{
		return;
	}

	err_msg = error["message"].asString();
	status = error["code"].asInt();
}

// static
bool LLGoogleTranslationHandler::parseTranslation(
	const Json::Value& root,
	std::string& translation,
	std::string& detected_lang)
{
	// JsonCpp is prone to aborting the program on failed assertions,
	// so be super-careful and verify the response format.
	const Json::Value& data = root.get("data", 0);
	if (!data.isObject() || !data.isMember("translations"))
	{
		return false;
	}

	const Json::Value& translations = data["translations"];
	if (!translations.isArray() || translations.size() == 0)
	{
		return false;
	}

	const Json::Value& first = translations[0U];
	if (!first.isObject() || !first.isMember("translatedText"))
	{
		return false;
	}

	translation = first["translatedText"].asString();
	detected_lang = first.get("detectedSourceLanguage", "").asString();
	return true;
}

// static
std::string LLGoogleTranslationHandler::getAPIKey()
{
	return gSavedSettings.getString("GoogleTranslateAPIKey");
}

// virtual
void LLBingTranslationHandler::getTranslateURL(
	std::string &url,
	const std::string &from_lang,
	const std::string &to_lang,
	const std::string &text) const
{
	url = BING_URL_BASE + "Translate?text=" + LLURI::escape(text, allowed_chars(), true) + "&to=" + getAPILanguageCode(to_lang);
	if (!from_lang.empty())
	{
		url += "&from=" + getAPILanguageCode(from_lang);
	}
}

// virtual
void LLBingTranslationHandler::getKeyVerificationURL(
	std::string& url,
	const std::string& key) const
{
	url = BING_URL_BASE + "GetLanguagesForTranslate?appId=" + LLURI::escape(key, allowed_chars(), true);
}

// virtual
bool LLBingTranslationHandler::parseResponse(
	int& status,
	const std::string& body,
	std::string& translation,
	std::string& detected_lang,
	std::string& err_msg) const
{
	if (status != HTTP_OK)
	{
		static const std::string MSG_BEGIN_MARKER = "Message: ";
		size_t begin = body.find(MSG_BEGIN_MARKER);
		if (begin != std::string::npos)
		{
			begin += MSG_BEGIN_MARKER.size();
		}
		else
		{
			begin = 0;
			err_msg.clear();
		}
		size_t end = body.find("</p>", begin);
		err_msg = body.substr(begin, end-begin);
		LLStringUtil::replaceString(err_msg, "&#xD;", ""); // strip CR
		return false;
	}

	// Sample response: <string xmlns="http://schemas.microsoft.com/2003/10/Serialization/">Hola</string>
	size_t begin = body.find(">");
	if (begin == std::string::npos || begin >= (body.size() - 1))
	{
		begin = 0;
	}
	else
	{
		++begin;
	}

	size_t end = body.find("</string>", begin);

	detected_lang = ""; // unsupported by this API
	translation = body.substr(begin, end-begin);
	LLStringUtil::replaceString(translation, "&#xD;", ""); // strip CR
	return true;
}

// virtual
bool LLBingTranslationHandler::isConfigured() const
{
	return !getAPIKey().empty();
}

// static
std::string LLBingTranslationHandler::getAPIKey()
{
	return gSavedSettings.getString("BingTranslateAPIKey");
}

// static
std::string LLBingTranslationHandler::getAPILanguageCode(const std::string& lang)
{
	return lang == "zh" ? "zh-CHT" : lang; // treat Chinese as Traditional Chinese
}

LLTranslate::TranslationReceiver::TranslationReceiver(const std::string& from_lang, const std::string& to_lang)
:	mFromLang(from_lang)
,	mToLang(to_lang)
,	mHandler(LLTranslate::getPreferredHandler())
{
}

// virtual
void LLTranslate::TranslationReceiver::completedRaw(
	const LLChannelDescriptors& channels,
	const LLIOPipe::buffer_ptr_t& buffer)
{
	LLBufferStream istr(channels, buffer.get());
	std::stringstream strstrm;
	strstrm << istr.rdbuf();

	const std::string body = strstrm.str();
	std::string translation, detected_lang, err_msg;
	int status = getStatus();
	LL_DEBUGS("Translate") << "HTTP status: " << status << " " << getReason() << LL_ENDL;
	LL_DEBUGS("Translate") << "Response body: " << body << LL_ENDL;
	if (mHandler.parseResponse(status, body, translation, detected_lang, err_msg))
	{
		// Fix up the response
		LLStringUtil::replaceString(translation, "&lt;", "<");
		LLStringUtil::replaceString(translation, "&gt;",">");
		LLStringUtil::replaceString(translation, "&quot;","\"");
		LLStringUtil::replaceString(translation, "&#39;","'");
		LLStringUtil::replaceString(translation, "&amp;","&");
		LLStringUtil::replaceString(translation, "&apos;","'");

		handleResponse(translation, detected_lang);
	}
	else
	{
		if (err_msg.empty())
		{
			err_msg = LLTrans::getString("TranslationResponseParseError");
		}

		llwarns << "Translation request failed: " << err_msg << llendl;
		handleFailure(status, err_msg);
	}
}

LLTranslate::KeyVerificationReceiver::KeyVerificationReceiver(EService service)
:	mService(service)
{
}

LLTranslate::EService LLTranslate::KeyVerificationReceiver::getService() const
{
	return mService;
}

// virtual
void LLTranslate::KeyVerificationReceiver::completedRaw(
	const LLChannelDescriptors& channels,
	const LLIOPipe::buffer_ptr_t& buffer)
{
	bool ok = (getStatus() == HTTP_OK);
	if (!ok)
	{
		std::string content;
		decode_raw_body(channels, buffer, content);
		LL_DEBUGS("Translate") << "Status: " << mStatus << ", Reason: " << mReason << ", Content: " << content << LL_ENDL;
	}
	setVerificationStatus(ok);
}

class BingResponderBase : public LLHTTPClient::ResponderWithCompleted
{
	/*virtual*/ char const* getName() const { return "BingAuthPostResponder"; }
	/*virtual*/ void completedRaw(const LLChannelDescriptors& channels, const buffer_ptr_t& buffer)
	{
		std::string content;
		decode_raw_body(channels, buffer, content);
		// HTTP status good?
		if (isGoodStatus(mStatus))
		{
			Json::Value root;
			Json::Reader reader;

			if (reader.parse(content, root))
			{
				LLSD header(LLTranslate::getHeader());
				content = "Bearer " + root["access_token"].asString();
				header.insert("Authorization", content);
				std::string url;
				getUrl(url, content); // Allow derived class to override at this point.
				if (!url.empty())
				{
					LL_DEBUGS("Translate") << "Sending translation request: " << url << LL_ENDL;
					LLHTTPClient::get(url, header, getResponder()); // Allow derived class to override at this point.
				}
			}
			else LL_DEBUGS("Translate") << reader.getFormatedErrorMessages() << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("Translate") << "Status: " << mStatus << ", Reason: " << mReason << ", Content: " << content << LL_ENDL;
			// Allow derived class to override at this point.
			httpFailure();
		}
	}
	virtual void httpFailure() = 0;
	virtual void getUrl(std::string&, const std::string&) = 0;
	virtual LLHTTPClient::ResponderPtr getResponder() const = 0;
};

class BingAuthPostResponder : public BingResponderBase
{
	/*virtual*/ void httpFailure() { mReceiver->handleFailure(mStatus, mContent); }
	/*virtual*/ void getUrl(std::string& url, const std::string&) { mReceiver->mHandler.getTranslateURL(url, mFrom, mTo, mMesg); }
	/*virtual*/ LLHTTPClient::ResponderPtr getResponder() const { return mReceiver; }
	LLTranslate::TranslationReceiverPtr mReceiver;
	const std::string mFrom;
	const std::string mMesg;
	const std::string mTo;
public:
	BingAuthPostResponder(LLTranslate::TranslationReceiverPtr& receiver, const std::string& from, const std::string& to, const std::string& mesg)
	: mReceiver(receiver), mFrom(from), mMesg(mesg), mTo(to)
	{}
};

class BingVerifyAuthPostResponder : public BingResponderBase
{
	/*virtual*/ void httpFailure() { mReceiver->setVerificationStatus(false); }
	/*virtual*/ void getUrl(std::string& url, const std::string& /*key*/)
	{
		//LLTranslate::getHandler(LLTranslate::SERVICE_BING).getKeyVerificationURL(url, key);
		url = LLStringUtil::null;
		mReceiver->setVerificationStatus(true);
	}
	/*virtual*/ LLHTTPClient::ResponderPtr getResponder() const { return mReceiver; }
	LLTranslate::KeyVerificationReceiverPtr mReceiver;
public:
	BingVerifyAuthPostResponder(LLTranslate::KeyVerificationReceiverPtr receiver)
	: mReceiver(receiver)
	{}
};

static void post_bing_auth(const std::string& id, const std::string& secret, LLHTTPClient::ResponderPtr responder)
{
	const std::string str("grant_type=client_credentials&client_id=" + LLURI::escape(id, allowed_chars(), true) + "&client_secret=" + LLURI::escape(secret, allowed_chars(), true) + "&scope=http://api.microsofttranslator.com");
	LLHTTPClient::postURLEncoded("https://datamarket.accesscontrol.windows.net/v2/OAuth2-13", str, responder);
}

void LLTranslate::postBingAuth(const std::string& id, const std::string& secret, TranslationReceiverPtr& receiver, const std::string& from, const std::string& to, const std::string& mesg)
{
	LL_DEBUGS("Translate") << "Posting bing authentification" << LL_ENDL;
	post_bing_auth(id, secret, new BingAuthPostResponder(receiver, from, to, mesg));
}

void LLTranslate::postBingVerifyAuth(const std::string& id, const std::string& secret, const KeyVerificationReceiverPtr& receiver)
{
	LL_DEBUGS("Translate") << "Posting bing authentification to verify credentials" << LL_ENDL;
	post_bing_auth(id, secret, new BingVerifyAuthPostResponder(receiver));
}

//static
void LLTranslate::translateMessage(
	TranslationReceiverPtr &receiver,
	const std::string &from_lang,
	const std::string &to_lang,
	const std::string &mesg)
{
	if (gSavedSettings.getString("TranslationService") != "google")
	{
		postBingAuth(gSavedSettings.getString("BingTranslateAPIID"), LLBingTranslationHandler::getAPIKey(), receiver, from_lang, to_lang, mesg);
		return;
	}
	std::string url;
	receiver->mHandler.getTranslateURL(url, from_lang, to_lang, mesg);

	LL_DEBUGS("Translate") << "Sending translation request: " << url << LL_ENDL;
	LLHTTPClient::get(url, getHeader(), receiver);
}

// static
void LLTranslate::verifyKey(
	KeyVerificationReceiverPtr& receiver,
	const std::string& key)
{
	const EService service(receiver->getService());
	if (service == SERVICE_BING)
	{
		postBingVerifyAuth(LLPanelTranslationSettings::instance().getEnteredBingID(), key, receiver);
		return;
	}
	std::string url;
	getHandler(service).getKeyVerificationURL(url, key);

	LL_DEBUGS("Translate") << "Sending key verification request: " << url << LL_ENDL;
	LLHTTPClient::get(url, getHeader(), receiver);
}

//static
std::string LLTranslate::getTranslateLanguage()
{
	std::string language = gSavedSettings.getString("TranslateLanguage");
	if (language.empty() || language == "default")
	{
		language = LLUI::getLanguage();
	}
	language = language.substr(0,2);
	return language;
}

// static
bool LLTranslate::isTranslationConfigured()
{
	return getPreferredHandler().isConfigured();
}

// static
const LLTranslationAPIHandler& LLTranslate::getPreferredHandler()
{
	EService service = SERVICE_BING;

	std::string service_str = gSavedSettings.getString("TranslationService");
	if (service_str == "google")
	{
		service = SERVICE_GOOGLE;
	}

	return getHandler(service);
}

// static
const LLTranslationAPIHandler& LLTranslate::getHandler(EService service)
{
	static LLGoogleTranslationHandler google;
	static LLBingTranslationHandler bing;

	if (service == SERVICE_GOOGLE)
	{
		return google;
	}

	return bing;
}

// static
const LLSD& LLTranslate::getHeader()
{
	static LLSD sHeader;

	if (!sHeader.size())
	{
		std::string user_agent = llformat("%s %d.%d.%d (%d)",
			gVersionChannel,
			gVersionMajor,
			gVersionMinor,
			gVersionPatch,
			gVersionBuild);

		sHeader.insert("Accept", "text/plain");
		sHeader.insert("User-Agent", user_agent);
	}

	return sHeader;
}
