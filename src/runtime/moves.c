#include "chess.h"
#include "runtime_internal.h"

#include <stdio.h>

/* mover pieza hacia casilla;  el tablero es uno solo para todo el programa */

static ChessBoard board;
static int board_ready = 0;

int runtime_execute_move(const ASTNode *node) {
  if (!board_ready) {
    chess_board_init(&board);
    board_ready = 1;
  }

  int piece = node->data.move.piece;
  int to = node->data.move.square;
  int from = board.square_of[piece];
  int captured;
  char message[160];

  if (!chess_move(&board, piece, to, &captured, message, sizeof(message))) {
    return runtime_error(node->line, "movimiento ilegal: %s", message);
  }

  char from_name[3];
  char to_name[3];
  chess_square_name(from, from_name);
  chess_square_name(to, to_name);
  if (captured >= 0) {
    printf("%s: %s -> %s (captura a %s)\n", chess_piece_name(piece), from_name,
           to_name, chess_piece_name(captured));
  } else {
    printf("%s: %s -> %s\n", chess_piece_name(piece), from_name, to_name);
  }
  return 1;
}