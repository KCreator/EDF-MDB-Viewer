#pragma once

//Read Integer from byte buffer
int ReadInt( std::vector<char> *buf, int pos, bool flipBytes = false );

//Read half int (short) from byte buffer
short ReadShort( std::vector<char> *buf, int pos );
unsigned short ReadUShort( std::vector<char> *buf, int pos );

//Read Floating Point from byte buffer
float ReadFloat( std::vector<char> *buf, int pos, bool reverseBytes = false );

//Read a unicode wide string from byte buffer
std::wstring ReadUnicode( std::vector<char> *chunk, int pos, bool swapEndian );

//Read ASCII string from byte buffer
std::string ReadASCII( std::vector<char> *chunk, int pos );

//Convert 2 byte half float, bytes stored as 'short' to float
float UnpackHalf( unsigned short bytes );
