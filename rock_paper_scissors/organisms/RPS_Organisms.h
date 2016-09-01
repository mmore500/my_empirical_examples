
#ifndef RPS_ORG_H
#define RPS_ORG_H

#include "geometry/Angle2D.h"
#include "physics/Body2D.h"

enum class RPS_TYPE { ROCK, PAPER, SCISSORS };


class RPSOrg {

protected:
  using Body_t = emp::Body<emp::Circle>;
  Body_t *body;
  bool has_body;

  RPS_TYPE type;
public:
  RPSOrg(const emp::Circle &_p, RPS_TYPE t) :
    body(nullptr),
    has_body(false),
    type(t)
  {
    AttachBody(new Body_t(_p));
    body->SetMaxPressure(10);
    body->SetMass(10);
  }

  RPSOrg(const RPSOrg &other) :
    body(nullptr),
    has_body(other.has_body),
    type(other.GetType())
  {
    if (has_body) {
      AttachBody(new Body_t(other.GetConstBody().GetConstShape()));
      body->SetMaxPressure(10);
      body->SetMass(10);
    }
  }

  ~RPSOrg() { ; }

  Body_t * GetBodyPtr() { emp_assert(has_body); return body; }
  Body_t & GetBody() { emp_assert(has_body); return *body; }
  const Body_t & GetConstBody() const { emp_assert(has_body) return *body; }

  RPS_TYPE GetType() const { return type; };

  void AttachBody(Body_t * in_body) {
    body = in_body;
    has_body = true;
  }

  void DetachBody() {
    body = nullptr;
    has_body = false;
  }

};

#endif
