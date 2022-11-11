#pragma once

//2D UI Elements to overlay on the rendered screen
class CBaseUIElement
{
public:
	void SetActive( bool active ) { bActive = active; };
	virtual void HandleEvent( sf::Event e ){};
	virtual void Draw( sf::RenderWindow *window ){};
	virtual void SetPosition( sf::Vector2f newPos ){};

	sf::Vector2f GetPosition(){ return vecPosition; };
	sf::FloatRect GetBounds(){ return recBounds; };
protected:
	bool bActive;
	sf::Vector2f vecPosition;
	sf::FloatRect recBounds;
};

//Container with title.
class CUITitledContainer : public CBaseUIElement
{
public:
	CUITitledContainer(){};
	CUITitledContainer( sf::Font &txtFont, std::string title, float sizeX, float sizeY, bool horizontal = false );

	void AddElement( CBaseUIElement *element );

	void HandleEvent( sf::Event e );

	void Draw( sf::RenderWindow *window );

	void SetPosition( sf::Vector2f pos );

protected:
	sf::RectangleShape containerShape;
	sf::RectangleShape containerTitleShape;
	sf::Text containerTitle;

	float xSize;
	float ySize;

	bool bHorizontalAligned;

	std::vector< CBaseUIElement * > containedElements;
};

//Simple button implementation
class CUIButton : public CBaseUIElement
{
public:
	CUIButton(){};
	CUIButton( sf::Font &txtFont, std::string name, float xSize = 150, float ySize = 30, unsigned int textSize = 20U );

	void SetPosition( sf::Vector2f pos );

	void HandleEvent( sf::Event e );

	void Draw( sf::RenderWindow *window );

	void SetCallback(std::function< void( ) > icallback );

protected:
	sf::RectangleShape buttonShape;
	sf::Text buttonName;

	std::function< void() > callback;

	bool bIsMouseOver;
	bool bHasCallback;
};

//Simple slider that controls a float value
class CUISlider : public CBaseUIElement
{
public:

	CUISlider(){};
	CUISlider( float *value, float low, float high, float size );

	void Draw( sf::RenderWindow *window );

	void HandleEvent( sf::Event e );

	void SetPosition( sf::Vector2f pos );

protected:
	float *controlledValue; //Ptr to float value we control.
	float minVaue;
	float maxValue;

	bool isDragging;
	float dragXOffs;

	//sf drawables:
	sf::RectangleShape shpSlider;
	sf::RectangleShape shpSliderBG;
};

//Text field UI element.
class CUITextfield : public CBaseUIElement
{
public:
	CUITextfield(){};
	CUITextfield( sf::Font &txtFont, std::string defaultString, float size, unsigned int txtSize = 20U );

	void SetPosition( sf::Vector2f pos );

	//Handle SF events.
	void HandleEvent( sf::Event e );

	//Draw elements
	void Draw( sf::RenderWindow *window );

protected:
	//Control variables
	bool bHasContext;
	int iCursorPos;
	sf::String strText;

	//Rendered sf elements
	sf::Text txtText;
	sf::RectangleShape shpBG;
	sf::RectangleShape shpCursor;
};

//Simple labeled value implementation
class CUILabelledValue : public CBaseUIElement
{
public:
	CUILabelledValue(){};
	CUILabelledValue( sf::Font &txtFont, std::string name, unsigned int fontSize = 20U )
	{
		txtLabelText = sf::Text( name, txtFont, fontSize );
		txtLabelText.setFillColor( sf::Color( 0, 255, 0 ) );

		labelString = name;

		recBounds = txtLabelText.getGlobalBounds();
	};

	void SetTrackedValue( float *value )
	{
		trackedValue = value;

		oldValue = *trackedValue;
		txtLabelText.setString( labelString + std::to_string( *value ) );
	};

	void Draw( sf::RenderWindow *window )
	{
		if( !bActive )
			return;

		//Check if tracked value has changed.
		if( trackedValue != NULL )
		{
			if( *trackedValue != oldValue )
			{
				txtLabelText.setString( labelString + std::to_string( *trackedValue ) );
				oldValue = *trackedValue;
			}
		}

		window->draw( txtLabelText );
	};

	void SetPosition( sf::Vector2f pos )
	{
		txtLabelText.setPosition( pos );
		vecPosition = pos;

		recBounds = txtLabelText.getGlobalBounds();
		recBounds.height *= 2;
	};

protected:
	float oldValue;
	float *trackedValue;

	std::string labelString;
	sf::Text txtLabelText;
};
