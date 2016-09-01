
#ifndef POPULATION_MANAGER_RPS_H
#define POPULATION_MANAGER_RPS_H

#include <iostream>
#include <limits>

#include "physics/Physics2D.h"

#include "evo/PopulationManager.h"
#include "tools/vector.h"
#include "tools/random_utils.h"

namespace emp {
namespace evo {

template <typename ORG>
class PopulationManager_RPS {
protected:
  using Organism_t = ORG;
  using Physics_t = CirclePhysics2D<RPSOrg>;

  Physics_t physics;
  emp::vector<Organism_t*> population;

  Random *random_ptr;
public:

  PopulationManager_RPS() :
    physics()
  {
    ;
  }

  // Allow this and derived classes to be identified as a population manager:
  static constexpr bool emp_is_population_manager = true;
  static constexpr bool emp_has_separate_generations = false;

  // Setup iterator for the population.
  friend class PopulationIterator<PopulationManager_RPS<Organism_t> >;
  using iterator = PopulationIterator<PopulationManager_RPS<Organism_t> >;
  // Operator overloading.
  Organism_t* & operator[](int i) { return population[i]; }

  iterator begin() { return iterator(this, 0); }
  iterator end() { return iterator(this, population.size()); }

  uint32_t size() const { return population.size(); }
  int GetSize() const { return (int) this->size(); }

  int AddOrg(Organism_t *new_org) {
    int pos = this->GetSize();
    population.push_back(new_org);
    physics.AddBody(new_org);
    return pos;
  }

  void Setup(Random *r) { random_ptr = r; }

  void Clear() {
    physics.Clear();
    population.clear();
  }

  void ConfigPop(double width, double height, double surface_friction) {
    physics.ConfigPhysics(width, height, random_ptr, surface_friction);
  }

  void Update() {
    std::cout << "POPM UPATE" << std::endl;
    physics.Update();
    std::cout << " -- popM manage population" << std::endl;
  }

};

}
}

#endif
