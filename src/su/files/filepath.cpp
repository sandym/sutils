//
//  filepath.cpp
//  sutils
//
//  Created by Sandy Martel on 12/03/10.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any
// purpose is hereby granted without fee. The sotware is provided "AS-IS" and
// without warranty of any kind, express, implied or otherwise.
//

#include "su/files/filepath.h"
#include "su/base/cfauto.h"
#include "su/log/logger.h"
#include "su/strings/str_ext.h"
#include <string.h>
#include <algorithm>
#include <cassert>
#include <ciso646>
#include <map>

#if !UPLATFORM_WIN
#	include <dirent.h>
#	include <sys/stat.h>
#	include <unistd.h>
#else
#	include <Shlobj.h>
#	include <Windows.h>
#endif
#if UPLATFORM_MAC || UPLATFORM_IOS
#	include <sysdir.h>
#endif

namespace {

#if UPLATFORM_WIN
const char k_separators[] = {'/', '\\', 0};
const char k_illegalChars[] = {'?', ':', '*', '<', '>', 0};
const size_t k_minimumAbsolutePathLength = 3;
inline bool isSeparator( char c )
{
	return c == '/' or c == '\\';
}
#else
const char k_separators[] = {'/', 0};
const char k_illegalChars[] = {0};
const size_t k_minimumAbsolutePathLength = 1;
inline bool isSeparator( char c )
{
	return c == '/';
}
#endif

const char kPathTag[] = "PATH:";
const char kAliasTag[] = "ALIAS:";

/*!
   @brief Return true if a path is absolute.

       On windows, consider x:/ paths and unc path. On other platform, return
       true just for string starting with a separator.
   @param[in]     i_path the path to test
   @return     true the the path is absolute
*/
inline bool isAbsolute( const std::string_view &i_path )
{
#if UPLATFORM_WIN
	if ( i_path.length() < k_minimumAbsolutePathLength )
	{
		// unc or local path must be at least x char long ("c:\" or "\\s")
		return false;
	}
	if ( isalpha( i_path[0] ) and i_path[1] == ':' and
	     isSeparator( i_path[2] ) )
	{
		return true; // "c:\"
	}
	if ( isSeparator( i_path[0] ) and isSeparator( i_path[1] ) )
		return true; // "\\s" unc path
#else
	if ( not i_path.empty() and isSeparator( i_path[0] ) )
		return true;
#endif
	return false;
}

#if UPLATFORM_MAC

CFURLRef createURLRef( const std::string_view &i_path )
{
	return CFURLCreateFromFileSystemRepresentation(
	    nullptr, (const UInt8 *)i_path.data(), (CFIndex)i_path.size(), false );
}

std::string CFURLToPath( CFURLRef i_url )
{
	std::string p;
	if ( i_url != nullptr )
	{
		su::cfauto<CFStringRef> pathRef(
		    CFURLCopyFileSystemPath( i_url, kCFURLPOSIXPathStyle ) );
		return to_string( pathRef );
	}
	return p;
}

std::string getUserSysDir( sysdir_search_path_directory_t i_sysdir )
{
	auto state = sysdir_start_search_path_enumeration(
	    i_sysdir, SYSDIR_DOMAIN_MASK_USER );
	char buffer[PATH_MAX];
	state = sysdir_get_next_search_path_enumeration( state, buffer );
	if ( state != 0 )
	{
		su::filepath fs( su::filepath::location::kHome );
		fs.add( std::string_view( buffer + 2 ) );
		return fs.path();
	}
	return {};
}

#elif UPLATFORM_UNIX
std::string getUserSysDir( const std::string &i_key,
                           const std::string_view &i_default )
{
	su::filepath home( su::filepath::location::kHome );
	su::filepath fs( home );
	fs.add( ".config/user-dirs.dirs" );
	std::ifstream istr;
	if ( fs.fsopen( istr ) )
	{
		std::map<std::string, std::string> folderMap;
		std::string buf;
		while ( std::getline( istr, buf ) )
		{
			auto line = su::trim_spaces_view( buf );
			if ( line.empty() or line[0] == '#' )
				continue;
			auto l = su::split<char>( line, '=' );
			if ( l.size() != 2 )
				continue;
			auto p = su::trim_spaces_view( l[1] );
			if ( p.size() >= 2 and p.front() == '"' and p.back() == '"' )
			{
				p.remove_prefix( 1 );
				p.remove_suffix( 1 );
			}
			folderMap[su::trim_spaces( l[0] )] = p;
		}
		auto it = folderMap.find( i_key );
		if ( it != folderMap.end() )
		{
			auto path = it->second;
			if ( su::starts_with( path, "$HOME" ) )
			{
				path.erase( 0, 5 );
				path.insert( 0, home.path() );
			}
			return path;
		}
	}
	// fallback
	fs = home;
	fs.add( i_default );
	return fs.path();
}
#endif
}

