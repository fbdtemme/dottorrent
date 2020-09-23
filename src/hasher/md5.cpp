#include "dottorrent/hasher/md5.hpp"

namespace dottorrent {

#if defined(DOTTORRENT_USE_OPENSSL)

md5_hasher::md5_hasher()
        : context_()
{
    MD5_Init(&context_);
}

void md5_hasher::update(std::span<const std::byte> data)
{
    MD5_Update(&context_, reinterpret_cast<const void*>(data.data()), data.size());
}

void md5_hasher::finalize_to(std::span<std::byte> out)
{
    Expects(out.size() >= md5_hasher::hash_type::size_bytes);
    MD5_Final(reinterpret_cast<unsigned char*>(out.data()), &context_);
    // reinitialize the context_ object to be able to reuse the object
    MD5_Init(&context_);
}

auto md5_hasher::finalize() -> md5_hasher::hash_type
{
    hash_type hash_value;
    MD5_Final(reinterpret_cast<unsigned char*>(hash_value.data()), &context_);
    // reinitialize the context_ object to be able to reuse the object
    MD5_Init(&context_);
    return hash_value;
}


#endif


#if defined(DOTTORRENT_USE_BOTAN)

md5_hasher::md5_hasher()
        : constext_()
{}

void md5_hasher::update(std::span<const std::byte> data)
{
    context_.update(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
}

auto md5_hasher::finalize_to(std::span<const std::byte> out)
{
    hash_type hash_value;
    context_.final(reinterpret_cast<std::uint8_t*>(out.data()));
    return hash_value;
}

auto md5_hasher::finalize() -> hash_type
{
    hash_type hash_value;
    context_.final(reinterpret_cast<std::uint8_t*>(hash_value.data()));
    return hash_value;
}



#endif

} // namespace dottorrent
