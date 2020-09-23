#include "dottorrent/hasher/sha1.hpp"

namespace dottorrent {

#if defined(DOTTORRENT_CRYPTO_OPENSSL)

sha1_hasher::sha1_hasher()
        : context_()
{
    SHA1_Init(&context_);
}

void sha1_hasher::update(std::span<const std::byte> data)
{
    SHA1_Update(&context_, reinterpret_cast<const void*>(data.data()), data.size());
}

sha1_hasher::hash_type sha1_hasher::finalize()
{
    hash_type hash_value {};
    SHA1_Final(reinterpret_cast<unsigned char*>(hash_value.data()), &context_);
    // reinitialize the context_ object to be able to reuse the object
    SHA1_Init(&context_);
    return hash_value;
}

void sha1_hasher::finalize_to(std::span<std::byte> out)
{
    SHA1_Final(reinterpret_cast<unsigned char*>(out.data()), &context_);
    // reinitialize the context_ object to be able to reuse the object
    SHA1_Init(&context_);
}

#endif

#if defined(DOTTORRENT_CRYPTO_BOTAN)

sha1_hasher::sha1_hasher()
        : context_()
{}

void sha1_hasher::update(std::span<const std::byte> data)
{
    context_.update(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
}


auto sha1_hasher::finalize() -> sha1_hasher::hash_type
{
    sha1_hasher::hash_type digest {};
    context_.final(reinterpret_cast<std::uint8_t*>(digest.data()));
    return digest;
}

#endif


#if defined(DOTTORRENT_CRYPTO_CRYPTOPP)

sha1_hasher::sha1_hasher()
        : context_()
{}


void sha1_hasher::update(std::span<const std::byte> data)
{
    context_.Update(reinterpret_cast<const byte*>(data.data()), data.size());
}


auto sha1_hasher::finalize() -> hash_type
{
    hash_type hash_value;
    context_.Final(reinterpret_cast<byte*>(hash_value.data()));
    context_.Restart();
    return hash_value;
}

#endif

auto make_sha1_hash(std::string_view data) -> sha1_hash
{
    sha1_hasher hasher{};
    hasher.update(std::span(reinterpret_cast<const std::byte*>(data.data()), data.size()));
    return hasher.finalize();
}

} // namespace dottorrent