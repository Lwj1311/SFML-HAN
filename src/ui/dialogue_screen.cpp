#include "dialogue_screen.h"

#include <algorithm>
#include <string>
#include <vector>

namespace han::ui {
namespace {

constexpr unsigned kBodySize = 24;
constexpr unsigned kNameSize = 26;
constexpr unsigned kOptionSize = 22;
constexpr float kLineHeight = 30.f;

const sf::Color kInk(14, 12, 10);
const sf::Color kPanel(22, 19, 15, 228);
const sf::Color kGold(214, 178, 106);
const sf::Color kText(236, 228, 210);
const sf::Color kDim(158, 146, 126);
const sf::Color kSeal(140, 46, 38);

sf::String from_utf8(const std::string& text) {
    return sf::String::fromUtf8(text.begin(), text.end());
}

/// Chinese typesetting keeps closing punctuation on the previous line.
bool no_line_start(sf::Uint32 ch) {
    switch (ch) {
    case L'，':
    case L'。':
    case L'、':
    case L'；':
    case L'：':
    case L'！':
    case L'？':
    case L'）':
    case L'」':
    case L'』':
    case L'》':
    case L'…':
        return true;
    default:
        return false;
    }
}

/// Greedy per-glyph wrapping; measures with font advances so any CJK font works.
std::vector<sf::String> wrap_text(const sf::Font& font, unsigned size,
                                  const sf::String& source, float max_width) {
    std::vector<sf::String> lines;
    sf::String line;
    float width = 0.f;

    for (std::size_t i = 0; i < source.getSize(); ++i) {
        const sf::Uint32 ch = source[i];
        if (ch == L'\n') {
            lines.push_back(line);
            line.clear();
            width = 0.f;
            continue;
        }

        const float advance = font.getGlyph(ch, size, false).advance;
        if (!line.isEmpty() && width + advance > max_width && !no_line_start(ch)) {
            lines.push_back(line);
            line.clear();
            width = 0.f;
        }
        line += ch;
        width += advance;
    }

    if (!line.isEmpty() || lines.empty()) {
        lines.push_back(line);
    }
    return lines;
}

sf::String join_lines(const std::vector<sf::String>& lines) {
    sf::String joined;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) {
            joined += L'\n';
        }
        joined += lines[i];
    }
    return joined;
}

void draw_text(sf::RenderTarget& target, const sf::Font& font, unsigned size,
               const sf::String& text, float x, float y, sf::Color color,
               float line_spacing = 1.f) {
    sf::Text drawable(text, font, size);
    drawable.setFillColor(color);
    drawable.setLineSpacing(line_spacing);
    drawable.setPosition(x, y);
    target.draw(drawable);
}

} // namespace

bool DialogueScreen::initialize(const UiConfig& config, std::string* error) {
    config_ = config;

    std::vector<std::string> candidates;
    if (!config_.font_path.empty()) {
        candidates.push_back(config_.font_path.string());
    }
    candidates.emplace_back("C:/Windows/Fonts/simhei.ttf");
    candidates.emplace_back("C:/Windows/Fonts/msyh.ttc");
    candidates.emplace_back("C:/Windows/Fonts/simsun.ttc");
    candidates.emplace_back("C:/Windows/Fonts/Deng.ttf");

    for (const std::string& path : candidates) {
        if (font_.loadFromFile(path)) {
            return true;
        }
    }

    if (error != nullptr) {
        *error = "无法加载中文字体，请用 --font 指定 TTF 路径（已尝试 simhei/msyh/simsun/Deng）";
    }
    return false;
}

const sf::Texture* DialogueScreen::texture_for(
    std::map<std::string, std::unique_ptr<sf::Texture>>& cache,
    const std::string& folder, const std::string& id, const char* extension) {
    if (id.empty()) {
        return nullptr;
    }

    const std::string key = folder + "/" + id + extension;
    const auto cached = cache.find(key);
    if (cached != cache.end()) {
        return cached->second.get();
    }

    if (missing_assets_.find(key) != missing_assets_.end()) {
        return nullptr;
    }

    auto texture = std::make_unique<sf::Texture>();
    const std::filesystem::path path =
        config_.asset_dir / folder / (id + extension);
    if (!std::filesystem::exists(path) || !texture->loadFromFile(path.string())) {
        missing_assets_.insert(key);
        return nullptr;
    }
    texture->setSmooth(true);
    cache.emplace(key, std::move(texture));
    return cache.at(key).get();
}

const sf::Texture* DialogueScreen::background_texture(const std::string& id) {
    const sf::Texture* texture = texture_for(backgrounds_, "backgrounds", id, ".jpg");
    if (texture == nullptr && id != "bg_station_interior") {
        texture = texture_for(backgrounds_, "backgrounds", "bg_station_interior", ".jpg");
    }
    return texture;
}

const sf::Texture* DialogueScreen::portrait_texture(const std::string& id) {
    return texture_for(portraits_, "characters", id, ".jpg");
}

const sf::Texture* DialogueScreen::ui_texture(const std::string& id) {
    const sf::Texture* texture = texture_for(ui_textures_, "ui", id, ".png");
    if (texture == nullptr) {
        texture = texture_for(ui_textures_, "ui", id, ".jpg");
    }
    return texture;
}

