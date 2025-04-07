#include "csv.h"

namespace robot {

std::string_view CSVRow::operator[](std::size_t index) const {
    return std::string_view(&m_line[m_data[index] + 1], m_data[index + 1] -  (m_data[index] + 1));
}

std::size_t CSVRow::size() const {
    return m_data.size() - 1;
}

void CSVRow::readNextRow(std::istream& str) {
    while (str.good()) {
        std::getline(str, m_line);
        auto f_comma = m_line.find(',');

        if (m_line.find('\n') < f_comma)
            continue;
        if (m_line.find('#') < f_comma)
            continue;
        if (m_line.size() < 1)
            continue;

        break;
    }

    m_data.clear();
    m_data.emplace_back(-1);
    std::string::size_type pos = 0;
    std::string::size_type comment_pos = m_line.find('#', 0);

    while((pos = m_line.find(',', pos)) != std::string::npos)
    {
        m_data.emplace_back(pos);
        ++pos;

        if (pos > comment_pos)
            break;
    }
    // This checks for a trailing comma with no data after it.
    pos   = m_line.size();
    m_data.emplace_back(pos);
}

inline std::istream& operator>>(std::istream& str, CSVRow& data) {
    data.readNextRow(str);
    return str;
}

CSVIterator::CSVIterator(std::istream& str):
        m_str(str.good()?&str:nullptr) { ++(*this); }

CSVIterator::CSVIterator():
        m_str(nullptr) { }

CSVIterator& CSVIterator::operator++() {
    if (m_str) { 
        if (!m_str->good() || !((*m_str) >> m_row)) {
            m_str = nullptr;
        }
    }
    return *this;
}

CSVIterator CSVIterator::operator++(int) {
    CSVIterator tmp(*this);
    ++(*this);
    return tmp;
}

CSVRow const& CSVIterator::operator*() const {
    return m_row;
}

CSVRow const* CSVIterator::operator->() const {
    return &m_row;
}

bool CSVIterator::operator==(CSVIterator const& rhs) const {
    return ((this == &rhs) || ((this->m_str == nullptr) && (rhs.m_str == nullptr)));
}

bool CSVIterator::operator!=(CSVIterator const& rhs) const {
    return !((*this) == rhs);
}

CSVRowIterator::CSVRowIterator(const CSVRow &row):
        row(&row),
        pos(0) { set(); }

CSVRowIterator::CSVRowIterator():
        row(nullptr),
        pos(0) { }

void CSVRowIterator::set() {
    if (available()) {
        field = get();
    } else {
        row = nullptr;
    }
}

bool CSVRowIterator::available() const {
    return row && pos < row->size();
}

CSVRowIterator &CSVRowIterator::operator++() {
    pos++;
    set();
    return *this;
}

CSVRowIterator CSVRowIterator::operator++(int) {
    CSVRowIterator tmp(*this);
    ++(*this);
    return tmp;
}

CSVRowIterator::value_type const &CSVRowIterator::operator*() const {
    return field;
}

CSVRowIterator::value_type const *CSVRowIterator::operator->() const {
    return &field;
}

CSVRowIterator::value_type const CSVRowIterator::get() const { 
    auto sv = (*row)[pos];
    auto st = value_type(sv.begin(), sv.end()); 
    return st;
}

CSVRowIterator::value_type::const_pointer CSVRowIterator::str() const { 
    return field.c_str();
}

bool CSVRowIterator::operator==(CSVRowIterator const &rhs) const {
    return ((this == &rhs) || ((this->row == nullptr) && (rhs.row == nullptr)));
}

bool CSVRowIterator::operator!=(CSVRowIterator const &rhs) const {
    return !((*this) == rhs);
}

CSVRowRange::CSVRowRange(CSVRow &row):
        row(row) { }

CSVRowIterator CSVRowRange::begin() const {
    return CSVRowIterator(row);
}

CSVRowIterator CSVRowRange::end() const {
    return CSVRowIterator();
}

CSVRange::CSVRange(std::istream& str):
        stream(str) { }

CSVIterator CSVRange::begin() const {
    return CSVIterator{stream};
}

CSVIterator CSVRange::end() const {
    return CSVIterator{};
}

}