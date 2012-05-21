/*
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Initial Developer of the Original Code is Adriano dos Santos Fernandes.
 * Portions created by the Initial Developer are Copyright (C) 2011 the Initial Developer.
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 */

#ifndef FBSTUFF_ODS_ODS_H
#define FBSTUFF_ODS_ODS_H

#include <boost/cstdint.hpp>

namespace fbods
{

//------------------------------------------------------------------------------


struct PageHeader
{
	static const boost::int8_t TYPE_POINTER = 4;

	boost::int8_t type;
	boost::int8_t flags;
	boost::uint16_t checksum;
	boost::uint32_t generation;
	boost::uint32_t scn;
	boost::uint32_t reserved;
};

struct HeaderPage
{
	PageHeader pageHeader;
	boost::uint16_t pageSize;
	boost::uint16_t odsVersion;
	boost::int32_t pages;
	boost::uint32_t nextPage;
	boost::int32_t oldestTransaction;
	boost::int32_t oldestActive;
	boost::int32_t nextTransaction;
	boost::uint16_t sequence;
	boost::uint16_t flags;
	boost::int32_t creationDate[2];
	boost::int32_t attachmentId;
	boost::int32_t shadowCount;
	boost::int16_t implementation;
	boost::uint16_t odsMinor;
	boost::uint16_t odsMinorOriginal;
	boost::uint16_t end;
	boost::uint32_t pageBuffers;
	boost::int32_t bumpedTransaction;
	boost::int32_t oldestSnapshot;
	boost::int32_t backupPages;
	boost::int32_t misc[3];
	boost::uint8_t data[];
};

struct PointerPage
{
	PageHeader pageHeader;
	boost::int32_t sequence;
	boost::int32_t next;
	boost::uint16_t count;
	boost::uint16_t relation;
	boost::uint16_t minSpace;
	boost::uint16_t maxSpace;
	boost::int32_t page[];
};

struct DataPage
{
	PageHeader pageHeader;
	boost::int32_t sequence;
	boost::uint16_t relation;
	boost::uint16_t count;
	struct Repeat
	{
		boost::uint16_t offset;
		boost::uint16_t length;
	} rpt[];
};

struct RecordHeader
{
	static const unsigned FLAG_DELETED	= 0x01;
	static const unsigned FLAG_BLOB		= 0x10;

	boost::int32_t transaction;
	boost::int32_t backPage;
	boost::uint16_t backLine;
	boost::uint16_t flags;
	boost::uint8_t format;
	union
	{
		boost::uint8_t data[];
		/***
		struct
		{
			boost::int32_t page;
			boost::uint16_t line;
			boost::uint8_t data[];
		} fragment;
		***/
	};
};


//------------------------------------------------------------------------------

}	// fbods

#endif	// FBSTUFF_ODS_ODS_H