void DialogueScreen::track_node(const narrative::NarrativeSession& session) {
    const std::string background = session.background();
    if (!background.empty()) {
        background_id_ = background;
    }

    if (!session.scene_active()) {
        return;
    }

    const narrative::NodeView& view = session.current();
    if (view.scene_id != last_scene_id_ || view.node_id != last_node_id_) {
        selection_ = 0;
        last_scene_id_ = view.scene_id;
        last_node_id_ = view.node_id;
        node_elapsed_ = 0.f;
        register_scroll_ = 0.f;
        register_user_scrolled_ = false;
    }

    if (view.kind == narrative::NodeKind::Line) {
        portrait_id_ = view.line.portrait;
    } else if (view.kind == narrative::NodeKind::End ||
               view.kind == narrative::NodeKind::Register ||
               view.kind == narrative::NodeKind::Seal) {
        portrait_id_.clear();
    }
}

void DialogueScreen::scroll_register(float delta) {
    register_scroll_ = std::clamp(register_scroll_ + delta, 0.f, register_max_scroll_);
    register_user_scrolled_ = true;
}

void DialogueScreen::render(sf::RenderWindow& window,
                            const narrative::NarrativeSession& session) {
    track_node(session);

    const float dt = std::min(frame_clock_.restart().asSeconds(), 0.1f);
    node_elapsed_ += dt;

    const sf::Vector2u window_size = window.getSize();
    const float width = static_cast<float>(window_size.x);
    const float height = static_cast<float>(window_size.y);

    window.clear(kInk);

    if (const sf::Texture* texture = background_texture(background_id_)) {
        const sf::Vector2f texture_size(static_cast<float>(texture->getSize().x),
                                        static_cast<float>(texture->getSize().y));
        const float scale =
            std::max(width / texture_size.x, height / texture_size.y);
        sf::Sprite sprite(*texture);
        sprite.setScale(scale, scale);
        sprite.setPosition((width - texture_size.x * scale) * 0.5f,
                           (height - texture_size.y * scale) * 0.5f);
        window.draw(sprite);
    }

    sf::RectangleShape shade(sf::Vector2f(width, height));
    shade.setFillColor(sf::Color(10, 8, 6, chapter_finished_ ? 165 : 62));
    window.draw(shade);

    if (card_active_) {
        card_elapsed_ += dt;
        constexpr float kCardDuration = 3.2f;
        if (card_elapsed_ < kCardDuration) {
            render_chapter_card(window, width, height);
            return;
        }
        card_active_ = false;
    }

    if (journal_open_ && session.scene_active()) {
        render_journal(window, session, width, height);
        return;
    }

    if (session.scene_active()) {
        const narrative::NodeView& overlay = session.current();
        if (overlay.kind == narrative::NodeKind::Register) {
            render_register(window, width, height, overlay, dt);
            return;
        }
        if (overlay.kind == narrative::NodeKind::Seal) {
            render_seal(window, width, height, overlay);
            return;
        }
    }

    // Scene label, top left.
    if (session.scene_active()) {
        const narrative::SceneDef* scene =
            session.document().find_scene(session.current().scene_id);
        sf::RectangleShape accent(sf::Vector2f(4.f, 24.f));
        accent.setPosition(36.f, 32.f);
        accent.setFillColor(kGold);
        window.draw(accent);
        draw_text(window, font_, 20,
                  scene != nullptr ? from_utf8(scene->title) : sf::String(),
                  50.f, 29.f, sf::Color(232, 224, 206, 235));
    }

    draw_text(window, font_, 15, sf::String(L"Tab 日志 · Esc 返回主菜单"),
              width - 244.f, 34.f, sf::Color(kDim.r, kDim.g, kDim.b, 190));

    if (chapter_finished_ || !session.scene_active()) {
        sf::RectangleShape panel(sf::Vector2f(540.f, 180.f));
        panel.setPosition((width - 540.f) * 0.5f, (height - 180.f) * 0.5f - 20.f);
        panel.setFillColor(sf::Color(20, 17, 13, 232));
        panel.setOutlineThickness(2.f);
        panel.setOutlineColor(kGold);
        window.draw(panel);

        const std::string title_text = session.chapter_title().empty()
                                           ? std::string("本章结束")
                                           : session.chapter_title() + " · 本章结束";
        const sf::String title = from_utf8(title_text);
        const sf::FloatRect title_bounds = sf::Text(title, font_, 34)
                                               .getLocalBounds();
        draw_text(window, font_, 34, title,
                  width * 0.5f - title_bounds.width * 0.5f,
                  panel.getPosition().y + 52.f, kGold);

        const sf::String hint(L"回车 继续 · Esc 返回主菜单");
        const sf::FloatRect hint_bounds = sf::Text(hint, font_, 18).getLocalBounds();
        draw_text(window, font_, 18, hint,
                  width * 0.5f - hint_bounds.width * 0.5f,
                  panel.getPosition().y + 122.f, kDim);
        return;
    }

    const narrative::NodeView& view = session.current();
    const bool is_choice = view.kind == narrative::NodeKind::Choice;
    const bool is_combat = view.kind == narrative::NodeKind::Combat;

    sf::String body;
    sf::String speaker_name;
    if (is_choice) {
        body = from_utf8(view.prompt.empty() ? std::string("你要怎么做？") : view.prompt);
    } else if (is_combat) {
        std::string encounter_name = view.encounter;
        const auto& encounters = session.document().encounters();
        const auto found = encounters.find(view.encounter);
        if (found != encounters.end()) {
            encounter_name = found->second.value("name", encounter_name);
        }
        body = from_utf8("遭遇：" + encounter_name + "\n（回车迎战 · Backspace 暂避）");
    } else {
        body = from_utf8(view.line.text);
        speaker_name = from_utf8(view.line.speaker_name);
    }

    const float box_x = 40.f;
    const float box_width = width - 80.f;
    const float text_x = box_x + 34.f;
    const float text_width = box_width - 68.f;

    const std::vector<sf::String> body_lines =
        wrap_text(font_, kBodySize, body, text_width);
    const std::size_t option_count = is_choice ? view.choices.size() : 0;
    const float body_height = static_cast<float>(body_lines.size()) * kLineHeight;

    const bool has_name = !is_choice && !speaker_name.isEmpty();
    const float name_height = has_name ? 38.f : 0.f;
    const float options_height = static_cast<float>(option_count) * 40.f;

    float box_height = 20.f + name_height + body_height + options_height + 40.f;
    box_height = std::max(box_height, 190.f);
    const float box_y = height - 40.f - box_height;

    // Portrait sits above the box, overlapping its top edge slightly.
    if (const sf::Texture* texture = portrait_texture(portrait_id_)) {
        const float available = box_y + 24.f - 16.f;
        const float portrait_height = std::min(470.f, available);
        if (portrait_height > 160.f) {
            const float portrait_width = portrait_height * 0.75f;
            sf::Sprite portrait(*texture);
            const sf::Vector2f texture_size(static_cast<float>(texture->getSize().x),
                                            static_cast<float>(texture->getSize().y));
            portrait.setScale(portrait_width / texture_size.x,
                              portrait_height / texture_size.y);
            portrait.setPosition(width - 40.f - portrait_width,
                                 box_y + 24.f - portrait_height);
            window.draw(portrait);
        }
    }

    sf::RectangleShape box(sf::Vector2f(box_width, box_height));
    box.setPosition(box_x, box_y);
    box.setFillColor(kPanel);
    box.setOutlineThickness(2.f);
    box.setOutlineColor(sf::Color(kGold.r, kGold.g, kGold.b, 170));
    window.draw(box);

    sf::RectangleShape inner_line(sf::Vector2f(box_width - 12.f, 1.f));
    inner_line.setPosition(box_x + 6.f, box_y + 6.f);
    inner_line.setFillColor(sf::Color(kGold.r, kGold.g, kGold.b, 60));
    window.draw(inner_line);

    float body_y = box_y + 20.f;
    if (has_name) {
        sf::RectangleShape seal(sf::Vector2f(6.f, 28.f));
        seal.setPosition(box_x + 16.f, box_y + 18.f);
        seal.setFillColor(kSeal);
        window.draw(seal);
        draw_text(window, font_, kNameSize, speaker_name,
                  box_x + 32.f, box_y + 14.f, kGold);
        body_y += name_height;
    }

    draw_text(window, font_, kBodySize, join_lines(body_lines), text_x, body_y,
              kText, kLineHeight / static_cast<float>(kBodySize));

    option_rects_.clear();
    if (is_choice) {
        float option_y = body_y + body_height + 8.f;
        for (std::size_t i = 0; i < view.choices.size(); ++i) {
            const sf::FloatRect row(box_x + 24.f, option_y, box_width - 48.f, 36.f);
            option_rects_.push_back(row);

            if (i == selection_) {
                sf::RectangleShape highlight(sf::Vector2f(row.width, row.height));
                highlight.setPosition(row.left, row.top);
                highlight.setFillColor(sf::Color(kGold.r, kGold.g, kGold.b, 38));
                window.draw(highlight);

                sf::RectangleShape marker(sf::Vector2f(4.f, row.height));
                marker.setPosition(row.left, row.top);
                marker.setFillColor(kGold);
                window.draw(marker);
            }

            const sf::String label =
                sf::String(std::to_string(i + 1)) + L"   " +
                from_utf8(view.choices[i].text);
            draw_text(window, font_, kOptionSize, label, row.left + 16.f,
                      option_y + 4.f,
                      i == selection_ ? kGold : sf::Color(kText.r, kText.g, kText.b, 220));
            option_y += 40.f;
        }
    }

    sf::String hint;
    if (is_choice) {
        hint = L"↑↓ 选择 · 数字键直选 · 回车确认";
    } else if (is_combat) {
        hint = L"回车 迎战 · Backspace 暂避";
    } else {
        hint = L"空格 / 回车 / 左键  继续";
    }
    const sf::FloatRect hint_bounds = sf::Text(hint, font_, 15).getLocalBounds();
    draw_text(window, font_, 15, hint,
              box_x + box_width - 26.f - hint_bounds.width,
              box_y + box_height - 30.f, kDim);
}

