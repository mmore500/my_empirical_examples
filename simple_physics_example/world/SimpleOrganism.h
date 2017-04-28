/*
  organisms/SimpleOrganism.h
*/

#ifndef SIMPLEORGANISM_H
#define SIMPLEORGANISM_H

#include "tools/BitVector.h"
#include "tools/Random.h"
#include "tools/math.h"

#include "geometry/Angle2D.h"
#include "geometry/Circle2D.h"
#include "physics/PhysicsBody2D.h"
#include "physics/PhysicsBodyOwner.h"
#include "SimpleResource.h"

class SimpleOrganism : public emp::PhysicsBodyOwner_Base<emp::PhysicsBody2D<emp::Circle>> {
  private:
    using Body_t = emp::PhysicsBody2D<emp::Circle>;
    using emp::PhysicsBodyOwner_Base<Body_t>::body;
    using emp::PhysicsBodyOwner_Base<Body_t>::has_body;

    int offspring_count;
    double birth_time;
    double energy;
    int resources_collected;
    bool detach_on_birth;
    int genome_id;

  public:
    emp::BitVector genome;
    // TODO: use argument forwarding to be able to create body when making organism!
    SimpleOrganism(const emp::Circle &_p, int genome_length = 1, bool detach_on_birth = true)
      : offspring_count(0),
        birth_time(0.0),
        energy(0.0),
        resources_collected(0.0),
        detach_on_birth(detach_on_birth),
        genome(genome_length, false)
    {
      UpdateGenomeID();
      body = nullptr;
      has_body = false;
      AttachBody(new Body_t(_p));
      body->RegisterOnLinkUpdateCallback([this](emp::BodyLink * link) { this->OnBodyLinkUpdate(link); });
      body->SetMass(10); // TODO: make this not a magic number.
    }

    SimpleOrganism(const SimpleOrganism &other)
       : offspring_count(other.GetOffspringCount()),
         birth_time(other.GetBirthTime()),
         energy(other.GetEnergy()),
         resources_collected(other.GetResourcesCollected()),
         detach_on_birth(other.GetDetachOnBirth()),
         genome_id(other.GetGenomeID()),
         genome(other.genome)
    {
      body = nullptr;
      has_body = other.has_body;
      if (has_body) {
        AttachBody(new Body_t(other.GetConstBody().GetConstShape()));
        body->RegisterOnLinkUpdateCallback([this](emp::BodyLink * link) { this->OnBodyLinkUpdate(link); });
        body->SetMass(other.GetConstBody().GetMass());
        //SetColorID();
      }
    }

    ~SimpleOrganism() { ; }

    int GetOffspringCount() const { return offspring_count; }
    double GetEnergy() const { return energy; }
    int GetResourcesCollected() const { return resources_collected; }
    double GetBirthTime() const { return birth_time; }
    bool GetDetachOnBirth() const { return detach_on_birth; }
    int GetGenomeID() const { return genome_id; }

    void Evaluate() override {
      // Required: Be sure to call BodyOwner_Base evaluate.
      emp::PhysicsBodyOwner_Base<Body_t>::Evaluate();
      // - This is where things that organism should do every update go -
    }

    void OnCollision(emp::PhysicsBody2D_Base * other_body) {
      // Here is where an organism could handle a collision.
    }

    void OnBodyLinkUpdate(emp::BodyLink * link) {
      // What to do with reproduction links
      if (link->type == emp::BODY_LINK_TYPE::REPRODUCTION && detach_on_birth) {
        if (link->cur_dist >= link->target_dist) link->destroy = true;
      }
    }

    void ConsumeResource(const SimpleResource &resource) {
      //  * Calculate resource affinity (matches)
      //  * Able to digest resource.GetValue() * (match score / max match score)
      // (resource.Affinity & genome).CountOnes() + (~resource.Affinity & ~genome).CountOnes()
      const double max_score = (double)genome.GetSize();
      const double score = (resource.GetAffinity() & genome).CountOnes() + (~resource.GetAffinity() & ~genome).CountOnes();
      energy += (score / max_score) * resource.GetValue();
      resources_collected++;
    }

    void SetDetachOnBirth(bool detach) { detach_on_birth = detach; }
    void SetEnergy(double e) { energy = e; }
    void SetBirthTime(double t) { birth_time = t; }

    SimpleOrganism * Reproduce(emp::Random *r, double mut_rate = 0.0, double cost = 0.0) {
      energy -= cost;
      // Build offspring
      auto *offspring = new SimpleOrganism(*this);
      offspring->Reset();
      // Mutate offspring
      for (int i = 0; i < offspring->genome.GetSize(); i++) {
        if (r->P(mut_rate)) offspring->genome[i] = !offspring->genome[i];
      }
      offspring->UpdateGenomeID();
      // Link and nudge. offspring
      emp::Angle repro_angle(r->GetDouble(2.0 * emp::PI)); // What angle should we put the offspring at?
      auto offset = repro_angle.GetPoint(0.1);
      body->AddLink(emp::BODY_LINK_TYPE::REPRODUCTION, offspring->GetBody(), offset.Magnitude(), body->GetShapePtr()->GetRadius() * 2);
      offspring->GetBodyPtr()->GetShapePtr()->Translate(offset);
      offspring_count++;
      return offspring;
    }

    void UpdateGenomeID() {
      // int val = 0;
      // const int glen = (int)genome.GetSize();
      // for (int g = 0; g < glen; ++g) {
      //   if (!genome[g]) continue;
      //   val += emp::Pow2(g);
      // }
      int val = genome.CountOnes();
      genome_id = val;
    }

    void Reset() {
      energy = 0.0;
      resources_collected = 0;
      offspring_count = 0;
    }
};

#endif
