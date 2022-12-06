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

TopMux::Upward::Upward(const std::string &name):
	Rt::Block::UpwardMux(name)
{
}


bool TopMux::Upward::onEvent(const Rt::Event* const event)
{
	if (event->getType() != Rt::EventType::Message)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
		return false;
	}
	auto msg = static_cast<const Rt::MessageEvent*>(event);
	auto data = msg->getMessage<std::string>();
	std::cout << getName() << ": Sharing message to downward channel: " << *data << "\n";
	shareMessage(std::move(data), 0);
	return true;
}


TopMux::Downward::Downward(const std::string &name):
	Rt::Block::DownwardDemux<Side>(name)
{
}


bool TopMux::Downward::onEvent(const Rt::Event* const event)
{
	if (event->getType() != Rt::EventType::Message)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
		return false;
	}
	auto msg = static_cast<const Rt::MessageEvent*>(event);
	auto data = msg->getMessage<std::string>();
	std::cout << getName() << ": Sending message downward: " << *data << "\n";
	enqueueMessage(Side::RIGHT, std::move(data), 0);
	return true;
}


///////////////////////// TopBlock /////////////////////////

TopBlock::TopBlock(const std::string &name, Side side):
	Rt::Block{name}
{
}


TopBlock::Upward::Upward(const std::string &name, Side side):
	Rt::Block::UpwardMux{name},
	side{side}
{
}


bool TopBlock::Upward::onEvent(const Rt::Event* const event)
{
	if (event->getType() != Rt::EventType::Message)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
		return false;
	}
	if (side == Side::RIGHT)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto msg = static_cast<const Rt::MessageEvent*>(event);
	auto data = msg->getMessage<std::string>();
	std::cout << getName() << ": Sending message upward: " << *data << "\n";
	enqueueMessage(std::move(data), 0);
	return true;
}


TopBlock::Downward::Downward(const std::string &name, Side side):
	Rt::Block::DownwardDemux<Side>{name},
	side{side}
{
}


bool TopBlock::Downward::onEvent(const Rt::Event* const event)
{
	if (event->getType() != Rt::EventType::Message)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
		return false;
	}
	if (side == Side::LEFT)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto msg = static_cast<const Rt::MessageEvent*>(event);
	auto data = msg->getMessage<std::string>();
	std::cout << getName() << ": Sending message downward: " << *data << "\n";
	enqueueMessage(Side::RIGHT, std::move(data), 0);
	return true;
}


///////////////////////// MiddleBlock /////////////////////////

MiddleBlock::MiddleBlock(const std::string &name, Side side):
	Rt::Block{name}
{
}


MiddleBlock::Upward::Upward(const std::string &name, Side side):
	Rt::Block::UpwardMuxDemux<Side>(name),
	side{side}
{
}


bool MiddleBlock::Upward::onEvent(const Rt::Event* const event)
{
	if (event->getType() != Rt::EventType::Message)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
		return false;
	}
	if (side == Side::RIGHT)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto msg = static_cast<const Rt::MessageEvent*>(event);
	auto data = msg->getMessage<std::string>();
	std::cout << getName() << ": Sending message upward: " << *data << "\n";
	enqueueMessage(Side::LEFT, std::move(data), 0);
	return true;
}


MiddleBlock::Downward::Downward(const std::string &name, Side side):
	Rt::Block::DownwardMuxDemux<Side>(name),
	side{side}
{
}


bool MiddleBlock::Downward::onEvent(const Rt::Event* const event)
{
	if (event->getType() != Rt::EventType::Message)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
		return false;
	}
	if (side == Side::LEFT)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto msg = static_cast<const Rt::MessageEvent*>(event);
	auto data = msg->getMessage<std::string>();
	std::cout << getName() << ": Sending message downward: " << *data << "\n";
	enqueueMessage(Side::RIGHT, std::move(data), 0);
	return true;
}


///////////////////////// BottomBlock /////////////////////////

BottomBlock::BottomBlock(const std::string &name, Side side):
	Rt::Block{name}
{
}


BottomBlock::Upward::Upward(const std::string &name, Side side):
	Rt::Block::UpwardDemux<Side>(name),
	side{side}
{
}


bool BottomBlock::Upward::onEvent(const Rt::Event* const event)
{
	if (event->getType() != Rt::EventType::Message)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
		return false;
	}
	if (side == Side::RIGHT)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto msg = static_cast<const Rt::MessageEvent*>(event);
	auto data = msg->getMessage<std::string>();
	std::cout << getName() << ": Sending message upward: " << *data << "\n";
	enqueueMessage(Side::LEFT, std::move(data), 0);
	return true;
}


BottomBlock::Downward::Downward(const std::string &name, Side side):
	Rt::Block::DownwardMux(name),
	side{side}
{
}


bool BottomBlock::Downward::onEvent(const Rt::Event* const event)
{
	if (event->getType() != Rt::EventType::Message)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
		return false;
	}
	if (side == Side::LEFT)
	{
		Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "The wrong block received the message");
		return false;
	}
	auto msg = static_cast<const Rt::MessageEvent*>(event);
	auto data = msg->getMessage<std::string>();
	std::cout << getName() << ": Sending message downward: " << *data << "\n";
	enqueueMessage(std::move(data), 0);
	return true;
}


///////////////////////// BottomMux /////////////////////////

BottomMux::Upward::Upward(const std::string &name):
	Rt::Block::UpwardDemux<Side>(name)
{
}


bool BottomMux::Upward::onInit()
{
	auto data = Rt::make_ptr<std::string>("test");
	std::cout << getName() << ": Sending message upward: " << *data << "\n";
	enqueueMessage(Side::LEFT, std::move(data), 0);
	return true;
}


bool BottomMux::Upward::onEvent(const Rt::Event* const)
{
	Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
	return false;
}

BottomMux::Downward::Downward(const std::string &name):
	Rt::Block::DownwardMux(name)
{
}


bool BottomMux::Downward::onInit()
{
	addTimerEvent("timeout", 1000, false);
	return true;
}


bool BottomMux::Downward::onEvent(const Rt::Event* const event)
{
	switch (event->getType())
	{
		case Rt::EventType::Message:
		{
			auto msg = static_cast<const Rt::MessageEvent*>(event);
			auto data = msg->getMessage<std::string>();
			std::cout << "Received message: " << *data << "\n";
			if (*data != "test")
			{
				Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Message has been modified");
				return false;
			}
			kill(getpid(), SIGTERM);
			return true;
		}
		case Rt::EventType::Timer:
			Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Timeout while waiting for message");
			return false;
		default:
			Rt::Rt::reportError(getName(), std::this_thread::get_id(), true, "Unexpected message received");
			return false;
	}
	return false;
}


int main()
{
	auto top_mux = Rt::Rt::createBlock<TopMux>("top_mux");
	auto top_left = Rt::Rt::createBlock<TopBlock>("top_left", Side::LEFT);
	auto top_right = Rt::Rt::createBlock<TopBlock>("top_right", Side::RIGHT);
	auto middle_left = Rt::Rt::createBlock<MiddleBlock>("middle_left", Side::LEFT);
	auto middle_right = Rt::Rt::createBlock<MiddleBlock>("middle_right", Side::RIGHT);
	auto bottom_left = Rt::Rt::createBlock<BottomBlock>("bottom_left", Side::LEFT);
	auto bottom_right = Rt::Rt::createBlock<BottomBlock>("bottom_right", Side::RIGHT);
	auto bottom_mux = Rt::Rt::createBlock<BottomMux>("bottom_mux");
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
