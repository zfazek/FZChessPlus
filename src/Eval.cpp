#include "Eval.h"

#include "Chess.h"
#include "Hash.h"

Eval::Eval(Chess *ch) : chess(ch) {
    hash = new Hash();
}

Eval::~Eval() {
    delete hash;
}

// Evaluates material and king position, pawn structure, etc
int Eval::evaluation_material(const int dpt) {
    int c;
    int evaking;
    int e = 0;
    const int *pt = chess->tablelist[chess->move_number];
    const struct position_t *pm = chess->movelist + chess->move_number;

    // Calculate summa of material for end game threshold
    // sm = sum_material(player_to_move);
    random_window = 10;

    // Goes through the table
    for (int i = 20; i < 100; ++i) {
        const int figure = *(pt + i);
        if (figure > 0 && figure < OFFBOARD) {
            // c=1 : own figure is found, c=-1 opposite figure is found
            if ((chess->player_to_move == chess->WHITE && (figure & 128) == 0) ||
                (chess->player_to_move == chess->BLACK && (figure & 128) == 128)) {
                c = 1;
            } else {
                c = -1;
            }

            // Evaulates King position (friendly pawn structure)
            if ((figure & 127) == chess->table->King &&
                chess->move_number > 6) {
                e += c * evaluation_king(i, figure);
            }

            // Evaulates Pawn structures
            if ((figure & 127) == chess->table->Pawn) {
                e += c * evaluation_pawn(i, figure, chess->sm);
            }

            // Calculates figure material values
            e += c * figure_value[(figure & 127)];
        }
    }

    if (chess->player_to_move == chess->WHITE) {
        c = 1;
    } else {
        c = -1;
    }

    // Bonus for castling
    e += c * (pm->white_king_castled - pm->black_king_castled);

    // Bonus for double bishops
    e += c * (pm->white_double_bishops - pm->black_double_bishops);

    // In the end game bonus if the enemy king is close
    if (chess->sm < end_game_threshold) {
        random_window = 2;
        evaking = 3 * abs((pm->pos_white_king / 10) - (pm->pos_black_king / 10));
        evaking += 3 * abs((pm->pos_white_king % 10) - (pm->pos_black_king % 10));
        if ((dpt % 2) == 0) {
            e += evaking;
        } else {
            e -= evaking;
        }

        // force to the corner
        if ((chess->player_to_move == chess->WHITE) && ((dpt % 2) == 1)) {
            evaking = 5 * (abs(5 - (pm->pos_black_king / 10)) +
                           abs(5 - (pm->pos_black_king % 10)));
        } else if ((chess->player_to_move == chess->WHITE) && ((dpt % 2) == 0)) {
            evaking = -5 * (abs(5 - (pm->pos_white_king / 10)) +
                            abs(5 - (pm->pos_white_king % 10)));
        } else if ((chess->player_to_move == chess->BLACK) && ((dpt % 2) == 0)) {
            evaking = -5 * (abs(5 - (pm->pos_black_king / 10)) +
                            abs(5 - (pm->pos_black_king % 10)));
        } else if ((chess->player_to_move == chess->BLACK) && ((dpt % 2) == 1)) {
            evaking = 5 * (abs(5 - (pm->pos_white_king / 10)) +
                           abs(5 - (pm->pos_white_king % 10)));
        }
        e += evaking;
    } else {
#ifdef CASTLING_PUNISHMENT

        // Punishment if can not castle
        if (pm->white_king_castled == 0 && (pm->castle & 3) == 0) {
            e += c * cant_castle;
        }
        if (pm->black_king_castled == 0 && (pm->castle & 12) == 0) {
            e += c * cant_castle;
        }
#endif
    }

    return e;
}

// Evaluates material plus number of legal moves of both sides plus a random number
int Eval::evaluation(const int e_legal_pointer, const int dpt) {
    hash->set_hash(chess);
    if (hash->posInHashtable()) {
        ++hash->hash_nodes;
        return hash->getU();
    }

    // not in the hashtable. normal evaluating
    if (chess->table->third_occurance() ||
        chess->table->is_not_enough_material() ||
        chess->movelist[chess->move_number].not_pawn_move >= 100) {
        hash->setU(chess->table->eval->DRAW);
        return chess->table->eval->DRAW;
    }
    chess->table->list_legal_moves();

    int lp = e_legal_pointer;
    chess->invert_player_to_move();
    chess->table->list_legal_moves();
    lp -= chess->legal_pointer;
    chess->invert_player_to_move();
    if (chess->legal_pointer == -1) { // No legal move
        if (!chess->table->is_attacked(
                chess->player_to_move == chess->WHITE
                    ? (chess->movelist + chess->move_number)->pos_black_king
                    : (chess->movelist + chess->move_number)->pos_white_king,
                -chess->player_to_move)) {
            return DRAW;
        } else {
            return WON - dpt;
        }
    }

    // Adds random to evaluation
    const int random_number = 0; // rand() % (random_window + 1);
    const int u = evaluation_material(dpt) + 2 * lp + random_number;

    // if this position is not in the hashtable->insert it to hashtable
    hash->setU(u);
    return u;
}

int Eval::evaluation_only_end_game(const int dpt) {
    chess->invert_player_to_move();
    chess->table->list_legal_moves();
    chess->invert_player_to_move();
    if (chess->legal_pointer == -1) { // No legal move
        if (!chess->table->is_attacked(
                chess->player_to_move == chess->WHITE
                    ? (chess->movelist + chess->move_number)->pos_black_king
                    : (chess->movelist + chess->move_number)->pos_white_king,
                -chess->player_to_move)) {
            // Stalemate
            return DRAW;
        } else {
            // Mate
            return WON - dpt;
        }
    }
    return 32767; // not end
}

// Bonus for king's adjacent own pawns
int Eval::evaluation_king(const int field, const int figure) {
    int e = 0;
    const int *pt = chess->tablelist[chess->move_number];
    const int *pdir = chess->table->dir_king;
    for (int k = 0; k < 8; ++k, ++pdir) {
        if (*(pt + field + *pdir) == (figure & 128) + chess->table->Pawn) {
            e += chess->table->eval->friendly_pawn;
        }
    }
    return e;
}

int Eval::evaluation_pawn(const int field, const int figure, const int sm) {
    int e = 0;

    // punishing double pawns
    int dir = 0;
    const int *pt = chess->tablelist[chess->move_number];
    do {
        dir += 10;
        if (*(pt + field + dir) == figure) {
            e += double_pawn;
        }
    } while (*(pt + field + dir) != OFFBOARD);
    dir = 0;
    do {
        dir -= 10;
        if (*(pt + field + dir) == figure) {
            e += double_pawn;
        }
    } while (*(pt + field + dir) != OFFBOARD);

    // Bonus for pawn advantage
    if (sm < 2000) {
        if ((figure & 128) == 0) {
            e += (field / 10) * pawn_advantage;
        } else {
            e += (11 - (field / 10)) * pawn_advantage;
        }
    }
    return e;
}

// Calculates material for evaluating end game threshold
int Eval::sum_material(const int color) {
    int e = 0;
    // int* pt = tablelist + move_number;
    for (int i = 20; i < 100; ++i) {
        const int figure = chess->tablelist[chess->move_number][i];
        if (figure > 0 && figure < OFFBOARD) {
            if ((color == chess->WHITE && (figure & 128) == 0) ||
                (color == chess->BLACK && (figure & 128) == 128)) {
                e += figure_value[(figure & 127)];
            }
        }
    }
    return e;
}
