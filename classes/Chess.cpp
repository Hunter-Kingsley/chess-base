#include "Chess.h"
#include <limits>
#include <cmath>
#include <chrono>
#include <iomanip>
#include "MagicBitboards.h"
#include "PieceSquare.h"
#include "GameState.h"
#include "ChessSquare.h"

static int actualPS[128][64];
#define FLIP(x) (x^56)

static const std::array<int, 128> evaluateScores = []() {
    std::array<int, 128> scores {};
    scores['P'] = 100;
    scores['N'] = 320;
    scores['B'] = 330;
    scores['R'] = 500;
    scores['Q'] = 900;
    scores['K'] = 20000;
    scores['p'] = -scores['P'];
    scores['n'] = -scores['N'];
    scores['b'] = -scores['B'];
    scores['r'] = -scores['R'];
    scores['q'] = -scores['Q'];
    scores['k'] = -scores['K'];
    return scores;
}();

static const std::array<int *, 128> pieceSquareTables = []() {
    std::array<int *, 128> pieceSquare{};
    pieceSquare['P'] = (int* )&pawnTableWhite;
    pieceSquare['N'] = (int* )&knightTableWhite;
    pieceSquare['B'] = (int* )&bishopTableWhite;
    pieceSquare['R'] = (int* )&rookTableWhite;
    pieceSquare['Q'] = (int* )&queenTableWhite;
    pieceSquare['K'] = (int* )&kingTableWhite;
    pieceSquare['0'] = (int* )&emptyTable;

    pieceSquare['p'] = (int* )&pawnTableBlack;
    pieceSquare['n'] = (int* )&knightTableBlack;
    pieceSquare['b'] = (int* )&bishopTableBlack;
    pieceSquare['r'] = (int* )&rookTableBlack;
    pieceSquare['q'] = (int* )&queenTableBlack;
    pieceSquare['k'] = (int* )&kingTableBlack;
    return pieceSquare;
}();

Chess::Chess()
{
    _grid = new Grid(8, 8);

    std::memset(actualPS, 0, sizeof(actualPS));
    const char pieces[] = {'P', 'N', 'B', 'R', 'Q', 'K'};
    for (int p = 0; p < 6; p++) {
        int score = evaluateScores[pieces[p]];
        for (int sq = 0; sq < 64; sq++) {
            int finalW = pieceSquareTables[pieces[p]][sq] + score;
            int finalB = pieceSquareTables[tolower(pieces[p])][sq] - score;
            actualPS[p + WHITE_PAWNS][sq] = finalW;
            actualPS[p + BLACK_PAWNS][sq] = finalB;
            actualPS[pieces[p]][sq] = finalW;
            actualPS[tolower(pieces[p])][sq] = finalB;
        }
    }
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

    _currentPlayer = WHITE;
    currGameState.init(stateString().c_str(), _currentPlayer);
    _moves = currGameState.generateAllMoves();

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }

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
    // Logic for pawn promotion
    int tag = bit.gameTag();
    bool isPawn = (tag & 127) == Pawn;
    if (isPawn) {
        ChessSquare* sq = dynamic_cast<ChessSquare*>(&dst);
        if (sq) {
            int row = sq->getRow();
            if ((tag < 128 && row == 7) || (tag >= 128 && row == 0)) {
                int playerNumber = (tag >= 128) ? 1 : 0;
                Bit* promoted = PieceForPlayer(playerNumber, Queen);
                promoted->setGameTag((playerNumber == 0) ? Queen : (Queen + 128));
                dst.setBit(promoted);
                promoted->setPosition(dst.getPosition());
            }
        }
    }

    _currentPlayer = _currentPlayer == WHITE ? BLACK : WHITE;
    
    // Logic for Castling
    ChessSquare* srcSq = dynamic_cast<ChessSquare*>(&src);
    ChessSquare* dstSq = dynamic_cast<ChessSquare*>(&dst);
    if (srcSq && dstSq) {
        int srcCol = srcSq->getColumn();
        int dstCol = dstSq->getColumn();
        int srcRow = srcSq->getRow();
        int dstRow = dstSq->getRow();
        int tag = bit.gameTag();
        if ((tag & 127) == King && srcRow == dstRow && abs(dstCol - srcCol) == 2) {
            int dstIdx = dstSq->getSquareIndex();
            if (dstCol > srcCol) {
                // King-side castling
                int rookFrom = dstIdx + 1;
                int rookTo = dstIdx - 1;
                if (rookFrom >= 0 && rookFrom < 64 && rookTo >= 0 && rookTo < 64) {
                    BitHolder& rsrc = getHolderAt(rookFrom & 7, rookFrom / 8);
                    BitHolder& rdst = getHolderAt(rookTo & 7, rookTo / 8);
                    Bit* rookBit = rsrc.bit();
                    if (rookBit) {
                        rdst.dropBitAtPoint(rookBit, ImVec2(0, 0));
                        rsrc.setBit(nullptr);
                        rookBit->setPosition(rdst.getPosition());
                    }
                }
            } else {
                // Queen-side castling
                int rookFrom = dstIdx - 2;
                int rookTo = dstIdx + 1;
                if (rookFrom >= 0 && rookFrom < 64 && rookTo >= 0 && rookTo < 64) {
                    BitHolder& rsrc = getHolderAt(rookFrom & 7, rookFrom / 8);
                    BitHolder& rdst = getHolderAt(rookTo & 7, rookTo / 8);
                    Bit* rookBit = rsrc.bit();
                    if (rookBit) {
                        rdst.dropBitAtPoint(rookBit, ImVec2(0, 0));
                        rsrc.setBit(nullptr);
                        rookBit->setPosition(rdst.getPosition());
                    }
                }
            }
        }
    }

    currGameState.init(stateString().c_str(), _currentPlayer);
    _moves = currGameState.generateAllMoves();
    _grid->forEachSquare([](ChessSquare* sq, int x, int y) {
        sq->setHighlighted(false);
    });
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
    return s;
}

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

