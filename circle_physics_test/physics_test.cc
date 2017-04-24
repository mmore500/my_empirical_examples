#include "tools/Random.h"
#include "physics/Physics2D.h"
#include "physics/PhysicsBodyOwner.h"

#include <iostream>

class EntityType0 : public emp::PhysicsBodyOwner_Base<emp::PhysicsBody2D<emp::Circle>> {
public:
  EntityType0() {

  }
  ~EntityType0() { ; }

};

class EntityType1 : public emp::PhysicsBodyOwner_Base<emp::PhysicsBody2D<emp::Circle>> {
public:
  EntityType1() {

  }
  ~EntityType1() { ; }
};

int main() {
  // Some useful aliases.
  using Physics_t = emp::CirclePhysics2D<EntityType0, EntityType1>;
  // Some necessaries.
  int random_seed = 10;
  emp::Random *random = new emp::Random(random_seed);

  // Test physics.
  std::cout << "Creating physics..." << std::endl;
  Physics_t physics;
  double w = 10;
  double h = 10;
  double f = 10;
  physics.ConfigPhysics(w, h, random, f);
  std::cout << "physics.GetWidth(): " << physics.GetWidth() << std::endl;
  std::cout << "physics.GetHeight(): " << physics.GetHeight() << std::endl;

  delete random;
  return 0.0;
}
