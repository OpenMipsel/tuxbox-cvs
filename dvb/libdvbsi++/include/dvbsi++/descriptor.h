/*
 * $Id: descriptor.h,v 1.4 2005/09/29 23:49:41 ghostrider Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __descriptor_h__
#define __descriptor_h__

#include "compat.h"

class Descriptor
{
	protected:
		unsigned descriptorTag				: 8;
		unsigned descriptorLength			: 8;

	public:
		Descriptor(const uint8_t * const buffer);
		virtual ~Descriptor() { };

		uint8_t getTag(void) const;
		uint8_t getLength(void) const;

		size_t writeToBuffer(uint8_t * const buffer) const;
};

typedef std::list<Descriptor *> DescriptorList;
typedef DescriptorList::iterator DescriptorIterator;
typedef DescriptorList::const_iterator DescriptorConstIterator;

#endif /* __descriptor_h__ */