namespace su {

filepath::filepath( const std::string_view &i_path,
                    const filepath *i_relativeTo )
{
	if ( isAbsolute( i_path ) )
	{
		// path is absolute
		_path = i_path;
	}
	else if ( i_relativeTo != nullptr and isAbsolute( i_relativeTo->_path ) )
	{
		_path = i_relativeTo->_path;
		add( i_path );
	}
}

filepath::filepath( location i_folder )
{
#if UPLATFORM_WIN
	wchar_t buffer[MAX_PATH + 1];
	// DWORD bufferLen;
#else
	char buffer[PATH_MAX];
#endif
	switch ( i_folder )
	{
		case location::kHome:
		{
#if UPLATFORM_WIN
			if ( SHGetFolderPathW(
			         0, CSIDL_PERSONAL, 0, SHGFP_TYPE_CURRENT, buffer ) == 0 )
			{
				_path.assign( su::to_string( buffer ) );
			}
#else
			const char *ev = getenv( "HOME" );
			if ( ev != nullptr )
				_path.assign( ev );
			else
				_path.assign( "/root" ); // ?
#endif
			break;
		}
		case location::kCurrentWorkingDir:
		{
#if UPLATFORM_WIN
			_path.assign( su::to_string( _wgetcwd( buffer, MAX_PATH ) ) );
#else
			_path.assign( getcwd( buffer, PATH_MAX ) );
#endif
			break;
		}
		case location::kApplicationFolder:
		{
#if UPLATFORM_MAC
			CFBundleRef bundleRef = CFBundleGetMainBundle();
			if ( bundleRef != 0 )
			{
				cfauto<CFURLRef> bundleURL(
				    CFBundleCopyBundleURL( bundleRef ) );
				if ( not bundleURL.isNull() )
				{
					_path = CFURLToPath( bundleURL );
					if ( not empty() )
						up();
				}
			}
			else
			{
				//	if no bundle found, use the current working directory...
				_path.assign( getcwd( buffer, PATH_MAX ) );
			}
#elif UPLATFORM_WIN
			if ( GetModuleFileNameW( 0, buffer, MAX_PATH ) != 0 )
			{
				_path.assign( su::to_string( buffer ) );
				if ( not empty() )
					up();
			}
#else
			// whatch out, readlink does NOT set a null terminator it seems...
			memset( buffer, 0, PATH_MAX );
			if ( readlink( "/proc/self/exe", buffer, PATH_MAX ) <= 0 )
				return;
			_path.assign( buffer );
			if ( not empty() )
				up();
#endif
			break;
		}
		case location::kApplicationSupport:
		{
#if UPLATFORM_MAC
			_path = getUserSysDir( SYSDIR_DIRECTORY_APPLICATION_SUPPORT );
#elif UPLATFORM_WIN
			if ( SHGetFolderPathW(
			         0, CSIDL_COMMON_APPDATA, 0, SHGFP_TYPE_CURRENT, buffer ) ==
			     0 )
			{
				_path.assign( su::to_string( buffer ) );
			}
#else
			filepath fs( location::kHome );
			fs.add( ".config" );
			_path = std::move( fs._path );
#endif
			break;
		}
		case location::kDesktop:
		{
#if UPLATFORM_MAC
			_path = getUserSysDir( SYSDIR_DIRECTORY_DESKTOP );
#elif UPLATFORM_WIN
			if ( SHGetFolderPathW( 0,
			                       CSIDL_DESKTOPDIRECTORY,
			                       0,
			                       SHGFP_TYPE_CURRENT,
			                       buffer ) == 0 )
			{
				_path.assign( su::to_string( buffer ) );
			}
#else
			_path = getUserSysDir( "XDG_DESKTOP_DIR", "Desktop" );
#endif
			break;
		}
		case location::kDocuments:
		{
#if UPLATFORM_MAC
			_path = getUserSysDir( SYSDIR_DIRECTORY_DOCUMENT );
#elif UPLATFORM_WIN
			if ( SHGetFolderPathW(
			         0, CSIDL_PERSONAL, 0, SHGFP_TYPE_CURRENT, buffer ) == 0 )
			{
				_path.assign( su::to_string( buffer ) );
			}
#else
			_path = getUserSysDir( "XDG_DOCUMENTS_DIR", "Documents" );
#endif
			break;
		}
		case location::kTempFolder:
		{
#if UPLATFORM_WIN
			if ( GetTempPathW( MAX_PATH, buffer ) != 0 )
				_path.assign( su::to_string( buffer ) );
#else
			const char *ev = getenv( "TMPDIR" );
			if ( ev == nullptr or *ev == 0 )
				ev = getenv( "TMP" );
			if ( ev == nullptr or *ev == 0 )
				ev = getenv( "TEMP" );
			if ( ev == nullptr or *ev == 0 )
				ev = getenv( "TEMPDIR" );
			if ( ev == nullptr or *ev == 0 )
				ev = "/tmp";
			_path.assign( ev );
#endif
			break;
		}
		case location::kNewTempSpec:
		{
			static int s_counter = 0;
			std::string uniqueName( "temp_" );
#if UPLATFORM_WIN
			uniqueName += std::to_string( GetCurrentProcessId() );
#else
			uniqueName += std::to_string( getpid() );
#endif
			uniqueName += "_" + std::to_string( s_counter++ );
			filepath folder( location::kTempFolder );
			_path = std::move( folder._path );
			add( uniqueName );
			break;
		}
		default:
			assert( false );
			break;
	}
}

filepath::filepath( const BookmarkData &i_bookmark,
                    const filepath *i_relativeTo )
{
	// make sure that the data start with "PATH:"
	if ( i_bookmark.size() <
	     sizeof( kPathTag ) - 1 + k_minimumAbsolutePathLength )
	{
		return;
	}
	if ( strncmp( i_bookmark.data(), kPathTag, sizeof( kPathTag ) - 1 ) != 0 )
		return;

#if UPLATFORM_MAC || UPLATFORM_IOS
	// on mac, we might have an optional "ALIAS:" tag as well
	size_t pathPartLen = strlen( i_bookmark.data() );
	if ( i_bookmark.size() > ( pathPartLen + sizeof( kAliasTag ) ) and
	     strncmp( i_bookmark.data() + pathPartLen + 1,
	              kAliasTag,
	              sizeof( kAliasTag ) - 1 ) == 0 )
	{
		// yes, we have "ALIAS:", extract the data
		size_t aliasSize =
		    i_bookmark.size() - ( pathPartLen + sizeof( kAliasTag ) );

		cfauto<CFDataRef> bookmarkData(
		    CFDataCreate( 0,
		                  (const UInt8 *)( i_bookmark.data() + pathPartLen +
		                                   sizeof( kAliasTag ) ),
		                  (CFIndex)aliasSize ) );
		cfauto<CFURLRef> relativeTo(
		    i_relativeTo != nullptr ? createURLRef( i_relativeTo->ospath() ) :
		                              nullptr );
		cfauto<CFURLRef> url( CFURLCreateByResolvingBookmarkData(
		    0, bookmarkData, 0, relativeTo, nullptr, nullptr, nullptr ) );
		_path = CFURLToPath( url );
		if ( not empty() )
			return;
	}
// fallback to the path.
#endif

	std::string_view the_path( i_bookmark.data() + 5 );
	filepath fs( the_path, i_relativeTo );
	_path = std::move( fs._path );
}

bool filepath::getBookmarkData( BookmarkData &o_data,
                                const filepath *i_relativeTo ) const
{
	o_data.clear();

	if ( empty() )
		return false;

	auto the_path = _path;
	if ( i_relativeTo != nullptr )
	{
		auto relativeTo = i_relativeTo->path();
		if ( not relativeTo.empty() )
		{
			//	this wil compute the current path relative to "relativeTo" path
			//	so if the_path == "/a/b/c/d/e" and relativeTo == "a/b/f/g"
			//	will compute "../../c/d/e"

#if UPLATFORM_WIN
			std::replace( relativeTo.begin(), relativeTo.end(), '\\', '/' );
			std::replace( the_path.begin(), the_path.end(), '\\', '/' );
#endif
			auto relativeToComp = split_view<char>( relativeTo, '/' );
			auto pathComp = split_view<char>( _path, '/' );
			std::pair<decltype( pathComp )::iterator,
			          decltype( pathComp )::iterator>
			    mismatchPoint;
			if ( pathComp.size() > relativeToComp.size() )
			{
				mismatchPoint = std::mismatch(
				    pathComp.begin(), pathComp.end(), relativeToComp.begin() );
				std::swap( mismatchPoint.first, mismatchPoint.second );
			}
			else
				mismatchPoint = std::mismatch( relativeToComp.begin(),
				                               relativeToComp.end(),
				                               pathComp.begin() );

			the_path.clear();
			auto goUp =
			    std::distance( mismatchPoint.first, relativeToComp.end() );
			for ( ; goUp > 0; --goUp )
				the_path += "../";
			for ( auto it = mismatchPoint.second; it != pathComp.end(); ++it )
			{
				the_path += *it;
				the_path.append( 1, '/' );
			}
			while ( the_path.length() > k_minimumAbsolutePathLength and
			        isSeparator( the_path[the_path.length() - 1] ) )
				the_path.erase( the_path.length() - 1 );
		}
	}

	//	append the path data
	o_data.insert( o_data.end(), kPathTag, kPathTag + sizeof( kPathTag ) - 1 );
	o_data.insert( o_data.end(), the_path.begin(), the_path.end() );
	o_data.push_back( 0 );

#if UPLATFORM_MAC
	//	add the "ALIAS:" tag for the Mac
	cfauto<CFURLRef> url( createURLRef( ospath() ) );
	cfauto<CFURLRef> relativeToURL( i_relativeTo != nullptr ?
	                                    createURLRef( i_relativeTo->ospath() ) :
	                                    nullptr );
	cfauto<CFErrorRef> error;
	cfauto<CFDataRef> bm(
	    CFURLCreateBookmarkData( nullptr,
	                             url,
	                             kCFURLBookmarkCreationSuitableForBookmarkFile,
	                             nullptr,
	                             relativeToURL,
	                             error.addr() ) );
	if ( bm )
	{
		o_data.push_back( '\0' );
		o_data.insert(
		    o_data.end(), kAliasTag, kAliasTag + sizeof( kAliasTag ) - 1 );
		auto ptr = (const char *)CFDataGetBytePtr( bm );
		o_data.insert( o_data.end(), ptr, ptr + CFDataGetLength( bm ) );
	}
	else if ( error )
	{
		cfauto<CFStringRef> desc( CFErrorCopyDescription( error ) );
		log_error() << to_string( desc );
	}
#endif

	return true;
}

std::string filepath::name() const
{
	std::string_view the_name;
	if ( _path.size() > k_minimumAbsolutePathLength )
	{
		auto pos = _path.find_last_of( k_separators );
		the_name = std::string_view( _path ).substr( pos + 1 );
	}
	return std::string{the_name};
}

std::string filepath::stem() const
{
	std::string_view the_name;
	if ( _path.size() > k_minimumAbsolutePathLength )
	{
		auto pos = _path.find_last_of( k_separators );
		the_name = std::string_view( _path ).substr( pos + 1 );
	}
	auto pos = the_name.find_last_of( '.' );
	the_name = the_name.substr( 0, pos );
	return std::string{the_name};
}

std::string filepath::extension() const
{
	auto the_name = name();
	auto pos = the_name.find_last_of( '.' );
	return the_name.substr( pos + 1 );
}

void filepath::setExtension( const std::string_view &i_ext )
{
	auto filename = stem();
	if ( not i_ext.empty() )
	{
		filename.append( 1, '.' );
		filename.append( i_ext.data(), i_ext.size() );
	}
	up();
	add( filename );
}

#if UPLATFORM_WIN
std::wstring filepath::ospath() const
{
	return su::to_wstring( _path );
}
#else
std::string filepath::ospath() const
{
	return _path;
}
#endif

bool filepath::exists() const
{
#if UPLATFORM_WIN
	auto attrs = GetFileAttributesW( ospath().c_str() );
	return ( attrs != INVALID_FILE_ATTRIBUTES );
#else
	struct stat info;
	return stat( ospath().c_str(), &info ) == 0;
#endif
}
bool filepath::isFolder() const
{
#if UPLATFORM_WIN
	auto attrs = GetFileAttributesW( ospath().c_str() );
	return ( attrs != INVALID_FILE_ATTRIBUTES ) and
	       ( attrs & FILE_ATTRIBUTE_DIRECTORY );
#else
	struct stat info;
	return ( stat( ospath().c_str(), &info ) == 0 ) and
	       ( ( info.st_mode & S_IFDIR ) != 0 );
#endif
}

bool filepath::isFile() const
{
#if UPLATFORM_WIN
	auto attrs = GetFileAttributesW( ospath().c_str() );
	return ( attrs != INVALID_FILE_ATTRIBUTES ) and
	       not( attrs & FILE_ATTRIBUTE_DIRECTORY );
#else
	struct stat info;
	return ( stat( ospath().c_str(), &info ) == 0 ) and
	       ( ( info.st_mode & S_IFREG ) != 0 );
#endif
}

std::time_t filepath::creation_date() const
{
#if UPLATFORM_WIN
	WIN32_FILE_ATTRIBUTE_DATA info;
	if ( GetFileAttributesExW(
	         ospath().c_str(), GetFileExInfoStandard, &info ) )
	{
		ULARGE_INTEGER ull;
		ull.LowPart = info.ftCreationTime.dwLowDateTime;
		ull.HighPart = info.ftCreationTime.dwHighDateTime;
		return ull.QuadPart / 10000000ULL - 11644473600ULL;
	}
#else
	struct stat info;
	if ( stat( ospath().c_str(), &info ) == 0 )
#	if UPLATFORM_MAC
		return info.st_birthtimespec.tv_sec;
#	else
		return info.st_mtim.tv_sec;
#	endif
#endif
	return 0;
}

size_t filepath::file_size() const
{
#if UPLATFORM_WIN
	WIN32_FILE_ATTRIBUTE_DATA info;
	if ( GetFileAttributesExW(
	         ospath().c_str(), GetFileExInfoStandard, &info ) )
	{
		assert( false );
	}
#else
	struct stat info;
	if ( stat( ospath().c_str(), &info ) == 0 )
#	if UPLATFORM_MAC
		return info.st_size;
#	else
		return info.st_size;
#	endif
#endif
	return 0;
}

bool filepath::isFolderEmpty() const
{
	bool foundOne = false;
#if UPLATFORM_WIN
	if ( not empty() )
	{
		WIN32_FIND_DATAW info;
		wcscpy( info.cFileName, L"." );
		auto startPath = ospath();
		startPath += L"/*.*";
		HANDLE fsIterator = FindFirstFileW( startPath.c_str(), &info );
		if ( fsIterator != INVALID_HANDLE_VALUE )
		{
			do
			{
				if ( wcscmp( info.cFileName, L"." ) != 0 and
				     wcscmp( info.cFileName, L".." ) != 0 )
					foundOne = true;
			} while ( not foundOne and FindNextFileW( fsIterator, &info ) );
			FindClose( fsIterator );
		}
	}
#else
	auto d = opendir( ospath().c_str() );
	if ( d != nullptr )
	{
		struct dirent *result;
		while ( not foundOne and ( result = readdir( d ) ) != nullptr )
		{
			if ( strcmp( result->d_name, "." ) != 0 and
			     strcmp( result->d_name, ".." ) != 0 )
				foundOne = true;
		}
		closedir( d );
	}
#endif
	return not foundOne;
}

std::vector<filepath> filepath::folderContent() const
{
	std::vector<filepath> l;

#if UPLATFORM_WIN
	if ( not empty() )
	{
		WIN32_FIND_DATAW info;
		wcscpy( info.cFileName, L"." );
		auto startPath = ospath();
		startPath += L"/*.*";
		HANDLE fsIterator = FindFirstFileW( startPath.c_str(), &info );
		if ( fsIterator != INVALID_HANDLE_VALUE )
		{
			do
			{
				if ( wcscmp( info.cFileName, L"." ) != 0 and
				     wcscmp( info.cFileName, L".." ) != 0 )
					l.emplace_back( _path + "/" +
					                su::to_string( info.cFileName ) );
			} while ( FindNextFileW( fsIterator, &info ) );
			FindClose( fsIterator );
		}
	}
#else
	auto d = opendir( ospath().c_str() );
	if ( d != nullptr )
	{
		struct dirent *result;
		while ( ( result = readdir( d ) ) != nullptr )
		{
			if ( strcmp( result->d_name, "." ) != 0 and
			     strcmp( result->d_name, ".." ) != 0 )
				l.emplace_back( _path + "/" + result->d_name );
		}
		closedir( d );
	}
#endif
	return l;
}

bool filepath::up()
{
	if ( _path.length() <= k_minimumAbsolutePathLength )
		return false;
	auto pos = _path.find_last_of( k_separators );
	_path = _path.substr( 0, pos + 1 );
	while ( _path.length() > k_minimumAbsolutePathLength and
	        isSeparator( _path.back() ) )
		_path.pop_back();
	return true;
}

bool filepath::add( const std::string_view &i_name )
{
	if ( isAbsolute( i_name ) )
		return false;
	if ( i_name.find_first_of( k_illegalChars ) != std::string_view::npos )
		return false;

	std::string_view the_name( i_name );
	while ( not the_name.empty() and isSeparator( the_name.back() ) )
		the_name.remove_suffix( 1 );

	filepath fspec( *this ); // work copy

	auto components = su::split_view_if<char>( the_name, isSeparator );
	for ( auto it : components )
	{
		if ( it.empty() or it == "." )
			continue;
		if ( it == ".." )
		{
			if ( not fspec.up() )
				return false;
		}
		else
		{
			if ( not fspec.exists() )
				return false;
			if ( not isSeparator( fspec._path[fspec._path.length() - 1] ) )
				fspec._path.append( 1, '/' );
			fspec._path.append( it );
		}
	}
	*this = std::move( fspec );
	return true;
}

std::ifstream &filepath::fsopen( std::ifstream &o_fs,
                                 std::ios_base::openmode i_mode ) const
{
	o_fs.open( ospath().c_str(), i_mode );
	return o_fs;
}

std::ofstream &filepath::fsopen( std::ofstream &o_fs,
                                 std::ios_base::openmode i_mode ) const
{
	o_fs.open( ospath().c_str(), i_mode );
	return o_fs;
}

std::fstream &filepath::fsopen( std::fstream &o_fs,
                                std::ios_base::openmode i_mode ) const
{
	o_fs.open( ospath().c_str(), i_mode );
	return o_fs;
}

bool filepath::unlink() const
{
	auto l = folderContent();
	for ( auto &it : l )
		it.unlink();

#if UPLATFORM_WIN
	SetFileAttributesW( ospath().c_str(), FILE_ATTRIBUTE_NORMAL );
	if ( DeleteFileW( ospath().c_str() ) )
		return true;
	if ( RemoveDirectoryW( ospath().c_str() ) )
		return true;
#else
	if (::unlink( ospath().c_str() ) == 0 )
		return true;
	if (::rmdir( ospath().c_str() ) == 0 )
		return true;
#endif
	return false;
}

bool filepath::mkdir() const
{
#if UPLATFORM_WIN
	if ( CreateDirectoryW( ospath().c_str(), 0 ) == FALSE )
	{
		auto err = GetLastError();
		log_error() << "CreateDirectory returned " << err;
		return false;
	}
	return true;
#else
	auto err = ::mkdir(
	    ospath().c_str(),
	    S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
	if ( err != 0 )
	{
		log_error() << "mkdir returned " << errno;
		return false;
	}
	return true;
#endif
}
// bool filepath::mkpath() const;
bool filepath::move( const filepath &i_dest ) const
{
	if ( i_dest == *this )
		return true;

	// dest should be valid and non-existing
	if ( i_dest.empty() )
	{
		log_error() << "attempt to move a file to an invalid destination";
		return false;
	}
	if ( i_dest.exists() )
	{
		log_error()
		    << "attempt to move a file to an already existing destination";
		return false;
	}

	// source has to exist
	if ( not exists() )
	{
		log_error() << "attempt to move a non-existing file";
		return false;
	}

#if UPLATFORM_WIN
	if ( MoveFileW( ospath().c_str(), i_dest.ospath().c_str() ) )
		return true;
#else
	if (::rename( ospath().c_str(), i_dest.ospath().c_str() ) == 0 )
		return true;
#endif
	return false;
}
// bool filepath::copy( const filepath &i_dest ) const;
}
