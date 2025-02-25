#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "array.h"
#include "basic.h"

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
    bool is_promotion = false; // if true, piece refers to the pawn, dest_r & dest_c to the promotion location, promoted_type to the promoted piece
    Piece_Type promotion_type;
    bool is_castling = false; // if true, piece refers to the king, dest_r & dest_c to the king's castling position
    int castling_rook = -1; // refers to the rook we are castling with
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

    Array<Move> legal_moves(bool check_self_check = true) const {
        Array<Move> result {};
        result.reserve(40);

        const Piece *board[64] {};
        to_board(board);

        for (int i = 0; i < 32; ++i) {
            const Piece &piece = pieces[i];
            if (piece.taken) continue;
            if (piece.color != turn) continue;
            
            switch (piece.type) {
                
                case PAWN: {
                    Array<Move> raw_moves {};
                    defer( raw_moves.destroy() );

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

                        raw_moves.push(move);
                    }

                    // capturing
                    for (int attack_c_off = -1; attack_c_off <= 1; ++attack_c_off) {
                        if (attack_c_off == 0) continue;

                        int attack_c = piece.c + attack_c_off;
                        int attack_r = piece.r + step_dir;

                        if (!is_valid_pos(attack_r, attack_c)) continue;
                        
                        const Piece *attacked_piece = board[to_index(attack_r, attack_c)];
                        
                        if (attacked_piece && attacked_piece->color != piece.color) {
                            Move move {};
                            move.piece = i;
                            move.dest_r = attack_r;
                            move.dest_c = attack_c;
                            move.taken_piece = attacked_piece->index;

                            raw_moves.push(move);
                        }
                    }

                    // TODO: en-passant moves

                    // check for promotions
                    for (int raw_move_i = 0; raw_move_i < raw_moves.size(); ++raw_move_i) {
                        Move &raw_move = raw_moves[raw_move_i];

                        if (check_self_check && does_move_cause_self_check(raw_move)) continue;
                        
                        if (raw_move.dest_r == 7 && piece.color == WHITE || raw_move.dest_r == 0 && piece.color == BLACK) {
                            // The pawn must promote => push only promotion moves in result

                            raw_move.is_promotion = true;
                            
                            raw_move.promotion_type = QUEEN;
                            result.push(raw_move);
                            raw_move.promotion_type = ROOK;
                            result.push(raw_move);
                            raw_move.promotion_type = KNIGHT;
                            result.push(raw_move);
                            raw_move.promotion_type = BISHOP;
                            result.push(raw_move);
                        } else {
                            result.push(raw_move);
                        }
                    }
                } break;

                case ROOK: {
                    add_legal_moves_for_direction(result, i, piece.r, piece.c, -1,  0, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c,  1,  0, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c,  0, -1, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c,  0,  1, board, check_self_check);
                    // castling, don't add castling moves if check_self_check is false as a castling move cannot capture any pieces (check_self_check is false only because we want to return threatened pieces/squares)
                    if (!piece.has_moved && check_self_check) {
                        // search for king
                        const Piece *piece_at_king_pos = board[to_index(piece.r, 4)];
                        if (piece_at_king_pos && piece_at_king_pos->type == KING && piece_at_king_pos->color == piece.color && !piece_at_king_pos->has_moved) {
                            // piece_at_king_pos is the king we can castle with
                            // check if every square between this rook and the king is empty
                            // + check if neighboring squares near king are non-threatened
                            bool castling_allowed = true;
                            
                            if (is_check()) castling_allowed = false;

                            if (piece.c == 0) {
                                for (int c = 1; c < 4; ++c) if (board[to_index(piece.r, c)]) castling_allowed = false;
                                Move move {};
                                move.piece = piece_at_king_pos->index;
                                move.dest_r = piece_at_king_pos->r;
                                move.dest_c = 3;
                                if (castling_allowed && does_move_cause_self_check(move)) castling_allowed = false;
                            }
                            else {
                                // => piece.c == 7
                                for (int c = 5; c < 7; ++c) if (board[to_index(piece.r, c)]) castling_allowed = false;
                                Move move {};
                                move.piece = piece_at_king_pos->index;
                                move.dest_r = piece_at_king_pos->r;
                                move.dest_c = 5;
                                if (castling_allowed && does_move_cause_self_check(move)) castling_allowed = false;
                            }

                            if (castling_allowed) {
                                Move move {};
                                move.piece = piece_at_king_pos->index;
                                move.dest_r = piece_at_king_pos->r;
                                move.dest_c = (piece_at_king_pos->c - piece.c) > 0 ? 2 : 6;
                                move.is_castling = true;
                                move.castling_rook = piece.index;
                                if (!does_move_cause_self_check(move)) {
                                    result.push(move);
                                }
                            }
                        } 
                    }
                } break;

                case BISHOP: {
                    add_legal_moves_for_direction(result, i, piece.r, piece.c, -1, -1, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c, -1,  1, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c,  1, -1, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c,  1,  1, board, check_self_check);
                } break;

                case QUEEN: {
                    add_legal_moves_for_direction(result, i, piece.r, piece.c, -1,  0, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c,  1,  0, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c,  0, -1, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c,  0,  1, board, check_self_check);

                    add_legal_moves_for_direction(result, i, piece.r, piece.c, -1, -1, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c, -1,  1, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c,  1, -1, board, check_self_check);
                    add_legal_moves_for_direction(result, i, piece.r, piece.c,  1,  1, board, check_self_check);
                } break;

                case KNIGHT: {
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
                        int dest_r = piece.r + off_r;
                        int dest_c = piece.c + off_c;
                        maybe_add_move(result, piece, dest_r, dest_c, board, check_self_check);
                    }
                    
                } break;

                case KING: {
                    for (int off_r = -1; off_r <= 1; ++off_r) {
                        for (int off_c = -1; off_c <= 1; ++off_c) {
                            if (off_r == 0 && off_c == 0) continue;
                            int dest_r = piece.r + off_r;
                            int dest_c = piece.c + off_c;
                            maybe_add_move(result, piece, dest_r, dest_c, board, check_self_check);
                        }
                    }
                } break;

                default: break;
            }
        }

        return result;
    }

    bool maybe_add_move(Array<Move> &legal_moves, const Piece &piece, int dest_r, int dest_c, const Piece **board, bool check_self_check) const {
        if (is_valid_pos(dest_r, dest_c)) {
            const Piece *other_piece = board[to_index(dest_r, dest_c)];

            if (other_piece && other_piece->color == piece.color) return false;
            
            Move move {};
            move.piece = piece.index;
            move.dest_r = dest_r;
            move.dest_c = dest_c;
            
            if (other_piece && other_piece->color != piece.color) {
                move.taken_piece = other_piece->index;
            }

            if (!check_self_check || !does_move_cause_self_check(move)) {
                legal_moves.push(move);
                return true;
            }
        }
        return false;
    }

    void add_legal_moves_for_direction(Array<Move> &legal_moves, int piece_i, int r, int c, int step_r, int step_c, const Piece **board, bool check_self_check) const {
        int cur_r = r + step_r;
        int cur_c = c + step_c;
        while (is_valid_pos(cur_r, cur_c)) {
            defer ( { cur_r += step_r; cur_c += step_c; } );
            const Piece *other_piece = board[to_index(cur_r, cur_c)];
            if (other_piece && other_piece->color != turn) {
                Move move {};
                move.piece = piece_i;
                move.dest_r = cur_r;
                move.dest_c = cur_c;
                move.taken_piece = other_piece->index;

                if (!check_self_check || !does_move_cause_self_check(move)) {
                    legal_moves.push(move);
                }
                
                break; // Break to not teleport through the piece
            }

            if (other_piece && other_piece->color == turn) break; // Break to not teleport through the piece

            // other_piece == nullptr => empty square
            Move move {};
            move.piece = piece_i;
            move.dest_r = cur_r;
            move.dest_c = cur_c;
            if (!check_self_check || !does_move_cause_self_check(move)) {
                legal_moves.push(move);
            }
        }
    }

    void get_taken_pieces(int *taken_pieces, int &taken_pieces_count, int color) const {
        taken_pieces_count = 0;
        for (int i = 0; i <32; ++i) {
            const Piece &piece = pieces[i];
            if (piece.taken && piece.color == color) {
                taken_pieces[taken_pieces_count] = i;
                ++taken_pieces_count;
            }
        }
    }

    Chess next_state(const Move &move) const {
        Chess next = *this;

        next.turn = turn == WHITE ? BLACK : WHITE;

        next.pieces[move.piece].r = move.dest_r;
        next.pieces[move.piece].c = move.dest_c;
        next.pieces[move.piece].has_moved = true;

        if (move.taken_piece != -1) {
            next.pieces[move.taken_piece].taken = true;
            next.pieces[move.taken_piece].r = -1;
            next.pieces[move.taken_piece].c = -1;
        }

        if (move.is_promotion) {
            next.pieces[move.piece].type = move.promotion_type;
        }

        if (move.is_castling) {
            // we already moved the king (move.piece refers to the king), now move the rook
            next.pieces[move.castling_rook].r = move.dest_r;
            next.pieces[move.castling_rook].c = move.dest_c == 2 ? 3 : 5;
        }
        
        return next;
    }

    bool is_check() const {
        const Piece *king {};
        for (int i = 0; i < 32; ++i) if (pieces[i].type == KING && pieces[i].color == turn) king = &pieces[i];
        assert(king);
        
        // "move" king to the same position it is already at
        Move move {};
        move.piece = king->index;
        move.dest_r = king->r;
        move.dest_c = king->c;

        return does_move_cause_self_check(move);
    }

    bool is_check_mate() const {
        auto moves = legal_moves();
        defer (moves.destroy());
        return is_check() && moves.size() == 0;
    }

    bool is_stalemate() const {
        auto moves = legal_moves();
        defer (moves.destroy());
        return !is_check() && moves.size() == 0;
    }

    bool does_move_cause_self_check(const Move &move) const {
        Chess simulated_move_state = next_state(move);

        const Piece *board[64] {};
        simulated_move_state.to_board(board);

        u64 threatened = 0;
        int first_enemy_piece_index = turn == WHITE ? 16 : 0;
        int opl_enemy_piece_index = turn == WHITE ? 32 : 16;
        for (int i = first_enemy_piece_index; i < opl_enemy_piece_index; ++i) {
            const Piece &piece = simulated_move_state.pieces[i];
            if (piece.taken) continue;
            switch(piece.type) {
                case PAWN: {
                    u64 pawn_threats = 0;
                    if (piece.color == WHITE) {
                        if (is_valid_pos(piece.r+1,piece.c-1)) pawn_threats |= (1ULL << to_index(piece.r+1, piece.c-1));
                        if (is_valid_pos(piece.r+1,piece.c+1)) pawn_threats |= (1ULL << to_index(piece.r+1, piece.c+1));
                    } else {
                        if (is_valid_pos(piece.r-1,piece.c-1)) pawn_threats |= (1ULL << to_index(piece.r-1, piece.c-1));
                        if (is_valid_pos(piece.r-1,piece.c+1)) pawn_threats |= (1ULL << to_index(piece.r-1, piece.c+1));
                    }
                    threatened |= pawn_threats;
                } break;
                case ROOK: {
                    u64 threats = 0;
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color, -1,  0);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color,  1,  0);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color,  0, -1);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color,  0,  1);
                    threatened |= threats;
                } break;
                case BISHOP: {
                    u64 threats = 0;
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color, -1,  -1);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color, -1,   1);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color,  1,  -1);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color,  1,   1);
                    threatened |= threats;
                } break;
                case QUEEN: {
                    u64 threats = 0;
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color, -1,  0);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color,  1,  0);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color,  0, -1);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color,  0,  1);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color, -1,  -1);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color, -1,   1);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color,  1,  -1);
                    threats |= calc_threats_dir(board, piece.r, piece.c, piece.color,  1,   1);
                    threatened |= threats;
                } break;
                case KNIGHT: {
                    u64 threats = 0;
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
                       int dest_r = piece.r + off_r;
                       int dest_c = piece.c + off_c;
                       if (is_valid_pos(dest_r, dest_c)) threats |= (1ULL << to_index(dest_r, dest_c));
                   }
                   threatened |= threats;
                } break;
                case KING: {
                    u64 threats = 0;
                    for (int off_r = -1; off_r <= 1; ++off_r) {
                        for (int off_c = -1; off_c <= 1; ++off_c) {
                            if (off_r == 0 && off_c == 0) continue;
                            int dest_r = piece.r + off_r;
                            int dest_c = piece.c + off_c;
                            if (is_valid_pos(dest_r, dest_c)) threats |= (1ULL << to_index(dest_r, dest_c));
                        }
                    }
                    threatened |= threats;
                } break;
            }
        }

        int king_index = turn == WHITE ? 12 : 12 + 16;
        const Piece &king = simulated_move_state.pieces[king_index];
        assert(king.type == KING);

        return ( threatened & (1ULL << to_index(king.r, king.c)) ) > 0;
    }

    u64 calc_threats_dir(const Piece **board, int r, int c, int piece_color, int step_r, int step_c) const {
        u64 threats = 0;

        int cur_r = r + step_r;
        int cur_c = c + step_c;
        while (is_valid_pos(cur_r, cur_c)) {
            defer ( { cur_r += step_r; cur_c += step_c; } );
            const Piece *other_piece = board[to_index(cur_r, cur_c)];
            if (!other_piece || other_piece->color != piece_color) {
                threats |= (1ULL << to_index(cur_r, cur_c));
            }
            if (other_piece) break; // break to not teleport through the piece
        }

        return threats;
    }

    // bool does_move_cause_self_check(const Move &move) const {
    //     Chess simulated_move_state = next_state(move);

    //     auto legal_moves = simulated_move_state.legal_moves(false);
    //     defer( legal_moves.destroy() );

    //     for (int i = 0; i < legal_moves.size(); ++i) {
    //         const Move &legal_move = legal_moves[i];
    //         const Piece &taken_piece = simulated_move_state.pieces[legal_move.taken_piece];
    //         if (taken_piece.type == KING && taken_piece.color == turn) {
    //             return true;
    //         }
    //     }

    //     return false;
    // }

    static char piece_to_char(const Piece &piece) {        
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
};

