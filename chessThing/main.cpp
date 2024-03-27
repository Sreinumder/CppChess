#include <iostream>
#include<iomanip>
#include<vector>
#include<string>
#include<ctime>
#include<cstdio>
#include<conio.h>
#include<Windows.h>
#include<utility>
#include <type_traits>


struct key {
    std::string function;
    char keys[2];
};
typedef std::vector<key> keys;

const int playerKeysSize = 14;
key playerKeys[playerKeysSize] = { {"Enter",{'5','f'}},{"Up",{'8','w'}},{"Down",{'2','s'}},{"Left",{'4','a'}},{"Right",{'6','d'}},
    {"UL",{'7','q'}},{"UR" ,{'9','e'}},{"DL",{'1','z'}},{"DR",{'3','c'}},
    {"Undo",{'u','u'}},{"Redo",{'i','i'}},{"Reload",{'r','r'}}, {"Help",{'h','h'}}, {"2Ctrl",{'t','t'}} };

const wchar_t* keysymbols[14] = {L"⏎", L"↑" ,L"↓" ,L"←",L"→",L"↖" ,L"↗",L"↙",L"↘",L"↶",L"↷",L"⟳",L"⇄",L"⇄" };

//debug
bool vulnerableDebug = false;
bool moveLog = false;


bool twoController = false;
bool showHelp = true;

//scale and size of grid
int singleSquareHeight = 1; //odd numbers gives correct orientation
int singleSquareWidth = 2 * singleSquareHeight + 1;//odd numbers starting from 3
const int boardH = 8, boardW = 8;

//starts board at
const int pos_x = 34, pos_y = 3;
const int disx = 13;//distance between movelog and board

//representation of Pieces
const char dark = char(254);//■
const wchar_t* whiteChessPiece[7] = { L" ", L"♔", L"♕", L"♗",L"♘", L"♖", L"♙" };
const wchar_t* blackChessPiece[7] = { L" ", L"♚", L"♛", L"♝",L"♞", L"♜", L"♟" };
const char pieceTypeName[] = { ' ', 'K', 'Q', 'B', 'N', 'R', 'P'};

const char horPos[] = { 'a','b','c','d','e','f','g','h' };
const int pawnSpawn[2] = { boardH - 2, 1 };

enum colorCode { Black, Blue, Green, Cyan, Red, Purple, Yellow, Default_white, Grey, Bright_blue, Bright_green, Bright_cyan, Bright_red, pink, yellow, Bright_white };
colorCode cursorColor[2] = { Bright_blue, Yellow };

colorCode moveableColor = Bright_green, selectedColor = Cyan,
checkColor = Purple, captureColor = Bright_red,
castleColor = pink, checkmateColor = yellow,
DefaultColor = Black, availableMoveColor = Blue,
promotionColor = Bright_blue;

enum class pieceColor { WHITE, BLACK, EMPTY };
enum class pieceType { EMPTY, KING, QUEEN, BISHOP, KNIGHT, ROOK, PAWN};


template <typename E>
int castEnumToInt(E e) noexcept//returns enum to int cast
{
    int i = -1;
    while (static_cast<E>(++i) != e);
    return i;
}


void wout(const wchar_t* c) {//prints out wide characters
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    // explicitly call the wide version (which always accepts UTF-16)
    WriteConsoleW(handle, c, 1, &written, NULL);
}
void gotoxy(int x, int y) {
    COORD c = { (short)x, (short)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}


inline void highlight(colorCode color) {
    // Set console color
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), castEnumToInt(color) | 0xF0);
    // text color from 0x1-0x9
    // text background color from 0x10-0x90   
}

struct chessPiece {
    int x, y;
    pieceColor color;
    pieceType piece;
    chessPiece() {}
    chessPiece(int a, int b, pieceColor pc = pieceColor::EMPTY, pieceType p = pieceType::EMPTY) {
        x = a; y = b;
        color = pc; piece = p;
    }
    chessPiece(chessPiece* another) {
        x = another->x;         y = another->y;
        color = another->color; piece = another->piece;
    }
    void setPiece(pieceColor c, pieceType p) {
        color = c; piece = p;
    }
};
typedef std::vector<chessPiece> setOfChessPiece;


std::string squareName(int x, int y) { return horPos[x]+ std::to_string(-y + 8); }

struct move {
    chessPiece from, to;

    bool capture = false;
    bool check = false, checkmate = false, stalemate = false;
    bool enPassent = false, castle = false;//if the move is a castle itself
    bool pawnPromotion = false;
    pieceType promotedTo;


    bool castleLeftToggle = false, castleRightToggle = false;//if a move changes the castleablity to left or right
    move() {};
    move(chessPiece x, chessPiece y) : from(x), to(y) {}


    friend class chessBoard;
    //flag changing functions that can be chainged//move a.captures().checks().enpassent();
    move& captures() { capture = true;    return *this; }
    move& checks() { check = true;      return *this; }
    move& checkmates() { checkmate = true;  return *this; }
    move& stalemates() { stalemate = true; return *this; }
    move& enPassents() { enPassent = true;  return *this; }
    move& castles() { castle = true;     return *this; }
    move& promotes() { pawnPromotion = true;  return *this;}
    move& setPawnPromoteTo(pieceType p) { promotedTo = p; return *this; }
    
