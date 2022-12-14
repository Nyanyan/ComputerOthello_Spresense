#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define TFT_DC 15
#define TFT_CS -1
#define TFT_RST 14

#define HW 8
#define HW2 64
#define HW_M1 7
#define HW2_M1 63
#define CELL_SIZE 25
#define BOARD_SIZE (CELL_SIZE * HW)
#define BOARD_SX 20
#define BOARD_SY 50
#define DOT_RADIUS 2
#define DISC_RADIUS 10
#define LEGAL_RADIUS 5

#define SET_BUTTON 0
#define PASS_BUTTON 1
#define SCORE_BUTTON 2
#define LEVEL_BUTTON 3
#define BUTTON_X_MIN 12
#define BUTTON_X_MAX 19
#define BUTTON_Y_MIN 4
#define BUTTON_Y_MAX 11

#define FONT_PX 6

#define MAX_LEVEL 3

const int button_com[5] = {28, 27, 26, 25, 22};
const int button_rec[4] = {21, 20, 19, 18};

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI5, TFT_DC, TFT_CS, TFT_RST);

int raw_level = 0;
bool level_last_pressed = false;

inline void swap(uint64_t *x, uint64_t *y) {
  *x ^= *y;
  *y ^= *x;
  *x ^= *y;
}

inline int pop_count_ull(uint64_t x) {
  x = x - ((x >> 1) & 0x5555555555555555ULL);
  x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
  x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
  x = (x * 0x0101010101010101ULL) >> 56;
  return x;
}

class Flip {
  public:
    uint_fast8_t pos;
    uint64_t flip;

  public:
    void calc_flip(const uint64_t me, const uint64_t op, int cell) {
      uint64_t rev = 0ULL;
      uint64_t rev2, mask;
      uint64_t pt = 1ULL << cell;
      for (uint8_t k = 0; k < 8; ++k) {
        rev2 = 0ULL;
        mask = trans(pt, k);
        while (mask && (mask & op)) {
          rev2 |= mask;
          mask = trans(mask, k);
        }
        if ((mask & me) != 0) {
          rev |= rev2;
        }
      }
      pos = cell;
      flip = rev;
    }

  private:
    inline uint64_t trans(const uint64_t pt, const int k) {
      switch (k) {
        case 0:
          return (pt << 8) & 0xFFFFFFFFFFFFFF00ULL;
        case 1:
          return (pt << 7) & 0x7F7F7F7F7F7F7F00ULL;
        case 2:
          return (pt >> 1) & 0x7F7F7F7F7F7F7F7FULL;
        case 3:
          return (pt >> 9) & 0x007F7F7F7F7F7F7FULL;
        case 4:
          return (pt >> 8) & 0x00FFFFFFFFFFFFFFULL;
        case 5:
          return (pt >> 7) & 0x00FEFEFEFEFEFEFEULL;
        case 6:
          return (pt << 1) & 0xFEFEFEFEFEFEFEFEULL;
        case 7:
          return (pt << 9) & 0xFEFEFEFEFEFEFE00ULL;
        default:
          return 0ULL;
      }
    }
};

inline uint64_t vertical_mirror(uint64_t x) {
  x = ((x >> 8) & 0x00FF00FF00FF00FFULL) | ((x << 8) & 0xFF00FF00FF00FF00ULL);
  x = ((x >> 16) & 0x0000FFFF0000FFFFULL) | ((x << 16) & 0xFFFF0000FFFF0000ULL);
  return ((x >> 32) & 0x00000000FFFFFFFFULL) | ((x << 32) & 0xFFFFFFFF00000000ULL);
}

inline uint64_t horizontal_mirror(uint64_t x) {
  x = ((x >> 1) & 0x5555555555555555ULL) | ((x << 1) & 0xAAAAAAAAAAAAAAAAULL);
  x = ((x >> 2) & 0x3333333333333333ULL) | ((x << 2) & 0xCCCCCCCCCCCCCCCCULL);
  return ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((x << 4) & 0xF0F0F0F0F0F0F0F0ULL);
}

inline uint64_t black_line_mirror(uint64_t x) {
  uint64_t a = (x ^ (x >> 9)) & 0x0055005500550055ULL;
  x = x ^ a ^ (a << 9);
  a = (x ^ (x >> 18)) & 0x0000333300003333ULL;
  x = x ^ a ^ (a << 18);
  a = (x ^ (x >> 36)) & 0x000000000F0F0F0FULL;
  return x = x ^ a ^ (a << 36);
}