void DialogueScreen::render_register(sf::RenderWindow& window, float width,
                                     float height,
                                     const narrative::NodeView& view, float dt) {
    const narrative::RegisterView& reg = view.register_view;

    sf::RectangleShape veil(sf::Vector2f(width, height));
    veil.setFillColor(sf::Color(8, 7, 5, 224));
    window.draw(veil);

    const float viewport_w = std::min(620.f, width - 120.f);
    const float viewport_h = std::max(240.f, height - 194.f);
    const float vx = (width - viewport_w) * 0.5f;
    const float vy = 104.f;

    const float row_h = 44.f;
    const float top_pad = 128.f;
    const float bottom_pad = 132.f;
    const float content_h =
        std::max(viewport_w * 8.f / 3.f,
                 top_pad + row_h * static_cast<float>(reg.names.size()) + bottom_pad);

    register_viewport_height_ = viewport_h;
    register_max_scroll_ = std::max(0.f, content_h - viewport_h);

    const std::size_t revealed =
        std::min(reg.names.size(),
                 static_cast<std::size_t>(node_elapsed_ / 0.055f) + 1);
    if (!register_user_scrolled_ && register_max_scroll_ > 0.f && revealed > 0) {
        const float focus = top_pad + row_h * static_cast<float>(revealed - 1);
        const float target =
            std::clamp(focus - viewport_h * 0.62f, 0.f, register_max_scroll_);
        register_scroll_ += (target - register_scroll_) * std::min(1.f, 3.5f * dt);
    }
    register_scroll_ = std::clamp(register_scroll_, 0.f, register_max_scroll_);

    // Header above the scroll.
    const sf::String title =
        from_utf8(reg.title.empty() ? std::string("名籍") : reg.title);
    const sf::FloatRect title_bounds = sf::Text(title, font_, 30).getLocalBounds();
    draw_text(window, font_, 30, title, (width - title_bounds.width) * 0.5f, 34.f, kGold);

    sf::String subtitle = from_utf8(reg.subtitle);
    if (subtitle.isEmpty()) {
        subtitle = sf::String(L"共 ") + sf::String(std::to_string(reg.names.size())) +
                   sf::String(L" 人");
    }
    const sf::FloatRect subtitle_bounds = sf::Text(subtitle, font_, 16).getLocalBounds();
    draw_text(window, font_, 16, subtitle, (width - subtitle_bounds.width) * 0.5f,
              74.f, kDim);

    // Thin frame around the scrolling viewport.
    sf::RectangleShape frame(sf::Vector2f(viewport_w + 4.f, viewport_h + 4.f));
    frame.setPosition(vx - 2.f, vy - 2.f);
    frame.setFillColor(sf::Color::Transparent);
    frame.setOutlineThickness(1.f);
    frame.setOutlineColor(sf::Color(kGold.r, kGold.g, kGold.b, 90));
    window.draw(frame);

    const sf::View previous = window.getView();
    sf::View scroll_view(sf::FloatRect(0.f, 0.f, viewport_w, viewport_h));
    scroll_view.setViewport(sf::FloatRect(vx / width, vy / height,
                                          viewport_w / width, viewport_h / height));
    window.setView(scroll_view);

    if (const sf::Texture* texture = ui_texture("ui_register_scroll")) {
        const sf::Vector2f texture_size(static_cast<float>(texture->getSize().x),
                                        static_cast<float>(texture->getSize().y));
        const float draw_w = content_h * (texture_size.x / texture_size.y);
        sf::Sprite scroll(*texture);
        scroll.setScale(draw_w / texture_size.x, content_h / texture_size.y);
        scroll.setPosition((viewport_w - draw_w) * 0.5f, -register_scroll_);
        window.draw(scroll);
    } else {
        sf::RectangleShape paper(sf::Vector2f(viewport_w, content_h));
        paper.setPosition(0.f, -register_scroll_);
        paper.setFillColor(sf::Color(226, 208, 170));
        window.draw(paper);
    }

    if (reg.names.empty()) {
        const sf::String empty = from_utf8("名籍未录");
        const sf::FloatRect bounds = sf::Text(empty, font_, 22).getLocalBounds();
        draw_text(window, font_, 22, empty, (viewport_w - bounds.width) * 0.5f,
                  viewport_h * 0.5f - 16.f, sf::Color(58, 46, 34, 170));
    }

    for (std::size_t i = 0; i < reg.names.size(); ++i) {
        const float y = top_pad + row_h * static_cast<float>(i) - register_scroll_;
        if (y < -row_h || y > viewport_h + row_h) {
            continue;
        }
        const float reveal = std::clamp(
            (node_elapsed_ - 0.055f * static_cast<float>(i)) / 0.3f, 0.f, 1.f);
        if (reveal <= 0.f) {
            continue;
        }
        const sf::Uint8 alpha = static_cast<sf::Uint8>(255.f * reveal);
        const sf::String name = from_utf8(reg.names[i]);
        const sf::FloatRect bounds = sf::Text(name, font_, 23).getLocalBounds();
        draw_text(window, font_, 23, name, (viewport_w - bounds.width) * 0.5f,
                  y + (1.f - reveal) * 10.f, sf::Color(58, 46, 34, alpha));

        sf::RectangleShape rule(sf::Vector2f(viewport_w * 0.56f, 1.f));
        rule.setPosition(viewport_w * 0.22f, y + row_h - 7.f);
        rule.setFillColor(sf::Color(90, 68, 46, static_cast<sf::Uint8>(70.f * reveal)));
        window.draw(rule);
    }

    window.setView(previous);

    // Scrollbar on the right edge of the viewport.
    if (register_max_scroll_ > 0.f) {
        const float track_x = vx + viewport_w + 16.f;
        sf::RectangleShape track(sf::Vector2f(3.f, viewport_h));
        track.setPosition(track_x, vy);
        track.setFillColor(sf::Color(214, 178, 106, 45));
        window.draw(track);

        const float handle_h = std::max(
            42.f, viewport_h * viewport_h / (viewport_h + register_max_scroll_));
        const float progress = register_scroll_ / register_max_scroll_;
        sf::RectangleShape handle(sf::Vector2f(3.f, handle_h));
        handle.setPosition(track_x, vy + progress * (viewport_h - handle_h));
        handle.setFillColor(sf::Color(kGold.r, kGold.g, kGold.b, 190));
        window.draw(handle);
    }

    const sf::String hint = from_utf8("滚轮 / ↑↓ 翻卷 · 空格 / 回车 继续");
    const sf::FloatRect hint_bounds = sf::Text(hint, font_, 17).getLocalBounds();
    draw_text(window, font_, 17, hint, (width - hint_bounds.width) * 0.5f,
              height - 34.f, kDim);

    const sf::String back = from_utf8("Esc 返回主菜单");
    const sf::FloatRect back_bounds = sf::Text(back, font_, 15).getLocalBounds();
    draw_text(window, font_, 15, back, width - 28.f - back_bounds.width, 24.f, kDim);
}

