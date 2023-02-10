/*
 *
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
 *
 */

/**
 * @file FifoElement.h
 * @brief Fifo element
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef FIFO_ELEMENT_H
#define FIFO_ELEMENT_H


#include <opensand_rt/Ptr.h>

#include "Except.h"


class NetContainer;


/**
 * @class FifoElement
 * @brief Fifo element
 */
class FifoElement
{
protected:
	/// The element stored in the FIFO
	Rt::Ptr<NetContainer> elem;

public:
	/**
	 * Build a fifo element
	 * @param elem       The element to store in the FIFO
	 */
	FifoElement(Rt::Ptr<NetContainer> elem);

	/**
	 * Get the FIFO elelement
	 * @return The FIFO element
	 */
	template<class T = NetContainer>
	Rt::Ptr<T> releaseElem();

	/**
	 * Set the FIFO element
	 *
	 * @param packet The new FIFO element
	 */
	void setElem(Rt::Ptr<NetContainer> elem);

	/**
	 * Get the element length
	 * @return The element length
	 */
	std::size_t getTotalLength() const;

	/**
	 * Check whether the fifo element actually contains an element
	 * @return true if elem != nullptr false otherwise
	 */
	explicit operator bool() const noexcept;
};


template<class T>
Rt::Ptr<T> FifoElement::releaseElem()
{
	if (!elem)
	{
		return Rt::make_ptr<T>(nullptr);
	}

	T* cast_elem = static_cast<T*>(elem.release());
	// Can't see how this could fail...
	ASSERT(cast_elem != nullptr, "Casting FifoElement data failed in releaseElem");
	return {cast_elem, std::move(elem.get_deleter())};
}


template<>
Rt::Ptr<NetContainer> FifoElement::releaseElem();


#endif
