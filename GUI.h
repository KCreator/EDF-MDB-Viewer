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
protected:
	bool bActive;
	sf::Vector2f vecPosition;
};

//Container with title.
class CUITitledContainer : public CBaseUIElement
{
public:
	CUITitledContainer(){};
	CUITitledContainer( sf::Font &txtFont, std::string title, float sizeX, float sizeY );

	void AddElement( CBaseUIElement *element );

	void HandleEvent( sf::Event e );

	void Draw( sf::RenderWindow *window );

protected:
	sf::RectangleShape containerShape;
	sf::RectangleShape containerTitleShape;
	sf::Text containerTitle;

	float xSize;
	float ySize;

	std::vector< CBaseUIElement * > containedElements;
};

//Simple button implementation
class CUIButton : public CBaseUIElement
{
public:
	CUIButton(){};
	CUIButton( sf::Font &txtFont, std::string name );

	void SetPosition( sf::Vector2f pos );

	void HandleEvent( sf::Event e );

	void Draw( sf::RenderWindow *window );

	void SetCallback(std::function< void( ) > icallback );

protected:
	sf::RectangleShape buttonShape;
	sf::Text buttonName;

	std::function< void() > callback;

	bool bIsMouseOver;
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
