#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string.h>
#include <codecvt>
#include <locale>

//OPENGL INCLUDE
#include <SFML/OpenGL.hpp>
#include <GLES3/gl3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>

#include "MDBParser.h"
#include "Util.h"

//#########################################
//MDB Reader
//#########################################

//Constructor, reading from a path
MDBReader::MDBReader( const char* path )
{
    //Create input stream from path
    std::ifstream file( path, std::ios::binary | std::ios::ate );
    std::streamsize size = file.tellg( );
	file.seekg( 0, std::ios::beg );

	std::vector<char> buffer( size );
	if( file.read( buffer.data( ), size ) )
	{
        //Put the buffer into our stored bytes
        mdbBytes = buffer;

        //Parse the bytes.
		Parse( );
	}

	//Clear buffers
	buffer.clear( );
	file.close( );
}

//Constructor, reading from a byte vector
MDBReader::MDBReader( std::vector< char > bytes )
{
    //Put the buffer into our stored bytes
    mdbBytes = bytes;

    //Parse the bytes.
    Parse( );
}

//Parse the MDB bytes
void MDBReader::Parse( )
{
    //Todo: Error checking, full file parsing, ect.
    int pos = 0x8;

    //Read header:
    model.namescount = ReadInt( &mdbBytes, pos );
    int iNameTableOffs = ReadInt( &mdbBytes, pos + 4 );

    model.bonescount = ReadInt( &mdbBytes, 0x10 );
    int iBoneOffs = ReadInt( &mdbBytes, 0x14 );

    model.objectscount = ReadInt( &mdbBytes, 0x18 );
    int iObjectTableOffs = ReadInt( &mdbBytes, 0x1C );

    model.materialscount = ReadInt( &mdbBytes, 0x20 );
    int materialOffs = ReadInt( &mdbBytes, 0x24 );

    model.texturescount = ReadInt( &mdbBytes, 0x28 );
    int textureOffs = ReadInt( &mdbBytes, 0x2C );

    //Read tables:
    ReadNameTable( iNameTableOffs );

    //Read bones
    pos = iBoneOffs;
    for( int i = 0; i < model.bonescount ; ++i )
    {
        MDBBone bone;

        //Bone Index
        bone.boneIndex = ReadInt( &mdbBytes, pos ); //0x0
        pos += 0x4;

        //std::wcout << L"Bone? " + model.names[ bone.boneIndex ] + L"\n";

        //Bone Parent
        bone.boneParent = ReadInt( &mdbBytes, pos ); //0x4
        pos += 0x4;

        //Currently unknown x3
        bone.unk0 = ReadInt( &mdbBytes, pos ); //0x8
        pos += 0x4;

        bone.unk1 = ReadInt( &mdbBytes, pos ); //0xc
        pos += 0x4;

        int boneNameIndex = ReadInt( &mdbBytes, pos ); //0x10
        bone.name = model.names[ boneNameIndex ];
        pos += 0x4;

        //0x14
        pos += 12;

        //0x20

        //Bone transformations
        float px = ReadFloat( &mdbBytes, pos, true );
        float py = ReadFloat( &mdbBytes, pos + 0x04, true );
        float pz = ReadFloat( &mdbBytes, pos + 0x08, true );
        float pw = ReadFloat( &mdbBytes, pos + 0x0c, true );

        pos += 0x10; //0x30

        float rx = ReadFloat( &mdbBytes, pos, true );
        float ry = ReadFloat( &mdbBytes, pos + 0x04, true );
        float rz = ReadFloat( &mdbBytes, pos + 0x08, true );
        float rw = ReadFloat( &mdbBytes, pos + 0x0c, true );

        pos += 0x10; //0x40

        float sx = ReadFloat( &mdbBytes, pos, true );
        float sy = ReadFloat( &mdbBytes, pos + 0x04, true );
        float sz = ReadFloat( &mdbBytes, pos + 0x08, true );
        float sw = ReadFloat( &mdbBytes, pos + 0x0c, true );

        pos += 0x10; //0x50

        float wx = ReadFloat( &mdbBytes, pos, true );
        float wy = ReadFloat( &mdbBytes, pos + 0x04, true );
        float wz = ReadFloat( &mdbBytes, pos + 0x08, true );
        float ww = ReadFloat( &mdbBytes, pos + 0x0c, true );

        //construct matrix?
        bone.matrix = { px, py, pz, pw, rx, ry, rz, rw, sx, sy, sz, sw, wx, wy, wz, ww };

        pos += 0x10; //0x60

        //Bone transformations 2
        px = ReadFloat( &mdbBytes, pos, true );
        py = ReadFloat( &mdbBytes, pos + 0x04, true );
        pz = ReadFloat( &mdbBytes, pos + 0x08, true );
        pw = ReadFloat( &mdbBytes, pos + 0x0c, true );

        pos += 0x10; //0x30

        rx = ReadFloat( &mdbBytes, pos, true );
        ry = ReadFloat( &mdbBytes, pos + 0x04, true );
        rz = ReadFloat( &mdbBytes, pos + 0x08, true );
        rw = ReadFloat( &mdbBytes, pos + 0x0c, true );

        pos += 0x10; //0x40

        sx = ReadFloat( &mdbBytes, pos, true );
        sy = ReadFloat( &mdbBytes, pos + 0x04, true );
        sz = ReadFloat( &mdbBytes, pos + 0x08, true );
        sw = ReadFloat( &mdbBytes, pos + 0x0c, true );

        pos += 0x10; //0x50

        wx = ReadFloat( &mdbBytes, pos, true );
        wy = ReadFloat( &mdbBytes, pos + 0x04, true );
        wz = ReadFloat( &mdbBytes, pos + 0x08, true );
        ww = ReadFloat( &mdbBytes, pos + 0x0c, true );

        //construct matrix?
        bone.matrix2 = { px, py, pz, pw, rx, ry, rz, rw, sx, sy, sz, sw, wx, wy, wz, ww };

        pos += 0x10; //0x60

        //TEMP
        //Coords?
        float x = ReadFloat( &mdbBytes, pos, true );
        float y = ReadFloat( &mdbBytes, pos + 4, true );
        float z = ReadFloat( &mdbBytes, pos + 8, true );
        float w = ReadFloat( &mdbBytes, pos + 12, true );
        //bonepos.push_back( glm::vec3( x, y ,z ) );
        bone.x = x;
        bone.y = y;
        bone.z = z;
        bone.w = w;

        //UNK
        pos += 0x10;

        x = ReadFloat( &mdbBytes, pos, true );
        y = ReadFloat( &mdbBytes, pos + 4, true );
        z = ReadFloat( &mdbBytes, pos + 8, true );
        w = ReadFloat( &mdbBytes, pos + 12, true );

        bone.test1 = glm::vec4( x, y, z, w );

        pos += 0x10;

        model.bones.push_back( bone );
    }

    //Read Object Tables
    pos = iObjectTableOffs;
    for( int i = 0; i < model.objectscount ; ++i )
    {
        int base = pos;

        MDBObject obj;

        //Object Index
        obj.index = ReadInt( &mdbBytes, pos );
        pos += 4;

        //Object Name Index
        obj.nameindex = ReadInt( &mdbBytes, pos );
        pos += 4;

        //Object Mesh Count
        obj.meshcount = ReadInt( &mdbBytes, pos );
        pos += 4;

        //Object Mesh Offset
        int meshOfs = ReadInt( &mdbBytes, pos );
        pos += 4;

        //Read Mesh Info:
        int meshpos = base + meshOfs;

        ReadMeshInfo( obj, meshpos );

        //std::wcout << L"OBJECT: NUMBER = " + std::to_wstring( obj.index ) + L" NAME = " + model.names.names[obj.nameindex] + L" Mesh Count: " + std::to_wstring( obj.meshcount ) + L"\n";
        model.objects.push_back(obj);
    }

    //Read material offset:
    pos = materialOffs;
    for( int i = 0; i < model.materialscount; ++i )
    {
        int base = pos;

        MDBMaterial material;

        //Index:
        material.materialIndex = ReadShort( &mdbBytes, pos );
        pos += 0x2;

        //Unknowns
        material.unk0 = mdbBytes[pos];
        pos++;

        material.unk1 = mdbBytes[pos];
        pos++;

        //Material Name
        material.materialName = model.names[ ReadInt( &mdbBytes, pos ) ];
        pos += 0x4;

        //Shader Name:
        material.shader = ReadUnicode( &mdbBytes, base + ReadInt( &mdbBytes, pos ), false );
        pos += 0x4;


        //Unimplemented as of yet:
        pos += 0x4;
        pos += 0x4;

        //Material Textures:
        int materialTexturesOfs = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        material.textureCount = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Read MaterialTexture
        //Todo: Move this elsewhere.
        //Store 'pos'
        int oldPos = pos;

        pos = base + materialTexturesOfs;
        for( int j = 0; j < material.textureCount; ++j )
        {
            base = pos;
            MDBMaterialTexture matTex;

            //Texture Index
            matTex.textureIndex = ReadInt( &mdbBytes, pos );
            pos += 0x4;

            //Type
            matTex.type = ReadASCII( &mdbBytes, base + ReadInt( &mdbBytes, pos ) );
            pos += 0x4;

            //Unknown Values:
            matTex.unk0 = ReadInt( &mdbBytes, pos );
            pos += 0x4;
            matTex.unk1 = ReadInt( &mdbBytes, pos );
            pos += 0x4;
            matTex.unk2 = ReadInt( &mdbBytes, pos );
            pos += 0x4;
            matTex.unk3 = ReadInt( &mdbBytes, pos );
            pos += 0x4;
            matTex.unk4 = ReadInt( &mdbBytes, pos );
            pos += 0x4;

            material.textures.push_back( matTex );
        }

        //Restore 'pos'
        pos = oldPos;

        //3rd Unknown
        material.unk2 = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Add to "material" list
        model.materials.push_back( material );
    }

    //Read texture offset:
    pos = textureOffs;
    for( int i = 0; i < model.texturescount; ++i )
    {
        int base = pos;

        MDBTexture texture;

        //Texture Index:
        texture.index = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Texture Name;
        texture.name = ReadUnicode( &mdbBytes, base + ReadInt( &mdbBytes, pos ), false );
        pos += 0x4;

        //Texture Filename        
        texture.filename = ReadUnicode( &mdbBytes, base + ReadInt( &mdbBytes, pos ), false );
        pos += 0x4;

        //Unknown (Always Zero )
        texture.UNK0 = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        model.textures.push_back( texture );
    }

    //Parse complete, We no longer need to store our own bytes. Clear the buffer.
    mdbBytes.clear();
}

