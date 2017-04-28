/*
  simple_physics_example.cc

  Third iteration of learning physics in Empirical.
  Goal:
    * Everything from EvolveInPhysicsPt1
    * Sensors
    * Actuators
    * Parameter configuration from webpage.
*/

#include <iostream>
#include <string>
#include <sstream>

#include "./geometry/Point2D.h"
#include "./organisms/SimpleOrganism.h"
#include "./resources/SimpleResource.h"
#include "./population-managers/PopulationManager_SimplePhysics.h"

#include "web/web.h"
#include "web/Document.h"
#include "web/Animate.h"
#include "web/Canvas.h"
#include "web/canvas_utils.h"

#include "web/emfunctions.h"

#include "tools/Random.h"
#include "tools/vector.h"
#include "tools/assert.h"
#include "tools/TypeTracker.h"

#include "evo/World.h"

namespace web = emp::web;

// Some useful macros:
#define PLAY_GLYPH        "<span class=\"glyphicon glyphicon-play\" aria-hidden=\"true\"></span>"
#define PAUSE_GLYPH       "<span class=\"glyphicon glyphicon-pause\" aria-hidden=\"true\"></span>"
#define REFRESH_GLYPH     "<span class=\"glyphicon glyphicon-refresh\" aria-hidden=\"true\"></span>"
#define SINGLE_STEP_GLYPH "<span class=\"glyphicon glyphicon-step-forward\" aria-hidden=\"true\"></span>"
#define SETTINGS_GLYPH    "<span class=\"glyphicon glyphicon-cog\" aria-hidden=\"true\"></span>"
#define LOLLY_GLYPH       "<span class=\"glyphicon glyphicon-ice-lolly\" aria-hidden=\"true\"></span>"

// Default experiment settings.
// TODO: try to switch this to using config system (use config system to generate HTML stuff as well)
//  * Create a config object.
//  * Write a function that, given a config object, generates the necessary HTML form.
//  * Write a function that, when called, will automatically extract updated variable values from HTML form input.
//  -- General Settings --
const int DEFAULT_RANDOM_SEED = 3;
const int DEFAULT_WORLD_WIDTH = 350;
const int DEFAULT_WORLD_HEIGHT = 250;
//  -- Population-specific --
const int DEFAULT_MAX_POP_SIZE = 250;
//  -- Organism-specific --
const int DEFAULT_GENOME_LENGTH = 10;
const double DEFAULT_POINT_MUTATION_RATE = 0.01;
const double DEFAULT_MAX_ORGANISM_RADIUS = 4;
const double DEFAULT_COST_OF_REPRO = 3;
const bool DEFAULT_DETACH_ON_BIRTH = false;
const double DEFAULT_ORGANISM_MEMBRANE_STRENGTH = 10.0;
//  -- Resource-specific --
const int DEFAULT_MAX_RESOURCE_AGE = 10000;
const int DEFAULT_MAX_RESOURCE_COUNT = 25;
const double DEFAULT_RESOURCE_RADIUS = 2.0;
const double DEFAULT_RESOURCE_VALUE = 1.0;
const int DEFAULT_RESOURCE_IN_FLOW = 10;
//  -- Physics-specific --
const double DEFAULT_SURFACE_FRICTION = 0.0025;
const double DEFAULT_MOVEMENT_NOISE = 0.10;


class EvoInPhysicsInterface {
  private:
    // Aliases
    using Organism_t = SimpleOrganism;
    using SimplePhysicsWorld = emp::evo::World<Organism_t, emp::evo::PopulationManager_SimplePhysics<Organism_t> >;

    emp::Random *random;
    SimplePhysicsWorld *world;
    // Interface-specific objects.
    //  - Exp run views.
    //web::Document dashboard;      // Visible during exp page mode.
    web::Document world_view;     // Visible during exp page mode.
    //  - Config views.
    //web::Document exp_config;
    // Animation
    web::Animate anim;
    // Localized exp configuration variables.
    //  -- General Settings --
    int random_seed;
    int world_width;
    int world_height;
    //  -- Population-specific --
    int max_pop_size;
    //  -- Organism-specific --
    double point_mutation_rate;
    int genome_length;
    double max_organism_radius;
    double cost_of_repro;
    bool detach_on_birth;
    double organism_membrane_strength;
    //double organism_density;
    //  -- Resource-specific --
    int max_resource_age;
    int max_resource_count;
    double resource_radius;
    double resource_value;
    int resource_in_flow;
    //  -- Physics-specific --
    double surface_friction;
    double movement_noise;

