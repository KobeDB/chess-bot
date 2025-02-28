#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "array.h"
#include "basic.h"

int bitScanForward(u64 bb);
int bitScanReverse(u64 bb);

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

inline bool in_bounds(int r, int c) {
    return r >= 0 && r < 8 && c >= 0 && c < 8; 
}

u64 ray_attacks[8][64] {};

inline u64 get_ray_attack_for_dir(int r, int c, int step_r, int step_c) {
    u64 dir_attacks = 0;
    int cur_r = r + step_r;
    int cur_c = c + step_c;
    while (in_bounds(cur_r, cur_c)) {
        dir_attacks |= (1ULL << to_index(cur_r, cur_c));
        cur_r += step_r;
        cur_c += step_c;
    }
    return dir_attacks;
}

inline void init_ray_attacks() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            ray_attacks[0][to_index(r,c)] |= get_ray_attack_for_dir(r,c,  1,  0);
            ray_attacks[1][to_index(r,c)] |= get_ray_attack_for_dir(r,c,  1,  1);
            ray_attacks[2][to_index(r,c)] |= get_ray_attack_for_dir(r,c,  0,  1);
            ray_attacks[3][to_index(r,c)] |= get_ray_attack_for_dir(r,c, -1,  1);
            ray_attacks[4][to_index(r,c)] |= get_ray_attack_for_dir(r,c, -1,  0);
            ray_attacks[5][to_index(r,c)] |= get_ray_attack_for_dir(r,c, -1, -1);
            ray_attacks[6][to_index(r,c)] |= get_ray_attack_for_dir(r,c,  0, -1);
            ray_attacks[7][to_index(r,c)] |= get_ray_attack_for_dir(r,c,  1, -1);
        }
    }
}

u64 knight_attacks[64] {};

inline u64 knight_attacks_for_pos(int r, int c) {
    u64 attacks = 0;

    const int offsets[] = {
        2,  1,
        2, -1,
       -2,  1,
       -2, -1,
        1,  2,
        1, -2,
       -1,  2,
       -1, -2
   };

   for (int off_i = 0; off_i < 8; ++off_i) {
       int off_r = offsets[off_i * 2];
       int off_c = offsets[off_i * 2 + 1];
       int dest_r = r + off_r;
       int dest_c = c + off_c;
       if (in_bounds(dest_r, dest_c)) attacks |= (1ULL << to_index(dest_r, dest_c));
   }

   return attacks;
}

inline void init_knight_attacks() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            knight_attacks[to_index(r,c)] = knight_attacks_for_pos(r,c);
        }
    }
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
    i8 captured_type = -1;
    i8 promotion_type = -1;
    i8 castling_rook_src = -1;
    i8 castling_rook_dest = -1;
};

struct Chess {
    u64 boards[2][6] {};
    u64 has_moved = 0;

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

    void post_process_pawn_move_and_push_onto_move_arena(Move &move, Array<Move> &move_arena) const {
        if ((turn == WHITE && move.dest / 8 == 7) || (turn == BLACK && move.dest / 8 == 0)) {
            move.promotion_type = QUEEN;
            move_arena.push(move);
            move.promotion_type = ROOK;
            move_arena.push(move);
            move.promotion_type = KNIGHT;
            move_arena.push(move);
            move.promotion_type = BISHOP;
            move_arena.push(move);
        } else {
            move_arena.push(move);
        }
    }

    struct Square_Info {
        i8 piece_type;
        i8 color;
    };

    struct Move_Arena_Span {
        size_t first;
        size_t opl;
    };

