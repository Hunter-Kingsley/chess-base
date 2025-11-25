#include "Chess.h"
#include <limits>
#include <cmath>
#include "MagicBitboards.h"

Chess::Chess()
{
    _grid = new Grid(8, 8);

    initMagicBitboards();

    for(int i = 0; i < 128; i++) { _bitboardLookup[i] = 0; }

    _bitboardLookup['P'] = WHITE_PAWNS;
    _bitboardLookup['N'] = WHITE_KNIGHTS;
    _bitboardLookup['B'] = WHITE_BISHOPS;
    _bitboardLookup['R'] = WHITE_ROOKS;
    _bitboardLookup['Q'] = WHITE_QUEENS;
    _bitboardLookup['K'] = WHITE_KING;
    _bitboardLookup['p'] = BLACK_PAWNS;
    _bitboardLookup['n'] = BLACK_KNIGHTS;
    _bitboardLookup['b'] = BLACK_BISHOPS;
    _bitboardLookup['r'] = BLACK_ROOKS;
    _bitboardLookup['q'] = BLACK_QUEENS;
    _bitboardLookup['k'] = BLACK_KING;
    _bitboardLookup['0'] = EMPTY_SQUARES;
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    // Populate Knight Bitboards for each square
    for (int sq = 0; sq < 64; ++sq) {
    _knightBitboards[sq] = generateKnightMoveBitboard(sq);
    _kingBitboards[sq] = generateKingMoveBitboard(sq);
    }

    _currentPlayer = WHITE;
    _moves = generateAllMoves();
    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)

    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
            square->setBit(nullptr);
    });

    int y = 7;
    int x = 0;
    for (char character : fen) {
        if (character == '/') {
            y--;
            x = 0;
        } else if (isdigit(character)) {
            x += character - '0';
        } else {
            ChessPiece piece = Pawn;
            switch (toupper(character)) {
            case 'K':
                piece = King;
                break;
            case 'Q':
                piece = Queen;
                break;
            case 'R':
                piece = Rook;
                break;
            case 'N':
                piece = Knight;
                break;
            case 'B':
                piece = Bishop;
                break;
            case 'P':
                piece = Pawn;
                break;
            }
            Bit* bit = PieceForPlayer(isupper(character) ? 0 : 1, piece);
            ChessSquare *square = _grid->getSquare(x, y);
            bit->setPosition(square->getPosition());
            bit->setParent(square);
            bit->setGameTag(isupper(character) ? piece : (piece + 128));
            square->setBit(bit);
            x++;
        }
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;

    _grid->forEachSquare([](ChessSquare* sq, int x, int y) {
        sq->setHighlighted(false);
    });

    bool returnVal = false;
    ChessSquare* square = (ChessSquare *)&src;
    if (square) {
        int squareIndex = square->getSquareIndex();
        for (auto move : _moves) {
            if (move.from == squareIndex) {
                returnVal = true;
                auto dest = _grid->getSquareByIndex(move.to);
                dest->setHighlighted(true);
            }
        }
    }

    return returnVal;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srdsquare = (ChessSquare *)&src;
    ChessSquare* square = (ChessSquare *)&dst;
    if (square) {
        int squareIndex = square->getSquareIndex();
        for (auto move : _moves) {
            if (move.to == squareIndex && move.from == srdsquare->getSquareIndex()) {
                return true;
            }
        }
    }

    return false;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    _grid->forEachSquare([](ChessSquare* sq, int x, int y) {
        sq->setHighlighted(false);
    });
    
    _currentPlayer = _currentPlayer == BLACK ? WHITE : BLACK;
    _moves = generateAllMoves();
    endTurn();
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

void Chess::addPawnBitboardMovesToList(std::vector<BitMove>& moves, const BitboardElement bitBoard, const int shift) {
    if (bitBoard.getData() == 0) {
        return;
    }
    bitBoard.forEachBit([&](int toSquare){
        int fromSquare = toSquare - shift;
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });
}

void Chess::generatePawnMoveList(std::vector<BitMove>& moves, const BitboardElement Pawns, const BitboardElement emptySquares, const BitboardElement enemyPieces, char color) {
    if (Pawns.getData() == 0) {
        return;
    }

    // Single Moves
    BitboardElement singleMoves = (color == WHITE) ? (Pawns.getData() << 8) & emptySquares.getData() : (Pawns.getData() >> 8) & emptySquares.getData();
    //Double Moves
    BitboardElement doubleMoves = (color == WHITE) ? ((singleMoves.getData() & RANK_3) << 8) & emptySquares.getData() : ((singleMoves.getData() & RANK_6) >> 8) & emptySquares.getData();
    // Captures
    BitboardElement leftCaptures = (color == WHITE) ? ((Pawns.getData() & NOT_FILE_A) << 7) & enemyPieces.getData() : ((Pawns.getData() & NOT_FILE_A) >> 7) & enemyPieces.getData();
    BitboardElement rightCaptures =  (color == WHITE) ? ((Pawns.getData() & NOT_FILE_H) << 9) & enemyPieces.getData() : ((Pawns.getData() & NOT_FILE_H) >> 9) & enemyPieces.getData();

    int shiftForward = (color == WHITE) ? 8 : -8;
    int doubleShift = (color == WHITE) ? 16 : -16;
    int leftCaptureShift = (color == WHITE) ? 7 : -7;
    int rightCaptureShift = (color == WHITE) ? 9 : -9;

    addPawnBitboardMovesToList(moves, singleMoves, shiftForward);
    addPawnBitboardMovesToList(moves, doubleMoves, doubleShift);
    addPawnBitboardMovesToList(moves, leftCaptures, leftCaptureShift);
    addPawnBitboardMovesToList(moves, rightCaptures, rightCaptureShift);
}

BitboardElement Chess::generateKnightMoveBitboard(int square) {
    BitboardElement bitboard = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    std::pair<int, int> knightOffsets[] = {
        { 2, 1 }, { 2, -1 }, { -2, 1 }, { -2, -1 },
        { 1, 2 }, { 1, -2 }, { -1, 2 }, { -1, -2 }
    };

    constexpr uint64_t oneBit = 1;
    for (auto [dr, df] : knightOffsets) {
        int r = rank + dr, f = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            bitboard |= oneBit << (r * 8 + f);
        }
    }

    return bitboard;
}

