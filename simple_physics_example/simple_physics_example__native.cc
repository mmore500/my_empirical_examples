/*
*/

#include <iostream>
#include <string>
#include <sstream>

#include "./geometry/Point2D.h"
#include "./world/SimplePhysicsWorld.h"
#include "./world/SimpleOrganism.h"
#include "./world/SimpleResource.h"
#include "./world/SimpleResourceDispenser.h"

#include "physics/Physics2D.h"

#include "base/vector.h"

#include "meta/meta.h"

#include "tools/Random.h"
#include "tools/assert.h"
#include "tools/TypeTracker.h"

#include "evo/World.h"

int main(int argc, char *argv[]) {
  std::cout << "Hello world" << std::endl;

  using Organism_t = SimpleOrganism;
  using Resource_t = SimpleResource;
  using Dispenser_t = SimpleResourceDispenser;
  using World_t = emp::evo::SimplePhysicsWorld;
  World_t *world;
  emp::Random *random;

  int rseed = 1;
  double width = 100;
  double height = 100;
  double friction = 0;
  int total_updates = 100;

  random = new emp::Random(rseed);

  std::function<void(Dispenser_t*)> dcallback = [](Dispenser_t* disp) { std::cout << "Dispense timer going off!" << std::endl; };
  Dispenser_t dispenser0(emp::Circle(emp::Point(0, 0), 10));
  dispenser0.SetDispenseRate(10);
  dispenser0.SetDispenseAmount(1);
  dispenser0.RegisterDispenserTimerCallback(dcallback);
  dispenser0.Dispense(random);
  for (int u = 0; u < total_updates; ++u) {
    //std::cout << "========= u: " << u << "=========" << std::endl;
    dispenser0.Evaluate();
  }

  return 0;
}
