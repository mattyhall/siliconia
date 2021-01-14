#pragma once

#include <optional>
#include <string>
#include <vector>

namespace siliconia::chunks {

class asc_parse_exception : public std::exception {
public:
  asc_parse_exception(std::string filename, int line, std::string explaination);
  const char *what() const noexcept override;

private:
  std::string filename_;
  int line_;
  std::string explaination_;
  std::string what_;
};

struct rect {
  rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
  rect(std::pair<unsigned int, unsigned int> top_left,
      std::pair<unsigned int, unsigned int> bottom_right);

  unsigned int right() const;
  unsigned int bottom() const;

  // Naughty
  rect operator|(const rect &other) const;
  void operator|=(const rect &other);

  unsigned int x, y, width, height;
};

struct range {
public:
  range();
  range(float min, float max);

  void extend(float n);

  float size() const;

  range operator|(const range &other) const;
  void operator|=(const range &other);

  float min, max;
};

class Chunk {
public:
  explicit Chunk(const std::string &path);

  rect rect() const;

  range range;
  unsigned int cell_size;
  unsigned int nrows;
  unsigned int ncols;
  unsigned int xllcorner;
  unsigned int yllcorner;
  std::vector<float> data;
  float nodata_value;

private:
  bool parse_header(const std::string &path, int n, std::string_view sv);
  void parse_numbers(const std::string &path, int n, std::string_view line);

  // Only used in init
};

} // namespace siliconia::chunks