/*
Name: Emmanuel Rivas
ID: 15310887
Class: Fall 2024, CSC 211H
Date: 02/01/2025
Instructor: Dr. Azhar
Honors Project: Hangman
*/

#include "Database.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>

Database::Database() : db(nullptr) { }

void Database::initialize(const std::string& dbPath) {
    try {
        if (!databaseExists(dbPath)) {
            open(dbPath);
            createTables();

            std::string category = "";
            std::string filePath = "";

            for (const std::string& file : CATEGORY_FILES) {
                category = file.substr(0, file.find('.'));
                filePath = RESOURCES_FOLDER + "/" + file;

                this->loadQuestionsFromTSV(filePath, category);
            }

            close();
        }
    }
    catch (const std::exception& err) {
        throw DatabaseException("Failed to initialize database: " + std::string(err.what()));
    }
}

bool Database::databaseExists(const std::string& dbPath) const {
    return std::filesystem::exists(dbPath);
}

void Database::open(const std::string& dbPath) {
    if (isOpen()) {
        close();
    }

    int result = sqlite3_open(dbPath.c_str(), &db);
    checkError(result, "Opening database");
    currentDbPath = dbPath;

    executeQuery("PRAGMA foreign_keys = ON;");
}

void Database::close() {
    if (isOpen()) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool Database::isOpen() const {
    return db != nullptr;
}

void Database::createTables() {
    bool isTransactionActive = false;
    
    try {
        beginTransaction();
        isTransactionActive = true;

        const std::vector<std::string> createTableQueries = {
            // Game_Modes table - reference table for valid modes
            R"(CREATE TABLE IF NOT EXISTS Game_Modes (
                mode_id INTEGER PRIMARY KEY AUTOINCREMENT,
                mode_name TEXT NOT NULL UNIQUE CHECK(mode_name IN ('Classic', 'Test'))
            );)",

            // Insert valid modes
            R"(INSERT OR IGNORE INTO Game_Modes (mode_name) VALUES ('Classic'), ('Test');)",

            // Categories table - reference table for valid categories
            R"(CREATE TABLE IF NOT EXISTS Categories (
                category_id INTEGER PRIMARY KEY AUTOINCREMENT,
                category_name TEXT NOT NULL UNIQUE CHECK(category_name IN ('CSC_111', 'CSC_211', 'CSC_231'))
            );)",

            // Insert valid categories
            R"(INSERT OR IGNORE INTO Categories (category_name) VALUES ('CSC_111'), ('CSC_211'), ('CSC_231');)",

            // Players table - for player information
            R"(CREATE TABLE IF NOT EXISTS Players (
                player_id INTEGER PRIMARY KEY AUTOINCREMENT,
                player_name TEXT NOT NULL UNIQUE,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );)",

            // Questions table - for question storage
            R"(CREATE TABLE IF NOT EXISTS Questions (
                question_id INTEGER PRIMARY KEY AUTOINCREMENT,
                category_id INTEGER NOT NULL,
                question_text TEXT NOT NULL,
                answer_text TEXT NOT NULL,
                FOREIGN KEY(category_id) REFERENCES Categories(category_id) ON DELETE CASCADE,
                UNIQUE(category_id, question_text)
            );)",

            // Game_Sessions table - tracks individual game sessions
            R"(CREATE TABLE IF NOT EXISTS Game_Sessions (
                session_id INTEGER PRIMARY KEY AUTOINCREMENT,
                player_id INTEGER NOT NULL,
                category_id INTEGER NOT NULL,
                mode_id INTEGER NOT NULL,
                score REAL NOT NULL CHECK((score >= 0.00) AND (score <= 100.00)),
                played_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY(player_id) REFERENCES Players(player_id) ON DELETE CASCADE,
                FOREIGN KEY(category_id) REFERENCES Categories(category_id) ON DELETE CASCADE,
                FOREIGN KEY(mode_id) REFERENCES Game_Modes(mode_id) ON DELETE CASCADE
            );)",

            // High_Scores table - stores the current top 10 scores per category-mode combination
            R"(CREATE TABLE IF NOT EXISTS High_Scores (
                high_score_id INTEGER PRIMARY KEY AUTOINCREMENT,
                session_id INTEGER NOT NULL,
                rank INTEGER NOT NULL CHECK(rank BETWEEN 1 AND 10),
                FOREIGN KEY(session_id) REFERENCES Game_Sessions(session_id) ON DELETE CASCADE,
                UNIQUE(session_id)
            );)",

            // Indices for performance optimization
            "CREATE INDEX IF NOT EXISTS idx_game_sessions_category_mode_score ON Game_Sessions(category_id, mode_id, score DESC, played_at DESC);",
            "CREATE INDEX IF NOT EXISTS idx_game_sessions_player ON Game_Sessions(player_id);",
            "CREATE INDEX IF NOT EXISTS idx_game_sessions_player_score ON Game_Sessions(player_id, category_id, mode_id, score);",
            "CREATE INDEX IF NOT EXISTS idx_high_scores_session ON High_Scores(session_id);",
            "CREATE INDEX IF NOT EXISTS idx_questions_category ON Questions(category_id);"
        };

        for (const std::string& query : createTableQueries) {
            executeQuery(query);
        }

        commitTransaction();
        isTransactionActive = false;
    }
    catch (const std::exception& err) {
        if (isTransactionActive) rollbackTransaction();
        throw DatabaseException("Failed to create tables: " + std::string(err.what()));
    }
}

