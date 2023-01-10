/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file Block.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 * @brief  The block description
 *
 */

#ifndef BLOCK_H
#define BLOCK_H

#include <memory>
#include <string>
#include <thread>
#include <type_traits>

#include "MessageEvent.h"
#include "NetSocketEvent.h"
#include "TimerEvent.h"
#include "SignalEvent.h"
#include "FileEvent.h"
#include "TcpListenEvent.h"


class OutputLog;
class OutputEvent;


namespace Rt
{


class Fifo;
class Channel;
class ChannelMux;
template<typename Key> class ChannelDemux;
template<typename Key> class ChannelMuxDemux;


/**
 * @class BlockBase
 * @brief describes base capabilities of a block without knowing their channels yet
 */
class BlockBase
{
	friend class BlockManager;

 public:
	/**
	 * @brief Block constructor
	 *
	 * @param name      The name of the block
	 */
	BlockBase(const std::string &name);

	static std::shared_ptr<Fifo> createFifo();

 protected:
	/**
	 * @brief Initialize the block
	 *
	 * @warning Do not do anything else than basic initialization here,
	 *          as it is realized before channel initialization
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool onInit();
	
	/**
	 * @brief Get the name of the block
	 *
	 * @return the name of the block
	 */
	std::string getName() const;

	/**
	 * @brief Check whether the block is initialized
	 *
	 * @return true if the block is initialized, false otherwise
	 */
	bool isInitialized() const;

	void setInitialized();

	/**
	 * @brief Internal block initialization
	 *
	 * @param stop_fd  file descriptor to the stop signals listener
	 * @return true on success, false otherwise
	 */
	virtual bool init(int stop_fd) = 0;

	/**
	 * @brief Specific block and channels initialization
	 *        Call onInit methods
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool initSpecific() = 0;

	/**
	 * @brief Initialize upward thread
	 */
	virtual std::thread initUpwardThread() = 0;

	/**
	 * @brief Initialize downward thread
	 */
	virtual std::thread initDownwardThread() = 0;

	/**
	 * @brief start the channel threads
	 *
	 * @return true on success, false otherwise
	 */
	bool start();

	/*
	 * @brief Stop the channel threads and call block destructor
	 *
	 * @return true on success, false otherwise
	 */
	bool stop();

	/*
	 * @brief Report an error
	 *
	 * @param message  The error message to report
	 */
	void reportError(const std::string& message) const;

	/*
	 * @brief Report a notice init message
	 *
	 * @param message  The message to report
	 */
	void reportSuccess(const std::string& message) const;

	/// Output Log
	std::shared_ptr<OutputLog> log_rt;
	std::shared_ptr<OutputLog> log_init;

	/// The name of the block
	const std::string name;

 private:
	/// The upward channel thread
	std::thread up_thread;
	/// The downward channel thread
	std::thread down_thread;

	/// Whether the block is initialized
	bool initialized;

	/// The event for block initialization
	std::shared_ptr<OutputEvent> event_init;
};


template<typename T>
bool handleEventImpl(T& channel, const Event * const event)
{
	switch (event->getType())
	{
		case EventType::Message:
			return channel.onEvent(static_cast<const MessageEvent&>(*event));
		case EventType::NetSocket:
			return channel.onEvent(static_cast<const NetSocketEvent&>(*event));
		case EventType::Timer:
			return channel.onEvent(static_cast<const TimerEvent&>(*event));
		case EventType::Signal:
			return channel.onEvent(static_cast<const SignalEvent&>(*event));
		case EventType::File:
			return channel.onEvent(static_cast<const FileEvent&>(*event));
		case EventType::TcpListen:
			return channel.onEvent(static_cast<const TcpListenEvent&>(*event));
		default:
			return channel.onEvent(*event);
	}
};


/**
 * @class Upward channel
 *        With this class we are able to define Upward channel
 *        functions in Block
 */
template <typename CRTP, typename ChannelType>
class UpwardBase: public ChannelType
{
	friend CRTP;
	UpwardBase(const std::string &name): ChannelType(name, "Upward") {};

 protected:
	bool handleEvent(const Event * const event) override { return handleEventImpl(static_cast<CRTP&>(*this), event); }

 public:
	using Upward = ChannelType;
};


namespace Channels
{
/**
 * @brief An upward channel with 1 input and 1 output
 */
template<typename CRTP>
using Upward = UpwardBase<CRTP, Channel>;
/**
 * @brief An upward channel with N inputs and 1 output
 */
template<typename CRTP>
using UpwardMux = UpwardBase<CRTP, ChannelMux>;
/**
 * @brief An upward channel with 1 input and N outputs
 */
template<typename CRTP, typename Key>
using UpwardDemux = UpwardBase<CRTP, ChannelDemux<Key>>;
/**
 * @brief An upward channel with N inputs and N outputs
 */
template<typename CRTP, typename Key>
using UpwardMuxDemux = UpwardBase<CRTP, ChannelMuxDemux<Key>>;
};


/**
 * @class Downward channel
 *        With this class we are able to define Downward channel
 *        functions in Block
 */
template <typename CRTP, typename ChannelType>
class DownwardBase: public ChannelType
{
	friend CRTP;
	DownwardBase(const std::string &name): ChannelType(name, "Downward") {};

