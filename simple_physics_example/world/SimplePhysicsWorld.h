#include "SimpleOrganism.h"
#include "SimpleResource.h"
#include "SimpleResourceDispenser.h"

#include "base/vector.h"
#include "tools/BitVector.h"
#include "tools/Random.h"
#include "tools/random_utils.h"
#include "tools/TypeTracker.h"
#include "physics/Physics2D.h"
#include "geometry/Point2D.h"



namespace emp {
namespace evo {
  class SimplePhysicsWorld {
  protected:
    using Organism_t = SimpleOrganism;
    using Resource_t = SimpleResource;
    using Dispenser_t = SimpleResourceDispenser;
    using Physics_t = CirclePhysics2D<Organism_t, Resource_t, Dispenser_t>;


    Physics_t physics;
    Random *random_ptr;
    emp::vector<Organism_t*> population;
    emp::vector<Resource_t*> resources;
    emp::vector<Dispenser_t*> dispensers;


    int cur_update;
    // ---- Parameters ----
    // Population specific
    int max_pop_size;
    // Organism specific
    int genome_length;
    double cost_of_repro;
    double point_mutation_rate;

    double resource_value;
    int max_resource_age;

  public:
    // TODO: PopulationManager_Base doesn't handle organisms just dying in the population very well
    SimplePhysicsWorld(double _w, double _h, Random *_random_ptr, double _surface_friction,
                       int _max_pop_size, int _genome_length, double _cost_of_repro, double _resource_value,
                       int _max_resource_age)
    : physics(), cur_update(0), max_pop_size(_max_pop_size), genome_length(_genome_length),
      cost_of_repro(_cost_of_repro), resource_value(_resource_value), max_resource_age(_max_resource_age)
    {
      random_ptr = _random_ptr;
      physics.ConfigPhysics(_w, _h, _random_ptr, _surface_friction);

      std::function<void(Organism_t*, Resource_t*)> fun0 = [this](Organism_t *org, Resource_t *res) {
        this->ResOrgCollisionHandler(org, res);
      };
      std::function<void(Dispenser_t*, Resource_t*)> fun1 = [this](Dispenser_t *disp, Resource_t *res) {
        this->DispCollisionHandler(disp, res->GetBodyPtr());
      };
      std::function<void(Dispenser_t*, Organism_t*)> fun2 = [this](Dispenser_t *disp, Organism_t *org) {
        this->DispCollisionHandler(disp, org->GetBodyPtr());
      };
      // todo: add dispenser vs dispenser

      physics.RegisterCollisionHandler(fun0);
      physics.RegisterCollisionHandler(fun1);
      physics.RegisterCollisionHandler(fun2);
    }
    ~SimplePhysicsWorld() {
      Clear();
    }

    void Clear() {
      physics.Clear();
      for (auto *org : population) delete org;
      for (auto *res : resources) delete res;
      for (auto *dis : dispensers) delete dis;
      population.resize(0);
      resources.resize(0);
      dispensers.resize(0);
    }

    void Reset() {
      Clear();
      cur_update = 0;
    }

    int GetCurrentUpdate() const { return cur_update; }
    int GetPopulationSize() const { return (int)population.size(); }
    int GetResourceCnt() const { return (int)resources.size(); }
    int GetDispenserCnt() const { return (int)dispensers.size(); }
    double GetWidth() const { return physics.GetWidth(); }
    double GetHeight() const { return physics.GetHeight(); }

    const emp::vector<Organism_t*> GetConstPopulation() const { return population; }
    const emp::vector<Resource_t*> GetConstResources() const { return resources; }
    const emp::vector<Dispenser_t*> GetConstDispensers() const { return dispensers; }

    // TODO: At the moment, totally ignores POpulationManager_Base stuff. Does not update fitness manager.
    int AddOrg(Organism_t *new_org) {
      int pos = (int)population.size();
      population.push_back(new_org);
      physics.AddBody(new_org);
      return pos;
    }

    int AddResource(Resource_t *new_resource) {
      int pos = GetResourceCnt();
      resources.push_back(new_resource);
      physics.AddBody(new_resource);
      return pos;
    }

    int AddDispenser(Dispenser_t *new_dispenser) {
      int pos = GetDispenserCnt();
      // Register dispenser callback with this dispenser.
      std::function<void(Dispenser_t*)> fun = [this](Dispenser_t *disp) {
        this->DispenseCallback(disp);
      };
      new_dispenser->RegisterDispenserTimerCallback(fun);
      dispensers.push_back(new_dispenser);
      physics.AddBody(new_dispenser);
      return pos;
    }

