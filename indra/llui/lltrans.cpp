/**
 * @file lltrans.cpp
 * @brief LLTrans implementation
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "linden_common.h"

#include "lltrans.h"
#include "llxmlnode.h"
#include "lluictrlfactory.h"
#include "llnotificationsutil.h"

#include <map>

LLTrans::template_map_t LLTrans::sStringTemplates;
LLStringUtil::format_map_t LLTrans::sDefaultArgs;

//static 
bool LLTrans::parseStrings(const std::string& xml_filename, const std::set<std::string>& default_args)
{
	LLXMLNodePtr root;
	BOOL success  = LLUICtrlFactory::getLayeredXMLNode(xml_filename, root);

	if (!success || root.isNull() || !root->hasName( "strings" ))
	{
		llerrs << "Problem reading strings: " << xml_filename << llendl;
		return false;
	}
	
	sDefaultArgs.clear();
	for (LLXMLNode* string = root->getFirstChild();
		 string != NULL; string = string->getNextSibling())
	{
		if (!string->hasName("string"))
		{
			continue;
		}
		
		std::string string_name;

		if (! string->getAttributeString("name", string_name))
		{
			llwarns << "Unable to parse string with no name" << llendl;
			continue;
		}
	
		std::string contents;
		//value is used for strings with preceeding spaces. If not present, fall back to getTextContents()
		if(!string->getAttributeString("value",contents))
			contents=string->getTextContents();
		LLTransTemplate xml_template(string_name, contents);
		sStringTemplates[xml_template.mName] = xml_template;

		std::set<std::string>::const_iterator iter = default_args.find(xml_template.mName);
		if (iter != default_args.end())
		{
			std::string name = *iter;
			if (name[0] != '[')
				name = llformat("[%s]",name.c_str());
			sDefaultArgs[name] = xml_template.mText;
		}
	}

	return true;
}

static LLFastTimer::DeclareTimer FTM_GET_TRANS("Translate string");

//static 
std::string LLTrans::getString(const std::string &xml_desc, const LLStringUtil::format_map_t& msg_args)
{
	// Don't care about time as much as call count.  Make sure we're not
	// calling LLTrans::getString() in an inner loop. JC
	LLFastTimer timer(FTM_GET_TRANS);
	
	template_map_t::iterator iter = sStringTemplates.find(xml_desc);
	if (iter != sStringTemplates.end())
	{
		std::string text = iter->second.mText;
		LLStringUtil::format_map_t args = sDefaultArgs;
		args.insert(msg_args.begin(), msg_args.end());
		LLStringUtil::format(text, args);
		
		return text;
	}
	else
	{
		LLSD args;
		args["STRING_NAME"] = xml_desc;
		LL_WARNS_ONCE("configuration") << "Missing String in strings.xml: [" << xml_desc << "]" << LL_ENDL;
		LLNotificationsUtil::add("MissingString", args);
		
		return "MissingString("+xml_desc+")";
	}
}

//static
std::string LLTrans::getString(const std::string &xml_desc, const LLSD& msg_args)
{
	// Don't care about time as much as call count.  Make sure we're not
	// calling LLTrans::getString() in an inner loop. JC
	LLFastTimer timer(FTM_GET_TRANS);

	template_map_t::iterator iter = sStringTemplates.find(xml_desc);
	if (iter != sStringTemplates.end())
	{
		std::string text = iter->second.mText;
		LLStringUtil::format(text, msg_args);
		return text;
	}
	else
	{
		LL_WARNS_ONCE("configuration") << "Missing String in strings.xml: [" << xml_desc << "]" << LL_ENDL;
		return "MissingString("+xml_desc+")";
	}
}

//static 
bool LLTrans::findString(std::string &result, const std::string &xml_desc, const LLStringUtil::format_map_t& msg_args)
{
	LLFastTimer timer(FTM_GET_TRANS);
	
	template_map_t::iterator iter = sStringTemplates.find(xml_desc);
	if (iter != sStringTemplates.end())
	{
		std::string text = iter->second.mText;
		LLStringUtil::format_map_t args = sDefaultArgs;
		args.insert(msg_args.begin(), msg_args.end());
		LLStringUtil::format(text, args);
		result = text;
		return true;
	}
	else
	{
		LL_WARNS_ONCE("configuration") << "Missing String in strings.xml: [" << xml_desc << "]" << LL_ENDL;	
		return false;
	}
}

//static
bool LLTrans::findString(std::string &result, const std::string &xml_desc, const LLSD& msg_args)
{
	LLFastTimer timer(FTM_GET_TRANS);

	template_map_t::iterator iter = sStringTemplates.find(xml_desc);
	if (iter != sStringTemplates.end())
	{
		std::string text = iter->second.mText;
		LLStringUtil::format(text, msg_args);
		result = text;
		return true;
	}
	else
	{
		LL_WARNS_ONCE("configuration") << "Missing String in strings.xml: [" << xml_desc << "]" << LL_ENDL;	
		return false;
	}
}

void LLTrans::setDefaultArg(const std::string& name, const std::string& value)
{
	sDefaultArgs[name] = value;
}
