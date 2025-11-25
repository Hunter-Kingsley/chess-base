#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"

#define WHITE 1
#define BLACK -1

constexpr int pieceSize = 80;

// Constants for ranks and files
constexpr uint64_t FILE_A = 0x0101010101010101ULL;
constexpr uint64_t FILE_B = FILE_A << 1;
constexpr uint64_t FILE_C = FILE_A << 2;
constexpr uint64_t FILE_D = FILE_A << 3;
constexpr uint64_t FILE_E = FILE_A << 4;
constexpr uint64_t FILE_F = FILE_A << 5;
constexpr uint64_t FILE_G = FILE_A << 6;
constexpr uint64_t FILE_H = FILE_A << 7;

constexpr uint64_t RANK_1 = 0x00000000000000FFULL;
constexpr uint64_t RANK_2 = 0x000000000000FF00ULL;
constexpr uint64_t RANK_3 = 0x0000000000FF0000ULL;
constexpr uint64_t RANK_4 = 0x00000000FF000000ULL;
constexpr uint64_t RANK_5 = 0x000000FF00000000ULL;
constexpr uint64_t RANK_6 = 0x0000FF0000000000ULL;
constexpr uint64_t RANK_7 = 0x00FF000000000000ULL;
constexpr uint64_t RANK_8 = 0xFF00000000000000ULL;

constexpr uint64_t NOT_FILE_A = ~FILE_A;
constexpr uint64_t NOT_FILE_B = ~FILE_B;
constexpr uint64_t NOT_FILE_G = ~FILE_G;
constexpr uint64_t NOT_FILE_H = ~FILE_H;
constexpr uint64_t NOT_FILE_AB = ~(FILE_A | FILE_B);
constexpr uint64_t NOT_FILE_GH = ~(FILE_G | FILE_H);
constexpr int negInfinite = -1000000;
constexpr int posInfinite = +1000000;

enum AllBitBoards
{
    WHITE_PAWNS,
    WHITE_KNIGHTS,
    WHITE_BISHOPS,
    WHITE_ROOKS,
    WHITE_QUEENS,
    WHITE_KING,
    WHITE_ALL_PIECES,
    BLACK_PAWNS,
    BLACK_KNIGHTS,
    BLACK_BISHOPS,
    BLACK_ROOKS,
    BLACK_QUEENS,
    BLACK_KING,
    BLACK_ALL_PIECES,
    OCCUPANCY,
    EMPTY_SQUARES,
    e_numBitboards
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    std::vector<BitMove> _moves;
    int _currentPlayer = 999;
    int _countMoves = 0;

    BitboardElement _knightBitboards[64];
    BitboardElement _kingBitboards[64];

    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    // void updateAI() override;
    int negamax(std::string& state, int depth, int playerColor);
    void addPawnBitboardMovesToList(std::vector<BitMove>& moves, const BitboardElement bitBoard, const int shift);
    void generatePawnMoveList(std::vector<BitMove>& moves, const BitboardElement Pawns, const BitboardElement emptySquares, const BitboardElement enemyPieces, char color);
    std::vector<BitMove> generateAllMoves();
    void generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, uint64_t emptySquares);
    BitboardElement generateKnightMoveBitboard(int square);
    void generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares);
    BitboardElement generateKingMoveBitboard(int square);
    void generateBishopMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, uint64_t occupancy, uint64_t friendlies);
    void generateRookMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, uint64_t occupancy, uint64_t friendlies);
    void generateQueenMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, uint64_t occupancy, uint64_t friendlies);

    BitboardElement _bitboards[e_numBitboards];
    int _bitboardLookup[128];
    Grid* _grid;
};