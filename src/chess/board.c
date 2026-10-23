#include "chess.h"

#include <stdio.h>
#include <stdlib.h>

/* El tablero y las reglas de movimiento de cada pieza */

/* Casilla inicial de las 16 piezas blancas; las negras son la misma
   casilla reflejada (sq ^ 56 cambia la fila 1 por la 8, la 2 por la 7...) */
static const int white_start[16] = {8, 9, 10, 11, 12, 13, 14, 15, /* peones   */
                                    0, 7,                         /* torres   */
                                    1, 6,                         /* caballos */
                                    2, 5,                         /* alfiles  */
                                    3,                            /* reina    */
                                    4};                           /* rey      */

void chess_board_init(ChessBoard *board) {
  for (int square = 0; square < CHESS_SQUARES; square++) {
    board->piece_at[square] = -1;
  }
  for (int piece = 0; piece < CHESS_PIECES; piece++) {
    int square = white_start[piece % 16];
    if (piece >= 16) {
      square ^= 56;
    }
    board->square_of[piece] = square;
    board->piece_at[square] = piece;
  }
  board->white_to_move = 1;
}

/* Mensaje de jugada ilegal: "caballo_b1 no puede ir de b1 a b3 (motivo)" */
static int illegal(char *message, size_t size, int piece, int from, int to,
                   const char *why, const char *detail) {
  char from_name[3];
  char to_name[3];
  chess_square_name(from, from_name);
  chess_square_name(to, to_name);
  snprintf(message, size, "%s no puede ir de %s a %s (%s%s)",
           chess_piece_name(piece), from_name, to_name, why, detail);
  return 0;
}

static int sign(int value) { return (value > 0) - (value < 0); }

/* En linea recta o diagonal: hay alguna pieza entre "from" y "to"?
   Devuelve la casilla de la primera que encuentra, o -1 si el camino esta libre */
static int blocker(const ChessBoard *board, int from, int to) {
  int step = sign(to % 8 - from % 8) + 8 * sign(to / 8 - from / 8);
  for (int square = from + step; square != to; square += step) {
    if (board->piece_at[square] >= 0) {
      return square;
    }
  }
  return -1;
}

/* Una torre, un alfil o una reina: el camino debe estar libre */
static int slide(const ChessBoard *board, int piece, int from, int to,
                 int straight, int diagonal, const char *rule, char *message,
                 size_t size) {
  int df = abs(to % 8 - from % 8);
  int dr = abs(to / 8 - from / 8);
  int is_straight = df == 0 || dr == 0;
  int is_diagonal = df == dr;

  if (!((straight && is_straight) || (diagonal && is_diagonal))) {
    return illegal(message, size, piece, from, to, rule, "");
  }
  int between = blocker(board, from, to);
  if (between >= 0) {
    char name[3];
    chess_square_name(between, name);
    return illegal(message, size, piece, from, to,
                   "hay una pieza en el camino, en ", name);
  }
  return 1;
}

static int pawn_ok(const ChessBoard *board, int piece, int from, int to,
                   char *message, size_t size) {
  const char *rule =
      "el peon avanza 1 casilla, o 2 desde su fila inicial, y solo captura en diagonal";
  int white = chess_piece_is_white(piece);
  int df = to % 8 - from % 8;
  int dr = (to / 8 - from / 8) * (white ? 1 : -1); /* positivo = hacia adelante */
  int start_row = white ? 1 : 6;

  if (df == 0 && (dr == 1 || (dr == 2 && from / 8 == start_row))) {
    int between = blocker(board, from, to);
    if (between >= 0 || board->piece_at[to] >= 0) {
      char name[3];
      chess_square_name(between >= 0 ? between : to, name);
      return illegal(message, size, piece, from, to,
                     "el peon no captura de frente, hay una pieza en ", name);
    }
    return 1;
  }
  if ((df == 1 || df == -1) && dr == 1 && board->piece_at[to] >= 0) {
    return 1; /* captura en diagonal (de que color es se revisa antes) */
  }
  return illegal(message, size, piece, from, to, rule, "");
}

int chess_move(ChessBoard *board, int piece, int to, int *captured,
               char *message, size_t size) {
  *captured = -1;
  int from = board->square_of[piece];
  int white = chess_piece_is_white(piece);

  if (from < 0) {
    snprintf(message, size, "%s ya fue capturada", chess_piece_name(piece));
    return 0;
  }
  if (white != board->white_to_move) {
    snprintf(message, size, "no es el turno de las %s, le toca a las %s",
             white ? "blancas" : "negras", white ? "negras" : "blancas");
    return 0;
  }
  if (to == from) {
    return illegal(message, size, piece, from, to, "ya esta en esa casilla", "");
  }

  int target = board->piece_at[to];
  if (target >= 0 && chess_piece_is_white(target) == white) {
    char detail[24];
    snprintf(detail, sizeof(detail), "%s", chess_piece_name(target));
    return illegal(message, size, piece, from, to,
                   "ahi esta su propia pieza ", detail);
  }

  int ok;
  int df = abs(to % 8 - from % 8);
  int dr = abs(to / 8 - from / 8);
  switch (chess_piece_kind(piece)) {
  case KIND_PEON:
    ok = pawn_ok(board, piece, from, to, message, size);
    break;
  case KIND_TORRE:
    ok = slide(board, piece, from, to, 1, 0, "la torre se mueve en linea recta",
               message, size);
    break;
  case KIND_ALFIL:
    ok = slide(board, piece, from, to, 0, 1, "el alfil se mueve en diagonal",
               message, size);
    break;
  case KIND_REINA:
    ok = slide(board, piece, from, to, 1, 1,
               "la reina se mueve en linea recta o en diagonal", message, size);
    break;
  case KIND_CABALLO:
    ok = (df == 1 && dr == 2) || (df == 2 && dr == 1);
    if (!ok) {
      illegal(message, size, piece, from, to, "el caballo se mueve en L", "");
    }
    break;
  default: /* KIND_REY */
    ok = df <= 1 && dr <= 1;
    if (!ok) {
      illegal(message, size, piece, from, to, "el rey se mueve 1 casilla", "");
    }
    break;
  }
  if (!ok) {
    return 0;
  }

  /* Limites de esta primera etapa: ni el rey se captura ni el peon corona */
  if (target >= 0 && chess_piece_kind(target) == KIND_REY) {
    return illegal(message, size, piece, from, to,
                   "el rey no se puede capturar", "");
  }
  if (chess_piece_kind(piece) == KIND_PEON && (to / 8 == 0 || to / 8 == 7)) {
    return illegal(message, size, piece, from, to,
                   "la coronacion del peon aun no esta disponible", "");
  }

  /* la jugada es legal: se hace */
  if (target >= 0) {
    board->square_of[target] = -1;
    *captured = target;
  }
  board->piece_at[from] = -1;
  board->piece_at[to] = piece;
  board->square_of[piece] = to;
  board->white_to_move = !board->white_to_move;
  return 1;
}