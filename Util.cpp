//########################################################
//Utility Functions:
//########################################################

#include <vector>
#include <string.h>
#include <codecvt>
#include <locale>
#include <math.h>
#include "Util.h"

//Read Integer from byte buffer
int ReadInt( std::vector<char> *buf, int pos, bool flipBytes )
{
    unsigned char chunk[4];

	if( flipBytes )
	{
		chunk[0] = buf->at( pos );
		chunk[1] = buf->at( pos + 1 );
		chunk[2] = buf->at( pos + 2 );
		chunk[3] = buf->at( pos + 3 );
	}
	else
	{
		chunk[3] = buf->at( pos );
		chunk[2] = buf->at( pos + 1 );
		chunk[1] = buf->at( pos + 2 );
		chunk[0] = buf->at( pos + 3 );
	}

	int num = 0;
	for(int i = 0; i < 4; i++)
	{
		num <<= 8;
		num |= chunk[i];
	}

    return num;
}

//Read half int (short) from byte buffer
short ReadShort( std::vector<char> *buf, int pos )
{
    unsigned char chunk[2];

	chunk[0] = buf->at( pos );
	chunk[1] = buf->at( pos + 1 );

	short num;
    memcpy( &num, chunk, sizeof( short ) );

    return num;
}

//Read unsigned int (short) from byte buffer
unsigned short ReadUShort( std::vector<char> *buf, int pos )
{
    //Error checking:
    if( pos >= buf->size() - 1 )
        return 0;

    unsigned char chunk[2];

	chunk[0] = buf->at( pos );
	chunk[1] = buf->at( pos + 1 );

	unsigned short num;
    memcpy( &num, chunk, sizeof( unsigned short ) );

    return num;
}

//Read Floating Point from byte buffer
float ReadFloat( std::vector<char> *buf, int pos, bool reverseBytes )
{
    unsigned char chunk[4];

    if( !reverseBytes )
    {
        chunk[3] = buf->at( pos );
        chunk[2] = buf->at( pos + 1 );
        chunk[1] = buf->at( pos + 2 );
        chunk[0] = buf->at( pos + 3 );
    }
    else
    {
        //float val;
        //memcpy( &val, &buf->data()[pos], sizeof( float ) );
        //return val;

        chunk[0] = buf->at( pos );
	    chunk[1] = buf->at( pos + 1 );
	    chunk[2] = buf->at( pos + 2 );
	    chunk[3] = buf->at( pos + 3 );
    }

    float val;
	memcpy( &val, chunk, sizeof( float ) );
    return val;
}

//Read a unicode wide string from byte buffer
#ifndef WINDOWS
std::wstring ReadUnicode( std::vector<char> *chunk, int pos, bool swapEndian )
{
	if( pos > chunk->size( ) )
		return L"";

	unsigned int bufPos = pos;

    std::wstring strn;
	unsigned char c[2];
    std::wstring_convert<std::codecvt_utf16<wchar_t>,wchar_t> convert;

	//Repeat until EOF, or otherwise broken
	while( bufPos < chunk->size() )
	{
		c[0] = chunk->at(bufPos);
		c[1] = chunk->at(bufPos+1);

        if( c[0] == 0x0 && c[1] == 0x0 )
         break;

        //No longer crashes, but gets the wrong output for Japanese text. Windows machines might have better luck.

        //Hack:
        if( c[1] == 0x0 )
        {
            wchar_t ch = L' ';
            memcpy( &ch, &c, 2 );
            strn += ch;
        }
        else
        {
            strn += convert.from_bytes( ( const char * )&c[0]  );
        }

        bufPos += 2;
	}

	return strn;
}
#else
std::wstring ReadUnicode(std::vector<char> *chunk, int pos, bool swapEndian)
{
	if (pos > chunk->size())
		return L"";

	unsigned int bufPos = pos;

	std::vector< unsigned char > bytes;

	int zeroCount = 0;

	//Repeat until EOF, or otherwise broken
	while (bufPos < chunk->size())
	{
		if (swapEndian)
		{
			if (bufPos % 2)
			{
				bytes.push_back(chunk->at(bufPos));
				bytes.push_back(chunk->at(bufPos - 1));
			}
		}
		else
			bytes.push_back(chunk->at(bufPos));

		if( chunk->at(bufPos) == 0x00 )
		{
			zeroCount++;
			if (zeroCount == 2)
				break;
		}
		else
			zeroCount = 0;
		bufPos++;
	}

	//if( bytes.size( ) == 0 );
	//return L"";


	std::wstring wstr(reinterpret_cast<wchar_t*>(&bytes[0]), bytes.size() / sizeof(wchar_t));

	return wstr;
}
#endif

//Read ASCII string from byte buffer
std::string ReadASCII( std::vector<char> *chunk, int pos )
{
    if( pos > chunk->size( ) )
		return "";

	unsigned int bufPos = pos;
    std::string strn;

    //Repeat until EOF, or otherwise broken
	while( bufPos < chunk->size() )
	{
        if( chunk->at(bufPos) == 0x0)
        break;

        strn += chunk->at(bufPos);
        bufPos++;
    }

    return strn;
}

//Convert 2 byte half float, bytes stored as 'short' to float
float UnpackHalf( unsigned short bytes )
{
    int s = (bytes & 0x8000) >> 15;
    int e = (bytes & 0x7C00) >> 10;
    float f = bytes & 0x03FF;

    if(e == 0)
    {
        return (s?-1:1) * pow(2,-14) * (f/pow(2, 10));
    }
    else if (e == 0x1F)
    {
        return f ? NAN:(( s ? -1:1 ) * INFINITY );
    }

    return (s?-1:1) * pow(2, e-15) * (1+(f/pow(2, 10)));
}