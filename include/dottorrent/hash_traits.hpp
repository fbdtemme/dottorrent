#pragma once

#include "dottorrent/hash_function.hpp"
#include "dottorrent/hash.hpp"
#include "dottorrent/checksum.hpp"

#include "dottorrent/hasher/md5.hpp"
#include "dottorrent/hasher/sha1.hpp"
#include "dottorrent/hasher/sha256.hpp"


namespace dottorrent {

template <hash_function F>
struct hash_function_traits {};

#define DOTTORRENT_REGISTER_HASH_TRAIT(name)     \
template <>                                         \
struct hash_function_traits<hash_function::name>    \
{                                                   \
    using hasher_type   = name##_hasher;            \
    using hash_type     = name##_hash;              \
    using checksum_type = name##_checksum;          \
};                                                  \

DOTTORRENT_REGISTER_HASH_TRAIT(md5)
DOTTORRENT_REGISTER_HASH_TRAIT(sha1)
DOTTORRENT_REGISTER_HASH_TRAIT(sha256)

#undef DOTTORRENT_REGISTER_HASH_TRAIT

} // namespace dottorrent