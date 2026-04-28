#ifndef POKEMON_BATTLE_CORE_H
#define POKEMON_BATTLE_CORE_H

#include <stddef.h>
#include <stdint.h>

#define POKEMON_TYPE_COUNT 18
#define POKEMON_NO_EFFECT 0
#define POKEMON_NOT_VERY_EFFECTIVE 50
#define POKEMON_NORMAL_EFFECTIVE 100
#define POKEMON_SUPER_EFFECTIVE 200

typedef enum {
    PTYPE_NORMAL = 0,
    PTYPE_FIRE,
    PTYPE_WATER,
    PTYPE_ELECTRIC,
    PTYPE_GRASS,
    PTYPE_ICE,
    PTYPE_FIGHTING,
    PTYPE_POISON,
    PTYPE_GROUND,
    PTYPE_FLYING,
    PTYPE_PSYCHIC,
    PTYPE_BUG,
    PTYPE_ROCK,
    PTYPE_GHOST,
    PTYPE_DRAGON,
    PTYPE_DARK,
    PTYPE_STEEL,
    PTYPE_FAIRY
} PokemonType;

typedef struct {
    size_t rows;
    size_t cols;
    uint8_t *cells;
} PokemonGrid;

const char *pokemon_type_name(uint8_t type);
int pokemon_effectiveness_percent(uint8_t attacker, uint8_t defender);

PokemonGrid pokemon_grid_create(size_t rows, size_t cols);
void pokemon_grid_free(PokemonGrid *grid);
uint8_t pokemon_grid_get(const PokemonGrid *grid, size_t row, size_t col);
void pokemon_grid_set(PokemonGrid *grid, size_t row, size_t col, uint8_t type);
void pokemon_grid_fill(PokemonGrid *grid, uint8_t type);
void pokemon_grid_seed_pattern(PokemonGrid *grid);
void pokemon_grid_seed_random(PokemonGrid *grid, unsigned int seed);
void pokemon_grid_print(const PokemonGrid *grid);

uint8_t pokemon_next_cell(const PokemonGrid *grid, size_t row, size_t col);
uint8_t pokemon_next_cell_bounded(const PokemonGrid *grid,
                                  size_t row,
                                  size_t col,
                                  size_t min_row,
                                  size_t max_row);
void pokemon_step(const PokemonGrid *current, PokemonGrid *next);
void pokemon_run(PokemonGrid *current, PokemonGrid *scratch, int steps, int print_each_step);

#endif
