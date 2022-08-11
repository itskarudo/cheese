#include <ctype.h>
#include <raylib.h>
#include <stdio.h>

#define PIECES_IMG_RATIO 3.0f

#define MIN(x, y) (x) < (y) ? (x) : (y)
#define OPPOSITE_COLOR (to_play == WHITE_PIECE ? BLACK_PIECE : WHITE_PIECE)

Texture2D boardTexture = {0};
Texture2D piecesTexture = {0};

enum GameColor { WHITE_PIECE = 1 << 3, BLACK_PIECE = 1 << 4 };

enum PieceType { NONE, KING, QUEEN, BISHOP, KNIGHT, ROOK, PAWN };

int board[64] = {0};

int selected_piece_index = -1;
int selected_piece = NONE;

struct Move {
  char starting_square;
  char target_square;
};

// TODO: replace this garbage with a dynamic array
struct Move moves[64 * 28];
int move_pointer = 0;

enum GameColor to_play = WHITE_PIECE;

const int movingDirections[8] = {-9, -8, -7, 1, 9, 8, 7, -1};

int directionOffsets[64][8];

void FenToPosition(char* fen) {
  enum State { POSITION, TO_PLAY, CASTLING, EN_PASSENT, HALFMOVE, FULLMOVE };

  enum State state = POSITION;

  int file = 0;
  int rank = 7;
  for (int i = 0; fen[i] != 0; i++) {
    char c = fen[i];

    switch (state) {
      case POSITION: {
        if (c == ' ') {
          state = TO_PLAY;
          continue;
        }
        if (c == '/') {
          file = 0;
          rank--;
        } else {
          if (isdigit(c)) {
            file += c - 48;
          } else {
            enum GameColor color = isupper(c) ? WHITE_PIECE : BLACK_PIECE;

            enum PieceType type;
            switch (tolower(c)) {
              case 'k':
                type = KING;
                break;
              case 'p':
                type = PAWN;
                break;
              case 'n':
                type = KNIGHT;
                break;
              case 'b':
                type = BISHOP;
                break;
              case 'r':
                type = ROOK;
                break;
              case 'q':
                type = QUEEN;
                break;
            }

            board[(7 - rank) * 8 + file] = type | color;
            file++;
          }
        }
        break;
      }
      case TO_PLAY: {
        if (c == ' ') {
          state = CASTLING;
          continue;
        }
        if (c == 'w')
          to_play = WHITE_PIECE;
        else if (c == 'b')
          to_play = BLACK_PIECE;

        break;
      }
      default:
        break;
    }
  }
}

void CalculateOffsets(void) {
  for (int i = 0; i < 64; i++) {
    directionOffsets[i][1] = i / 8;        // UP
    directionOffsets[i][3] = 7 - (i % 8);  // RIGHT
    directionOffsets[i][5] = 7 - (i / 8);  // BOTTOM
    directionOffsets[i][7] = i % 8;        // LEFT

    directionOffsets[i][0] =
        MIN(directionOffsets[i][1], directionOffsets[i][7]);  // UP LEFT
    directionOffsets[i][2] =
        MIN(directionOffsets[i][1], directionOffsets[i][3]);  // UP RIGHT
    directionOffsets[i][4] =
        MIN(directionOffsets[i][5], directionOffsets[i][3]);  // BOTTOM RIGHT
    directionOffsets[i][6] =
        MIN(directionOffsets[i][5], directionOffsets[i][7]);  // BOTTOM LEFT
  }
}

void GenerateBoard(void) {
  Image img = GenImageChecked(600, 600, 75, 75, (Color){0xf1, 0xd9, 0xc0, 0xff},
                              (Color){0xa9, 0x7a, 0x65, 0xff});

  boardTexture = LoadTextureFromImage(img);
  UnloadImage(img);
  SetTextureWrap(boardTexture, TEXTURE_WRAP_CLAMP);

  CalculateOffsets();

  FenToPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  /*FenToPosition("8/p7/k7/8/8/K7/P7/8");*/
}

