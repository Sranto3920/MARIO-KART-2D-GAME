#include<cstdint>
#include<string>
#include<cstring> //For memcpy,memmove
#include <fstream>
#include <algorithm>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <vector>
#include <random>
#include <sstream>

const int WINDOW_WIDTH = 900;
const int WINDOW_HEIGHT = 600;
const int GROUND_Y = 500;
const float GRAVITY = 0.3f; // Reduced from 0.5f to 0.3f for lighter gravity

const int PLATFORM_WIDTH = 180;
const int PLATFORM_HEIGHT = 30;
const int COIN_SIZE = 32;
const int OBSTACLE_SIZE = 40; // Increased from 32 to 40 for slightly taller obstacles
const int LIFE_ICON_SIZE = 32;
const int MAX_LIVES = 3;

enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER, HIGH_SCORE };
struct Platform {
    sf::Sprite sprite;
    float x, y;
    Platform(const sf::Texture& tex, float x_, float y_)
        : sprite(tex), x(x_), y(y_) {
        sprite.setPosition(x, y);
    }
};

std::vector<Platform> platforms;

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Jump & Dodge");
    window.setFramerateLimit(60);

    // Load Textures
    sf::Texture playerTexture;
    if (!playerTexture.loadFromFile("assets/player_spritesheet.png")) {
        throw std::runtime_error("Failed to load player sprite sheet!");
    }

    sf::Texture cloudTexture;
    if (!cloudTexture.loadFromFile("assets/cloud.png")) {
        throw std::runtime_error("Failed to load cloud texture!");
    }

    // --- Add this for the background image ---
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("assets/background.jpg")) {
        throw std::runtime_error("Failed to load background image!");
    }

        // ADD GROUND TEXTURE HERE (after line 84)
    sf::Texture groundTexture;
    if (!groundTexture.loadFromFile("assets/ground.png")) {
        throw std::runtime_error("Failed to load ground texture!");
    }
    sf::Sprite backgroundSprite(backgroundTexture);
    backgroundSprite.setScale(
        float(WINDOW_WIDTH) / backgroundTexture.getSize().x,
        float(WINDOW_HEIGHT) / backgroundTexture.getSize().y
    );
    // --- End background image addition ---

    // Animation setup
    const int FRAME_WIDTH = 1773 / 12;   // 75
    const int FRAME_HEIGHT = 150;      // 365
    const int FRAME_COUNT = 8;     // number of frames
    int currentFrame = 0;
    float animationTimer = 0.0f;
    const float FRAME_DURATION = 0.05f; // seconds per frame

    //Player setup
    sf::Sprite playerSprite;
    playerSprite.setTexture(playerTexture);
    playerSprite.setTextureRect(sf::IntRect(0, 0, FRAME_WIDTH, FRAME_HEIGHT));
    playerSprite.setPosition(100, GROUND_Y - FRAME_HEIGHT); // Correct Y position
    float playerVelocityY = 0;
    bool isJumping = false;

    //Background ground - Replace rectangle with sprite
    sf::Sprite groundSprite(groundTexture);
    groundSprite.setPosition(0, GROUND_Y);
    // Scale the ground image to fit the area (900x100 pixels)
    groundSprite.setScale(
        float(WINDOW_WIDTH) / groundTexture.getSize().x,
        100.0f / groundTexture.getSize().y
    );

    //Cloud setup
    std::vector<sf::Sprite> clouds;
    for (int i = 0; i < 3; ++i) {
        sf::Sprite cloud;
        cloud.setTexture(cloudTexture); // <-- Use the correct texture here!
        cloud.setPosition(200 + i * 250, 80 + (i % 2) * 40);
        clouds.push_back(cloud);
    }

    // Load additional textures
    sf::Texture platformTexture;
    if (!platformTexture.loadFromFile("assets/platform.png")) {
        throw std::runtime_error("Failed to load platform texture!");
    }
    sf::Texture coinTexture;
    if (!coinTexture.loadFromFile("assets/coin.png")) {
        throw std::runtime_error("Failed to load coin texture!");
    }
    sf::Texture obstacle1Texture;
    if (!obstacle1Texture.loadFromFile("assets/obstacle1.png")) {
        throw std::runtime_error("Failed to load obstacle1 texture!");
    }
    sf::Texture obstacle2Texture;
    if (!obstacle2Texture.loadFromFile("assets/obstacle2.png")) {
        throw std::runtime_error("Failed to load obstacle2 texture!");
    }
    sf::Texture lifeTexture;
    if (!lifeTexture.loadFromFile("assets/life.png")) {
        throw std::runtime_error("Failed to load life texture!");
    }

    // --- Platforms setup ---
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(150, WINDOW_WIDTH - PLATFORM_WIDTH - 50);
    std::uniform_real_distribution<float> yDist(200, GROUND_Y - 80);

    std::vector<Platform> platforms;
    for (int i = 0; i < 5; ++i) {
        float randX = xDist(gen) + i * 120; // Spread out a bit horizontally
        float randY = yDist(gen);
        platforms.emplace_back(platformTexture, randX, randY);
        platforms.back().sprite.setPosition(randX, randY);
    }

    // --- Coins setup ---
    struct Coin {
        sf::Sprite sprite;
        bool collected = false;
        Coin(const sf::Texture& tex, bool collected_ = false) : sprite(tex), collected(collected_) {}
    };
    std::vector<Coin> coins = {
        {coinTexture, false}, {coinTexture, false}, {coinTexture, false},
        {coinTexture, false}, {coinTexture, false}
    };
    // Place coins just above platforms or ground (different positions from obstacles)
    coins[0].sprite.setPosition(platforms[0].x + 20, platforms[0].y - COIN_SIZE - 15); // Left side
    coins[1].sprite.setPosition(platforms[1].x + 130, platforms[1].y - COIN_SIZE - 15); // Right side
    coins[2].sprite.setPosition(platforms[2].x + 20, platforms[2].y - COIN_SIZE - 15); // Left side
    coins[3].sprite.setPosition(platforms[3].x + 130, platforms[3].y - COIN_SIZE - 15); // Right side
    coins[4].sprite.setPosition(650, GROUND_Y - COIN_SIZE - 15); // Ground coin - different position

    // --- Obstacles setup ---
    struct Obstacle {
        sf::Sprite sprite;
        int type; // 1 or 2
        bool visible = true;
        Obstacle(const sf::Texture& tex, int type_) : sprite(tex), type(type_), visible(true) {}
    };
    std::vector<Obstacle> obstacles = {
        {obstacle1Texture, 1}, {obstacle2Texture, 2}, {obstacle1Texture, 1},
        {obstacle2Texture, 2}
    };
    // Place obstacles on platforms or ground (starting off-screen)
    obstacles[0].sprite.setPosition(WINDOW_WIDTH + 300, platforms[0].y - OBSTACLE_SIZE); // obstacle1 on platform 1
    obstacles[1].sprite.setPosition(WINDOW_WIDTH + 600, GROUND_Y - OBSTACLE_SIZE); // obstacle2 on ground only
    obstacles[2].sprite.setPosition(WINDOW_WIDTH + 900, platforms[2].y - OBSTACLE_SIZE); // obstacle1 on platform 3
    obstacles[3].sprite.setPosition(WINDOW_WIDTH + 1200, GROUND_Y - OBSTACLE_SIZE); // obstacle2 on ground only
    
    // Ensure obstacles are scaled properly if needed
    for (auto& obs : obstacles) {
        if (obs.type == 2) {
            // Make obstacle2 (type 2) a bit larger (e.g., 1.3x)
            obs.sprite.setScale(
                1.3f * float(OBSTACLE_SIZE) / obs.sprite.getTexture()->getSize().x,
                1.3f * float(OBSTACLE_SIZE) / obs.sprite.getTexture()->getSize().y
            );
        } else {
            // Default scaling for obstacle1
            obs.sprite.setScale(
                float(OBSTACLE_SIZE) / obs.sprite.getTexture()->getSize().x,
                float(OBSTACLE_SIZE) / obs.sprite.getTexture()->getSize().y
            );
        }
    }

    // --- Life icons ---
    std::vector<sf::Sprite> lifeIcons(MAX_LIVES, sf::Sprite(lifeTexture));
    for (int i = 0; i < MAX_LIVES; ++i)
        lifeIcons[i].setPosition(10 + i * (LIFE_ICON_SIZE + 5), 10);

    int lives = MAX_LIVES;
    int coinCount = 0;
    sf::Clock gameClock;
    float gameSpeed = 1.0f; // Base speed multiplier
    const float baseSpeed = 2.34f * 1.5f; // 1.5x faster initial speed
    static float gameEndTime = 0.0f; // Move this declaration here

    // --- Font for UI ---
    sf::Font font;
    if (!font.loadFromFile("assets/arial.ttf")) {
        throw std::runtime_error("Failed to load font!");
    }
    sf::Text scoreText, coinText, gameOverText, restartText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    coinText.setFont(font);
    coinText.setCharacterSize(24);
    coinText.setFillColor(sf::Color::Yellow);
    gameOverText.setFont(font);
    gameOverText.setCharacterSize(48);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setString("GAME OVER");
    gameOverText.setPosition(WINDOW_WIDTH / 2 - 120, WINDOW_HEIGHT / 2 - 60);
    restartText.setFont(font);
    restartText.setCharacterSize(24);
    restartText.setFillColor(sf::Color::White);
    restartText.setString("Press R to Restart");
    restartText.setPosition(WINDOW_WIDTH / 2 - 80, WINDOW_HEIGHT / 2 - 10);

    // --- BGM setup ---
    sf::Music bgm1, bgm2;
    bool bgm1Loaded = bgm1.openFromFile("assets/bgm1.ogg"); // Place your BGM at assets/bgm.ogg
    bool bgm2Loaded = bgm2.openFromFile("assets/bgm2.ogg"); // Place your BGM at assets/bgm.ogg
    bgm1.setLoop(true);
    bgm2.setLoop(true);
    bgm1.play(); // Always playing
    // bgm2 will be started/stopped on Start/Game Over

    // --- Logo and Title for Menu ---
    sf::Texture logoTexture;
    sf::Sprite logoSprite;
    bool logoLoaded = false;
    if (logoTexture.loadFromFile("assets/logo.png")) { // Place your logo at assets/logo.png
        logoSprite.setTexture(logoTexture);
        // Scale and position the logo as needed
        float logoScale = 0.4f; // Adjust as needed
        logoSprite.setScale(logoScale, logoScale);
        logoSprite.setPosition(WINDOW_WIDTH / 2 - logoTexture.getSize().x * logoScale / 2, 60);
        logoLoaded = true;
    }

    sf::Text titleText("RUNFINITY", font, 56);
    titleText.setFillColor(sf::Color(50, 50, 50));
    titleText.setStyle(sf::Text::Bold);
    titleText.setPosition(WINDOW_WIDTH / 2 - titleText.getLocalBounds().width / 2, 180);

    // --- Start button setup (unchanged) ---
    sf::RectangleShape startButton(sf::Vector2f(250, 60));
    startButton.setFillColor(sf::Color(70, 130, 180));
    startButton.setPosition(WINDOW_WIDTH / 2 - 125, WINDOW_HEIGHT / 2 - 30);

    sf::Text startText("Start Game", font, 32);
    startText.setFillColor(sf::Color::White);
    startText.setPosition(WINDOW_WIDTH / 2 - 90, WINDOW_HEIGHT / 2 - 18);

    // --- Resume, Restart, Main Menu buttons (new) ---
    sf::RectangleShape resumeButton(sf::Vector2f(200, 50));
    resumeButton.setFillColor(sf::Color(60, 179, 113));
    resumeButton.setPosition(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 80);

    sf::Text resumeText("Resume", font, 28);
    resumeText.setFillColor(sf::Color::White);
    resumeText.setPosition(WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT / 2 - 70);

    sf::RectangleShape restartButton(sf::Vector2f(200, 50));
    restartButton.setFillColor(sf::Color(70, 130, 180));
    restartButton.setPosition(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2);

    sf::Text restartMenuText("Restart", font, 28);
    restartMenuText.setFillColor(sf::Color::White);
    restartMenuText.setPosition(WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT / 2 + 10);

    sf::RectangleShape mainMenuButton(sf::Vector2f(200, 50));
    mainMenuButton.setFillColor(sf::Color(220, 20, 60));
    mainMenuButton.setPosition(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 80);

    sf::Text mainMenuText("Main Menu", font, 28);
    mainMenuText.setFillColor(sf::Color::White);
    mainMenuText.setPosition(WINDOW_WIDTH / 2 - 65, WINDOW_HEIGHT / 2 + 90);
    // --- End new buttons ---

    // High Score button
    sf::RectangleShape highScoreButton(sf::Vector2f(250, 60));
    highScoreButton.setFillColor(sf::Color(100, 100, 180));
    highScoreButton.setPosition(WINDOW_WIDTH / 2 - 125, WINDOW_HEIGHT / 2 + 40);

    sf::Text highScoreText("High Score", font, 32);
    highScoreText.setFillColor(sf::Color::White);
    highScoreText.setPosition(WINDOW_WIDTH / 2 - 90, WINDOW_HEIGHT / 2 + 52);

    // --- Back button (top right corner) ---
    sf::RectangleShape backButton(sf::Vector2f(100, 40));
    backButton.setFillColor(sf::Color(200, 60, 60));
    backButton.setPosition(WINDOW_WIDTH - 120, 20);

    sf::Text backText("Back", font, 22);
    backText.setFillColor(sf::Color::White);
    backText.setPosition(WINDOW_WIDTH - 95, 28);
    // --- End back button ---

    bool gameOver = false;

    GameState gameState = GameState::MENU;

    // High Scores
    std::vector<int> highScores = {0, 0, 0}; // Always start with three zeros

    // Load high scores from file (if file doesn't exist, keep zeros)
    auto loadHighScores = [&]() {
        std::ifstream fin("assets/highscores.txt");
        int score, i = 0;
        while (fin >> score && i < 3) {
            highScores[i++] = score;
        }
        // If less than 3 scores in file, fill the rest with 0
        for (; i < 3; ++i) highScores[i] = 0;
    };

    // Save high scores to file
    auto saveHighScores = [&]() {
        std::ofstream fout("assets/highscores.txt");
        for (int s : highScores) fout << s << std::endl;
    };

    // Call once at start
    loadHighScores();

    // --- Coin sound setup ---
    sf::SoundBuffer coinBuffer;
    if (!coinBuffer.loadFromFile("assets/coin.wav")) {
        throw std::runtime_error("Failed to load coin sound!");
    }
    sf::Sound coinSound;
    coinSound.setBuffer(coinBuffer);

    // --- Obstacle hit sound setup ---
    sf::SoundBuffer obsBuffer;
    if (!obsBuffer.loadFromFile("assets/obs.wav")) {
        throw std::runtime_error("Failed to load obstacle sound!");
    }
    sf::Sound obsSound;
    obsSound.setBuffer(obsBuffer);

    // --- Game over sound setup ---
    sf::SoundBuffer gameOverBuffer;
    if (!gameOverBuffer.loadFromFile("assets/gameover.wav")) {
        throw std::runtime_error("Failed to load game over sound!");
    }
    sf::Sound gameOverSound;
    gameOverSound.setBuffer(gameOverBuffer);

    static bool gameOverSoundPlayed = false; // Ensuring this is at the top of main() or just before the main game loop

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // Pause game with ESC
            if (gameState == GameState::PLAYING && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                gameState = GameState::PAUSED;
            }

            // Pause menu button handling
            if (gameState == GameState::PAUSED) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    if (resumeButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        gameState = GameState::PLAYING;
                    } else if (restartButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        // Reset game state
                        gameOver = false;
                        lives = MAX_LIVES;
                        coinCount = 0;
                        gameClock.restart();
                        gameSpeed = 1.0f;
                        playerSprite.setPosition(100, GROUND_Y - FRAME_HEIGHT);
                        playerVelocityY = 0;
                        isJumping = false;
                        gameEndTime = 0.0f;
                        for (auto& coin : coins) coin.collected = false;
                        // Reset platforms, coins, obstacles as in your restart logic
                        platforms[0].sprite.setPosition(200, 400);
                        platforms[1].sprite.setPosition(500, 300);
                        platforms[2].sprite.setPosition(800, 200);
                        platforms[3].sprite.setPosition(1200, 350);
                        platforms[4].sprite.setPosition(1600, 250);
                        coins[0].sprite.setPosition(platforms[0].x + 20, platforms[0].y - COIN_SIZE - 15);
                        coins[1].sprite.setPosition(platforms[1].x + 130, platforms[1].y - COIN_SIZE - 15);
                        coins[2].sprite.setPosition(platforms[2].x + 20, platforms[2].y - COIN_SIZE - 15);
                        coins[3].sprite.setPosition(platforms[3].x + 130, platforms[3].y - COIN_SIZE - 15);
                        coins[4].sprite.setPosition(650, GROUND_Y - COIN_SIZE - 15);
                        obstacles[0].sprite.setPosition(WINDOW_WIDTH + 300, platforms[0].y - OBSTACLE_SIZE);
                        obstacles[0].visible = true;
                        obstacles[1].sprite.setPosition(WINDOW_WIDTH + 600, GROUND_Y - OBSTACLE_SIZE);
                        obstacles[1].visible = true;
                        obstacles[2].sprite.setPosition(WINDOW_WIDTH + 900, platforms[2].y - OBSTACLE_SIZE);
                        obstacles[2].visible = true;
                        obstacles[3].sprite.setPosition(WINDOW_WIDTH + 1200, GROUND_Y - OBSTACLE_SIZE);
                        obstacles[3].visible = true;
                        gameState = GameState::PLAYING;
                    } else if (mainMenuButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        gameState = GameState::MENU; // Always go to MENU, not HIGH_SCORE
                    }
                }
            }
            // In MENU state
            if (gameState == GameState::MENU) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    if (startButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        gameState = GameState::PLAYING;
                        // Reset game state if needed
                        gameOver = false;
                        lives = MAX_LIVES;
                        coinCount = 0;
                        gameClock.restart();
                        gameSpeed = 1.0f;
                        playerSprite.setPosition(100, GROUND_Y - FRAME_HEIGHT);
                        playerVelocityY = 0;
                        isJumping = false;
                        gameEndTime = 0.0f;
                        for (auto& coin : coins) coin.collected = false;
                        // Start BGM2 when game starts
                        if (bgm2Loaded) bgm2.play();
                    } else if (highScoreButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        gameState = GameState::HIGH_SCORE;
                    }
                }
            }
            // In HIGH_SCORE state
            if (gameState == GameState::HIGH_SCORE) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    if (backButton.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        gameState = GameState::MENU;
                    }
                }
                // (Keep ESC key handling if you want)
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                    gameState = GameState::MENU;
                }
            }
            // ...existing event handling for PLAYING state...
            if (gameState == GameState::PLAYING) {
                // (keep your existing event handling for restart/gameplay here)
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R && gameOver) {
                    // Reset game state
                    gameOver = false;
                    lives = MAX_LIVES;
                    coinCount = 0;
                    gameClock.restart();
                    gameSpeed = 1.0f; // Reset speed multiplier
                    playerSprite.setPosition(100, GROUND_Y - FRAME_HEIGHT);
                    playerVelocityY = 0;
                    isJumping = false;
                    // Reset game end time
                    gameEndTime = 0.0f; // This will now work
                    // Reset all coins
                    for (auto& coin : coins) {
                        coin.collected = false;
                    }
                    // Reset platform positions
                    platforms[0].sprite.setPosition(200, 400);
                    platforms[1].sprite.setPosition(500, 300);
                    platforms[2].sprite.setPosition(800, 200);
                    platforms[3].sprite.setPosition(1200, 350);
                    platforms[4].sprite.setPosition(1600, 250);
                    // Reset coin positions (different from obstacles)
                    coins[0].sprite.setPosition(platforms[0].x + 20, platforms[0].y - COIN_SIZE - 15);
                    coins[1].sprite.setPosition(platforms[1].x + 130, platforms[1].y - COIN_SIZE - 15);
                    coins[2].sprite.setPosition(platforms[2].x + 20, platforms[2].y - COIN_SIZE - 15);
                    coins[3].sprite.setPosition(platforms[3].x + 130, platforms[3].y - COIN_SIZE - 15);
                    coins[4].sprite.setPosition(650, GROUND_Y - COIN_SIZE - 15);
                    // Reset obstacle positions and visibility (starting off-screen)
                    obstacles[0].sprite.setPosition(WINDOW_WIDTH + 300, platforms[0].y - OBSTACLE_SIZE);
                    obstacles[0].visible = true;
                    obstacles[1].sprite.setPosition(WINDOW_WIDTH + 600, GROUND_Y - OBSTACLE_SIZE); // obstacle2 on ground only
                    obstacles[1].visible = true;
                    obstacles[2].sprite.setPosition(WINDOW_WIDTH + 900, platforms[2].y - OBSTACLE_SIZE);
                    obstacles[2].visible = true;
                    obstacles[3].sprite.setPosition(WINDOW_WIDTH + 1200, GROUND_Y - OBSTACLE_SIZE); // obstacle2 on ground only
                    obstacles[3].visible = true;
                    // --- FIX: Restart BGM2 on restart ---
                    if (bgm2Loaded) {
                        bgm2.stop(); // Ensure it is stopped first
                        bgm2.play(); // Start again
                    }
                }
            }
        }

        window.clear(sf::Color(100, 149, 237)); // sky blue

        if (gameState == GameState::MENU) {
            window.draw(backgroundSprite);
            if (logoLoaded) window.draw(logoSprite);
            window.draw(titleText);
            window.draw(startButton);
            window.draw(startText);
            window.draw(highScoreButton);
            window.draw(highScoreText);
            window.display();
            continue;
        }

        if (gameState == GameState::PAUSED) {
            window.draw(backgroundSprite);
            window.draw(resumeButton);
            window.draw(resumeText);
            window.draw(restartButton);
            window.draw(restartMenuText);
            window.draw(mainMenuButton);
            window.draw(mainMenuText);
            window.display();
            continue;
        }

        if (gameState == GameState::HIGH_SCORE) {
            window.draw(backgroundSprite);
            sf::Text hsTitle("HIGH SCORES", font, 40);
            hsTitle.setFillColor(sf::Color::Black);
            hsTitle.setPosition(WINDOW_WIDTH / 2 - hsTitle.getLocalBounds().width / 2, 120);
            window.draw(hsTitle);

            for (size_t i = 0; i < highScores.size(); ++i) {
                sf::Text scoreLine(std::to_string(i + 1) + ". " + std::to_string(highScores[i]), font, 32);
                scoreLine.setFillColor(sf::Color::Black);
                scoreLine.setPosition(WINDOW_WIDTH / 2 - 60, 200 + 50 * i);
                window.draw(scoreLine);
            }

            sf::Text escHint("Press ESC to return", font, 24);
            escHint.setFillColor(sf::Color::Black);
            escHint.setPosition(WINDOW_WIDTH / 2 - 100, 400);
            window.draw(escHint);

            // --- Back button (top right corner) ---
            window.draw(backButton);
            window.draw(backText);
            // --- End back button ---

            window.display();
            continue;
        }

        if (!gameOver) {
            // Calculate game speed based on time - gradual increase every 10 seconds
            float elapsedTime = gameClock.getElapsedTime().asSeconds();
            if (elapsedTime > 10.0f) {
                // Gradual speed increase by 1.3x every 10 seconds
                float speedMultiplier = 1.0f + (std::floor(elapsedTime / 10.0f) * 0.3f);
                gameSpeed = std::min(speedMultiplier, 3.0f); // Cap at 3x speed
            } else {
                gameSpeed = 1.0f; // Normal speed for first 10 seconds
            }

            float currentSpeed = baseSpeed * gameSpeed;

            // Move clouds to the left, loop them
            for (auto& cloud : clouds) {
                cloud.move(-currentSpeed, 0);
                if (cloud.getPosition().x < -150) {
                    cloud.setPosition(WINDOW_WIDTH + 50, cloud.getPosition().y);
                }
            }

            // Move platforms to the left, loop them
            for (auto& p : platforms) {
                p.sprite.move(-currentSpeed, 0);
                if (p.sprite.getPosition().x < -PLATFORM_WIDTH) {
    float maxX = 0;
    for (auto& pp : platforms) {
        maxX = std::max(maxX, pp.sprite.getPosition().x);
    }
    float randY = yDist(gen);
    p.y = randY;
    p.sprite.setPosition(maxX + 300 + xDist(gen) / 2, randY); // Randomize both X gap and Y
}
            }

            // Move obstacles to the left, loop them (only after 5 seconds)
            elapsedTime = gameClock.getElapsedTime().asSeconds();
            if (elapsedTime > 5.0f) { // Only move obstacles after 5 seconds
                for (auto& obs : obstacles) {
                    obs.sprite.move(-currentSpeed, 0);
                    if (obs.sprite.getPosition().x < -OBSTACLE_SIZE) {
                        // Finding the rightmost obstacle and place this one after it
                        float maxX = 0;
                        for (auto& oo : obstacles) {
                            maxX = std::max(maxX, oo.sprite.getPosition().x);
                        }
                        
                        // Set Y position based on obstacle type
                        float newY;
                        if (obs.type == 2) {
                            newY = GROUND_Y - OBSTACLE_SIZE; // Type 2 obstacles only on ground
                        } else {
                            // Type 1 obstacles can be on platforms or ground
                            int randomChoice = rand() % 6; // 0-5
                            if (randomChoice < 5) {
                                newY = platforms[randomChoice].y - OBSTACLE_SIZE; // On platform
                            } else {
                                newY = GROUND_Y - OBSTACLE_SIZE; // On ground
                            }
                        }
                        
                        obs.sprite.setPosition(maxX + 500, newY); // More spacing
                        obs.visible = true; // Reset visibility when recycling
                    }
                }
            }

            // Move coins to the left, loop them (ensure they keep coming)
            for (auto& coin : coins) {
                coin.sprite.move(-currentSpeed, 0);
                if (coin.sprite.getPosition().x < -COIN_SIZE) {
                    // Find the rightmost coin and place this one after it
                    float maxX = 0;
                    for (auto& cc : coins) {
                        maxX = std::max(maxX, cc.sprite.getPosition().x);
                    }
                    coin.sprite.setPosition(maxX + 350, coin.sprite.getPosition().y);
                    coin.collected = false; // Reset collected status when recycling
                }
            }

            //Jump - reduced jump height
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !isJumping) {
                playerVelocityY = -10; // *** JUMP HEIGHT VALUE *** - Changed from -10 to -12 for higher jump
                isJumping = true;
            }

            //Gravity
            playerVelocityY += GRAVITY;
            playerSprite.move(0, playerVelocityY);

            //Landing on ground or platforms
            bool onPlatform = false;
            for (auto& p : platforms) {
                sf::FloatRect platBounds = p.sprite.getGlobalBounds();
                sf::FloatRect playerBounds = playerSprite.getGlobalBounds();
                if (playerBounds.intersects(platBounds) &&
                    playerVelocityY >= 0 &&
                    playerBounds.top + playerBounds.height - 10 < platBounds.top + 10) {
                    playerSprite.setPosition(playerSprite.getPosition().x, platBounds.top - playerBounds.height);
                    playerVelocityY = 0;
                    isJumping = false;
                    onPlatform = true;
                }
            }
            // Ground landing
            if (playerSprite.getPosition().y >= GROUND_Y - playerSprite.getGlobalBounds().height) {
                playerSprite.setPosition(playerSprite.getPosition().x, GROUND_Y - playerSprite.getGlobalBounds().height);
                playerVelocityY = 0;
                isJumping = false;
            }

            // Animate player
            animationTimer += (1.0f / 60.0f);
            if (animationTimer >= FRAME_DURATION) {
                animationTimer = 0.0f;
                currentFrame = (currentFrame + 1) % FRAME_COUNT;
                playerSprite.setTextureRect(sf::IntRect(currentFrame * FRAME_WIDTH, 0, FRAME_WIDTH, FRAME_HEIGHT));
            }

            // --- Coin collection ---
            for (auto& coin : coins) {
                if (!coin.collected && playerSprite.getGlobalBounds().intersects(coin.sprite.getGlobalBounds())) {
                    coin.collected = true;
                    coinCount++;
                    coinSound.play(); // Play sound when coin is collected
                }
            }

            // --- Obstacle collision (only after 5 seconds) ---
            static sf::Clock collisionCooldown;
            if (elapsedTime > 5.0f && collisionCooldown.getElapsedTime().asSeconds() > 0.7f) {
                for (auto& obs : obstacles) {
                    if (obs.visible && playerSprite.getGlobalBounds().intersects(obs.sprite.getGlobalBounds())) {
                        lives--;
                        collisionCooldown.restart();
                        obsSound.play();
                        obs.visible = false;
                        if (lives <= 0) {
                            if (!gameOver) {
                                gameOver = true;
                                gameOverSoundPlayed = false; // Reset flag on new game over
                            }
                            if (!gameOverSoundPlayed) {
                                gameOverSound.play();
                                gameOverSoundPlayed = true;
                            }
                        }
                        break;
                    }
                }
            }
        }

        //Draw everything
        window.clear(sf::Color(100, 149, 237)); // sky blue

        // --- Draw background first ---
        window.draw(backgroundSprite);

        // Draw clouds
        for (auto& cloud : clouds) window.draw(cloud);

        // Draw platforms
        for (auto& p : platforms) window.draw(p.sprite);

        // Draw coins
        for (auto& coin : coins) if (!coin.collected) window.draw(coin.sprite);

        // Draw obstacles
        for (auto& obs : obstacles) {
            if (obs.visible) {
                window.draw(obs.sprite);
            }
        }

        window.draw(groundSprite); // Changed from window.draw(ground);
        window.draw(playerSprite);

        // Draw lives
        for (int i = 0; i < lives; ++i) window.draw(lifeIcons[i]);

        // Draw score and coin count
        std::stringstream ss;
        
        if (!gameOver) {
            gameEndTime = gameClock.getElapsedTime().asSeconds(); // Update while game is running
            ss << "Time: " << static_cast<int>(gameEndTime);
        } else {
            ss << "Time: " << static_cast<int>(gameEndTime); // Show final time when game over
        }
        scoreText.setString(ss.str());
        scoreText.setPosition(10, 50);
        window.draw(scoreText);

        coinText.setString("Coins: " + std::to_string(coinCount));
        coinText.setPosition(10, 80);
        window.draw(coinText);

        if (gameOver) {
            window.draw(gameOverText);
            window.draw(restartText);
            // Stop BGM2 when game is over
            if (bgm2Loaded && bgm2.getStatus() == sf::Music::Playing) bgm2.stop();
        }

        // After detecting game over and before drawing the high score screen, add this block:
        if (gameOver) {
            // Only update high scores once per game over
            static bool highScoreUpdated = false;
            if (!highScoreUpdated) {
                // Add the new score and keep top 3
                highScores.push_back(static_cast<int>(gameEndTime));
                std::sort(highScores.rbegin(), highScores.rend());
                if (highScores.size() > 3) highScores.resize(3);
                highScoreUpdated = true;
            }
            // Reset flag when restarting or going to menu
            if (gameState == GameState::MENU || (sf::Keyboard::isKeyPressed(sf::Keyboard::R) && gameOver)) {
                highScoreUpdated = false;
            }
        }

        window.display();
    }

    return 0;
}