  public:
    EvoInPhysicsInterface(int argc, char *argv[]) :
      world_view("world-view"),
      anim([this]() { EvoInPhysicsInterface::Animate(anim); })
    {
      /* Web interface constructor. */

      // Localize parameter values.
      // -- General --
      random_seed = DEFAULT_RANDOM_SEED;
      world_width = DEFAULT_WORLD_WIDTH;
      world_height = DEFAULT_WORLD_HEIGHT;
      //  -- Population-specific --
      max_pop_size = DEFAULT_MAX_POP_SIZE;
      //  -- Organism-specific --
      point_mutation_rate = DEFAULT_POINT_MUTATION_RATE;
      genome_length = DEFAULT_GENOME_LENGTH;
      max_organism_radius = DEFAULT_MAX_ORGANISM_RADIUS;
      cost_of_repro = DEFAULT_COST_OF_REPRO;
      detach_on_birth = DEFAULT_DETACH_ON_BIRTH;
      organism_membrane_strength = DEFAULT_ORGANISM_MEMBRANE_STRENGTH;
      //  -- Resource-specific --
      max_resource_age = DEFAULT_MAX_RESOURCE_AGE;
      max_resource_count = DEFAULT_MAX_RESOURCE_COUNT;
      resource_radius = DEFAULT_RESOURCE_RADIUS;
      resource_value = DEFAULT_RESOURCE_VALUE;
      resource_in_flow = DEFAULT_RESOURCE_IN_FLOW;
      //  -- Physics-specific --
      surface_friction = DEFAULT_SURFACE_FRICTION;
      movement_noise = DEFAULT_MOVEMENT_NOISE;

      // Setup page
      InitializeExperiment();
      DoToggleRun();

    }

    ~EvoInPhysicsInterface() {
      if (world != nullptr) delete world;
    }

    template<typename FieldType>
    std::string GenerateParamTextField(std::string field_name, std::string field_id, FieldType default_value) {
      return GenerateParamField(field_name, field_id, default_value, "Text");
    }

    template<typename FieldType>
    std::string GenerateParamNumberField(std::string field_name, std::string field_id, FieldType default_value) {
      return GenerateParamField(field_name, field_id, default_value, "Number");
    }

    std::string GenerateParamCheckboxField(std::string field_name, std::string field_id, bool default_value) {
      return GenerateParamField(field_name, field_id, default_value, "checkbox");
    }

    // TODO: Make checkbox input look nice.
    template<typename FieldType>
    std::string GenerateParamField(std::string field_name, std::string field_id, FieldType default_value, std::string input_type) {
      std::stringstream param_html;
        param_html << "<div class=\"input-group\">";
          param_html << "<span class=\"input-group-addon\" id=\"" << field_id << "-addon\">" << field_name << "</span>  ";
          param_html << "<input type=\"" << input_type << "\""
                     << " class=\"form-control\""
                     << " aria-describedby=\"" << field_id << "-addon\""
                     << " id=\"" << field_id << "-param\"";
                     if (input_type == "checkbox") param_html << "checked=\"" << default_value << "\">";
                     else param_html << " value=\"" << default_value << "\">";
        param_html << "</div>";
      return param_html.str();
    }

    // Call to initialize a new experiment with current config values.
    void InitializeExperiment() {
      // Setup the world.
      if (world != nullptr) delete world;
      if (random != nullptr) delete random; // World does not own *random. Delete it.
      random = new emp::Random(random_seed);
      world = new SimplePhysicsWorld(random, "simple-world");
      // Setup world view canvs.
      world_view.ClearChildren();
      world_view << web::Canvas(world_width, world_height, "simple-world-canvas") << "<br>";
      // Configure new experiment.
      // TODO: add resource_in_flow_rate parameter. Currently magic number.
      world->ConfigPop(world_width, world_height, surface_friction,
                       max_pop_size, point_mutation_rate, max_organism_radius,
                       cost_of_repro, max_resource_age, max_resource_count, resource_in_flow,
                       resource_radius, resource_value, movement_noise);
      // Run a reset
      DoReset();
    }

