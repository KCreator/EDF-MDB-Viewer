#pragma once

//###################################################
//RMPA (EDF Waypoint Data) reader
//###################################################

//Supporting data structures.
struct RMPASpawnpoint
{
	float x;
	float y;
	float z;

	float fx;
	float fy;
	float fz;

	std::wstring name;
};

class CRMPAReader
{
public:
	CRMPAReader( const char *path );

	//Temp:
	std::vector< RMPASpawnpoint > spawnpoints;
};
