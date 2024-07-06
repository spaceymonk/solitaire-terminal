#define VERSION "1.1"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#define STB_KEYPRESS_IMPLEMENTATION
#include "stb_keypress.h"

#define LEN(array)             (sizeof(array) / sizeof((array)[0]))
#define MOD(dividend, divisor) ((((int)(dividend)) % ((int)(divisor)) + ((int)(divisor))) % ((int)(divisor)))

#define CARD_WIDTH        7
#define CARD_HEIGHT       5
#define GAP_HORIZONTAL    2
#define GAP_VERTICAL      2
#define OFFSET_VERTICAL   (CARD_HEIGHT / 3 + 1)
#define OFFSET_HORIZONTAL (2 * CARD_WIDTH / 3)
#define BOARD_HEIGHT      (CARD_HEIGHT + GAP_VERTICAL + CARD_HEIGHT * 6)
#define BOARD_WIDTH       (CARD_WIDTH * 7 + GAP_HORIZONTAL * 6 )
#define BOARD_SIZE        (BOARD_HEIGHT * BOARD_WIDTH)
#define BOARD_POS(x, y)   ((y) * BOARD_WIDTH + (x))

#define DECK_SIZE              52
#define LAST_NTH_CARD_OF(x, y) ((x).cards[(x).size-(y)])
#define LAST_CARD_OF(x)        LAST_NTH_CARD_OF(x, 1)
#define SUITE_OF(x)            ((x).number / 13)
#define RANK_OF(x)             ((x).number % 13)

#define TERM_RESET          (0 << 24)
#define TERM_BOLD           (1 << 24)
#define TERM_FAINT          (2 << 24)
#define TERM_ITALIC         (3 << 24)
#define TERM_UNDERLINE      (4 << 24)
#define TERM_BLINKING       (5 << 24)
#define TERM_REVERSE        (7 << 24)
#define TERM_HIDDEN         (8 << 24)
#define TERM_STRIKETHROUGH  (9 << 24)

#define TERM_BG_BLACK       (40 << 16)
#define TERM_BG_RED         (41 << 16)
#define TERM_BG_GREEN       (42 << 16)
#define TERM_BG_YELLOW      (43 << 16)
#define TERM_BG_BLUE        (44 << 16)
#define TERM_BG_MAGENTA     (45 << 16)
#define TERM_BG_CYAN        (46 << 16)
#define TERM_BG_WHITE       (47 << 16)
#define TERM_BG_DEFAULT     (49 << 16)

#define TERM_FG_BLACK       (30 << 8)
#define TERM_FG_RED         (31 << 8)
#define TERM_FG_GREEN       (32 << 8)
#define TERM_FG_YELLOW      (33 << 8)
#define TERM_FG_BLUE        (34 << 8)
#define TERM_FG_MAGENTA     (35 << 8)
#define TERM_FG_CYAN        (36 << 8)
#define TERM_FG_WHITE       (37 << 8)
#define TERM_FG_DEFAULT     (39 << 8)

const char suite_symbols[] = { 'H', 'D', 'S', 'C' };
const char rank_symbols[]  = { 'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K' }; 

typedef struct Card {
    int  number;
    bool hidden;
    bool selected;
    bool dragged;
} Card;

typedef struct Pile {
    Card   cards[DECK_SIZE];
    size_t size;
} Pile;

typedef struct Selection {
    int pile_idx;
    int card_idx;
} Selection;

void PrintPixel(uint32_t pixel_data)
{
    uint8_t term        = (pixel_data >> 24) & 0xff;
    uint8_t color_bg    = (pixel_data >> 16) & 0xff;
    uint8_t color_fg    = (pixel_data >>  8) & 0xff;
    char symbol         = pixel_data & 0xff;
    printf("\x1B[%d;%d;%dm%c\x1B[0m", term, color_bg, color_fg, symbol);
}