    void DispenseCallback(Dispenser_t *dispenser) {
      emp::vector<Resource_t*> new_resources = dispenser->Dispense(random_ptr);
      for (auto *res : new_resources) {
        AddResource(res);
      }
    }

    void ResOrgCollisionHandler(Organism_t *org, Resource_t *res) {
      using Body_t = PhysicsBody2D<Circle>;
      Body_t *org_body = org->GetBodyPtr();
      Body_t *res_body = res->GetBodyPtr();
      // If organism and resource collide, link with a CONSUME link.
      if (!org_body->IsLinked(*res_body)) {
        double strength;
        const double sq_pair_dist = (org_body->GetShape().GetCenter() - res_body->GetShape().GetCenter()).SquareMagnitude();
        const double radius_sum = org_body->GetShape().GetRadius() + res_body->GetShape().GetRadius();
        const double sq_min_dist = radius_sum * radius_sum;
        // Strength is a function of how close the two organisms are.
        sq_pair_dist == 0.0 ? strength = std::numeric_limits<double>::max() : strength = sq_min_dist / sq_pair_dist;
        org_body->AddLink(BODY_LINK_TYPE::CONSUME_RESOURCE, *res_body, sqrt(sq_pair_dist), sqrt(sq_min_dist), strength);
      }
      org_body->ResolveCollision();
      res_body->ResolveCollision();
    }

    void DispCollisionHandler(Dispenser_t *disp, PhysicsBody2D<Circle> *other_body) {
      using Body_t = PhysicsBody2D<Circle>;
      Body_t *disp_body = disp->GetBodyPtr();
      // disp_body = 1, other_body = 2
      disp_body->ResolveCollision();
      other_body->ResolveCollision();
    }

    void Update() {
      // Progress physics by one time step.
      physics.Update();
      emp::vector<Organism_t*> new_organisms;
      // Manage resources.
      int cur_size = GetResourceCnt();
      int cur_id = 0;
      while (cur_id < cur_size) {
        Resource_t *resource = resources[cur_id];
        // Evaluate.
        resource->Evaluate();
        // Handle resource consumption.
        auto consumption_links = resource->GetBody().GetLinksToByType(BODY_LINK_TYPE::CONSUME_RESOURCE);
        if ((int)consumption_links.size() > 0) {
          // Find the strongest link!
          auto *max_link = consumption_links[0];
          for (auto *link : consumption_links) {
            if (link->link_strength > max_link->link_strength) max_link = link;
          }
          // Feed resource to strongest link.
          if (physics.template IsBodyOwnerType<Organism_t>((PhysicsBody2D<Circle>*)max_link->from)) {
            Organism_t *org = physics.template ToBodyOwnerType<Organism_t>((PhysicsBody2D<Circle>*)max_link->from);
            org->ConsumeResource(*resource);
            delete resource;
            cur_size--;
            resources[cur_id] = resources[cur_size];
            continue;
          }
        }
        // TODO: Remove resources flagged for removal.
        // Check on resource aging.
        if (resource->GetAge() > max_resource_age) {
          delete resource;
          cur_size--;
          resources[cur_id] = resources[cur_size];
          continue;
        }
        resource->GetBody().IncVelocity(Angle(random_ptr->GetDouble() * (2.0 * emp::PI)).GetPoint(0.1));
        ++cur_id;
      }
      resources.resize(cur_size);

      // Update dispensers.
      for (auto *disp : dispensers) {
        disp->Evaluate();
      }
      // Manage population.
      cur_size = GetPopulationSize();
      cur_id = 0;
      while (cur_id < cur_size) {
        Organism_t *org = population[cur_id];
        // Evaluate organism.
        org->Evaluate();
        // Reproduction?
        if (org->GetEnergy() >= cost_of_repro) {
          //auto *offspring = ;
          new_organisms.push_back(org->Reproduce(random_ptr, 0.1, cost_of_repro));
        }
        // Movement noise.
        org->GetBody().IncVelocity(Angle(random_ptr->GetDouble() * (2.0 * emp::PI)).GetPoint(0.01));
        ++cur_id;
      }
      population.resize(cur_size);
      // Cull the population if necessary.
      int total_size = (int)(population.size() + new_organisms.size());
      if (total_size > 200) {
        // Cull population to make room for new organisms.
        int new_size = (int)population.size() - (total_size - 200);
        emp::Shuffle<Organism_t *>(*random_ptr, population, new_size);
        for (int i = new_size; i < (int)population.size(); i++) delete population[i];
        population.resize(new_size);
      }
      // Add new organisms.
      for (auto *offspring : new_organisms) AddOrg(offspring);
      ++cur_update;
    }
  };
}
}
