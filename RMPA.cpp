//###################################################
//RMPA (EDF Waypoint Data) reader
//###################################################

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "Util.h"
#include "RMPA.h"

CRMPAReader::CRMPAReader( const char *path )
{
	//Create input stream from path
	std::ifstream file( path, std::ios::binary | std::ios::ate );
	std::streamsize size = file.tellg( );
	file.seekg( 0, std::ios::beg );

	std::vector<char> buffer( size );
	if( file.read( buffer.data( ), size ) )
	{
		int pos = 0x0;

		//TODO: Read file header, determine if valid.

		//Read spawnpoints:
		pos = 0x20;
		bool bParseSpawns = ReadInt( &buffer, pos );

		pos = 0x24;
		int offsSpawnTable = ReadInt( &buffer, pos, true );

		//Read header 1.
		pos = offsSpawnTable;

		//Type header
		int subheaderCount = ReadInt( &buffer, offsSpawnTable, true ); //Subheader Count
		int subheaderOffs = ReadInt( &buffer, offsSpawnTable + 0x4, true ); //Subheader offset.
		int dataEndOffs = ReadInt( &buffer, offsSpawnTable + 0xc, true ); //Data end offset
		int typeHeaderID = ReadInt( &buffer, offsSpawnTable + 0x10, true ); //Type header identifier
		int stringTableOffs = ReadInt( &buffer, offsSpawnTable + 0x18, true ); //Offset to strings

		//Read subheaders
		pos = offsSpawnTable + subheaderOffs;
		for( int i = 0; i < subheaderCount; ++i )
		{
			int subheaderEndOffs = ReadInt( &buffer, pos + 0x8, true ); //Subheader Data End Point
			int subheaderNameLen = ReadInt( &buffer, pos + 0x10, true ); //Subheader Name Length
			int subheaderNameOffs = ReadInt( &buffer, pos + 0x14, true ); //Subheader Name offset
			int subheaderDataCount = ReadInt( &buffer, pos + 0x18, true ); //Subheader Data Count
			int subheaderDataOffs = ReadInt( &buffer, pos + 0x1c, true ); //Subheader Data Start Pos

			std::wstring subheaderName = ReadUnicode( &buffer, pos + subheaderNameOffs, true ); //Get our actual name.

			//Read spawn points.
			for( int j = 0; j < subheaderDataCount; ++j )
			{
				int ofs = pos + subheaderDataOffs + (j * 0x40);

				int spawnpointID = ReadInt( &buffer, ofs + 0x8, true );

				//Position
				float x = ReadFloat( &buffer, ofs + 0x0c );
				float y = ReadFloat( &buffer, ofs + 0x10 );
				float z = ReadFloat( &buffer, ofs + 0x14 );

				//Orientation
				float fx = ReadFloat( &buffer, ofs + 0x1c );
				float fy = ReadFloat( &buffer, ofs + 0x20 );
				float fz = ReadFloat( &buffer, ofs + 0x24 );

				int spawnpointNameSize = ReadInt( &buffer, ofs + 0x30, true );
				int spawnpointNameOffs = ReadInt( &buffer, ofs + 0x34, true );

				std::wstring spawnpointName = ReadUnicode( &buffer, ofs + spawnpointNameOffs, true ); //Get our actual name.

				//Store spawn point. This is largely temporary.
				RMPASpawnpoint point;

				point.x = x;
				point.y = y;
				point.z = z;

				point.fx = fx;
				point.fy = fy;
				point.fz = fz;

				point.name = spawnpointName;

				spawnpoints.push_back( point );
			}

			pos += 0x20;
		}
	}

	//Clear buffers
	buffer.clear( );
	file.close( );
};