 protected:
	bool handleEvent(const Event * const event) override { return handleEventImpl(static_cast<CRTP&>(*this), event); }

 public:
	using Downward = ChannelType;
};


namespace Channels
{
/**
 * @brief A downward channel with 1 inputs and 1 outputs
 */
template<typename CRTP>
using Downward = DownwardBase<CRTP, Channel>;
/**
 * @brief A downward channel with N inputs and 1 outputs
 */
template<typename CRTP>
using DownwardMux = DownwardBase<CRTP, ChannelMux>;
/**
 * @brief A downward channel with 1 inputs and N outputs
 */
template<typename CRTP, typename Key>
using DownwardDemux = DownwardBase<CRTP, ChannelDemux<Key>>;
/**
 * @brief A downward channel with N inputs and N outputs
 */
template<typename CRTP, typename Key>
using DownwardMuxDemux = DownwardBase<CRTP, ChannelMuxDemux<Key>>;
};


/// CRTP helpers
/// implement them by inheriting from a channel type above
/// e.g.: template<> class UpwardChannel<class MyBlock>: public UpwardMux<UpwardChannel<MyBlock>> { /* impl */ };
template<typename> class UpwardChannel;
template<typename> class DownwardChannel;


/* C++20 concepts: enable when possible
class ChannelBase;


template<typename Bl>
concept HasTwoChannels = std::is_base_of<ChannelBase, UpwardChannel<Bl>>::value && std::is_base_of<ChannelBase, DownwardChannel<Bl>>::value;


template<typename Bl>
concept HasUpwardChannel = std::is_base_of<UpwardBase<UpwardChannel<Bl>, typename UpwardChannel<Bl>::Upward>, UpwardChannel<Bl>>::value;


template<typename Bl>
concept HasDownwardChannel = std::is_base_of<DownwardBase<DownwardChannel<Bl>, typename DownwardChannel<Bl>::Downward>, DownwardChannel<Bl>>::value;


template<typename Bl>
concept IsBlock = HasTwoChannels<Bl> && HasUpwardChannel<Bl> && HasDownwardChannel<Bl>;


template<IsBlock Bl, typename Specific = void>*/
/**
 * @class Block
 * @brief describes a block
 *
 * upper block and lower block are absolute;
 * upward channel processes data from lower block to upper block.
 * downward channel processes data from lower block to upper block.
 */
template<typename Bl, typename Specific = void>
class Block: public BlockBase
{
	friend Bl;
	friend class BlockManager;

	template<typename Void = Specific, typename std::enable_if_t<std::is_void<Void>::value, bool> = true>
	Block(const std::string &name): BlockBase{name}, upward{name}, downward{name}
	{
		auto up_fifo = BlockBase::createFifo();
		auto down_fifo = BlockBase::createFifo();
		upward.setOppositeFifo(up_fifo, down_fifo);
		downward.setOppositeFifo(down_fifo, up_fifo);
	};

	template<typename Void = Specific, typename std::enable_if_t<!std::is_void<Void>::value, bool> = true>
	Block(const std::string &name, Void specific): BlockBase{name}, upward{name, specific}, downward{name, specific}
	{
		auto up_fifo = BlockBase::createFifo();
		auto down_fifo = BlockBase::createFifo();
		upward.setOppositeFifo(up_fifo, down_fifo);
		downward.setOppositeFifo(down_fifo, up_fifo);
	};

	std::thread initUpwardThread() override { return std::thread{&ChannelUpward::executeThread, &this->upward}; };
	std::thread initDownwardThread() override { return std::thread{&ChannelDownward::executeThread, &this->downward}; };

 public:
	using ChannelUpward = UpwardChannel<Bl>;
	using ChannelDownward = DownwardChannel<Bl>;

 protected:
	bool init(int stop_fd) override { return upward.init(stop_fd) && downward.init(stop_fd); };

	bool initSpecific()
	{
		// specific block initialization
		if(!this->onInit())
		{
			this->reportError("Block onInit failed");
			return false;
		}

		// initialize channels
		if(!this->upward.onInit())
		{
			this->reportError("Upward onInit failed");
			return false;
		}
		if(!this->downward.onInit())
		{
			this->reportError("Downward onInit failed");
			return false;
		}
		this->setInitialized();
		this->upward.setIsBlockInitialized(true);
		this->downward.setIsBlockInitialized(true);
		this->reportSuccess("Block initialization complete\n");

		return true;
	}

	/// The upward channel
	ChannelUpward upward;
	/// The downward channel
	ChannelDownward downward;
};


};  // namespace Rt


#endif