void DialogueScreen::render_seal(sf::RenderWindow& window, float width, float height,
                                 const narrative::NodeView& view) {
    const narrative::SealView& seal = view.seal_view;

    sf::RectangleShape veil(sf::Vector2f(width, height));
    veil.setFillColor(sf::Color(8, 7, 5, 234));
    window.draw(veil);

    const float panel_w = std::min(760.f, width - 160.f);
    const float panel_h = std::min(440.f, height - 140.f);
    const float px = (width - panel_w) * 0.5f;
    const float py = (height - panel_h) * 0.5f;

    sf::RectangleShape panel(sf::Vector2f(panel_w, panel_h));
    panel.setPosition(px, py);
    panel.setFillColor(sf::Color(22, 19, 15, 240));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(kGold.r, kGold.g, kGold.b, 150));
    window.draw(panel);

    float body_top = py + 44.f;
    if (!seal.title.empty()) {
        const sf::String title = from_utf8(seal.title);
        const sf::FloatRect bounds = sf::Text(title, font_, 27).getLocalBounds();
        draw_text(window, font_, 27, title, px + (panel_w - bounds.width) * 0.5f,
                  py + 34.f, kGold);
        body_top = py + 94.f;
    }

    const std::vector<sf::String> lines =
        wrap_text(font_, 23, from_utf8(seal.text), panel_w - 150.f);
    const float line_h = 34.f;
    const float body_h = line_h * static_cast<float>(lines.size());
    const float stamp_space = 168.f;
    const float body_y =
        body_top + std::max(0.f, (panel_h - (body_top - py) - stamp_space - body_h) * 0.5f);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const sf::FloatRect bounds = sf::Text(lines[i], font_, 23).getLocalBounds();
        draw_text(window, font_, 23, lines[i], px + (panel_w - bounds.width) * 0.5f,
                  body_y + line_h * static_cast<float>(i), kText);
    }

    // Stamp slams down from above, settling at its final size.
    const float t = std::clamp(node_elapsed_ / 0.55f, 0.f, 1.f);
    const float eased = 1.f - (1.f - t) * (1.f - t) * (1.f - t);
    const float scale = 2.1f - 1.1f * eased;
    const sf::Uint8 alpha = static_cast<sf::Uint8>(255.f * std::min(1.f, t * 2.5f));
    if (const sf::Texture* texture = ui_texture(seal.seal_id)) {
        const sf::Vector2f texture_size(static_cast<float>(texture->getSize().x),
                                        static_cast<float>(texture->getSize().y));
        const float base = 150.f;
        sf::Sprite stamp(*texture);
        stamp.setOrigin(texture_size * 0.5f);
        stamp.setPosition(px + panel_w * 0.5f, py + panel_h - 82.f);
        stamp.setScale((base / texture_size.x) * scale,
                       (base / texture_size.y) * scale);
        stamp.setRotation(-9.f * (1.f - eased));
        stamp.setColor(sf::Color(255, 255, 255, alpha));
        window.draw(stamp);
    }

    const sf::String hint = from_utf8("空格 / 回车 / 左键 继续");
    const sf::FloatRect hint_bounds = sf::Text(hint, font_, 16).getLocalBounds();
    draw_text(window, font_, 16, hint, px + (panel_w - hint_bounds.width) * 0.5f,
              py + panel_h + 22.f, kDim);
}