    move& toggleCastleLeft() { castleLeftToggle = true; return *this; }
    move& toggleCastleRight() { castleRightToggle = true; return *this; }

    void printMove() {
        highlight(DefaultColor);

        if (check) highlight(checkColor);
        else if (capture || enPassent) highlight(captureColor);
        else if (castle) highlight(castleColor);
        else if (pawnPromotion) highlight(promotionColor);
        if (!castle && !checkmate && !pawnPromotion) {
            if (to.piece != pieceType::PAWN) std::cout << pieceTypeName[castEnumToInt(from.piece)];
            std::cout << squareName(from.x, from.y);
        }

        if (castle) {
            if (from.x - to.x < 0) {//kingside castling
                std::cout << "o-o";
            }
            else if (from.x - to.x > 0) {//queenside castling
                std::cout << "o-o-o";
            }
        }
        if (checkmate)std::cout << 'X';
        else if (check) std::cout << '+';
        if (capture) std::cout << 'x';


        if (!castle && !checkmate) {
            if(to.piece!=pieceType::PAWN)std::cout << pieceTypeName[castEnumToInt(to.piece)];
            std::cout << squareName(to.x, to.y);
        }
        if (pawnPromotion) { std::cout << "=(" << pieceTypeName[castEnumToInt(promotedTo)] << ")"; }
        
        highlight(DefaultColor);
    }
    
    void operator =(move f) {
        from = f.from;
        to = f.to;
        capture = f.capture;
        check = f.capture; checkmate = f.checkmate;
        enPassent = f.enPassent; castle = f.castle;
        castleLeftToggle = f.castleLeftToggle; castleRightToggle = f.castleRightToggle;
    }
};
move moveFromTo(chessPiece x, chessPiece y) {
    move m(x, y);
    return m;
}
typedef std::vector<move> moveset;

class chessBoard;
class player {
    char name[40];
    chessPiece king;
    std::vector<pieceType> capturedPiece;
    bool castleLeft, castleRight;
    clock_t timeRemaining;
public:
    player() {
        name[40] = {};
        capturedPiece = {};
        capturedPiece.reserve(16);
        castleLeft = true;  castleRight=true;
    }
    friend class chessBoard;
};

static bool inVulnerable = false;
static bool inRedo = false;
class chessBoard {
    chessPiece board[8][8];
    moveset gameMoveset, moveable;
    unsigned int currentlyOnGameMovesetIndex;
    int currentx, currenty;
    int selectedx, selectedy;
    bool hasSelected;
    player player[2];
    pieceColor currentTurn;//WHITE OR BLACK
public:
    int currentturn() { return castEnumToInt(currentTurn); }
    chessBoard() {
        moveable = {};
        moveable.reserve(32);
        gameMoveset.reserve(128);
        currentx = 4; currenty = 6;
        selectedx = -1; selectedy = -1;
        currentTurn = pieceColor::WHITE;
        currentlyOnGameMovesetIndex = 0;
        hasSelected = false;
        //to check the position of captured pieces
        /*for (int i = 0; i < 16; i++) { 
            player[0].capturedPiece.push_back(static_cast<pieceType>(i % 6 + 1));
            player[1].capturedPiece.push_back(static_cast<pieceType>(i % 6 + 1));
        }*/
        //We track the kings for check and checkmate and checkmate
        //and rooks for castling
        for (int j = 0; j < boardH; j++) {
            for (int i = 0; i < boardW; i++) {
                if (j == 0 || j == 7) {
                    pieceColor c = pieceColor::WHITE;
                    if (j == 0) c = pieceColor::BLACK;
                    if (i == 0 || i == 7)       board[i][j] = { i, j, c, pieceType::ROOK };
                    else if (i == 1 || i == 6)  board[i][j] = { i, j, c, pieceType::KNIGHT };
                    else if (i == 2 || i == 5)  board[i][j] = { i, j, c, pieceType::BISHOP };
                    else if (i == 3)            board[i][j] = { i, j, c, pieceType::QUEEN };
                    else if (i == 4) {
                                                board[i][j] = { i, j, c, pieceType::KING };
                                                player[castEnumToInt(c)].king = board[i][j];
                    }
                    else                        board[i][j] = { i, j };
                }
                else if (j == 1)board[i][j] = { i, j, pieceColor::BLACK, pieceType::PAWN };
                else if (j == 6)board[i][j] = { i, j, pieceColor::WHITE, pieceType::PAWN };
                else            board[i][j] = { i, j };
            }
        }
    }

