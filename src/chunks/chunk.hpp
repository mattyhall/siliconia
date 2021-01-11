#pragma once

#include <vector>
#include <optional>
#include <string>

namespace siliconia::chunks {

class asc_parse_exception : public std::exception {
public:
  asc_parse_exception(const std::string &filename, int line, std::string explaination);
  const char * what () const throw ();

private:
  std::string filename_;
  int line_;
  std::string explaination_;
};

struct rect {
  rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
  rect(std::pair<unsigned int, unsigned int> top_left, std::pair<unsigned int, unsigned int> bottom_right);

  unsigned int right() const;
  unsigned int bottom() const;
  
  // Naughty
  rect operator|(const rect &other) const;
  
  unsigned int x, y, width, height;
};

class Chunk {
public:
  Chunk(const std::string &path);
  
  rect rect() const;

  unsigned int cell_size;
  unsigned int nrows;
  unsigned int ncols;
  unsigned int xllcorner;
  unsigned int yllcorner;
  std::vector<std::optional<double>> data;
  
private:
  bool parse_header(const std::string &path, int n, std::string_view sv);
  void parse_numbers(const std::string &path, int n, const std::string &line);

  // Only used in init
  double nodata_value_;
};

}