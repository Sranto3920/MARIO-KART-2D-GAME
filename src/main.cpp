#include <cstdint>
#include <string>
#include <cstring> // For memcpy, memmove

namespace sf {
    using Uint32 = uint32_t;
}

namespace std {
    template <>
    struct char_traits<sf::Uint32> {
        using char_type = sf::Uint32;
        using int_type = int;

        static void assign(char_type& c1, const char_type& c2) { c1 = c2; }
        static bool eq(char_type c1, char_type c2) { return c1 == c2; }
        static bool lt(char_type c1, char_type c2) { return c1 < c2; }
        static int compare(const char_type* s1, const char_type* s2, size_t n) {
            for (size_t i = 0; i < n; ++i) {
                if (lt(s1[i], s2[i])) return -1;
                if (lt(s2[i], s1[i])) return 1;
            }
            return 0;
        }
        static size_t length(const char_type* s) {
            size_t len = 0;
            while (s[len] != 0) ++len;
            return len;
        }
        static const char_type* find(const char_type* s, size_t n, const char_type& a) {
            for (size_t i = 0; i < n; ++i) if (eq(s[i], a)) return s + i;
            return nullptr;
        }
        static char_type* move(char_type* s1, const char_type* s2, size_t n) {
            return static_cast<char_type*>(memmove(s1, s2, n * sizeof(char_type)));
        }
        static char_type* copy(char_type* s1, const char_type* s2, size_t n) {
            return static_cast<char_type*>(memcpy(s1, s2, n * sizeof(char_type)));
        }
        static char_type* assign(char_type* s, size_t n, char_type a) {
            for (size_t i = 0; i < n; ++i) s[i] = a;
            return s;
        }
        static int_type eof() { return -1; }
        static char_type to_char_type(int_type c) { return static_cast<char_type>(c); }
        static int_type to_int_type(char_type c) { return static_cast<int_type>(c); }
        static bool eq_int_type(int_type c1, int_type c2) { return c1 == c2; }
        static int_type not_eof(int_type c) { return c != eof() ? c : 0; }
    };
}

#include <SFML/Graphics.hpp>
#include <vector>
#include <random>

const int WINDOW_WIDTH = 900;
const int WINDOW_HEIGHT = 600;
const int GROUND_Y = 500;
const int MAX_LIFE = 3;
const float GRAVITY = 0.5f;

enum class EntityType { Coin, Obstacle, Heart, Platform, Cloud };

struct GameEntity {
    sf::Sprite sprite;
    sf::Vector2f velocity;
    EntityType type;
    bool collected = false;
};

sf::Texture loadTexture(const std::string& filePath) {
    sf::Texture texture;
    if (!texture.loadFromFile(filePath)) {
        throw std::runtime_error("Failed to load texture: " + filePath);
    }
    return texture;
}