inline uint64_t white_line_mirror(uint64_t x) {
  uint64_t a = (x ^ (x >> 7)) & 0x00AA00AA00AA00AAULL;
  x = x ^ a ^ (a << 7);
  a = (x ^ (x >> 14)) & 0x0000CCCC0000CCCCULL;
  x = x ^ a ^ (a << 14);
  a = (x ^ (x >> 28)) & 0x00000000F0F0F0F0ULL;
  return x = x ^ a ^ (a << 28);
}

inline uint64_t rotate_90(uint64_t x) {
  return vertical_mirror(white_line_mirror(x));
}

inline uint64_t rotate_270(uint64_t x) {
  return vertical_mirror(black_line_mirror(x));
}

inline uint64_t rotate_45(uint64_t x) {
  uint64_t a = (x ^ (x >> 8)) & 0x0055005500550055ULL;
  x = x ^ a ^ (a << 8);
  a = (x ^ (x >> 16)) & 0x0000CC660000CC66ULL;
  x = x ^ a ^ (a << 16);
  a = (x ^ (x >> 32)) & 0x00000000C3E1F078ULL;
  return x ^ a ^ (a << 32);
}

inline uint64_t unrotate_45(uint64_t x) {
  uint64_t a = (x ^ (x >> 32)) & 0x00000000C3E1F078ULL;
  x = x ^ a ^ (a << 32);
  a = (x ^ (x >> 16)) & 0x0000CC660000CC66ULL;
  x = x ^ a ^ (a << 16);
  a = (x ^ (x >> 8)) & 0x0055005500550055ULL;
  return x ^ a ^ (a << 8);
}

inline uint64_t rotate_135(uint64_t x) {
  uint64_t a = (x ^ (x >> 8)) & 0x00AA00AA00AA00AAULL;
  x = x ^ a ^ (a << 8);
  a = (x ^ (x >> 16)) & 0x0000336600003366ULL;
  x = x ^ a ^ (a << 16);
  a = (x ^ (x >> 32)) & 0x00000000C3870F1EULL;
  return x ^ a ^ (a << 32);
}

inline uint64_t unrotate_135(uint64_t x) {
  uint64_t a = (x ^ (x >> 32)) & 0x00000000C3870F1EULL;
  x = x ^ a ^ (a << 32);
  a = (x ^ (x >> 16)) & 0x0000336600003366ULL;
  x = x ^ a ^ (a << 16);
  a = (x ^ (x >> 8)) & 0x00AA00AA00AA00AAULL;
  return x ^ a ^ (a << 8);
}

inline uint64_t calc_legal(const uint64_t me, const uint64_t op) {
  // horizontal
  uint64_t p1 = (me & 0x7F7F7F7F7F7F7F7FULL) << 1;
  uint64_t legal = ~(p1 | op) & (p1 + (op & 0x7F7F7F7F7F7F7F7FULL));
  uint64_t me_proc = horizontal_mirror(me);
  uint64_t op_proc = horizontal_mirror(op);
  p1 = (me_proc & 0x7F7F7F7F7F7F7F7FULL) << 1;
  legal |= horizontal_mirror(~(p1 | op_proc) & (p1 + (op_proc & 0x7F7F7F7F7F7F7F7FULL)));

  // vertical
  me_proc = black_line_mirror(me);
  op_proc = black_line_mirror(op);
  p1 = (me_proc & 0x7F7F7F7F7F7F7F7FULL) << 1;
  uint64_t legal_proc = ~(p1 | op_proc) & (p1 + (op_proc & 0x7F7F7F7F7F7F7F7FULL));
  me_proc = horizontal_mirror(me_proc);
  op_proc = horizontal_mirror(op_proc);
  p1 = (me_proc & 0x7F7F7F7F7F7F7F7FULL) << 1;
  legal_proc |= horizontal_mirror(~(p1 | op_proc) & (p1 + (op_proc & 0x7F7F7F7F7F7F7F7FULL)));
  legal |= black_line_mirror(legal_proc);

  // 45 deg
  me_proc = rotate_45(me);
  op_proc = rotate_45(op);
  p1 = (me_proc & 0x5F6F777B7D7E7F3FULL) << 1;
  legal_proc = ~(p1 | op_proc) & (p1 + (op_proc & 0x5F6F777B7D7E7F3FULL));
  me_proc = horizontal_mirror(me_proc);
  op_proc = horizontal_mirror(op_proc);
  p1 = (me_proc & 0x7D7B776F5F3F7F7EULL) << 1;
  legal_proc |= horizontal_mirror(~(p1 | op_proc) & (p1 + (op_proc & 0x7D7B776F5F3F7F7EULL)));
  legal |= unrotate_45(legal_proc);

  // 135 deg
  me_proc = rotate_135(me);
  op_proc = rotate_135(op);
  p1 = (me_proc & 0x7D7B776F5F3F7F7EULL) << 1;
  legal_proc = ~(p1 | op_proc) & (p1 + (op_proc & 0x7D7B776F5F3F7F7EULL));
  me_proc = horizontal_mirror(me_proc);
  op_proc = horizontal_mirror(op_proc);
  p1 = (me_proc & 0x5F6F777B7D7E7F3FULL) << 1;
  legal_proc |= horizontal_mirror(~(p1 | op_proc) & (p1 + (op_proc & 0x5F6F777B7D7E7F3FULL)));
  legal |= unrotate_135(legal_proc);

  return legal & ~(me | op);
}