    void updateBoardSquare(int x, int y, colorCode c = DefaultColor) {
        char ULC = char(218), URC = char(191), DLC = char(192), DRC = char(217), FJ = char(197), HL=char(196), VL = char(179);
        char ul = char(197), ur = char(197), dl = char(197), dr = char(197);//┌ ┐└ ┘218 191 192 217 // 197┼ 
        //┬ ├ ┤ ┴ 194 195 180 193 ─ 196 │ 179
        char u = char(196), d = char(196);//─
        char l = char(179), r = char(179);//│

        if (y == 0) {
            ul = char(194);//┬
            ur = char(194);//┬
            if (x == 0) {
                ul = char(218);//┌
                dl = char(195);//├
            }
            else if (x == boardW - 1) {
                ur = char(191);//┐
                dr = char(180);//┤
            }
        }
        else if (y == boardH - 1) {
            dl = char(193);//┴
            dr = char(193);//┴
            if (x == 0) {
                ul = char(195);//├
                dl = char(192);//└
            }
            else if (x == boardW - 1) {
                ur = char(180);//┤
                dr = char(217);//┘
            }
        }
        else {
            if (x == 0) {
                ul = char(195);//├
                dl = char(195);//├
            }
            else if (x == boardW - 1) {
                ur = char(180);//┤
                dr = char(180);//┤
            }
        }
        int px, py, dy;
        px = pos_x + x * (singleSquareWidth + 1); py = pos_y + y * (singleSquareHeight + 1);

        gotoxy(px, py); {//top line
            highlight(c);
            std::cout << ul; for (int i = 0; i < singleSquareWidth; i++) std::cout << u; std::cout << ur;
        }
        for (int j = 1; j <= singleSquareHeight; j++) {//middle section
            gotoxy(px, py + j); {//left side
                highlight(c);
                std::cout << l;
                if (x == selectedx && y == selectedy && c!=DefaultColor) std::cout << '<';
                else if (x == currentx && y == currenty && c != DefaultColor) {
                    if(isInMoveset(moveable, currentx, currenty)) std::cout << '>';
                    else if(board[x][y].color == currentTurn) std::cout << '>';
                    else {//invalid input
                        highlight(Grey);
                        std::cout << 'x';//char(254);//■
                        highlight(c);
                    }
                }
                else if ((x + y) % 2 == 1 && j == (singleSquareHeight + 1) / 2)std::cout << char(177);//▓178 ▒177 ░176
                else std::cout << ' ';
            }

            gotoxy(px + (singleSquareWidth + 1) / 2, py + j); {//centre //reserved for the piece
                highlight(c);
                if (board[x][y].color == pieceColor::WHITE) {
                    wout(whiteChessPiece[castEnumToInt(board[x][y].piece)]);
                }
                else if (board[x][y].color == pieceColor::BLACK) {
                    wout(blackChessPiece[castEnumToInt(board[x][y].piece)]);
                }
                else std::cout << ' ';
            }

            gotoxy(px + singleSquareWidth + 1, py + j); {
                highlight(c);
                std::cout << r;//rightside
            }
        }

        gotoxy(px, py + singleSquareHeight + 1); {//bottom line
            highlight(c);
            std::cout << dl; for (int i = 0; i < singleSquareWidth; i++) std::cout << d; std::cout << dr;
        }
        highlight(DefaultColor);
    }

    void updateBorderNotation(int x, int y, colorCode c = DefaultColor) {
        highlight(c);
        gotoxy(pos_x + x * (singleSquareWidth + 1) + (singleSquareWidth + 1) / 2, pos_y + 8 * (singleSquareHeight + 1) + 1);
        std::cout << horPos[x];

        gotoxy(pos_x - 2, pos_y + y * (singleSquareHeight + 1) + (singleSquareHeight + 1) / 2);
        std::cout << -y + 8;
        highlight(DefaultColor);
    }

    void displayBoard() {
        for (int j = 0; j < boardH; j++) {
            for (int i = 0; i < boardW; i++) {
                updateBoardSquare(i, j, DefaultColor);
            }
        }
        for (int i = 0; i < 8; i++) {
            gotoxy(pos_x + i * (singleSquareWidth + 1) + (singleSquareWidth + 1) / 2, pos_y + 8 * (singleSquareHeight + 1) + 1);
            (i == currentx) ? highlight(cursorColor[currentturn()]) : highlight(DefaultColor);
            std::cout << horPos[i];
            gotoxy(pos_x - 2, pos_y + i * (singleSquareHeight + 1) + (singleSquareHeight + 1) / 2);
            (i == currenty) ? highlight(cursorColor[currentturn()]) : highlight(DefaultColor);
            std::cout << -i + 8;
            highlight(DefaultColor);
        }
    }

    void updateDeadPiece() {
        for (int i = 0;i<16; i++) {
            gotoxy(pos_x - 4 - 1 * i / 8, pos_y + i % 8 * (singleSquareHeight + 1) + (singleSquareHeight + 1) / 2);
            if (i < player[1].capturedPiece.size())wout(whiteChessPiece[castEnumToInt(player[1].capturedPiece.at(i))]);
            else std::cout << ' ';
            
        }
        for (int i = 0; i < 16; i++) {
            gotoxy(pos_x +8*(singleSquareWidth+1) + 2 + 1 * i / 8, pos_y + i % 8 * (singleSquareHeight + 1) + (singleSquareHeight + 1) / 2);
            if (i < player[0].capturedPiece.size())wout(blackChessPiece[castEnumToInt(player[0].capturedPiece.at(i))]);
            else std::cout << ' ';
        }
    }

