#ifndef GAME_OF_LIFE_PATTERN_MENU_SCREEN_H
#define GAME_OF_LIFE_PATTERN_MENU_SCREEN_H

#include "screens.h"

class PatternMenuScreen: public MenuScreen{
private:
    // Saves *parent* path of pattern files, so with 'menu_options' we can get the full file path
    std::vector <std::string> menu_options_pattern_paths;
    std::string chosen_file_path;
    std::unordered_set<sf::Vector2i, pair_hash, pair_equal> chosen_pattern;

    static void truncateFileNameIfTooLong(sf::Text& text);
    void iterateOverPatternDirectory();
    sf::Vector2i centerOfMass() const;
    void RLEToGrid();
    void putPatternInGrid();

public:
    PatternMenuScreen();
    short int run() override;
};

#endif
