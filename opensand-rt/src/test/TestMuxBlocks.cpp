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
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */


// This test creates the following block structure:
//
//  ┌───────────────────────┐
//  │       top_mux         │
//  │                       │
//  └─────▲──────────┬──────┘
//  ┌─────┴────┐ ┌───▼──────┐
//  │   top    │ │   top    │
//  │   left   │ │  right   │
//  └─────▲────┘ └───┬──────┘
//  ┌─────┴────┐ ┌───▼──────┐
//  │  middle  │ │  middle  │
//  │   left   │ │  right   │
//  └─────▲────┘ └───┬──────┘
//  ┌─────┴────┐ ┌───▼──────┐
//  │  bottom  │ │  bottom  │
//  │   left   │ │  right   │
//  └─────▲────┘ └───┬──────┘
//  ┌─────┴──────────▼──────┐
//  │      bottom_mux       │
//  │                       │
//  └───────────────────────┘
//
// All block can send messages to both their right and left vertical neighbours
// Arrows only represents the messages that are actually sent


#include "TestMuxBlocks.h"
#include "Rt.h"
#include "MessageEvent.h"
#include <csignal>
#include <sys/types.h>
#include <unistd.h>


///////////////////////// TopMux /////////////////////////

bool Rt::UpwardChannel<TopMux>::onEvent(const Event& event)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}


Rt::UpwardChannel<TopMux>::UpwardChannel(const std::string& name):
	Channels::UpwardMux<UpwardChannel<TopMux>>{name}
{
}


bool Rt::UpwardChannel<TopMux>::onEvent(const MessageEvent& event)
{
	auto data = event.getMessage<std::string>();
	std::cout << getName() << ": Sharing message to downward channel: " << *data << "\n";
	shareMessage(std::move(data), 0);
	return true;
}


Rt::DownwardChannel<TopMux>::DownwardChannel(const std::string& name):
	Channels::DownwardDemux<DownwardChannel<TopMux>, Side>{name}
{
}


bool Rt::DownwardChannel<TopMux>::onEvent(const Event& event)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}


bool Rt::DownwardChannel<TopMux>::onEvent(const MessageEvent& event)
{
	auto data = event.getMessage<std::string>();
	std::cout << getName() << ": Sending message downward: " << *data << "\n";
	enqueueMessage(Side::RIGHT, std::move(data), 0);
	return true;
}


///////////////////////// TopBlock /////////////////////////

Rt::UpwardChannel<TopBlock>::UpwardChannel(const std::string &name, Side side):
	Channels::UpwardMux<UpwardChannel<TopBlock>>{name},
	side{side}
{
}


bool Rt::UpwardChannel<TopBlock>::onEvent(const Event& event)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}


bool Rt::UpwardChannel<TopBlock>::onEvent(const MessageEvent& event)
{
	if (side == Side::RIGHT)
	{
		Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto data = event.getMessage<std::string>();
	std::cout << getName() << ": Sending message upward: " << *data << "\n";
	enqueueMessage(std::move(data), 0);
	return true;
}


Rt::DownwardChannel<TopBlock>::DownwardChannel(const std::string &name, Side side):
	Channels::DownwardDemux<DownwardChannel<TopBlock>, Side>{name},
	side{side}
{
}


bool Rt::DownwardChannel<TopBlock>::onEvent(const Event& event)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}


bool Rt::DownwardChannel<TopBlock>::onEvent(const MessageEvent& event)
{
	if (side == Side::LEFT)
	{
		Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto data = event.getMessage<std::string>();
	std::cout << getName() << ": Sending message downward: " << *data << "\n";
	enqueueMessage(Side::RIGHT, std::move(data), 0);
	return true;
}


///////////////////////// MiddleBlock /////////////////////////

Rt::UpwardChannel<MiddleBlock>::UpwardChannel(const std::string &name, Side side):
	Channels::UpwardMuxDemux<UpwardChannel<MiddleBlock>, Side>{name},
	side{side}
{
}


bool Rt::UpwardChannel<MiddleBlock>::onEvent(const Event& event)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}


bool Rt::UpwardChannel<MiddleBlock>::onEvent(const MessageEvent& event)
{
	if (side == Side::RIGHT)
	{
		Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto data = event.getMessage<std::string>();
	std::cout << getName() << ": Sending message upward: " << *data << "\n";
	enqueueMessage(Side::LEFT, std::move(data), 0);
	return true;
}


Rt::DownwardChannel<MiddleBlock>::DownwardChannel(const std::string &name, Side side):
	Channels::DownwardMuxDemux<DownwardChannel<MiddleBlock>, Side>{name},
	side{side}
{
}


bool Rt::DownwardChannel<MiddleBlock>::onEvent(const Event& event)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}


