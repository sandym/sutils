//
//  su_filepath.h
//  sutils
//
//  Created by Sandy Martel on 12/03/10.
//  Copyright (c) 2015年 Sandy Martel. All rights reserved.
//
// Permission to use, copy, modify, distribute, and sell this software for any
// purpose is hereby granted without fee. The sotware is provided "AS-IS" and
// without warranty of any kind, express, implied or otherwise.
//

#ifndef H_SU_FILEPATH
#define H_SU_FILEPATH

#include "su_platform.h"
#include <string_view>
#include <vector>
#include <memory>
#include <ctime>
#include <fstream>

namespace su {

class filepath final
{
public:
	filepath() = default;
	filepath( const std::string_view &i_path,
				const filepath *i_relativeTo = nullptr );
	
	filepath( const filepath &rhs ) = default;
	filepath &operator=( const filepath &rhs ) = default;
	filepath( filepath &&rhs ) = default;
	filepath &operator=( filepath &&rhs ) = default;
	
	//	file spec initialization from special file system location
	enum class location
	{
		kHome,
		kCurrentWorkingDir,
		kApplicationFolder,
		kApplicationSupport,
		kDesktop,
		kDocuments,
		kTempFolder, //!< temp folder for the current running instance
		kNewTempSpec //!< a new, non-existing file path
	};
	filepath( location i_folder );

	using BookmarkData = std::vector<char>;
	filepath( const BookmarkData &i_fileBookmark,
				const filepath *i_relativeTo = nullptr );
	bool getBookmarkData( BookmarkData &o_data,
				const filepath *i_relativeTo = nullptr ) const;

	bool empty() const { return _path.empty(); }
	
	void clear() { _path.clear(); }
	
	// info
	bool operator==( const filepath &rhs ) const { return _path == rhs._path; }
	bool operator!=( const filepath &rhs ) const { return _path != rhs._path; }
	bool operator<( const filepath &rhs ) const { return _path < rhs._path; }
	bool operator<=( const filepath &rhs ) const { return _path <= rhs._path; }
	bool operator>( const filepath &rhs ) const { return _path > rhs._path; }
	bool operator>=( const filepath &rhs ) const { return _path >= rhs._path; }
	
	const std::string &path() const { return _path; }
	
	std::string name() const;
	std::string stem() const;
	std::string extension() const;
	void setExtension( const std::string_view &i_ext );

#if UPLATFORM_WIN
	std::wstring ospath() const;
#else
	std::string ospath() const;
#endif

	bool empty() { return _path.empty(); }
	bool exists() const;
	bool isFolder() const;
	bool isFile() const;

	std::time_t creation_date() const;
	size_t file_size() const;

	// iterating folder
	bool isFolderEmpty() const;
	std::vector<filepath> folderContent() const;

	// Navigation
	bool up();
	bool add( const std::string_view &i_name );

	// opening files
	std::ifstream &fsopen( std::ifstream &o_fs,
					std::ios_base::openmode i_mode = std::ios_base::in |
												std::ios_base::binary ) const;
	std::ofstream &fsopen( std::ofstream &o_fs,
					std::ios_base::openmode i_mode = std::ios_base::out |
												std::ios_base::trunc |
												std::ios_base::binary ) const;
	std::fstream &fsopen( std::fstream &o_fs,
					std::ios_base::openmode i_mode = std::ios_base::in |
												std::ios_base::out |
												std::ios_base::binary ) const;

	// file operation
	bool unlink() const;
	bool mkdir() const;
	bool mkpath() const;
	bool move( const filepath &i_dest ) const;
	bool copy( const filepath &i_dest ) const;

private:
	std::string _path;
};

}

inline std::string to_string( const su::filepath &v ) { return v.path(); }

#endif


