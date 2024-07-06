#define VERSION "1.0"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

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

const char suite_symbols[] = { 'H', 'D', 'S', 'C' };
const char rank_symbols[]  = { 'A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K' }; 

typedef struct Card {
    int  number;
    bool hidden;
} Card;

typedef struct Pile {
    Card   cards[DECK_SIZE];
    size_t size;
} Pile;

void print_buffer(char *buffer)
{
    for (size_t row=0; row<BOARD_HEIGHT; ++row) {
        for (size_t col=0; col<BOARD_WIDTH; ++col) {
            printf("%c", buffer[BOARD_POS(col, row)]);
        }
        printf("\n");
    }
}

void draw_rect(char *buffer, int x, int y, size_t w, size_t h)
{
    if (x < 0 || y < 0 || x+w > BOARD_WIDTH || y+h > BOARD_HEIGHT) {
        fprintf(stderr, "%s:%d: Cannot draw rectangle due to out of bounds", __FILE__, __LINE__);
        return;
    }
    for (size_t i=0; i<w; ++i) {
        buffer[BOARD_POS(x+i, y  )]   = '-';
        buffer[BOARD_POS(x+i, y+h-1)] = '-';
    }
    for (size_t i=0; i<h; ++i) {
        buffer[BOARD_POS(x    , y+i)] = '|';
        buffer[BOARD_POS(x+w-1, y+i)] = '|';
    }
    buffer[BOARD_POS(x    , y    )] = '+';
    buffer[BOARD_POS(x    , y+h-1)] = '+';
    buffer[BOARD_POS(x+w-1, y    )] = '+';
    buffer[BOARD_POS(x+w-1, y+h-1)] = '+';
}

void fill_rect(char *buffer, int x, int y, size_t w, size_t h, char value)
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

void draw_card(char *buffer, int x, int y, int card_number, bool hidden)
{
    if (card_number > 52 || card_number < 0) {
        fprintf(stderr, "%s:%d: Card number should between 0 and 51 (inclusive), but passed %d", __FILE__, __LINE__, card_number);
        return;
    }
    int suite = card_number / 13;
    int rank  = card_number % 13;
    
    draw_rect(buffer, x, y, CARD_WIDTH, CARD_HEIGHT);

    if (!hidden) {
        fill_rect(buffer, x, y, CARD_WIDTH, CARD_HEIGHT, '.');
        buffer[BOARD_POS(x+1,            y+1)]             = rank_symbols[rank];
        buffer[BOARD_POS(x+CARD_WIDTH-2, y+1)]             = suite_symbols[suite];
        buffer[BOARD_POS(x+CARD_WIDTH-2, y+CARD_HEIGHT-2)] = rank_symbols[rank];
        buffer[BOARD_POS(x+1,            y+CARD_HEIGHT-2)] = suite_symbols[suite];
    } else {
        fill_rect(buffer, x, y, CARD_WIDTH, CARD_HEIGHT, '#');
    }
}

void render_board(char *buffer)
{
    for (int i=0; i<4; ++i) {
        draw_rect(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * i, 0, CARD_WIDTH, CARD_HEIGHT);
    }
    for (int i=0; i<7; ++i) {
        draw_rect(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * i, CARD_HEIGHT + GAP_VERTICAL, CARD_WIDTH, CARD_HEIGHT);
    }
    draw_rect(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 6, 0, CARD_WIDTH, CARD_HEIGHT);
}

