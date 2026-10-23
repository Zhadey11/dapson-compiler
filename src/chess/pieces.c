#include "chess.h"

#include <stdio.h>
#include <string.h>

/* Nombres de las piezas y de las casillas */

/* Los 16 nombres de un color; %c se cambia por b (blancas) o n (negras) */
static const char *base_names[16] = {
    "peon_%c1",    "peon_%c2",    "peon_%c3",   "peon_%c4",
    "peon_%c5",    "peon_%c6",    "peon_%c7",   "peon_%c8",
    "torre_%c1",   "torre_%c2",   "caballo_%c1", "caballo_%c2",
    "alfil_%c1",   "alfil_%c2",   "reina_%c",    "rey_%c"};

const char *chess_piece_name(int piece) {
  static char names[CHESS_PIECES][16];
  if (names[piece][0] == '\0') {
    snprintf(names[piece], sizeof(names[piece]), base_names[piece % 16],
             piece < 16 ? 'b' : 'n');
  }
  return names[piece];
}

int chess_piece_from_name(const char *name) {
  for (int piece = 0; piece < CHESS_PIECES; piece++) {
    if (strcmp(chess_piece_name(piece), name) == 0) {
      return piece;
    }
  }
  return -1;
}

PieceKind chess_piece_kind(int piece) {
  int number = piece % 16;
  if (number < 8) return KIND_PEON;
  if (number < 10) return KIND_TORRE;
  if (number < 12) return KIND_CABALLO;
  if (number < 14) return KIND_ALFIL;
  return number == 14 ? KIND_REINA : KIND_REY;
}

int chess_piece_is_white(int piece) { return piece < 16; }

/* "e4" -> columna e (4) + 8 * fila 4 (3) */
int chess_square_from_name(const char *name) {
  if (name[0] < 'a' || name[0] > 'h' || name[1] < '1' || name[1] > '8' ||
      name[2] != '\0') {
    return -1;
  }
  return (name[0] - 'a') + 8 * (name[1] - '1');
}

void chess_square_name(int square, char out[3]) {
  out[0] = (char)('a' + square % 8);
  out[1] = (char)('1' + square / 8);
  out[2] = '\0';
}