void DialogueScreen::show_chapter_card(const std::string& title) {
    card_active_ = true;
    card_elapsed_ = 0.f;
    card_title_ = title;
    journal_open_ = false;
}

void DialogueScreen::render_chapter_card(sf::RenderWindow& window, float width,
                                         float height) {
    sf::RectangleShape veil(sf::Vector2f(width, height));
    veil.setFillColor(sf::Color(8, 7, 5, 214));
    window.draw(veil);

    const float fade = std::clamp(card_elapsed_ / 0.5f, 0.f, 1.f);
    const auto alpha = [fade](sf::Color color, float factor = 1.f) {
        return sf::Color(color.r, color.g, color.b,
                         static_cast<sf::Uint8>(
                             static_cast<float>(color.a) * fade * factor));
    };

    const sf::String prelude = from_utf8("悬泉渡");
    const sf::FloatRect prelude_bounds = sf::Text(prelude, font_, 20).getLocalBounds();
    draw_text(window, font_, 20, prelude, (width - prelude_bounds.width) * 0.5f,
              height * 0.32f, alpha(kGold));

    const sf::String title = from_utf8(card_title_.empty() ? "本章" : card_title_);
    const sf::FloatRect title_bounds = sf::Text(title, font_, 52).getLocalBounds();
    draw_text(window, font_, 52, title, (width - title_bounds.width) * 0.5f,
              height * 0.40f, alpha(kText));

    sf::RectangleShape rule(sf::Vector2f(260.f, 1.f));
    rule.setPosition((width - 260.f) * 0.5f, height * 0.40f + 82.f);
    rule.setFillColor(alpha(sf::Color(kGold.r, kGold.g, kGold.b, 170)));
    window.draw(rule);

    const sf::String hint = from_utf8("任意键继续");
    const sf::FloatRect hint_bounds = sf::Text(hint, font_, 16).getLocalBounds();
    draw_text(window, font_, 16, hint, (width - hint_bounds.width) * 0.5f,
              height * 0.40f + 104.f, alpha(sf::Color(kDim.r, kDim.g, kDim.b, 220)));
}

