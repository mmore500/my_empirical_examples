
#ifndef POPULATION_MANAGER_RPS_H
#define POPULATION_MANAGER_RPS_H

#include <iostream>
#include <limits>

#include "physics/Physics2D.h"

#include "evo/PopulationManager.h"
#include "tools/vector.h"
#include "tools/random_utils.h"

namespace emp {
namespace evo {

template <typename ORG>
class PopulationManager_RPS {
protected:
  using Organism_t = ORG;
  using Physics_t = CirclePhysics2D<RPSOrg>;

  Physics_t physics;
  emp::vector<Organism_t*> population;

  Random *random_ptr;

  int num_papers;
  int num_rocks;
  int num_scissors;

  // Population manager parameters.
  double type_mutation_rate;
  double cost_of_repro;
  double movement_noise;

  void CountObject(Organism_t * org) {
    // Count.
    switch(org->GetType()) {
      case(RPS_TYPE::ROCK):
        num_rocks++;
        break;
      case(RPS_TYPE::PAPER):
        num_papers++;
        break;
      case(RPS_TYPE::SCISSORS):
        num_scissors++;
        break;
    }
  }

public:

  PopulationManager_RPS() :
    physics(),
    num_papers(0),
    num_rocks(0),
    num_scissors(0),
    type_mutation_rate(0.0075),
    cost_of_repro(1.0),
    movement_noise(0.1)
  {
    // Register a collision handler with the physics.
    std::function<void(RPSOrg *, RPSOrg *)> f =
      [this](RPSOrg *org1, RPSOrg *org2) {
        this->RPSCollisionHandler(org1, org2);
      };
    physics.RegisterCollisionHandler(f);
  }

  // Allow this and derived classes to be identified as a population manager:
  static constexpr bool emp_is_population_manager = true;
  static constexpr bool emp_has_separate_generations = false;

  // Setup iterator for the population.
  friend class PopulationIterator<PopulationManager_RPS<Organism_t> >;
  using iterator = PopulationIterator<PopulationManager_RPS<Organism_t> >;
  // Operator overloading.
  Organism_t* & operator[](int i) { return population[i]; }

  iterator begin() { return iterator(this, 0); }
  iterator end() { return iterator(this, population.size()); }

  uint32_t size() const { return population.size(); }
  int GetSize() const { return (int) this->size(); }
  Physics_t & GetPhysics() { return physics; }
  int GetNumPapers() const { return num_papers; }
  int GetNumRocks() const { return num_rocks; }
  int GetNumScissors() const { return num_scissors; }

  int AddOrg(Organism_t *new_org) {
    int pos = this->GetSize();
    population.push_back(new_org);
    physics.AddBody(new_org);
    return pos;
  }

  void Setup(Random *r) { random_ptr = r; }

  void Clear() {
    physics.Clear();
    population.clear();
  }

  void ConfigPop(double width, double height, double surface_friction,
                 double type_mutation_rate, double cost_of_repro, double movement_noise) {
    this->type_mutation_rate = type_mutation_rate;
    this->cost_of_repro = cost_of_repro;
    this->movement_noise = movement_noise;
    physics.ConfigPhysics(width, height, random_ptr, surface_friction);
  }

  void RPSCollisionHandler(RPSOrg *org1, RPSOrg *org2) {
    using OrgBody_t = Body<Circle>;
    OrgBody_t *body1 = org1->GetBodyPtr();
    OrgBody_t *body2 = org2->GetBodyPtr();
    if (org1->GetType() == org2->GetType()) return;
    // Things are eaten by (int) type + 1 % 3, things eat (int) type + 2 % 3
    double strength;
    const double sq_pair_dist = (body1->GetShape().GetCenter() - body2->GetShape().GetCenter()).SquareMagnitude();
    const double radius_sum = body1->GetShape().GetRadius() + body2->GetShape().GetRadius();
    const double sq_min_dist = radius_sum * radius_sum;
    sq_pair_dist == 0.0 ? strength = std::numeric_limits<double>::max() : strength = sq_min_dist / sq_pair_dist;
    // Use assumptions about enum class to determine who eats who.
    int decision = (((int)org1->GetType()) + 1) % 3;
    if ( decision == (int) org2->GetType()) {
      // Org2 eats org1.
      body2->AddLink(BODY_LINK_TYPE::CONSUME_RESOURCE, *body1, sqrt(sq_pair_dist), sqrt(sq_min_dist), strength);
    } else {
      // Org1 eats org2.
      body1->AddLink(BODY_LINK_TYPE::CONSUME_RESOURCE, *body2, sqrt(sq_pair_dist), sqrt(sq_min_dist), strength);
    }
    // If resource was consumed, mark collision as resolved.
    body1->ResolveCollision();
    body2->ResolveCollision();
  }

  void Update() {
    physics.Update();
    // A place for new organisms.
    emp::vector<Organism_t*> new_organisms;
    int cur_size = GetSize();
    int cur_id = 0;

    num_papers = 0;
    num_scissors = 0;
    num_rocks = 0;
    while (cur_id < cur_size) {
      Organism_t *org = population[cur_id];
      // Evaluate.
      org->Evaluate();
      if (!org->HasBody()) {
        delete org;
        cur_size--;
        population[cur_id] = population[cur_size];
        continue;
      }
      // Org has a body, proceed.
      // Handle consumption.
      auto consumption_links = org->GetBodyPtr()->GetLinksToByType(BODY_LINK_TYPE::CONSUME_RESOURCE);
      if ((int) consumption_links.size() > 0) {
        // Find the strongest link!
        auto *max_link = consumption_links[0];
        for (auto *link : consumption_links) {
          if (link->link_strength > max_link->link_strength) max_link = link;
        }
        // Feed organiism to the strongest link.
        if (physics.template IsBodyOwnerType<RPSOrg>((Body<Circle>*)max_link->from)) {
          RPSOrg *eater_org = physics.template ToBodyOwnerType<RPSOrg>((Body<Circle>*)max_link->from);
          eater_org->IncEnergy();
          delete org;
          cur_size--;
          population[cur_id] = population[cur_size];
          continue;
        } // TODO: else { delete all links }
      }
      CountObject(org);
      // Reproduction. Organisms that have sufficient energy and are not too stressed may reproduce.
      if (!org->GetBodyPtr()->ExceedsStressThreshold() && org->GetEnergy() >= cost_of_repro) {
        auto *baby_org = org->Reproduce(random_ptr, type_mutation_rate, cost_of_repro);
        new_organisms.push_back(baby_org);
        CountObject(baby_org);
      }
      // Add some noise to movement.
      org->GetBody().IncSpeed(Angle(random_ptr->GetDouble() * (2.0 * emp::PI)).GetPoint(0.15));
      cur_id++;
    }
    population.resize(cur_size);
    for (auto *new_organism : new_organisms) AddOrg(new_organism);
  }
};

}
}

#endif