void Database::loadQuestionsFromTSV(const std::string& filePath, const std::string& categoryName) {
    if (!isOpen()) throw DatabaseException("Database is not open");

    std::ifstream file(filePath);

    if (!file.is_open()) throw DatabaseException("Unable to open TSV file: " + filePath);

    try {
        beginTransaction();

        // Get category_id
        const std::string categoryQuery = "SELECT category_id FROM Categories WHERE category_name = ?;";
        auto categoryStmt = prepareStatement(categoryQuery);

        bindText(categoryStmt.get(), 1, categoryName);

        if (sqlite3_step(categoryStmt.get()) != SQLITE_ROW) throw DatabaseException("Invalid category: " + categoryName);

        int categoryId = sqlite3_column_int(categoryStmt.get(), 0);

        // Prepare the insert statement
        const std::string insertQuery = "INSERT OR IGNORE INTO Questions (category_id, question_text, answer_text) VALUES (?, ?, ?);";
        auto insertStmt = prepareStatement(insertQuery);

        std::string line;
        while (std::getline(file, line)) {
            size_t tabPos = line.find('\t');

            if (tabPos == std::string::npos) continue;  // Skip malformed lines

            std::string question = line.substr(0, tabPos);
            std::string answer = line.substr(tabPos + 1);

            bindInt(insertStmt.get(), 1, categoryId);
            bindText(insertStmt.get(), 2, question);
            bindText(insertStmt.get(), 3, answer);

            int result = sqlite3_step(insertStmt.get());

            if (result != SQLITE_DONE) throw DatabaseException("Failed to insert question: " + getLastError());

            sqlite3_reset(insertStmt.get());
        }

        commitTransaction();
    }
    catch (const std::exception& err) {
        rollbackTransaction();
        throw DatabaseException("Failed to load questions: " + std::string(err.what()));
    }

    file.close();
}

std::vector<QuestionAnswer> Database::getQuestions(Category category) {
    std::string categoryName = Hangman::categoryToString(category);

    const std::string query =
        "SELECT q.question_text, q.answer_text "
        "FROM Questions q "
        "JOIN Categories c ON q.category_id = c.category_id "
        "WHERE c.category_name = ? "
        "ORDER BY RANDOM();";

    auto stmt = prepareStatement(query);
    bindText(stmt.get(), 1, categoryName);

    std::vector<QuestionAnswer> questions;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::string question = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        std::string answer = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));

        questions.emplace_back(question, answer);
    }

    return questions;
}

