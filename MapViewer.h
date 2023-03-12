#pragma once

//###################################################
// MAC Data storage structs
//###################################################

struct MAPBObj
{
	std::wstring modelname;
	glm::vec3 positions;
	glm::vec3 rotation;
};

struct MAPO
{

};

//###################################################
//MAC (EDF Map File) reader
//###################################################
class MACReader
{
public:
	MACReader( const char *path );

	void ParseMapB( std::vector< char> buffer );

	void ParseMapO( std::vector< char> buffer );

	//Temp:
	//This needs to be redone...
	std::vector< std::wstring > modelNames;
	std::vector< std::wstring > modelNames2;
	std::vector< std::wstring > subModelNames;

	std::vector< glm::vec3 > positions;
	std::vector< glm::vec3 > submodelPositionOffsets;
	std::vector< glm::vec3 > rotations;
};
