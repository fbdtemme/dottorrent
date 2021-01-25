#include <algorithm>
#include "dottorrent/announce_url_list.hpp"


namespace dottorrent {

namespace rng = std::ranges;

std::vector<std::vector<std::string>>
as_nested_vector(const announce_url_list& announces)
{
    std::vector<std::vector<std::string>> result;
    result.resize(announces.tier_count());

    for (auto idx = 0; idx < announces.tier_count(); ++idx) {
        auto [begin, end] = announces.get_tier(idx);
        std::transform(begin, end, std::back_inserter(result[idx]),
                       [](const announce_url& a) { return  a.url; });
    }
    return result;
}

}