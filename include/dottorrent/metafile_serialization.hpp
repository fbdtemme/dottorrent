#pragma once

#include "dottorrent/file_storage.hpp"
#include "dottorrent/file_entry.hpp"
#include "dottorrent/metafile.hpp"

namespace dottorrent::detail {

namespace bc = bencode;

bencode::bvalue make_bvalue_infodict_v1(const metafile& m);

bencode::bvalue make_bvalue_infodict_v2(const metafile& m);

bencode::bvalue make_bvalue_infodict_hybrid(const metafile& m);

bencode::bvalue make_bvalue_v1(const metafile& m);

bencode::bvalue make_bvalue_v2(const metafile& m);

bencode::bvalue make_bvalue_hybrid(const metafile& m);



}