int Database::addPlayer(const std::string& playerName) {
    if (!isOpen()) throw DatabaseException("Database connection is not open");

    int playerId = -1;

    try {
        beginTransaction();

        // Check if player exists
        const std::string checkQuery = "SELECT player_id FROM Players WHERE player_name = ?;";

        auto checkStmt = prepareStatement(checkQuery);
        bindText(checkStmt.get(), 1, playerName);

        if (sqlite3_step(checkStmt.get()) == SQLITE_ROW) {
            playerId = sqlite3_column_int(checkStmt.get(), 0);
        }
        else {
            // Player doesn't exist, insert new player
            const std::string insertQuery = "INSERT INTO Players (player_name) VALUES (?);";

            auto insertStmt = prepareStatement(insertQuery);
            bindText(insertStmt.get(), 1, playerName);

            if (sqlite3_step(insertStmt.get()) != SQLITE_DONE) throw DatabaseException("Failed to insert player: " + getLastError());

            playerId = static_cast<int>(sqlite3_last_insert_rowid(db));
        }

        commitTransaction();
    }
    catch (const std::exception& err) {
        rollbackTransaction();
        throw DatabaseException("Failed to add player: " + std::string(err.what()));
    }

    if (playerId == -1) throw DatabaseException("Failed to get player ID after insertion");

    return playerId;
}

bool Database::playerExists(const std::string& playerName) {
    const std::string query = "SELECT 1 FROM Players WHERE player_name = ?;";

    auto stmt = prepareStatement(query);
    bindText(stmt.get(), 1, playerName);

    return sqlite3_step(stmt.get()) == SQLITE_ROW;
}

int Database::getPlayerId(const std::string& playerName) {
    if (!isOpen()) throw DatabaseException("Database connection is not open");

    const std::string query = "SELECT player_id FROM Players WHERE player_name = ?;";

    auto stmt = prepareStatement(query);
    bindText(stmt.get(), 1, playerName);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW) return sqlite3_column_int(stmt.get(), 0);

    throw DatabaseException("Player not found: " + playerName);
}

void Database::saveScore(int playerId, const std::string& categoryName, const std::string& modeName, double score) {
    if (!isOpen()) throw DatabaseException("Database connection is not open");

    bool isTransactionActive = false;

    try {
        beginTransaction();
        isTransactionActive = true;

        // Get category and mode IDs
        const std::string lookupQuery =
            "SELECT c.category_id, m.mode_id "
            "FROM Categories c, Game_Modes m "
            "WHERE c.category_name = ? AND m.mode_name = ?;";

        auto lookupStmt = prepareStatement(lookupQuery);

        bindText(lookupStmt.get(), 1, categoryName);
        bindText(lookupStmt.get(), 2, modeName);

        if (sqlite3_step(lookupStmt.get()) != SQLITE_ROW) throw DatabaseException("Invalid category or mode");

        int categoryId = sqlite3_column_int(lookupStmt.get(), 0);
        int modeId = sqlite3_column_int(lookupStmt.get(), 1);

        // Insert new game session
        const std::string sessionQuery =
            "INSERT INTO Game_Sessions (player_id, category_id, mode_id, score) "
            "VALUES (?, ?, ?, ?);";

        auto sessionStmt = prepareStatement(sessionQuery);

        bindInt(sessionStmt.get(), 1, playerId);
        bindInt(sessionStmt.get(), 2, categoryId);
        bindInt(sessionStmt.get(), 3, modeId);
        bindDouble(sessionStmt.get(), 4, score);

        if (sqlite3_step(sessionStmt.get()) != SQLITE_DONE) throw DatabaseException("Failed to save game session");

        int sessionId = static_cast<int>(sqlite3_last_insert_rowid(db));

        // Update high scores
        updateHighScores(sessionId, categoryId, modeId);

        commitTransaction();
        isTransactionActive = false;
    }
    catch (const std::exception& err) {
        if (isTransactionActive) rollbackTransaction();
        throw DatabaseException("Failed to save score: " + std::string(err.what()));
    }
}