void Chess::updateAI() 
{
    int bestVal = negInfinite;
    BitMove bestMove;
    std::string state = stateString();
    const auto searchStart = std::chrono::steady_clock::now();
    _countMoves = 0;

    for(auto move : _moves) {
        int srcSquare = move.from;
        int dstSquare = move.to;

        char oldDst = state[dstSquare];
        char srcPce = state[srcSquare];
        state[dstSquare] = srcPce;
        state[srcSquare] = '0';
        int moveVal = -negamax(currGameState, 6, negInfinite, posInfinite);
        // Undo Move
        state[dstSquare] = oldDst;
        state[srcSquare] = srcPce;
        // If the value of the current move is more than the best value, update best
        if (moveVal > bestVal) {
            bestMove = move;
            bestVal = moveVal;
        }
    }

    // Make the best move
    if(bestVal != negInfinite) {
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - searchStart).count();
        const double boardsPerSecond = seconds > 0.0 ? static_cast<double>(_countMoves) / seconds : 0.0;
        std::cout << "Moves checked: " << _countMoves
                << " (" << std::fixed << std::setprecision(2) << boardsPerSecond
                << " boards/s)" << std::defaultfloat << std::endl;
        int srcSquare = bestMove.from;
        int dstSquare = bestMove.to;
        BitHolder& src = getHolderAt(srcSquare&7, srcSquare/8);
        BitHolder& dst = getHolderAt(dstSquare&7, dstSquare/8);
        Bit* bit = src.bit();
        dst.dropBitAtPoint(bit, ImVec2(0, 0));
        src.setBit(nullptr);
        bitMovedFromTo(*bit, src, dst);
    }


}

int Chess::negamax(GameState& gameState, int depth, int alpha, int beta) 
{
    _countMoves++;

    if (depth == 0) {
        return evaluateBoard(gameState);
    }

    auto newMoves = gameState.generateAllMoves();

    int bestVal = negInfinite; // std::max(bestVal, -negamax(state, depth - 1, -playerColor));

    for(const auto& move : newMoves) {
        gameState.pushMove(move);
        bestVal = std::max(bestVal, -negamax(gameState, depth - 1, -beta, -alpha));
        // Undo Move
        gameState.popState();
        // alpha/beta cut off
        alpha = std::max(alpha, bestVal);
        if (alpha >= beta) {
            break;
        }
    }
    return bestVal;
}

int Chess::evaluateBoard(const GameState& gamestate) {
    int value = 0;
    for (int square = 0; square < 64; ++square) {
        const unsigned char piece = (gamestate.state[square]);
        value += actualPS[piece][square];
    }
    return value * gamestate.color;
}