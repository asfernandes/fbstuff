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
 * Portions created by the Initial Developer are Copyright (C) 2015 the Initial Developer.
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 */

#include "V3Util.h"
#include <string>
#include <boost/test/unit_test.hpp>

using std::string;
using namespace V3Util;
using namespace Firebird;

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(v3api)


static string legacyFormat(const intptr_t* vector)
{
	char buffer[1024];
	string s;
	int n;

	while ((n = fb_interpret(buffer, sizeof(buffer), &vector)) != 0)
	{
		if (!s.empty())
			s += "\n-";

		s += string(buffer, n);
	}

	return s;
}


BOOST_AUTO_TEST_CASE(status)
{
	IStatus* status = master->getStatus();

	intptr_t errors[] = {
		isc_arg_gds,
		isc_random,
		isc_arg_string,
		(intptr_t) "Error 1",
		isc_arg_gds,
		isc_random,
		isc_arg_string,
		(intptr_t) "Error 2",
		isc_arg_end
	};

	intptr_t warnings[] = {
		isc_arg_warning,
		isc_random,
		isc_arg_string,
		(intptr_t) "Warning 1",
		isc_arg_warning,
		isc_random,
		isc_arg_string,
		(intptr_t) "Warning 2",
		isc_arg_end
	};

	intptr_t errorsAndWarnings[
		sizeof(errors) / sizeof(errors[0]) - 1 + sizeof(warnings) / sizeof(warnings[0])];
	memcpy(errorsAndWarnings, errors, sizeof(errors) - sizeof(errors[0]));
	memcpy(errorsAndWarnings + sizeof(errors) / sizeof(errors[0]) - 1, warnings, sizeof(warnings));

	status->setErrors(errors);
	status->setWarnings(warnings);

	string s1(legacyFormat(errorsAndWarnings));

	IUtil* util = master->getUtilInterface();

	char buffer[1024];
	string s2(buffer, util->formatStatus(buffer, sizeof(buffer), status));

	BOOST_CHECK_EQUAL(s1, s2);

	status->dispose();
}


BOOST_AUTO_TEST_SUITE_END()