class Board {
  public:
    uint64_t player;
    uint64_t opponent;

  public:
    int operator == (Board a) {
      return player == a.player && opponent == a.opponent;
    }

    inline Board copy() {
      Board res;
      res.player = player;
      res.opponent = opponent;
      return res;
    }

    inline void copy(Board *res) {
      res->player = player;
      res->opponent = opponent;
    }

    inline void print() const {
      for (int i = HW2_M1; i >= 0; --i) {
        if (1 & (player >> i))
          Serial.print("X ");
        else if (1 & (opponent >> i))
          Serial.print("O ");
        else
          Serial.print(". ");
        if (i % HW == 0)
          Serial.println("");
      }
    }

    inline uint64_t get_legal() {
      return calc_legal(player, opponent);
    }

    inline void move_board(const Flip *flip) {
      player ^= flip->flip;
      opponent ^= flip->flip;
      player ^= 1ULL << flip->pos;
      swap(&player, &opponent);
    }

    inline void move_copy(const Flip *flip, Board *res) {
      res->opponent = player ^ flip->flip;
      res->player = opponent ^ flip->flip;
      res->opponent ^= 1ULL << flip->pos;
    }

    inline Board move_copy(const Flip *flip) {
      Board res;
      move_copy(flip, &res);
      return res;
    }

    inline void pass() {
      swap(&player, &opponent);
    }

    inline void undo_board(const Flip *flip) {
      swap(&player, &opponent);
      player ^= 1ULL << flip->pos;
      player ^= flip->flip;
      opponent ^= flip->flip;
    }

    inline int score_player() {
      int p_score = pop_count_ull(player), o_score = pop_count_ull(opponent);
      int score = p_score - o_score, vacant_score = HW2 - p_score - o_score;
      if (score > 0)
        score += vacant_score;
      else if (score < 0)
        score -= vacant_score;
      return score;
    }

    inline int count_player() {
      return pop_count_ull(player);
    }

    inline int count_opponent() {
      return pop_count_ull(opponent);
    }

    inline bool check_player() {
      bool passed = (get_legal() == 0);
      if (passed) {
        pass();
        passed = (get_legal() == 0);
        if (passed)
          pass();
      }
      return passed;
    }

    inline bool check_pass() {
      bool passed = (get_legal() == 0);
      if (passed) {
        pass();
        passed = (get_legal() == 0);
        if (passed)
          return false;
      }
      return true;
    }

    inline void reset() {
      player = 0x0000000810000000ULL;
      opponent = 0x0000001008000000ULL;
    }
};

int input_button() {
  int i, j, res = 0;
  for (i = 0; i < 5; ++i) {
    digitalWrite(button_com[i], HIGH);
  }
  for (i = 0; i < 5; ++i) {
    digitalWrite(button_com[i], LOW);
    for (j = 0; j < 4; ++j) {
      if (!digitalRead(button_rec[j])) {
        for (i = 0; i < 5; ++i) {
          digitalWrite(button_com[i], LOW);
        }
        //Serial.println(res);
        return res;
      }
      ++res;
    }
    digitalWrite(button_com[i], HIGH);
  }
  for (i = 0; i < 5; ++i) {
    digitalWrite(button_com[i], LOW);
  }
  return -1;
}

void print_discs(Board *board, int player);
void print_legal(Board *board);
void print_board(Board *board, int player);
void print_score(Board *board, int player);
void show_score(Board *board, int player);
int ai(Board *board);

