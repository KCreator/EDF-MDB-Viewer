#include <vector>
#include <iostream>
#include <fstream>
#include "Util.h"

//OPENGL INCLUDE
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
//#include <GLES3/gl3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>

#include "MapViewer.h"

//###################################################
//MAC (EDF Map File) reader
//###################################################

MACReader::MACReader( const char *path )
{
	//Create input stream from path
	std::ifstream file( path, std::ios::binary | std::ios::ate );
	std::streamsize size = file.tellg( );
	file.seekg( 0, std::ios::beg );

	std::vector<char> buffer( size );
	if( file.read( buffer.data( ), size ) )
	{
		//Read MAC file.
		int dataStartOfs = ReadInt( &buffer, 0x8 );
		int unk0 = ReadInt( &buffer, 0xc );
		int unk1 = ReadInt( &buffer, 0x10 );

		//MARC table offset and size
		int iMARCTableOffs = ReadInt( &buffer, 0x14 );
		int iMARCTableSize = ReadInt( &buffer, 0x18 );

		//String Table offset and size
		int iStringTableOffs = ReadInt( &buffer, 0x1c );
		int iStringTableSize = ReadInt( &buffer, 0x20 );

		//Read MARC:
		int pos = 0x14 + iMARCTableOffs;
		for( int i = 0; i < iMARCTableSize; ++i )
		{
			//Get the MARC internal name:
			int ofs = ReadInt( &buffer, pos );
			std::wstring name = ReadUnicode( &buffer, pos + ofs, false ); //String is located relative to read position

			//std::wcout << name + L"\n"; //Debugging.

			pos += 0x4;

			//Block Address (Absolute)
			int blockAddress = ReadInt( &buffer, pos );
			pos += 0x4;

			//Block Size:
			int blockSize = ReadInt( &buffer, pos );
			pos += 0x4;

			//Several unknowns.
			int iMARCUnk0 = ReadInt( &buffer, pos );
			pos += 0x4;

			int iMARCUnk1 = ReadInt( &buffer, pos );
			pos += 0x4;

			int iMARCUnk2 = ReadInt( &buffer, pos );
			pos += 0x4;

			//Attempt to do something with this data
			//TODO: Cleanup, organise, ect.
			int oldPos = pos;
			if( name == L"map.mapo" )
			{
				std::vector<char> mapoBuff( blockSize );
				std::copy( buffer.begin() + blockAddress, buffer.begin() + blockAddress + blockSize, mapoBuff.begin() );
				ParseMapO( mapoBuff );

				mapoBuff.clear();
			}
			if( name == L"map.mapb" )
			{
				std::vector<char> mapbBuff( blockSize );
				std::copy( buffer.begin() + blockAddress, buffer.begin() + blockAddress + blockSize, mapbBuff.begin() );
				ParseMapB( mapbBuff );

				mapbBuff.clear();
			}
			pos = oldPos;
		}
	}

	//Clear buffers
	buffer.clear( );
	file.close( );
}

