#include "Game.hpp"
#include <iostream>
#include <sstream>
#include <vector>

Music music("music.ogg");

Game::Game() :
    window(sf::VideoMode(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT), "Tetris"),
    board(),
    gen(rd()),
    dist(0, static_cast<int>(Tetromino::Type::COUNT) - 1),
    isPaused(false),
    fastDrop(false),
    moveDelay(Constants::MOVE_DELAY),
    timeSinceLastMove(0.0f)
{
    window.setFramerateLimit(60);
    initializeText();
    spawnTetromino();
}

void Game::initializeText() {
    // Try these font locations in order
    const std::vector<std::string> fontPaths = {
        "Fonts/PressStart.ttf","fonts/PressStart.ttf",  // Recommended location
        "PressStart.ttf",        // Same folder as executable
        "assets/fonts/PressStart.ttf" // Common alternative
    };

    bool fontLoaded = false;
    for (const auto& path : fontPaths) {
        if (font.loadFromFile(path)) {
            fontLoaded = true;
            break;
        }
    }

    // Fallback to SFML's default font if no file found
    if (!fontLoaded) {
        std::cerr << "\033[1;37m" << "Note: Place PressStart.ttf in fonts/ folder for best results"
            << "\033[0m" << std::endl;
        font = sf::Font(); // Default font
    }

    // Initialize all text elements with WHITE color
    auto initText = [this](sf::Text& text, const std::string& str, float x, float y, int size = 24) {
        text.setFont(font);
        text.setString(str);
        text.setCharacterSize(size);
        text.setFillColor(sf::Color::White);
        text.setPosition(x, y);
        };

    // Moved UI text down to fit taller window
    initText(scoreText, "Score: 0", 50.0f, 120.0f);
    initText(levelText, "Level: 1", 50.0f, 170.0f);
    initText(linesText, "Lines: 0", 50.0f, 220.0f);
    initText(gameOverText, "GAME OVER",
        Constants::WINDOW_WIDTH / 2.0f - 150.0f,
        Constants::WINDOW_HEIGHT / 2.0f - 30.0f, 48);
}

void Game::run() {
    music.play();
    sf::Clock clock;
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        processEvents();
        if (!isPaused && !board.isGameOver()) {
            update(deltaTime);
        }
        render();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }

        if (event.type == sf::Event::KeyPressed) {
            if (board.isGameOver() && event.key.code == sf::Keyboard::R) {
                board.reset();
                spawnTetromino();
                continue;
            }

            if (event.key.code == sf::Keyboard::P) {
                isPaused = !isPaused;
                continue;
            }

            if (isPaused || !currentTetromino) continue;

            switch (event.key.code) {
            case sf::Keyboard::Left:
                board.moveTetromino(*currentTetromino, -1, 0);
                break;
            case sf::Keyboard::Right:
                board.moveTetromino(*currentTetromino, 1, 0);
                break;
            case sf::Keyboard::Down:
                fastDrop = true;
                moveDelay = Constants::FAST_MOVE_DELAY;
                break;
            case sf::Keyboard::Up:
                board.rotateTetromino(*currentTetromino);
                break;
            case sf::Keyboard::Space:
                board.dropTetromino(*currentTetromino);
                board.placeTetromino(*currentTetromino);
                spawnTetromino();
                break;
            }
        }

        if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Down) {
            fastDrop = false;
            moveDelay = Constants::MOVE_DELAY;
        }
    }
}

void Game::update(float deltaTime) {
    if (!currentTetromino) {
        spawnTetromino();
        return;
    }

    timeSinceLastMove += deltaTime;
    if (timeSinceLastMove >= moveDelay) {
        timeSinceLastMove = 0.0f;
        if (!board.moveTetromino(*currentTetromino, 0, 1)) {
            board.placeTetromino(*currentTetromino);
            if (!board.isGameOver()) {
                spawnTetromino();
            }
        }
    }
}

void Game::spawnTetromino() {
    currentTetromino = std::move(nextTetromino);
    nextTetromino = std::make_unique<Tetromino>(static_cast<Tetromino::Type>(dist(gen)));

    if (currentTetromino) {
        currentTetromino->move(Constants::BOARD_WIDTH / 2 - 2, 0);
        if (currentTetromino->isCollision(board.getGrid())) {
            board.setGameOver(true);
        }
    }
}

void Game::render() {
    window.clear(sf::Color::Black);

    // Draw game elements
    board.draw(window);
    if (currentTetromino) currentTetromino->draw(window);

    // Update and draw UI text
    std::ostringstream ss;
    ss << "Score: " << board.getScore();
    scoreText.setString(ss.str());
    ss.str(""); ss << "Level: " << board.getLevel();
    levelText.setString(ss.str());
    ss.str(""); ss << "Lines: " << board.getLinesCleared();
    linesText.setString(ss.str());

    window.draw(scoreText);
    window.draw(levelText);
    window.draw(linesText);

    // Next piece preview (moved down for taller window)
    if (nextTetromino) {
        sf::Text nextText;
        nextText.setFont(font);
        nextText.setString("Next:");
        nextText.setCharacterSize(24);
        nextText.setFillColor(sf::Color::White);
        nextText.setPosition(50.0f, 320.0f);
        window.draw(nextText);

        sf::RectangleShape block(sf::Vector2f(
            Constants::CELL_SIZE - 2.f,
            Constants::CELL_SIZE - 2.f
        ));
        block.setFillColor(nextTetromino->getColor());

        for (const auto& pos : nextTetromino->getBlocks()) {
            block.setPosition(
                100.f + pos.x * Constants::CELL_SIZE,
                370.f + pos.y * Constants::CELL_SIZE
            );
            window.draw(block);
        }
    }

    // Game state displays
    if (board.isGameOver()) window.draw(gameOverText);

    if (isPaused) {
        sf::Text pauseText;
        pauseText.setFont(font);
        pauseText.setString("PAUSED");
        pauseText.setCharacterSize(48);
        pauseText.setFillColor(sf::Color::White);
        pauseText.setPosition(
            Constants::WINDOW_WIDTH / 2.f - pauseText.getLocalBounds().width / 2.f,
            Constants::WINDOW_HEIGHT / 2.f - pauseText.getLocalBounds().height / 2.f
        );
        window.draw(pauseText);
    }

    window.display();
}
