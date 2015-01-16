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

class MessageImpl;

class OffsetBase
{
public:
	OffsetBase()
		: index(0)
	{
	}

	unsigned index;
};

template <class T>
class Offset : public OffsetBase
{
public:
};

template <>
class Offset<void*> : public OffsetBase
{
public:
	template <typename StatusType>
	Offset(StatusType* status, MessageImpl& message);

	template <typename StatusType>
	void setType(StatusType* /*status*/, Firebird::IMetadataBuilder* /*builder*/)
	{
	}
};

class MessageImpl
{
public:
	template <typename StatusType>
	MessageImpl(StatusType* status, Firebird::IMessageMetadata* aMetadata)
		: metadata(aMetadata),
		  buffer(NULL),
		  offsets(NULL),
		  item(0)
	{
		itemCount = metadata->getCount(status);
		builder = metadata->getBuilder(status);
	}

	~MessageImpl()
	{
		builder->release();
		delete [] offsets;
		delete [] buffer;
	}

	template <typename StatusType, typename T>
	void add(StatusType* status, Offset<T>& offset)
	{
		if (item >= itemCount)
			return;	// return an error, this is already constructed message

		offset.index = item;

		if (metadata->getType(status, item) == SQL_TEXT)
			builder->setType(status, item, SQL_VARYING);

		offset.setType(status, builder);

		if (++item == itemCount)
		{
			metadata = builder->getMetadata(status);

			unsigned bufferLength = metadata->getMessageLength(status);
			buffer = new ISC_UCHAR[bufferLength];
			memset(buffer, 0, bufferLength);

			offsets = new unsigned[itemCount * 2];

			for (unsigned i = 0; i < itemCount; ++i)
			{
				offsets[i * 2] = metadata->getOffset(status, i);
				offsets[i * 2 + 1] = metadata->getNullOffset(status, i);
			}
		}
	}

	bool isNull(const OffsetBase& offset)
	{
		return *(ISC_SHORT*) (buffer + offsets[offset.index * 2 + 1]);
	}

	void setNull(const OffsetBase& offset, bool null)
	{
		*(ISC_SHORT*) (buffer + offsets[offset.index * 2 + 1]) = null ? -1 : 0;
	}

	template <typename T> T& operator [](const Offset<T>& offset)
	{
		return *(T*) (buffer + offsets[offset.index * 2]);
	}

	void* operator [](const Offset<void*>& offset)
	{
		return buffer + offsets[offset.index * 2];
	}

	Firebird::IMessageMetadata* getMetadata()
	{
		return metadata;
	}

	void* getBuffer()
	{
		return buffer;
	}

private:
	Firebird::IMessageMetadata* metadata;
	ISC_UCHAR* buffer;
	unsigned* offsets;
	unsigned item;
	unsigned itemCount;
	Firebird::IMetadataBuilder* builder;
};

template <>
class Offset<ISC_SHORT> : public OffsetBase
{
public:
	template <typename StatusType>
	Offset(StatusType* status, MessageImpl& message, ISC_UCHAR aScale = 0)
		: scale(aScale)
	{
		message.add(status, *this);
	}

	template <typename StatusType>
	void setType(StatusType* status, Firebird::IMetadataBuilder* builder)
	{
		builder->setType(status, index, SQL_SHORT);
		builder->setScale(status, index, scale);
		builder->setLength(status, index, sizeof(ISC_SHORT));
	}

private:
	ISC_UCHAR scale;
};

template <>
class Offset<ISC_LONG> : public OffsetBase
{
public:
	template <typename StatusType>
	Offset(StatusType* status, MessageImpl& message, ISC_UCHAR aScale = 0)
		: scale(aScale)
	{
		message.add(status, *this);
	}

	template <typename StatusType>
	void setType(StatusType* status, Firebird::IMetadataBuilder* builder)
	{
		builder->setType(status, index, SQL_LONG);
		builder->setScale(status, index, scale);
		builder->setLength(status, index, sizeof(ISC_LONG));
	}

private:
	ISC_UCHAR scale;
};

template <>
class Offset<ISC_INT64> : public OffsetBase
{
public:
	template <typename StatusType>
	Offset(StatusType* status, MessageImpl& message, ISC_UCHAR aScale = 0)
		: scale(aScale)
	{
		message.add(status, *this);
	}

	template <typename StatusType>
	void setType(StatusType* status, Firebird::IMetadataBuilder* builder)
	{
		builder->setType(status, index, SQL_INT64);
		builder->setScale(status, index, scale);
		builder->setLength(status, index, sizeof(ISC_INT64));
	}

private:
	ISC_UCHAR scale;
};

template <>
class Offset<float> : public OffsetBase
{
public:
	template <typename StatusType>
	Offset(StatusType* status, MessageImpl& message)
	{
		message.add(status, *this);
	}

	template <typename StatusType>
	void setType(StatusType* status, Firebird::IMetadataBuilder* builder)
	{
		builder->setType(status, index, SQL_FLOAT);
		builder->setLength(status, index, sizeof(float));
	}
};

template <>
class Offset<double> : public OffsetBase
{
public:
	template <typename StatusType>
	Offset(StatusType* status, MessageImpl& message)
	{
		message.add(status, *this);
	}

	template <typename StatusType>
	void setType(StatusType* status, Firebird::IMetadataBuilder* builder)
	{
		builder->setType(status, index, SQL_DOUBLE);
		builder->setLength(status, index, sizeof(double));
	}
};

template <>
class Offset<ISC_QUAD> : public OffsetBase
{
public:
	template <typename StatusType>
	Offset(StatusType* status, MessageImpl& message)
	{
		message.add(status, *this);
	}

	template <typename StatusType>
	void setType(StatusType* status, Firebird::IMetadataBuilder* builder)
	{
		builder->setType(status, index, SQL_BLOB);
		builder->setLength(status, index, sizeof(ISC_QUAD));
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
	template <typename StatusType>
	Offset(StatusType* status, MessageImpl& message, ISC_USHORT aLength)
		: length(aLength)
	{
		message.add(status, *this);
	}

	template <typename StatusType>
	void setType(StatusType* status, Firebird::IMetadataBuilder* builder)
	{
		builder->setType(status, index, SQL_VARYING);
		builder->setLength(status, index, length);
	}

private:
	ISC_USHORT length;
};

template <typename StatusType>
inline Offset<void*>::Offset(StatusType* status, MessageImpl& message)
{
	message.add(status, *this);
}

//// TODO: boolean, date, time, timestamp

//--------------------------------------

bool checkStatus(Firebird::IStatus* status);
void getEngineVersion(Firebird::IAttachment* attachment, unsigned* major, unsigned* minor = NULL,
	unsigned* revision = NULL);
std::string valueToString(Firebird::IAttachment* attachment, Firebird::ITransaction* transaction,
	MessageImpl& message, Offset<void*>& field);

//------------------------------------------------------------------------------

}	// V3Util
