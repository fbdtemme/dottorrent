#include "dottorrent/hasher/sha256.hpp"

namespace dottorrent {

// ----------------------------------------------------------------------------------------------//

#if defined(DOTTORRENT_CRYPTO_OPENSSL)

sha256_hasher::sha256_hasher()
        : context_()
{
    SHA256_Init(&context_);
}


void sha256_hasher::update(std::span<const std::byte> data)
{
    SHA256_Update(&context_, reinterpret_cast<const void*>(data.data()), data.size());
}


auto sha256_hasher::finalize() -> hash_type
{
    hash_type hash_value;
    SHA256_Final(reinterpret_cast<unsigned char*>(hash_value.data()), &context_);
    // reinitialize the context_ object to be able to reuse the object
    SHA256_Init(&context_);
    return hash_value;
}

void sha256_hasher::finalize_to(std::span<std::byte> out)
{
    SHA256_Final(reinterpret_cast<unsigned char*>(out.data()), &context_);
    // reinitialize the context_ object to be able to reuse the object
    SHA256_Init(&context_);
}

#endif

// ----------------------------------------------------------------------------------------------//


auto make_sha256_hash(std::string_view data) -> sha256_hash
{
    sha256_hasher hasher{};
    hasher.update(std::span(reinterpret_cast<const std::byte*>(data.data()), data.size()));
    return hasher.finalize();
}

} // namespace dottorrent