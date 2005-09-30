/*
 * $Id: ca_descriptor.h,v 1.3 2005/09/30 16:13:49 ghostrider Exp $
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

#ifndef __ca_descriptor_h__
#define __ca_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> CaDataByteVector;
typedef CaDataByteVector::iterator CaDataByteIterator;
typedef CaDataByteVector::const_iterator CaDataByteConstIterator;

class CaDescriptor : public Descriptor
{
	protected:
		unsigned caSystemId				: 16;
		unsigned caPid					: 13;
		CaDataByteVector caDataBytes;

	public:
		CaDescriptor(const uint8_t * const buffer);

		uint16_t getCaSystemId(void) const;
		uint16_t getCaPid(void) const;
		const CaDataByteVector *getCaDataBytes(void) const;

		size_t writeToBuffer(uint8_t * const buffer) const;
};

typedef std::list<CaDescriptor *> CaDescriptorList;
typedef CaDescriptorList::iterator CaDescriptorIterator;
typedef CaDescriptorList::const_iterator CaDescriptorConstIterator;

#endif /* __ca_descriptor_h__ */