//Read the name table and store it to our MDB struct.
void MDBReader::ReadNameTable( int nametableoffs )
{
    //Read Name Table.
    int pos = nametableoffs;
    for( int i = 0; i < model.namescount ; ++i )
    {
        //Get the offset to the string
        int ofs = ReadInt( &mdbBytes, pos );
        if( ofs != 0 ) //We need to check this, to fill the name table with "Empty" should be be empty.
        {
            model.names.push_back( ReadUnicode( &mdbBytes, pos + ofs, false ) ); //Add to names array
        }
        else
            model.names.push_back( L"empty" );

        pos += 0x4;
    }
}

//Read mesh info, store mesh info to an MDBObject struct.
void MDBReader::ReadMeshInfo( MDBObject &obj, int position )
{
    int pos = position;

    for( int i = 0; i < obj.meshcount; ++i )
    {
        int base = pos;

        MDBMeshInfo mesh;

        //Skin data TODO: Parse this properly.
        mesh.skindata = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Material
        mesh.material = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Unknown (Always zero?)
        mesh.UNK0 = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Offset to Vertex Layout Info
        int vertexLayoutInfoOffs = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Size of vertices:
        mesh.vertexsize = ReadShort( &mdbBytes, pos );
        pos += 0x2;

        //Layout Count:
        mesh.layoutcount = ReadShort( &mdbBytes, pos );
        pos += 0x2;

        //Read the vertex layout information.
        ReadVertexLayoutInfo( mesh, base + vertexLayoutInfoOffs );

        //Number of vertices
        mesh.vertexnumber = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Mesh Index
        mesh.meshindex = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Offset to Vertex Data
        int vertexDataOffs = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Number of Indices
        mesh.indicesnumber = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Offset to Index Data
        int IndexDataInfoOffs = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Parse index data:
        int indPos = base + IndexDataInfoOffs;
        for( int v = 0; v < mesh.indicesnumber; ++v )
        {
            unsigned short data = ReadUShort( &mdbBytes, indPos );
            mesh.indicedata.push_back( data );
            //test2.push_back( data );
            indPos += 0x2;
        }

        //Parse vertex data.
        //run through "Vertex Size" using Vertex Layout
        int vertPos = base + vertexDataOffs;
        int oldVertPos = vertPos;
        for( int v = 0; v < mesh.vertexnumber; ++v )
        {
            for( int vi = 0; vi < mesh.layoutcount; ++vi )
            {
                MDBVertexLayoutInfo info = mesh.vertexinfo[vi];

                if( info.type == 1 ) //Float4 (16 bytes)
                {
                    vertPos += 16;
                }
                else if( info.type == 4 ) //Float3 (12 bytes)
                {
                    float x = ReadFloat( &mdbBytes, vertPos, true );
                    vertPos += 0x4;

                    float y = ReadFloat( &mdbBytes, vertPos, true );
                    vertPos += 0x4;

                    float z = ReadFloat( &mdbBytes, vertPos, true );
                    vertPos += 0x4;

                    //Store to our vertexData map.
                    std::string vertexName = mesh.vertexinfo[ vi ].name + std::to_string( mesh.vertexinfo[ vi ].channel );
                    mesh.vertexdata[ vertexName ].push_back( x );
                    mesh.vertexdata[ vertexName ].push_back( y );
                    mesh.vertexdata[ vertexName ].push_back( z );
                }
                else if( info.type == 7 ) //array of 4 half floats (8 bytes)
                {
                    short bytes = ReadShort( &mdbBytes, vertPos );
                    vertPos += 0x2;

                    float x = UnpackHalf( bytes );

                    bytes = ReadShort( &mdbBytes, vertPos );
                    vertPos += 0x2;

                    float y = UnpackHalf( bytes );

                    bytes = ReadShort( &mdbBytes, vertPos );
                    vertPos += 0x2;

                    float z = UnpackHalf( bytes );

                    bytes = ReadShort( &mdbBytes, vertPos );
                    vertPos += 0x2;

                    float w = UnpackHalf( bytes );

                    //Store to our vertexData map.
                    std::string vertexName = mesh.vertexinfo[ vi ].name + std::to_string( mesh.vertexinfo[ vi ].channel );
                    mesh.vertexdata[ vertexName ].push_back( x );
                    mesh.vertexdata[ vertexName ].push_back( y );
                    mesh.vertexdata[ vertexName ].push_back( z );
                    mesh.vertexdata[ vertexName ].push_back( w );
                }
                else if( info.type == 12 ) //Float2, 2 float values. (8 bytes)
                {
                    float x = ReadFloat( &mdbBytes, vertPos, true );
                    vertPos += 0x4;

                    float y = ReadFloat( &mdbBytes, vertPos, true );
                    vertPos += 0x4;

                    //Store to our vertexData map.
                    std::string vertexName = mesh.vertexinfo[ vi ].name + std::to_string( mesh.vertexinfo[ vi ].channel );
                    mesh.vertexdata[ vertexName ].push_back( x );
                    mesh.vertexdata[ vertexName ].push_back( y );

                    //Todo: Store this properly, this is a temporary solution.
                    if( mesh.vertexinfo[ vi ].name == "texcoord" )
                        uvs.push_back( glm::vec2( x, y ) );
                }
                if( info.type == 21 ) //ubyte4 (4 bytes)
                {
                    vertPos += 4;
                }
            }

            //Failsafe
            int difference = vertPos - oldVertPos;
            if( difference != mesh.vertexsize )
                vertPos += mesh.vertexsize - difference;

            oldVertPos = vertPos;
        }

        obj.meshs.push_back( mesh );
    }
}

