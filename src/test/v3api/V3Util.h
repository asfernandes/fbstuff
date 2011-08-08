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
#include <firebird/Interface.h>
#include <firebird/Message.h>
#include <firebird/Provider.h>
#include <string>
#include <vector>
#include <string.h>
#include <ibase.h>
#include <boost/assert.hpp>

namespace V3Util
{

//------------------------------------------------------------------------------

extern Firebird::IMaster* master;
extern Firebird::IProvider* dispatcher;

//--------------------------------------

static inline size_t ALIGN(size_t n, size_t b)
{
	return ((n + b - 1) & ~(b - 1));
}

class MessageImpl;

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

template <>
class Offset<void*> : public OffsetBase
{
public:
	Offset(MessageImpl& message, const Firebird::IParametersMetadata* aParams);

	unsigned align(unsigned size, unsigned aIndex)
	{
		index = aIndex;

		Firebird::IStatus* status = master->getStatus();

		switch ((type = params->getType(status, index)))
		{
			case SQL_SHORT:
				size = ALIGN(size, sizeof(ISC_SHORT));
				break;

			case SQL_LONG:
				size = ALIGN(size, sizeof(ISC_LONG));
				break;

			case SQL_INT64:
				size = ALIGN(size, sizeof(ISC_INT64));
				break;

			case SQL_FLOAT:
				size = ALIGN(size, sizeof(float));
				break;

			case SQL_DOUBLE:
				size = ALIGN(size, sizeof(double));
				break;

			case SQL_BLOB:
				size = ALIGN(size, sizeof(ISC_QUAD));
				break;

			case SQL_TEXT:
			case SQL_VARYING:
				size = ALIGN(size, sizeof(ISC_USHORT));
				break;

			default:
				BOOST_VERIFY(false);
				break;
		}

		status->dispose();

		return size;
	}

	unsigned addBlr(ISC_UCHAR*& blr)
	{
		Firebird::IStatus* status = master->getStatus();
		unsigned ret;

		switch (type)
		{
			case SQL_SHORT:
			{
				unsigned scale = params->getScale(status, index);
				*blr++ = blr_short;
				*blr++ = scale;
				ret = sizeof(ISC_SHORT);
				break;
			}

			case SQL_LONG:
			{
				unsigned scale = params->getScale(status, index);
				*blr++ = blr_long;
				*blr++ = scale;
				ret = sizeof(ISC_LONG);
				break;
			}

			case SQL_INT64:
			{
				unsigned scale = params->getScale(status, index);
				*blr++ = blr_int64;
				*blr++ = scale;
				ret = sizeof(ISC_INT64);
				break;
			}

			case SQL_FLOAT:
				*blr++ = blr_float;
				ret = sizeof(float);
				break;

			case SQL_DOUBLE:
				*blr++ = blr_double;
				ret = sizeof(double);
				break;

			case SQL_BLOB:
				*blr++ = blr_blob2;
				*blr++ = 0;
				*blr++ = 0;
				*blr++ = 0;
				*blr++ = 0;
				ret = sizeof(ISC_QUAD);
				break;

			case SQL_TEXT:
			case SQL_VARYING:
			{
				unsigned length = params->getLength(status, index);
				*blr++ = blr_varying;
				*blr++ = length & 0xFF;
				*blr++ = (length >> 8) & 0xFF;
				ret = sizeof(ISC_USHORT) + length;
				break;
			}

			default:
				BOOST_VERIFY(false);
				ret = 0;
				break;
		}

		status->dispose();

		return ret;
	}

	unsigned getType() const
	{
		return type;
	}

private:
	const Firebird::IParametersMetadata* params;
	unsigned type;
	unsigned index;
};

class MessageImpl : public Firebird::FbMessage
{
public:
	MessageImpl(unsigned aItemCount)
		: itemCount(aItemCount * 2),
		  items(0)
	{
		static const ISC_UCHAR HEADER[] = {
			blr_version5,
			blr_begin,
			blr_message, 0, 0, 0
		};

		blrLength = 0;
		blr = blrPos = new ISC_UCHAR[sizeof(HEADER) + 10 * itemCount + 2];
		bufferLength = 0;
		buffer = NULL;

		memcpy(blrPos, HEADER, sizeof(HEADER));
		blrPos += sizeof(HEADER);
	}

