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
    std::map< std::string, std::vector< float > > vertexdata;
    //MDBVertexData vertexdata;
    int indicesnumber;
    std::vector< unsigned int > indicedata; //Technically supposed to be a short.
};

//Object Structure, Holds data related to an Object in the MDB
struct MDBObject
{
    int index;
    int nameindex;
    int meshcount;
    std::vector<MDBMeshInfo> meshs;
};

struct MDBBone
{
    int boneIndex;
    int boneParent;
    int unk0;
    int unk1;
    int unk2;
    //Transformation matrix.
};

//The main MDB storage structure, holds all data extracted from the file.
struct MDB
{
    int namescount;
    std::vector<std::wstring> names;

    int bonescount;
    std::vector<MDBBone> bones;

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
    //Constructor
    MDBReader( const char* path );  //From file
    MDBReader( std::vector< char > bytes ); //From bytes

    //Accessors
    //TODO: This probably should just be a generic type, as the MDB reader component should be self contained.
    std::vector< glm::vec3 > GetMeshPositionVertices( int objNum, int meshNum );

    std::vector< unsigned int > GetMeshIndices( int objNum, int meshNum );

    //Temp:
    std::vector< glm::vec2 > uvs; //This needs to go ASAP
    //std::vector< glm::vec3 > bonepos;

    //The model data itself.
    MDB model;

private:
    void Parse();
    void ReadNameTable( int nametableoffs );
    void ReadMeshInfo( MDBObject &obj,int position );
    void ReadVertexLayoutInfo( MDBMeshInfo &mesh, int position );

    std::vector< char > mdbBytes;
};