    Move_Arena_Span pseudo_legal_moves(Array<Move> &move_arena) const {
        Move_Arena_Span result {};
        
        result.first = move_arena.size();

        // I think this is a bad idea for perf.. have to fix later
        Square_Info board[64] {};
        for (int i = 0; i < 64; ++i) board[i] = {-1, -1};
        for (int color = 0; color < 2; ++color) {
            for (int p = 0; p < 6; ++p) {
                u64 bb = boards[color][p];
                while (bb) {
                    int index = bitScanForward(bb);
                    bb &= bb-1;
                    board[index] = {(i8)p, (i8)color};
                }
            }
        }

        u64 occupied[2] {};
        for (int color = 0; color < 2; ++color) for (int p = 0; p < 6; ++p) occupied[color] |= boards[color][p];

        u64 occupied_full = occupied[WHITE] + occupied[BLACK];

        const u64 empty = (occupied[0] | occupied[1]) ^ -1ULL;

        // pawn moves
        {
            u64 pawns = boards[turn][PAWN];

            u64 one_moves = turn == WHITE ? (pawns << 8) : (pawns >> 8);
            one_moves &= empty;
            while (one_moves) {
                int dest = bitScanForward(one_moves);
                one_moves &= one_moves-1;

                Move move {};
                move.src = turn == WHITE ? dest - 8 : dest + 8;
                move.dest = dest;
                move.piece_type = PAWN;

                post_process_pawn_move_and_push_onto_move_arena(move, move_arena);
            }

            u64 two_moves = turn == WHITE ? ((pawns & row_mask[1]) << 16) : ((pawns & row_mask[6]) >> 16);
            two_moves &= empty;
            while (two_moves) {
                int dest = bitScanForward(two_moves);
                two_moves &= two_moves-1;
                
                Move move {};
                move.src = turn == WHITE ? dest - 16 : dest + 16;
                move.dest = dest;
                move.piece_type = PAWN;
                //printf("src: %d, dest: %d\n", move.src, move.dest);

                move_arena.push(move);
            }

            const u64 not_file_a = 0xfefefefefefefefeULL;
            const u64 not_file_h = 0x7f7f7f7f7f7f7f7fULL;

            u64 left_attacks = turn == WHITE ? (pawns & not_file_a) << 7 : (pawns & not_file_h) >> 7;
            left_attacks &= occupied[turn == WHITE ? BLACK : WHITE];
            while (left_attacks) {
                int dest = bitScanForward(left_attacks);
                left_attacks &= left_attacks-1;

                Move move {};
                move.dest = dest;
                move.src = turn == WHITE ? dest-7 : dest+7;
                move.piece_type = PAWN;
                move.captured_type = board[move.dest].piece_type;

                post_process_pawn_move_and_push_onto_move_arena(move, move_arena);
            }

            u64 right_attacks = turn == WHITE ? (pawns & not_file_h) << 9 : (pawns & not_file_a) >> 9;
            right_attacks &= occupied[turn == WHITE ? BLACK : WHITE];
            while (right_attacks) {
                int dest = bitScanForward(right_attacks);
                right_attacks &= right_attacks-1;

                Move move {};
                move.dest = dest;
                move.src = turn == WHITE ? dest-9 : dest+9;
                move.piece_type = PAWN;
                move.captured_type = board[move.dest].piece_type;

                post_process_pawn_move_and_push_onto_move_arena(move, move_arena);
            }
        }

        // knight moves
        {
            u64 bb = boards[turn][KNIGHT];
            while (bb) {
                int src = bitScanForward(bb);
                bb &= bb-1;

                u64 attacks = knight_attacks[src] ^ (knight_attacks[src] & occupied[turn]);
                push_attacks_on_move_arena(attacks, src, KNIGHT, board, move_arena);
            }
        }

        // king moves
        {
            // TODO
        }

        //castling moves
        {
            if (turn == WHITE) {
                u64 has_not_moved = has_moved ^ -1ULL;
                u64 threats = get_threats(BLACK, occupied[WHITE], occupied[BLACK]);
                u64 not_threats = threats ^ -1ULL;

                bool queenside_rook_not_taken = boards[turn][ROOK] & 1ULL;
                if (queenside_rook_not_taken && (has_not_moved & 0b00010001ULL) == 0b00010001ULL) {
                    // queenside castling
                    if ((not_threats & 0b00011100ULL) == 0b00011100ULL && (empty & 0b00001110ULL) == 0b00001110ULL) {
                        Move move {};
                        move.src = 4; // we will move the king
                        move.dest = 2;
                        move.piece_type = KING;
                        move.castling_rook_src = 0;
                        move.castling_rook_dest = 3;
                        move_arena.push(move);
                    }
                }

                bool kingside_rook_not_taken = boards[turn][ROOK] & (1ULL << 7);
                if (kingside_rook_not_taken && (has_not_moved & 0b10010000ULL) == 0b10010000ULL) {
                    // kingside castling
                    if ((not_threats & 0b01110000ULL) == 0b01110000ULL &&
                        (empty & 0b01100000ULL) == 0b01100000ULL) {
                        Move move {};
                        move.src = 4;
                        move.dest = 6;
                        move.piece_type = KING;
                        move.castling_rook_src = 7;
                        move.castling_rook_dest = 5;
                        move_arena.push(move);
                    }
                }
            } else {
                u64 has_not_moved = has_moved ^ -1ULL;
                u64 threats = get_threats(WHITE, occupied[WHITE], occupied[BLACK]);
                u64 not_threats = threats ^ -1ULL;
        
                bool queenside_rook_not_taken = boards[turn][ROOK] & (1ULL << 56);
                if (queenside_rook_not_taken &&
                    (has_not_moved & ((1ULL << 60) | (1ULL << 56))) ==
                    ((1ULL << 60) | (1ULL << 56))) {

                    if ((not_threats & ((1ULL << 60) | (1ULL << 59) | (1ULL << 58))) ==
                        ((1ULL << 60) | (1ULL << 59) | (1ULL << 58)) &&
                        (empty & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59))) ==
                        ((1ULL << 57) | (1ULL << 58) | (1ULL << 59))) {
                        Move move {};
                        move.src = 60;
                        move.dest = 58;
                        move.piece_type = KING;
                        move.castling_rook_src = 56;
                        move.castling_rook_dest = 59;
                        move_arena.push(move);
                    }
                }
        
                bool kingside_rook_not_taken = boards[turn][ROOK] & (1ULL << 63);
                if (kingside_rook_not_taken &&
                    (has_not_moved & ((1ULL << 60) | (1ULL << 63))) ==
                    ((1ULL << 60) | (1ULL << 63))) {
                    if ((not_threats & ((1ULL << 60) | (1ULL << 61) | (1ULL << 62))) ==
                        ((1ULL << 60) | (1ULL << 61) | (1ULL << 62)) &&
                        (empty & ((1ULL << 61) | (1ULL << 62))) ==
                        ((1ULL << 61) | (1ULL << 62))) {
                        Move move {};
                        move.src = 60;
                        move.dest = 62;
                        move.piece_type = KING;
                        move.castling_rook_src = 63;
                        move.castling_rook_dest = 61;
                        move_arena.push(move);
                    }
                }
            }
        }

        // rook moves
        {
            u64 rooks = boards[turn][ROOK];
            while (rooks) {
                int rook_pos = bitScanForward(rooks);
                rooks &= rooks-1;

                u64 attacks = get_rook_threats(rook_pos, turn, occupied[WHITE], occupied[BLACK]);
                push_attacks_on_move_arena(attacks, rook_pos, ROOK, board, move_arena);
            }
        }

        // bishop threats
        {
            u64 bb = boards[turn][BISHOP];
            while (bb) {
                int pos = bitScanForward(bb);
                bb &= bb-1;

                u64 attacks = get_bishop_threats(pos, turn, occupied[WHITE], occupied[BLACK]);
                push_attacks_on_move_arena(attacks, pos, BISHOP, board, move_arena);
            }
        }

        // queen threats
        {
            u64 bb = boards[turn][QUEEN];
            while (bb) {
                int pos = bitScanForward(bb);
                bb &= bb-1;

                u64 attacks = get_queen_threats(pos, turn, occupied[WHITE], occupied[BLACK]);
                push_attacks_on_move_arena(attacks, pos, QUEEN, board, move_arena);
            }
        }

        result.opl = move_arena.size();
        return result;
    }

    void push_attacks_on_move_arena(u64 attacks, i8 pos, i8 piece_type, const Square_Info *board, Array<Move> &move_arena) const {
        while (attacks) {
            int dest = bitScanForward(attacks);
            attacks &= attacks-1;

            Move move {};
            move.src = pos;
            move.dest = dest;
            move.piece_type = piece_type;
            if (board[dest].piece_type != -1) {
                move.captured_type = board[dest].piece_type;
            }

            move_arena.push(move);
        }
    }

    u64 get_threats(i8 color, u64 occupied_white, u64 occupied_black) const {
        u64 result = 0;

        // pawn threats
        {
            u64 pawns = boards[color][PAWN];

            const u64 not_file_a = 0xfefefefefefefefeULL;
            const u64 not_file_h = 0x7f7f7f7f7f7f7f7fULL;

            u64 left_attacks = color == WHITE ? (pawns & not_file_a) << 7 : (pawns & not_file_h) >> 7;
            result |= left_attacks;

            u64 right_attacks = color == WHITE ? (pawns & not_file_h) << 9 : (pawns & not_file_a) >> 9;
            result |= right_attacks;
        }

        // knight threats
        {
            u64 bb = boards[color][KNIGHT];
            while (bb) {
                int src = bitScanForward(bb);
                bb &= bb-1;

                result |= knight_attacks[src] ^ (knight_attacks[src] & (color==WHITE ? occupied_white : occupied_black));
            }
        }

        // king threats
        {

        }

        // rook threats
        {
            u64 bb = boards[color][ROOK];
            while (bb) {
                int pos = bitScanForward(bb);
                bb &= bb-1;

                result |= get_rook_threats(pos, color, occupied_white, occupied_black);
            }
        }

        // bishop threats
        {
            u64 bb = boards[color][BISHOP];
            while (bb) {
                int pos = bitScanForward(bb);
                bb &= bb-1;

                result |= get_bishop_threats(pos, color, occupied_white, occupied_black);
            }
        }

        // queen threats
        {
            u64 bb = boards[color][QUEEN];
            while (bb) {
                int pos = bitScanForward(bb);
                bb &= bb-1;

                result |= get_queen_threats(pos, color, occupied_white, occupied_black);
            }
        }
        
        return result;
    }

    u64 get_rook_threats(i8 pos, i8 color, u64 occupied_white, u64 occupied_black) const {
        u64 result = 0;

        i8 piece_type = ROOK;
        result |= get_sliding_threats(0, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(2, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(4, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(6, pos, piece_type, color, occupied_white, occupied_black);

        return result;
    }

    u64 get_bishop_threats(i8 pos, i8 color, u64 occupied_white, u64 occupied_black) const {
        u64 result = 0;

        i8 piece_type = BISHOP;
        result |= get_sliding_threats(1, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(3, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(5, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(7, pos, piece_type, color, occupied_white, occupied_black);

        return result;
    }

    u64 get_queen_threats(i8 pos, i8 color, u64 occupied_white, u64 occupied_black) const {
        u64 result = 0;

        i8 piece_type = QUEEN;
        result |= get_sliding_threats(0, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(2, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(4, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(6, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(1, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(3, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(5, pos, piece_type, color, occupied_white, occupied_black);
        result |= get_sliding_threats(7, pos, piece_type, color, occupied_white, occupied_black);

        return result;
    }

    u64 get_sliding_threats(i8 dir, i8 pos, i8 piece_type, i8 piece_color, u64 occupied_white, u64 occupied_black) const {
        u64 attacks = 0;
        if (dir == 7 || dir == 0 || dir == 1 || dir == 2) {
            attacks = get_positive_ray_attacks(occupied_white | occupied_black, dir, pos);
        } else {
            attacks = get_negative_ray_attacks(occupied_white | occupied_black, dir, pos);
        }

        return attacks ^ (attacks & (piece_color==WHITE ? occupied_white : occupied_black));
    }

    u64 get_positive_ray_attacks(u64 occupied, i8 dir, i8 square) const {
        u64 attacks = ray_attacks[dir][square];
        u64 blockers = attacks & occupied;
        if (blockers) {
            int blocker_square = bitScanForward(blockers);
            attacks ^= ray_attacks[dir][blocker_square];
        }
        return attacks;
    }

    u64 get_negative_ray_attacks(u64 occupied, i8 dir, i8 square) const {
        u64 attacks = ray_attacks[dir][square];
        u64 blockers = attacks & occupied;
        if (blockers) {
            int blocker_square = bitScanReverse(blockers);
            attacks ^= ray_attacks[dir][blocker_square];
        }
        return attacks;
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
        boards[turn][move.piece_type] &= ((1ULL << move.src) ^ -1ULL);
        boards[turn][move.piece_type] |= (1ULL << move.dest);

        has_moved |= (1ULL << move.src);
        
        if (move.captured_type != -1) {
            boards[turn == WHITE ? BLACK : WHITE][move.captured_type] &= ((1ULL << move.dest) ^ -1ULL);
        }

        if (move.promotion_type != -1) {
            boards[turn][move.piece_type] &= ((1ULL << move.dest) ^ -1ULL);
            boards[turn][move.promotion_type] |= (1ULL << move.dest);
        }

        if (move.castling_rook_src != -1) {
            boards[turn][ROOK] &= ((1ULL << move.castling_rook_src) ^ -1ULL);
            boards[turn][ROOK] |= (1ULL << move.castling_rook_dest);
            has_moved |= (1ULL << move.castling_rook_src);
        }

        turn = turn==WHITE ? BLACK : WHITE;
    }

    void undo_move(const Move &move) {
        // TODO
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

/**
 * bitScanReverse
 * @authors Kim Walisch, Mark Dickinson
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of most significant one bit
 */
inline int bitScanReverse(u64 bb) {
   const u64 debruijn64 = 0x03f79d71b4cb0a89ULL;
   assert (bb != 0);
   bb |= bb >> 1; 
   bb |= bb >> 2;
   bb |= bb >> 4;
   bb |= bb >> 8;
   bb |= bb >> 16;
   bb |= bb >> 32;
   return index64[(bb * debruijn64) >> 58];
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

        if (move.dest == to_index(r,c) && piece_to_char(move.piece_type, chess.turn) == piece) {
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

    init_ray_attacks();
    init_knight_attacks();

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