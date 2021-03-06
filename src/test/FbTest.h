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

#ifndef FBSTUFF_TEST_FB_TEST_H
#define FBSTUFF_TEST_FB_TEST_H

#include <string>
#include <ibase.h>

namespace FbTest
{

//------------------------------------------------------------------------------


const int DIALECT = 3;
const unsigned char ASCII_DPB[] = {
	isc_dpb_version1,
	isc_dpb_lc_ctype, 5, 'A', 'S', 'C', 'I', 'I'
};
const unsigned char UTF8_DPB[] = {
	isc_dpb_version1,
	isc_dpb_lc_ctype, 4, 'U', 'T', 'F', '8'
};

std::string getLocation(const std::string& file);


//------------------------------------------------------------------------------

}	// FbTest

#endif	// FBSTUFF_TEST_FB_TEST_H
