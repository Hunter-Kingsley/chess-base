#pragma once

#include <array>
#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"
#include "GameState.h"

constexpr int pieceSize = 80;

constexpr int negInfinite = -1000000;
constexpr int posInfinite = +1000000;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;
    GameState currGameState;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    void updateAI() override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

    // Tournament support methods
    void setBoardFromFEN(const std::string& fen);
    BitMove getLastAIMove() const { return _lastAIMove; }
    std::string getFEN() const;

    // Get current player color (WHITE=1, BLACK=-1)
    int getCurrentPlayerColor() const { return _currentPlayer; }

    // you can make this variable private, it's just grouped with the public methods for convenience
    BitMove _lastAIMove;  // Stores the last move calculated by AI (for tournament)

private:
    std::vector<BitMove> _moves;
    int _currentPlayer = 999;
    int _countMoves = 0;

    BitBoard _knightBitboards[64];
    BitBoard _kingBitboards[64];

    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    bool gameHasAI() override { return true; }
    int negamax(GameState& gameState, int depth, int alpha, int beta);
    int evaluateBoard(const GameState& gamestate);

    BitBoard _bitboards[e_numBitboards];
    int _bitboardLookup[128];
    Grid* _grid;
};