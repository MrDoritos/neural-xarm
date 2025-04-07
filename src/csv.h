#pragma once

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

class CSVRow
{
    public:
        std::string_view operator[](std::size_t index) const
        {
            return std::string_view(&m_line[m_data[index] + 1], m_data[index + 1] -  (m_data[index] + 1));
        }
        std::size_t size() const
        {
            return m_data.size() - 1;
        }
        void readNextRow(std::istream& str)
        {
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
        
        std::string         m_line;
        std::vector<int>    m_data;
};

inline std::istream& operator>>(std::istream& str, CSVRow& data)
{
    data.readNextRow(str);
    return str;
}

class CSVIterator
{   
    public:
        typedef std::input_iterator_tag     iterator_category;
        typedef CSVRow                      value_type;
        typedef std::size_t                 difference_type;
        typedef CSVRow*                     pointer;
        typedef CSVRow&                     reference;

        CSVIterator(std::istream& str)  :m_str(str.good()?&str:nullptr) { ++(*this); }
        CSVIterator()                   :m_str(nullptr) {}

        // Pre Increment
        CSVIterator& operator++()               {if (m_str) { if (!m_str->good() || !((*m_str) >> m_row)){m_str = nullptr;}}return *this;}
        // Post increment
        CSVIterator operator++(int)             {CSVIterator    tmp(*this);++(*this);return tmp;}
        CSVRow const& operator*()   const       {return m_row;}
        CSVRow const* operator->()  const       {return &m_row;}

        bool operator==(CSVIterator const& rhs) {return ((this == &rhs) || ((this->m_str == nullptr) && (rhs.m_str == nullptr)));}
        bool operator!=(CSVIterator const& rhs) {return !((*this) == rhs);}
    private:
        std::istream*       m_str;
        CSVRow              m_row;
};

class CSVRowIterator
{
    public:
        typedef std::input_iterator_tag iterator_category;
        typedef std::string value_type;
        typedef std::size_t difference_type;
        typedef value_type *pointer;
        typedef value_type &reference;

        CSVRowIterator(const CSVRow &row):row(&row),pos(0) { set(); }
        CSVRowIterator():row(nullptr),pos(0) { }

        void set() { if (available()) { field = get(); } else { row = nullptr; } }
        bool available() const { return row && pos < row->size(); }
        CSVRowIterator &operator++() { pos++; set(); return *this; }
        CSVRowIterator operator++(int) { CSVRowIterator tmp(*this); ++(*this); return tmp; }
        value_type const &operator*() const { return field; }
        value_type const *operator->() const { return &field; }
        value_type const get() const { 
            auto sv = (*row)[pos];
            auto st = value_type(sv.begin(), sv.end()); 
            return st;
        }
        value_type::const_pointer str() const { 
            return field.c_str();
        }

        bool operator==(CSVRowIterator const &rhs) { return ((this == &rhs) || ((this->row == nullptr) && (rhs.row == nullptr))); }
        bool operator!=(CSVRowIterator const &rhs) { return !((*this) == rhs); }
    private:
        difference_type pos;
        const CSVRow *row;
        std::string field;
};

class CSVRowRange
{
    CSVRow &row;
    public:
        CSVRowRange(CSVRow &row)
            :row(row) { }
        CSVRowIterator begin() const { return CSVRowIterator(row); }
        CSVRowIterator end() const { return CSVRowIterator(); }
};

class CSVRange
{
    std::istream&   stream;
    public:
        CSVRange(std::istream& str)
            : stream(str)
        {}
        CSVIterator begin() const {return CSVIterator{stream};}
        CSVIterator end()   const {return CSVIterator{};}
};