#ifndef __STDAFX_H__
#define __STDAFX_H__

#ifdef _WIN32
	#include <SDKDDKVer.h>
#endif

#include <stdio.h>
#include <iostream>
#include <memory>

#ifdef _MSC_VER
#define DATAPATH "../../test/Data/"
#else
#define DATAPATH "../Data/"
#endif

// TODO: reference additional headers your program requires here
using namespace std;

//Adding required boost header
#define BOOST_TEST_DYN_LINK
#ifdef SINGLE_TEST_EXECUTABLES
#define BOOST_TEST_MODULE TEST_NAME
#endif
#include <boost/test/unit_test.hpp>

#endif