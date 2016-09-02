
#ifndef RPS_ORG_H
#define RPS_ORG_H

#include "geometry/Angle2D.h"
#include "physics/Body2D.h"
#include "physics/BodyOwner2D.h"
#include "tools/Random.h"

enum class RPS_TYPE { ROCK, PAPER, SCISSORS };


class RPSOrg : public emp::BodyOwner_Base<emp::Body<emp::Circle>> {

protected:
  using Body_t = emp::Body<emp::Circle>;
  using emp::BodyOwner_Base<Body_t>::body;
  using emp::BodyOwner_Base<Body_t>::has_body;

  double energy;

  RPS_TYPE type;
public:
  RPSOrg(const emp::Circle &_p, RPS_TYPE t) :
    energy(0),
    type(t)
  {
    AttachBody(new Body_t(_p));
    body->SetMaxPressure(10);
    body->SetMass(10);
  }

  RPSOrg(const RPSOrg &other) :
    energy(0),
    type(other.GetType())
  {
    has_body = other.has_body;
    if (has_body) {
      AttachBody(new Body_t(other.GetConstBody().GetConstShape()));
      body->SetMaxPressure(10);
      body->SetMass(10);
    }
  }

  ~RPSOrg() { ; }

  RPS_TYPE GetType() const { return type; }
  double GetEnergy() const { return energy; }

  void IncEnergy(double e = 1.0) { energy += e; }

  RPSOrg * Reproduce(emp::Random *r, double mut_rate = 0.0, double cost = 0.0) {
    energy -= cost;
    // Build offspring
    auto *offspring = new RPSOrg(*this);
    offspring->energy = 0;
    // Mutate offspring
    if (r->P(mut_rate)) {
      int s = r->GetInt(2);
      switch (type) {
        case RPS_TYPE::ROCK:
          s == 0 ? offspring->type = RPS_TYPE::PAPER : offspring->type = RPS_TYPE::SCISSORS;
          break;
        case RPS_TYPE::PAPER:
          s == 0 ? offspring->type = RPS_TYPE::ROCK : offspring->type = RPS_TYPE::SCISSORS;
          break;
        case RPS_TYPE::SCISSORS:
          s == 0 ? offspring->type = RPS_TYPE::PAPER : offspring->type = RPS_TYPE::ROCK;
          break;
      }
    }
    // Link and nudge.
    emp::Angle repro_angle(r->GetDouble(2.0 * emp::PI)); // What angle should we put the offspring at?
    auto offset = repro_angle.GetPoint(offspring->GetBodyPtr()->GetShapePtr()->GetRadius() + body->GetShapePtr()->GetRadius() + 5);
    //body->AddLink(emp::BODY_LINK_TYPE::REPRODUCTION, offspring->GetBody(), offset.Magnitude(), body->GetShapePtr()->GetRadius() * 2);
    offspring->GetBodyPtr()->GetShapePtr()->Translate(offset);
    return offspring;
  }

};

#endif
