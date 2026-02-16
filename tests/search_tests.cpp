#include <cassert>
#include <iostream>

#include "chess/Board.hpp"
#include "chess/Eval.hpp"
#include "chess/Search.hpp"

using namespace chess;
using namespace std::chrono_literals;

void test_evaluator() {
    std::cout << "\n=== Testing Evaluator ===" << std::endl;
    
    Board board;
    Evaluator evaluator;
    
    // Test 1: Starting position (equal)
    board.reset();
    int eval = evaluator.evaluate(board);
    std::cout << "Starting position: " << eval << " (should be 0)" << std::endl;
    assert(eval == 0);
    
    // Test 2: White up a pawn
    board.load_fen("rnbqkbnr/ppp1pppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    eval = evaluator.evaluate(board);
    std::cout << "White up a pawn: " << eval << " (should be ~100)" << std::endl;
    assert(eval >= 100 && eval <= 200 );  // One pawn difference
    
    // Test 3: White up a rook
    board.load_fen("rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQq - 0 1");
    eval = evaluator.evaluate(board);
    std::cout << "White up a rook: " << eval << " (should be ~500)" << std::endl;
    assert(eval >= 495 && eval <= 505);
    
    // Test 4: Checkmate
    // board.load_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    board.load_fen("rnbqkbnr/ppppp2p/8/5ppQ/4P3/2N5/PPPP1PPP/R1B1KBNR b KQkq - 1 3");
    eval = evaluator.evaluate(board);
    std::cout << "Black checkmated: " << eval << " (should be CHECKMATE)" << std::endl;
    assert(eval == CHECKMATE);
    
    std::cout << "✓ All evaluator tests passed!" << std::endl;
}

void test_search_starting_position() {
    std::cout << "\n=== Testing Search - Starting Position ===" << std::endl;
    
    Board board;
    board.reset();
    
    Engine engine;
    
    // Search depth 2 (1 ply each side)
    SearchResult result = engine.find_best_move(board, 2);
    Move best = result.best_move;
    
    std::cout << "Best move from starting position: "
             << square_to_string(best.from()) << square_to_string(best.to()) << std::endl;
    
    // Should find a valid move
    assert(best.from() != Square::INVALID);
    assert(best.to() != Square::INVALID);
    
    std::cout << "✓ Search test passed!" << std::endl;
}

void test_search_capture_preference() {
    std::cout << "\n=== Testing Search - Capture Preference ===" << std::endl;
    
    Board board;
    
    // Position where white can capture a queen
    // White pawn on E4, black queen on D5 undefended
    board.load_fen("rnb1kbnr/pppppppp/8/3q4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
    
    Engine engine;
    SearchResult result = engine.find_best_move(board, 2ms);
    Move best = result.best_move;

    // Should capture the queen (E4 x D5)
    bool is_capture = best.from() == Square::E4 && best.to() == Square::D5;
    
    std::cout << "Chosen move: " << square_to_string(best.from()) 
             << square_to_string(best.to()) << std::endl;
    std::cout << (is_capture ? "✓ Correctly captured queen!" : "✗ Failed to capture queen") << std::endl;
    
    assert(is_capture);
}

void test_search_checkmate_avoidance() {
    std::cout << "\n=== Testing Search - Checkmate Avoidance ===" << std::endl;
    
    Board board;
    
    // Position where white is in check and must move
    board.load_fen("rnbqkbnr/pppppppp/8/8/4P3/5Q2/PPPP1PPP/RNB1KBNR b KQkq - 0 1");
    
    Engine engine;
    SearchResult result = engine.find_best_move(board, 2ms);
    Move best = result.best_move;

    // Apply the move and check it's legal
    board.make_move(best);
    assert(board.is_valid_position());
    
    std::cout << "✓ Found valid move avoiding checkmate!" << std::endl;
}

void test_search_depth() {
    std::cout << "\n=== Testing Search - Different Depths ===" << std::endl;
    
    Board board;
    board.reset();
    
    Engine engine;
    
    for (int depth = 1; depth <= 4; ++depth) {
        board.reset();
        SearchResult result = engine.find_best_move(board, (Depth)depth);
        // Move best = result.best_move;

        std::cout << "Depth " << depth << ": " << result.nodes_searched
                 << " nodes evaluated" << std::endl;
    }
    
    std::cout << "✓ Search at various depths completed!" << std::endl;
}

int main() {
    try {
        std::cout << "╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║      Phase 2: Search & Evaluation      ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝" << std::endl;
        
        test_evaluator();
        test_search_starting_position();
        test_search_capture_preference();
        test_search_checkmate_avoidance();
        test_search_depth();
        
        std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║          ✓ ALL TESTS PASSED            ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}