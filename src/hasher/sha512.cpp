#include "dottorrent/hasher/sha512.hpp"

namespace dottorrent {

// ----------------------------------------------------------------------------------------------//

#if defined(DOTTORRENT_CRYPTO_OPENSSL)

sha512_hasher::sha512_hasher()
        : context_()
{
    SHA512_Init(&context_);
}


void sha512_hasher::update(std::span<const std::byte> data)
{
    SHA512_Update(&context_, reinterpret_cast<const void*>(data.data()), data.size());
}


auto sha512_hasher::finalize() -> hash_type
{
    hash_type hash_value;
    SHA512_Final(reinterpret_cast<unsigned char*>(hash_value.data()), &context_);
    // reinitialize the context_ object to be able to reuse the object
    SHA512_Init(&context_);
    return hash_value;
}

void sha512_hasher::finalize_to(std::span<std::byte> out)
{
    SHA512_Final(reinterpret_cast<unsigned char*>(out.data()), &context_);
    // reinitialize the context_ object to be able to reuse the object
    SHA512_Init(&context_);
}

#endif

// ----------------------------------------------------------------------------------------------//


auto make_sha512_hash(std::string_view data) -> sha512_hash
{
    sha512_hasher hasher{};
    hasher.update(std::span(reinterpret_cast<const std::byte*>(data.data()), data.size()));
    return hasher.finalize();
}

} // namespace dottorrent