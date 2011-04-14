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

#define BOOST_TEST_MODULE fbtest

#include <boost/test/unit_test.hpp>
#include "FbTest.h"

namespace FbTest
{
	using std::string;

//------------------------------------------------------------------------------

static const string LOCATION = "/tmp/";

string getLocation(const string& file)
{
	return LOCATION + file;
}

//------------------------------------------------------------------------------

}	// FbTest
