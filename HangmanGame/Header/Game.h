/*
Name: Emmanuel Rivas
ID: 15310887
Class: Fall 2024, CSC 211H
Date: 02/01/2025
Instructor: Dr. Azhar
Honors Project: Hangman
*/

#ifndef GAME_H
#define GAME_H

#include <string>
#include <stdexcept>

// Base exception class for game-related errors
class GameException : public std::runtime_error {
public:
    explicit GameException(const std::string& message) : std::runtime_error(message) { }
};

// Abstract base class for games
class Game {
protected:
    std::string gameName;
    bool hasScore;
    bool hasDatabase;
    bool isMultiplayer;
    bool isGameActive;

    // Protected constructor to prevent instantiation of Game objects
    Game(const std::string& name, bool useScore = false, bool useDB = false, bool multiplayer = false, bool isActive = false) {
        gameName = name;
        hasScore = useScore;
        hasDatabase = useDB;
        isMultiplayer = multiplayer;
        isGameActive = isActive;
    }

public:
    // Getters
    std::string getGameName() const { return gameName; }
    bool getHasScore() const { return hasScore; }
    bool getHasDatabase() const { return hasDatabase; }
    bool getIsMultiplayer() const { return isMultiplayer; }
    bool getIsGameActive() const { return isGameActive; }

    // Pure virtual functions that must be implemented by derived classes
    virtual bool startGame() = 0;
    virtual bool isGameOver() const = 0;
    virtual void endGame() = 0;
    virtual void resetGame() = 0;

    // Game operations that can be overridden if necessary
    virtual void pauseGame() {
        if (!isGameActive) throw GameException("Cannot pause: Game is inactive");
        isGameActive = false;
    }

    virtual void resumeGame() {
        if (isGameActive) throw GameException("Cannot resume: Game is active");
        isGameActive = true;
    }

    // Virtual destructor for proper cleanup in derived classes
    virtual ~Game() = default;
};

#endif  // GAME_H