void DialogueScreen::render_journal(sf::RenderWindow& window,
                                    const narrative::NarrativeSession& session,
                                    float width, float height) {
    struct JournalRow {
        sf::String text;
        unsigned size = 16;
        sf::Color color = kText;
        float indent = 0.f;
    };

    const float panel_x = 48.f;
    const float panel_y = 44.f;
    const float panel_w = width - panel_x * 2.f;
    const float panel_h = height - panel_y * 2.f;
    const float inner_pad = 28.f;
    const float col_gap = 28.f;
    const float col_w = (panel_w - inner_pad * 2.f - col_gap * 2.f) / 3.f;
    const float content_top = panel_y + 112.f;
    const float content_bottom = panel_y + panel_h - 44.f;
    const float viewport_h = content_bottom - content_top;
    journal_viewport_height_ = viewport_h;

    std::vector<JournalRow> quest_rows;
    std::vector<JournalRow> clue_rows;
    std::vector<JournalRow> item_rows;

    const auto add_wrapped = [&](std::vector<JournalRow>& rows, const std::string& text,
                                 unsigned size, sf::Color color, float indent) {
        const std::vector<sf::String> lines =
            wrap_text(font_, size, from_utf8(text), col_w - indent);
        for (const sf::String& line : lines) {
            rows.push_back(JournalRow{line, size, color, indent});
        }
    };

    const narrative::NarrativeState& state = session.state();
    for (const narrative::QuestDef& quest : session.quests().definitions()) {
        const narrative::QuestStatus& status = state.quest_status(quest.id);
        if (status.state == narrative::QuestState::Inactive) {
            continue;
        }
        const bool done = status.state == narrative::QuestState::Completed;
        add_wrapped(quest_rows,
                    std::string(done ? "〔已了〕" : "〔进行〕") + quest.title, 20,
                    done ? kDim : kGold, 0.f);
        for (const narrative::ObjectiveDef& objective : quest.objectives) {
            const bool objective_done =
                status.objectives_done.find(objective.id) != status.objectives_done.end();
            std::string line =
                std::string(objective_done ? "√ " : "· ") + objective.text;
            if (objective.optional) {
                line += "（可选）";
            }
            add_wrapped(quest_rows, line, 16, objective_done ? kDim : kText, 12.f);
        }
        quest_rows.push_back(JournalRow{sf::String(), 10, kDim, 0.f});
    }

    for (const std::string& clue : state.clues()) {
        add_wrapped(clue_rows, "· " + session.clue_text(clue), 16, kText, 0.f);
    }

    for (const auto& [item, count] : state.items()) {
        std::string name = session.item_name(item);
        if (count > 1) {
            name += " ×" + std::to_string(count);
        }
        add_wrapped(item_rows, name, 18, kGold, 0.f);
        const std::string description = session.item_description(item);
        if (!description.empty()) {
            add_wrapped(item_rows, description, 14, kDim, 12.f);
        }
        item_rows.push_back(JournalRow{sf::String(), 8, kDim, 0.f});
    }

    const auto rows_height = [](const std::vector<JournalRow>& rows) {
        float total = 0.f;
        for (const JournalRow& row : rows) {
            total += static_cast<float>(row.size) * 1.5f;
        }
        return total;
    };
    const float tallest = std::max({rows_height(quest_rows), rows_height(clue_rows),
                                    rows_height(item_rows)});
    journal_max_scroll_ = std::max(0.f, tallest + 8.f - viewport_h);
    journal_scroll_ = std::clamp(journal_scroll_, 0.f, journal_max_scroll_);

    sf::RectangleShape veil(sf::Vector2f(width, height));
    veil.setFillColor(sf::Color(6, 5, 4, 202));
    window.draw(veil);

    sf::RectangleShape panel(sf::Vector2f(panel_w, panel_h));
    panel.setPosition(panel_x, panel_y);
    panel.setFillColor(sf::Color(22, 19, 15, 246));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(kGold.r, kGold.g, kGold.b, 130));
    window.draw(panel);

    draw_text(window, font_, 28, from_utf8("案牍"), panel_x + inner_pad, panel_y + 24.f,
              kGold);
    draw_text(window, font_, 15, from_utf8("任务 · 线索 · 物品"),
              panel_x + inner_pad + 76.f, panel_y + 36.f, kDim);
    draw_text(window, font_, 15, from_utf8("Tab / Esc 关闭"),
              panel_x + panel_w - inner_pad - 112.f, panel_y + 36.f, kDim);

    const char* headings[] = {"任务", "线索", "物品"};
    const std::vector<JournalRow>* columns[] = {&quest_rows, &clue_rows, &item_rows};

    for (int column = 0; column < 3; ++column) {
        const float cx =
            panel_x + inner_pad + static_cast<float>(column) * (col_w + col_gap);

        draw_text(window, font_, 19, from_utf8(headings[column]), cx, panel_y + 72.f,
                  kGold);
        sf::RectangleShape rule(sf::Vector2f(col_w, 1.f));
        rule.setPosition(cx, content_top - 8.f);
        rule.setFillColor(sf::Color(kGold.r, kGold.g, kGold.b, 90));
        window.draw(rule);

        const sf::View previous = window.getView();
        sf::View view(sf::FloatRect(0.f, 0.f, col_w, viewport_h));
        view.setViewport(sf::FloatRect(cx / width, content_top / height, col_w / width,
                                       viewport_h / height));
        window.setView(view);

        if (columns[column]->empty()) {
            draw_text(window, font_, 16, from_utf8("（暂无）"), 0.f, 4.f,
                      sf::Color(kDim.r, kDim.g, kDim.b, 150));
        }

        float y = -journal_scroll_;
        for (const JournalRow& row : *columns[column]) {
            const float line_h = static_cast<float>(row.size) * 1.5f;
            if (y + line_h > 0.f && y < viewport_h) {
                draw_text(window, font_, row.size, row.text, row.indent, y, row.color);
            }
            y += line_h;
        }
        window.setView(previous);
    }

    if (journal_max_scroll_ > 0.f) {
        const float track_x = panel_x + panel_w - 22.f;
        sf::RectangleShape track(sf::Vector2f(3.f, viewport_h));
        track.setPosition(track_x, content_top);
        track.setFillColor(sf::Color(kGold.r, kGold.g, kGold.b, 45));
        window.draw(track);

        const float handle_h =
            std::max(48.f, viewport_h * viewport_h / (viewport_h + journal_max_scroll_));
        const float progress = journal_scroll_ / journal_max_scroll_;
        sf::RectangleShape handle(sf::Vector2f(3.f, handle_h));
        handle.setPosition(track_x, content_top + progress * (viewport_h - handle_h));
        handle.setFillColor(sf::Color(kGold.r, kGold.g, kGold.b, 190));
        window.draw(handle);
    }

    const sf::String hint = from_utf8("↑↓ / 滚轮 翻阅 · Tab / Esc 关闭");
    const sf::FloatRect hint_bounds = sf::Text(hint, font_, 15).getLocalBounds();
    draw_text(window, font_, 15, hint, (width - hint_bounds.width) * 0.5f,
              panel_y + panel_h - 30.f, kDim);
}

