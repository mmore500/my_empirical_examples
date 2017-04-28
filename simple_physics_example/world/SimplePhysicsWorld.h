#include "SimpleOrganism.h"
#include "SimpleResource.h"

#include "base/vector.h"
#include "tools/BitVector.h"
#include "tools/Random.h"
#include "tools/TypeTracker.h"
#include "physics/Physics2D.h"
#include "geometry/Point2D.h"



namespace emp {
namespace evo {
  class SimplePhysicsWorld {
  protected:
    using Organism_t = SimpleOrganism;
    using Resource_t = SimpleResource;
    using Physics_t = CirclePhysics2D<Organism_t, Resource_t>;


    Physics_t physics;
    Random *random_ptr;
    emp::vector<Organism_t*> population;
    emp::vector<Resource_t*> resources;


    int cur_update;

    int max_pop_size;
    int genome_length;

  public:
    // TODO: PopulationManager_Base doesn't handle organisms just dying in the population very well
    SimplePhysicsWorld(double _w, double _h, Random *_random_ptr, double _surface_friction,
                       int _max_pop_size, int _genome_length)
    : physics(), cur_update(0), max_pop_size(_max_pop_size), genome_length(_genome_length) {
      random_ptr = _random_ptr;
      physics.ConfigPhysics(_w, _h, _random_ptr, _surface_friction);
      std::function<void(Organism_t*, Resource_t*)> fun = [this](Organism_t *org, Resource_t *res) {
        this->ResOrgCollisionHandler(org, res);
      };
      physics.RegisterCollisionHandler(fun);
    }
    ~SimplePhysicsWorld() {
      Clear();
    }

    void Clear() {
      physics.Clear();
      for (auto *org : population) delete org;
      for (auto *res : resources) delete res;
      population.resize(0);
      resources.resize(0);
    }

    void Reset() {
      Clear();
      cur_update = 0;
    }

    int GetCurrentUpdate() const { return cur_update; }
    int GetPopulationSize() const { return (int)population.size(); }
    int GetResourceCnt() const { return (int)resources.size(); }
    double GetWidth() const { return physics.GetWidth(); }
    double GetHeight() const { return physics.GetHeight(); }

    const emp::vector<Organism_t*> GetConstPopulation() const { return population; }
    const emp::vector<Resource_t*> GetConstResources() const { return resources; }

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

    // TODO: Res, dispenser collision handler
    // TODO: Org, dispenser collision handler

    void Update() {
      // Progress physics by one time step.
      physics.Update();
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
        resource->GetBody().IncVelocity(Angle(random_ptr->GetDouble() * (2.0 * emp::PI)).GetPoint(0.1));
        ++cur_id;
      }
      resources.resize(cur_size);
      // Pump in new resources.
      for (int i = 0; (i < 5) && GetResourceCnt() < 50; ++i) {
        int radius = 5;
        Point res_loc(random_ptr->GetDouble(radius, physics.GetWidth() - radius),
                      random_ptr->GetDouble(radius, physics.GetHeight() - radius));
        Resource_t *new_resource = new Resource_t(Circle(res_loc, radius));
        emp::BitVector aff = emp::BitVector(genome_length, 1);
        new_resource->SetAffinity(aff);
        AddResource(new_resource);
      }
      // Manage population.
      cur_size = GetPopulationSize();
      cur_id = 0;
      while (cur_id < cur_size) {
        Organism_t *org = population[cur_id];
        // Evaluate organism.
        org->Evaluate();
        // Movement noise.
        org->GetBody().IncVelocity(Angle(random_ptr->GetDouble() * (2.0 * emp::PI)).GetPoint(0.1));
        ++cur_id;
      }
      population.resize(cur_size);
      // Add new organisms.
      ++cur_update;
    }
  };
}
}
