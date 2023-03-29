//###################################################
//CAS (EDF Animation Data) reader
//###################################################

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "Util.h"
#include "CAS.h"

CAS::CAS( const char* path )
{
	//Create input stream from path
	std::ifstream file( path, std::ios::binary | std::ios::ate );
	std::streamsize size = file.tellg();
	file.seekg( 0, std::ios::beg );

	std::vector<char> buffer( size );
	if( file.read( buffer.data(), size ) )
	{
		//Read CAS Data.

		//Read version
		int version = ReadInt( &buffer, 0x4 );
		int i_CasDCCount;

		if( version == 515 ) //EDF 5 CAS
		{
			i_CasDCCount = 13;
		}

		//Read CANM offset
		int ofsCANM = ReadInt( &buffer, 0x8 );

		//Read TControl info
		int iTControlCount = ReadInt( &buffer, 0xc );
		int iTControlOfs = ReadInt( &buffer, 0x10 );

		//Read VControl info
		int iVControlCount = ReadInt( &buffer, 0x14 );
		int iVControlOfs = ReadInt( &buffer, 0x18 );

		//Read AnimGroup info
		int iAnimGroupCount = ReadInt( &buffer, 0x1c );
		int iAnimGroupOfs = ReadInt( &buffer, 0x20 );

		//Read Bone info
		int iBoneCount = ReadInt( &buffer, 0x24 );
		int iBoneOfs = ReadInt( &buffer, 0x28 );

		//Unknown
		int iunkOfs = ReadInt( &buffer, 0x2c );

		//Populate bone list:
		for( int i = 0; i < iBoneCount; ++i )
		{
			int pos = iBoneOfs + (i * 4);
			int ofs = ReadInt( &buffer, pos );

			std::wstring boneName = ReadUnicode( &buffer, pos + ofs, false );
			vecBoneList.push_back( boneName );
		}

		//Read CANM data
		ReadCANM( &buffer, ofsCANM );
	}

	//Clear buffers
	buffer.clear();
	file.close();
};

//Todo: Perhaps make this a separate function for a CANM class?
void CAS::ReadCANM( std::vector< char >* buffer, int pos )
{
	//Read AnimData
	int iAnimDataCount = ReadInt( buffer, pos + 0x8 );
	int iAnimDataOfs = ReadInt( buffer, pos + 0xc );

	//Read AnimPoint
	int iAnimPointCount = ReadInt( buffer, pos + 0x10 );
	int iAnimPointOfs = ReadInt( buffer, pos + 0x14 );

	//Read Bone Data
	int iBoneCount = ReadInt( buffer, pos + 0x18 );
	int iBoneOfs = ReadInt( buffer, pos + 0x1c );

	//Populate bone list:
	for( int i = 0; i < iBoneCount; ++i )
	{
		//Pos2 variable name needs to be reworked, looks a bit bad.

		int pos2 = iBoneOfs + (i * 4);
		int ofs = ReadInt( buffer, pos + pos2 );

		std::wstring boneName = ReadUnicode( buffer, pos + pos2 + ofs, false );

		canm.vecBoneList.push_back( boneName );

		//Todo: Store the result.
		//std::wcout << boneName + L"\n";
	}

	canm.animPointCount = iAnimPointCount;

	//Read anim points
	for( int i = 0; i < iAnimPointCount; ++i )
	{
		int curPos = pos + iAnimPointOfs + (i * 0x20);

		AnimPoint point;

		point.type = ReadShort( buffer, curPos );
		point.keyframeCount = ReadShort( buffer, curPos + 0x2 );

		point.x = ReadFloat( buffer, curPos + 0x4, true );
		point.y = ReadFloat( buffer, curPos + 0x8, true );
		point.z = ReadFloat( buffer, curPos + 0xc, true );

		point.dx = ReadFloat( buffer, curPos + 0x10, true );
		point.dy = ReadFloat( buffer, curPos + 0x14, true );
		point.dz = ReadFloat( buffer, curPos + 0x18, true );

		int kfOfs = ReadInt( buffer, curPos + 0x1c );

		for( int j = 0; j < point.keyframeCount; ++j )
		{
			int kfPos = curPos + kfOfs + (j * 0x6);

			Keyframe kf;
			kf.x = ReadShort( buffer, curPos + kfOfs );
			kf.y = ReadShort( buffer, curPos + kfOfs + 0x2 );
			kf.z = ReadShort( buffer, curPos + kfOfs + 0x4 );
			point.keyframes.push_back( kf );
		}

		canm.animPoints.push_back( point );
	}

	//Read anim data:
	for( int i = 0; i < iAnimDataCount; ++i )
	{
		int curPos = pos + iAnimDataOfs + (i * 0x1c);

		AnimData data;

		//Unknown int 1
		int unk1 = ReadInt( buffer, curPos );

		int ofs = ReadInt( buffer, curPos + 0x4 );
		data.name = ReadUnicode( buffer, curPos + ofs, false );

		data.time = ReadFloat( buffer, curPos + 0x8, true );
		data.speed = ReadFloat( buffer, curPos + 0xc, true );
		data.kf = ReadInt( buffer, curPos + 0x10 );
		data.dataCount = ReadInt( buffer, curPos + 0x14 );
		int dataOfs = ReadInt( buffer, curPos + 0x18 );

		//Read data
		for( int dataIter = 0; dataIter < data.dataCount; ++dataIter )
		{
			int datapos = curPos + dataOfs + (dataIter * 0x8);

			AnimDataGroup group;

			group.boneIndex = ReadShort( buffer, datapos );
			group.transformAnimPointIndex = ReadShort( buffer, datapos + 0x2 );
			group.rotateAnimPointIndex = ReadShort( buffer, datapos + 0x4 );
			group.transparancyAnimPointIndex = ReadShort( buffer, datapos + 0x6 );

			data.dataGroup.push_back( group );
		}

		canm.animData.push_back( data );
	}
}