    void updateMoveLog(bool undo = false) {
        updateDeadPiece();
        int offset = 0;
        if (undo) offset = -1;

        gotoxy(pos_x + (singleSquareWidth + 1) * 8 + disx, pos_y + -1); std::cout << "White:\t   Black:";
        gotoxy(pos_x + (singleSquareWidth + 1) * 8 + disx, pos_y + -2); std::cout << "Move log:   ";
        if (currentlyOnGameMovesetIndex + 1 + offset < 1000000) {//this astronomically large number because when the game movesset vector is empty and we decrement it it jumps to 2power32
            std::cout << std::setprecision(-3) << currentlyOnGameMovesetIndex+1 + offset << '/' << std::setprecision(-3) << gameMoveset.size();
        }
        else std::cout << "error/"<<gameMoveset.size();
        std::cout <<" ded:(" << std::setprecision(-2) << player[0].capturedPiece.size() << ',' << std::setprecision(-2) << player[1].capturedPiece.size()<<')';
        size_t size = gameMoveset.size();
        for (int i = 0; i < size; i++) {
            if (i % 2 == 0) {
                gotoxy(pos_x + (singleSquareWidth + 1) * 8 + disx-3, pos_y + (i / 2));
                std::cout << i / 2 + 1;
            }
            gotoxy(pos_x + (singleSquareWidth + 1) * 8 + disx-1 + (12) * (i % 2), pos_y + (i / 2));
            if (undo) {
                if (i == currentlyOnGameMovesetIndex - 1) std::cout << '>';
                else if (i >= currentlyOnGameMovesetIndex) {
                    std::cout << 'x';
                }
                else std::cout << ' ';
            }
            else {
                if (i == currentlyOnGameMovesetIndex) std::cout << '>';
                else std::cout << ' ';
            }
            gameMoveset.at(i).printMove();
        }
    }

    int isInMoveset(moveset moves, int x, int y) {
        for (int index = 0; index < moves.size(); index++) {
            if (moves.at(index).to.x == x && moves.at(index).to.y == y) {
                return index+1;
            }
        }
        return 0;
    }

    bool ifEmptyPush(chessPiece p, int x, int y, moveset& moves) {
        if (board[x][y].piece == pieceType::EMPTY) {
            moves.push_back(moveFromTo(p,board[x][y]));
            popBackLastMoveIfKingVulnerable(moves);
            return true;
        }
        return false;
    }

    bool ifCaptureAblePush(chessPiece p, int x, int y, moveset& moves) {
        if (p.color != board[x][y].color && board[x][y].piece != pieceType::EMPTY) {
            moves.push_back(moveFromTo(p, board[x][y]).captures());
            popBackLastMoveIfKingVulnerable(moves);
            return true;
        }
        else return false;
    }

