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

#include "../FbTest.h"
#include <Interface.h>
#include <ProviderInterface.h>
#include <vector>
#include <ibase.h>

namespace V3Util
{

//------------------------------------------------------------------------------

extern Firebird::IMaster* master;
extern Firebird::IProvider* dispatcher;

class OffsetBase
{
public:
	OffsetBase()
		: pos(0),
		  nullPos(0)
	{
	}

	unsigned pos;
	unsigned nullPos;
};

template <class T>
class Offset : public OffsetBase
{
};

class MessageImpl : public Firebird::FbMessage
{
public:
	MessageImpl()
		: items(0)
	{
		blrLength = 0;
		blr = NULL;
		bufferLength = 0;
		buffer = NULL;

		static const ISC_UCHAR HEADER[] = {
			blr_version5,
			blr_begin,
			blr_message, 0, 0, 0
		};

		blrArray.insert(blrArray.end(), HEADER, HEADER + sizeof(HEADER));
	}

	~MessageImpl()
	{
		if (buffer)
			delete [] buffer;
	}

	template <typename T>
	void add(Offset<T>& offset)
	{
		if (blr)
			return;	// return an error, this is already constructed message

		bufferLength = offset.align(bufferLength);
		offset.pos = bufferLength;
		bufferLength += offset.addBlr(blrArray);

		bufferLength = FB_ALIGN(bufferLength, sizeof(ISC_SHORT));
		offset.nullPos = bufferLength;
		bufferLength += sizeof(ISC_SHORT);

		blrArray.push_back(blr_short);
		blrArray.push_back(0);

		items += 2;
	}

	unsigned finish()
	{
		if (blr)
			return bufferLength;

		blrArray.push_back(blr_end);
		blrArray.push_back(blr_eoc);
		blrArray[4] = items & 0xFF;
		blrArray[5] = (items >> 8) & 0xFF;

		blrLength = (unsigned) blrArray.size();
		blr = &blrArray.front();

		buffer = new ISC_UCHAR[bufferLength];
		memset(buffer, 0, bufferLength);

		return bufferLength;
	}

	bool isNull(const OffsetBase& index)
	{
		return *(ISC_SHORT*) (buffer + index.nullPos);
	}

	void setNull(const OffsetBase& index, bool null)
	{
		*(ISC_SHORT*) (buffer + index.nullPos) = (ISC_SHORT) null;
	}

	template <typename T> T& operator [](const Offset<T>& index)
	{
		return *(T*) (buffer + index.pos);
	}

public:
	std::vector<ISC_UCHAR> blrArray;
	unsigned items;
};

template <>
class Offset<ISC_SHORT> : public OffsetBase
{
public:
	Offset(MessageImpl& message, ISC_UCHAR aScale = 0)
		: scale(aScale)
	{
		message.add(*this);
	}

	unsigned align(unsigned size)
	{
		return FB_ALIGN(size, sizeof(ISC_SHORT));
	}

	unsigned addBlr(std::vector<ISC_UCHAR>& blr)
	{
		blr.push_back(blr_short);
		blr.push_back(scale);
		return sizeof(ISC_SHORT);
	}

private:
	ISC_UCHAR scale;
};

template <>
class Offset<ISC_LONG> : public OffsetBase
{
public:
	Offset(MessageImpl& message, ISC_UCHAR aScale = 0)
		: scale(aScale)
	{
		message.add(*this);
	}

	unsigned align(unsigned size)
	{
		return FB_ALIGN(size, sizeof(ISC_LONG));
	}

	unsigned addBlr(std::vector<ISC_UCHAR>& blr)
	{
		blr.push_back(blr_long);
		blr.push_back(scale);
		return sizeof(ISC_LONG);
	}

private:
	ISC_UCHAR scale;
};

template <>
class Offset<ISC_INT64> : public OffsetBase
{
public:
	Offset(MessageImpl& message, ISC_UCHAR aScale = 0)
		: scale(aScale)
	{
		message.add(*this);
	}

	unsigned align(unsigned size)
	{
		return FB_ALIGN(size, sizeof(ISC_INT64));
	}

	unsigned addBlr(std::vector<ISC_UCHAR>& blr)
	{
		blr.push_back(blr_int64);
		blr.push_back(scale);
		return sizeof(ISC_INT64);
	}

private:
	ISC_UCHAR scale;
};

template <>
class Offset<float> : public OffsetBase
{
public:
	Offset(MessageImpl& message)
	{
		message.add(*this);
	}

	unsigned align(unsigned size)
	{
		return FB_ALIGN(size, sizeof(float));
	}

	unsigned addBlr(std::vector<ISC_UCHAR>& blr)
	{
		blr.push_back(blr_float);
		return sizeof(float);
	}
};

template <>
class Offset<double> : public OffsetBase
{
public:
	Offset(MessageImpl& message)
	{
		message.add(*this);
	}

	unsigned align(unsigned size)
	{
		return FB_ALIGN(size, sizeof(double));
	}

	unsigned addBlr(std::vector<ISC_UCHAR>& blr)
	{
		blr.push_back(blr_double);
		return sizeof(double);
	}
};

template <>
class Offset<ISC_QUAD> : public OffsetBase
{
public:
	Offset(MessageImpl& message)
	{
		message.add(*this);
	}

	unsigned align(unsigned size)
	{
		return FB_ALIGN(size, sizeof(ISC_QUAD));
	}

	unsigned addBlr(std::vector<ISC_UCHAR>& blr)
	{
		blr.push_back(blr_blob2);
		blr.push_back(0);
		blr.push_back(0);
		blr.push_back(0);
		blr.push_back(0);
		return sizeof(ISC_QUAD);
	}
};

struct FbString
{
	ISC_USHORT length;
	char str[1];

	std::string asStdString()
	{
		return std::string(str, length);
	}
};

template <>
class Offset<FbString> : public OffsetBase
{
public:
	Offset(MessageImpl& message, ISC_USHORT aLength)
		: length(aLength)
	{
		message.add(*this);
	}

	unsigned align(unsigned size)
	{
		return FB_ALIGN(size, sizeof(ISC_USHORT));
	}

	unsigned addBlr(std::vector<ISC_UCHAR>& blr)
	{
		blr.push_back(blr_varying);
		blr.push_back(length & 0xFF);
		blr.push_back((length >> 8) & 0xFF);
		return sizeof(ISC_USHORT) + length;
	}

private:
	ISC_USHORT length;
};

//// TODO: date, time, timestamp

//------------------------------------------------------------------------------

}	// V3Util
