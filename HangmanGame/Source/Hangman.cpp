/*
Name: Emmanuel Rivas
ID: 15310887
Class: Fall 2024, CSC 211H
Date: 02/01/2025
Instructor: Dr. Azhar
Honors Project: Hangman
*/

#include "Hangman.h"
#include <iostream>
#include <stdexcept>
#include <random>
#include <algorithm>
#include <cctype>

Hangman::Hangman() : Game("Hangman", true, true) {
    currentState = GameState::MENU;
    currentMode = GameMode::CLASSIC;
    currentCategory = Category::CSC_111;
    currentDbPath = "";
    currentRound = 0;
    currentScore = 0.00;
    incorrectRounds = 0;
    incorrectGuesses = 0;
    remainingChances = MAX_CLASSIC_QUESTION_CHANCES;
    timeLimit = std::chrono::seconds(SECONDS_PER_TEST_QUESTION * TOTAL_ROUNDS);
    currentPlayerId = -1;
    hasPlayerWon = false;
}

bool Hangman::startGame() {
    try {
        if (fullQuestionBank.empty()) throw HangmanException("Question bank is empty");

        initializeGame();
        selectRandomQuestions();

        currentState = GameState::PLAYING;

        if (currentMode == GameMode::TEST) gameStartTime = std::chrono::steady_clock::now();

        isGameActive = true;
    }
    catch (const std::exception& err) {
        throw HangmanException("Failed to start game: " + std::string(err.what()));
    }

    return isGameActive;
}

void Hangman::initializeGame() {
    currentState = GameState::MENU;
    currentRound = 1;
    currentScore = 0.00;
    incorrectRounds = 0;
    incorrectGuesses = 0;
    remainingChances = MAX_CLASSIC_QUESTION_CHANCES;

    questionSet.clear();
}

void Hangman::selectRandomQuestions() {
    if (fullQuestionBank.size() < TOTAL_ROUNDS) throw HangmanException("Insufficient number of questions in question bank");

    // Reset "used" flag for all questions
    for (QuestionAnswer& qa : fullQuestionBank) qa.used = false;

    // Create random number generator
    std::random_device rd;
    std::mt19937 gen(rd());

    int values = 0;
    values = static_cast<int>(fullQuestionBank.size() - 1);

    std::uniform_int_distribution<int> dis(0, values);
    questionSet.clear();

    // Put questions into the current question set
    while (questionSet.size() < TOTAL_ROUNDS) {
        int index = 0;
        index = dis(gen);

        if (!fullQuestionBank[index].used) {
            questionSet.push_back(fullQuestionBank[index]);
            fullQuestionBank[index].used = true;
        }
    }

    if (questionSet.size() != TOTAL_ROUNDS) throw HangmanException("Cannot load random questions into question set");
}

bool Hangman::isGameOver() const {
    bool gameOverFlag = false;

    if (currentState == GameState::GAME_OVER) gameOverFlag = true;
    else if (currentRound > TOTAL_ROUNDS) gameOverFlag = true;
    else if (currentScore >= MAX_SCORE) gameOverFlag = true;
    else if ((currentMode == GameMode::TEST) && isTimeExpired()) gameOverFlag = true;

    return gameOverFlag;
}

void Hangman::endGame() {
    currentState = GameState::GAME_OVER;
    isGameActive = false;
}

void Hangman::resetGame() {
    isGameActive = false;
    currentState = GameState::MENU;
    currentRound = 0;
    currentScore = 0.00;
    incorrectRounds = 0;
    incorrectGuesses = 0;
    remainingChances = MAX_CLASSIC_QUESTION_CHANCES;
    hasPlayerWon = false;

    questionSet.clear();
}

bool Hangman::makeGuess(const std::string& guess) {
    if (!validateGuess(guess)) throw HangmanException("Invalid guess format");
    if (currentRound > TOTAL_ROUNDS) throw HangmanException("Cannot make guess: Game is over");
    if (currentState != GameState::PLAYING) throw HangmanException("Cannot make guess: Game is not in playing state");
    if ((currentMode == GameMode::TEST) && isTimeExpired()) currentState = GameState::GAME_OVER;

    const QuestionAnswer& currentQA = getCurrentQuestion();

    // Case-insensitive comparison
    std::string guessLower = guess;
    std::string answerLower = currentQA.answer;

    std::transform(guessLower.begin(), guessLower.end(), guessLower.begin(), ::tolower);
    std::transform(answerLower.begin(), answerLower.end(), answerLower.begin(), ::tolower);

    bool isCorrect = false;
    isCorrect = (guessLower == answerLower);

    // Update score
    updateScore(isCorrect);

    // Update round
    if (currentMode == GameMode::CLASSIC) {  // Classic mode
        if (!isCorrect) {
            if (incorrectGuesses >= 5) incorrectRounds++;
        }

        if ((incorrectGuesses >= 5) || isCorrect) {
            currentRound++;
            incorrectGuesses = 0;
            remainingChances = MAX_CLASSIC_QUESTION_CHANCES;
        }
    }
    else {  // Test mode
        if (!isCorrect) incorrectRounds++;
        currentRound++;
    }

    if ((currentRound > TOTAL_ROUNDS) || (currentScore >= MAX_SCORE)) currentState = GameState::GAME_OVER;

    return isCorrect;
}

