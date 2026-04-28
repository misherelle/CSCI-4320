#include "pokemon_battle_core.h"

#include <stdio.h>
#include <stdlib.h>

static int failures = 0;

static void check_int(const char *label, int got, int expected)
{
    if (got != expected) {
        printf("FAIL: %s got=%d expected=%d\n", label, got, expected);
        failures++;
    } else {
        printf("PASS: %s\n", label);
    }
}

static void set_matchups(int expected[POKEMON_TYPE_COUNT][POKEMON_TYPE_COUNT],
                         uint8_t attacker,
                         const uint8_t *defenders,
                         size_t ndefenders,
                         int value)
{
    for (size_t i = 0; i < ndefenders; i++) {
        expected[attacker][defenders[i]] = value;
    }
}

#define SET_MATCHUPS(expected, attacker, defenders, value) \
    set_matchups((expected), (attacker), (defenders), sizeof(defenders) / sizeof((defenders)[0]), (value))

static void test_full_effectiveness_chart(void)
{
    int expected[POKEMON_TYPE_COUNT][POKEMON_TYPE_COUNT];

    for (int attacker = 0; attacker < POKEMON_TYPE_COUNT; attacker++) {
        for (int defender = 0; defender < POKEMON_TYPE_COUNT; defender++) {
            expected[attacker][defender] = POKEMON_NORMAL_EFFECTIVE;
        }
    }

    const uint8_t normal_no[] = {PTYPE_GHOST};
    const uint8_t normal_half[] = {PTYPE_ROCK, PTYPE_STEEL};

    const uint8_t fire_double[] = {PTYPE_GRASS, PTYPE_ICE, PTYPE_BUG, PTYPE_STEEL};
    const uint8_t fire_half[] = {PTYPE_FIRE, PTYPE_WATER, PTYPE_ROCK, PTYPE_DRAGON};

    const uint8_t water_double[] = {PTYPE_FIRE, PTYPE_GROUND, PTYPE_ROCK};
    const uint8_t water_half[] = {PTYPE_WATER, PTYPE_GRASS, PTYPE_DRAGON};

    const uint8_t electric_no[] = {PTYPE_GROUND};
    const uint8_t electric_double[] = {PTYPE_WATER, PTYPE_FLYING};
    const uint8_t electric_half[] = {PTYPE_ELECTRIC, PTYPE_GRASS, PTYPE_DRAGON};

    const uint8_t grass_double[] = {PTYPE_WATER, PTYPE_GROUND, PTYPE_ROCK};
    const uint8_t grass_half[] = {PTYPE_FIRE, PTYPE_GRASS, PTYPE_POISON, PTYPE_FLYING, PTYPE_BUG, PTYPE_DRAGON, PTYPE_STEEL};

    const uint8_t ice_double[] = {PTYPE_GRASS, PTYPE_GROUND, PTYPE_FLYING, PTYPE_DRAGON};
    const uint8_t ice_half[] = {PTYPE_FIRE, PTYPE_WATER, PTYPE_ICE, PTYPE_STEEL};

    const uint8_t fighting_no[] = {PTYPE_GHOST};
    const uint8_t fighting_double[] = {PTYPE_NORMAL, PTYPE_ICE, PTYPE_ROCK, PTYPE_DARK, PTYPE_STEEL};
    const uint8_t fighting_half[] = {PTYPE_POISON, PTYPE_FLYING, PTYPE_PSYCHIC, PTYPE_BUG, PTYPE_FAIRY};

    const uint8_t poison_no[] = {PTYPE_STEEL};
    const uint8_t poison_double[] = {PTYPE_GRASS, PTYPE_FAIRY};
    const uint8_t poison_half[] = {PTYPE_POISON, PTYPE_GROUND, PTYPE_ROCK, PTYPE_GHOST};

    const uint8_t ground_no[] = {PTYPE_FLYING};
    const uint8_t ground_double[] = {PTYPE_FIRE, PTYPE_ELECTRIC, PTYPE_POISON, PTYPE_ROCK, PTYPE_STEEL};
    const uint8_t ground_half[] = {PTYPE_GRASS, PTYPE_BUG};

    const uint8_t flying_double[] = {PTYPE_GRASS, PTYPE_FIGHTING, PTYPE_BUG};
    const uint8_t flying_half[] = {PTYPE_ELECTRIC, PTYPE_ROCK, PTYPE_STEEL};

    const uint8_t psychic_no[] = {PTYPE_DARK};
    const uint8_t psychic_double[] = {PTYPE_FIGHTING, PTYPE_POISON};
    const uint8_t psychic_half[] = {PTYPE_PSYCHIC, PTYPE_STEEL};

    const uint8_t bug_double[] = {PTYPE_GRASS, PTYPE_PSYCHIC, PTYPE_DARK};
    const uint8_t bug_half[] = {PTYPE_FIRE, PTYPE_FIGHTING, PTYPE_POISON, PTYPE_FLYING, PTYPE_GHOST, PTYPE_STEEL, PTYPE_FAIRY};

    const uint8_t rock_double[] = {PTYPE_FIRE, PTYPE_ICE, PTYPE_FLYING, PTYPE_BUG};
    const uint8_t rock_half[] = {PTYPE_FIGHTING, PTYPE_GROUND, PTYPE_STEEL};

    const uint8_t ghost_no[] = {PTYPE_NORMAL};
    const uint8_t ghost_double[] = {PTYPE_PSYCHIC, PTYPE_GHOST};
    const uint8_t ghost_half[] = {PTYPE_DARK};

    const uint8_t dragon_no[] = {PTYPE_FAIRY};
    const uint8_t dragon_double[] = {PTYPE_DRAGON};
    const uint8_t dragon_half[] = {PTYPE_STEEL};

    const uint8_t dark_double[] = {PTYPE_PSYCHIC, PTYPE_GHOST};
    const uint8_t dark_half[] = {PTYPE_FIGHTING, PTYPE_DARK, PTYPE_FAIRY};

    const uint8_t steel_double[] = {PTYPE_ICE, PTYPE_ROCK, PTYPE_FAIRY};
    const uint8_t steel_half[] = {PTYPE_FIRE, PTYPE_WATER, PTYPE_ELECTRIC, PTYPE_STEEL};

    const uint8_t fairy_double[] = {PTYPE_FIGHTING, PTYPE_DRAGON, PTYPE_DARK};
    const uint8_t fairy_half[] = {PTYPE_FIRE, PTYPE_POISON, PTYPE_STEEL};

    SET_MATCHUPS(expected, PTYPE_NORMAL, normal_no, POKEMON_NO_EFFECT);
    SET_MATCHUPS(expected, PTYPE_NORMAL, normal_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_FIRE, fire_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_FIRE, fire_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_WATER, water_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_WATER, water_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_ELECTRIC, electric_no, POKEMON_NO_EFFECT);
    SET_MATCHUPS(expected, PTYPE_ELECTRIC, electric_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_ELECTRIC, electric_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_GRASS, grass_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_GRASS, grass_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_ICE, ice_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_ICE, ice_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_FIGHTING, fighting_no, POKEMON_NO_EFFECT);
    SET_MATCHUPS(expected, PTYPE_FIGHTING, fighting_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_FIGHTING, fighting_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_POISON, poison_no, POKEMON_NO_EFFECT);
    SET_MATCHUPS(expected, PTYPE_POISON, poison_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_POISON, poison_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_GROUND, ground_no, POKEMON_NO_EFFECT);
    SET_MATCHUPS(expected, PTYPE_GROUND, ground_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_GROUND, ground_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_FLYING, flying_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_FLYING, flying_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_PSYCHIC, psychic_no, POKEMON_NO_EFFECT);
    SET_MATCHUPS(expected, PTYPE_PSYCHIC, psychic_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_PSYCHIC, psychic_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_BUG, bug_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_BUG, bug_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_ROCK, rock_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_ROCK, rock_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_GHOST, ghost_no, POKEMON_NO_EFFECT);
    SET_MATCHUPS(expected, PTYPE_GHOST, ghost_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_GHOST, ghost_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_DRAGON, dragon_no, POKEMON_NO_EFFECT);
    SET_MATCHUPS(expected, PTYPE_DRAGON, dragon_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_DRAGON, dragon_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_DARK, dark_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_DARK, dark_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_STEEL, steel_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_STEEL, steel_half, POKEMON_NOT_VERY_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_FAIRY, fairy_double, POKEMON_SUPER_EFFECTIVE);
    SET_MATCHUPS(expected, PTYPE_FAIRY, fairy_half, POKEMON_NOT_VERY_EFFECTIVE);

    int chart_errors = 0;
    for (int attacker = 0; attacker < POKEMON_TYPE_COUNT; attacker++) {
        for (int defender = 0; defender < POKEMON_TYPE_COUNT; defender++) {
            int got = pokemon_effectiveness_percent((uint8_t)attacker, (uint8_t)defender);
            if (got != expected[attacker][defender]) {
                if (chart_errors < 10) {
                    printf("FAIL: %s attacking %s got=%d expected=%d\n",
                           pokemon_type_name((uint8_t)attacker),
                           pokemon_type_name((uint8_t)defender),
                           got,
                           expected[attacker][defender]);
                }
                chart_errors++;
            }
        }
    }

    if (chart_errors == 0) {
        printf("PASS: full 18 x 18 type-effectiveness chart\n");
    } else {
        printf("FAIL: full type-effectiveness chart had %d mismatch(es)\n", chart_errors);
        failures++;
    }
}