void Database::updateHighScores(int sessionId, int categoryId, int modeId) {
    try {
        // Check if the new score ranks in the top 10
        const std::string rankQuery =
            "WITH RankedScores AS ("
            "    SELECT "
            "        gs.session_id, "
            "        DENSE_RANK() OVER ("
            "            PARTITION BY gs.category_id, gs.mode_id "
            "            ORDER BY gs.score DESC, gs.played_at DESC"
            "        ) AS rank "
            "    FROM Game_Sessions gs "
            "    WHERE gs.category_id = ? AND gs.mode_id = ?"
            ") "
            "SELECT rank FROM RankedScores WHERE session_id = ?;";

        auto rankStmt = prepareStatement(rankQuery);

        bindInt(rankStmt.get(), 1, categoryId);
        bindInt(rankStmt.get(), 2, modeId);
        bindInt(rankStmt.get(), 3, sessionId);

        if (sqlite3_step(rankStmt.get()) == SQLITE_ROW) {
            int rank = sqlite3_column_int(rankStmt.get(), 0);

            if (rank <= 10) {
                // Get the player ID and new score
                const std::string scoreQuery =
                    "SELECT gs.player_id, gs.score "
                    "FROM Game_Sessions gs "
                    "WHERE gs.session_id = ?;";

                auto scoreStmt = prepareStatement(scoreQuery);
                bindInt(scoreStmt.get(), 1, sessionId);

                if (sqlite3_step(scoreStmt.get()) != SQLITE_ROW) throw DatabaseException("Could not find new score session");

                int playerId = sqlite3_column_int(scoreStmt.get(), 0);
                double newScore = sqlite3_column_double(scoreStmt.get(), 1);

                // Remove any duplicate scores in the same category & mode from the same player
                const std::string removeDuplicateQuery =
                    "DELETE FROM High_Scores "
                    "WHERE session_id IN ("
                    "    SELECT hs.session_id "
                    "    FROM High_Scores hs "
                    "    JOIN Game_Sessions gs ON hs.session_id = gs.session_id "
                    "    WHERE gs.player_id = ? "
                    "    AND gs.category_id = ? "
                    "    AND gs.mode_id = ? "
                    "    AND gs.score = ? "
                    "    AND gs.session_id < ?"
                    ");";

                auto removeDuplicateStmt = prepareStatement(removeDuplicateQuery);

                bindInt(removeDuplicateStmt.get(), 1, playerId);
                bindInt(removeDuplicateStmt.get(), 2, categoryId);
                bindInt(removeDuplicateStmt.get(), 3, modeId);
                bindDouble(removeDuplicateStmt.get(), 4, newScore);
                bindInt(removeDuplicateStmt.get(), 5, sessionId);

                if (sqlite3_step(removeDuplicateStmt.get()) != SQLITE_DONE) throw DatabaseException("Failed to remove duplicate scores");

                // Insert the new high score
                const std::string insertQuery = "INSERT INTO High_Scores (session_id, rank) VALUES (?, ?);";

                auto insertStmt = prepareStatement(insertQuery);

                bindInt(insertStmt.get(), 1, sessionId);
                bindInt(insertStmt.get(), 2, rank);

                if (sqlite3_step(insertStmt.get()) != SQLITE_DONE) throw DatabaseException("Failed to insert high score");

                // Remove scores that are now outside the top 10
                const std::string cleanupQuery =
                    "WITH RankedScores AS ("
                    "    SELECT "
                    "        hs.high_score_id, "
                    "        DENSE_RANK() OVER ("
                    "            PARTITION BY gs.category_id, gs.mode_id "
                    "            ORDER BY gs.score DESC, gs.played_at DESC"
                    "        ) AS new_rank "
                    "    FROM High_Scores hs "
                    "    JOIN Game_Sessions gs ON hs.session_id = gs.session_id "
                    "    WHERE gs.category_id = ? AND gs.mode_id = ?"
                    ") "
                    "DELETE FROM High_Scores "
                    "WHERE high_score_id IN ("
                    "    SELECT high_score_id FROM RankedScores WHERE new_rank > 10"
                    ");";

                auto cleanupStmt = prepareStatement(cleanupQuery);

                bindInt(cleanupStmt.get(), 1, categoryId);
                bindInt(cleanupStmt.get(), 2, modeId);

                if (sqlite3_step(cleanupStmt.get()) != SQLITE_DONE) throw DatabaseException("Failed to cleanup old high scores");
            }
        }
        else {
            throw DatabaseException("Query for the rank of the new score failed");
        }
    }
    catch (const std::exception& err) {
        throw DatabaseException("Failed to update high scores: " + std::string(err.what()));
    }
}

