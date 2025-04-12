#pragma once

#include "xarm_common.h"
#include <type_traits>

namespace robot {

enum IteratorType : int {
    None,
    Visible,
    Slider,
    Kinematic
};

template<int ITER_T, typename input_range>
struct BaseIterator {
    typedef std::input_iterator_tag iterator_category;
    typedef input_range::value_type value_type;
    typedef std::size_t difference_type;
    typedef value_type *pointer;
    typedef value_type &reference;

    difference_type pos;
    input_range *vector;

    BaseIterator(input_range *vec)
        :vector(vec),pos(0) { if (!check()) ++(*this); }
    BaseIterator()
        :vector(0),pos(0) { }

    bool check() {
        if constexpr (ITER_T == IteratorType::None)
            return true;
        else if constexpr (ITER_T == IteratorType::Visible)
            return this->get()->visible;
        else if constexpr (ITER_T == IteratorType::Slider)
            return this->get()->add_slider;
        else if constexpr (ITER_T == IteratorType::Kinematic)
            return this->get()->solve_kinematic;
        else
            static_assert(0, "No value\n");
    }

    value_type get() {
        return vector->at(pos);
    }

    BaseIterator &operator++() {
        pos++;
        for (;vector && pos < vector->size(); pos++) {
            if (check()) {
                return *this;
            }
        }

        vector = nullptr;
        return *this;
    }

    BaseIterator operator++(int) {
        BaseIterator tmp(*this);
        ++(*this);
        return tmp;
    }

    value_type &operator*() {
        return vector->at(pos);
    }

    value_type *operator->() {
        return &vector->at(pos);
    }

    bool operator==(BaseIterator const &rhs) const {
        return this == &rhs ||
              (this->vector == nullptr &&
               rhs.vector == nullptr);
    }

    bool operator!=(BaseIterator const &rhs) const {
        return !((*this) == rhs);
    }
};

template<int ITER_T, typename VECTOR_T>
struct BaseRange {
    using base_iterator = BaseIterator<ITER_T, VECTOR_T>;

    VECTOR_T *v;

    BaseRange() { }
    BaseRange(VECTOR_T *_v):
        v(_v) { }

    base_iterator begin() {
        return base_iterator(v);
    }

    base_iterator end() {
        return base_iterator();
    }
};  

template<typename seg_T>
struct Robot_T {
    using seg_type = seg_T;
    using seg_vec = std::vector<seg_type*>;
    using iterator_type = seg_vec::iterator;
    template<int ITER_T>
    using base_iterator = BaseIterator<ITER_T, seg_vec>;
    template<int ITER_T>
    using range_type = BaseRange<ITER_T, seg_vec>;

    seg_vec &segments;

    Robot_T() { }
    Robot_T(seg_vec &segments)
        :segments(segments) { }

    range_type<robot::None> get_segments() {
        return range_type<robot::None>(&segments);
    }

    range_type<robot::Visible> get_visible_segments() {
        return range_type<robot::Visible>(&segments);
    }

    range_type<robot::Slider> get_slider_segments() {
        return range_type<robot::Slider>(&segments);
    }

    range_type<robot::Kinematic> get_kinematic_segments() {
        return range_type<robot::Kinematic>(&segments);
    }
};

using Robot = Robot_T<robot::Segment>;

}