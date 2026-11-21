#pragma once

#include "narrative/session.h"

#include <SFML/Graphics.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace han::ui {

struct UiConfig {
    /// Root of game-assets (backgrounds/, characters/).
    std::filesystem::path asset_dir;
    std::filesystem::path font_path;
};

/// Renders the 2D dialogue layer for one chapter and converts player input
/// into narrative actions. It owns no narrative state: the session stays the
/// single source of truth and is driven from the outside.
class DialogueScreen {
public:
    bool initialize(const UiConfig& config, std::string* error = nullptr);

    void render(sf::RenderWindow& window, const narrative::NarrativeSession& session);

    /// Handles a pressed key. Returns true when the player asked to quit.
    bool handle_key(sf::Keyboard::Key key, narrative::NarrativeSession& session);
    /// Handles a left click in window coordinates. Returns true when consumed.
    bool handle_click(sf::Vector2f position, narrative::NarrativeSession& session);
    /// Handles a vertical mouse wheel notch. Returns true when consumed.
    bool handle_wheel(float delta, narrative::NarrativeSession& session);

    bool chapter_finished() const { return chapter_finished_; }
    void set_chapter_finished(bool value) { chapter_finished_ = value; }

    /// Clears per-chapter presentation state when switching chapters.
    void reset_chapter() {
        background_id_.clear();
        portrait_id_.clear();
        last_scene_id_.clear();
        last_node_id_.clear();
        selection_ = 0;
        chapter_finished_ = false;
        option_rects_.clear();
        node_elapsed_ = 0.f;
        register_scroll_ = 0.f;
        register_max_scroll_ = 0.f;
        register_user_scrolled_ = false;
    }

private:
    const sf::Texture* texture_for(std::map<std::string, std::unique_ptr<sf::Texture>>& cache,
                                   const std::string& folder,
                                   const std::string& id,
                                   const char* extension);
    const sf::Texture* background_texture(const std::string& id);
    const sf::Texture* portrait_texture(const std::string& id);
    const sf::Texture* ui_texture(const std::string& id);
    void scroll_register(float delta);

    void track_node(const narrative::NarrativeSession& session);
    void render_register(sf::RenderWindow& window, float width, float height,
                         const narrative::NodeView& view, float dt);
    void render_seal(sf::RenderWindow& window, float width, float height,
                     const narrative::NodeView& view);

    UiConfig config_;
    sf::Font font_;

    std::map<std::string, std::unique_ptr<sf::Texture>> backgrounds_;
    std::map<std::string, std::unique_ptr<sf::Texture>> portraits_;
    std::map<std::string, std::unique_ptr<sf::Texture>> ui_textures_;
    std::set<std::string> missing_assets_;

    std::string background_id_;
    std::string portrait_id_;
    std::string last_scene_id_;
    std::string last_node_id_;

    std::size_t selection_ = 0;
    bool chapter_finished_ = false;

    // Rebuilt every frame so click handling can use the latest layout.
    std::vector<sf::FloatRect> option_rects_;

    // Presentation timing for the register and seal overlays.
    sf::Clock frame_clock_;
    float node_elapsed_ = 0.f;
    float register_scroll_ = 0.f;
    float register_max_scroll_ = 0.f;
    float register_viewport_height_ = 0.f;
    bool register_user_scrolled_ = false;
};

} // namespace han::ui