float evaluate_board(const Chess &chess) {
    float value = 0.0f;
    for (int i = 0; i < 32; ++i) {
        const Piece &piece = chess.pieces[i];
        if (!piece.taken) {
            float piece_value = 0.0f;
            switch (piece.type) {
                case PAWN: piece_value = 1.0f; break;
                case KNIGHT: piece_value = 5.0f; break;
                case ROOK: piece_value = 10.0f; break;
                case BISHOP: piece_value = 10.0f; break;
                case QUEEN: piece_value = 90.0f; break;
                case KING: piece_value = 100.0f; break; // TODO: this is probably stupid, fix it soon
            }
            if (piece.color == WHITE)
                value += piece_value;
            else
                value -= piece_value;
        }
    }
    return value;
}

struct Minimax_Result {
    Move best_move;
    float value;
};

int minimax_calls = 0;

float minimax(const Chess &chess, int depth, int max_depth, Move *best_move, float alpha, float beta);

Minimax_Result minimax(const Chess &chess) {
    minimax_calls = 0;

    clock_t start = clock();

    Move best_move {};
    float value = minimax(chess, 0, 5, &best_move, -999999.0f, 999999.0f);

    clock_t end = clock();
    double elapsed = ((double)(end-start))/CLOCKS_PER_SEC;

    double minimax_calls_per_second = ((double)minimax_calls)/elapsed;
    printf("Minimax calls/s: %f\n", minimax_calls_per_second);

    return {best_move, value};
}