bool isTooClose(const sf::Vector2f& pos, const std::vector<GameEntity>& entities, float minDist) {
    for (const auto& e : entities) {
        if (!e.collected) {
            float dx = pos.x - e.sprite.getPosition().x;
            float dy = pos.y - e.sprite.getPosition().y;
            if (std::sqrt(dx*dx + dy*dy) < minDist) return true;
        }
    }
    return false;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Mario Jump & Dodge");
    window.setFramerateLimit(60);

    std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
    std::uniform_real_distribution<float> cloudYDist(50, 200);
    std::uniform_real_distribution<float> platformYDist(250, GROUND_Y - 80);

    // Load Textures
    sf::Texture playerTexture = loadTexture("assets/player.png");
    sf::Texture coinTexture = loadTexture("assets/coin.png");
    sf::Texture heartTexture = loadTexture("assets/heart.png");
    sf::Texture obstacleTexture = loadTexture("assets/obstacle.png");
    sf::Texture cloudTexture = loadTexture("assets/cloud.png");
    sf::Texture platformTexture = loadTexture("assets/platform.png");

    // Background ground
    sf::RectangleShape ground(sf::Vector2f(WINDOW_WIDTH, 100));
    ground.setFillColor(sf::Color(150, 75, 0));
    ground.setPosition(0, GROUND_Y);

    // Player setup
    GameEntity player;
    player.sprite.setTexture(playerTexture);
    player.sprite.setPosition(100, GROUND_Y - player.sprite.getGlobalBounds().height);
    player.velocity.y = 0;
    player.type = EntityType::Obstacle; // Not used for player
    bool isJumping = false;
    bool onPlatform = false;

    // Score, life, coins
    int coinsCollected = 0;
    int life = MAX_LIFE;

    sf::Font font;
    if (!font.loadFromFile("assets/arial.ttf")) {
        throw std::runtime_error("Failed to load font!");
    }
    sf::Text scoreText("Time: 0", font, 24);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 10);

    sf::Text coinText("Coins: 0", font, 20);
    coinText.setFillColor(sf::Color::Yellow);
    coinText.setPosition(10, 40);

    // Heart indicators
    std::vector<sf::Sprite> hearts;
    for (int i = 0; i < MAX_LIFE; ++i) {
        sf::Sprite heart;
        heart.setTexture(heartTexture);
        heart.setPosition(700 + i * 30, 10);
        hearts.push_back(heart);
    }

    // Entities
    std::vector<GameEntity> entities;
    std::vector<GameEntity> clouds;
    std::vector<GameEntity> platforms;

    // Initial clouds
    for (int i = 0; i < 3; ++i) {
        GameEntity cloud;
        cloud.sprite.setTexture(cloudTexture);
        cloud.sprite.setPosition(200 + i * 250, cloudYDist(rng));
        cloud.velocity = sf::Vector2f(-1.5f, 0);
        cloud.type = EntityType::Cloud;
        clouds.push_back(cloud);
    }

    // Initial platforms
    for (int i = 0; i < 2; ++i) {
        GameEntity platform;
        platform.sprite.setTexture(platformTexture);
        platform.sprite.setPosition(400 + i * 250, platformYDist(rng));
        platform.velocity = sf::Vector2f(-3.0f, 0);
        platform.type = EntityType::Platform;
        platforms.push_back(platform);
    }

    sf::Clock coinClock, obstacleClock, heartClock, platformClock, gameClock;

    // --- Start/Restart Button Setup ---
    bool gameStarted = false;
    bool gameOver = false;

    sf::Text startText("Welcome! Click START to Play", font, 36);
    startText.setFillColor(sf::Color::White);
    startText.setPosition(WINDOW_WIDTH / 2 - startText.getGlobalBounds().width / 2, WINDOW_HEIGHT / 2 - 100);

    sf::RectangleShape startButton(sf::Vector2f(300, 60));
    startButton.setFillColor(sf::Color(70, 130, 180));
    startButton.setPosition(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2);

    sf::Text startButtonText("START", font, 32);
    startButtonText.setFillColor(sf::Color::Black);
    startButtonText.setPosition(
        WINDOW_WIDTH / 2 - startButtonText.getGlobalBounds().width / 2,
        WINDOW_HEIGHT / 2 + (60 - startButtonText.getGlobalBounds().height) / 2
    );

    sf::Text gameOverText("Game Over!", font, 36);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setPosition(WINDOW_WIDTH / 2 - gameOverText.getGlobalBounds().width / 2, WINDOW_HEIGHT / 2 - 100);

    sf::RectangleShape restartButton(sf::Vector2f(300, 60));
    restartButton.setFillColor(sf::Color(220, 20, 60));
    restartButton.setPosition(WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2);

    sf::Text restartButtonText("RESTART", font, 32);
    restartButtonText.setFillColor(sf::Color::Black);
    restartButtonText.setPosition(
        WINDOW_WIDTH / 2 - restartButtonText.getGlobalBounds().width / 2,
        WINDOW_HEIGHT / 2 + (60 - restartButtonText.getGlobalBounds().height) / 2
    );

    // --- Spawn Timings ---
    float coinSpawnInterval = 1.0f;      // Coins every 1.0s
    float obstacleSpawnInterval = 2.5f;  // Obstacles every 2.5s
    float heartSpawnInterval = 7.0f;     // Hearts every 7.0s

    // --- Minimum distance between entities ---
    const float minEntityDistance = 80.0f;

    // --- Button bounds for mouse click detection ---
    sf::FloatRect startButtonBounds(
        WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 40, 300, 80
    );
    sf::FloatRect restartButtonBounds(
        WINDOW_WIDTH / 2 - 150, WINDOW_HEIGHT / 2 - 40, 300, 80
    );

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // Mouse click for start text
            if (!gameStarted && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
                if (startButtonBounds.contains(mousePos)) {
                    gameStarted = true;
                    gameClock.restart();
                }
            }
            // Mouse click for restart text
            if (gameOver && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
                if (restartButtonBounds.contains(mousePos)) {
                    // Reset game state
                    entities.clear();
                    coinsCollected = 0;
                    life = MAX_LIFE;
                    hearts.clear();
                    for (int i = 0; i < MAX_LIFE; ++i) {
                        sf::Sprite heart;
                        heart.setTexture(heartTexture);
                        heart.setPosition(700 + i * 30, 10);
                        hearts.push_back(heart);
                    }
                    player.sprite.setPosition(100, GROUND_Y - player.sprite.getGlobalBounds().height);
                    player.velocity.y = 0;
                    isJumping = false;
                    onPlatform = false;
                    gameClock.restart();
                    gameOver = false;
                }
            }
        }

        // --- Start Screen ---
        if (!gameStarted) {
            window.clear(sf::Color(100, 149, 237));

            // Draw large, centered "START" text (acts as button)
            sf::Text startButtonText("START", font, 80);
            startButtonText.setFillColor(sf::Color::Black);
            sf::FloatRect textBounds = startButtonText.getLocalBounds();
            startButtonText.setPosition(
                WINDOW_WIDTH / 2 - textBounds.width / 2,
                WINDOW_HEIGHT / 2 - textBounds.height / 2 - textBounds.top
            );
            window.draw(startButtonText);

            // Draw welcome message below
            sf::Text welcomeText("Welcome! Click START to Play", font, 36);
            welcomeText.setFillColor(sf::Color::White);
            sf::FloatRect welcomeBounds = welcomeText.getLocalBounds();
            welcomeText.setPosition(
                WINDOW_WIDTH / 2 - welcomeBounds.width / 2,
                WINDOW_HEIGHT / 2 + 60
            );
            window.draw(welcomeText);

            window.display();
            continue;
        }

        // --- Game Over Screen ---
        if (gameOver) {
            window.clear(sf::Color(100, 149, 237));

            // Draw large, centered "RESTART" text (acts as button)
            sf::Text restartButtonText("RESTART", font, 80);
            restartButtonText.setFillColor(sf::Color::Black);
            sf::FloatRect textBounds = restartButtonText.getLocalBounds();
            restartButtonText.setPosition(
                WINDOW_WIDTH / 2 - textBounds.width / 2,
                WINDOW_HEIGHT / 2 - textBounds.height / 2 - textBounds.top
            );
            window.draw(restartButtonText);

            // Draw game over message below
            sf::Text gameOverText("Game Over!", font, 36);
            gameOverText.setFillColor(sf::Color::Red);
            sf::FloatRect gameOverBounds = gameOverText.getLocalBounds();
            gameOverText.setPosition(
                WINDOW_WIDTH / 2 - gameOverBounds.width / 2,
                WINDOW_HEIGHT / 2 + 60
            );
            window.draw(gameOverText);

            window.display();
            continue;
        }

        // Jump
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !isJumping) {
            player.velocity.y = -11;
            isJumping = true;
            onPlatform = false;
        }

        // Gravity
        player.velocity.y += GRAVITY;
        player.sprite.move(0, player.velocity.y);

        // Platform collision
        onPlatform = false;
        for (auto& plat : platforms) {
            plat.sprite.move(plat.velocity);
            if (plat.sprite.getPosition().x < -plat.sprite.getGlobalBounds().width) {
                plat.sprite.setPosition(WINDOW_WIDTH + 100, platformYDist(rng));
            }
            if (player.sprite.getGlobalBounds().intersects(plat.sprite.getGlobalBounds()) &&
                player.velocity.y >= 0 &&
                player.sprite.getPosition().y + player.sprite.getGlobalBounds().height - plat.sprite.getPosition().y < 20) {
                player.sprite.setPosition(player.sprite.getPosition().x, plat.sprite.getPosition().y - player.sprite.getGlobalBounds().height);
                player.velocity.y = 0;
                isJumping = false;
                onPlatform = true;
            }
        }

        // Landing on ground
        if (player.sprite.getPosition().y >= GROUND_Y - player.sprite.getGlobalBounds().height) {
            player.sprite.setPosition(player.sprite.getPosition().x, GROUND_Y - player.sprite.getGlobalBounds().height);
            player.velocity.y = 0;
            isJumping = false;
            onPlatform = false;
        }

        // Cloud movement and respawn
        for (auto& cloud : clouds) {
            cloud.sprite.move(cloud.velocity);
            if (cloud.sprite.getPosition().x < -150) {
                cloud.sprite.setPosition(WINDOW_WIDTH + 50, cloudYDist(rng));
            }
        }

        // --- Entity Spawning Logic ---

        // Coins: spawn most frequently
        if (coinClock.getElapsedTime().asSeconds() > coinSpawnInterval) {
            float y = (rand() % 2 == 0) ? GROUND_Y - 40 : platformYDist(rng);
            sf::Vector2f spawnPos(WINDOW_WIDTH, y);
            if (!isTooClose(spawnPos, entities, minEntityDistance)) {
                GameEntity coin;
                coin.sprite.setTexture(coinTexture);
                coin.sprite.setPosition(spawnPos);
                coin.velocity = sf::Vector2f(-5, 0);
                coin.type = EntityType::Coin;
                entities.push_back(coin);
                coinClock.restart();
            }
        }
        // Obstacles: spawn less frequently than coins
        if (obstacleClock.getElapsedTime().asSeconds() > obstacleSpawnInterval) {
            float y = GROUND_Y - obstacleTexture.getSize().y;
            sf::Vector2f spawnPos(WINDOW_WIDTH, y);
            if (!isTooClose(spawnPos, entities, minEntityDistance)) {
                GameEntity obstacle;
                obstacle.sprite.setTexture(obstacleTexture);
                obstacle.sprite.setPosition(spawnPos);
                obstacle.velocity = sf::Vector2f(-5, 0);
                obstacle.type = EntityType::Obstacle;
                entities.push_back(obstacle);
                obstacleClock.restart();
            }
        }
        // Hearts: spawn rarely
        if (heartClock.getElapsedTime().asSeconds() > heartSpawnInterval) {
            float y = (rand() % 2 == 0) ? GROUND_Y - 40 : platformYDist(rng);
            sf::Vector2f spawnPos(WINDOW_WIDTH, y);
            if (!isTooClose(spawnPos, entities, minEntityDistance)) {
                GameEntity heart;
                heart.sprite.setTexture(heartTexture);
                heart.sprite.setPosition(spawnPos);
                heart.velocity = sf::Vector2f(-5, 0);
                heart.type = EntityType::Heart;
                entities.push_back(heart);
                heartClock.restart();
            }
        }

        // Move entities and check collisions
        for (auto& e : entities) {
            e.sprite.move(e.velocity);

            if (!e.collected && player.sprite.getGlobalBounds().intersects(e.sprite.getGlobalBounds())) {
                if (e.type == EntityType::Coin) {
                    coinsCollected++;
                    e.collected = true;
                } else if (e.type == EntityType::Obstacle) {
                    if (life > 0) {
                        life--;
                        if (!hearts.empty()) hearts.pop_back();
                    }
                    e.collected = true;
                } else if (e.type == EntityType::Heart) {
                    if (life < MAX_LIFE) {
                        life++;
                        sf::Sprite heart;
                        heart.setTexture(heartTexture);
                        heart.setPosition(700 + (life - 1) * 30, 10);
                        hearts.push_back(heart);
                    }
                    e.collected = true;
                }
            }
        }

        // Remove off-screen or collected entities
        entities.erase(std::remove_if(entities.begin(), entities.end(), [](const GameEntity& e) {
            return e.sprite.getPosition().x < -50 || e.collected;
        }), entities.end());

        // Update score text (time survived)
        int timeSurvived = static_cast<int>(gameClock.getElapsedTime().asSeconds());
        scoreText.setString("Time: " + std::to_string(timeSurvived));
        coinText.setString("Coins: " + std::to_string(coinsCollected));

        // Game Over check
        if (life <= 0) {
            gameOver = true;
        }

        // Draw everything
        window.clear(sf::Color(100, 149, 237)); // sky blue

        // Draw clouds
        for (auto& cloud : clouds) window.draw(cloud.sprite);

        window.draw(ground);

        // Draw platforms
        for (auto& plat : platforms) window.draw(plat.sprite);

        window.draw(player.sprite);

        // Draw entities
        for (auto& e : entities) {
            if (!e.collected) window.draw(e.sprite);
        }

        for (auto& heart : hearts) window.draw(heart);

        window.draw(scoreText);
        window.draw(coinText);

        window.display();
    }

    return 0;
}