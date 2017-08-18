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

#if (UPLATFORM_MAC || UPLATFORM_IOS)
#include "cfauto.h"
#endif

#if UPLATFORM_WIN
#include <Windows.h>
#endif

namespace su {
namespace resource_access {

su::filepath getFolder()
{
#if (UPLATFORM_MAC || UPLATFORM_IOS)
	cfauto<CFURLRef> urlRef( CFBundleCopyResourcesDirectoryURL( CFBundleGetMainBundle() ) );
	// get the filespec for the resource's url
	cfauto<CFStringRef> pathRef( CFURLCopyFileSystemPath( urlRef, kCFURLPOSIXPathStyle ) );
	return su::filepath( su::to_string( pathRef ) );
#else
	su::filepath fs( su::filepath::location::kApplicationFolder );
#if defined(RSRC_FOLDER_NAME)
	fs.add( RSRC_FOLDER_NAME );
#else
	fs.add( "Resources" );
#endif
	return fs;
#endif
}

su::filepath get( const std::string_view &i_name )
{
#if (UPLATFORM_MAC || UPLATFORM_IOS)
	// we have a relative path to the resource in i_name, we want to split the string in 3: sub-folder path, name and extension
	auto p = i_name.rfind( '/' );
	std::string_view subDir, name;
	if ( p != std::string_view::npos )
	{
		subDir = i_name.substr( 0, p );
		name = i_name.substr( p + 1 );
	}
	else
		name = i_name;

	p = name.rfind( '.' );
	std::string_view ext;
	if ( p != std::string_view::npos )
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
	static su::filepath rsrc = getFolder();
	static su::filepath s_locale;
	if ( s_locale.empty() )
	{
		std::string specific;

		wchar_t lang[8];
		/*int s =*/ GetLocaleInfoW( LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lang, 8 );
		auto locale = su::to_string( lang );
		specific = locale;
		if ( locale == "en" )
		{
			switch ( SUBLANGID( ::GetUserDefaultLangID() ) )
			{
				case SUBLANG_ENGLISH_US:
					specific += "_US";
					break;
				default:
					break;
			}
		}
		if ( specific != locale )
		{
			auto f = rsrc;
			f.add( specific + ".lproj" );
			if ( f.isFolder() )
				s_locale = f;
		}
		if ( s_locale.empty() )
		{
			auto f = rsrc;
			f.add( locale + ".lproj" );
			if ( f.isFolder() )
				s_locale = f;
		}
		if ( s_locale.empty() )
		{
			s_locale = rsrc;
			s_locale.add( "en.lproj" );
		}
	}
	
	// todo: optimise by building a map of available resources on first call
	// instead of 3 disk access for each call...

	// try to find the localised resource
	auto resourceFile = s_locale;
	resourceFile.add( i_name );
	if ( resourceFile.exists() ) // file access
		return resourceFile;
	
	// try english
	resourceFile = rsrc;
	resourceFile.add( "en.lproj/" + std::string(i_name) );
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