void MACReader::ParseMapB( std::vector< char> buffer )
{
	int pos = 0;

	int iStringOffs = ReadInt( &buffer, pos + 0x8 );
	int iStringCount = ReadInt( &buffer, pos + 0xc );

	int iMAPBTable1Offs = ReadInt( &buffer, pos + 0x18 );
	int iMAPBTable1Count = ReadInt( &buffer, pos + 0x1c );

	int iMAPBTable2Offs = ReadInt( &buffer, pos + 0x20 );
	int iMAPBTable2Count = ReadInt( &buffer, pos + 0x24 );

	/*
	pos = iMAPBTable1Offs;
	for( int i = 0; i < iMAPBTable1Count; ++i )
	{
		int unk0 = ReadInt( &buffer, pos );
		pos += 0x4;

		int sgoOffs = ReadInt( &buffer, pos );
		pos += 0x4;

		int unk1 = ReadInt( &buffer, pos );
		pos += 0x4;

		//Read SGO?

		int strOffs = ReadInt( &buffer, pos );
		std::wstring name = ReadUnicode( &buffer, pos + strOffs, false );
		std::wcout << name + L"\n"; //Debugging.

		pos += 0x4;

		pos += 0x38 - ( 0x4 * 4 );
	}
	*/

	pos = iMAPBTable2Offs;

	for( int i = 0; i < iMAPBTable2Count; ++i )
	{
		//Offset to Table
		int offs = ReadInt( &buffer, pos );

		//Attempt to use this table offset.
		int table1Pos = pos + offs;

		int unk0 = ReadInt( &buffer, table1Pos );

		int sgoOffs = ReadInt( &buffer, table1Pos + 0x4 );
		int strOffs = ReadInt( &buffer, table1Pos + 0xc );
		std::wstring name = ReadUnicode( &buffer, table1Pos + 0xc + strOffs, false );

		size_t dotPos = name.find_last_of( L"." );
		std::wstring test = name.substr( 0, dotPos );
		modelNames2.push_back( test );
		//std::wcout << name + L"\n"; //Debugging.

		//Try to read some key data from the SGO (TODO: Write SGO handler)
		int sgoPos = table1Pos + 0x4 + sgoOffs;
		
		/*
		//This confirms we have an SGO file here.
		char bytes[4];
		bytes[0] = buffer[sgoPos];
		bytes[1] = buffer[sgoPos+1];
		bytes[2] = buffer[sgoPos+2];
		bytes[3] = buffer[sgoPos+3];

		int numSGOVars = ReadInt( &buffer, sgoPos + 0x8 );
		int SGOVarsStart = ReadInt( &buffer, sgoPos + 0xc );
		int numVarNames = ReadInt( &buffer, sgoPos + 0x10 );
		int SGOVarNamesStart = ReadInt( &buffer, sgoPos + 0x14 );

		//Read Variable Names
		std::vector< std::wstring > variableNames;
		for( int j = 0; j < numVarNames; ++j )
		{
			int stroffs = ReadInt( &buffer, sgoPos + SGOVarNamesStart + ( j * 0x8 ) );
			int id = ReadInt( &buffer, sgoPos + SGOVarNamesStart + ( j * 0x8 ) + 0x4  );
			
			std::wstring vName = ReadUnicode( &buffer, sgoPos + SGOVarNamesStart + ( j * 8 ) + stroffs, false );
			variableNames.push_back( vName );
		}

		for( int j = 0; j < numSGOVars; ++j )
		{
			if( variableNames[j] == L"filename" )
			{
				int locOfs = sgoPos + SGOVarsStart + ( j * 0xc ); //position + varStart + j * Size of SGO Node
				int type = ReadInt( &buffer, locOfs );
				if( type == 3 )
				{
					int strSize = ReadInt( &buffer, locOfs + 0x4 );
					std::wcout << L"Variable '" + variableNames[j] + L"' size =  '" + std::to_wstring( strSize ) + L"'\n";

					int stroffs = ReadInt( &buffer, locOfs + 0x8 );
					std::wstring value = ReadUnicode( &buffer, locOfs + stroffs, false );
					std::wcout << L"Variable '" + variableNames[j] + L"' = '" + value + L"'\n";
				}
			}
		}
		*/

		pos += 0x4;

		//X Pos
		float xPos = ReadFloat( &buffer, pos, true );
		pos += 0x4;
		//Y Pos
		float yPos = ReadFloat( &buffer, pos, true );
		pos += 0x4;
		//Z Pos
		float zPos = ReadFloat( &buffer, pos, true );
		pos += 0x4;

		//RotationX
		float xRot = ReadFloat( &buffer, pos, true );
		pos += 0x4;
		//RotationY
		float yRot = ReadFloat( &buffer, pos, true );
		pos += 0x4;
		//RotationZ
		float zRot = ReadFloat( &buffer, pos, true );
		pos += 0x4;

		//Store this for later?
		positions.push_back( glm::vec3( xPos, yPos, zPos ) );
		rotations.push_back( glm::vec3( xRot, yRot, zRot ) );

		//Ignore unknowns.
		pos += 0x4 * 10;
	}
}

