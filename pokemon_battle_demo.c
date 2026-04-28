#include "pokemon_battle_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s [rows cols steps] [--random seed]\n\n"
            "Examples:\n"
            "  %s\n"
            "  %s 20 40 10\n"
            "  %s 20 40 10 --random 4320\n",
            prog, prog, prog, prog);
}

int main(int argc, char **argv)
{
    size_t rows = 12;
    size_t cols = 24;
    int steps = 5;
    int use_random = 0;
    unsigned int seed = 4320;

    if (argc != 1 && argc != 4 && argc != 6) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc >= 4) {
        rows = (size_t)strtoull(argv[1], NULL, 10);
        cols = (size_t)strtoull(argv[2], NULL, 10);
        steps = atoi(argv[3]);
    }

    if (argc == 6) {
        if (strcmp(argv[4], "--random") != 0) {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
        use_random = 1;
        seed = (unsigned int)strtoul(argv[5], NULL, 10);
    }

    if (rows == 0 || cols == 0 || steps < 0) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    PokemonGrid grid = pokemon_grid_create(rows, cols);
    PokemonGrid scratch = pokemon_grid_create(rows, cols);

    if (use_random) {
        pokemon_grid_seed_random(&grid, seed);
    } else {
        pokemon_grid_seed_pattern(&grid);
    }

    printf("Pokemon battle territory simulation (serial core)\n");
    printf("Types: N Normal, F Fire, W Water, E Electric, G Grass, I Ice, H Fighting, P Poison, D Ground\n");
    printf("       L Flying, Y Psychic, B Bug, R Rock, O Ghost, A Dragon, K Dark, S Steel, Q Fairy\n");
    printf("\nStep 0\n");
    pokemon_grid_print(&grid);

    pokemon_run(&grid, &scratch, steps, 1);

    pokemon_grid_free(&scratch);
    pokemon_grid_free(&grid);
    return EXIT_SUCCESS;
}