//Read Vertex Layout info, store layout info info to a MeshInfo struct.
void MDBReader::ReadVertexLayoutInfo( MDBMeshInfo &mesh, int position )
{
    int pos = position;
    for( int i = 0; i < mesh.layoutcount; ++i )
    {
        int base = pos;

        MDBVertexLayoutInfo info;

        //Vertex Type
        info.type = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Offset in vertex data
        info.offset = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Channel
        info.channel = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Name
        int nameOffs = ReadInt( &mdbBytes, pos );
        pos += 0x4;

        //Read Name:
        info.name = ReadASCII( &mdbBytes, base + nameOffs );

        mesh.vertexinfo.push_back( info );
    }
}

//Accessors.
std::vector< glm::vec3 > MDBReader::GetMeshPositionVertices( int objNum, int meshNum )
{
    std::vector< glm::vec3 > returnValue;

    //Check if in bounds:
    if( objNum > model.objectscount )
        return returnValue;

    if( meshNum > model.objects[objNum].meshcount )
        return returnValue;

    //Access relevant vector and translate to vertex vec3
    MDBMeshInfo *mesh = &model.objects[objNum].meshs[meshNum];

    //Find the vertex layout type of 'position'
    int id = 0;
    for( int i = 0; i < mesh->layoutcount; ++i )
    {
        if( mesh->vertexinfo[i].name == "position" )
        {
            id = i;
        }
    }

    int ofs = 4;
    if( mesh->vertexinfo[id].type == 4 )
        ofs = 3;

    for( int i = 0; i < mesh->vertexdata[ "position0" ].size(); i += ofs ) //This should be fine in most cases. However, I should test for "type"
    {
        float x = mesh->vertexdata[ "position0" ][i];
        float y = mesh->vertexdata[ "position0" ][i+1];
        float z = mesh->vertexdata[ "position0" ][i+2];

        returnValue.push_back( glm::vec3( x, y, z ) );
    }

    return returnValue;
}

