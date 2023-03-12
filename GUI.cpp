#include <vector>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "GUI.h"

//##################################################
// class: CUITitledContainer
// desc: Container with title.
//##################################################

//Constructor
CUITitledContainer::CUITitledContainer( sf::Font &txtFont, std::string title, float sizeX, float sizeY, bool horizontal )
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

	bHorizontalAligned = horizontal;
};

//Adds an element to the container
void CUITitledContainer::AddElement( CBaseUIElement *element )
{
	float spacing = 2; //TODO: Make this configurable.

	if( bHorizontalAligned )
	{
		float xOffset = 0;
		for( int i = 0; i < containedElements.size(); ++i )
		{
			xOffset += containedElements[i]->GetBounds().width + spacing;
		}

		element->SetPosition( GetPosition() + sf::Vector2f( 4 + xOffset, containerTitleShape.getGlobalBounds().height + 4 ) );
	}
	else
	{
		float yOffset = 0;
		for( int i = 0; i < containedElements.size(); ++i )
		{
			yOffset += containedElements[i]->GetBounds().height + spacing;
		}

		element->SetPosition( GetPosition() + sf::Vector2f( 4, containerTitleShape.getGlobalBounds().height + 4 + yOffset ) );
	}

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
void CUITitledContainer::Draw( sf::RenderTarget *window )
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

//Set position, and update the position of elements that comprise the container.
void CUITitledContainer::SetPosition( sf::Vector2f pos )
{
	containerShape.setPosition( containerShape.getPosition() + ( pos - vecPosition ) );
	containerTitleShape.setPosition( containerTitleShape.getPosition() + ( pos - vecPosition ) );
	containerTitle.setPosition( containerTitle.getPosition() + ( pos - vecPosition ) );

	//Update positions of everything in the list
	for( int i = 0; i < containedElements.size(); ++i )
	{
		containedElements[i]->SetPosition( containedElements[i]->GetPosition() + ( pos - vecPosition ) );
	}

	vecPosition = pos;

	//Update bounds position:
	recBounds.top = pos.y;
	recBounds.left = pos.x;
};

//##################################################
// class: CUIButton
// desc: Simple button implementation
//##################################################

//Constructor
CUIButton::CUIButton( sf::Font &txtFont, std::string name, float xSize, float ySize, unsigned int textSize )
{
	buttonShape = sf::RectangleShape( sf::Vector2f( xSize, ySize ) );
	buttonShape.setFillColor( sf::Color( 150, 150, 150 ) );
	buttonShape.setOutlineColor( sf::Color( 200, 200, 200 ) );
	buttonShape.setOutlineThickness( -1 );

	buttonName = sf::Text( name, txtFont, textSize );
	buttonName.setFillColor( sf::Color::Black );

	float textX = ( xSize / 2.0f ) - ( buttonName.getGlobalBounds().width / 2.0f );
	float textY = ( ySize / 2.0f ) - ( buttonName.getGlobalBounds().height );
	buttonName.setPosition( textX, textY );

	bIsMouseOver = false;
	bHasCallback = false;

	recBounds = sf::FloatRect( vecPosition, sf::Vector2f( xSize, ySize ) );
};

//Set button position, and update the position of elememts that comprise the button.
void CUIButton::SetPosition( sf::Vector2f pos )
{
	buttonShape.setPosition( buttonShape.getPosition() + ( pos - vecPosition ) );
	buttonName.setPosition( buttonName.getPosition() + ( pos - vecPosition ) );

	vecPosition = pos;

	//Update bounds position:
	recBounds.top = pos.y;
	recBounds.left = pos.x;
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
			if( bIsMouseOver && bHasCallback )
				callback( );
		}
	}
}

//Draw the button
void CUIButton::Draw( sf::RenderTarget *window )
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
	bHasCallback = true;
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

		float v1 = ( ( ( *value ) / ( high + low ) ) );

		shpSlider.setPosition( v1 * upperBounds, 0 );
	}

	//Set bounds:
	recBounds = sf::FloatRect( vecPosition, sf::Vector2f( shpSliderBG.getGlobalBounds().width, shpSlider.getGlobalBounds().height ) );

	//Standard init:
	isDragging = false;
}