float minimax(const Chess &chess, int depth, int max_depth, Move *best_move, float alpha, float beta) {
    ++minimax_calls;
    if ((minimax_calls % 10000) == 0) printf("nodes visited: %d\n", minimax_calls);

    if (chess.is_check_mate()) {
        float value = chess.turn == WHITE ? -10000.0f : 10000.0f;
        return value;
    }

    if (chess.is_stalemate()) {
        return 0;
    }

    if (depth >= max_depth) {
        return evaluate_board(chess);
    }

    float best_value = chess.turn == WHITE ? -99999.0f : 99999.0f;

    auto moves = chess.legal_moves();
    defer (moves.destroy());

    for (int i = 0; i < moves.size(); ++i) {
        const Move &move = moves[i];
        float child_value = minimax(chess.next_state(move), depth+1, max_depth, nullptr, alpha, beta);

        if (chess.turn == WHITE) {
            if (child_value > best_value) {
                best_value = child_value;
                if (best_move) *best_move = move;
            }
            if (best_value > alpha) {
                alpha = best_value;
            }
            if (alpha >= beta) break;
        }
        else {
            if (child_value < best_value) {
                best_value = child_value;
                if (best_move) *best_move = move;
            }
            if (best_value < beta) {
                beta = best_value;
            }
            if (beta <= alpha) break;
        }
    }

    return best_value;
}

