#include <vector>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "GUI.h"

//##################################################
// class: CUITitledContainer
// desc: Container with title.
//##################################################

//Constructor
CUITitledContainer::CUITitledContainer( sf::Font &txtFont, std::string title, float sizeX, float sizeY )
{
	xSize = sizeX;
	ySize = sizeY;

	containerShape = sf::RectangleShape( sf::Vector2f( xSize, ySize ) );
	containerShape.setFillColor( sf::Color::Black );
	containerShape.setOutlineColor( sf::Color::Green );
	containerShape.setOutlineThickness( -1 );

	containerTitle = sf::Text( title, txtFont, 20u );

	float textX = ( xSize / 2.0f ) - (containerTitle.getGlobalBounds().width / 2.0f);
	containerTitle.setPosition( textX, 0 );
	containerTitle.setFillColor( sf::Color( 0, 255, 0 ) );

	containerTitleShape = sf::RectangleShape( sf::Vector2f( xSize, containerTitle.getGlobalBounds().height * 2.0f ) );
	containerTitleShape.setFillColor( sf::Color::Black );
	containerTitleShape.setOutlineColor( sf::Color::Green );
	containerTitleShape.setOutlineThickness( -1 );
};

//Adds an element to the container
void CUITitledContainer::AddElement( CBaseUIElement *element )
{
	float yOffset = containedElements.size() * 32;

	element->SetPosition( GetPosition() + sf::Vector2f( 4, containerTitleShape.getGlobalBounds().height + 4 + yOffset ) );

	containedElements.push_back( element );
}

//Event handler
void CUITitledContainer::HandleEvent( sf::Event e )
{
	for( int i = 0; i < containedElements.size(); ++i )
	{
		containedElements[i]->HandleEvent( e );
	}
}

//Draws the container and its elements
void CUITitledContainer::Draw( sf::RenderWindow *window )
{
	if( !bActive )
		return;

	window->draw( containerShape );
	window->draw( containerTitleShape );
	window->draw( containerTitle );

	for( int i = 0; i < containedElements.size(); ++i )
	{
		containedElements[i]->Draw( window );
	}
};

//##################################################
// class: CUIButton
// desc: Simple button implementation
//##################################################

//Constructor
CUIButton::CUIButton( sf::Font &txtFont, std::string name )
{
	float xSize = 150;
	float ySize = 30;

	buttonShape = sf::RectangleShape( sf::Vector2f( xSize, ySize ) );
	buttonShape.setFillColor( sf::Color( 150, 150, 150 ) );
	buttonShape.setOutlineColor( sf::Color( 200, 200, 200 ) );
	buttonShape.setOutlineThickness( -1 );

	buttonName = sf::Text( name, txtFont, 20u );
	buttonName.setFillColor( sf::Color::Black );

	float textX = ( xSize / 2.0f ) - ( buttonName.getGlobalBounds().width / 2.0f );
	float textY = ( ySize / 2.0f ) - ( buttonName.getGlobalBounds().height );
	buttonName.setPosition( textX, textY );

	bIsMouseOver = false;
};

//Set button position, and update the position of elememts that comprise the button.
void CUIButton::SetPosition( sf::Vector2f pos )
{
	buttonShape.setPosition( buttonShape.getPosition() + ( pos - vecPosition ) );
	buttonName.setPosition( buttonName.getPosition() + ( pos - vecPosition ) );

	vecPosition = pos;
};

//Handle Events
void CUIButton::HandleEvent( sf::Event e )
{
	if( !bActive )
		return;

	if( e.type == sf::Event::MouseMoved ) //Highlight button when moused over.
	{
		if( buttonShape.getGlobalBounds().contains( sf::Vector2f( e.mouseMove.x, e.mouseMove.y ) ) )
		{
			buttonShape.setFillColor( sf::Color::Red );
			bIsMouseOver = true;
		}
		else
		{
			buttonShape.setFillColor( sf::Color( 150, 150, 150 ) );
			bIsMouseOver = false;
		}
	}

	//Perform button action on click
	else if( e.type == sf::Event::MouseButtonPressed )
	{
		if( e.mouseButton.button == sf::Mouse::Button::Left )
		{
			//Todo: Implement this.
			if( bIsMouseOver )
				callback( );
		}
	}
}

