amespace emp {
namespace evo {
  // TODO: WTF about these template arguments? Why can't I NOT specify fitness manager, etc?
  template <typename ORG=SimpleOrganism, typename FIT_MANAGER=int>
  class PopulationManager_CirclePhysics : public PopulationManager_Base<ORG, FIT_MANAGER> {
  protected:
    using base_t = PopulationManager_Base<ORG, FIT_MANAGER>;
    using Organism_t = SimpleOrganism;
    using Resource_t = SimpleResource;
    using Physics_t = CirclePhysics2D<Organism_t, Resource_t>;

    using base_t::random_ptr;
    using base_t::pop;

    Physics_t physics;
    emp::vector<Resource_t> resources;

  public:
    // TODO: PopulationManager_Base doesn't handle organisms just dying in the population very well
    PopulationManager_CirclePhysics(const std::string _w_name, FIT_MANAGER &_fm)
    : base_t(_w_name, _fm), physics() {
    }
    ~PopulationManager_CirclePhysics() { ; }

    void Clear() {
      physics.Clear();
      base_t::Clear();
    }

    void ConfigPop(double _w, double _h, double _surface_friction) {
      physics.ConfigPhysics(_w, _h, random_ptr, _surface_friction);
    }

    // TODO: At the moment, totally ignores POpulationManager_Base stuff. Does not update fitness manager.
    size_t AddOrg(Organism_t *new_org) {
      size_t pos = base_t::GetSize();
      pop.push_back(new_org);
      physics.AddBody(new_org);
      return pos;
    }

    void Update() {
      // Progress physics by one time step.
      physics.Update();

      // Manage resources.

      // Manage population.

      // Add new organisms.
    }

  };
  using PopCirclePhysics = PopulationManager_CirclePhysics<SimpleOrganism>;
}
}