void PlacePieces(void) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (board[i * 8 + j] == NONE) continue;

      Rectangle rec = {66.6f * ((board[i * 8 + j] & 7) - 1),
                       (board[i * 8 + j] & ~7) == WHITE_PIECE ? 0 : 66.6f,
                       66.6f, 66.6f};

      DrawTextureRec(piecesTexture, rec, (Vector2){5 + (j * 75), 5 + (i * 75)},
                     WHITE);
    }
  }
}

int CoordsToSquare(int x, int y) {
  int file = (int)(x / 75);
  int rank = (int)(y / 75);
  return (rank * 8) + file;
}

void UpdateBoard(int x, int y) {
  if (selected_piece_index != -1) {
    Rectangle rec = {66.6f * ((selected_piece & 7) - 1),
                     (selected_piece & WHITE_PIECE) ? 0 : 66.6f, 66.6f,
                     66.6f};

    DrawTextureRec(piecesTexture, rec, (Vector2){x - 37.5, y - 37.5}, WHITE);
  }
}

void CalculateGlidingPieceMoves(int square) {
  int initial = (board[square] & 7) == ROOK ? 1 : 0;
  int step = (board[square] & 7) == QUEEN ? 1 : 2;

  for (int direction = initial; direction < 8; direction += step) {
    for (int j = 1; j <= directionOffsets[square][direction]; j++) {
      if (board[square + movingDirections[direction] * j] & to_play)
        break;
      else {
        struct Move move = {
            .starting_square = square,
            .target_square = (square + movingDirections[direction] * j)};
        moves[move_pointer] = move;
        move_pointer++;
        if ((board[square + movingDirections[direction] * j] & 7) != NONE)
          break;
      }
    }
  }
}

void CalculateKingMoves(int square) {
  for (int direction = 0; direction < 8; direction++) {
    if (directionOffsets[square][direction] == 0) continue;
    if (board[square + movingDirections[direction]] & to_play) continue;

    struct Move move = {
        .starting_square = square,
        .target_square = (square + movingDirections[direction])};
    moves[move_pointer] = move;
    move_pointer++;
  }
}

void CalculateKnightMoves(int square) {
  for (int direction = 1; direction < 8; direction += 2) {
    if (directionOffsets[square][direction] < 2) continue;

    bool skip_1 = false;
    bool skip_2 = false;

    if (directionOffsets[square + movingDirections[direction] * 2]
                        [(direction + 2) % 8] == 0)
      skip_1 = true;
    if (directionOffsets[square + movingDirections[direction] * 2]
                        [(direction + 6) % 8] == 0)
      skip_2 = true;

    int sq1 = square + (movingDirections[direction] * 2) +
              (movingDirections[(direction + 2) % 8]);

    int sq2 = square + (movingDirections[direction] * 2) +
              (movingDirections[(direction + 6) % 8]);

    if (!(skip_1 || board[sq1] & to_play)) {
      struct Move move = {.starting_square = square, .target_square = sq1};
      moves[move_pointer] = move;
      move_pointer++;
    }
    if (!(skip_2 || board[sq2] & to_play)) {
      struct Move move = {.starting_square = square, .target_square = sq2};
      moves[move_pointer] = move;
      move_pointer++;
    }
  }
}

