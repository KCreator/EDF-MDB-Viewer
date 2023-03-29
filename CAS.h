#pragma once

struct Keyframe
{
	short x;
	short y;
	short z;
};

struct AnimPoint
{
	short type;
	short keyframeCount;

	float x;
	float y;
	float z;

	float dx;
	float dy;
	float dz;

	std::vector< Keyframe > keyframes;
};

struct AnimDataGroup
{
	short boneIndex;
	short transformAnimPointIndex;
	short rotateAnimPointIndex;
	short transparancyAnimPointIndex;
};

struct AnimData
{
	int unk0;
	std::wstring name;
	float time;
	float speed;
	int kf;
	int dataCount;
	std::vector< AnimDataGroup > dataGroup;
};

struct CANM
{
	std::vector< std::wstring > vecBoneList;

	int animPointCount;
	std::vector< AnimPoint > animPoints;

	int animDataCount;
	std::vector< AnimData > animData;
};

class CAS
{
public:
	CAS( const char* path );

	void ReadCANM( std::vector< char >* buffer, int pos );

	std::vector< std::wstring > vecBoneList;

	CANM canm;
};