void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, uint64_t emptySquares) {
    knightBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_knightBitboards[fromSquare].getData() & emptySquares);
        moveBitboard.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

BitboardElement Chess::generateKingMoveBitboard(int square) {
    BitboardElement bitboard = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    std::pair<int, int> kingOffsets[] = {
        { -1, 1 }, { 0, 1 }, { 1, 1 },
        {-1, 0 },            { 1, 0 },
        {-1, -1}, { 0, -1 }, { 1, -1 } 
    };

    constexpr uint64_t oneBit = 1;
    for (auto [dr, df] : kingOffsets) {
        int r = rank + dr, f = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            bitboard |= oneBit << (r * 8 + f);
        }
    }

    return bitboard;
}

void Chess::generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares) {
    kingBoard.forEachBit([&](int fromSquare){
        BitboardElement moveBitBoard = BitboardElement(_kingBitboards[fromSquare].getData() & emptySquares);
        moveBitBoard.forEachBit([&](int toSquare){
            moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

void Chess::generateBishopMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, uint64_t occupancy, uint64_t friendlies) {
    piecesBoard.forEachBit([&](int fromSquare){
        BitboardElement moveBitBoard = BitboardElement(getBishopAttacks(fromSquare, occupancy) & ~friendlies);
        moveBitBoard.forEachBit([&](int toSquare){
            moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

void Chess::generateRookMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, uint64_t occupancy, uint64_t friendlies) {
    piecesBoard.forEachBit([&](int fromSquare){
        BitboardElement moveBitBoard = BitboardElement(getRookAttacks(fromSquare, occupancy) & ~friendlies);
        moveBitBoard.forEachBit([&](int toSquare){
            moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

void Chess::generateQueenMoves(std::vector<BitMove>& moves, BitboardElement piecesBoard, uint64_t occupancy, uint64_t friendlies) {
    piecesBoard.forEachBit([&](int fromSquare){
        BitboardElement moveBitBoard = BitboardElement(getQueenAttacks(fromSquare, occupancy) & ~friendlies);
        moveBitBoard.forEachBit([&](int toSquare){
            moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

std::vector<BitMove> Chess::generateAllMoves()
{
    std::vector<BitMove> moves;
    moves.reserve(32);

    std::string state = stateString();

    for (int i = 0; i < e_numBitboards; i++) {
        _bitboards[i] = 0;
    }

    for (int i = 0; i < 64; i++) {
        int bitIndex = _bitboardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;
    }

    _bitboards[WHITE_ALL_PIECES] = _bitboards[WHITE_PAWNS].getData() |
    _bitboards[WHITE_KNIGHTS].getData() |
    _bitboards[WHITE_BISHOPS].getData() |
    _bitboards[WHITE_ROOKS].getData() |
    _bitboards[WHITE_QUEENS].getData() |
    _bitboards[WHITE_KING].getData();

    _bitboards[BLACK_ALL_PIECES] = _bitboards[BLACK_PAWNS].getData() |
    _bitboards[BLACK_KNIGHTS].getData() |
    _bitboards[BLACK_BISHOPS].getData() |
    _bitboards[BLACK_ROOKS].getData() |
    _bitboards[BLACK_QUEENS].getData() |
    _bitboards[BLACK_KING].getData();

    _bitboards[OCCUPANCY] = _bitboards[WHITE_ALL_PIECES].getData() | _bitboards[BLACK_ALL_PIECES].getData();
    int bitIndex = _currentPlayer == WHITE ? WHITE_PAWNS : BLACK_PAWNS;
    int oppBitIndex = _currentPlayer == WHITE ? BLACK_PAWNS : WHITE_PAWNS;

    generateKnightMoves(moves, _bitboards[WHITE_KNIGHTS + bitIndex], ~_bitboards[OCCUPANCY].getData());
    generatePawnMoveList(moves, _bitboards[WHITE_PAWNS + bitIndex], ~_bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + oppBitIndex].getData(), _currentPlayer);
    generateKingMoves(moves, _bitboards[WHITE_KING + bitIndex], ~_bitboards[OCCUPANCY].getData());
    generateBishopMoves(moves, _bitboards[WHITE_BISHOPS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generateRookMoves(moves, _bitboards[WHITE_ROOKS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generateQueenMoves(moves, _bitboards[WHITE_QUEENS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + bitIndex].getData());

    return moves;
}

// void Chess::updateAI() 
// {
//     int bestVal = negInfinite;
//     BitMove* bestMove = nullptr;
//     std::string state = stateString();

//     for(auto move : newMoves) {
//         int srcSquare = move.from;
//         int dstSquare = move.to;

//         char oldDst = state[dstSquare];
//         char srcPce = state[srcSquare];
//         state[dstSquare] = state[srcSquare];
//         state[srcSquare] = '0';
//         // Undo Move
//         int moveVal = negamax(state, 5, negInfinite);
//         state[dstSquare] = oldDst;
//         state[srcSquare] = srcPce;
//         // If the value of the current move is more than the best value, update best
//         if (moveVal > bestVal) {
//             bestMove = move;
//             bestVal = moveVal;
//         }
//     }
//     // Make the best move
// }

// int Chess::negamax(std::string& state, int depth, int playerColor) 
// {
//     _countMoves++;

//     if (depth == 0) {
//         return evaluateBoard(state) * playerColor;
//     }

//     auto newMoves = generateAllMoves(state, playerColor);

//     int bestVal = negamax(); // Min value

//     return bestVal;
// }

// #define FLIP(x) (x^56)

// int evaluateBoard(std::string state) {
//     int values[128];
//     values['P'] = 100;
//     values['N'] = 300;
//     values['B'] = 400;
//     values['R'] = 500;
//     values['Q'] = 900;
//     values['K'] = 2000;
//     values['p'] = -100;
//     values['n'] = -300;
//     values['b'] = -400;
//     values['r'] = -500;
//     values['q'] = -900;
//     values['k'] = -2000;

//     values['0'] = 0;
//     int score = 0;
//     for(char ch: state) {
//         score += values[ch];
//     }

//     return score;
// }