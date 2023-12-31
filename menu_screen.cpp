#include "screens.h"

/* For a given font and baseline (character size),
the function finds the maximum height among the glyphs of standard ASCII (0-127),
*in the global 2D world coordinates*.

Basically, because SFML is giving me a hard time, we need to iterate over all the glyphs in order to find the maximum height of a character;
so that we can give this height to the rectangle that contains each menu option.
It's a bit complicated, best to read in this thread:
https://en.sfml-dev.org/forums/index.php?topic=29332.0 */
float MenuScreen::getTopAndBottomOfGlyphs(int baseline){
    sf::Font temp_font;
    if (!temp_font.loadFromFile("resources/arial.ttf")) exit(-1);

    float max_height = 0;
    for (int i = 0; i < 128; i++){
        sf::Glyph glyph = temp_font.getGlyph(i, baseline, false);
        sf::FloatRect glyph_bounds = glyph.bounds; // The bounds are relative to the baseline, so we transform them below

        float curr_top = glyph_bounds.top + baseline;
        max_height = std::max(max_height, curr_top + glyph_bounds.height);
    }

    return max_height;
}

MenuScreen::MenuScreen():
option_chosen_color(sf::Color::Red), option_not_chosen_color(sf::Color::White),
menu_title("", font, TITLE_CHARACTER_SIZE),
menu_title_rectangle_height(getTopAndBottomOfGlyphs(TITLE_CHARACTER_SIZE) + DISTANCE),
menu_option_rectangle_height(getTopAndBottomOfGlyphs(OPTION_CHARACTER_SIZE) + DISTANCE) { // DISTANCE here is like vertical padding inside the rectangle
    menu_title.setFillColor(important_color);
    menu_title.setStyle(sf::Text::Bold | sf::Text::Underlined);

    if (!arrow_up_texture.loadFromFile("resources/up.png")) exit(-1);
    arrow_up_texture.setSmooth(true);
    arrow_up_sprite.setTexture(arrow_up_texture);
    arrow_up_sprite.setScale(0.1, 0.1);

    if (!arrow_down_texture.loadFromFile("resources/down.png")) exit(-1);
    arrow_down_texture.setSmooth(true);
    arrow_down_sprite.setTexture(arrow_down_texture);
    arrow_down_sprite.setScale(0.1, 0.1);

    setArrows();
}

/* This function is used by PatternMenuScreen and AutomatonMenuScreen, and they have slightly different needs, so it's a bit convoluted;
but it's worth it, in order to reduce code duplication and to encapsulate.
1) When 'AutomatonMenuScreen' calls this function, 'pattern_menu_screen_container' is nullptr and doesn't affect anything.
When 'PatternMenuScreen' calls this function, 'pattern_menu_screen_container' tells whether the menu option is a directory name,
and if it is, we return and not hover on it.
2) Clarification: When hovering is false, 'hovered_menu_option' simply holds the option corresponding to the vertical cursor height
When hovering becomes true, this variable holds the real option on which we hover */
void MenuScreen::handleHover(bool& hovering, int& rectangle_index, std::vector <std::string>* pattern_menu_screen_container,
                             sf::Text*& hovered_menu_option, sf::FloatRect& hovered_rectangle_bounds){
    if (!window.hasFocus()) return;

    // Convert mouse position (if it's inside window) to menu option it hovers above
    sf::Vector2i pixel_pos = sf::Mouse::getPosition(window);
    sf::Vector2f view_pos = window.mapPixelToCoords(pixel_pos);

    if (hovering){
        if (!hovered_rectangle_bounds.contains(view_pos)){ // Un-hover
            hovering = false;
            hovered_menu_option->setFillColor(option_not_chosen_color);

            cursor.loadFromSystem(sf::Cursor::Arrow);
            window.setMouseCursor(cursor);
        }
    }
    else{
        // Note that we shift in 'menu_title_rectangle_height' downward, to compensate for the title text.
        bool inside_menu = 0 <= view_pos.x && view_pos.x < window_width &&
                menu_title_rectangle_height <= view_pos.y && view_pos.y < menu_screen_total_height;
        if (!inside_menu) return;
        rectangle_index = (view_pos.y - menu_title_rectangle_height) / menu_option_rectangle_height;
        // For PatternMenuScreen - we don't hover if we're on a directory name
        if (pattern_menu_screen_container && (*pattern_menu_screen_container)[rectangle_index].empty()) return;

        hovered_menu_option = &menu_options[rectangle_index];
        float hovered_menu_option_left = hovered_menu_option->getGlobalBounds().left;
        float hovered_menu_option_width = hovered_menu_option->getGlobalBounds().width;

        if (hovered_menu_option_left <= view_pos.x && view_pos.x < hovered_menu_option_left + hovered_menu_option_width){ // Start hovering
            hovering = true;
            hovered_menu_option->setFillColor(option_chosen_color);
            // We create our own rectangle, *as to use our own top and height* (and not GlobalBounds),
            // as to create the illusion that the menu options are directly "stacked above each other"
            hovered_rectangle_bounds = sf::FloatRect(hovered_menu_option_left, menu_title_rectangle_height + rectangle_index * menu_option_rectangle_height,
                                                   hovered_menu_option_width, menu_option_rectangle_height);

            cursor.loadFromSystem(sf::Cursor::Hand);
            window.setMouseCursor(cursor);
        }
    }
}

