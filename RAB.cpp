#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Util.h"
#include "RAB.h"

//CMPL Decompressor
std::vector< char > CMPLDecompress( std::vector< char > data )
{
	//Check header:
	if( data[0] != 'C' && data[1] != 'M' && data[2] != 'P' && data[3] != 'L' )
	{
		std::wcout << L"FILE IS NOT CMPL COMPRESSED!\n";
		return data;
	}
	else
		std::wcout << L"BEGINNING DECOMPRESSION\n";

	//Variables
	uint8_t bitbuf;
	uint8_t bitbufcnt;

	std::vector< char > out;

	char mainbuf[4096];

	int bufPos;

	int desiredSize = ReadInt( &data, 4, true );

	int streamPos = 8;

	//Init buffer:
	for( int i = 0; i < 4078; ++i )
		mainbuf[i] = 0;
	bitbuf = 0;
	bitbufcnt = 0;
	bufPos = 4078;

	//Loop
	while( streamPos < data.size( ) )
	{
		//If bitbuf empty
		if( bitbufcnt == 0 )
		{
			//Read and store in buffer
			bitbuf = data[streamPos];
			bitbufcnt = 8;

			++streamPos;
		}

		//if first bit is one, copy byte from input
		if( bitbuf & 0x1 > 0 )
		{
			out.push_back( data[streamPos] );
			mainbuf[bufPos] = data[streamPos];
			++bufPos;
			if( bufPos >= 4096 )
				bufPos = 0;
			++streamPos;
		}
		else //Copy bytes from buffer
		{
			uint8_t chunk[2];
			chunk[1] = data[streamPos + 1];
			chunk[0] = data[streamPos];

			streamPos += 2;

			uint16_t val = ( chunk[0] << 8 ) | chunk[1];
			int copyLen = ( val & 0xf ) + 3;
			int copyPos = val >> 4;

			for( int i = 0; i < copyLen; ++i )
			{
				unsigned char byte = mainbuf[copyPos];
				out.push_back( byte );
				mainbuf[bufPos] = byte;

				++bufPos;
				if( bufPos >= 4096 )
					bufPos = 0;

				++copyPos;
				if( copyPos >= 4096 )
					copyPos = 0;
			}
		}

		--bitbufcnt;
		bitbuf >>= 1;
	}

	if( out.size( ) == desiredSize )
	{
		std::wcout << L"FILE SIZE MATCH! " + std::to_wstring( desiredSize ) + L" bytes expected, got " + std::to_wstring( (int)out.size( ) ) +  L" DECOMPRESSION SUCCESSFUL!\n";
	}
	else
		std::wcout << L"FILE SIZE MISMATCH! " + std::to_wstring( desiredSize ) + L" bytes expected, got " + std::to_wstring( (int)out.size( ) ) + L" DECOMPRESSION FAILED!\n";

	return out;
}

//Based on my old RAB reader, simplified for this project
RABReader::RABReader( const char *path )
{
	//Create input stream from path
    std::ifstream file( path, std::ios::binary | std::ios::ate );
    std::streamsize size = file.tellg( );
	file.seekg( 0, std::ios::beg );

	std::vector<char> buffer( size );
	if( file.read( buffer.data( ), size ) )
	{
		//Keep a copy of the data.
		data = buffer;
		//Parse the RAB file, keeping track of certain data

		//Begin read
		int pos;

		dataStartOfs = ReadInt( &buffer, 0x8 ); //Data starting offset

		numFiles = ReadInt( &buffer, 0x14 ); //Number of archived files

		fileTreeStructPos = ReadInt( &buffer, 0x1c ); //File tree structure position

		numFolders = ReadInt( &buffer, 0x20 ); //Number of folders.

		nameTablePos = ReadInt( &buffer, 0x24 ); //Offset to name table

		//Read folders:
		pos = nameTablePos;

		for( int i = 0; i < numFolders; ++i )
		{
			folders.push_back( ReadUnicode( &buffer, pos + ReadInt( &buffer, pos ), false ) );

			pos += 0x4;
		}

		//Read files:
		pos = 0x28;
		for( int i = 0; i < numFiles; ++i )
		{
			std::wstring fileName = ReadUnicode( &buffer, pos + ReadInt( &buffer, pos ), false );
			pos += 0x4;

			int fileSize = ReadInt( &buffer, pos );
			pos += 0x4;

			std::wstring folderName = folders.at( ReadInt( &buffer, pos ) );
			pos += 0x4;

			//???
			pos += 0x4;

			//Tbh, I dont care about "File Time, so lets skip over this junk"
			//unsigned char data[8];

			//Read4BytesReversed( seg, buffer, position );
			//data[0] = seg[0];
			//data[1] = seg[1];
			//data[2] = seg[2];
			//data[3] = seg[3];

			pos += 0x4;
			//Read4BytesReversed( seg, buffer, position );

			//data[4] = seg[0];
			//data[5] = seg[1];
			//data[6] = seg[2];
			//data[7] = seg[3];

			//FILETIME ft;
			//memcpy( &ft, data, sizeof( FILETIME ) );

			//SYSTEMTIME st;

			//FileTimeToSystemTime( &ft, &st );

			//std::wstring fileTimeString;

			//fileTimeString += ToString( st.wHour ) + L":" + ToString( st.wMinute ) + L" ";
			//fileTimeString += ToString( st.wDay ) + L"/";
			//fileTimeString += ToString( st.wMonth ) + L"/";
			//fileTimeString += ToString( st.wYear );

			//std::wcout << L"--FILE TIME: " + fileTimeString + L"\n";
			pos += 0x4;

			int fileStart = ReadInt( &buffer, pos );
			pos += 0x4;

			//Unknown block:
			//std::wcout << L"--UNKNOWN BLOCK: ";
			//Read4BytesReversed( seg, buffer, position );
			//for( int j = 0; j < 4; ++j )
			//{
			//	std::wcout << L"0x";
			//	std::wcout << std::hex << seg[j];
			//	std::wcout << L" ";
			//}
			//std::wcout << L"\n";

			pos += 0x4;

			RABFile file;
			file.name = fileName;
			file.filesize = fileSize;
			file.folder = folderName;
			file.position = fileStart;

			files.push_back( file );
		}
	}

	//Clear buffers
	buffer.clear( );
	file.close( );
}

//Converts a string to lower case
std::wstring ConvertToLower( std::wstring in )
{
	std::wstring out;
	for( int i = 0; i < in.size( ); ++i )
	{
		out += towlower( in[i] );
	}
	return out;
}

std::vector< char > RABReader::ReadFile( std::wstring folder, std::wstring file )
{
	std::vector< char > uncompressedBytes;

	for( int i = 0; i < files.size(); ++i )
	{
		if( files[i].folder == folder && ConvertToLower(files[i].name) == ConvertToLower(file) )
		{
			std::vector< char > compressedBytes;
			for( int j = 0; j < files[i].filesize; ++j )
			{
				compressedBytes.push_back(data[files[i].position + j]);
			}
			uncompressedBytes = CMPLDecompress( compressedBytes );
			compressedBytes.clear();

			return uncompressedBytes;
		}
	}

	return uncompressedBytes;
}