void print_grid() {
  tft.fillScreen(ILI9341_DARKGREEN);
  int i, x, y;
  for (i = 0; i < HW + 1; ++i) {
    x = BOARD_SX + i * CELL_SIZE;
    y = BOARD_SY;
    tft.drawLine(x, y, x, y + BOARD_SIZE, ILI9341_BLACK);
    x = BOARD_SX;
    y = BOARD_SY + i * CELL_SIZE;
    tft.drawLine(x, y, x + BOARD_SIZE, y, ILI9341_BLACK);
  }
  tft.fillCircle(BOARD_SX + CELL_SIZE * 2, BOARD_SY + CELL_SIZE * 2, DOT_RADIUS, ILI9341_BLACK);
  tft.fillCircle(BOARD_SX + CELL_SIZE * 6, BOARD_SY + CELL_SIZE * 2, DOT_RADIUS, ILI9341_BLACK);
  tft.fillCircle(BOARD_SX + CELL_SIZE * 2, BOARD_SY + CELL_SIZE * 6, DOT_RADIUS, ILI9341_BLACK);
  tft.fillCircle(BOARD_SX + CELL_SIZE * 6, BOARD_SY + CELL_SIZE * 6, DOT_RADIUS, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1);
  for (i = 0; i < HW; ++i) {
    tft.setCursor(30 + i * CELL_SIZE, 40);
    tft.print((char)('A' + i));
  }
  for (i = 0; i < HW; ++i) {
    tft.setCursor(10, 60 + CELL_SIZE * i);
    tft.print((char)('1' + i));
  }
}

void print_discs(Board *board, int player) {
  int i, y, x;
  for (i = 0; i < HW2; ++i) {
    x = HW_M1 - i % HW;
    y = HW_M1 - i / HW;
    x = BOARD_SX + x * CELL_SIZE + CELL_SIZE / 2 + 1;
    y = BOARD_SY + y * CELL_SIZE + CELL_SIZE / 2 + 1;
    if (1 & (board->player >> i)) {
      if (player == 0)
        tft.fillCircle(x, y, DISC_RADIUS, ILI9341_BLACK);
      else
        tft.fillCircle(x, y, DISC_RADIUS, ILI9341_WHITE);
    } else if (1 & (board->opponent >> i)) {
      if (player == 0)
        tft.fillCircle(x, y, DISC_RADIUS, ILI9341_WHITE);
      else
        tft.fillCircle(x, y, DISC_RADIUS, ILI9341_BLACK);
    }
  }
}

void print_legal(Board *board) {
  uint64_t legal = board->get_legal();
  int i, y, x;
  for (i = 0; i < HW2; ++i) {
    x = HW_M1 - i % HW;
    y = HW_M1 - i / HW;
    x = BOARD_SX + x * CELL_SIZE + CELL_SIZE / 2 + 1;
    y = BOARD_SY + y * CELL_SIZE + CELL_SIZE / 2 + 1;
    if (1 & (legal >> i))
      tft.fillCircle(x, y, LEGAL_RADIUS, ILI9341_CYAN);
  }
}

void blink_place(int pos, bool state, int player) {
  pos = HW2_M1 - pos;
  int x = pos % HW;
  int y = pos / HW;
  x = BOARD_SX + x * CELL_SIZE + CELL_SIZE / 2 + 1;
  y = BOARD_SY + y * CELL_SIZE + CELL_SIZE / 2 + 1;
  if (state) {
    if (player == 0)
      tft.fillCircle(x, y, DISC_RADIUS, ILI9341_BLACK);
    else
      tft.fillCircle(x, y, DISC_RADIUS, ILI9341_WHITE);
  } else {
    tft.fillCircle(x, y, DISC_RADIUS, ILI9341_DARKGREEN);
    tft.fillCircle(x, y, LEGAL_RADIUS, ILI9341_CYAN);
  }
}

void print_score(Board *board, int player) {
  int b_score, w_score;
  if (player == 0) {
    b_score = board->count_player();
    w_score = board->count_opponent();
  } else {
    b_score = board->count_opponent();
    w_score = board->count_player();
  }
  tft.setTextSize(1);

  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(30, 20);
  tft.print((char)('0' + b_score / 10));
  tft.setCursor(30 + FONT_PX, 20);
  tft.print((char)('0' + b_score % 10));

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(240 - 30 - FONT_PX * 2, 20);
  tft.print((char)('0' + w_score / 10));
  tft.setCursor(240 - 30 - FONT_PX, 20);
  tft.print((char)('0' + w_score % 10));
}

void print_info() {
  int level = raw_level / 2;
  int ai_player = raw_level % 2;
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(70, 15);
  tft.print("Level ");
  tft.setCursor(70 + FONT_PX * 6, 15);
  tft.print((char)('1' + level));
  tft.setCursor(70, 25);
  if (ai_player == 0)
    tft.print("AI plays Black");
  else
    tft.print("AI plays White");
}