bool Hangman::validateGuess(const std::string& guess) const {
    bool isValid = false;

    if (!guess.empty() && (guess.size() <= 200)) isValid = true;  // Integer 200 used as an arbitrary max length
    
    return isValid;
}

void Hangman::updateScore(bool isCorrect) {
    if (currentMode == GameMode::CLASSIC) {  // Classic mode
        if (isCorrect) {
            currentScore += (POINTS_PER_ROUND - (incorrectGuesses * POINTS_PER_ATTEMPT));
            incorrectGuesses = 0;
        }
        else {
            incorrectGuesses++;
        }
    }
    else {  // Test mode
        if (isCorrect) currentScore += POINTS_PER_ROUND;
    }

    // Ensure score does not exceed MAX_SCORE or go below 0
    currentScore = std::min(std::max(currentScore, 0.00), MAX_SCORE);
}

bool Hangman::isTimeExpired() const {
    bool isExpired = false;

    if (currentMode == GameMode::TEST) {
        std::chrono::steady_clock::time_point now;
        now = std::chrono::steady_clock::now();

        std::chrono::seconds elapsed;
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - gameStartTime);

        if (elapsed >= timeLimit) isExpired = true;
    }

    return isExpired;
}

void Hangman::setGameMode(GameMode mode) {
    if (currentState == GameState::PLAYING) throw HangmanException("Cannot change game mode while playing");
    currentMode = mode;
}

void Hangman::setCategory(Category cat) {
    if (currentState == GameState::PLAYING) throw HangmanException("Cannot change category while playing");
    currentCategory = cat;
}

bool Hangman::loadQuestions(const std::vector<QuestionAnswer>& questions) {
    if (questions.empty()) throw HangmanException("Cannot load empty question set");
    fullQuestionBank = questions;

    return true;
}

const QuestionAnswer& Hangman::getCurrentQuestion() const {
    if (currentRound > questionSet.size()) throw HangmanException("No current question available");
    
    int index = 0;
    index = currentRound - 1;

    return questionSet[index];
}

int Hangman::getRemainingChances() const {
    int numAttemptsLeft = 0;

    if (currentMode == GameMode::CLASSIC) numAttemptsLeft = (MAX_CLASSIC_QUESTION_CHANCES - incorrectGuesses);;

    return numAttemptsLeft;
}

std::chrono::seconds Hangman::getRemainingTime() const {
    std::chrono::seconds remaining = std::chrono::seconds(0);

    if (currentMode == GameMode::TEST) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::seconds elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - gameStartTime);
        remaining = timeLimit - elapsed;
    }

    remaining = ((remaining > std::chrono::seconds(0)) ? remaining : std::chrono::seconds(0));

    return remaining;
}

bool Hangman::getHasPlayerWon() const {
    bool hasWon = false;

    if ((currentRound >= TOTAL_ROUNDS) && (currentScore > 0)) {
        if (currentMode == GameMode::CLASSIC) hasWon = true;  // Classic mode
        else if (currentMode == GameMode::TEST) hasWon = !isTimeExpired();  // Test mode
    }
    
    return hasWon;
}

bool Hangman::isNewHighScore() const {
    bool isNew = false;

    if (!isGameOver() || (currentScore <= 0)) isNew = false;

    try {
        Database db;
        db.open(currentDbPath);

        double highScore = db.getHighScore(currentPlayerId, categoryToString(currentCategory), gameModeToString(currentMode));

        db.close();
        isNew = currentScore > highScore;
    }
    catch (const DatabaseException& err) {
        throw HangmanException("Cannot check for new high score: " + std::string(err.what()));
    }

    return isNew;
}

std::string Hangman::categoryToString(Category cat) {
    std::string catStr = "";

    switch (cat) {
    case Category::CSC_111:
        catStr = "CSC_111";
        break;
    case Category::CSC_211:
        catStr = "CSC_211";
        break;
    case Category::CSC_231:
        catStr = "CSC_231";
        break;
    default:
        throw HangmanException("Invalid question category");
    }

    return catStr;
}

std::string Hangman::gameModeToString(GameMode mode) {
    std::string modeStr = "";

    switch (mode) {
    case GameMode::CLASSIC:
        modeStr = "Classic";
        break;
    case GameMode::TEST:
        modeStr = "Test";
        break;
    default:
        throw HangmanException("Invalid game mode");
    }

    return modeStr;
}

Hangman::~Hangman() { }

// HANGMAN_CPP