double Database::getHighScore(int playerId, const std::string& categoryName, const std::string& modeName) {
    const std::string query =
        "SELECT gs.score "
        "FROM Game_Sessions gs "
        "JOIN Categories c ON gs.category_id = c.category_id "
        "JOIN Game_Modes m ON gs.mode_id = m.mode_id "
        "JOIN High_Scores hs ON gs.session_id = hs.session_id "
        "WHERE gs.player_id = ? AND c.category_name = ? AND m.mode_name = ? "
        "ORDER BY gs.score DESC "
        "LIMIT 1;";

    auto stmt = prepareStatement(query);

    bindInt(stmt.get(), 1, playerId);
    bindText(stmt.get(), 2, categoryName);
    bindText(stmt.get(), 3, modeName);

    if (sqlite3_step(stmt.get()) == SQLITE_ROW) return sqlite3_column_double(stmt.get(), 0);

    return 0.0;
}

std::vector<std::string> Database::getHighScores(const std::string& categoryName, const std::string& modeName) {
    std::vector<std::string> formattedScores;
    bool isTransactionActive = false;
    const std::string query =
        "WITH RankedScores AS ("
        "    SELECT "
        "        p.player_name, "
        "        gs.score, "
        "        DENSE_RANK() OVER (ORDER BY gs.score DESC) as dense_rank, "
        "        ROW_NUMBER() OVER (ORDER BY gs.score DESC, gs.played_at DESC) as sequential_rank, "
        "        COUNT(*) OVER (PARTITION BY gs.score) as tie_count, "
        "        ROW_NUMBER() OVER (PARTITION BY gs.score ORDER BY gs.played_at DESC) as tiebreaker "
        "    FROM High_Scores hs "
        "    JOIN Game_Sessions gs ON hs.session_id = gs.session_id "
        "    JOIN Players p ON gs.player_id = p.player_id "
        "    JOIN Categories c ON gs.category_id = c.category_id "
        "    JOIN Game_Modes m ON gs.mode_id = m.mode_id "
        "    WHERE c.category_name = ? AND m.mode_name = ? "
        ") "
        "SELECT "
        "    player_name, "
        "    score, "
        "    dense_rank, "
        "    sequential_rank, "
        "    tie_count, "
        "    tiebreaker "
        "FROM RankedScores "
        "ORDER BY score DESC, tiebreaker ASC "
        "LIMIT 10;";

    try {
        beginTransaction();
        isTransactionActive = true;

        auto stmt = prepareStatement(query);

        bindText(stmt.get(), 1, categoryName);
        bindText(stmt.get(), 2, modeName);

        while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
            double score = sqlite3_column_double(stmt.get(), 1);
            int sequentialRank = sqlite3_column_int(stmt.get(), 3);
            int tieCount = sqlite3_column_int(stmt.get(), 4);

            std::ostringstream ss;

            ss << std::setw(2) << sequentialRank << ". "
                << std::left << std::setw(20) << name
                << std::right << std::fixed << std::setprecision(2) << score;

            // Add tie indicator if there are ties for this score
            if (tieCount > 1) {
                ss << " (tie)";
            }

            formattedScores.push_back(ss.str());
        }

        commitTransaction();
        isTransactionActive = false;
    }
    catch (const std::exception& err) {
        if (isTransactionActive) rollbackTransaction();
        throw DatabaseException("Failed to get formatted high scores: " + std::string(err.what()));
    }

    return formattedScores;
}

