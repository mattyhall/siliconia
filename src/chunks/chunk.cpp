#include "chunk.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <locale>

namespace siliconia::chunks {
  
asc_parse_exception::asc_parse_exception(const std::string &filename, int line, std::string explaination)
  : std::exception()
  , filename_(filename)
  , line_(line)
  , explaination_(explaination) {}

const char *asc_parse_exception::what () const throw ()
{
  auto ss = std::ostringstream{};
  ss << "Couldn't parse " << filename_ << ", error at line " << line_ << ": " << explaination_;
  return ss.str().c_str(); 
}

rect::rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
  : x(x), y(y), width(w), height(h)
{}

rect::rect(std::pair<unsigned int, unsigned int> top_left, std::pair<unsigned int, unsigned int> bottom_right)
  : x(top_left.first)
  , y(top_left.second)
  , width(bottom_right.first - top_left.first)
  , height(bottom_right.second - top_left.second)
{}

unsigned int rect::right() const { return x + width; }
unsigned int rect::bottom() const { return y + height; }

rect rect::operator|(const rect &other) const
{
  auto l  = std::min(x, other.x);
  auto r = std::max(right(), other.right());
  auto t = std::min(y, other.y);
  auto b = std::max(bottom(), other.bottom());
  return {{l, t}, {r, b}};
}

Chunk::Chunk(const std::string &path)
  : cell_size(0), nrows(0), ncols(0), xllcorner(0), yllcorner(0), data()
{
  auto stream = std::ifstream{path.c_str(), std::ios::in};
  auto n = 1;
  auto line = std::string{};
  bool in_numbers = false;
  while (std::getline(stream, line)) {
    auto sv = std::string_view{line};
    if (!in_numbers) {
      if (!parse_header(path, n, sv)) {
        in_numbers = true;
      }
    }
    if (in_numbers) {
      parse_numbers(path, n, line);
    }
    n++;
  }
}

bool Chunk::parse_header(const std::string &path, int n, std::string_view sv)
{
  auto pos = sv.find(' ');
  if (pos == std::string::npos) {
    throw asc_parse_exception{path, n, "Ill formed header (no space)"};
  }
  auto k = sv.substr(0, pos);
  if (k[0] >= '0' && k[0] <= '9') {
    if (nrows == 0 || ncols == 0 || cell_size == 0) {
      throw asc_parse_exception{path, n, "Didn't get all expected values"};
    }
    data.reserve(ncols * nrows);
    return false;
  }
  auto v_pos = pos;
  while (sv[v_pos] == ' ') { v_pos++; }
  auto v = sv.substr(v_pos);
  if (k == "ncols") {
    ncols = std::atoi(std::string{v}.c_str());
  } else if (k == "nrows") {
    nrows = std::atoi(std::string{v}.c_str());
  } else if (k == "xllcorner") {
    xllcorner = std::atoi(std::string{v}.c_str());
  } else if (k == "yllcorner") {
    yllcorner = std::atoi(std::string{v}.c_str());
  } else if (k == "cellsize") {
    cell_size = std::atoi(std::string{v}.c_str());
  } else if (k == "NODATA_value") {
    nodata_value_ = std::atof(std::string{v}.c_str());
  } else {
    throw asc_parse_exception{path, n, "Unexpected key"};
  }
  return true;
}

void Chunk::parse_numbers(const std::string &path, int n, const std::string &line)
{
  auto spaces = std::count_if(line.begin(), line.end(), [](char c) { return std::isspace(c); });
  if (spaces == line.size()) {
    return;
  }
  auto last_pos = size_t{0};
  auto pos = line.find(' ');
  while (true) {
    auto num = line.substr(0, pos);
    auto f = std::atof(num.c_str());
    if (f == nodata_value_) {
      data.push_back({});
    } else {
      data.push_back(f);
    }
    
    if (pos == std::string::npos) {
      break;
    }

    last_pos = pos;
    pos = line.find(' ', last_pos+1);
  }
}

rect Chunk::rect() const
{
  auto w = ncols * cell_size;
  auto h = nrows * cell_size;
  return { xllcorner, yllcorner - h, w, h};
}

}