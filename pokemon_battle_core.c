#include "pokemon_battle_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TYPE_NAMES[POKEMON_TYPE_COUNT] = {
    "Normal", "Fire", "Water", "Electric", "Grass", "Ice",
    "Fighting", "Poison", "Ground", "Flying", "Psychic", "Bug",
    "Rock", "Ghost", "Dragon", "Dark", "Steel", "Fairy"
};

static const char TYPE_SYMBOLS[POKEMON_TYPE_COUNT] = {
    'N', 'F', 'W', 'E', 'G', 'I', 'H', 'P', 'D',
    'L', 'Y', 'B', 'R', 'O', 'A', 'K', 'S', 'Q'
};

/*
 * Rows are attacking types, columns are defending types.
 * Values use integer percentages: 0, 50, 100, 200.
 */
static const int EFFECTIVENESS[POKEMON_TYPE_COUNT][POKEMON_TYPE_COUNT] = {
    {100,100,100,100,100,100,100,100,100,100,100,100, 50,  0,100,100, 50,100},
    {100, 50, 50,100,200,200,100,100,100,100,100,200, 50,100, 50,100,200,100},
    {100,200, 50,100, 50,100,100,100,200,100,100,100,200,100, 50,100,100,100},
    {100,100,200, 50, 50,100,100,100,  0,200,100,100,100,100, 50,100,100,100},
    {100, 50,200,100, 50,100,100, 50,200, 50,100, 50,200,100, 50,100, 50,100},
    {100, 50, 50,100,200, 50,100,100,200,200,100,100,100,100,200,100, 50,100},
    {200,100,100,100,100,200,100, 50,100, 50, 50, 50,200,  0,100,200,200, 50},
    {100,100,100,100,200,100,100, 50, 50,100,100,100, 50, 50,100,100,  0,200},
    {100,200,100,200, 50,100,100,200,100,  0,100, 50,200,100,100,100,200,100},
    {100,100,100, 50,200,100,200,100,100,100,100,200, 50,100,100,100, 50,100},
    {100,100,100,100,100,100,200,200,100,100, 50,100,100,100,100,  0, 50,100},
    {100, 50,100,100,200,100, 50, 50,100, 50,200,100,100, 50,100,200, 50, 50},
    {100,200,100,100,100,200, 50,100, 50,200,100,200,100,100,100,100, 50,100},
    {  0,100,100,100,100,100,100,100,100,100,200,100,100,200,100, 50,100,100},
    {100,100,100,100,100,100,100,100,100,100,100,100,100,100,200,100, 50,  0},
    {100,100,100,100,100,100, 50,100,100,100,200,100,100,200,100, 50,100, 50},
    {100, 50, 50, 50,100,200,100,100,100,100,100,100,200,100,100,100, 50,200},
    {100, 50,100,100,100,100,200, 50,100,100,100,100,100,100,200,200, 50,100}
};

