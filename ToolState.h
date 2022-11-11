#pragma once

//###################################################
//Tool State System
//###################################################

//Tool state system, will allow the tool to have multiple "screens", IE, a file browser, that it can swap between
class CToolStateHandler;

class BaseToolState
{
public:
	BaseToolState()
	{
		pOwner = NULL;
	};

	virtual void Init( sf::RenderWindow *iwindow )
	{
		window = iwindow;
	};
	virtual void ProccessEvent( sf::Event event ){};
	virtual void Draw(){};

	void SetOwner( CToolStateHandler *owner )
	{
		pOwner = owner;
	};

	CToolStateHandler * GetOwner()
	{
		return pOwner;
	};

protected:
	sf::RenderWindow *window;
	CToolStateHandler *pOwner;
};

//Ugly little State manager system
class CToolStateHandler
{
public:
	~CToolStateHandler()
	{
		state_map.clear(); //Purge map and delete elements.
	}

	void AddState( std::string name, BaseToolState *state )
	{
		if( state_map.count( name ) == 0 ) //We do not already have a state of this name:
		{
			state_map[ name ] = state;
			state->SetOwner( this );
		}
	};
	BaseToolState * GetState( std::string name )
	{
		return state_map[ name ];
	};

	void SetState( std::string state )
	{
		curState = state;
	};

	void ProccessEvent( sf::Event e )
	{
		if( state_map.count( curState ) > 0 )
		{
			state_map[ curState ]->ProccessEvent( e );
		}
	};
	void Draw()
	{
		if( state_map.count( curState ) > 0 )
		{
			state_map[ curState ]->Draw();
		}
	};

	std::map< std::string, BaseToolState * > state_map;

private:
	std::string curState;
};
