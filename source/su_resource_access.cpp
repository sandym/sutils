/*
 *  su_resource_access.cpp
 *  sutils
 *
 *  Created by Sandy Martel on 08-09-07.
 *  Copyright (c) 2015年 Sandy Martel. All rights reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software for any purpose is hereby
 * granted without fee. The sotware is provided "AS-IS" and without warranty of any kind, express,
 * implied or otherwise.
 */

#include "su_resource_access.h"
#include "su_platform.h"
#include "su_string.h"

#if UPLATFORM_MAC || UPLATFORM_IOS
#define USE_CF_IMPLEMENTAION
#include "cfauto.h"
#endif

#if UPLATFORM_WIN
#include <Windows.h>
#endif

namespace su {
namespace resource_access {

su::filepath getFolder()
{
#ifdef USE_CF_IMPLEMENTAION
	cfauto<CFURLRef> urlRef( CFBundleCopyResourcesDirectoryURL( CFBundleGetMainBundle() ) );
	// get the filespec for the resource's url
	cfauto<CFStringRef> pathRef( CFURLCopyFileSystemPath( urlRef, kCFURLPOSIXPathStyle ) );
	return su::filepath( su::to_string( pathRef ) );
#else
	su::filepath fs( su::filepath::kApplicationFolder );
	fs.add( "Resources" );
	return fs;
#endif
}

su::filepath get( const su_std::string_view &i_name )
{
#ifdef USE_CF_IMPLEMENTAION
	// we have a relative path to the resource in i_name, we want to split the string in 3: sub-folder path, name and extension
	auto p = i_name.rfind( '/' );
	su_std::string_view subDir, name;
	if ( p != su_std::string_view::npos )
	{
		subDir = i_name.substr( 0, p );
		name = i_name.substr( p + 1 );
	}
	else
		name = i_name;

	p = name.rfind( '.' );
	su_std::string_view ext;
	if ( p != su_std::string_view::npos )
	{
		ext = name.substr( p + 1 );
		name = name.substr( 0, p );
	}
	
	// ustrings to CFString
	cfauto<CFStringRef> resourceName( not name.empty() ? su::CreateCFString(name) : nullptr );
	cfauto<CFStringRef> resourceType( not ext.empty() ? su::CreateCFString(ext) : nullptr );
	cfauto<CFStringRef> subDirName( not subDir.empty() ? su::CreateCFString(subDir) : nullptr );
	
	cfauto<CFURLRef> urlRef( CFBundleCopyResourceURL( CFBundleGetMainBundle(), resourceName, resourceType, subDirName ) );
	// get the filespec for the resource's url
	cfauto<CFStringRef> pathRef( CFURLCopyFileSystemPath( urlRef, kCFURLPOSIXPathStyle ) );
	return su::filepath( su::to_string( pathRef ) );
#else
	static ustring s_locale;
	if ( s_locale.empty() )
	{
		// predefined languages
		const std::vector<ustring> supportedLocal = { "en" /*, "en_US", "ja", "fr"*/ };

		wchar_t lang[32];
		/*int s =*/ GetLocaleInfoW( LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lang, 32 );
		s_locale.assign( lang );
		if ( s_locale == "en" and SUBLANGID( ::GetUserDefaultLangID() ) == SUBLANG_ENGLISH_US )
		{
			s_locale += "_US";
		}
		if ( su::find( supportedLocal, s_locale ) == supportedLocal.end() )
			s_locale = "en";
	}
	auto rsrc = getFolder();
	
	// todo: optimise by building a map of available resources on first call
	// instead of 3 disk access for each call...

	// try to find the localised resource
	auto resourceFile = rsrc;
	resourceFile.add( s_locale + ".lproj/" + i_name );
	if ( resourceFile.exists() ) // file access
		return resourceFile;
	
	// try english
	resourceFile = rsrc'
	resourceFile.add( "en.lproj/" + i_name );
	if ( resourceFile.exists() ) // file access
		return resourceFile;

	// try un-localised
	resourceFile = rsrc;
	resourceFile.add( i_name );
	if ( resourceFile.exists() ) // file access
		return resourceFile;
	
	// give up, resource not found...
	return {};
#endif
}

}
}
