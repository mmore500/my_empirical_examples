/*
  organisms/SimpleOrganism.h
*/

#ifndef SIMPLEORGANISM_H
#define SIMPLEORGANISM_H

#include "tools/BitVector.h"
#include "tools/Random.h"

#include "geometry/Angle2D.h"
#include "physics/Body2D.h"
#include "../resources/SimpleResource.h"

class SimpleOrganism {
  using Body_t = emp::Body<emp::Circle>;
  private:
    Body_t *body;
    int offspring_count;
    double birth_time;
    double membrane_strengh;  // How much pressure able to withstand before popping? TODO: should this be stored in body?
    bool has_body;
    double energy;
    int resources_collected;
    bool detach_on_birth;

  public:
    emp::BitVector genome;
    // TODO: use argument forwarding to be able to create body when making organism!
    SimpleOrganism(const emp::Circle &_p, int genome_length = 1, bool detach_on_birth = true)
      : body(nullptr),
        offspring_count(0),
        birth_time(0.0),
        membrane_strengh(1.0),
        has_body(false),
        energy(0.0),
        resources_collected(0.0),
        detach_on_birth(detach_on_birth),
        genome(genome_length, false)
    {
      AttachBody(new Body_t(_p));
      body->RegisterOnLinkUpdateCallback([this](emp::BodyLink * link) { this->OnBodyLinkUpdate(link); });
      body->SetMaxPressure(membrane_strengh);
      body->SetMass(10); // TODO: make this not a magic number.
      SetColorID();
    }

    // At the moment does not copy a body over.
    SimpleOrganism(const SimpleOrganism &other)
       : body(nullptr),
         offspring_count(other.GetOffspringCount()),
         birth_time(other.GetBirthTime()),
         membrane_strengh(other.GetMembraneStrength()),
         has_body(other.has_body),
         energy(other.GetEnergy()),
         resources_collected(other.GetResourcesCollected()),
         detach_on_birth(other.GetDetachOnBirth()),
         genome(other.genome)
    {
      if (has_body) {
        AttachBody(new Body_t(other.GetConstBody().GetConstShape()));
        body->RegisterOnLinkUpdateCallback([this](emp::BodyLink * link) { this->OnBodyLinkUpdate(link); });
        body->SetMaxPressure(membrane_strengh);
        body->SetMass(other.GetConstBody().GetMass());
        SetColorID();
      }
    }

    ~SimpleOrganism() {
      if (has_body) {
        delete body;
        DetachBody();
      }
    }

    int GetOffspringCount() const { return offspring_count; }
    double GetEnergy() const { return energy; }
    int GetResourcesCollected() const { return resources_collected; }
    double GetBirthTime() const { return birth_time; }
    bool GetDetachOnBirth() const { return detach_on_birth; }
    double GetMembraneStrength() const { return membrane_strengh; }
    Body_t * GetBodyPtr() { emp_assert(has_body); return body; }
    Body_t & GetBody() { emp_assert(has_body); return *body; }
    const Body_t & GetConstBody() const { emp_assert(has_body) return *body; }
    bool HasBody() const { return has_body; }
    void FlagBodyDestruction() { has_body = false; }
    double GetResourceConsumptionProb(const SimpleResource &resource) {
      if (genome.GetSize() == 0) return 1.0;
      else return genome.CountOnes() / (double) genome.GetSize();
    }

    void AttachBody(Body_t * in_body) {
      body = in_body;
      has_body = true;
    }

    void DetachBody() {
      body = nullptr;
      has_body = false;
    }

    void Evaluate() {
      if (body->GetDestroyFlag()) {
        delete body;
        DetachBody();
      }
    }

    void OnCollision(emp::Body2D_Base * other_body) {
      // Here is where an organism could handle a collision.
    }

    void OnBodyLinkUpdate(emp::BodyLink * link) {
      // What to do with reproduction links
      if (link->type == emp::BODY_LINK_TYPE::REPRODUCTION) {
        if (link->cur_dist >= link->target_dist) link->flag_for_removal = true;
      }
    }

    void ConsumeResource(const SimpleResource &resource) {
      energy += resource.GetValue();
      resources_collected++;
    }

    void SetDetachOnBirth(bool detach) { detach_on_birth = detach; }
    void SetMembraneStrength(double strength) {
      membrane_strengh = strength;
      if (has_body) body->SetMaxPressure(membrane_strengh);
    }
    void SetEnergy(double e) { energy = e; }
    void SetBirthTime(double t) { birth_time = t; }
    void SetColorID(int id) { emp_assert(has_body); body->SetColorID(id); }
    void SetColorID() {
      emp_assert(has_body);
      if (genome.GetSize() > 0) body->SetColorID((genome.CountOnes() / (double) genome.GetSize()) * 200);
      else body->SetColorID(0);
    }

    SimpleOrganism * Reproduce(emp::Random *r, double mut_rate = 0.0, double cost = 0.0) {
      energy -= cost;
      // Build offspring
      auto *offspring = new SimpleOrganism(*this);
      offspring->Reset();
      // Mutate offspring
      for (int i = 0; i < offspring->genome.GetSize(); i++) {
        if (r->P(mut_rate)) offspring->genome[i] = !offspring->genome[i];
      }
      // Link and nudge. offspring
      emp::Angle repro_angle(r->GetDouble(2.0 * emp::PI)); // What angle should we put the offspring at?
      auto offset = repro_angle.GetPoint(0.1);
      body->AddLink(emp::BODY_LINK_TYPE::REPRODUCTION, offspring->GetBody(), offset.Magnitude(), body->GetShapePtr()->GetRadius() * 2);
      offspring->GetBodyPtr()->GetShapePtr()->Translate(offset);
      offspring_count++;
      return offspring;
    }

    void Reset() {
      energy = 0.0;
      resources_collected = 0;
      offspring_count = 0;
    }

    bool operator==(const SimpleOrganism &other) const {
      /* Do these organisms have the same genotype? */
      return this->genome == other.genome;
    }

    bool operator<(const SimpleOrganism &other) const {
      return this->genome < other.genome;
    }

    bool operator>(const SimpleOrganism &other) const {
      return this->genome > other.genome;
    }

    bool operator!=(const SimpleOrganism &other) const {
      return this->genome != other.genome;
    }

    bool operator>=(const SimpleOrganism &other) const {
      return !this->operator<(other);
    }

    bool operator<=(const SimpleOrganism &other) const {
      return !this->operator>(other);
    }
};

#endif