bool DialogueScreen::handle_key(sf::Keyboard::Key key,
                                narrative::NarrativeSession& session) {
    if (card_active_) {
        dismiss_chapter_card();
        return false;
    }

    if (journal_open_) {
        const float page = journal_viewport_height_ > 0.f
                               ? journal_viewport_height_ * 0.8f
                               : 320.f;
        if (key == sf::Keyboard::Up || key == sf::Keyboard::W || key == sf::Keyboard::K) {
            journal_scroll_ = std::max(0.f, journal_scroll_ - 48.f);
        } else if (key == sf::Keyboard::Down || key == sf::Keyboard::S ||
                   key == sf::Keyboard::J) {
            journal_scroll_ = std::min(journal_max_scroll_, journal_scroll_ + 48.f);
        } else if (key == sf::Keyboard::PageUp) {
            journal_scroll_ = std::max(0.f, journal_scroll_ - page);
        } else if (key == sf::Keyboard::PageDown) {
            journal_scroll_ = std::min(journal_max_scroll_, journal_scroll_ + page);
        } else if (key == sf::Keyboard::Home) {
            journal_scroll_ = 0.f;
        } else if (key == sf::Keyboard::End) {
            journal_scroll_ = journal_max_scroll_;
        } else if (key == sf::Keyboard::Tab || key == sf::Keyboard::Escape ||
                   key == sf::Keyboard::BackSpace) {
            journal_open_ = false;
        }
        return false;
    }

    if (key == sf::Keyboard::Escape) {
        return true;
    }

    if (key == sf::Keyboard::Tab && session.scene_active() && !chapter_finished_) {
        journal_open_ = true;
        journal_scroll_ = 0.f;
        return false;
    }

    if (chapter_finished_ || !session.scene_active()) {
        return false;
    }

    const narrative::NodeView& view = session.current();
    std::string error;

    if (view.kind == narrative::NodeKind::Line) {
        if (key == sf::Keyboard::Space || key == sf::Keyboard::Enter ||
            key == sf::Keyboard::Return) {
            session.advance();
        }
        return false;
    }

    if (view.kind == narrative::NodeKind::Choice) {
        const std::size_t count = view.choices.size();
        if (count == 0) {
            return false;
        }

        if (key == sf::Keyboard::Up || key == sf::Keyboard::W ||
            key == sf::Keyboard::K) {
            selection_ = (selection_ + count - 1) % count;
        } else if (key == sf::Keyboard::Down || key == sf::Keyboard::S ||
                   key == sf::Keyboard::J) {
            selection_ = (selection_ + 1) % count;
        } else if (key == sf::Keyboard::Enter || key == sf::Keyboard::Return ||
                   key == sf::Keyboard::Space) {
            session.choose(view.choices[selection_].id, &error);
        } else {
            static const sf::Keyboard::Key kDigits[] = {
                sf::Keyboard::Num1, sf::Keyboard::Num2, sf::Keyboard::Num3,
                sf::Keyboard::Num4, sf::Keyboard::Num5, sf::Keyboard::Num6,
                sf::Keyboard::Num7, sf::Keyboard::Num8, sf::Keyboard::Num9,
            };
            for (std::size_t i = 0; i < 9 && i < count; ++i) {
                if (key == kDigits[i]) {
                    selection_ = i;
                    session.choose(view.choices[i].id, &error);
                    break;
                }
            }
        }
        return false;
    }

    if (view.kind == narrative::NodeKind::Register) {
        const float page = register_viewport_height_ > 0.f
                               ? register_viewport_height_ * 0.8f
                               : 400.f;
        if (key == sf::Keyboard::Up || key == sf::Keyboard::W ||
            key == sf::Keyboard::K) {
            scroll_register(-70.f);
        } else if (key == sf::Keyboard::Down || key == sf::Keyboard::S ||
                   key == sf::Keyboard::J) {
            scroll_register(70.f);
        } else if (key == sf::Keyboard::PageUp) {
            scroll_register(-page);
        } else if (key == sf::Keyboard::PageDown) {
            scroll_register(page);
        } else if (key == sf::Keyboard::Home) {
            register_scroll_ = 0.f;
            register_user_scrolled_ = true;
        } else if (key == sf::Keyboard::End) {
            register_scroll_ = register_max_scroll_;
            register_user_scrolled_ = true;
        } else if (key == sf::Keyboard::Space || key == sf::Keyboard::Enter ||
                   key == sf::Keyboard::Return) {
            session.advance();
        }
        return false;
    }

    if (view.kind == narrative::NodeKind::Seal) {
        if (key == sf::Keyboard::Space || key == sf::Keyboard::Enter ||
            key == sf::Keyboard::Return) {
            session.advance();
        }
        return false;
    }

    if (view.kind == narrative::NodeKind::Combat) {
        if (key == sf::Keyboard::Enter || key == sf::Keyboard::Return ||
            key == sf::Keyboard::Space) {
            session.resolve_combat(true);
        } else if (key == sf::Keyboard::BackSpace) {
            session.resolve_combat(false);
        }
    }
    return false;
}

