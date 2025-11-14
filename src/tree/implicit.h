#pragma once
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <vector>

class Implicit {
public:
  float cutoff = 0.f;
  // Point
  virtual float eval(glm::vec3 p1, glm::vec3 p2) const = 0;
  // LineSegment
  virtual float eval(glm::vec3 p1, glm::vec3 a, glm::vec3 b) const = 0;
  // Path
  virtual float eval(glm::vec3 position, const std::vector<glm::vec3> &strand,
                     std::size_t hint = 0) const = 0;
};

class DistanceField : public Implicit {
public:
  DistanceField(float cutoff) { this->cutoff = cutoff; };
  // Point
  float eval(glm::vec3 p1, glm::vec3 p2) const;
  // LineSegment
  float eval(glm::vec3 p1, glm::vec3 a, glm::vec3 b) const;
  // Path
  float eval(glm::vec3 position, const std::vector<glm::vec3> &strand,
             std::size_t hint = 0) const;

private:
  virtual float potential(float distance) const = 0;
};

class MetaBalls : public DistanceField {
public:
  MetaBalls(float a, float b) : DistanceField(b), a(a), b(b){};
  MetaBalls(nlohmann::json &options)
      : MetaBalls(options.at("max_val"),
                  options.at("range")){};

  float get_a() const {return a;}
  float get_b() const {return b;}
private:
  float a;
  float b;
  float potential(float distance) const;
};

struct Segment{
    glm::vec3 start;    
    glm::vec3 end;    
    uint32_t strand_id;
    MetaBalls f;
};