void PrintBuffer(uint32_t *buffer)
{
    for (size_t row=0; row<BOARD_HEIGHT; ++row) {
        for (size_t col=0; col<BOARD_WIDTH; ++col) {
            PrintPixel(buffer[BOARD_POS(col, row)]);
        }
        printf("\n");
    }
}

void DrawRectangle(uint32_t *buffer, int x, int y, size_t w, size_t h, uint32_t value)
{
    if (x < 0 || y < 0 || x+w > BOARD_WIDTH || y+h > BOARD_HEIGHT) {
        fprintf(stderr, "%s:%d: Cannot draw rectangle due to out of bounds", __FILE__, __LINE__);
        return;
    }
    for (size_t i=0; i<w; ++i) {
        buffer[BOARD_POS(x+i, y    )] = value;
        buffer[BOARD_POS(x+i, y+h-1)] = value;
    }
    for (size_t i=0; i<h; ++i) {
        buffer[BOARD_POS(x    , y+i)] = value;
        buffer[BOARD_POS(x+w-1, y+i)] = value;
    }
    buffer[BOARD_POS(x    , y    )] = value;
    buffer[BOARD_POS(x    , y+h-1)] = value;
    buffer[BOARD_POS(x+w-1, y    )] = value;
    buffer[BOARD_POS(x+w-1, y+h-1)] = value;
}

void FillRectangle(uint32_t *buffer, int x, int y, size_t w, size_t h, uint32_t value)
{
    if (x < 0 || y < 0 || x+w > BOARD_WIDTH || y+h > BOARD_HEIGHT) {
        fprintf(stderr, "%s:%d: Cannot draw rectangle due to out of bounds", __FILE__, __LINE__);
        return;
    }
    for (int i=1; i<h-1; ++i) {
        for (int j=1; j<w-1; ++j) {
            buffer[BOARD_POS(x+j, y+i)] = value;
        }
    }
}

void RenderCard(uint32_t *buffer, int x, int y, int card_number, bool hidden, bool selected, bool dragged)
{
    if (card_number > 52 || card_number < 0) {
        fprintf(stderr, "%s:%d: Card number should between 0 and 51 (inclusive), but passed %d", __FILE__, __LINE__, card_number);
        return;
    }
    int suite = card_number / 13;
    int rank  = card_number % 13;
    
    if (selected) {
        DrawRectangle(buffer, x, y, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | TERM_BG_YELLOW | ' ');
    } else if (dragged) {
        DrawRectangle(buffer, x, y, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | TERM_BG_GREEN | ' ');
    } else {
        DrawRectangle(buffer, x, y, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | TERM_BG_BLUE | ' ');
    }

    if (hidden) {
        FillRectangle(buffer, x, y, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | TERM_BG_MAGENTA | ' ');
    } else {
        uint32_t bg_color = suite < 2 ? TERM_BG_RED : TERM_BG_BLACK;
        FillRectangle(buffer, x, y, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | bg_color | ' ');
        buffer[BOARD_POS(x+1,            y+1)]             = TERM_BOLD | TERM_FG_WHITE | bg_color | rank_symbols[rank];
        buffer[BOARD_POS(x+CARD_WIDTH-2, y+1)]             = TERM_BOLD | TERM_FG_WHITE | bg_color | suite_symbols[suite];
        buffer[BOARD_POS(x+CARD_WIDTH-2, y+CARD_HEIGHT-2)] = TERM_BOLD | TERM_FG_WHITE | bg_color | rank_symbols[rank];
        buffer[BOARD_POS(x+1,            y+CARD_HEIGHT-2)] = TERM_BOLD | TERM_FG_WHITE | bg_color | suite_symbols[suite];
    }
}