void MenuScreen::handleScroll(short int delta){
    delta *= 6 * DISTANCE;

    if (0 < delta && left_top_view_pos.y - delta < 0){ // Scroll up beyond bound - we don't allow it
        left_top_view_pos.y = 0;
    }
    else if (delta < 0 && menu_screen_total_height < left_top_view_pos.y + window_height - delta){ // Scroll down beyond bound - we don't allow it
        // We use 'max' here for cases when 'menu_screen_total_height < window_height' (like in automaton screen)
        left_top_view_pos.y = std::max(0, (int)menu_screen_total_height - window_height);
    }
    else{ // Legal scroll
        left_top_view_pos.y -= delta;
    }

    setArrows();
    view.reset(sf::FloatRect(0, left_top_view_pos.y, window_width, window_height));
    window.setView(view);
}

// Making sure to truncate (if needed) filenames, and setting the Text objects in a list in the middle of the screen
void MenuScreen::setText(){
    centerText(menu_title, 0);

    float height = menu_title_rectangle_height; // Start positioning options right below title
    for (auto& menu_option : menu_options){
        centerText(menu_option, height);
        height += menu_option_rectangle_height;
    }
}

// Only drawing the viewed menu options
void MenuScreen::drawText() const{
    int first_menu_option_index = left_top_view_pos.y / menu_option_rectangle_height - 1;
    int last_menu_option_index = (left_top_view_pos.y + window_height) / menu_option_rectangle_height + 1;
    for (int i = first_menu_option_index; i < last_menu_option_index; i++){
        // Since we add 1 or subtract 1 above, there's a chance we got out of range,
        // and it's easier to ask for forgiveness here than to ask for permission :-)
        try {
            window.draw(menu_options.at(i));
        } catch (std::out_of_range& exc) {}
    }
}

void MenuScreen::setArrows(){
    arrow_up_sprite.setPosition(0, left_top_view_pos.y);
    arrow_down_sprite.setPosition(0, left_top_view_pos.y + window_height - arrow_down_sprite.getGlobalBounds().width);
}

void MenuScreen::drawArrows() const{
    if (menu_screen_total_height <= window_height) return;

    if (0 < left_top_view_pos.y) window.draw(arrow_up_sprite);
    if (left_top_view_pos.y + window_height < menu_screen_total_height) window.draw(arrow_down_sprite);
}

std::string MenuScreen::textToString(const sf::Text* text){
    std::string str = text->getString();
    return str.substr(str.find_last_of('.') + 2); // Removes number at the start
}