void print_legal_moves(const Chess &chess, const Array<Move> &legal_moves) {
    printf("Legal moves:\n");   
    for (int i = 0; i < legal_moves.size(); ++i) {
        const Move &move = legal_moves[i];
        const Piece &piece = chess.pieces[move.piece];
        char pc = chess.piece_to_char(piece);
        printf("%c(%d) to (%d, %d)\n", pc, piece.index, move.dest_r, move.dest_c);
    }
}

Move get_user_move(Chess &chess, bool &move_ok) {
    auto legal_moves = chess.legal_moves();
    defer( legal_moves.destroy() );
    print_legal_moves(chess, legal_moves);
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

    for (int i = 0; i < legal_moves.size(); ++i) {
        const Move &move = legal_moves[i];
        if (move.dest_r == r && move.dest_c == c && chess.piece_to_char(chess.pieces[move.piece]) == piece) {
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
        return legal_moves[ambiguous_moves[0]];
    }
    else {
        move_ok = true;
        printf("multiple possible pieces: \n");
        for (int i = 0; i < ambiguous_moves_count; ++i) {
            const Move &move = legal_moves[ambiguous_moves[i]];
            const Piece &piece = chess.pieces[move.piece];
            char col_char = piece.c + 'a';
            printf("%d: src=%c%d\n", i, col_char, piece.r+1);
        }
        printf("Enter choice: ");
        int chosen_move = 0;
        scanf(" %d", &chosen_move);
        if (chosen_move < 0 || chosen_move >= ambiguous_moves_count) chosen_move = 0;
        printf("Chosen %d.\n", chosen_move);
        return legal_moves[ambiguous_moves[chosen_move]];
    }
}

void print_move(const Move &move, const Chess &chess) {
    printf("move: ");
    char piece_char = Chess::piece_to_char(chess.pieces[move.piece]);
    char col_char = move.dest_c + 'a';
    printf("move: %c to %c%d\n", piece_char, col_char, move.dest_r + 1);
}

int main() {
    printf("Hello there\n");
    Chess chess {};
    chess.draw();

    while (true) {
        bool user_move_ok = false;
        while (!user_move_ok) {
            Move user_move = get_user_move(chess, user_move_ok);
            if (!user_move_ok) printf("That's an illegal move. Try Again...\n");
            else chess = chess.next_state(user_move);
        }
        chess.draw();
        Minimax_Result cpu_move = minimax(chess);
        chess = chess.next_state(cpu_move.best_move);
        print_move(cpu_move.best_move, chess);
        chess.draw();
    }


    Move move {};
    move.piece = 0;
    move.dest_c = 0;
    move.dest_r = 5;
    chess = chess.next_state(move);
    chess.draw();

    auto legal_moves = chess.legal_moves();
    print_legal_moves(chess, legal_moves);
}