std::vector< glm::vec2 > MDBReader::GetMeshUVs( int objNum, int meshNum )
{
    std::vector< glm::vec2 > returnValue;

    //Check if in bounds:
    if( objNum > model.objectscount )
        return returnValue;

    if( meshNum > model.objects[objNum].meshcount )
        return returnValue;

    //Access relevant vector and translate to vertex vec3
    MDBMeshInfo *mesh = &model.objects[objNum].meshs[meshNum];

    for( int i = 0; i < mesh->vertexdata[ "texcoord0" ].size(); i += 2 ) //This should be fine in most cases. However, I should test for "type"
    {
        float x = mesh->vertexdata[ "texcoord0" ][i];
        float y = mesh->vertexdata[ "texcoord0" ][i+1];

        returnValue.push_back( glm::vec2( x, y ) );
    }

    return returnValue;
}

//A nicer way to get mesh indices.
std::vector< unsigned int > MDBReader::GetMeshIndices( int objNum, int meshNum )
{
    std::vector< unsigned int > returnValue;

    //Check if in bounds:
    if( objNum > model.objectscount )
        return returnValue;

    if( meshNum > model.objects[objNum].meshcount )
        return returnValue;

    returnValue = model.objects[objNum].meshs[meshNum].indicedata;

    return returnValue;
}

std::wstring MDBReader::GetColourTextureFilename( int objNum, int meshNum )
{
    std::wstring returnValue;

    //Check if in bounds:
    if( objNum > model.objectscount )
        return returnValue;

    if( meshNum > model.objects[objNum].meshcount )
        return returnValue;

    //Look up Material:
    int materialID = model.objects[ objNum ].meshs[ meshNum ].material;

    //Safty check.
    if( materialID > model.materialscount )
        return returnValue;

    for( int i = 0; i < model.materials[ materialID ].textureCount; ++i )
    {
        if( model.materials[ materialID ].textures[i].type == "albedo" || model.materials[ materialID ].textures[i].type == "window_frame_texture" )
        {
            int textureID = model.materials[ materialID ].textures[i].textureIndex;
            return model.textures[ textureID ].filename;
        }
    }

    return returnValue;
}