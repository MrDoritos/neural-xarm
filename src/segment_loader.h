#pragma once

#include "xarm_common.h"

namespace robot {

struct CSVRowIterator;

template<typename seg_T = SegmentT<>>
struct SegmentLoader_T {
    using seg_type = seg_T;
    using mesh_type = seg_type::mesh_type;
    using seg_vec = std::vector<seg_type*>;
    using mesh_vec = std::vector<mesh_type*>;

    seg_vec segments;
    mesh_vec meshes;
    std::string directory;

    SegmentLoader_T() { }

    SegmentLoader_T(const std::string &directory):
            directory(directory) { }

    std::string get_directory_path_rel() const;

    std::string get_file_path_rel(const std::string &filename) const;

    std::ifstream open_in_directory(const std::string &filename) const;

    void set(mesh_vec &_meshes, seg_vec &_segments, seg_vec &_visibles, seg_vec &_sliders);

    int parse(CSVRowIterator &iter, seg_type *segment);

    int load_meshes();

    int load(const std::string &directory);

    int load();
};

using SegmentLoader = robot::SegmentLoader_T<>;

}