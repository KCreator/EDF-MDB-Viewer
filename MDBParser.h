#pragma once //Only comppile this header once.

//#########################################
//MDB Data Structures
//#########################################

//Texture struct
struct MDBTexture
{
    int index;
    std::wstring name;
    std::wstring filename;
    int UNK0;
};

//Vertex Layour STruct, holds data that helps read the vertices
struct MDBVertexLayoutInfo
{
    int type;
    int offset;
    int channel;
    std::string name;
};

//TODO: Store exctracted vertices using this structure?
struct MDBVertexData
{

};

//Todo: Should use this, somehow.
struct MDBIndiceData
{

};

//Mesh information structure, Holds data related to an object's mesh
struct MDBMeshInfo
{
    int skindata;
    int material;
    int UNK0;
    std::vector<MDBVertexLayoutInfo> vertexinfo;
    short vertexsize;
    short layoutcount;
    int vertexnumber;
    int meshindex;
    MDBVertexData vertexdata;
    int indicesnumber;
    MDBIndiceData indicedata;
};

//Object Structure, Holds data related to an Object in the MDB
struct MDBObject
{
    int index;
    int nameindex;
    int meshcount;
    std::vector<MDBMeshInfo> meshs;
};

//The main MDB storage structure, holds all data extracted from the file.
struct MDB
{
    int namescount;
    std::vector<std::wstring> names;

    int objectscount;
    std::vector<MDBObject> objects;    

    int texturescount;
    std::vector< MDBTexture > textures;
};

//#########################################
//MDB Reader
//#########################################

class MDBReader
{
public:
    MDBReader( const char* path );  //From file
    MDBReader( std::vector< char > bytes ); //From bytes

    //Temp:
    std::vector< glm::vec3 > test;
    std::vector< glm::vec2 > uvs;
    std::vector< unsigned int > test2;

    //The model data itself.
    MDB model;

private:
    void Parse();
    void ReadNameTable( int nametableoffs );
    void ReadMeshInfo( MDBObject &obj,int position );
    void ReadVertexLayoutInfo( MDBMeshInfo &mesh, int position );

    std::vector< char > mdbBytes;
};