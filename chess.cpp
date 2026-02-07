#include "include/chess.hpp"

#include <iostream>

using namespace std;
using namespace chess;

void test_board()
{

    Board board;
    // Position with en passant available
    board.load_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");

    std::cout << "Initial position:\n" << board.to_string() << std::endl;

    Move d5(Square::D7, Square::D5, MoveFlag::NORMAL);
    board.make_move(d5);

    std::cout << "\n\nAfter 1...d5:\n" << board.to_string() << std::endl;

    Move e4d5(Square::E4, Square::D5, MoveFlag::CAPTURE);
    board.make_move(e4d5);

    std::cout << "\n\nAfter 2...e4:\n" << board.to_string() << std::endl;

    // Push another pawn move to set up en passant for white
    Move c5 (Square::C7, Square::C5, MoveFlag::NORMAL);
    board.make_move(c5);

    std::cout << "\n\nAfter 3...c5:\n" << board.to_string() << std::endl;

    // White captures en passant: d5xc6
    Move d5c6_ep(Square::D5, Square::C6, MoveFlag::EN_PASSANT);
    board.make_move(d5c6_ep);

    std::cout << "\n\nAfter 4...c6:\n" << board.to_string() << std::endl;
}

int main()
{
    test_board();
}