void CUISlider::Draw( sf::RenderTarget *window )
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

	//Update bounds position:
	recBounds.top = pos.y;
	recBounds.left = pos.x;
};

//##################################################
// class: CUISliderVertical
// desc: Simple slider that controls a float value
//##################################################
CUISliderVertical::CUISliderVertical( float* value, float low, float high, float size )
{
	// Generate UI elements
	shpSliderBG = sf::RectangleShape( sf::Vector2f( 10, size ) );
	shpSliderBG.setFillColor( sf::Color( 150, 150, 150 ) );

	shpSlider = sf::RectangleShape( sf::Vector2f( 32, 10 ) );
	shpSlider.setFillColor( sf::Color( 255, 255, 255 ) );

	shpSliderBG.setPosition( (shpSlider.getGlobalBounds().width / 2.0f) - (shpSliderBG.getGlobalBounds().width / 2.0f), 0 );

	minValue = low;
	maxValue = high;

	controlledValue = value;

	if( controlledValue != nullptr )
	{
		// Set slider to a default position based on the controlled value:
		float upperBounds = shpSliderBG.getGlobalBounds().height - shpSlider.getGlobalBounds().height;
		float v1 = (((*value) / (high + low)));
		shpSlider.setPosition( 0, (v1 * upperBounds) );
	}

	// Set bounds:
	recBounds = sf::FloatRect( vecPosition, sf::Vector2f( shpSlider.getGlobalBounds().width, shpSliderBG.getGlobalBounds().height ) );

	// Standard init:
	isDragging = false;
}

void CUISliderVertical::Draw( sf::RenderTarget* window )
{
	if( !bActive )
		return;

	window->draw( shpSliderBG );
	window->draw( shpSlider );
};

void CUISliderVertical::HandleEvent( sf::Event e )
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

					dragYOffs = e.mouseButton.y - shpSlider.getPosition().y;
				}
				else if( shpSliderBG.getGlobalBounds().contains( e.mouseButton.x, e.mouseButton.y ) )
				{
					//Establish needed variables
					sf::Vector2f pos = shpSlider.getPosition();
					float upperBounds = shpSliderBG.getGlobalBounds().height - shpSlider.getGlobalBounds().height;

					//Calculate new position:
					pos.y = e.mouseButton.y;
					if( pos.y < shpSliderBG.getGlobalBounds().top ) //Clamp y min
						pos.y = shpSliderBG.getGlobalBounds().top;
					if( pos.y > shpSliderBG.getGlobalBounds().top + upperBounds ) //Clamp y max
						pos.y = shpSliderBG.getGlobalBounds().top + upperBounds;

					if( controlledValue != NULL )
					{
						//Calculate output value:
						float value = (pos.y - shpSliderBG.getGlobalBounds().top) / upperBounds;
						//Remap to mins and max
						value = minValue + (value * (maxValue - minValue));

						//Set assigned control value to our new value.
						*controlledValue = value;
					}

					pos.x = shpSlider.getPosition().x;

					shpSlider.setPosition( pos );
				}
			}
		}
	}
	else if( e.type == sf::Event::MouseButtonReleased )
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
			float upperBounds = shpSliderBG.getGlobalBounds().height - shpSlider.getGlobalBounds().height;

			//Calculate new position:
			pos.y = e.mouseMove.y - dragYOffs;
			if( pos.y < shpSliderBG.getGlobalBounds().top ) //Clamp y min
				pos.y = shpSliderBG.getGlobalBounds().top;
			if( pos.y > shpSliderBG.getGlobalBounds().top + upperBounds ) //Clamp y max
				pos.y = shpSliderBG.getGlobalBounds().top + upperBounds;

			if( controlledValue != NULL )
			{
				//Calculate output value:
				float value = (pos.y - shpSliderBG.getGlobalBounds().top) / upperBounds;
				//Remap to mins and max
				value = minValue + (value * (maxValue - minValue));

				//Set assigned control value to our new value.
				*controlledValue = value;
			}

			pos.x = shpSlider.getPosition().x;

			shpSlider.setPosition( pos );
		}
	}
}

