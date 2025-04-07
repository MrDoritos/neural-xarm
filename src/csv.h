#pragma once

#include <iostream>
#include <vector>
#include <string>

namespace robot {

struct CSVRow {
    std::string_view operator[](std::size_t index) const;
    std::size_t size() const;
    void readNextRow(std::istream& str);
    
    std::string m_line;
    std::vector<int> m_data;
};

struct CSVIterator {
    typedef std::input_iterator_tag iterator_category;
    typedef CSVRow value_type;
    typedef std::size_t difference_type;
    typedef CSVRow* pointer;
    typedef CSVRow& reference;

    std::istream* m_str;
    CSVRow m_row;

    CSVIterator(std::istream& str);
    CSVIterator();

    CSVIterator& operator++();
    CSVIterator operator++(int);
    CSVRow const& operator*() const;
    CSVRow const* operator->() const;

    bool operator==(CSVIterator const& rhs) const;
    bool operator!=(CSVIterator const& rhs) const;
};

struct CSVRowIterator {
    typedef std::input_iterator_tag iterator_category;
    typedef std::string value_type;
    typedef std::size_t difference_type;
    typedef value_type *pointer;
    typedef value_type &reference;

    difference_type pos;
    const CSVRow *row;
    std::string field;

    CSVRowIterator(const CSVRow &row);
    CSVRowIterator();

    void set();
    bool available() const;
    CSVRowIterator &operator++();
    CSVRowIterator operator++(int);
    value_type const &operator*() const;
    value_type const *operator->() const;
    value_type const get() const;
    value_type::const_pointer str() const;
    bool operator==(CSVRowIterator const &rhs) const;
    bool operator!=(CSVRowIterator const &rhs) const;
};

struct CSVRowRange {
    CSVRow &row;

    CSVRowRange(CSVRow &row);

    CSVRowIterator begin() const;
    CSVRowIterator end() const;
};

struct CSVRange {
    std::istream& stream;

    CSVRange(std::istream& str);

    CSVIterator begin() const;
    CSVIterator end() const;
};

}