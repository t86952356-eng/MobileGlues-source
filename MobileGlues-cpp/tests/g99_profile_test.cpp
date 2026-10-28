#include "../config/g99_profile.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace g99_profile;
    assert(eligible("Mali-G57 MC2", "MT6789", "", ""));
    assert(eligible("Mali-G57", "MediaTek Helio G99", "", ""));
    assert(eligible("Mali-G57 MC2", "", "mt6789v/cd", ""));
    assert(eligible("Mali-G57 MC2", "", "", "mt6789"));
    assert(!eligible("Adreno (TM) 740", "MT6789", "", ""));
    assert(!eligible("Mali-G570", "MT6789", "", ""));
    assert(!eligible("Mali-G57 MC2", "MT67890", "", ""));
    assert(!eligible("Mali-G57 MC2", "Helio G990", "", ""));
    assert(!eligible("Mali-G57 MC2", "MT6833", "", ""));
    assert(!eligible("Mali-G57 MC2", "", "", ""));
    assert(!eligible("", "MT6789", "", ""));
    assert(cache_mib(-1) == 64 && cache_mib(32) == 64);
    assert(cache_mib(96) == 96 && cache_mib(2048) == 128);
    assert(!cache_flush_due(true, 0, 9000000000LL));
    assert(!cache_flush_due(true, 63, 4999999999LL));
    assert(cache_flush_due(true, 64, 0));
    assert(cache_flush_due(true, 1, 5000000000LL));
    assert(cache_flush_due(false, 16, 0));
    assert(!cache_flush_due(false, 15, 4999999999LL));
    // Changing only the active flag restores upstream batching, no disk rewrite.
    assert(cache_batch(false) == 16 && cache_batch(true) == 64);
    std::cout << "G99 profile: eligibility, bounded cache and persistence checks passed\n";
}