bool DialogueScreen::handle_click(sf::Vector2f position,
                                  narrative::NarrativeSession& session) {
    if (card_active_) {
        dismiss_chapter_card();
        return true;
    }
    if (journal_open_) {
        return true;
    }
    if (chapter_finished_ || !session.scene_active()) {
        return false;
    }

    const narrative::NodeView& view = session.current();
    if (view.kind == narrative::NodeKind::Line) {
        session.advance();
        return true;
    }

    if (view.kind == narrative::NodeKind::Register ||
        view.kind == narrative::NodeKind::Seal) {
        session.advance();
        return true;
    }

    if (view.kind == narrative::NodeKind::Choice) {
        for (std::size_t i = 0; i < option_rects_.size() && i < view.choices.size(); ++i) {
            if (option_rects_[i].contains(position.x, position.y)) {
                selection_ = i;
                std::string error;
                session.choose(view.choices[i].id, &error);
                return true;
            }
        }
    }
    return false;
}

bool DialogueScreen::handle_wheel(float delta, narrative::NarrativeSession& session) {
    if (journal_open_) {
        journal_scroll_ =
            std::clamp(journal_scroll_ - delta * 60.f, 0.f, journal_max_scroll_);
        return true;
    }
    if (chapter_finished_ || !session.scene_active()) {
        return false;
    }
    if (session.current().kind != narrative::NodeKind::Register) {
        return false;
    }
    scroll_register(-delta * 60.f);
    return true;
}

} // namespace han::ui
