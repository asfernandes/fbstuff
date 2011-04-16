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

#include "V3Util.h"
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/assert.hpp>

namespace V3Util
{
	using namespace Firebird;
	using std::string;
	using boost::lexical_cast;

//------------------------------------------------------------------------------

IMaster* master = fb_get_master_interface();
IProvider* dispatcher = master->getDispatcher();

//--------------------------------------

string valueToString(IAttachment* attachment, ITransaction* transaction,
	MessageImpl& message, Offset<void*>& field)
{
	if (message.isNull(field))
		return "";

	void* p = message[field];

	switch (field.getType())
	{
		//// TODO: scale for SQL_SHORT, SQL_LONG and SQL_INT64

		case SQL_SHORT:
			return lexical_cast<string>(*static_cast<ISC_SHORT*>(p));

		case SQL_LONG:
			return lexical_cast<string>(*static_cast<ISC_LONG*>(p));

		case SQL_INT64:
			return lexical_cast<string>(*static_cast<ISC_INT64*>(p));

		case SQL_FLOAT:
			return lexical_cast<string>(*static_cast<float*>(p));

		case SQL_DOUBLE:
			return lexical_cast<string>(*static_cast<double*>(p));

		case SQL_BLOB:
		{
			IStatus* status = master->getStatus();

			IBlob* blob = attachment->openBlob(status, transaction,
				static_cast<ISC_QUAD*>(p), 0, NULL);
			BOOST_VERIFY(status->isSuccess());

			string str;
			char blobBuffer[1024];
			unsigned blobLen;

			while ((blobLen = blob->getSegment(status, sizeof(blobBuffer), blobBuffer)) != 0)
				str.append(blobBuffer, blobLen);

			blob->close(status);
			BOOST_VERIFY(status->isSuccess());

			status->dispose();

			return str;
		}

		case SQL_TEXT:
		case SQL_VARYING:
			return string(static_cast<char*>(p) + sizeof(ISC_USHORT),
				*static_cast<ISC_USHORT*>(p));

		default:
			BOOST_VERIFY(false);
			return "";
	}
}

//------------------------------------------------------------------------------

}	// V3Util
