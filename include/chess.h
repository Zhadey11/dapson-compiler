#ifndef CHESS_H
#define CHESS_H

#include <stddef.h>

/*
 * Ajedrez en Dapson.
 *
 * Piezas: 32, numeradas asi
 *   0..15  blancas (b)        16..31  negras (n)
 *   dentro de cada color: peon 1..8 = 0..7, torre 1..2 = 8..9,
 *   caballo 1..2 = 10..11, alfil 1..2 = 12..13, reina = 14, rey = 15
 * Casillas: 64, numeradas a1 = 0, b1 = 1, ... h1 = 7, a2 = 8, ... h8 = 63
 */

#define CHESS_PIECES 32
#define CHESS_SQUARES 64

typedef enum {
  KIND_PEON,
  KIND_TORRE,
  KIND_CABALLO,
  KIND_ALFIL,
  KIND_REINA,
  KIND_REY
} PieceKind;

typedef struct {
  int square_of[CHESS_PIECES];  /* casilla de cada pieza (-1 si fue capturada) */
  int piece_at[CHESS_SQUARES];  /* pieza de cada casilla (-1 si esta vacia)    */
  int white_to_move;            /* 1 = le toca a las blancas                   */
} ChessBoard;

/* pieces.c: nombres, tipos y casillas */
int chess_piece_from_name(const char *name); /* "peon_b5" -> 4, o -1       */
const char *chess_piece_name(int piece);     /* 4 -> "peon_b5"             */
PieceKind chess_piece_kind(int piece);
int chess_piece_is_white(int piece);
int chess_square_from_name(const char *name); /* "e4" -> 28, o -1          */
void chess_square_name(int square, char out[3]);

/* board.c: el tablero y las reglas */
void chess_board_init(ChessBoard *board); /* posicion inicial, blancas primero */

/*
 * Mueve "piece" a la casilla "to" si la jugada es legal y devuelve 1
 * ("captured" = la pieza capturada, o -1). Si es ilegal no cambia nada,
 * devuelve 0 y deja en "message" el motivo.
 */
int chess_move(ChessBoard *board, int piece, int to, int *captured,
               char *message, size_t size);

#endif