    int untilCollide(chessPiece p, moveset& moves, int dx, int dy, int distance = 7, bool vulnerableFlag=false) {
        int index = -1, i = 0; //index 0 means no interaction, 1 means moveable to some square, 2 means captureable to some square
        int x = p.x, y = p.y;
        //if(!inVulnerable)std::cout <<"<" << dx << dy<<">";
        do {
            x += dx;
            y += dy;
            i++;
            index++;
            if (x >= boardW || x < 0 || y >= boardH || y < 0) return -1;
            if (i > distance) return -2;
            //if (!inVulnerable)std::cout << "(" << horPos[x] << 8 - y << ")\n";
        } while (ifEmptyPush(p, x, y, moves));
        if (x < boardW && x >= 0 && y < boardH && y >= 0 && i <= distance) {
            ifCaptureAblePush(p, x, y, moves);
            index++;
            //if(!inVulnerable)std::cout << "("<<horPos[x]<<8-y<<")\n";
        }
        return index;
    }
    void popBackLastMoveIfKingVulnerable(moveset& moves) {
        if (inVulnerable) return;
        doMove(moves.at(moves.size() - 1), true);
        commitGameMove(moves.at(moves.size() - 1));
        if (vulnerable(player[currentturn()].king)) {
            if (moves.size() > 0)moves.pop_back();
        }
        undo(true);
        gameMoveset.pop_back();
    }
    bool possibleMoves(chessPiece p, moveset& moves) {
        moves = {}; //nullifying moveset
       if (p.piece == pieceType::PAWN) {//pawn is the only piece with asymmetric moveset, we can fix it by reflecting about x-axis 
            int ry = 1;//reflect about y coordinate for white (y*-1)
            if (p.color == pieceColor::WHITE) ry = -1;
            if (ifEmptyPush(p, p.x, p.y + 1 * ry, moves)&&p.y == pawnSpawn[castEnumToInt(p.color)] ) ifEmptyPush(p, p.x, p.y + 2 * ry, moves);
            
            if (p.x < boardW - 1) ifCaptureAblePush(p, p.x + 1, p.y + 1 * ry, moves);
            if (p.x > 0)    ifCaptureAblePush(p, p.x - 1, p.y + 1 * ry, moves);

            //en passant here
            if (currentlyOnGameMovesetIndex > 1&&!inVulnerable) {
                move lastMove = gameMoveset.at(currentlyOnGameMovesetIndex - 1);         // check if last move 
                if (lastMove.from.piece == pieceType::PAWN                              // is a pawn
                    && lastMove.from.y == pawnSpawn[!castEnumToInt(p.color)]            // from pawn spawn
                    && lastMove.to.y == lastMove.from.y + 4 * !castEnumToInt(p.color) - 2 //in two stepss  //if black last
                    && lastMove.to.y == p.y                                               //same level
                    && ((lastMove.to.x + 1 == p.x) || (lastMove.to.x - 1 == p.x))         // to beside us.
                    ) {
                        if (ifEmptyPush(p, lastMove.to.x, p.y + 1 * ry, moves))  moves.at(moves.size() - 1).captures().enPassents();
                        //popBackLastMoveIfKingVulnerable(moves);
                }
            }
        }

        else if (p.piece == pieceType::KNIGHT) {
            untilCollide(p, moves, -2, -1, 1);  untilCollide(p, moves, -1, -2, 1);//llu and luu
            untilCollide(p, moves, 2, -1, 1);   untilCollide(p, moves, 1, -2, 1);///rru and ruu
            untilCollide(p, moves, -2, 1, 1);   untilCollide(p, moves, -1, 2, 1);// lld and ldd
            untilCollide(p, moves, 1, 2, 1);    untilCollide(p, moves, 2, 1, 1);//  rrd and rdd
        }
        else {
            int travel = 7;
            chessPiece orginalKing = player[currentturn()].king;
            chessPiece oppositeOrginalKing = player[!currentturn()].king;
            if (p.piece == pieceType::KING) travel = 1;
            if (p.piece == pieceType::ROOK || p.piece == pieceType::QUEEN || p.piece == pieceType::KING) {
                untilCollide(p, moves, 1, 0, travel);   //r
                untilCollide(p, moves, -1, 0, travel);  //l
                untilCollide(p, moves, 0, 1, travel);   //u
                untilCollide(p, moves, 0, -1, travel);  //d
            }
            if (p.piece == pieceType::BISHOP || p.piece == pieceType::QUEEN || p.piece == pieceType::KING) {
                untilCollide(p, moves, 1, 1, travel);   //rd
                untilCollide(p, moves, -1, 1, travel);  //ld
                untilCollide(p, moves, 1, -1, travel);  //ru
                untilCollide(p, moves, -1, -1, travel); //lu
            }
            player[currentturn()].king = orginalKing;
            player[!currentturn()].king = oppositeOrginalKing;

            if (p.piece == pieceType::KING) {
                //castling
                bool leftCastleAble = true, rightCastleAble = true;
               if(!inVulnerable && !vulnerable(player[currentturn()].king)) {
                    for (int i = 1; i <= 3; i++) {//queen side castling move
                        if (!player[currentturn()].castleLeft
                            || board[p.x - i][p.y].piece != pieceType::EMPTY
                            || vulnerable(board[p.x - i][p.y])) {
                            leftCastleAble = false;
                            break;
                        }
                    }
                    for (int i = 1; i <= 2; i++) {//king side castling move
                        if (!player[currentturn()].castleRight
                            || board[p.x + i][p.y].piece != pieceType::EMPTY
                            || vulnerable(board[p.x + i][p.y])) {
                            rightCastleAble = false;
                            break;
                        }
                    }
                    if (leftCastleAble) moves.push_back(moveFromTo(p, { p.x - 2,p.y }).castles());
                    if (rightCastleAble) moves.push_back(moveFromTo(p, { p.x + 2,p.y }).castles());
                }
            }
        }
        if (moves.size() > 0)return true;
        else return false;
    }
    void markMoveset(moveset m, colorCode c = DefaultColor) {
        for (int i = 0; i < m.size(); i++) {
            if (c != DefaultColor) {
                colorCode cc;
                (m.at(i).capture) ? cc = captureColor : cc = c;
                updateBoardSquare(m[i].to.x, m[i].to.y, cc);
            }
            else                   updateBoardSquare(m[i].to.x, m[i].to.y, DefaultColor);
        }
    }

    bool vulnerable(chessPiece piece) {//, setOfChessPiece& attackingPieces=
        inVulnerable = true;
        moveset moves = {};
        piece.piece = pieceType::EMPTY;
        for (int i = castEnumToInt(pieceType::KING); i <= castEnumToInt(pieceType::PAWN); i++) {
            chessPiece p = { piece.x, piece.y, currentTurn, static_cast<pieceType>(i) };
            moves = {};
            possibleMoves(p, moves);
            for (int j = 0; j < moves.size(); j++) {
                if (moves.at(j).capture && moves.at(j).to.piece == moves.at(j).from.piece) {//
                    inVulnerable = false;
                    return true;
                }
            }
        }
        inVulnerable = false;
        return false;
    }

    void deleteBoardSquare(int x, int y) {
        board[x][y].color = pieceColor::EMPTY;
        board[x][y].piece = pieceType::EMPTY;
    }

    void copyPiece(chessPiece from, chessPiece& to) {
        to.color = from.color;
        to.piece = from.piece;
    }