void print_board(Board *board, int player) {
  print_grid();
  print_discs(board, player);
  print_legal(board);
  print_score(board, player);
  print_info();
}

int x_button_pressed() {
  int b = input_button();
  if (BUTTON_X_MIN <= b && b <= BUTTON_X_MAX)
    return BUTTON_X_MAX - b;
  else if (b != -1)
    return -2;
  return -1;
}

int y_button_pressed() {
  int b = input_button();
  if (BUTTON_Y_MIN <= b && b <= BUTTON_Y_MAX)
    return BUTTON_Y_MAX - b;
  else if (b != -1)
    return -2;
  return -1;
}

int get_pos_button() {
  int x = -1, y = -1;
  while (x < 0)
    x = x_button_pressed();
  while (y < 0)
    y = y_button_pressed();
  if (x < 0 || y < 0)
    return -1;
  return HW2_M1 - (y * HW + x);
}

int input_pos(uint64_t legal, int player) {
  int pos = get_pos_button();
  if (pos >= 0) {
    if (1 & (legal >> pos)) {
      Serial.print("candidate: ");
      Serial.println(pos);
      int b = input_button();
      while (b != -1)
        b = input_button();
      unsigned long button_released = millis();
      unsigned long past = millis(), now = millis();
      bool blink_state = true;
      blink_place(pos, blink_state, player);
      while (b != SET_BUTTON) {
        if (millis() - button_released >= 200 && BUTTON_Y_MIN <= b && b <= BUTTON_X_MAX)
          return -1;
        now = millis();
        if (now - past >= 500) {
          past = now;
          blink_state ^= 1;
          blink_place(pos, blink_state, player);
        }
        b = input_button();
      }
      blink_place(pos, false, player);
      return pos;
    }
  }
  return -1;
}

void show_score(Board *board, int player) {
  if (input_button() == SCORE_BUTTON) {
    print_board(board, player);
    print_score(board, player);
    while (input_button() == SCORE_BUTTON);
    print_board(board, player);
  }
}

int ai(Board *board) {
  uint64_t legal = board->get_legal();
  return pop_count_ull((legal & -legal) - 1);
}

void play() {
  int level = raw_level / 2;
  int ai_player = raw_level % 2;
  int player = 0;
  Board board;
  board.reset();
  print_board(&board, player);
  uint64_t legal;
  int pos;
  Flip flip;
  while (true) {
    legal = board.get_legal();
    if (legal == 0) {
      if (pop_count_ull(board.player | board.opponent) == HW2)
        break;
      if (player != ai_player) {
        Serial.println("user have to pass");
        while (input_button() != PASS_BUTTON);
        board.pass();
        player ^= 1;
        board.print();
        print_board(&board, player);
        legal = board.get_legal();
        if (legal == 0) {
          Serial.println("AI have to pass");
          board.pass();
          player ^= 1;
          board.print();
          break;
        }
      } else {
        Serial.println("AI have to pass");
        board.pass();
        player ^= 1;
        board.print();
        print_board(&board, player);
        legal = board.get_legal();
        if (legal == 0) {
          Serial.println("user have to pass");
          //while (input_button() != PASS_BUTTON);
          board.pass();
          player ^= 1;
          board.print();
          break;
        }
      }
    }
    Serial.print("player: ");
    Serial.print(player);
    Serial.print(" AI is ");
    Serial.println(ai_player);
    print_board(&board, player);
    if (player == ai_player) {
      pos = ai(&board);
    } else {
      pos = -1;
      while (pos == -1)
        pos = input_pos(legal, player);
    }
    Serial.print("select: ");
    Serial.println(pos);
    flip.calc_flip(board.player, board.opponent, pos);
    board.move_board(&flip);
    board.print();
    player ^= 1;
  }
  print_board(&board, player);
  Serial.println("finished");
  while (input_button() != SET_BUTTON);
  Serial.println("back to menu");
}

void setup() {
  Serial.begin(115200);
  int i;
  for (i = 0; i < 5; ++i)
    pinMode(button_com[i], OUTPUT);
  for (i = 0; i < 4; ++i)
    pinMode(button_rec[i], INPUT_PULLUP);
  tft.begin(40000000);
  print_grid();
  print_info();
}

void loop(void) {
  int b = input_button();
  if (b == LEVEL_BUTTON && !level_last_pressed) {
    ++raw_level;
    raw_level %= MAX_LEVEL * 2;
    print_grid();
    print_info();
    delay(100);
  }
  level_last_pressed = b == LEVEL_BUTTON;
  if (b == SET_BUTTON) {
    play();
    print_grid();
    print_info();
    delay(300);
  }
}
