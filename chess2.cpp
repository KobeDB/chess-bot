#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "array.h"
#include "basic.h"

int bitScanForward(u64 bb);

const u64 row_mask[8] = {
    0xffULL,
    0xff00ULL,
    0xff0000ULL,
    0xff000000ULL,
    0xff00000000ULL,
    0xff0000000000ULL,
    0xff000000000000ULL,
    0xff00000000000000ULL
};

inline int to_index(int r, int c) {
    return r * 8 + c;
}

#define PAWN    0
#define ROOK    1
#define KNIGHT  2
#define BISHOP  3
#define QUEEN   4
#define KING    5

#define WHITE 0
#define BLACK 1

struct Move {
    i8 src;
    i8 dest;
    i8 piece_type;
    i8 color;
    i8 captured_type;
};

struct Chess {
    u64 boards[2][6] {};

    i8 turn = WHITE;

    Chess() {
        reset();
    }

    void reset() {
        
        turn = WHITE;

        // init pawns
        boards[WHITE][PAWN] = (0b11111111ULL << 8);
        boards[BLACK][PAWN] = 0b11111111ULL << (8 * 6);

        // other pieces
        setup_back_pieces(WHITE);
        setup_back_pieces(BLACK);
    }

    void setup_back_pieces(i8 color) {
        boards[color][ROOK]    = 0b10000001ULL << (8 * 7 * color);
        boards[color][KNIGHT]  = 0b01000010ULL << (8 * 7 * color);
        boards[color][BISHOP]  = 0b00100100ULL << (8 * 7 * color);
        boards[color][QUEEN]   = 0b00001000ULL << (8 * 7 * color);
        boards[color][KING]    = 0b00010000ULL << (8 * 7 * color);
    }

    struct Move_Arena_Span {
        size_t first;
        size_t opl;
    };

    Move_Arena_Span pseudo_legal_moves(Array<Move> &move_arena) const {
        Move_Arena_Span result {};
        
        result.first = move_arena.size();

        // I think this is a bad idea for perf.. maybe fix later
        i8 board[64] {};
        for (int i = 0; i < 64; ++i) board[i] = -1;
        for (int color = 0; color < 2; ++color) {
            for (int p = 0; p < 6; ++p) {
                u64 bb = boards[color][p];
                while (bb) {
                    int index = bitScanForward(bb);
                    bb &= bb-1;
                    board[index] = (i8)p;
                }
            }
        }

        u64 occupied[2] {};
        for (int color = 0; color < 2; ++color) for (int p = 0; p < 6; ++p) occupied[color] |= boards[color][p];

        const u64 empty = (occupied[0] | occupied[1]) ^ -1ULL;

        // pawn moves
        for (int color = 0; color < 2; ++color) {
            u64 pawns = boards[color][PAWN];

            u64 one_moves = color == WHITE ? (pawns << 8) : (pawns >> 8);
            one_moves &= empty;
            while (one_moves) {
                int dest = bitScanForward(one_moves);
                one_moves &= one_moves-1;
                
                Move move {};
                move.src = color == WHITE ? dest - 8 : dest + 8;
                move.dest = dest;
                move.piece_type = PAWN;
                move.color = color;
                //printf("src: %d, dest: %d\n", move.src, move.dest);

                move_arena.push(move);
            }

            u64 two_moves = color == WHITE ? ((pawns & row_mask[1]) << 16) : ((pawns & row_mask[6]) >> 16);
            two_moves &= empty;
            while (two_moves) {
                int dest = bitScanForward(two_moves);
                two_moves &= two_moves-1;
                
                Move move {};
                move.src = color == WHITE ? dest - 16 : dest + 16;
                move.dest = dest;
                move.piece_type = PAWN;
                move.color = color;
                //printf("src: %d, dest: %d\n", move.src, move.dest);

                move_arena.push(move);
            }

            const u64 not_file_a = 0xfefefefefefefefeULL;
            const u64 not_file_h = 0x7f7f7f7f7f7f7f7fULL;

            u64 left_attacks = color == WHITE ? (pawns & not_file_a) << 7 : (pawns & not_file_h) >> 7;
            left_attacks &= occupied[color == WHITE ? BLACK : WHITE];
            //printf("Color: %d", color);
            //print_bitboard(left_attacks);
            while (left_attacks) {
                int dest = bitScanForward(left_attacks);
                left_attacks &= left_attacks-1;

                Move move {};
                move.dest = dest;
                move.src = color == WHITE ? dest-7 : dest+7;
                move.piece_type = PAWN;
                move.color = color;
                move.captured_type = board[move.dest];
                move_arena.push(move);
            }

            u64 right_attacks = color == WHITE ? (pawns & not_file_h) << 9 : (pawns & not_file_a) >> 9;
            right_attacks &= occupied[color == WHITE ? BLACK : WHITE];
            while (right_attacks) {
                int dest = bitScanForward(right_attacks);
                right_attacks &= right_attacks-1;

                Move move {};
                move.dest = dest;
                move.src = color == WHITE ? dest-9 : dest+9;
                move.piece_type = PAWN;
                move.color = color;
                move.captured_type = board[move.dest];
                move_arena.push(move);
            }
        }

        result.opl = move_arena.size();
        return result;
    }

    void print_bitboard(u64 bitboard) const {
        char board[64] {};
        for (int i = 0; i < 64; ++i) board[i] = '.';

        for (u64 i = 0; i < 64; ++i) {
            if (bitboard & (1ULL << i)) board[i] = 'X';
        }

        printf("============\n");

        for (int r = 7; r >= 0; --r) {
            printf("%d  ", r + 1);
            for (int c = 0; c < 8; ++c) {
                printf("%c ", board[c + 8*r]);
            }
            printf("\n");
        }

        printf("\n   ");
        for (int c = 0; c < 8; ++c) {
            printf("%c ", 'a' + c);
        }
        printf("\n");
    }

