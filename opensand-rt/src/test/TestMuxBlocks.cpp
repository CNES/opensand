/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file TestMuxBlocks.cpp
 * @brief
 * @author Yohan Simard <yohan.simard@viveris.com>
 */

#include "TestMuxBlocks.h"
#include "Rt.h"
#include <csignal>

bool DownMuxBlock::onInit()
{
	return true;
}

DownMuxBlock::Upward::Upward(const std::string &name):
    RtUpwardMux(name)
{}

bool DownMuxBlock::Upward::onInit()
{
	return true;
}

bool DownMuxBlock::Upward::onEvent(const RtEvent *const event)
{
	if (event->getType() != evt_message)
	{
		Rt::reportError(getName(), pthread_self(), true, "Unexpected message received");
		return false;
	}
	auto msg = static_cast<const MessageEvent *>(event);
	auto data = msg->getData();
	shareMessage(&data);
	return true;
}

DownMuxBlock::Downward::Downward(const std::string &name):
    RtDownwardDemux<Side>(name) {}

bool DownMuxBlock::Downward::onInit()
{
	return true;
}

bool DownMuxBlock::Downward::onEvent(const RtEvent *const event)
{
	if (event->getType() != evt_message)
	{
		Rt::reportError(getName(), pthread_self(), true, "Unexpected message received");
		return false;
	}
	auto msg = static_cast<const MessageEvent *>(event);
	auto data = msg->getData();
	enqueueMessage(Side::RIGHT, &data);
	return true;
}

///////////////////////// SimpleBlock /////////////////////////

SimpleBlock::SimpleBlock(const std::string &name, bool send_msg):
    Block{name} {}

bool SimpleBlock::onInit()
{
	return true;
}

SimpleBlock::Upward::Upward(const std::string &name, bool send_msg):
    RtUpward(name), send_msg{send_msg}
{}

bool SimpleBlock::Upward::onInit()
{
	if (send_msg)
	{
		addTimerEvent("send", 100, false);
	}
	return true;
}

bool SimpleBlock::Upward::onEvent(const RtEvent *const event)
{
	if (event->getType() != evt_timer)
	{
		Rt::reportError(getName(), pthread_self(), true, "Unexpected message received");
		return false;
	}
	if (send_msg)
	{
		void *data = new std::string{"test"};
		enqueueMessage(&data);
	}
	return true;
}

SimpleBlock::Downward::Downward(const std::string &name, bool send_msg):
    RtDownward(name), send_msg{send_msg} {}

bool SimpleBlock::Downward::onInit()
{
	if (!send_msg)
	{
		addTimerEvent("timeout", 1000, false);
	}
	return true;
}

bool SimpleBlock::Downward::onEvent(const RtEvent *const event)
{
	switch (event->getType())
	{
		case evt_message:
		{
			if (send_msg)
			{
				Rt::reportError(getName(), pthread_self(), true, "The wrong block received the message");
				return false;
			}
			auto msg = static_cast<const MessageEvent *>(event);
			auto data = static_cast<std::string *>(msg->getData());
			std::cout << "Received message: " << *data << "\n";
			delete data;
			kill(getpid(), SIGTERM);
			return true;
		}
		case evt_timer:
			Rt::reportError(getName(), pthread_self(), true, "Timeout while waiting for message");
			return false;
		default:
			Rt::reportError(getName(), pthread_self(), true, "Unexpected message received");
			return false;
	}
	return false;
}

int main()
{
	auto mux = Rt::createBlock<DownMuxBlock>("mux");
	auto left = Rt::createBlock<SimpleBlock>("left", true);
	auto right = Rt::createBlock<SimpleBlock>("right", false);
	Rt::connectBlocks(mux, left, Side::LEFT);
	Rt::connectBlocks(mux, right, Side::RIGHT);

	auto output = Output::Get();
	output->configureTerminalOutput();
	output->finalizeConfiguration();
	
	if (!Rt::run(true))
	{
		std::cerr << "Error during execution\n";
		return 1;
	}
	std::cout << "Successfull\n";
	return 0;
}