static void *checked_calloc(size_t count, size_t size)
{
    void *ptr = calloc(count, size);
    if (ptr == NULL) {
        fprintf(stderr, "Allocation failed for %zu objects of %zu bytes\n", count, size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static int valid_type(uint8_t type)
{
    return type < POKEMON_TYPE_COUNT;
}

const char *pokemon_type_name(uint8_t type)
{
    return valid_type(type) ? TYPE_NAMES[type] : "Unknown";
}

int pokemon_effectiveness_percent(uint8_t attacker, uint8_t defender)
{
    if (!valid_type(attacker) || !valid_type(defender)) {
        return 100;
    }
    return EFFECTIVENESS[attacker][defender];
}

PokemonGrid pokemon_grid_create(size_t rows, size_t cols)
{
    PokemonGrid grid;
    grid.rows = rows;
    grid.cols = cols;
    grid.cells = (uint8_t *)checked_calloc(rows * cols, sizeof(uint8_t));
    return grid;
}

void pokemon_grid_free(PokemonGrid *grid)
{
    if (grid == NULL) return;
    free(grid->cells);
    grid->cells = NULL;
    grid->rows = 0;
    grid->cols = 0;
}

uint8_t pokemon_grid_get(const PokemonGrid *grid, size_t row, size_t col)
{
    return grid->cells[row * grid->cols + col];
}

void pokemon_grid_set(PokemonGrid *grid, size_t row, size_t col, uint8_t type)
{
    grid->cells[row * grid->cols + col] = valid_type(type) ? type : PTYPE_NORMAL;
}

void pokemon_grid_fill(PokemonGrid *grid, uint8_t type)
{
    memset(grid->cells, valid_type(type) ? type : PTYPE_NORMAL, grid->rows * grid->cols);
}

void pokemon_grid_seed_pattern(PokemonGrid *grid)
{
    for (size_t r = 0; r < grid->rows; r++) {
        for (size_t c = 0; c < grid->cols; c++) {
            uint8_t type = (uint8_t)(((r / 2) * 5 + (c / 3) * 7 + r + c) % POKEMON_TYPE_COUNT);
            pokemon_grid_set(grid, r, c, type);
        }
    }

    if (grid->rows >= 3 && grid->cols >= 3) {
        size_t r = grid->rows / 2;
        size_t c = grid->cols / 2;
        pokemon_grid_set(grid, r, c, PTYPE_NORMAL);
        pokemon_grid_set(grid, r - 1, c, PTYPE_FIRE);
        pokemon_grid_set(grid, r, c - 1, PTYPE_WATER);
        pokemon_grid_set(grid, r, c + 1, PTYPE_GRASS);
        pokemon_grid_set(grid, r + 1, c, PTYPE_ELECTRIC);
    }
}

void pokemon_grid_seed_random(PokemonGrid *grid, unsigned int seed)
{
    srand(seed);
    for (size_t i = 0; i < grid->rows * grid->cols; i++) {
        grid->cells[i] = (uint8_t)(rand() % POKEMON_TYPE_COUNT);
    }
}

void pokemon_grid_print(const PokemonGrid *grid)
{
    for (size_t r = 0; r < grid->rows; r++) {
        for (size_t c = 0; c < grid->cols; c++) {
            uint8_t type = pokemon_grid_get(grid, r, c);
            putchar(TYPE_SYMBOLS[type]);
            putchar(c + 1 == grid->cols ? '\n' : ' ');
        }
    }
}

static int count_neighbor_type_bounded(const PokemonGrid *grid,
                                       size_t row,
                                       size_t col,
                                       uint8_t type,
                                       size_t min_row,
                                       size_t max_row)
{
    int count = 0;

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;

            long rr = (long)row + dr;
            long cc = (long)col + dc;
            if (rr < (long)min_row || cc < 0 || rr >= (long)max_row || cc >= (long)grid->cols) {
                continue;
            }

            if (pokemon_grid_get(grid, (size_t)rr, (size_t)cc) == type) {
                count++;
            }
        }
    }

    return count;
}

static int has_cardinal_type_bounded(const PokemonGrid *grid,
                                     size_t row,
                                     size_t col,
                                     uint8_t type,
                                     size_t min_row,
                                     size_t max_row)
{
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    for (int i = 0; i < 4; i++) {
        long rr = (long)row + dr[i];
        long cc = (long)col + dc[i];
        if (rr < (long)min_row || cc < 0 || rr >= (long)max_row || cc >= (long)grid->cols) {
            continue;
        }
        if (pokemon_grid_get(grid, (size_t)rr, (size_t)cc) == type) {
            return 1;
        }
    }

    return 0;
}

static uint8_t rare_evolution_event_bounded(const PokemonGrid *grid,
                                            size_t row,
                                            size_t col,
                                            size_t min_row,
                                            size_t max_row)
{
    uint8_t center = pokemon_grid_get(grid, row, col);

    if (center == PTYPE_NORMAL &&
        has_cardinal_type_bounded(grid, row, col, PTYPE_FIRE, min_row, max_row) &&
        has_cardinal_type_bounded(grid, row, col, PTYPE_WATER, min_row, max_row) &&
        has_cardinal_type_bounded(grid, row, col, PTYPE_GRASS, min_row, max_row) &&
        count_neighbor_type_bounded(grid, row, col, PTYPE_ELECTRIC, min_row, max_row) > 0) {
        return PTYPE_DRAGON;
    }

    if (center == PTYPE_ICE &&
        count_neighbor_type_bounded(grid, row, col, PTYPE_WATER, min_row, max_row) >= 3 &&
        count_neighbor_type_bounded(grid, row, col, PTYPE_ELECTRIC, min_row, max_row) >= 2) {
        return PTYPE_STEEL;
    }

    return center;
}

uint8_t pokemon_next_cell_bounded(const PokemonGrid *grid,
                                  size_t row,
                                  size_t col,
                                  size_t min_row,
                                  size_t max_row)
{
    uint8_t center = pokemon_grid_get(grid, row, col);
    uint8_t evolved = rare_evolution_event_bounded(grid, row, col, min_row, max_row);

    if (evolved != center) {
        return evolved;
    }

    int pressure[POKEMON_TYPE_COUNT] = {0};
    int same_support = 0;

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;

            long rr = (long)row + dr;
            long cc = (long)col + dc;
            if (rr < (long)min_row || cc < 0 || rr >= (long)max_row || cc >= (long)grid->cols) {
                continue;
            }

            uint8_t neighbor = pokemon_grid_get(grid, (size_t)rr, (size_t)cc);
            if (neighbor == center) {
                same_support++;
                pressure[center] += 120;
            } else {
                pressure[neighbor] += pokemon_effectiveness_percent(neighbor, center);
            }
        }
    }

    uint8_t best_type = center;
    int best_pressure = pressure[center];
    for (uint8_t type = 0; type < POKEMON_TYPE_COUNT; type++) {
        if (pressure[type] > best_pressure) {
            best_pressure = pressure[type];
            best_type = type;
        }
    }

    if (same_support >= 3 && best_pressure < pressure[center] + 220) {
        return center;
    }

    if (best_type != center && best_pressure >= pressure[center] + 150) {
        return best_type;
    }

    return center;
}

uint8_t pokemon_next_cell(const PokemonGrid *grid, size_t row, size_t col)
{
    return pokemon_next_cell_bounded(grid, row, col, 0, grid->rows);
}

void pokemon_step(const PokemonGrid *current, PokemonGrid *next)
{
    if (current->rows != next->rows || current->cols != next->cols) {
        fprintf(stderr, "pokemon_step requires matching grid dimensions\n");
        exit(EXIT_FAILURE);
    }

    for (size_t r = 0; r < current->rows; r++) {
        for (size_t c = 0; c < current->cols; c++) {
            pokemon_grid_set(next, r, c, pokemon_next_cell(current, r, c));
        }
    }
}

void pokemon_run(PokemonGrid *current, PokemonGrid *scratch, int steps, int print_each_step)
{
    for (int step = 0; step < steps; step++) {
        pokemon_step(current, scratch);

        uint8_t *tmp = current->cells;
        current->cells = scratch->cells;
        scratch->cells = tmp;

        if (print_each_step) {
            printf("\nStep %d\n", step + 1);
            pokemon_grid_print(current);
        }
    }
}
