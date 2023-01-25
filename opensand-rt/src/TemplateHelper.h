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
 * @file TemplateHelper.h
 * @author Yohan SIMARD / <yohan.simard@viveris.fr>
 * @author Mathias ETTINGER / <mathias.ettinger@viveris.fr>
 * @brief  Concepts for opensand-rt
 *
 */


#ifndef TEMPLATE_HELPER_H
#define TEMPLATE_HELPER_H


#include <type_traits>


namespace Rt
{


class ChannelBase;
class Channel;
class ChannelMux;
template<typename> class ChannelDemux;
template<typename> class UpwardChannel;
template<typename> class DownwardChannel;
template<typename, typename> class UpwardBase;
template<typename, typename> class DownwardBase;


template <class Chan, class = void>
struct has_one_input
    : std::is_base_of<Channel, Chan>
{};

template <class Chan>
struct has_one_input<Chan, std::void_t<typename Chan::DemuxKey>>
    : std::is_base_of<ChannelDemux<typename Chan::DemuxKey>, Chan>
{};


#if __cplusplus < 202002L
template <bool B>
using bool_constant = std::integral_constant<bool, B>;

// NOT
template <class B>
struct negation: bool_constant<!bool(B::value)>
{};

template <bool...>
struct bool_pack
{};

// AND
template <bool... Bs>
using conjunction = std::is_same<bool_pack<true, Bs...>, bool_pack<Bs..., true>>;

// OR
template <bool... Bs>
struct disjunction: bool_constant<!conjunction<!Bs...>::value>
{};

template <class Chan>
using has_one_output = disjunction<std::is_base_of<Channel, Chan>::value,
                                   std::is_base_of<ChannelMux, Chan>::value>;

template <class Chan>
using has_n_inputs = negation<has_one_input<Chan>>;

template <class Chan>
using has_n_outputs = negation<has_one_output<Chan>>;
#else
// C++20 concepts
template<class Chan>
concept HasOneInput = has_one_input<Chan>::value;

template<class Chan>
concept HasOneOutput = std::is_base_of<Channel, Chan>::value || std::is_base_of<ChannelMux, Chan>::value;


template<typename Bl>
concept SimpleUpper = HasOneInput<typename Bl::ChannelUpward> && HasOneOutput<typename Bl::ChannelDownward>;


template<typename Bl>
concept SimpleLower = HasOneInput<typename Bl::ChannelDownward> && HasOneOutput<typename Bl::ChannelUpward>;


template<typename Bl>
concept MultipleUpper = !(HasOneInput<typename Bl::ChannelUpward>) && !(HasOneOutput<typename Bl::ChannelDownward>);


template<typename Bl>
concept MultipleLower = !(HasOneInput<typename Bl::ChannelDownward>) && !(HasOneOutput<typename Bl::ChannelUpward>);


template<typename Bl>
concept HasTwoChannels = std::is_base_of<ChannelBase, UpwardChannel<Bl>>::value && std::is_base_of<ChannelBase, DownwardChannel<Bl>>::value;


template<typename Bl>
concept HasUpwardChannel = std::is_base_of<UpwardBase<UpwardChannel<Bl>, typename UpwardChannel<Bl>::Upward>, UpwardChannel<Bl>>::value;


template<typename Bl>
concept HasDownwardChannel = std::is_base_of<DownwardBase<DownwardChannel<Bl>, typename DownwardChannel<Bl>::Downward>, DownwardChannel<Bl>>::value;


template<typename Bl>
concept IsBlock = HasTwoChannels<Bl> && HasUpwardChannel<Bl> && HasDownwardChannel<Bl>;
#endif


};


#endif
