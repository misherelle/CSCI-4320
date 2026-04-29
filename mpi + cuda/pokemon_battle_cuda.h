#ifndef POKEMON_BATTLE_CUDA_H
#define POKEMON_BATTLE_CUDA_H

#include "pokemon_battle_core.h"

#ifdef __cplusplus
extern "C" {
#endif

void pokemon_cuda_init(void);

void pokemon_cuda_update_local_rows(
    const PokemonGrid *current,
    PokemonGrid *next,
    size_t local_rows,
    int rank,
    int nprocs
);

#ifdef __cplusplus
}
#endif

#endif