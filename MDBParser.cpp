#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
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
    model.namescount = ReadInt( mdbBytes, pos );
    int iNameTableOffs = ReadInt( mdbBytes, pos + 4 );

    model.objectscount = ReadInt( mdbBytes, 0x18 );
    int iObjectTableOff = ReadInt( mdbBytes, 0x1C );

    //Read tables:
    ReadNameTable( iNameTableOffs );

    //Read Object Tables
    pos = iObjectTableOff;
    for( int i = 0; i < model.objectscount ; ++i )
    {
        MDBObject obj;

        //Object Index
        obj.index = ReadInt( mdbBytes, pos );
        pos += 4;

        //Object Name Index
        obj.nameindex = ReadInt( mdbBytes, pos );
        pos += 4;

        //Object Mesh Count
        obj.meshcount = ReadInt( mdbBytes, pos );
        pos += 4;

        //Object Mesh Offset
        int meshOfs = ReadInt( mdbBytes, pos );
        pos += 4;

        //Read Mesh Info:
        int meshpos = iObjectTableOff + meshOfs;

        ReadMeshInfo( obj, meshpos );

        //std::wcout << L"OBJECT: NUMBER = " + std::to_wstring( obj.index ) + L" NAME = " + model.names.names[obj.nameindex] + L" Mesh Count: " + std::to_wstring( obj.meshcount ) + L"\n";
        model.objects.push_back(obj);
    }

    model.texturescount = ReadInt( mdbBytes, 0x28 );
    int textureOffs= ReadInt( mdbBytes, 0x2C );

    //Read texture offset:
    pos = textureOffs;
    for( int i = 0; i < model.texturescount; ++i )
    {
        int base = pos;

        MDBTexture texture;

        //Texture Index:
        texture.index = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Texture Name;
        texture.name = ReadUnicode( mdbBytes, base + ReadInt( mdbBytes, pos ), false );
        pos += 0x4;

        //Texture Filename        
        texture.filename = ReadUnicode( mdbBytes, base + ReadInt( mdbBytes, pos ), false );
        pos += 0x4;

        //Unknown (Always Zero )
        texture.UNK0 = ReadInt( mdbBytes, pos );
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
        int ofs = ReadInt( mdbBytes, pos );
        if( ofs != 0 ) //We need to check this, to fill the name table with "Empty" should be be empty.
        {
            model.names.push_back( ReadUnicode( mdbBytes, pos + ofs, false ) ); //Add to names array
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
        mesh.skindata = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Material
        mesh.material = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Unknown (Always zero?)
        mesh.UNK0 = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Offset to Vertex Layout Info
        int vertexLayoutInfoOffs = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Size of vertices:
        mesh.vertexsize = ReadShort( mdbBytes, pos );
        pos += 0x2;

        //Layout Count:
        mesh.layoutcount = ReadShort( mdbBytes, pos );
        pos += 0x2;

        //Read the vertex layout information.
        ReadVertexLayoutInfo( mesh, base + vertexLayoutInfoOffs );

        //Number of vertices
        mesh.vertexnumber = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Mesh Index
        mesh.meshindex = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Offset to Vertex Data
        int vertexDataOffs = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Number of Indices
        mesh.indicesnumber = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Offset to Index Data
        int IndexDataInfoOffs = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Parse index data:
        int indPos = base + IndexDataInfoOffs;
        for( int v = 0; v < mesh.indicesnumber; ++v )
        {
            short data = ReadShort( mdbBytes, indPos );
            test2.push_back( data );
            indPos += 0x2;
        }

        //Parse vertex data.
        //run through "Vertex Size" using Vertex Layout
        int vertPos = base + vertexDataOffs;
        for( int v = 0; v < mesh.vertexnumber; ++v )
        {
            for( int vi = 0; vi < mesh.layoutcount; ++vi )
            {
                MDBVertexLayoutInfo info = mesh.vertexinfo[vi];

                if( info.type == 1 )
                {
                    vertPos += 16;
                }
                else if( info.type == 4 )
                {
                    vertPos += 12;
                }
                else if( info.type == 7 ) //array of 4 half floats
                {
                    short bytes = ReadShort( mdbBytes, vertPos );
                    vertPos += 0x2;

                    float x = UnpackHalf( bytes );

                    bytes = ReadShort( mdbBytes, vertPos );
                    vertPos += 0x2;

                    float y = UnpackHalf( bytes );

                    bytes = ReadShort( mdbBytes, vertPos );
                    vertPos += 0x2;

                    float z = UnpackHalf( bytes );

                    bytes = ReadShort( mdbBytes, vertPos );
                    vertPos += 0x2;

                    float w = UnpackHalf( bytes );

                    //Todo: Store this properly, this is a temporary solution.
                    if( mesh.vertexinfo[ vi ].name == "position" )
                        test.push_back( glm::vec3( x, y, z ));
                }
                else if( info.type == 12 ) //Float2, 2 float values.
                {
                    float x = ReadFloat( mdbBytes, vertPos, true );
                    vertPos += 0x4;

                    float y = ReadFloat( mdbBytes, vertPos, true );
                    vertPos += 0x4;

                    //Todo: Store this properly, this is a temporary solution.
                    if( mesh.vertexinfo[ vi ].name == "texcoord" )
                        uvs.push_back( glm::vec2( x, y ) );
                }
                if( info.type == 21 )
                {
                    vertPos += 4;
                }
            }
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
        info.type = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Offset in vertex data
        info.offset = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Channel
        info.channel = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Name
        int nameOffs = ReadInt( mdbBytes, pos );
        pos += 0x4;

        //Read Name:
        info.name = ReadASCII( mdbBytes, base + nameOffs );

        mesh.vertexinfo.push_back( info );
    }
}