static void test_effectiveness_table(void)
{
    check_int("Water beats Fire", pokemon_effectiveness_percent(PTYPE_WATER, PTYPE_FIRE), 200);
    check_int("Electric has no effect on Ground", pokemon_effectiveness_percent(PTYPE_ELECTRIC, PTYPE_GROUND), 0);
    check_int("Normal has no effect on Ghost", pokemon_effectiveness_percent(PTYPE_NORMAL, PTYPE_GHOST), 0);
    check_int("Fairy beats Dragon", pokemon_effectiveness_percent(PTYPE_FAIRY, PTYPE_DRAGON), 200);
    check_int("Fire resists Grass attack", pokemon_effectiveness_percent(PTYPE_GRASS, PTYPE_FIRE), 50);
}

static void test_attack_takeover(void)
{
    PokemonGrid grid = pokemon_grid_create(3, 3);
    pokemon_grid_fill(&grid, PTYPE_FIRE);
    pokemon_grid_set(&grid, 1, 1, PTYPE_FIRE);
    pokemon_grid_set(&grid, 0, 0, PTYPE_WATER);
    pokemon_grid_set(&grid, 0, 1, PTYPE_WATER);
    pokemon_grid_set(&grid, 0, 2, PTYPE_WATER);
    pokemon_grid_set(&grid, 1, 0, PTYPE_WATER);

    check_int("Water pressure captures Fire center",
              pokemon_next_cell(&grid, 1, 1),
              PTYPE_WATER);

    pokemon_grid_free(&grid);
}