//Draw the button
void CUIButton::Draw( sf::RenderWindow *window )
{
	if( !bActive )
		return;

	window->draw( buttonShape );
	window->draw( buttonName );
};

//Sets the function the button will execute on click.
void CUIButton::SetCallback(std::function< void( ) > icallback )
{
	callback = icallback;
	//pState = owner;
};

//##################################################
// class: CUISlider
// desc: Simple slider that controls a float value
//##################################################

CUISlider::CUISlider( float *value, float low, float high, float size )
{
	//Generate UI elements
	shpSliderBG = sf::RectangleShape( sf::Vector2f( size, 10 ) );
	shpSliderBG.setFillColor( sf::Color( 150, 150, 150 ) );

	shpSlider = sf::RectangleShape( sf::Vector2f( 10, 32 ) );
	shpSlider.setFillColor( sf::Color( 255, 255, 255 ) );

	shpSliderBG.setPosition( 0, (shpSlider.getGlobalBounds().height / 2.0f) - (shpSliderBG.getGlobalBounds().height / 2.0f) );

	minVaue = low;
	maxValue = high;

	controlledValue = value;

	if( controlledValue != NULL )
	{
		//Set slider to a default position based on the controlled value:
		float upperBounds = shpSliderBG.getGlobalBounds().width - shpSlider.getGlobalBounds().width;

		float v1 = (low + (*value * ( high-low )));

		shpSlider.setPosition( v1 * upperBounds, 0 );
	}

	//Standard init:
	isDragging = false;
}

void CUISlider::Draw( sf::RenderWindow *window )
{
	if( !bActive )
		return;

	window->draw( shpSliderBG );
	window->draw( shpSlider );
};

void CUISlider::HandleEvent( sf::Event e )
{
	if( !bActive )
		return;

	if( e.type == sf::Event::MouseButtonPressed )
	{
		if( e.mouseButton.button == sf::Mouse::Button::Left )
		{
			if( !isDragging )
			{
				if( shpSlider.getGlobalBounds().contains( e.mouseButton.x, e.mouseButton.y ) )
				{
					shpSlider.setFillColor( sf::Color( 255, 0, 0 ) );
					isDragging = true;

					dragXOffs = e.mouseButton.x - shpSlider.getPosition().x;
				}
			}
		}
	}
	else if ( e.type == sf::Event::MouseButtonReleased )
	{
		if( e.mouseButton.button == sf::Mouse::Button::Left )
		{
			shpSlider.setFillColor( sf::Color( 255, 255, 255 ) );

			isDragging = false;
		}
	}
	else if( e.type == sf::Event::MouseMoved ) //Highlight button when moused over.
	{
		if( isDragging )
		{
			//Establish needed variables
			sf::Vector2f pos = shpSlider.getPosition();
			float upperBounds = shpSliderBG.getGlobalBounds().width - shpSlider.getGlobalBounds().width;

			//Calculate new position:
			pos.x = e.mouseMove.x - dragXOffs;
			if( pos.x < shpSliderBG.getGlobalBounds().left ) //Clamp x min
				pos.x = shpSliderBG.getGlobalBounds().left;
			if( pos.x > shpSliderBG.getGlobalBounds().left + upperBounds ) //Clamp x max
				pos.x = shpSliderBG.getGlobalBounds().left + upperBounds;

			if( controlledValue != NULL )
			{
				//Calculate output value:
				float value = (pos.x - shpSliderBG.getGlobalBounds().left) / upperBounds;
				//Remap to mins and max
				value = minVaue + ( value * (maxValue - minVaue ) );

				//Set assigned control value to our new value.
				*controlledValue = value;
			}

			pos.y = shpSlider.getPosition().y;

			shpSlider.setPosition( pos );
		}
	}
}

void CUISlider::SetPosition( sf::Vector2f pos )
{
	shpSlider.setPosition( shpSlider.getPosition() + ( pos - vecPosition ) );
	shpSliderBG.setPosition( shpSliderBG.getPosition() + ( pos - vecPosition ) );

	vecPosition = pos;
};