void RenderPiles(uint32_t *buffer, Pile *deck, Pile *poll, Pile *columns, Pile *foundations, Pile *piles[], int selected_pile_idx)
{
    /* Draw Deck Pile */
    if (piles[selected_pile_idx] == deck) {
        DrawRectangle(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 6, 0, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | TERM_BG_YELLOW | ' ');
    } else {
        DrawRectangle(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 6, 0, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | TERM_BG_CYAN | ' ');
    }
    if (deck->size >= 1) {
        RenderCard(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 6, 0,
                    LAST_CARD_OF(*deck).number, LAST_CARD_OF(*deck).hidden, LAST_CARD_OF(*deck).selected, LAST_CARD_OF(*deck).dragged);
    }
    /* Draw Poll Pile */
    if (poll->size == 1) {
        RenderCard(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 0, 0, 
                    LAST_CARD_OF(*poll).number, LAST_CARD_OF(*poll).hidden, LAST_CARD_OF(*poll).selected, LAST_CARD_OF(*poll).dragged);
    } else if (poll->size == 2) {
        RenderCard(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 0, 0, 
                    LAST_NTH_CARD_OF(*poll, 2).number, LAST_NTH_CARD_OF(*poll, 2).hidden, LAST_NTH_CARD_OF(*poll, 2).selected, LAST_NTH_CARD_OF(*poll, 2).dragged);
        RenderCard(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 1, 0, 
                    LAST_CARD_OF(*poll).number, LAST_CARD_OF(*poll).hidden, LAST_CARD_OF(*poll).selected, LAST_CARD_OF(*poll).dragged);
    } else if (poll->size >= 3) {
        RenderCard(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 0, 0, 
                    LAST_NTH_CARD_OF(*poll, 3).number, LAST_NTH_CARD_OF(*poll, 3).hidden, LAST_NTH_CARD_OF(*poll, 3).selected, LAST_NTH_CARD_OF(*poll, 3).dragged);
        RenderCard(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 1, 0, 
                    LAST_NTH_CARD_OF(*poll, 2).number, LAST_NTH_CARD_OF(*poll, 2).hidden, LAST_NTH_CARD_OF(*poll, 2).selected, LAST_NTH_CARD_OF(*poll, 2).dragged);
        RenderCard(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 2, 0, 
                    LAST_CARD_OF(*poll).number, LAST_CARD_OF(*poll).hidden, LAST_CARD_OF(*poll).selected, LAST_CARD_OF(*poll).dragged);
    }
    /* Draw Column Piles */
    for (int i=0; i<7; ++i) {
        if (piles[selected_pile_idx] == &columns[i]) {
            DrawRectangle(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * i, CARD_HEIGHT + GAP_VERTICAL, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | TERM_BG_YELLOW | ' ');
        } else {
            DrawRectangle(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * i, CARD_HEIGHT + GAP_VERTICAL, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | TERM_BG_CYAN | ' ');
        }
    }
    for (size_t i=0; i<7; ++i) {
        Pile column = columns[i];
        for (size_t j=0; j<column.size; ++j) {
            Card card = column.cards[j];
            RenderCard(buffer,
                (CARD_WIDTH  + GAP_HORIZONTAL) * i,
                (OFFSET_VERTICAL * j) + (CARD_HEIGHT + GAP_VERTICAL),
                card.number, card.hidden, card.selected, card.dragged); 
        }
    }
    /* Draw Foundation Piles */
    for (int i=0; i<4; ++i) {
        if (piles[selected_pile_idx] == &foundations[i]) {
            DrawRectangle(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * i, 0, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | TERM_BG_YELLOW | ' ');
        } else {
            DrawRectangle(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * i, 0, CARD_WIDTH, CARD_HEIGHT, TERM_FG_DEFAULT | TERM_BG_CYAN | ' ');
        }
    }
    for (size_t i=0; i<4; ++i) {
        Pile foundation = foundations[i];
        if (foundation.size >= 1) {
            RenderCard(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * i, 0, 
                        LAST_CARD_OF(foundation).number, LAST_CARD_OF(foundation).hidden, LAST_CARD_OF(foundation).selected, LAST_CARD_OF(foundation).dragged);
        }
    }
}

bool IsGameFinished(Pile *foundations)
{
    int filled = 0;
    for (size_t i=0; i<4; ++i) {
        if (foundations[i].size == 13) {
            filled++;
        }
    }
    return filled == 4;
}