static void test_same_type_support(void)
{
    PokemonGrid grid = pokemon_grid_create(3, 3);
    pokemon_grid_fill(&grid, PTYPE_GRASS);
    pokemon_grid_set(&grid, 0, 0, PTYPE_FIRE);
    pokemon_grid_set(&grid, 0, 1, PTYPE_FIRE);

    check_int("Three same-type neighbors stabilize Grass center",
              pokemon_next_cell(&grid, 1, 1),
              PTYPE_GRASS);

    pokemon_grid_free(&grid);
}

static void test_dragon_evolution_pattern(void)
{
    PokemonGrid grid = pokemon_grid_create(3, 3);
    pokemon_grid_fill(&grid, PTYPE_NORMAL);
    pokemon_grid_set(&grid, 0, 1, PTYPE_FIRE);
    pokemon_grid_set(&grid, 1, 0, PTYPE_WATER);
    pokemon_grid_set(&grid, 1, 2, PTYPE_GRASS);
    pokemon_grid_set(&grid, 2, 1, PTYPE_ELECTRIC);

    check_int("Starter-plus-electric pattern evolves Normal to Dragon",
              pokemon_next_cell(&grid, 1, 1),
              PTYPE_DRAGON);

    pokemon_grid_free(&grid);
}

static void test_steel_evolution_pattern(void)
{
    PokemonGrid grid = pokemon_grid_create(3, 3);
    pokemon_grid_fill(&grid, PTYPE_NORMAL);
    pokemon_grid_set(&grid, 1, 1, PTYPE_ICE);
    pokemon_grid_set(&grid, 0, 0, PTYPE_WATER);
    pokemon_grid_set(&grid, 0, 1, PTYPE_WATER);
    pokemon_grid_set(&grid, 0, 2, PTYPE_WATER);
    pokemon_grid_set(&grid, 1, 0, PTYPE_ELECTRIC);
    pokemon_grid_set(&grid, 1, 2, PTYPE_ELECTRIC);

    check_int("Water-electric pattern evolves Ice to Steel",
              pokemon_next_cell(&grid, 1, 1),
              PTYPE_STEEL);

    pokemon_grid_free(&grid);
}

static void test_step_uses_old_grid_only(void)
{
    PokemonGrid current = pokemon_grid_create(3, 3);
    PokemonGrid next = pokemon_grid_create(3, 3);
    pokemon_grid_fill(&current, PTYPE_NORMAL);
    pokemon_grid_set(&current, 1, 1, PTYPE_FIRE);
    pokemon_grid_set(&current, 0, 0, PTYPE_WATER);
    pokemon_grid_set(&current, 0, 1, PTYPE_WATER);
    pokemon_grid_set(&current, 0, 2, PTYPE_WATER);
    pokemon_grid_set(&current, 1, 0, PTYPE_WATER);

    pokemon_step(&current, &next);
    check_int("Step writes captured center",
              pokemon_grid_get(&next, 1, 1),
              PTYPE_WATER);
    check_int("Step does not mutate current grid",
              pokemon_grid_get(&current, 1, 1),
              PTYPE_FIRE);

    pokemon_grid_free(&next);
    pokemon_grid_free(&current);
}

int main(void)
{
    test_full_effectiveness_chart();
    test_effectiveness_table();
    test_attack_takeover();
    test_same_type_support();
    test_dragon_evolution_pattern();
    test_steel_evolution_pattern();
    test_step_uses_old_grid_only();

    if (failures == 0) {
        printf("\nAll Pokemon battle core tests passed.\n");
        return EXIT_SUCCESS;
    }

    printf("\n%d Pokemon battle core test(s) failed.\n", failures);
    return EXIT_FAILURE;
}
