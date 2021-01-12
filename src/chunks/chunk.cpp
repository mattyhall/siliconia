#include "chunk.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

namespace siliconia::chunks {

asc_parse_exception::asc_parse_exception(
    std::string filename, int line, std::string explaination)
  : std::exception()
  , filename_(std::move(filename))
  , line_(line)
  , explaination_(std::move(explaination))
{
  auto ss = std::ostringstream{};
  ss << "Couldn't parse " << filename_ << ", error at line " << line_ << ": "
     << explaination_;
  what_ = ss.str();
}

const char *asc_parse_exception::what() const noexcept
{
  return what_.c_str();
}

rect::rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
  : x(x), y(y), width(w), height(h)
{
}

rect::rect(std::pair<unsigned int, unsigned int> top_left,
    std::pair<unsigned int, unsigned int> bottom_right)
  : x(top_left.first)
  , y(top_left.second)
  , width(bottom_right.first - top_left.first)
  , height(bottom_right.second - top_left.second)
{
}

unsigned int rect::right() const
{
  return x + width;
}
unsigned int rect::bottom() const
{
  return y + height;
}

rect rect::operator|(const rect &other) const
{
  auto l = std::min(x, other.x);
  auto r = std::max(right(), other.right());
  auto t = std::min(y, other.y);
  auto b = std::max(bottom(), other.bottom());
  return {{l, t}, {r, b}};
}

void rect::operator|=(const rect &other)
{
  *this = *this | other;
}

range::range()
  : min(std::numeric_limits<double>::max())
  , max(std::numeric_limits<double>::min())
{
}

range::range(double min, double max) : min(min), max(max)
{
}

range range::operator|(const range &other) const
{
  return range{std::min(min, other.min), std::max(max, other.max)};
}

void range::operator|=(const range &other)
{
  *this = *this | other;
}
void range::extend(double n)
{
  if (n < min) {
    min = n;
  }
  if (n > max) {
    max = n;
  }
}

double range::size()
{
  return max-min;
}

Chunk::Chunk(const std::string &path)
  : cell_size(0)
  , nrows(0)
  , ncols(0)
  , xllcorner(0)
  , yllcorner(0)
  , data()
  , range()
{
  std::cout << "Parsing " << path << std::endl;
  auto stream = std::ifstream{path.c_str(), std::ios::binary | std::ios::ate};
  auto size = stream.tellg();
  stream.seekg(0, std::ios::beg);
  auto s = std::vector<char>(size);
  stream.read(s.data(), size);

  auto n = 1;
  bool in_numbers = false;

  auto last_pos = s.begin();
  auto pos = std::find(s.begin(), s.end(), '\n');

  while (pos != s.end()) {
    auto sv = std::string_view{last_pos, pos};
    if (!in_numbers) {
      if (!parse_header(path, n, sv)) {
        in_numbers = true;
      }
    }
    if (in_numbers) {
      parse_numbers(path, n, sv);
    }
    n++;

    last_pos = pos + 1;
    pos = std::find(last_pos, s.end(), '\n');
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
  while (sv[v_pos] == ' ') {
    v_pos++;
  }
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

void Chunk::parse_numbers(const std::string &path, int n, std::string_view line)
{
  auto spaces = std::count_if(
      line.begin(), line.end(), [](char c) { return std::isspace(c); });
  if (spaces == line.size()) {
    return;
  }
  auto last_pos = size_t{0};
  auto pos = line.find(' ');
  while (true) {
    double f = 0.0;
    auto end = std::min(pos, line.size());
    std::from_chars(line.data() + last_pos, line.data() + end, f);
    if (f == nodata_value_) {
      data.push_back({});
    } else {
      range.extend(f);
      data.push_back(f);
    }

    if (pos == std::string::npos) {
      break;
    }

    last_pos = pos + 1;
    pos = line.find(' ', last_pos);
  }
}

rect Chunk::rect() const
{
  auto w = ncols * cell_size;
  auto h = nrows * cell_size;
  return {xllcorner, yllcorner - h, w, h};
}
} // namespace siliconia::chunks