int GetIndexOfPile(Pile *piles[], size_t pile_count, Pile *pile)
{
    for (size_t i=0; i<pile_count; ++i) {
        if (pile == piles[i]) {
            return i;
        }
    }
    return -1;
}

int main()
{
    uint32_t *buffer = (uint32_t*) malloc(BOARD_SIZE * sizeof(uint32_t));
    if (buffer == NULL) {
        fprintf(stderr, "%s:%d: Couldn't allocate buffer memory", __FILE__, __LINE__);
        return 1;
    }

    /* size_t seed = 1720019880; */
    /* size_t seed = 1720205317; */
    size_t seed = time(NULL);
    printf("Seed: %ld\n", seed);
    srand(seed);
    Pile deck = { .size = DECK_SIZE };
    for (size_t i=0; i<deck.size; ++i) {
        Card card     = { .number = i, .hidden = true };
        deck.cards[i] = card;
    }
    for (size_t i=0; i<deck.size; ++i) {
        int rand_index = rand() % deck.size;
        Card tmp = deck.cards[i];
        deck.cards[i] = deck.cards[rand_index];
        deck.cards[rand_index] = tmp;
    }
    Pile poll           = { 0 };
    Pile foundations[4] = { 0 }; 
    Pile columns[7]     = { 0 };
    for (size_t i=0; i<7; ++i) {
        Pile column = { .size = i + 1 };
        for (size_t j=0; j<column.size; ++j) {
            column.cards[j] = deck.cards[deck.size - 1];
            deck.size--;
        }
        LAST_CARD_OF(column).hidden = false;
        columns[i] = column;
    }

    int turn_count     = 0;
    char status[256]   = {0};
    bool gameover      = false;
    Pile *piles[] = { &foundations[0], &foundations[1], &foundations[2], &foundations[3], &poll, &deck,
        &columns[0], &columns[1], &columns[2], &columns[3], &columns[4], &columns[5], &columns[6] };
    const size_t pile_count = LEN(piles);
    Selection selected = { .pile_idx = GetIndexOfPile(piles, pile_count, &deck), .card_idx = deck.size-1 };
    deck.cards[deck.size-1].selected = true;
    Selection dragged = { .pile_idx = -1, .card_idx = -1};
    while(!gameover) {
        /* Print Game State */
        memset(buffer, ' ', BOARD_SIZE * sizeof(uint32_t));
        RenderPiles(buffer, &deck, &poll, columns, foundations, piles, selected.pile_idx);
        PrintBuffer(buffer);

        /* Check Game Over */
        if (IsGameFinished(foundations) == true) {
            printf("\x1B[2KCongratulations! You solved it in %d turns.", turn_count);
            gameover = true;
            break;
        }

        /* Get User Input */
        printf("\x1B[2K[Turn #%d] %s\n", turn_count, status);
        status[0] = '\0';
        char key_pressed = GetKeyPress();
        printf("\x1B[%dF", BOARD_HEIGHT + 1);
        switch (key_pressed) {
            case 'q': {  /* Quit */
                gameover = true;
            } break;
            case 's': {  /* Traverse within Pile (only for columns) */
                if (selected.pile_idx >= GetIndexOfPile(piles, pile_count, &columns[0])) {
                    Pile *column = piles[selected.pile_idx];
                    column->cards[selected.card_idx].selected = false;
                    selected.card_idx = MOD(selected.card_idx + 1, column->size);
                    while (column->cards[selected.card_idx].hidden) {
                        selected.card_idx = MOD(selected.card_idx + 1, column->size);
                    }
                    column->cards[selected.card_idx].selected = true;
                }
            } break;
            case 'w': {  /* Traverse within Pile (only for columns) */
                if (selected.pile_idx >= GetIndexOfPile(piles, pile_count, &columns[0])) {
                    Pile *column = piles[selected.pile_idx];
                    column->cards[selected.card_idx].selected = false;
                    selected.card_idx = MOD(selected.card_idx - 1, column->size);
                    while (column->cards[selected.card_idx].hidden) {
                        selected.card_idx = MOD(selected.card_idx - 1, column->size);
                    }
                    column->cards[selected.card_idx].selected = true;
                }
            } break;
            case 'd': {  /* Traverse Piles Forward */
                Pile *selected_pile                               = piles[selected.pile_idx];
                selected_pile->cards[selected.card_idx].selected = false;
                selected.pile_idx = MOD(selected.pile_idx + 1, pile_count);
                selected_pile      = piles[selected.pile_idx];
                if (selected_pile == &poll && poll.size == 0)  {
                    selected.pile_idx = MOD(selected.pile_idx + 1, pile_count);
                    selected_pile      = piles[selected.pile_idx];
                }
                selected.card_idx                                = selected_pile->size - 1;
                if (selected.card_idx >= 0) {
                    selected_pile->cards[selected.card_idx].selected = true;
                }
            } break;
            case 'a': {  /* Traverse Piles Backward */
                Pile *selected_pile                               = piles[selected.pile_idx];
                selected_pile->cards[selected.card_idx].selected = false;
                selected.pile_idx = MOD(selected.pile_idx - 1, pile_count);
                selected_pile     = piles[selected.pile_idx];
                if (selected_pile == &poll && poll.size == 0)  {
                    selected.pile_idx = MOD(selected.pile_idx - 1, pile_count);
                    selected_pile      = piles[selected.pile_idx];
                }
                selected.card_idx                                = selected_pile->size - 1;
                if (selected.card_idx >= 0) {
                    selected_pile->cards[selected.card_idx].selected = true;
                }
            } break;
            case 'e': {  /* Collect or Draw Cards */
                if (dragged.pile_idx != -1 || dragged.card_idx != -1) {
                    piles[dragged.pile_idx]->cards[dragged.card_idx].dragged = false;
                    dragged.pile_idx = -1;
                    dragged.card_idx = -1;
                }
                if (piles[selected.pile_idx] == &poll) {  /* Collect from Poll */
                    int target_suite = SUITE_OF(LAST_CARD_OF(poll));
                    if (!(
                        (foundations[target_suite].size == 0 && RANK_OF(LAST_CARD_OF(poll)) == 0) ||
                        (foundations[target_suite].size > 0  && RANK_OF(LAST_CARD_OF(poll)) == RANK_OF(LAST_CARD_OF(foundations[target_suite])) + 1)
                    )) {
                        strcpy(status, "Ranks not matching!");
                        continue;
                    }
                    piles[selected.pile_idx]->cards[selected.card_idx].selected = false;
                    foundations[target_suite].size++;
                    LAST_CARD_OF(foundations[target_suite]) = LAST_CARD_OF(poll);
                    poll.size--;
                    selected.card_idx = piles[selected.pile_idx]->size - 1;
                    if (selected.card_idx >= 0) {
                        piles[selected.pile_idx]->cards[selected.card_idx].selected = true;
                    }
                    turn_count++;
                } else if (selected.pile_idx >= GetIndexOfPile(piles, pile_count, &columns[0]) &&
                            selected.pile_idx <= GetIndexOfPile(piles, pile_count, &columns[6])) {  /* Collect from Columns */
                    int source_col = selected.pile_idx - GetIndexOfPile(piles, pile_count, &columns[0]);
                    if (columns[source_col].size == 0) {
                        strcpy(status, "That column is empty!");
                        continue;
                    }
                    int target_suite = SUITE_OF(LAST_CARD_OF(columns[source_col]));
                    if (!(
                        (foundations[target_suite].size == 0 && RANK_OF(LAST_CARD_OF(columns[source_col])) == 0) ||
                        (foundations[target_suite].size > 0  && RANK_OF(LAST_CARD_OF(columns[source_col])) == RANK_OF(LAST_CARD_OF(foundations[target_suite])) + 1)
                    )) {
                        strcpy(status, "Ranks not matching!");
                        continue;
                    }
                    piles[selected.pile_idx]->cards[selected.card_idx].selected = false;
                    foundations[target_suite].size++;
                    LAST_CARD_OF(foundations[target_suite]) = LAST_CARD_OF(columns[source_col]);
                    columns[source_col].size--;
                    LAST_CARD_OF(columns[source_col]).hidden = false;
                    selected.card_idx = piles[selected.pile_idx]->size - 1;
                    if (selected.card_idx >= 0) {
                        piles[selected.pile_idx]->cards[selected.card_idx].selected = true;
                    }
                    turn_count++;
                }
            } break;
            case ' ': {  /* Move Cards */
                if (piles[selected.pile_idx] == &deck) {  /* Draw Cards */
                    if (dragged.pile_idx != -1 || dragged.card_idx != -1) {
                        piles[dragged.pile_idx]->cards[dragged.card_idx].dragged = false;
                        dragged.pile_idx = -1;
                        dragged.card_idx = -1;
                    }
                    deck.cards[selected.card_idx].selected = false;
                    int buyout_size = deck.size > 3 ? 3 : deck.size;
                    if (buyout_size == 0) {
                        while(poll.size > 0) {
                            deck.size++;
                            LAST_CARD_OF(deck)        = LAST_CARD_OF(poll);
                            LAST_CARD_OF(deck).hidden = true;
                            poll.size--;
                        }
                    } else {
                        for (int i=0; i<buyout_size; ++i) {
                            poll.size++;
                            LAST_CARD_OF(poll)        = LAST_CARD_OF(deck);
                            LAST_CARD_OF(poll).hidden = false;
                            deck.size--;
                        }
                        turn_count++;
                    }
                    selected.card_idx = deck.size - 1;
                    deck.cards[selected.card_idx].selected = true;
                } else if (dragged.pile_idx == -1 && dragged.card_idx == -1 && piles[selected.pile_idx]->size > 0) {
                    dragged.pile_idx = selected.pile_idx;
                    dragged.card_idx = selected.card_idx;
                    piles[dragged.pile_idx]->cards[dragged.card_idx].dragged = true;
                } else if (dragged.pile_idx == selected.pile_idx && dragged.card_idx == selected.card_idx) {
                    if (piles[dragged.pile_idx]->size > 0) {
                        piles[dragged.pile_idx]->cards[dragged.card_idx].dragged = false;
                    }
                    dragged.pile_idx = -1;
                    dragged.card_idx = -1;
                } else if (piles[dragged.pile_idx] == &poll) {  /* Move Poll to Col */
                    int target_col  = selected.pile_idx - GetIndexOfPile(piles, pile_count, &columns[0]);
                    if (selected.card_idx != columns[target_col].size - 1) {
                        strcpy(status, "You can only move cards from poll to the end of column piles!");
                        continue;
                    }
                    if (!(
                        (columns[target_col].size == 0 && RANK_OF(LAST_CARD_OF(poll)) == 12) ||
                        (columns[target_col].size > 0  && 
                            (RANK_OF(LAST_CARD_OF(poll)) + 1 == RANK_OF(LAST_CARD_OF(columns[target_col]))) &&
                            (SUITE_OF(LAST_CARD_OF(poll)) / 2 != SUITE_OF(LAST_CARD_OF(columns[target_col])) / 2)
                    ))) {
                        strcpy(status, "Ranks or Suites not matching!");
                        continue;
                    }
                    piles[dragged.pile_idx]->cards[dragged.card_idx].dragged = false;
                    dragged.pile_idx = -1;
                    dragged.card_idx = -1;
                    columns[target_col].size++;
                    LAST_CARD_OF(columns[target_col]) = LAST_CARD_OF(poll);
                    poll.size--;
                    piles[selected.pile_idx]->cards[selected.card_idx].selected = false;
                    selected.card_idx = columns[target_col].size - 1;
                    piles[selected.pile_idx]->cards[selected.card_idx].selected = true;
                    turn_count++;
                } else if (dragged.pile_idx >= GetIndexOfPile(piles, pile_count, &foundations[0]) &&   /* Move Foundation to Col */
                            dragged.pile_idx <= GetIndexOfPile(piles, pile_count, &foundations[3])) {
                    int source_suite  = dragged.pile_idx - GetIndexOfPile(piles, pile_count, &foundations[0]);
                    int target_col  = selected.pile_idx - GetIndexOfPile(piles, pile_count, &columns[0]);
                    if (foundations[source_suite].size == 0) {
                        strcpy(status, "That foundation pile is empty!");
                        continue;
                    }
                    if (selected.card_idx != columns[target_col].size - 1) {
                        strcpy(status, "You can only move cards from foundation piles to the end of column piles!");
                        continue;
                    }
                    if (!(
                        (columns[target_col].size == 0 && RANK_OF(LAST_CARD_OF(foundations[source_suite])) == 12) ||
                        (columns[target_col].size > 0  && 
                            (RANK_OF(LAST_CARD_OF(foundations[source_suite])) + 1 == RANK_OF(LAST_CARD_OF(columns[target_col]))) &&
                            (SUITE_OF(LAST_CARD_OF(foundations[source_suite])) / 2 != SUITE_OF(LAST_CARD_OF(columns[target_col])) / 2)
                    ))) {
                        strcpy(status, "Ranks or Suites not matching!");
                        continue;
                    }
                    piles[dragged.pile_idx]->cards[dragged.card_idx].dragged = false;
                    dragged.pile_idx = -1;
                    dragged.card_idx = -1;
                    columns[target_col].size++;
                    LAST_CARD_OF(columns[target_col]) = LAST_CARD_OF(foundations[source_suite]);
                    foundations[source_suite].size--;
                    piles[selected.pile_idx]->cards[selected.card_idx].selected = false;
                    selected.card_idx = columns[target_col].size - 1;
                    piles[selected.pile_idx]->cards[selected.card_idx].selected = true;
                    turn_count++;
                } else if (dragged.pile_idx >= GetIndexOfPile(piles, pile_count, &columns[0]) &&   /* Move Col to Col */
                            dragged.pile_idx <= GetIndexOfPile(piles, pile_count, &columns[6])) {
                    int card_idx = dragged.card_idx;
                    int source_col = dragged.pile_idx - GetIndexOfPile(piles, pile_count, &columns[0]);
                    int target_col = selected.pile_idx - GetIndexOfPile(piles, pile_count, &columns[0]);
                    if (selected.card_idx != columns[target_col].size - 1) {
                        strcpy(status, "You can only move cards from cards to the end of column piles!");
                        continue;
                    }
                    if (!(
                        (columns[target_col].size == 0 && RANK_OF(columns[source_col].cards[card_idx]) == 12) ||
                        (columns[target_col].size > 0  && 
                            (RANK_OF(columns[source_col].cards[card_idx]) + 1 == RANK_OF(LAST_CARD_OF(columns[target_col]))) &&
                            (SUITE_OF(columns[source_col].cards[card_idx]) / 2 != SUITE_OF(LAST_CARD_OF(columns[target_col])) / 2)
                    ))) {
                        strcpy(status, "Ranks or Suites not matching!");
                        continue;
                    }
                    piles[dragged.pile_idx]->cards[dragged.card_idx].dragged = false;
                    dragged.pile_idx = -1;
                    dragged.card_idx = -1;
                    piles[selected.pile_idx]->cards[selected.card_idx].selected = false;
                    for (int i=0; i<columns[source_col].size - card_idx; i++) {
                        columns[target_col].size++;
                        LAST_CARD_OF(columns[target_col]) = columns[source_col].cards[card_idx + i];
                    }
                    columns[source_col].size = card_idx;
                    LAST_CARD_OF(columns[source_col]).hidden = false;
                    selected.card_idx = columns[target_col].size - 1;
                    piles[selected.pile_idx]->cards[selected.card_idx].selected = true;
                    turn_count++;
                }
            } break;
        }
    }

    printf("\x1B[%dB", BOARD_HEIGHT + 2);
    free(buffer);
	return 0;
}
