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

struct Move {
    int piece;
    int dest_r;
    int dest_c;
    int taken_piece = -1;
};

struct Piece {
    Piece_Type type {};
    int color {};
    int r {};
    int c {};
    bool has_moved = false;
    bool taken = false;
    int index; // piece's own index in the pieces array, this is probably dumb but whatever

    Piece() {}

    Piece(Piece_Type type, int color, int r, int c, int index) : type{type}, color{color}, r{r}, c{c}, index{index} {}
};

struct Chess {
    Piece pieces[32] {};
    int turn = WHITE;

    Chess() {
        reset();
    }

    void reset() {
        
        turn = WHITE;

        // init pawns
        for (int i = 0; i < 8; ++i) {
            pieces[i] = Piece{PAWN, WHITE, 1, i, i};
            pieces[i + 16] = Piece{PAWN, BLACK, 6, i, i + 16};
        }

        // other pieces
        setup_back_pieces(WHITE);
        setup_back_pieces(BLACK);
    }

    void setup_back_pieces(int color) {
        int off = color == WHITE ? 0 : 16;
        int row = color == WHITE ? 0 : 7;
        pieces[off + 8]     = Piece{ ROOK,   color, row, 0, off + 8 };
        pieces[off + 9]     = Piece{ KNIGHT, color, row, 1, off + 9 };
        pieces[off + 10]    = Piece{ BISHOP, color, row, 2, off + 10 };
        pieces[off + 11]    = Piece{ QUEEN,  color, row, 3, off + 11 };
        pieces[off + 12]    = Piece{ KING,   color, row, 4, off + 12 };
        pieces[off + 13]    = Piece{ BISHOP, color, row, 5, off + 13 };
        pieces[off + 14]    = Piece{ KNIGHT, color, row, 6, off + 14 };
        pieces[off + 15]    = Piece{ ROOK,   color, row, 7, off + 15 };
    }

    void to_board(const Piece **board) const {
        for (int i = 0; i < 32; ++i) if (!pieces[i].taken) board[to_index(pieces[i].r, pieces[i].c)] = &pieces[i];
    }

    void to_board(Piece **board) {
        for (int i = 0; i < 32; ++i) if (!pieces[i].taken) board[to_index(pieces[i].r, pieces[i].c)] = &pieces[i];
    }

    Array<Move> legal_moves() const {
        Array<Move> result {};

        const Piece *board[64] {};
        to_board(board);

        for (int i = 0; i < 32; ++i) {
            const Piece &piece = pieces[i];
            if (piece.taken) continue;
            if (piece.color != turn) continue;
            
            switch (piece.type) {
                
                case PAWN: {
                    // normal forward step
                    int steps_forward = (piece.r == 1 && piece.color == WHITE || piece.r == 6 && piece.color == BLACK) ? 2 : 1;
                    int step_dir = piece.color == WHITE ? 1 : -1;
                    for (int step = 0; step < steps_forward; ++step) {
                        int next_row = piece.r + (step+1) * step_dir;
                        if (board[to_index(next_row, piece.c)]) break; // break to prevent pawn teleporting through blocking pieces
                        Move move {};
                        move.piece = i;
                        move.dest_r = next_row;
                        move.dest_c = piece.c;
                        if (!does_move_cause_self_check(move))
                            result.push(move);
                        // TODO: add pawn promotion here
                    }

                    // taking
                    for (int attack_c = -1; attack_c <= 1; ++attack_c) {
                        if (attack_c == 0) continue;
                        int attack_r = piece.r + step_dir;
                        const Piece *attacked_piece = board[to_index(attack_r, attack_c)];
                        if (attacked_piece) {
                            Move move {};
                            move.piece = i;
                            move.dest_r = attack_r;
                            move.dest_c = attack_c;
                            move.taken_piece = attacked_piece->index;
                            if (!does_move_cause_self_check(move))
                                result.push(move);
                        }
                    }
                } break;

                default: break;
            }
        }

        return result;
    }

    Chess next_state(const Move &move) const {
        Chess next = *this;

        next.turn = turn == WHITE ? BLACK : WHITE;

        next.pieces[move.piece].r = move.dest_r;
        next.pieces[move.piece].c = move.dest_c;

        if (move.taken_piece != -1) {
            next.pieces[move.taken_piece].taken = true;
            next.pieces[move.taken_piece].r = -1;
            next.pieces[move.taken_piece].c = -1;
        }
        
        return next;
    }

    bool does_move_cause_self_check(const Move &move) const {
        // TODO
        return false;
    }

    char piece_to_char(const Piece &piece) const {        
        char result = '?';
        switch (piece.type) {
            case PAWN:      result = 'P'; break;
            case ROOK:      result = 'R'; break;
            case KNIGHT:    result = 'N'; break;
            case BISHOP:    result = 'B'; break;
            case QUEEN:     result = 'Q'; break;
            case KING:      result = 'K'; break;
            default: fprintf(stderr, "square_to_char: invalid chess piece"); return '?';
        }
    
        if (piece.color == BLACK) result += ('a' - 'A');
    
        return result;
    }

    void draw() const {
        const Piece *board[64] {};
        to_board(board);

        printf("============\n");
        for (int r = 7; r >= 0; --r) {
            printf("%d  ", r + 1);
            for (int c = 0; c < 8; ++c) {
                if (!board[to_index(r,c)])
                    printf(".");
                else
                    printf("%c", piece_to_char(*board[to_index(r, c)]));
            }
            printf("\n");
        }

        printf("\n   ");
        for (int c = 0; c < 8; ++c) {
            printf("%c", 'a' + c);
        }
        printf("\n");
    }

    // const Square *get_square(int row, int col) const {
    //     if (row < 0 || row >= 8 || col < 0 || col >= 8) {
    //         return nullptr;
    //     }
    //     return &board[to_index(row, col)];
    // }

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

    Move move {};
    move.piece = 0;
    move.dest_c = 0;
    move.dest_r = 3;
    chess = chess.next_state(move);
    chess.draw();
}