void MACReader::ParseMapO( std::vector< char> buffer )
{
	//Can't seem to figure this out right now. Fukou da.
	int pos = 0;
				
	int objectTable1Offs = ReadInt( &buffer, pos + 0x8 );
	int objectTable1Count = ReadInt( &buffer, pos + 0xc );

	int objectStringTableOffs = ReadInt( &buffer, pos + 0x48 );
	int objectStringTableCount = ReadInt( &buffer, pos + 0x4C );

	int objectTable3Offs = ReadInt( &buffer, pos + 0x20 );
	int objectTable3Count = ReadInt( &buffer, pos + 0x24 );

	int objectTable5Offs = ReadInt( &buffer, pos + 0x38 );
	int objectTable5Size = ReadInt( &buffer, pos + 0x3c );

	//return;

	pos = objectTable1Offs;
	for( int j = 0; j < objectTable1Count; ++j )
	{
		int base = pos;

		//Read:
		//std::cout << "Var: ";
		//std::cout << std::to_string( ReadInt( &buffer, pos + 0x00 ) ) + ",";
		//std::cout << std::to_string( ReadInt( &buffer, pos + 0x04 ) ) + ","; //Apparently offset to 4th data table?
		//std::cout << std::to_string( ReadInt( &buffer, pos + 0x08 ) ) + ",";
		//std::cout << std::to_string( ReadInt( &buffer, pos + 0x0c ) ) + ",";
		//std::cout << std::to_string( ReadInt( &buffer, pos + 0x10 ) ) + ",";
		//std::cout << std::to_string( ReadInt( &buffer, pos + 0x14 ) ) + ",";
		//std::cout << std::to_string( ReadInt( &buffer, pos + 0x18 ) ) + "\n";

		int unk0 = ReadInt( &buffer, pos );
		pos += 0x4;

		int offset1 = ReadInt( &buffer, pos );

		if( offset1 != 0 )
		{
			//std::cout << "4th table value: " + std::to_string( ReadInt( &buffer, pos + offset1 ) ) + "\n";
			int test = offset1 + ReadInt( &buffer, pos + offset1 );

			int positionsOffs = ReadInt( &buffer, pos + test + 0x10 );
			if( positionsOffs != 0 )
			{
				int localPos = pos + test + positionsOffs;
				float x = ReadFloat( &buffer, localPos, true );
				float y = ReadFloat( &buffer, localPos + 0x4, true );
				float z = ReadFloat( &buffer, localPos + 0x8, true );

				submodelPositionOffsets.push_back( glm::vec3( x, y, z ) );
			}
			else
			{
				submodelPositionOffsets.push_back( glm::vec3( 0, 0, 0 ) );
			}

			int namOfs = ReadInt( &buffer, pos + test + 0x20 );
			std::wstring tname = ReadUnicode( &buffer, pos + test + namOfs, false );
			//std::wcout << tname + L"\n"; //Debugging.

			modelNames.push_back( tname ); //Store for later...
		}
		else
		{
			modelNames.push_back( L"" ); //Store for later...
		}

		//pos += 0x1c;
		//continue;

		pos += 0x4;

		int unk1 = ReadInt( &buffer, pos );
		pos += 0x4;

		float unk2 = ReadFloat( &buffer, pos );
		pos += 0x4;

		//Sub model name?
		int mdbNameOfs = ReadInt( &buffer, pos );
		std::wstring name = ReadUnicode( &buffer, base + mdbNameOfs, false );
		//std::wcout << name + L"\n"; //Debugging.
		subModelNames.push_back( name );

		pos += 0x4;

		int hktNameOfs = ReadInt( &buffer, pos );
		name = ReadUnicode( &buffer, base + hktNameOfs, false );
		//std::wcout << name + L"\n"; //Debugging.

		pos += 0x4;
		pos += 0x4;
	}

	return; //TEMP!

	//Test:
	pos = objectTable3Offs;
	for( int i = 0; i < objectTable3Count; ++i )
	{
		int base = pos;

		//Get object .hkt name
		int hktSringOffs = ReadInt( &buffer, pos + 0x04 );
		std::wstring hktString = ReadUnicode( &buffer, base + hktSringOffs, false );

		//Seems there is only 1 value at 0xC that matters. Everything else is zero... Very different compared to the 4.1 notes...

		//Get object hk.mdb name
		int hkmdbSringOffs = ReadInt( &buffer, pos + 0x14 );
		std::wstring hkmdbString = ReadUnicode( &buffer, base + hkmdbSringOffs, false );

		//This seems to get object names now.
		int test = ReadInt( &buffer, pos + 0x20 );
		std::wstring name = ReadUnicode( &buffer, base + test, false );
		std::wcout << name + L"\n"; //Debugging.

		modelNames.push_back( name ); //Store for later...

		//Apparently the 5 map format has an extra byte?
		pos += 0x38 + 0x4;
	}
}

