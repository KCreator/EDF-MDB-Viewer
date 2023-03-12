#pragma once //Compile this once.

//CMPL Decompressor
std::vector< char > CMPLDecompress( std::vector< char > in, bool verbose );

//RAB File storage struct
struct RABFile
{
	std::wstring folder;
	std::wstring name;
	int position;
	int filesize;
};

//Simple RAB Reader implementation
class RABReader
{
public:
	RABReader( const char *path ); //Read from path
	std::vector< char > ReadFile( std::wstring folder, std::wstring file );

	//Util fns
	bool HasFolder( std::wstring folderName );

	//Stored Data
	int numFiles;
	int numFolders;
	int nameTablePos;
	int fileTreeStructPos;
	int dataStartOfs;

	std::vector< std::wstring> folders;
	std::vector< RABFile > files;

	std::vector< char > data;
};