    void next_state(const Move &move) {
        boards[move.color][move.piece_type] &= ((1ULL << move.src) ^ -1ULL);
        boards[move.color][move.piece_type] |= (1ULL << move.dest);
        
        boards[move.color == WHITE ? BLACK : WHITE][move.captured_type] &= ((1ULL << move.dest) ^ -1ULL);
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
                if      (boards[color][PAWN]    & (1ULL << i))      board[i] = 'P' + capital_offset;
                else if (boards[color][ROOK]    & (1ULL << i))      board[i] = 'R' + capital_offset;
                else if (boards[color][KNIGHT]  & (1ULL << i))      board[i] = 'N' + capital_offset;
                else if (boards[color][BISHOP]  & (1ULL << i))      board[i] = 'B' + capital_offset;
                else if (boards[color][QUEEN]   & (1ULL << i))      board[i] = 'Q' + capital_offset;
                else if (boards[color][KING]    & (1ULL << i))      board[i] = 'K' + capital_offset;
            }
        }

        printf("============\n");

        for (int r = 7; r >= 0; --r) {
            printf("%d  ", r + 1);
            for (int c = 0; c < 8; ++c) {
                printf("%c ", board[c + 8*r]);
            }
            printf("\n");
        }

        printf("\n   ");
        for (int c = 0; c < 8; ++c) {
            printf("%c ", 'a' + c);
        }
        printf("\n");
    }
};

const int index64[64] = {
    0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

/**
 * bitScanForward
 * @author Kim Walisch (2012)
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
inline int bitScanForward(u64 bb) {
   const u64 debruijn64 = 0x03f79d71b4cb0a89ULL;
   assert (bb != 0);
   return index64[((bb ^ (bb-1)) * debruijn64) >> 58];
}

// void print_legal_moves(const Chess &chess, Chess::Legal_Moves_Result legal_moves, const Array<Move> &move_arena) {
//     printf("Legal moves:\n");   
//     for (size_t i = legal_moves.first; i < legal_moves.opl; ++i) {
//         const Move &move = move_arena[i];
//         const Piece &piece = chess.pieces[move.piece];
//         char pc = chess.piece_to_char(piece);
//         printf("%c(%d) to (%d, %d)\n", pc, piece.index, move.dest_r, move.dest_c);
//     }
// }

inline char piece_to_char(i8 piece_type, i8 color) {
    char result = '?';
    switch (piece_type) {
        case PAWN:      result = 'P'; break;
        case ROOK:      result = 'R'; break;
        case KNIGHT:    result = 'N'; break;
        case BISHOP:    result = 'B'; break;
        case QUEEN:     result = 'Q'; break;
        case KING:      result = 'K'; break;
        default: fprintf(stderr, "piece_to_char: invalid chess piece"); return '?';
    }

    if (color == BLACK) result += ('a' - 'A');

    return result;
}

Move get_user_move(Array<Move> &move_arena, Chess &chess, bool &move_ok) {
    auto legal_moves = chess.pseudo_legal_moves(move_arena);
    
    defer( move_arena.clear() );

    // print_legal_moves(chess, legal_moves, move_arena);
    printf("Give a move: ");
    char piece {};
    char col {};
    int row {};
    scanf(" %c%c%d", &piece, &col, &row);

    if (col < 'a' || col > 'h') {
        fprintf(stderr, "col: %c out of bounds\n", col);
        move_ok = false;
        return {};
    }
    if (row < 1 || row > 8) {
        fprintf(stderr, "row: %d out of bounds\n", row);
        move_ok = false;
        return {};
    }

    int c = (int)(col - 'a');
    int r = row - 1;

    printf("user moved: %c to (%d, %d)\n", piece, r, c);

    // find user's move in list of legal moves
    int ambiguous_moves[8] {};
    int ambiguous_moves_count = 0;

    for (size_t i = legal_moves.first; i < legal_moves.opl; ++i) {
        const Move &move = move_arena[i];

        if (move.dest == to_index(r,c) && piece_to_char(move.piece_type, move.color) == piece) {
            ambiguous_moves[ambiguous_moves_count] = i;
            ++ambiguous_moves_count;
        }

    }

    if(ambiguous_moves_count < 1) {
        move_ok = false;
        return {};
    }
    else if (ambiguous_moves_count == 1) {
        move_ok = true;
        return move_arena[ambiguous_moves[0]];
    }
    else {
        move_ok = true;
        printf("multiple possible pieces: \n");
        for (int i = 0; i < ambiguous_moves_count; ++i) {
            const Move &move = move_arena[ambiguous_moves[i]];
            
            int src_c = move.src % 8;
            int src_r = move.src / 8;

            char col_char = src_c + 'a';
            printf("%d: src=%c%d\n", i, col_char, src_r+1);
        }
        printf("Enter choice: ");
        int chosen_move = 0;
        scanf(" %d", &chosen_move);
        if (chosen_move < 0 || chosen_move >= ambiguous_moves_count) chosen_move = 0;
        printf("Chosen %d.\n", chosen_move);
        return move_arena[ambiguous_moves[chosen_move]];
    }
}

int main() {

    printf("Hello there\n");

    Array<Move> move_arena {};
    move_arena.reserve(1000000000);
    move_arena.lock_capacity();

    Chess chess {};
    chess.draw();

    chess.pseudo_legal_moves(move_arena);

    while (true) {
        bool user_move_ok = false;
        while (!user_move_ok) {
            Move user_move = get_user_move(move_arena, chess, user_move_ok);
            if (!user_move_ok) printf("That's an illegal move. Try Again...\n");
            else chess.next_state(user_move);
        }
        chess.draw();
    }
}