    void doMove(move& m, bool vulnerabilityCheck = false) {//w
        //en passant 
        if (m.enPassent) {
            deleteBoardSquare(m.to.x, m.from.y);
            if (!vulnerabilityCheck) {
                updateBoardSquare(m.to.x, m.from.y);
                player[currentturn()].capturedPiece.push_back(pieceType::PAWN);
            }
        }
        else if (m.capture&&!vulnerabilityCheck) {
            player[currentturn()].capturedPiece.push_back(m.to.piece);
        }

        //CORE PART
        copyPiece(m.from, board[m.to.x][m.to.y]);
        deleteBoardSquare(m.from.x, m.from.y);
        if (!vulnerabilityCheck) {
            updateBoardSquare(m.from.x, m.from.y);
            updateBoardSquare(m.to.x, m.to.y);
        }

        if (m.from.piece == pieceType::KING) {//works but will it break in future? 
            player[currentturn()].king = m.to;
        }
    }

    void applyFlags(move& m) {
        changeTurn();
        if (vulnerable(player[currentturn()].king)) m.checks();
        if (!inRedo&&!checkIfAnyMoveAvailable()) {
            if (m.check) {
                m.checkmates();
            }
            else m.stalemates();
        }
        else {
            if (m.check) {
                //checkIfAnyMoveAvailable(true);//markss
            }
        }
        changeTurn();

        if (m.from.piece == pieceType::PAWN && m.to.y == 7 * currentturn()) {
            char choice;
            HUD("Promote to Q[1] B[2] N[3] R[4]");
            do {
                choice = _getch();
            } while (choice < '1' || choice>'4');
            m.promotes().setPawnPromoteTo(static_cast<pieceType>(int(choice) + 1 - '0'));
            board[m.to.x][m.to.y].piece = static_cast<pieceType>(int(choice) + 1 - '0');
            updateBoardSquare(m.to.x, m.to.y);
        }
        if (m.from.piece == pieceType::KING) {
            if (player[currentturn()].castleLeft || player[currentturn()].castleRight) {
                m.toggleCastleRight();
                m.toggleCastleLeft();
                player[currentturn()].castleLeft = false;
                player[currentturn()].castleRight = false;
            }

            if (m.castle) {
                if (m.to.x<m.from.x) {//queenside
                    player[currentturn()].castleLeft = false;
                    copyPiece(board[0][m.to.y], board[m.to.x + 1][m.to.y]);
                    updateBoardSquare(m.to.x + 1, m.to.y);
                    deleteBoardSquare(0, m.to.y);
                    updateBoardSquare(0, m.to.y);
                }
                else {
                    player[currentturn()].castleRight = false;
                    copyPiece(board[7][m.to.y], board[m.to.x - 1][m.to.y]);
                    updateBoardSquare(m.to.x - 1, m.to.y);
                    deleteBoardSquare(7, m.to.y);
                    updateBoardSquare(7, m.to.y);
                }
            }
        }
        else if (player[currentturn()].castleLeft && m.from.x == 0 && m.from.y == 7 * (!currentturn())) {//left rook moved
            player[currentturn()].castleLeft = false;
            m.toggleCastleLeft();
        }
        else if (player[currentturn()].castleRight && m.from.x == 7 && m.from.y == 7 * (!currentturn())) {//right rook moved
            player[currentturn()].castleRight = false;
            m.toggleCastleRight();
        }
        else if (player[!currentturn()].castleLeft && m.to.x == 0 && m.to.y == 7 * (currentturn())) {//left rook captured
            player[!currentturn()].castleLeft = false;
            m.toggleCastleLeft();
        }
        else if (player[!currentturn()].castleRight && m.to.x == 7 && m.to.y == 7 * (currentturn())) {//right rook captured
            player[!currentturn()].castleRight = false;
            m.toggleCastleRight();
        }
    }

