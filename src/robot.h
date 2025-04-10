#pragma once

#include "xarm_common.h"

namespace robot {

template<typename ITER_T, typename VECTOR_T>
struct BaseRange {
    VECTOR_T &v;

    BaseRange() { }
    BaseRange(VECTOR_T &_v):
        v(_v) { }
    
    ITER_T begin() const {
        return v.begin();
    }

    ITER_T end() const {
        return v.end();
    }
};  

template<typename input_range>
struct BaseIterator {
    typedef std::input_iterator_tag iterator_category;
    typedef input_range::value_type value_type;
    typedef std::size_t difference_type;
    typedef value_type *pointer;
    typedef value_type &reference;

    difference_type pos;
    const input_range *vector;

    BaseIterator(const input_range &vec)
        :vector(&vec),pos(0) { }
    BaseIterator()
        :vector(0),pos(0) { }

    virtual bool check() const {
        return true;
    }

    pointer get() const {
        return vector[pos];
    }

    BaseIterator &operator++() {
        for (int i = pos + 1; i < vector.size(); i++) {
            if (vector[i]->visible) {
                pos++;
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

    value_type &operator*() const {
        return vector[pos];
    }

    value_type *operator->() const {
        return &vector[pos];
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

template<typename seg_T>
struct Robot_T {
    using seg_type = seg_T;
    using seg_vec = std::vector<seg_type*>;
    using iterator_type = seg_vec::iterator;
    using base_iterator = BaseIterator<seg_type*>;
    template<typename V_T>
    using range_type = BaseRange<base_iterator, V_T>;

    seg_vec segments;

    Robot_T() { }
    Robot_T(const seg_vec &segments)
        :segments(segments) { }

    struct VisibleIterator : public base_iterator {
        VisibleIterator(const base_iterator &v) { }
        VisibleIterator() { }

        bool check() const override {
            return this->get()->is_visible;
        }
    };

    struct ServoIterator : public base_iterator {
        ServoIterator(const base_iterator &v) { }
        ServoIterator() { }

        bool check() const override {
            return this->get()->add_slider;
        }
    };

    struct KinematicIterator : public base_iterator {
        KinematicIterator(const base_iterator &v) { }
        KinematicIterator() { }

        bool check() const override {
            return this->get()->solve_kinematic;
        }
    };

    range_type<base_iterator> get_segments() {
        return range_type<base_iterator>(segments);
    }

    range_type<VisibleIterator> get_visible_segments() {
        return range_type<VisibleIterator>(segments);
    }

    range_type<ServoIterator> get_servo_segments() {
        return range_type<ServoIterator>(segments);
    }

    range_type<KinematicIterator> get_kinematic_segments() {
        return range_type<KinematicIterator>(segments);
    }
};

using Robot = Robot_T<Segment>;

}