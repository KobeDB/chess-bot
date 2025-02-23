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
    bool has_moved {};
    bool is_empty = true;
};

Square make_piece(Piece_Type type, int color) {
    Square result {};
    result.type = type;
    result.color = color;
    result.is_empty = false;
    result.has_moved = false;
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
    int remove_r;
    int remove_c;
    int place_r;
    int place_c;
    Piece_Type piece;
    int color;
    bool is_castling = false;
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
                if (square->color != turn) continue;
                
                switch (square->type) {
                    
                    case PAWN: {
                        // normal forward step
                        int steps_forward = (r == 1 && square->color == WHITE || r == 6 && square->color == BLACK) ? 2 : 1;
                        int step_dir = square->color == WHITE ? 1 : -1;
                        for (int step = 0; step < steps_forward; ++step) {
                            int next_row = r + (step+1) * step_dir;
                            if (!board[to_index(next_row, c)].is_empty) break; // break to prevent pawn teleporting through blocking pieces
                            Move move {};
                            move.remove_r = r;
                            move.remove_c = c;
                            move.place_r = next_row;
                            move.place_c = c;
                            move.piece = PAWN;
                            move.color = square->color;
                            if (!does_move_cause_self_check(move))
                                result.push(move);
                            // TODO: add pawn promotion here
                        }

                        // taking
                        for (int attack_c = -1; attack_c <= 1; ++attack_c) {
                            if (attack_c == 0) continue;
                            int attack_r = r + step_dir;
                            const Square *attack_square = get_square(attack_r, attack_c);
                            if (attack_square && !attack_square->is_empty) {
                                if (attack_square->color != square->color) {
                                    Move move {};
                                    move.remove_r = r;
                                    move.remove_c = c;
                                    move.place_r = attack_r;
                                    move.place_c = attack_c;
                                    move.piece = PAWN;
                                    move.color = square->color;
                                    if (!does_move_cause_self_check(move))
                                        result.push(move);
                                }
                            }
                        }

                        // TODO: maybe in future handle stinky en-passant moves

                    } break;

                    default: fprintf(stderr, "legal_moves: unhandled piece type\n"); break;
                }
            }
        }
        return result;
    }

    Chess next_state(const Move &move) const {
        Chess next {};
        // TODO
        return next;
    }

    bool does_move_cause_self_check(const Move &move) const {
        // TODO
        return false;
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

    const Square *get_square(int row, int col) const {
        if (row < 0 || row >= 8 || col < 0 || col >= 8) {
            return nullptr;
        }
        return &board[to_index(row, col)];
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