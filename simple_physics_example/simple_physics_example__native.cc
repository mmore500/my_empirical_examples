/*
*/

#include <iostream>
#include <string>
#include <sstream>

#include "./geometry/Point2D.h"
#include "./organisms/SimpleOrganism.h"
#include "./resources/SimpleResource.h"

#include "physics/Physics2D.h"

#include "base/vector.h"

#include "meta/meta.h"

#include "tools/Random.h"
#include "tools/assert.h"
#include "tools/TypeTracker.h"

#include "evo/World.h"

// Population manager functions:
// OnClear
// OnUpdate
// OnAddOrg
// OnAddOrgBirth

namespace emp {
namespace evo {
  class SimplePhysicsWorld {
  protected:
    using Organism_t = SimpleOrganism;
    using Resource_t = SimpleResource;
    using Physics_t = CirclePhysics2D<Organism_t, Resource_t>;


    Physics_t physics;
    emp::vector<Organism_t*> population;
    emp::vector<Resource_t*> resources;

    int max_pop_size;

  public:
    // TODO: PopulationManager_Base doesn't handle organisms just dying in the population very well
    SimplePhysicsWorld(double _w, double _h, Random *random_ptr, double _surface_friction)
    : physics() {
      physics.ConfigPhysics(_w, _h, random_ptr, _surface_friction);
    }
    ~SimplePhysicsWorld() {
      Clear();
    }

    void Clear() {
      physics.Clear();
      for (auto *org : population) delete org;
      for (auto *res : resources) delete res;
      population.resize(0);
      resources.resize(0);
    }

    // TODO: At the moment, totally ignores POpulationManager_Base stuff. Does not update fitness manager.
    int AddOrg(Organism_t *new_org) {
      int pos = (int)population.size();
      population.push_back(new_org);
      physics.AddBody(new_org);
      return pos;
    }

    void Update() {
      // Progress physics by one time step.
      physics.Update();

      // Manage resources.

      // Manage population.

      // Add new organisms.
    }

  };
}
}

int main(int argc, char *argv[]) {
  std::cout << "Hello world" << std::endl;

  using Organism_t = SimpleOrganism;
  using Resource_t = SimpleResource;
  using World_t = emp::evo::SimplePhysicsWorld;
  World_t *world;
  emp::Random *random;

  int rseed = 1;
  double width = 100;
  double height = 100;
  double friction = 0;
  int total_updates = 100;

  random = new emp::Random(rseed);
  world = new World_t(width, height, random, friction);

  world->AddOrg(new Organism_t(emp::Circle(emp::Point(0, 0), 10)));
  world->AddOrg(new Organism_t(emp::Circle(emp::Point(50, 50), 10)));
  world->AddOrg(new Organism_t(emp::Circle(emp::Point(25, 25), 10)));

  std::cout << "Has unique type? " << emp::has_unique_types<Organism_t, Organism_t>() << std::endl;
  // TODO: Why does below code not work?
  //emp_assert(emp::has_unique_first_type<int, bool>());
  //emp_assert(true);
  for (int u = 0; u < total_updates; ++u) {
    std::cout << "Update: " << u << std::endl;
    world->Update();
  }

  return 0;
}