    void ResetEvolution() {
      // Purge the world!
      world->Clear();
      world->update = 0;
      // Initialize the population.
      const emp::Point<double> mid_point(world_width / 2.0, world_height / 2.0);
      int org_radius = max_organism_radius;
      Organism_t ancestor(emp::Circle(mid_point, org_radius), genome_length, detach_on_birth);
      // Randomize ancestor genome.
      for (int i = 0; i < ancestor.genome.GetSize(); i++) {
        if (random->P(0.5)) ancestor.genome[i] = !ancestor.genome[i];
      }
      // TODO: make mass dependent on density
      ancestor.GetBody().SetMass(10.0);
      ancestor.SetMembraneStrength(organism_membrane_strength);
      ancestor.SetBirthTime(-1);
      world->Insert(ancestor);
    }

    // Single animation step for this interface.
    void Animate(const web::Animate &anim) {
      world->Update();
      // Draw
      web::Draw(world_view.Canvas("simple-world-canvas"), world->popM.GetPhysics().GetSurface(), emp::GetHueMap(360));
    }

    // Called on start/stop button press.
    bool DoToggleRun() {
      anim.ToggleActive();
      // Grab buttons to manipulate:
      return true;
    }

    // Called on reset button press and when initializing the experiment.
    bool DoReset() {
      ResetEvolution();
      web::Draw(world_view.Canvas("simple-world-canvas"), world->popM.GetPhysics().GetSurface(), emp::GetHueMap(360));
      return true;
    }

    // Called on step button press.
    bool DoStep() {
      emp_assert(anim.GetActive() == false);
      anim.Step();
      return true;
    }

    // Called on run experiment button press (from config exp page).
    bool DoRunExperiment() {
      // Collect parameter values.
      // Initialize the experiment.
      InitializeExperiment();
      return true;
    }

    // Called on reconfigure exp button press (from run exp page).
    bool DoReconfigureExperiment() {
      // Set page mode to CONFIG.
      return true;
    }

    // Update parameter values from webpage using javascript.
    void UpdateParams() {
      // @amlalejini QUESTION: What is the best way to be collecting parameter values?
      // TODO: clean parameters (clip to max/min values, etc).
      // TODO: switch to config system, automate this.
      // -- General --
      random_seed = EM_ASM_INT_V({ return $("#random-seed-param").val(); });
      world_width = EM_ASM_INT_V({ return $("#world-width-param").val(); });
      world_height = EM_ASM_INT_V({ return $("#world-height-param").val(); });
      // -- Population-Specific --
      max_pop_size = EM_ASM_INT_V({ return $("#max-pop-size-param").val(); });
      // -- Organism-Specific --
      genome_length = EM_ASM_INT_V({ return $("#genome-length-param").val(); });
      point_mutation_rate = EM_ASM_DOUBLE_V({ return $("#point-mutation-rate-param").val(); });
      max_organism_radius = EM_ASM_DOUBLE_V({ return $("#max-organism-radius-param").val(); });
      cost_of_repro = EM_ASM_DOUBLE_V({ return $("#cost-of-repro-param").val(); });
      detach_on_birth = EM_ASM_INT_V({ return $("#detach-on-birth-param").is(":checked"); });
      organism_membrane_strength = EM_ASM_DOUBLE_V({ return $("#organism-membrane-strength-param").val(); });
      // -- Resource-Specific --
      max_resource_age = EM_ASM_INT_V({ return $("#max-resource-age-param").val(); });
      max_resource_count = EM_ASM_INT_V({ return $("#max-resource-count-param").val(); });
      resource_radius = EM_ASM_DOUBLE_V({ return $("#resource-radius-param").val(); });
      resource_value = EM_ASM_DOUBLE_V({ return $("#resource-value-param").val(); });
      resource_in_flow = EM_ASM_INT_V({ return $("#resource-in-flow-param").val(); });
      // -- Physics-Specific --
      surface_friction = EM_ASM_DOUBLE_V({ return $("#surface-friction-param").val(); });
      movement_noise = EM_ASM_DOUBLE_V({ return $("#movement-noise-param").val(); });
    }
};

EvoInPhysicsInterface *interface;

int main(int argc, char *argv[]) {
  interface = new EvoInPhysicsInterface(argc, argv);
  return 0;
}