    void undo(bool vulnerabilityCheck=false) {
        if (currentlyOnGameMovesetIndex <= 0&&vulnerabilityCheck==false) {
            HUD("cannot undo anymore!");
            return;
        }

        move prevMove;
        if (vulnerabilityCheck == false) {
            currentlyOnGameMovesetIndex--;
            prevMove = gameMoveset.at(currentlyOnGameMovesetIndex);
        }
        else {
            prevMove = gameMoveset.at(gameMoveset.size() - 1);
        }
        
        copyPiece(prevMove.from, board[prevMove.from.x][prevMove.from.y]);//copied to move start pos
        deleteBoardSquare(prevMove.to.x, prevMove.to.y);                  //deleted from move end pos
        if (!vulnerabilityCheck) {
            updateBoardSquare(prevMove.from.x, prevMove.from.y);          //updating the move start pos
            updateBoardSquare(prevMove.to.x, prevMove.to.y);              //updating the move end pos
        }
        
        //reseructing dead opposite pieces
        if (!vulnerabilityCheck &&prevMove.capture && player[!currentturn()&&!vulnerabilityCheck].capturedPiece.size()>0) player[!currentturn()].capturedPiece.pop_back();
        if (prevMove.enPassent) {
            board[prevMove.to.x][prevMove.from.y].setPiece(static_cast<pieceColor>(currentturn()), pieceType::PAWN);
            if (!vulnerabilityCheck)updateBoardSquare(prevMove.to.x, prevMove.from.y);
        }
        else if (prevMove.capture) {
            copyPiece(prevMove.to, board[prevMove.to.x][prevMove.to.y]);//copies dead piece to the move end pos
            updateBoardSquare(prevMove.to.x, prevMove.to.y);              //updating the move end after changing it
        }
        
        if (prevMove.from.piece == pieceType::KING) {
            player[!currentturn()].king = prevMove.from;
        }

        if (prevMove.castleLeftToggle) {
            player[!currentturn()].castleLeft = true;
        }
        if (prevMove.castleRightToggle) {
            player[!currentturn()].castleRight = true;
        }

        if (prevMove.castle) {
            if (prevMove.to.x < prevMove.from.x) {//queenside
                player[currentturn()].castleLeft = true;
                copyPiece(board[prevMove.to.x + 1][prevMove.to.y], board[0][prevMove.to.y]);
                updateBoardSquare(0, prevMove.to.y);
                deleteBoardSquare(prevMove.to.x+1, prevMove.to.y);
                updateBoardSquare(prevMove.to.x + 1, prevMove.to.y);
            }
            else {
                player[currentturn()].castleRight = false;
                copyPiece( board[prevMove.to.x - 1][prevMove.to.y], board[7][prevMove.to.y]);
                updateBoardSquare(7,prevMove.to.y);
                deleteBoardSquare(prevMove.to.x - 1, prevMove.to.y);
                updateBoardSquare(prevMove.to.x - 1, prevMove.to.y);
            }
        }

        if (!vulnerabilityCheck) {
            changeTurn();
            hasSelected = false;
            updateMoveLog(true);
        }
    }

    void redo() {
        inRedo = true;
        if (currentlyOnGameMovesetIndex == gameMoveset.size()) {
            HUD("cannot redo anymore!");
            return;
        }
        doMove(gameMoveset.at(currentlyOnGameMovesetIndex));
        applyFlags(gameMoveset.at(currentlyOnGameMovesetIndex));
        changeTurn();
        currentlyOnGameMovesetIndex++;
        updateMoveLog(true);
        inRedo = false;
    }

    void changeTurn() {
        if (currentTurn == pieceColor::BLACK) {
            currentTurn = pieceColor::WHITE;
            if(currenty<4)currenty = 7-currenty;
        }
        else {
            currentTurn = pieceColor::BLACK;
            if (currenty >= 4)currenty = 7-currenty;
        }
    }

    void help() {
        gotoxy(0, 1);
        std::cout << "Use:\tW";
        if (twoController)std::cout << " B";
        else std::cout << "  ";
        for (int i = 0; i < playerKeysSize; i++) {
            std::cout << std::endl << playerKeys[i].function;
            wout(keysymbols[i]);
            std::cout << '\t' << playerKeys[i].keys[0];
            if (twoController) std::cout << ' ' << playerKeys[i].keys[1];
            else std::cout << "  ";
        }
    }