void CalculatePawnMoves(int square) {
  struct Move move;
  if (board[square] & WHITE_PIECE) {
    if ((directionOffsets[square + movingDirections[0]] > 0) && (board[square + movingDirections[0]] & OPPOSITE_COLOR))
    {
        move = (struct Move){.starting_square = square, .target_square = (square + movingDirections[0])};
        moves[move_pointer] = move;
        move_pointer++;
    }

    if ((directionOffsets[square + movingDirections[2]] > 0) && (board[square + movingDirections[2]] & OPPOSITE_COLOR))
    {
        move = (struct Move){.starting_square = square, .target_square = (square + movingDirections[2])};
        moves[move_pointer] = move;
        move_pointer++;
    }

    if (directionOffsets[square + movingDirections[1]] == 0) return;
    if ((board[square + movingDirections[1]] & 7) != NONE) return;

    move = (struct Move){.starting_square = square,
                         .target_square = (square + movingDirections[1])};
    moves[move_pointer] = move;
    move_pointer++;
    if ((int)(square / 8) == 6) {
      if ((board[square + movingDirections[1] * 2] & 7) != NONE) return;
      move = (struct Move){.starting_square = square,
                           .target_square = (square + movingDirections[1] * 2)};
      moves[move_pointer] = move;
      move_pointer++;
    }

  } else if (board[square] & BLACK_PIECE) {
    if ((directionOffsets[square + movingDirections[4]] > 0) && (board[square + movingDirections[4]] & OPPOSITE_COLOR))
    {
        move = (struct Move){.starting_square = square, .target_square = (square + movingDirections[4])};
        moves[move_pointer] = move;
        move_pointer++;
    }

    if ((directionOffsets[square + movingDirections[6]] > 0) && (board[square + movingDirections[6]] & OPPOSITE_COLOR))
    {
        move = (struct Move){.starting_square = square, .target_square = (square + movingDirections[6])};
        moves[move_pointer] = move;
        move_pointer++;
    }
    if (directionOffsets[square + movingDirections[5]] == 0) return;
    if ((board[square + movingDirections[5]] & 7) != NONE) return;

    move = (struct Move){.starting_square = square,
                         .target_square = (square + movingDirections[5])};
    moves[move_pointer] = move;
    move_pointer++;

    if ((int)(square / 8) == 1) {
      if ((board[square + movingDirections[5] * 2] & 7) != NONE) return;
      move = (struct Move){.starting_square = square,
                           .target_square = (square + movingDirections[5] * 2)};
      moves[move_pointer] = move;
      move_pointer++;
    }
  }
}

void CalculateLegalMoves(void) {
  for (int i = 0; i < 64; i++) {
    if ((board[i] & 7) == NONE || !(board[i] & to_play)) continue;

    if ((board[i] & 7) == QUEEN || (board[i] & 7) == ROOK ||
        (board[i] & 7) == BISHOP) {
      CalculateGlidingPieceMoves(i);
    } else if ((board[i] & 7) == KING) {
      CalculateKingMoves(i);
    } else if ((board[i] & 7) == KNIGHT) {
      CalculateKnightMoves(i);
    } else if ((board[i] & 7) == PAWN) {
      CalculatePawnMoves(i);
    }
  }
}
void ProcessNewTurn(void) {
  move_pointer = 0;
  CalculateLegalMoves();
}

void ProcessInput(void) {
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    int current_square = CoordsToSquare(GetMouseX(), GetMouseY());

    if (board[current_square] & to_play) {
      selected_piece_index = CoordsToSquare(GetMouseX(), GetMouseY());
      selected_piece = board[selected_piece_index];

      board[selected_piece_index] = NONE;
    }

  } else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    UpdateBoard(GetMouseX(), GetMouseY());

  } else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    int current_square = CoordsToSquare(GetMouseX(), GetMouseY());

    int i;
    for (i = 0; i < move_pointer; i++)
      if ((moves[i].starting_square == selected_piece_index) &&
          (moves[i].target_square == current_square))
        break;

    if (selected_piece_index != -1) {
      if (i < move_pointer)
        board[current_square] = selected_piece;
      else
        board[selected_piece_index] = selected_piece;

      if ((current_square != selected_piece_index) && (i < move_pointer)) {
        to_play = to_play == WHITE_PIECE ? BLACK_PIECE : WHITE_PIECE;
        ProcessNewTurn();
      }

      selected_piece_index = -1;
      selected_piece = NONE;
    }
  }
}

int main(int argc, char* argv[]) {
  InitWindow(600, 600, "penis");

  Image img = LoadImage("../res/pieces.png");
  ImageResize(&img, 400, 400 / PIECES_IMG_RATIO);

  piecesTexture = LoadTextureFromImage(img);

  GenerateBoard();

  ProcessNewTurn();

  while (!WindowShouldClose()) {
    BeginDrawing();
    {
      ClearBackground(WHITE);
      DrawTexture(boardTexture, 0, 0, WHITE);
      PlacePieces();

      int current_square = board[CoordsToSquare(GetMouseX(), GetMouseY())];

      if (current_square & to_play)
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
      else
        SetMouseCursor(MOUSE_CURSOR_ARROW);

      ProcessInput();
    }
    EndDrawing();
  }

  CloseWindow();

  return 0;
}