std::vector<std::pair<std::string, double>> Database::getTopScores(const std::string& categoryName, const std::string& modeName, int limit) {
    const std::string query =
        "SELECT p.player_name, gs.score "
        "FROM High_Scores hs "
        "JOIN Game_Sessions gs ON hs.session_id = gs.session_id "
        "JOIN Players p ON gs.player_id = p.player_id "
        "JOIN Categories c ON gs.category_id = c.category_id "
        "JOIN Game_Modes m ON gs.mode_id = m.mode_id "
        "WHERE c.category_name = ? AND m.mode_name = ? "
        "ORDER BY gs.score DESC, gs.played_at ASC "
        "LIMIT ?;";

    auto stmt = prepareStatement(query);

    bindText(stmt.get(), 1, categoryName);
    bindText(stmt.get(), 2, modeName);
    bindInt(stmt.get(), 3, limit);

    std::vector<std::pair<std::string, double>> scores;

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        double score = sqlite3_column_double(stmt.get(), 1);

        scores.emplace_back(name, score);
    }

    return scores;
}

int Database::executeQuery(const std::string& query) {
    char* errMsg = nullptr;
    int result = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        std::string error = (errMsg ? errMsg : "Unknown error");
        sqlite3_free(errMsg);

        throw DatabaseException("Query execution failed: " + error);
    }

    return result;
}

bool Database::tableExists(const std::string& tableName) {
    const std::string query =
        "SELECT name FROM sqlite_master "
        "WHERE type='table' AND name=?;";

    auto stmt = prepareStatement(query);
    bindText(stmt.get(), 1, tableName);

    return sqlite3_step(stmt.get()) == SQLITE_ROW;
}

void Database::beginTransaction() {
    executeQuery("BEGIN TRANSACTION;");
}

void Database::commitTransaction() {
    executeQuery("COMMIT TRANSACTION;");
}

void Database::rollbackTransaction() {
    if (isOpen()) {
        executeQuery("ROLLBACK TRANSACTION;");
    }
}

Database::StmtPtr Database::prepareStatement(const std::string& query) {
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

    checkError(result, "Preparing statement");

    return StmtPtr(stmt, sqlite3_finalize);
}

void Database::checkError(int result, const std::string& operation) {
    if (result != SQLITE_OK) throw DatabaseException(operation + " failed: " + getLastError());
}

std::string Database::getLastError() const {
    std::string lastError = "";
    lastError = std::string(sqlite3_errmsg(db) ? sqlite3_errmsg(db) : "Unknown error");

    return lastError;
}

void Database::bindText(sqlite3_stmt* stmt, int index, const std::string& value) {
    int result = sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_STATIC);
    checkError(result, "Binding text parameter");
}

void Database::bindInt(sqlite3_stmt* stmt, int index, int value) {
    int result = sqlite3_bind_int(stmt, index, value);
    checkError(result, "Binding integer parameter");
}

void Database::bindDouble(sqlite3_stmt* stmt, int index, double value) {
    int result = sqlite3_bind_double(stmt, index, value);
    checkError(result, "Binding double parameter");
}

Database::~Database() {
    close();
}

// DATABASE_CPP