    bool takeInput(int& x, int& y) {
        char c;
        key pressedKey;
        bool doubleBreak = false;
        do{
            c = _getch();
            for (int i = 0; i < playerKeysSize; i++) {
                if (!twoController && c == playerKeys[i].keys[0]) {
                    pressedKey = playerKeys[i];
                    doubleBreak = true;
                    break;
                }
                else if (twoController && c == playerKeys[i].keys[currentturn()]) {
                    pressedKey = playerKeys[i];
                    doubleBreak = true;
                    break;
                }
                else continue;
            }
        }while (!doubleBreak);
        if (pressedKey.function == "Enter") return true;
        else if (pressedKey.function == "Up" && y > 0) y--;
        else if (pressedKey.function == "Down" && y < boardW - 1) y++;
        else if (pressedKey.function == "Left" && x > 0) x--;
        else if (pressedKey.function == "Right" && x < boardW - 1) x++;
        else if (pressedKey.function == "UL" && x > 0 && y > 0) { x--; y--; }
        else if (pressedKey.function == "UR" && x < boardW - 1 && y > 0) { x++; y--; }
        else if (pressedKey.function == "DL" && x > 0 && y < boardW - 1) { x--; y++; }
        else if (pressedKey.function == "DR" && x < boardW - 1 && y < boardW - 1) { x++; y++; }
        else if (pressedKey.function == "Undo") undo();
        else if (pressedKey.function == "Redo") redo();
        else if (pressedKey.function == "Reload") displayBoard();
        else if (pressedKey.function == "Help") help();
        else if (pressedKey.function == "2Ctrl") (twoController) ? twoController = false: twoController = true;
        return false;
    }
    bool ifCheckMateOrStalemateAndContinue() {
        if (gameMoveset.size() == 0) return true;
        char key = ' ';
        move lastMove = gameMoveset.at(gameMoveset.size() - 1);
        if (lastMove.checkmate) HUD("CHECKMATE", -2);
        else if (lastMove.stalemate) HUD("STALEMATE", -2);
        else if (lastMove.check) HUD("CHECK", -2);
        if (lastMove.checkmate|| lastMove.stalemate) {
            HUD("Do you Want to continue?(y/n)");
            do {
                key = _getch();
            } while (key != 'y' && key != 'n');
            HUD("", -2);
            if (key == 'y') return true;
            else return false;
        }
        return true;
    }
    move selectMove() {
        moveset moveable;
        int prevx = currentx, prevy = currenty;
        moveable = {};
        while(true) {
            up:
            gotoxy(0, 0);
            std::cout << "H for help\n";
            help();

            std::string turn;
            (currentturn()) ? turn = "Black" : turn = "White";
            HUD(turn + " on: " + squareName(currentx, currenty)+"King: "+squareName(player[currentturn()].king.x, player[currentturn()].king.y)
                , -1);

            if (gameMoveset.size() > 0 && gameMoveset.at(gameMoveset.size() - 1).check) {
                HUD("You are being Checked!!");
                updateBoardSquare(player[currentturn()].king.x, player[currentturn()].king.y,checkColor);
            }

            prevx = currentx, prevy = currenty;
            if(hasSelected) markMoveset(moveable, moveableColor);
            else markMoveset(moveable);
            updateBorderNotation(currentx, currenty, cursorColor[currentturn()]);
            updateBoardSquare(currentx, currenty, cursorColor[currentturn()]);

            bool goup = false;
            if (!takeInput(currentx, currenty)) goup=true;
            

            updateBorderNotation(prevx, prevy);
            updateBoardSquare(prevx, prevy);
            if(gameMoveset.size()>0&&gameMoveset.at(gameMoveset.size() - 1).check)updateBoardSquare(player[currentturn()].king.x, player[currentturn()].king.y, checkColor);
            
            if (goup) goto up;



            if (!hasSelected){ // Selecting a piece to move for first time
                if (board[currentx][currenty].piece == pieceType::EMPTY || board[currentx][currenty].color != currentTurn) continue;
            }

            //Selecting the destination of the selected piece i
            //invalid: if the selected square is not in moveset
            //valid:   if the entered square is of color of current turn.The selected piece will change to it.
            else if (!isInMoveset(moveable, currentx, currenty) && board[currentx][currenty].color != currentTurn) continue;

            //if entered square is the same square as selected piece square
            else if (currentx == selectedx && currenty == selectedy) continue;//entered same pos

            //Changing the selected piece or selecting for first time
            if (!isInMoveset(moveable, currentx, currenty)) {
                if (hasSelected) {
                    hasSelected = false;
                    updateBoardSquare(selectedx, selectedy);
                }
                hasSelected = true;
                selectedx = currentx;
                selectedy = currenty;

                markMoveset(moveable);
                possibleMoves(board[currentx][currenty], moveable);
                updateBoardSquare(selectedx, selectedy, selectedColor);
                markMoveset(moveable, moveableColor);
            }

            //Actually moving
            else {
                clearWarning();
                hasSelected = false;
                updateBoardSquare(selectedx, selectedy,DefaultColor);
                updateBoardSquare(currentx, currenty,DefaultColor);
                markMoveset(moveable);
                for (int i = 0; i < moveable.size(); i++) {
                    if (moveable.at(i).to.x == currentx && moveable.at(i).to.y == currenty) {
                        return(moveable.at(i));
                    }
                }
            }
        }
    }

    void HUD(std::string warn, int y= 3) {
        if (y > 0) y += pos_y + (singleSquareHeight + 1) * 8;
        else y += pos_y;
        std::string s((singleSquareWidth + 1) * 8, ' ');
        gotoxy(pos_x + (singleSquareWidth + 1) * 4 - s.length() / 2, y);
        std::cout << s;
        gotoxy(pos_x + (singleSquareWidth + 1) * 4 - warn.length()/2, y);
        std::cout << warn;
    }
    void commitGameMove(move m) {
        if (currentlyOnGameMovesetIndex < gameMoveset.size() && gameMoveset.size()>0) {//cutoff
            while (gameMoveset.size() != currentlyOnGameMovesetIndex) {
                gotoxy(pos_x + (singleSquareWidth + 1) * 8 + disx-5 + 16 * ((int(gameMoveset.size())-1) % 2), pos_y + (gameMoveset.size() / 2)-1* ((int(gameMoveset.size()) - 1) % 2));
                std::cout << "             ";
                gameMoveset.pop_back();
            }
        }
        gameMoveset.push_back(m);
    }
    void moveCountIncrement() {
        currentlyOnGameMovesetIndex++;
    }

    void clearWarning() {
        HUD("RISE"); wout(L"神");
    }

    bool checkIfAnyMoveAvailable() {
        moveset temp = {};
        bool available = false;
        for (int j = 0; j < boardW; j++) {
            for (int i = 0; i < boardH; i++) {
                if (castEnumToInt(board[i][j].color)  == currentturn()) {
                    if (possibleMoves(board[i][j], temp)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

};

int main() {
    system("color f0");
    chessBoard c;//initialize 
    c.displayBoard();//initializes the board on screen
    while (true) {
        c.clearWarning();
        if(!c.ifCheckMateOrStalemateAndContinue()) break;//no continue
        move m = c.selectMove();// miniloop inside it
        c.doMove(m);
        c.applyFlags(m);
        c.commitGameMove(m);
        c.updateMoveLog();
        c.moveCountIncrement();
        c.changeTurn();
    }
    return 0;
}