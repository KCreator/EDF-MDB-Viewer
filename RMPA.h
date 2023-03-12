#pragma once

//###################################################
//RMPA (EDF Waypoint Data) reader
//###################################################

//Supporting data structures.
struct RMPASpawnpoint
{
	//Position
	float x;
	float y;
	float z;

	//Facing
	float fx;
	float fy;
	float fz;

	std::wstring name;
};

struct RMPAShape
{

};

class CRMPAReader
{
public:
	CRMPAReader( const char *path );

	//void ParseSpawnpoints( std::vector< char > &buffer );

	//Temp:
	std::vector< RMPASpawnpoint > spawnpoints;
};