void render_piles(char *buffer, Pile deck, Pile poll, Pile columns[], Pile foundations[])
{
    /* Draw Deck Pile */
    if (deck.size >= 1) {
        draw_card(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 6, 0, LAST_CARD_OF(deck).number, LAST_CARD_OF(deck).hidden);
    }
    /* Draw Poll Pile */
    if (poll.size == 1) {
        draw_card(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 0, 0, LAST_CARD_OF(poll).number, false);
    } else if (poll.size == 2) {
        draw_card(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 0, 0, LAST_NTH_CARD_OF(poll, 2).number, false);
        draw_card(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 1, 0, LAST_CARD_OF(poll).number, false);
    } else if (poll.size >= 3) {
        draw_card(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 0, 0, LAST_NTH_CARD_OF(poll, 3).number, false);
        draw_card(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 1, 0, LAST_NTH_CARD_OF(poll, 2).number, false);
        draw_card(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * 5 - OFFSET_HORIZONTAL * 2, 0, LAST_CARD_OF(poll).number, false);
    }
    /* Draw Column Piles */
    for (size_t i=0; i<7; ++i) {
        Pile column = columns[i];
        for (size_t j=0; j<column.size; ++j) {
            Card card = column.cards[j];
            draw_card(buffer,
                (CARD_WIDTH  + GAP_HORIZONTAL) * i,
                (OFFSET_VERTICAL * j) + (CARD_HEIGHT + GAP_VERTICAL),
                card.number, card.hidden); 
        }
    }
    /* Draw Foundation Piles */
    for (size_t i=0; i<4; ++i) {
        Pile foundation = foundations[i];
        if (foundation.size >= 1) {
            draw_card(buffer, (CARD_WIDTH + GAP_HORIZONTAL) * i, 0, LAST_CARD_OF(foundation).number, false);
        }
    }
}

bool check_game_finished(Pile foundations[])
{
    int filled = 0;
    for (size_t i=0; i<4; ++i) {
        if (foundations[i].size == 13) {
            filled++;
        }
    }
    return filled == 4;
}

bool find_card(Pile piles[], char target_rank, char target_suite, int *pile_index, int *card_index)
{
    for (int i=0; i<7; ++i) {
        for (int j=0; j<piles[i].size; ++j) {
            int suite = SUITE_OF(piles[i].cards[j]);
            int rank  = RANK_OF(piles[i].cards[j]);
            if (piles[i].cards[j].hidden == false &&
                rank_symbols[rank] == target_rank && suite_symbols[suite] == target_suite
            ) {
                *card_index = j;
                *pile_index = i;
                return true;
            }
        }
    }
    return false;
}

