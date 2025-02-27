#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "array.h"
#include "basic.h"

#define PAWN    0
#define ROOK    1
#define KNIGHT  2
#define BISHOP  3
#define QUEEN   4
#define KING    5

#define WHITE 0
#define BLACK 1

// struct Move {
//     i8 piece;
//     i8 dest_r;
//     i8 dest_c;
//     i8 taken_piece = -1;
//     i8 promotion_type = -1; // if != -1, piece refers to the pawn, dest_r & dest_c to the promotion location, promotion_type to the promoted piece
//     i8 castling_rook = -1; // if != -1, piece refers to the king, dest_r & dest_c to the king's castling position, castling_rook to the rook we're castling with
// };

struct Move {
    i8 src;
    i8 dest;
    i8 piece_type;
    i8 color;
};

struct Chess {
    u64 boards[2][5] {};

    u64 pawns[2] {};
    u64 rooks[2] {};
    u64 knights[2] {};
    u64 bishops[2] {};
    u64 queens[2] {};
    u64 kings[2] {};

    i8 turn = WHITE;

    Chess() {
        reset();
    }

    void reset() {
        
        turn = WHITE;

        // init pawns
        boards[WHITE][PAWN] = 0b11111111ULL << 8;
        boards[BLACK][PAWN] = 0b11111111ULL << (8 * 6);
        pawns[WHITE] = 0b11111111ULL << 8;
        pawns[BLACK] = 0b11111111ULL << (8 * 6);

        // other pieces
        setup_back_pieces(WHITE);
        setup_back_pieces(BLACK);
    }

    void setup_back_pieces(i8 color) {
        rooks[color]    = 0b10000001ULL << (8 * 7 * color);
        knights[color]  = 0b01000010ULL << (8 * 7 * color);
        bishops[color]  = 0b00100100ULL << (8 * 7 * color);
        queens[color]   = 0b00001000ULL << (8 * 7 * color);
        kings[color]    = 0b00010000ULL << (8 * 7 * color);
    }

    struct Legal_Moves_Result {
        size_t first;
        size_t opl;
    };

    Legal_Moves_Result pseudo_legal_moves(Array<Move> &move_arena) const {
        Legal_Moves_Result result {};
        
        result.first = move_arena.size();
        defer ( result.opl = move_arena.size() );

        // pawn moves
        for (int color = 0; color < 2; ++color) {
            u64 occupied = 0;
            occupied |= pawns[0] | pawns[1];
            occupied |= rooks[0] | rooks[1];
            occupied |= knights[0] | knights[1];
            occupied |= bishops[0] | bishops[1];
            occupied |= queens[0] | queens[1];
            occupied |= kings[0] | kings[1];

            u64 empty = occupied ^ -1ULL;

            u64 forward_moves = (pawns[color] << 8) & empty;

            for (int i = 0; i < 64; ++i) {
                move_arena.push({});
            }
        }

        return result;
    }

    Chess next_state(const Move &move) const {
        return {};
    }

    bool is_check() const {
        return false;
    }

    // bool is_check_mate(Array<Move> &move_arena) const {
    //     auto moves = legal_moves(move_arena);
    //     return is_check() && moves.first == moves.opl;
    // }

    // bool is_stalemate(Array<Move> &move_arena) const {
    //     auto moves = legal_moves(move_arena);
    //     return !is_check() && moves.first == moves.opl;
    // }
    
    bool is_valid_pos(int row, int col) const {
        return row >= 0 && row < 8 && col >= 0 && col < 8; 
    }

    int to_index(int row, int col) const {
        if (!is_valid_pos(row, col)) {
            fprintf(stderr, "to_index: invalid row and col: %d, %d", row, col);
            exit(1);
        }
        return col + row * 8;
    }

    void draw() const {
        char board[64] {};
        for (int i = 0; i < 64; ++i) board[i] = '.';

        for (u64 i = 0; i < 64; ++i) {
            for (int color = 0; color < 2; ++color) {
                char capital_offset = color == WHITE ? 0 : 'a' - 'A';
                if (pawns[color]    & (1ULL << i))      board[i] = 'P' + capital_offset;
                else if (rooks[color]    & (1ULL << i))      board[i] = 'R' + capital_offset;
                else if (knights[color]  & (1ULL << i))      board[i] = 'N' + capital_offset;
                else if (bishops[color]  & (1ULL << i))      board[i] = 'B' + capital_offset;
                else if (queens[color]   & (1ULL << i))      board[i] = 'Q' + capital_offset;
                else if (kings[color]    & (1ULL << i))      board[i] = 'K' + capital_offset;
            }
        }

        printf("============\n");

        for (int r = 7; r >= 0; --r) {
            printf("%d  ", r + 1);
            for (int c = 0; c < 8; ++c) {
                printf("%c", board[c + 8*r]);
            }
            printf("\n");
        }

        printf("\n   ");
        for (int c = 0; c < 8; ++c) {
            printf("%c", 'a' + c);
        }
        printf("\n");
    }
};


int main() {

    printf("Hello there\n");

    Array<Move> move_arena {};
    move_arena.reserve(1000000000);
    move_arena.lock_capacity();

    Chess chess {};
    chess.draw();
}