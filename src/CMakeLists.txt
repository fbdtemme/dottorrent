
### ----------------------------------------------------------------------------
### Threading libs
### ----------------------------------------------------------------------------

target_sources(dottorrent
    PUBLIC
        chunk_reader.cpp
        file_entry.cpp
        file_storage.cpp
        hasher/md5.cpp
        hasher/sha1.cpp
        hasher/sha256.cpp
        hasher/sha512.cpp
        metafile.cpp
        metafile_parsing.cpp
        metafile_serialization.cpp
        storage_hasher.cpp
        v1_chunk_hasher.cpp
        v1_chunk_reader.cpp
        v1_chunk_verifier.cpp
        v2_chunk_reader.cpp
        v2_chunk_hasher.cpp
)