bool Rt::DownwardChannel<MiddleBlock>::onEvent(const MessageEvent& event)
{
	if (side == Side::LEFT)
	{
		Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto data = event.getMessage<std::string>();
	std::cout << getName() << ": Sending message downward: " << *data << "\n";
	enqueueMessage(Side::RIGHT, std::move(data), 0);
	return true;
}


///////////////////////// BottomBlock /////////////////////////

Rt::UpwardChannel<BottomBlock>::UpwardChannel(const std::string &name, Side side):
	Channels::UpwardDemux<UpwardChannel<BottomBlock>, Side>{name},
	side{side}
{
}


bool Rt::UpwardChannel<BottomBlock>::onEvent(const Event& event)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}


bool Rt::UpwardChannel<BottomBlock>::onEvent(const MessageEvent& event)
{
	if (side == Side::RIGHT)
	{
		Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto data = event.getMessage<std::string>();
	std::cout << getName() << ": Sending message upward: " << *data << "\n";
	enqueueMessage(Side::LEFT, std::move(data), 0);
	return true;
}


Rt::DownwardChannel<BottomBlock>::DownwardChannel(const std::string &name, Side side):
	Channels::DownwardMux<DownwardChannel<BottomBlock>>{name},
	side{side}
{
}


bool Rt::DownwardChannel<BottomBlock>::onEvent(const Event& event)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}


bool Rt::DownwardChannel<BottomBlock>::onEvent(const MessageEvent& event)
{
	if (side == Side::LEFT)
	{
		Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto data = event.getMessage<std::string>();
	std::cout << getName() << ": Sending message downward: " << *data << "\n";
	enqueueMessage(std::move(data), 0);
	return true;
}


///////////////////////// BottomMux /////////////////////////

bool Rt::UpwardChannel<BottomMux>::onInit()
{
	auto data = make_ptr<std::string>("test");
	std::cout << getName() << ": Sending message upward: " << *data << "\n";
	enqueueMessage(Side::LEFT, std::move(data), 0);
	return true;
}


Rt::UpwardChannel<BottomMux>::UpwardChannel(const std::string& name):
	Channels::UpwardDemux<UpwardChannel<BottomMux>, Side>{name}
{
}


bool Rt::UpwardChannel<BottomMux>::onEvent(const Event&)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}


Rt::DownwardChannel<BottomMux>::DownwardChannel(const std::string& name):
	Channels::DownwardMux<DownwardChannel<BottomMux>>{name}
{
}


bool Rt::DownwardChannel<BottomMux>::onInit()
{
	addTimerEvent("timeout", 1000, false);
	return true;
}


bool Rt::DownwardChannel<BottomMux>::onEvent(const Event& event)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}


bool Rt::DownwardChannel<BottomMux>::onEvent(const TimerEvent& event)
{
	Rt::reportError(getName(), std::this_thread::get_id(), true, "Timeout while waiting for message");
	return false;
}


bool Rt::DownwardChannel<BottomMux>::onEvent(const MessageEvent& event)
{
	auto data = event.getMessage<std::string>();
	std::cout << "Received message: " << *data << "\n";
	if (*data != "test")
	{
		Rt::reportError(getName(), std::this_thread::get_id(), true, "Message has been modified");
		return false;
	}
	kill(getpid(), SIGTERM);
	return true;
}


int main()
{
	auto& top_mux = Rt::Rt::createBlock<TopMux>("top_mux");
	auto& top_left = Rt::Rt::createBlock<TopBlock>("top_left", Side::LEFT);
	auto& top_right = Rt::Rt::createBlock<TopBlock>("top_right", Side::RIGHT);
	auto& middle_left = Rt::Rt::createBlock<MiddleBlock>("middle_left", Side::LEFT);
	auto& middle_right = Rt::Rt::createBlock<MiddleBlock>("middle_right", Side::RIGHT);
	auto& bottom_left = Rt::Rt::createBlock<BottomBlock>("bottom_left", Side::LEFT);
	auto& bottom_right = Rt::Rt::createBlock<BottomBlock>("bottom_right", Side::RIGHT);
	auto& bottom_mux = Rt::Rt::createBlock<BottomMux>("bottom_mux");
	Rt::Rt::connectBlocks(top_mux, top_left, Side::LEFT);
	Rt::Rt::connectBlocks(top_mux, top_right, Side::RIGHT);
	Rt::Rt::connectBlocks(top_left, middle_left, Side::LEFT, Side::LEFT);
	Rt::Rt::connectBlocks(top_right, middle_right, Side::RIGHT, Side::RIGHT);
	Rt::Rt::connectBlocks(middle_left, bottom_left, Side::LEFT, Side::LEFT);
	Rt::Rt::connectBlocks(middle_right, bottom_right, Side::RIGHT, Side::RIGHT);
	Rt::Rt::connectBlocks(bottom_left, bottom_mux, Side::LEFT);
	Rt::Rt::connectBlocks(bottom_right, bottom_mux, Side::RIGHT);

	auto output = Output::Get();
	output->configureTerminalOutput();
	output->finalizeConfiguration();

	if (!Rt::Rt::run(true))
	{
		std::cerr << "Error during execution\n";
		return 1;
	}
	std::cout << "Successfull\n";
	return 0;
}
