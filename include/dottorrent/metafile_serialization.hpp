#pragma once

#include <bencode/bvalue.hpp>
#include <bencode/events/event_connector.hpp>

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

//
//template <bc::event_consumer EC>
//void make_infodict_v1(bc::event_connector<EC>& connector, const metafile& m)
//{
//    const auto& storage = m.storage();
//
//    // begin the info dict
//    connector << bc::begin_dict;
//
//    if (storage.file_mode() == file_mode::single) {
//        auto& file = storage.at(0);
//
//        connector << "length" << file.file_size();
//        connector << "name" << file.path().filename().string();
//    }
//    else if (storage.file_mode() == file_mode::multi) {
//        connector << "files";
//        connector << bc::begin_list;
//
//        for (auto& file : storage) {
//            connector << bc::begin_dict;
//
//            if (file.attributes()) {
//                connector << "attr" << file.attributes().value();
//            }
//            connector << "length" <<  file.file_size();
//            connector << "path" <<  file.path();
//
//            if (file.is_symlink()) {
//                connector << "symlink path" << file.symlink_path().value();
//            }
//            connector << bc::end_dict;
//        }
//        connector << bc::end_list;
//
//        if (m.name().empty()) {
//            connector << "name" << storage.root_directory().filename().string());
//        }
//        else {
//            connector << "name" << m.name();
//        }
//
//        connector << "piece length" << storage.piece_size();
//        connector << "pieces" << make_v1_pieces_string(storage);
//    }
//}

}