void CUISliderVertical::SetPosition( sf::Vector2f pos )
{
	shpSlider.setPosition( shpSlider.getPosition() + (pos - vecPosition) );
	shpSliderBG.setPosition( shpSliderBG.getPosition() + (pos - vecPosition) );

	vecPosition = pos;

	//Update bounds position:
	recBounds.top = pos.y;
	recBounds.left = pos.x;
};

//##################################################
// class: CUITextfield
// desc: Text input field
//##################################################

CUITextfield::CUITextfield( sf::Font &txtFont, std::string defaultString, float size, unsigned int txtSize )
{
	//Establish defaut values:
	bHasContext = false;
	iCursorPos = 0;

	strText = defaultString;

	//Generate all nessasary sf elements.
	txtText = sf::Text( strText, txtFont, txtSize );
	txtText.setFillColor( sf::Color( 0, 0, 0 ) );

	//Create BG
	shpBG = sf::RectangleShape( sf::Vector2f( size, txtText.getGlobalBounds().height * 2.0f ) );
	shpBG.setFillColor( sf::Color( 200, 200, 200 ) );
	shpBG.setOutlineColor( sf::Color( 50, 50, 50 ) );
	shpBG.setOutlineThickness( -2 );

	txtText.setPosition( sf::Vector2f( 2, 0 ) );

	shpCursor = sf::RectangleShape( sf::Vector2f( 1, txtText.getGlobalBounds().height * 2.0f ) );
	shpCursor.setFillColor( sf::Color( 0, 0, 0 ) );

	recBounds = shpBG.getGlobalBounds();
}

void CUITextfield::SetPosition( sf::Vector2f pos )
{
	shpBG.setPosition( shpBG.getPosition() + ( pos - vecPosition ) );
	txtText.setPosition( txtText.getPosition() + ( pos - vecPosition ) );

	vecPosition = pos;

	//Update bounds position:
	recBounds.top = pos.y;
	recBounds.left = pos.x;
}

//Handle SF events.
void CUITextfield::HandleEvent( sf::Event e )
{
	if( e.type == sf::Event::MouseButtonPressed )
	{
		if( e.mouseButton.button == sf::Mouse::Button::Left )
		{
			//Gain or loose context.
			if( shpBG.getGlobalBounds().contains( sf::Vector2f( e.mouseButton.x, e.mouseButton.y ) ) )
			{
				bHasContext = true;

				//Figure out cursor position from mouse position:
				float closest = 100.0f;

				for( int i = 0; i <= strText.getSize(); ++i )
				{
					float dx = e.mouseButton.x - (txtText.findCharacterPos(i).x);
					if( abs(dx) < abs(closest) )
					{
						closest = dx;
						iCursorPos = i;
					}
				}
			}
			else
				bHasContext = false;
		}
	}

	if( bHasContext )
	{
		if( e.type == sf::Event::KeyPressed )
		{
			if( e.key.code == sf::Keyboard::BackSpace )
			{
				strText.erase( iCursorPos - 1, 1 );
				iCursorPos--;

				txtText.setString( strText );
			}
			else if( e.key.code == sf::Keyboard::Delete )
			{
				strText.erase( iCursorPos, 1 );
				txtText.setString( strText );
			}
			else if( e.key.code == sf::Keyboard::Left )
			{
				if( iCursorPos > 0 )
					iCursorPos--;
			}
			else if( e.key.code == sf::Keyboard::Right )
			{
				if( iCursorPos < strText.getSize() )
					iCursorPos++;
			}
		}
		else if( e.type == sf::Event::TextEntered )
		{
			if( !std::iscntrl( e.text.unicode ) )
			{
				strText.insert( iCursorPos, e.text.unicode );
				iCursorPos++;

				txtText.setString( strText );
			}
		}
	}
}

//Draw elements
void CUITextfield::Draw( sf::RenderTarget *window )
{
	if( !bActive )
		return;

	window->draw( shpBG );
	window->draw( txtText );

	if( bHasContext )
	{
		shpCursor.setPosition( txtText.findCharacterPos( iCursorPos ) );
		window->draw( shpCursor );
	}
};


//##################################################
// class: 
// desc: 
//##################################################

