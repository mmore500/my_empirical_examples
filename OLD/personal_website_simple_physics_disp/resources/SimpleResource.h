/*
  resources/SimpleResource.h
    Defines the SimpleResource class.
*/

#ifndef SIMPLERESOURCE_H
#define SIMPLERESOURCE_H

#include "physics/Body2D.h"


class SimpleResource : public emp::BodyOwner_Base<emp::Body<emp::Circle>> {

  private:
    using Body_t = emp::Body<emp::Circle>;
    using emp::BodyOwner_Base<Body_t>::body;
    using emp::BodyOwner_Base<Body_t>::has_body;
    double value;
    double age;

  public:
    SimpleResource(const emp::Circle &_p, double value = 1.0) :
      age(0.0)
    {
      body = nullptr;
      has_body = false;
      AttachBody(new Body_t(_p));
      body->SetMaxPressure(99999); // Big number.
      body->SetMass(5); // TODO: make this not magic number.
      this->value = value;
    }

    SimpleResource(const SimpleResource &other) :
        value(other.GetValue()),
        age(0.0)
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

    void SetValue(double value) { this->value = value; }
    void SetAge(double age) { this->age = age; }
    int IncAge() { return ++age; }
    void SetColorID(int id) { emp_assert(has_body); body->SetColorID(id); }

    void Evaluate() override {
      emp::BodyOwner_Base<Body_t>::Evaluate();
      IncAge();
    }

};

#endif
