#include "tools/Random.h"
#include "physics/Physics2D.h"
#include "physics/PhysicsBody2D.h"
#include "physics/PhysicsBodyOwner.h"
#include "geometry/Circle2D.h"

#include <iostream>

class EntityType0 : public emp::PhysicsBodyOwner_Base<emp::PhysicsBody2D<emp::Circle>> {
private:
  using Body_t = emp::PhysicsBody2D<emp::Circle>;
  using emp::PhysicsBodyOwner_Base<Body_t>::body;
  using emp::PhysicsBodyOwner_Base<Body_t>::has_body;
  int name;

public:
  EntityType0() : name(-1) { ; }
  EntityType0(int _name, const emp::Circle &_p) : name(_name) {
    // Make a new body for this entity.
    body = nullptr;
    has_body = false;
    AttachBody(new Body_t(_p));
    body->SetMaxPressure(10);
    body->SetMass(10);
  }
  ~EntityType0() { ; }

  int GetName() { return name; }
  void SetName(int _name) { name = _name; }

};

class EntityType1 : public emp::PhysicsBodyOwner_Base<emp::PhysicsBody2D<emp::Circle>> {
private:
  using Body_t = emp::PhysicsBody2D<emp::Circle>;
  using emp::PhysicsBodyOwner_Base<Body_t>::body;
  using emp::PhysicsBodyOwner_Base<Body_t>::has_body;
  int name;

public:
  EntityType1() : name(-1) { ; }
  EntityType1(int _name) : name(_name) { ; }
  ~EntityType1() { ; }

  int GetName() { return name; }
  void SetName(int _name) { name = _name; }
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

  // Test physics bodies.
  std::cout << "Creating some physics bodies..." << std::endl;
  // emp::PhysicsBody2D<emp::Circle> *pbody1 = new emp::PhysicsBody2D<emp::Circle>();
  // emp::PhysicsBody2D<emp::Circle> *pbody2 = new emp::PhysicsBody2D<emp::Circle>();
  // Make some entities that have bodies.
  emp::Point midpoint = emp::Point(0, 0);
  int radius = 1;
  EntityType0 et0_1(0, emp::Circle(midpoint, radius));
  std::cout << "Creating entity type0: " << std::endl;
  std::cout << "\thas body? " << et0_1.HasBody() << std::endl;
  std::cout << "\tCleanup? " << et0_1.GetBodyCleanup() << std::endl;
  EntityType1 et1_1(0);
  EntityType0 et0_2(1, emp::Circle(emp::Point(0, 0), 1));
  // Add entities to physics:
  physics.AddBody(&et0_1);
  physics.AddBody(&et0_2);
  std::cout << "\tCleanup? " << et0_1.GetBodyCleanup() << std::endl;
  // Loop through bodies:
  std::cout << "Printing bodies: " << std::endl;
  for (auto body : physics.GetBodySet()) {
    std::cout << "\tBody mass: " << body->GetMass() << std::endl;
  }

  // Test physics updates
  std::cout << "Testing physics update: " << std::endl;
  physics.Update();
  std::cout << "Physics update success!" << std::endl;
  
  //physics.RemoveBody(&et0_1);
  // std::cout << "Cleaning up physics bodies..." << std::endl;
  // delete pbody1;
  // delete pbody2;
  delete random;
  return 0.0;
}
