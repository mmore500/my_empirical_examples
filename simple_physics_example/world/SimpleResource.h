/*
  resources/SimpleResource.h
    Defines the SimpleResource class.
*/

#ifndef SIMPLERESOURCE_H
#define SIMPLERESOURCE_H

#include "geometry/Circle2D.h"
#include "tools/BitVector.h"

#include "physics/PhysicsBody2D.h"



class SimpleResource : public emp::PhysicsBodyOwner_Base<emp::PhysicsBody2D<emp::Circle>> {

  private:
    using Body_t = emp::PhysicsBody2D<emp::Circle>;
    using emp::PhysicsBodyOwner_Base<Body_t>::body;
    using emp::PhysicsBodyOwner_Base<Body_t>::has_body;

    double value;
    double age;
    int resource_id; // Used for coloring.
    emp::BitVector affinity;


  public:
    SimpleResource(const emp::Circle &_p, double _value = 1.0, const emp::BitVector & _affinity = emp::BitVector(1, false))
    : value(_value), age(0.0), affinity(_affinity)
    {
      UpdateResourceID();
      body = nullptr;
      has_body = false;
      AttachBody(new Body_t(_p));
      body->SetMaxPressure(99999); // Big number.
      body->SetMass(5); // TODO: make this not magic number.
    }

    SimpleResource(const SimpleResource &other) :
        value(other.GetValue()),
        age(0.0),
        resource_id(other.GetResourceID())
    {
      body = nullptr;
      has_body = other.has_body;
      if (has_body) {
        const emp::Circle circle(other.GetConstBody().GetConstShape());
        AttachBody(new Body_t(circle));
        body->SetMaxPressure(99999);
        body->SetMass(other.GetConstBody().GetMass());
      }
    }

    ~SimpleResource() { ; }

    double GetValue() const { return value; }
    double GetAge() const { return age; }
    int GetResourceID() const { return resource_id; }

    const emp::BitVector & GetAffinity() const { return affinity; }
    void SetAffinity(const emp::BitVector & _affinity) {
      affinity = emp::BitVector(_affinity);
      UpdateResourceID();
    }

    void SetValue(double value) { this->value = value; }
    void SetAge(double age) { this->age = age; }
    int IncAge() { return ++age; }
    //void SetColorID(int id) { emp_assert(has_body); body->SetColorID(id); }

    void Evaluate() override {
      emp::PhysicsBodyOwner_Base<Body_t>::Evaluate();
      IncAge();
    }

    void UpdateResourceID() {
      // int val = 0;
      // const int glen = (int)affinity.GetSize();
      // for (int g = 0; g < glen; ++g) {
      //   if (!affinity[g]) continue;
      //   val += emp::Pow2(g);
      // }
      int val = affinity.CountOnes();
      resource_id = val;
    }
};

#endif
