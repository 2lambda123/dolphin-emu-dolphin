// Copyright 2009 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <algorithm>
#include <list>
#include <map>

#include "Common/CommonTypes.h"
#include "Common/Timer.h"

#include "Core/ConfigManager.h"

#include "VideoCommon/OnScreenDisplay.h"
#include "VideoCommon/RenderBase.h"


namespace OSD
{

static std::multimap<CallbackType, Callback> s_callbacks;
static std::list<Message> s_msgList;
static std::map<MessageName, Message> s_namedMsgList;

void AddMessage(const std::string& message, u32 ms, u32 rgba)
{
	s_msgList.emplace_back(message, Common::Timer::GetTimeMs() + ms, rgba);
}

void AddNamedMessage(MessageName name, const std::string& message, u32 ms, u32 rgba)
{
	Message* msg = new Message(message, Common::Timer::GetTimeMs() + ms, rgba);
	s_namedMsgList[name] = *msg;
}

void DrawMessage(Message* msg, int top, int left, int time_left)
{
	float alpha = std::min(1.0f, std::max(0.0f, time_left / 1024.0f));
	u32 color = (msg->m_rgba & 0xFFFFFF) | ((u32)((msg->m_rgba >> 24) * alpha) << 24);

	g_renderer->RenderText(msg->m_str, left, top, color);
}

void DrawMessages()
{
	if (!SConfig::GetInstance().bOnScreenDisplayMessages)
		return;

	int left = 20, top = 35;
	u32 now = Common::Timer::GetTimeMs();

	auto namedIt = s_namedMsgList.begin();
	while (namedIt != s_namedMsgList.end())
	{
		Message msg = namedIt->second;
		int time_left = (int)(msg.m_timestamp - now);
		DrawMessage(&msg, top, left, time_left);

		if (time_left <= 0)
			s_namedMsgList.erase(namedIt->first);

		++namedIt;

		top += 15;
	}

	auto it = s_msgList.begin();
	while (it != s_msgList.end())
	{
		int time_left = (int)(it->m_timestamp - now);
		DrawMessage(&*it, top, left, time_left);

		if (time_left <= 0)
			it = s_msgList.erase(it);
		else
			++it;

		top += 15;
	}
}

void ClearMessages()
{
	s_msgList.clear();
}

// On-Screen Display Callbacks
void AddCallback(CallbackType type, Callback cb)
{
	s_callbacks.emplace(type, cb);
}

void DoCallbacks(CallbackType type)
{
	auto it_bounds = s_callbacks.equal_range(type);
	for (auto it = it_bounds.first; it != it_bounds.second; ++it)
	{
		it->second();
	}

	// Wipe all callbacks on shutdown
	if (type == CallbackType::Shutdown)
		s_callbacks.clear();
}

}  // namespace
