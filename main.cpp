#include <stdio.h>
#include <stdlib.h>

#include "array.h"

enum Piece_Type {
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING
};

#define WHITE 0
#define BLACK 1

struct Square {
    Piece_Type type {};
    int color {};
    bool is_empty = true;
};

Square make_piece(Piece_Type type, int color) {
    Square result {};
    result.type = type;
    result.color = color;
    result.is_empty = false;
    return result;
}

char square_to_char(const Square &square) {
    if (square.is_empty) return '.';
    
    char result = '?';
    switch (square.type) {
        case PAWN:      result = 'P'; break;
        case ROOK:      result = 'R'; break;
        case KNIGHT:    result = 'N'; break;
        case BISHOP:    result = 'B'; break;
        case QUEEN:     result = 'Q'; break;
        case KING:      result = 'K'; break;
        default: fprintf(stderr, "square_to_char: invalid chess piece"); return '?';
    }

    if (square.color == BLACK) result += ('a' - 'A');

    return result;
}

struct Move {
    int piece_row;
    int piece_col;
    int dest_row;
    int dest_col;
};

struct Chess {
    Square board[64] {};
    int turn = WHITE;

    Chess() {
        reset();
    }

    void reset() {
        for (int i = 0; i < 64; ++i) board[i] = {};
        turn = 0;

        // pawns
        for (int c = 0; c < 8; ++c) {
            board[to_index(1, c)] = make_piece(PAWN, WHITE);
            board[to_index(6, c)] = make_piece(PAWN, BLACK);
        }

        // other pieces
        setup_back_pieces(WHITE);
        setup_back_pieces(BLACK);
    }

    void setup_back_pieces(int color) {
        int r = color == WHITE ? 0 : 7;
        board[to_index(r,0)] = make_piece(ROOK,     color);
        board[to_index(r,1)] = make_piece(KNIGHT,   color);
        board[to_index(r,2)] = make_piece(BISHOP,   color);
        board[to_index(r,3)] = make_piece(QUEEN,    color);
        board[to_index(r,4)] = make_piece(KING,     color);
        board[to_index(r,5)] = make_piece(BISHOP,   color);
        board[to_index(r,6)] = make_piece(KNIGHT,   color);
        board[to_index(r,7)] = make_piece(ROOK,     color);
    }

    Array<Move> legal_moves() const {
        Array<Move> result {};
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                const Square *square = &board[to_index(r,c)];
                if (square->is_empty) continue;
                
                switch (square->type) {
                    
                    case PAWN: {
                        int steps_forward = (r == 1 && square->color == WHITE || r == 6 && square->color == BLACK) ? 2 : 1;
                        int step_dir = square->color == WHITE ? 1 : -1;
                        for (int step = 0; step < steps_forward; ++step) {
                            int next_row = r + (step+1) * step_dir;
                            if (!board[to_index(next_row, c)].is_empty) break; // break to prevent pawn teleporting through blocking pieces
                            Move move {};
                            move.piece_row = r;
                            move.piece_col = c;
                            move.dest_row = next_row;
                            move.dest_col = c;
                        }
                    } break;

                    default: fprintf(stderr, "legal_moves: unhandled piece type\n"); break;
                }
            }
        }
        return result;
    }

    void draw() const {
        printf("============\n");
        for (int r = 7; r >= 0; --r) {
            printf("%d  ", r + 1);
            for (int c = 0; c < 8; ++c) {
                printf("%c", square_to_char(board[to_index(r, c)]));
            }
            printf("\n");
        }

        printf("\n   ");
        for (int c = 0; c < 8; ++c) {
            printf("%c", 'a' + c);
        }
        printf("\n");
    }

    int to_index(int row, int col) const {
        if (row < 0 || row >= 8 || col < 0 || col >= 8) {
            fprintf(stderr, "to_index: invalid row and col: %d, %d", row, col);
            exit(1);
        }
        return col + row * 8;
    }
};



struct Minimax_Result {
    Move best_move;
    int value;
};

int main() {
    printf("Hello there\n");
    Chess chess {};
    chess.draw();
}