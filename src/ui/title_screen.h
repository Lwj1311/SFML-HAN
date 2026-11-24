#pragma once

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

namespace han::ui {

struct ChapterEntry {
    std::string id;
    std::string title;
};

/// Main menu: new game, continue, chapter select and quit. It owns only
/// presentation state; the application decides what each action means.
class TitleScreen {
public:
    enum class Action {
        None,
        NewGame,
        Continue,
        ChapterSelected,
        Quit,
    };

    void set_chapters(std::vector<ChapterEntry> chapters);
    void set_has_save(bool value) { has_save_ = value; }
    /// Returns to the main menu, keeping the current menu selection.
    void open();

    Action handle_key(sf::Keyboard::Key key);
    Action handle_click(sf::Vector2f position);
    Action handle_wheel(float delta);

    const std::string& selected_chapter() const { return selected_chapter_; }

    void render(sf::RenderWindow& window, const sf::Font& font);

private:
    enum class Mode {
        Main,
        Chapters,
    };

    Action activate_main();
    Action activate_chapter();

    std::vector<ChapterEntry> chapters_;
    Mode mode_ = Mode::Main;
    std::size_t main_selection_ = 0;
    std::size_t chapter_selection_ = 0;
    std::string selected_chapter_;
    bool has_save_ = false;

    // Rebuilt every frame so click handling matches the latest layout.
    std::vector<sf::FloatRect> main_rects_;
    std::vector<sf::FloatRect> chapter_rects_;
};

} // namespace han::ui
