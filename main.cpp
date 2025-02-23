#include <stdio.h>
#include <stdlib.h>

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
    bool is_castling = false; // if true, piece refers to the king
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

                    // check for promotions
                    for (int raw_move_i = 0; raw_move_i < raw_moves.size(); ++raw_move_i) {
                        Move &raw_move = raw_moves[raw_move_i];
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

                default: break;
            }
        }

        return result;
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



struct Minimax_Result {
    Move best_move;
    int value;
};

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
    for (int i = 0; i < legal_moves.size(); ++i) {
        const Move &move = legal_moves[i];
        if (move.dest_r == r && move.dest_c == c && chess.piece_to_char(chess.pieces[move.piece]) == piece) {
            move_ok = true;
            return move;
        }
    }

    move_ok = false;
    return {};
}

int main() {
    printf("Hello there\n");
    Chess chess {};
    chess.draw();
    print_legal_moves(chess, chess.legal_moves());

    while (true) {
        bool user_move_ok = false;
        while (!user_move_ok) {
            Move user_move = get_user_move(chess, user_move_ok);
            if (!user_move_ok) printf("That's an illegal move. Try Again...\n");
            else chess = chess.next_state(user_move);
        }
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