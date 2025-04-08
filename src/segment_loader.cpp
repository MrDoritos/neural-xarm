#include "segment_loader.h"
#include "csv.h"

namespace robot {

inline CSVRowIterator &operator>>(CSVRowIterator &iter, std::string &val) {
    assert(iter.available() && "Nothing left to parse\n");
    val = (iter++).str();
    return iter;
}

inline bool lower_equals(const unsigned char a, const unsigned char b) {
    return std::tolower(a) == std::tolower(b);
}

inline bool lower_contains(const std::string &a, const std::string &b) {
    const std::string *v[2] = {&a, &b};
    if (a.size() > b.size())
        std::swap(v[0], v[1]);
    return std::equal(v[0]->begin(), v[0]->end(), v[1]->begin(), lower_equals);
}

inline CSVRowIterator &operator>>(CSVRowIterator &iter, bool &val) {
    assert(iter.available() && "Nothing left to parse\n");
    std::string str((iter++).str());

    val = lower_contains(str, "1") || lower_contains(str, "true");
    return iter;
}

inline CSVRowIterator &operator>>(CSVRowIterator &iter, int &val) {
    assert(iter.available() && "Nothing left to parse\n");
    val = atoi((iter++).str());
    return iter;
}

inline CSVRowIterator &operator>>(CSVRowIterator &iter, float &val) {
    assert(iter.available() && "Nothing left to parse\n");
    val = atof((iter++).str());
    return iter;
}

inline CSVRowIterator &operator>>(CSVRowIterator &iter, glm::vec3 &vec) {
    iter >> vec.x;
    iter >> vec.y;
    iter >> vec.z;
    return iter;
}

template<>
SegmentLoader::SegmentLoader_T() { }

template<>
SegmentLoader::SegmentLoader_T(const std::string &directory):
        directory(directory) { }

template<>
std::string SegmentLoader::get_directory_path_rel() const {
    if (!directory.size())
        return "./";
    if (directory.find_last_of('/') == directory.size()-1)
        return directory;
    return directory + "/";
}

template<>
std::string SegmentLoader::get_file_path_rel(const std::string &filename) const {
    if (filename.find_first_of('/') == 0)
        return filename;
    return get_directory_path_rel() + filename;
}

template<>
std::ifstream SegmentLoader::open_in_directory(const std::string &filename) const {
    return std::ifstream(get_file_path_rel(filename));
}

template<>
void SegmentLoader::set(mesh_vec &_meshes, seg_vec &_segments, seg_vec &_visibles, seg_vec &_sliders) {
    _meshes = meshes;
    _segments = segments;
    _visibles = segments;
    _sliders = segments;

    for (auto it = segments.begin(); it != segments.end(); it++) {
        if (it != segments.begin())
            (*it)->parent = *(it - 1);
        if (it + 1 != segments.end())
            (*it)->child = *(it + 1);
    }
}

template<>
int SegmentLoader::parse(CSVRowIterator &iter, seg_type *segment) {
    std::string mesh_filename;

    iter >> segment->servo_num;
    iter >> segment->origin_offset;
    iter >> mesh_filename;
    iter >> segment->rotation_axis;
    iter >> segment->length;
    iter >> segment->model_scale;
    iter >> segment->add_slider;
    iter >> segment->solve_kinematic;
    iter >> segment->torque;
    iter >> segment->mass;

    segment->mesh = new mesh_type;
    this->meshes.push_back(segment->mesh);
    auto mesh_path = get_file_path_rel(mesh_filename);
    return segment->mesh->loadObj(mesh_path.c_str());
}

// Assign vertex buffers
template<>
int SegmentLoader::load_meshes() {
    for (auto *mesh : meshes)
        if (mesh->load())
            return glfail;
    return glsuccess;
}

template<>
int SegmentLoader::load() {
    auto path = get_file_path_rel("segments.csv");
    std::ifstream file(path);

    if (!file.is_open()) {
        fprintf(stderr, "Failed to open file %s\n", path.c_str());
        return glfail;
    }

    for (auto &row : CSVRange(file)) {
        auto iter = CSVRowIterator(row);
        seg_type segment;

        if (parse(iter, &segment)) {
            fprintf(stderr, "Failed to parse row \"%s\" in %s\n", row.m_line.c_str(), path.c_str());
            continue;
        }

        fprintf(stderr, "Loaded segment %i\n", segment.servo_num);

        segments.push_back(new seg_type(segment));
    }

    return glsuccess;
}

template<>
int SegmentLoader::load(const std::string &directory) {
    this->directory = directory;

    return load();
}

}