/*
  resources/SimpleResource.h
    Defines the SimpleResource class.
*/

#ifndef SIMPLERESOURCE_H
#define SIMPLERESOURCE_H

#include "physics/Body2D.h"


class SimpleResource {
  using Body_t = emp::Body<emp::Circle>;
  private:
    Body_t* body;                     // An organism/resource's body may be deleted by outside forces.
    double value;
    double age;
    bool has_body;

  public:
    SimpleResource(const emp::Circle &_p, double value = 1.0) :
      age(0.0),
      has_body(false)
    {
      AttachBody(new Body_t(_p));
      //body->SetDetachOnRepro(true);
      body->SetMaxPressure(99999); // Big number.
      body->SetMass(5); // TODO: make this not magic number.
      this->value = value;
    }

    SimpleResource(const SimpleResource &other) :
        value(other.GetValue()),
        age(0.0),
        has_body(other.HasBody())
    {
      if (has_body) {
        const emp::Circle circle(other.GetConstBody().GetConstShape());
        AttachBody(new Body_t(circle));
        //body->SetDetachOnRepro(other.GetConstBody().GetDetachOnRepro());
        body->SetMaxPressure(99999);
        body->SetMass(other.GetConstBody().GetMass());
      }
    }

    ~SimpleResource() {
      if (has_body) {
        delete body;
        DetachBody();
      }
    }

    double GetValue() const { return value; }
    double GetAge() const { return age; }
    Body_t * GetBodyPtr() { emp_assert(body); return body; }
    Body_t & GetBody() { emp_assert(body); return *body; }
    const Body_t & GetConstBody() const { emp_assert(has_body); return *body; }
    bool HasBody() const { return has_body; }
    void FlagBodyDestruction() { has_body = false; }

    void SetValue(double value) { this->value = value; }
    void SetAge(double age) { this->age = age; }
    int IncAge() { return ++age; }
    void SetColorID(int id) { emp_assert(has_body); body->SetColorID(id); }

    // TODO: should be able to point body to THIS as owner.
    // (These things need access to the lookup table that currently sits in the physics.)
    void AttachBody(Body_t * in_body) {
        body = in_body;
        has_body = true;
    }

    void DetachBody() {
      body = nullptr;
      has_body = false;
    }

    // TODO: how do we want to handle this? At the very least, this is not a great function name.
    //  Probably want to change over to Update() function or Execute() function.
    // DOC: Called once per population update.
    void Evaluate() {
      if (body->GetDestroyFlag()) {
        delete body;
        DetachBody();
      }
      IncAge();
    }

    // operator overloads
    bool operator==(const SimpleResource &other) const {
      /* Do these organisms have the same genotype? */
      return this->GetValue() == other.GetValue();
    }

    bool operator<(const SimpleResource &other) const {
      return this->GetValue() < other.GetValue();
    }

    bool operator>(const SimpleResource &other) const {
      return this->GetValue() > other.GetValue();
    }

    bool operator!=(const SimpleResource &other) const {
      return this->GetValue() != other.GetValue();
    }

    bool operator>=(const SimpleResource &other) const {
      return !this->operator<(other);
    }

    bool operator<=(const SimpleResource &other) const {
      return !this->operator>(other);
    }
};

#endif
