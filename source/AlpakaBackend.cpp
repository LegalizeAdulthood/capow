#include "AlpakaUtilities.hpp"

#include <alpaka/alpaka.hpp>

void capow_alpaka_build_anchor()
{
    (void) capow::alpaka_util::index_2d(0U, 0U, 1U);
}
