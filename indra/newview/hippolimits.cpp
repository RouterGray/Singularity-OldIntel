#include "llviewerprecompiledheaders.h"

#include "hippolimits.h"

#include "hippogridmanager.h"

#include "llerror.h"

#include "llviewercontrol.h"		// gSavedSettings

HippoLimits *gHippoLimits = 0;


HippoLimits::HippoLimits()
{
	setLimits();
}


void HippoLimits::setLimits()
{
	if (gHippoGridManager->getConnectedGrid()->getPlatform() == HippoGridInfo::PLATFORM_SECONDLIFE) {
		setSecondLifeLimits();
	} else if (gHippoGridManager->getConnectedGrid()->getPlatform() == HippoGridInfo::PLATFORM_AURORA) {
		setAuroraLimits();
	} else {
		setOpenSimLimits();
	}
}

namespace
{
// gMaxAgentGroups is now sent by login.cgi, which
// looks it up from globals.xml.
//
// For now we need an old default value however,
// so the viewer can be deployed ahead of login.cgi.
//
constexpr S32 DEFAULT_MAX_AGENT_GROUPS = 60;
}

void HippoLimits::setMaxAgentGroups()
{
	//KC: new server defined max groups
	if (auto grid = gHippoGridManager->getConnectedGrid())
		mMaxAgentGroups = grid->getMaxAgentGroups();
	if (mMaxAgentGroups <= 0)
		mMaxAgentGroups = DEFAULT_MAX_AGENT_GROUPS;
}

void HippoLimits::setOpenSimLimits()
{
	setMaxAgentGroups();
	mMaxPrimScale = 8192.0f;
	mMaxHeight = 10000.0f;
	if (gHippoGridManager->getConnectedGrid()->isRenderCompat()) {
		LL_INFOS() << "Using rendering compatible OpenSim limits." << LL_ENDL;
		mMinPrimScale = 0.01f;
		mMinHoleSize = 0.05f;
		mMaxHollow = 0.95f;
	} else {
		LL_INFOS() << "Using Hippo OpenSim limits." << LL_ENDL;
		mMinPrimScale = 0.001f;
		mMinHoleSize = 0.01f;
		mMaxHollow = 0.99f;
	}
}

void HippoLimits::setAuroraLimits()
{
	setMaxAgentGroups();
	mMaxPrimScale = 8192.0f;
	mMinPrimScale = 0.001f;
	mMaxHeight = 10000.0f;
	mMinHoleSize = 0.001f;
	mMaxHollow = 99.0f;
}

void HippoLimits::setSecondLifeLimits()
{
	LL_INFOS() << "Using Second Life limits." << LL_ENDL;
	setMaxAgentGroups();

	mMinPrimScale = 0.01f;
	mMaxHeight = 4096.0f;
	mMinHoleSize = 0.05f;
	mMaxHollow = 0.95f;
	mMaxPrimScale = 64.0f;
}