	~MessageImpl()
	{
		if (buffer)
			delete [] buffer;

		if (blr)
			delete [] blr;
	}

	template <typename T>
	void add(Offset<T>& offset)
	{
		if (items >= itemCount)
			return;	// return an error, this is already constructed message

		bufferLength = offset.align(bufferLength, items / 2);
		offset.pos = bufferLength;
		bufferLength += offset.addBlr(blrPos);

		bufferLength = ALIGN(bufferLength, sizeof(ISC_SHORT));
		offset.nullPos = bufferLength;
		bufferLength += sizeof(ISC_SHORT);

		*blrPos++ = blr_short;
		*blrPos++ = 0;

		items += 2;

		if (items == itemCount)
		{
			*blrPos++ = blr_end;
			*blrPos++ = blr_eoc;

			blrLength = blrPos - blr;

			ISC_UCHAR* blrStart = blrPos - blrLength;
			blrStart[4] = items & 0xFF;
			blrStart[5] = (items >> 8) & 0xFF;

			buffer = new ISC_UCHAR[bufferLength];
			memset(buffer, 0, bufferLength);
		}
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

	void* operator [](const Offset<void*>& index)
	{
		return buffer + index.pos;
	}

public:
	unsigned itemCount;
	unsigned items;
	ISC_UCHAR* blrPos;
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

	unsigned align(unsigned size, unsigned /*index*/)
	{
		return ALIGN(size, sizeof(ISC_SHORT));
	}

	unsigned addBlr(ISC_UCHAR*& blr)
	{
		*blr++ = blr_short;
		*blr++ = scale;
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

	unsigned align(unsigned size, unsigned /*index*/)
	{
		return ALIGN(size, sizeof(ISC_LONG));
	}

	unsigned addBlr(ISC_UCHAR*& blr)
	{
		*blr++ = blr_long;
		*blr++ = scale;
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

	unsigned align(unsigned size, unsigned /*index*/)
	{
		return ALIGN(size, sizeof(ISC_INT64));
	}

	unsigned addBlr(ISC_UCHAR*& blr)
	{
		*blr++ = blr_int64;
		*blr++ = scale;
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

	unsigned align(unsigned size, unsigned /*index*/)
	{
		return ALIGN(size, sizeof(float));
	}

	unsigned addBlr(ISC_UCHAR*& blr)
	{
		*blr++ = blr_float;
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

	unsigned align(unsigned size, unsigned /*index*/)
	{
		return ALIGN(size, sizeof(double));
	}

	unsigned addBlr(ISC_UCHAR*& blr)
	{
		*blr++ = blr_double;
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

	unsigned align(unsigned size, unsigned /*index*/)
	{
		return ALIGN(size, sizeof(ISC_QUAD));
	}

	unsigned addBlr(ISC_UCHAR*& blr)
	{
		*blr++ = blr_blob2;
		*blr++ = 0;
		*blr++ = 0;
		*blr++ = 0;
		*blr++ = 0;
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

	unsigned align(unsigned size, unsigned /*index*/)
	{
		return ALIGN(size, sizeof(ISC_USHORT));
	}

	unsigned addBlr(ISC_UCHAR*& blr)
	{
		*blr++ = blr_varying;
		*blr++ = length & 0xFF;
		*blr++ = (length >> 8) & 0xFF;
		return sizeof(ISC_USHORT) + length;
	}

private:
	ISC_USHORT length;
};

//// TODO: boolean, date, time, timestamp

//--------------------------------------

inline Offset<void*>::Offset(MessageImpl& message, const Firebird::IParametersMetadata* aParams)
	: params(aParams),
	  type(0)
{
	message.add(*this);
}

//--------------------------------------

void getEngineVersion(Firebird::IAttachment* attachment, unsigned* major, unsigned* minor = NULL,
	unsigned* revision = NULL);
std::string valueToString(Firebird::IAttachment* attachment, Firebird::ITransaction* transaction,
	MessageImpl& message, Offset<void*>& field);

//------------------------------------------------------------------------------

}	// V3Util