int main()
{
    char *buffer = (char*) malloc(BOARD_SIZE * sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "%s:%d: Couldn't allocate buffer memory", __FILE__, __LINE__);
        return 1;
    }

    /* size_t seed = 1720019880; */
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
    char cmd[256]      = {0};
    char prev_cmd[256] = {0};
    char status[256]   = {0};
    bool gameover      = false;
    while(!gameover) {
        /* Print Game State */
        memset(buffer, ' ', BOARD_SIZE);
        render_board(buffer);
        render_piles(buffer, deck, poll, columns, foundations);
        print_buffer(buffer);

        /* Check Game Over */
        if (check_game_finished(foundations) == true) {
            printf("Congratulations! You solved it in %d turns.", turn_count);
            gameover = true;
            break;
        }

        /* Get User Input */
        printf("[Turn #%d] %s> ", turn_count, status);
        fgets(cmd, sizeof(cmd), stdin);
        status[0] = '\0';
        if (strcmp(cmd, "\n") == 0) {
            strcpy(cmd, prev_cmd);
        } else {
            cmd[strlen(cmd)-1] = '\0';
            strcpy(prev_cmd, cmd);
        }

        /* Parse Command */
        if (strcmp(cmd, "quit") == 0) {
            gameover = true;
            break;
        } else if (strcmp(cmd, "buy") == 0) {
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
        } else if (strncmp(cmd, "move poll to col", 11) == 0) {
            int target_col;
            sscanf(cmd, "move poll to col %d", &target_col);
            target_col--;
            if (target_col < 0 || target_col > 7) {
                strcpy(status, "Invalid column number!");
                continue;
            }
            if (poll.size == 0) {
                strcpy(status, "Poll is empty!");
                continue;
            }
            if (!(
                (columns[target_col].size == 0 && RANK_OF(LAST_CARD_OF(poll)) == 13) ||
                (columns[target_col].size > 0  && 
                    (RANK_OF(LAST_CARD_OF(poll)) + 1 == RANK_OF(LAST_CARD_OF(columns[target_col]))) &&
                    (SUITE_OF(LAST_CARD_OF(poll)) / 2 != SUITE_OF(LAST_CARD_OF(columns[target_col])) / 2)
            ))) {
                strcpy(status, "Ranks or Suites not matching!");
                continue;
            }
            columns[target_col].size++;
            LAST_CARD_OF(columns[target_col]) = LAST_CARD_OF(poll);
            poll.size--;
            turn_count++;
        } else if (strncmp(cmd, "collect col", 11) == 0) {
            int source_col;
            sscanf(cmd, "collect col %d", &source_col);
            source_col--;
            if (source_col < 0 || source_col > 7) {
                strcpy(status, "Invalid column number!");
                continue;
            }
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
            foundations[target_suite].size++;
            LAST_CARD_OF(foundations[target_suite]) = LAST_CARD_OF(columns[source_col]);
            columns[source_col].size--;
            LAST_CARD_OF(columns[source_col]).hidden = false;
            turn_count++;
        } else if (strcmp(cmd, "collect poll") == 0) {
            if (poll.size == 0) {
                strcpy(status, "Poll is empty!");
                continue;
            }
            int target_suite = SUITE_OF(LAST_CARD_OF(poll));
            if (!(
                (foundations[target_suite].size == 0 && RANK_OF(LAST_CARD_OF(poll)) == 0) ||
                (foundations[target_suite].size > 0  && RANK_OF(LAST_CARD_OF(poll)) == RANK_OF(LAST_CARD_OF(foundations[target_suite])) + 1)
            )) {
                strcpy(status, "Ranks not matching!");
                continue;
            }
            foundations[target_suite].size++;
            LAST_CARD_OF(foundations[target_suite]) = LAST_CARD_OF(poll);
            poll.size--;
            turn_count++;
        } else if (strncmp(cmd, "move fnd", 8) == 0) {
            int source_suite, target_col; 
            sscanf(cmd, "move fnd %d to col %d", &source_suite, &target_col);
            source_suite--; target_col--;
            if (target_col < 0 || target_col > 7) {
                strcpy(status, "Invalid column number!");
                continue;
            }
            if (source_suite < 0 || source_suite > 4) {
                strcpy(status, "Invalid foundation number!");
                continue;
            }
            if (foundations[source_suite].size == 0) {
                strcpy(status, "That foundation is empty!");
                continue;
            }
            if (!(
                (columns[target_col].size == 0 && RANK_OF(LAST_CARD_OF(foundations[source_suite])) == 13) ||
                (columns[target_col].size > 0  && 
                    (RANK_OF(LAST_CARD_OF(foundations[source_suite])) + 1 == RANK_OF(LAST_CARD_OF(columns[target_col]))) &&
                    (SUITE_OF(LAST_CARD_OF(foundations[source_suite])) / 2 != SUITE_OF(LAST_CARD_OF(columns[target_col])) / 2)
            ))) {
                strcpy(status, "Ranks or Suites not matching!");
                continue;
            }
            columns[target_col].size++;
            LAST_CARD_OF(columns[target_col]) = LAST_CARD_OF(foundations[source_suite]);
            foundations[source_suite].size--;
            turn_count++;
        } else if (strncmp(cmd, "move seq", 8) == 0) {
            char target_rank, target_suite;
            int target_col, card_index, source_col;
            sscanf(cmd, "move seq %c%c to col %d", &target_rank, &target_suite, &target_col);
            target_col--;
            target_suite = target_suite >= 'a' ? target_suite - ' ' : target_suite;
            if (target_col < 0 || target_col > 7) {
                strcpy(status, "Invalid column number!");
                continue;
            }
            bool card_found = find_card(columns, target_rank, target_suite, &source_col, &card_index);
            if (!card_found) {
                strcpy(status, "Card not found!");
                continue;
            }
            if (!(
                (columns[target_col].size == 0 && RANK_OF(columns[source_col].cards[card_index]) == 13) ||
                (columns[target_col].size > 0  && 
                    (RANK_OF(columns[source_col].cards[card_index]) + 1 == RANK_OF(LAST_CARD_OF(columns[target_col]))) &&
                    (SUITE_OF(columns[source_col].cards[card_index]) / 2 != SUITE_OF(LAST_CARD_OF(columns[target_col])) / 2)
            ))) {
                strcpy(status, "Ranks or Suites not matching!");
                continue;
            }
            for (int i=0; i<columns[source_col].size - card_index; i++) {
                columns[target_col].size++;
                LAST_CARD_OF(columns[target_col]) = columns[source_col].cards[card_index + i];
                columns[source_col].size--;
            }
            LAST_CARD_OF(columns[source_col]).hidden = false;
            turn_count++;
        }
    }

    free(buffer);
	return 0;
}
