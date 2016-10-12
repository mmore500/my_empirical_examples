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

// #include "./geometry/Point2D.h"
 #include "./organisms/RPS_Organisms.h"
// #include "./resources/SimpleResource.h"
#include "./population-managers/PopulationManager_RPS.h"

#include "web/web.h"
#include "web/Document.h"
#include "web/Animate.h"
#include "web/Canvas.h"
#include "web/canvas_utils.h"
#include "web/Image.h"

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

// -- General Settings --
int DEFAULT_WORLD_WIDTH = 350;
int DEFAULT_WORLD_HEIGHT = 250;
int DEFAULT_RANDOM_SEED = 1;
// -- Physics Settings --
double DEFAULT_SURFACE_FRICTION = 0.0025;
// -- Organism Settings --
double DEFAULT_TYPE_MUTATION_RATE = 0.075;
double DEFAULT_COST_OF_REPRO = 1;
double DEFAULT_MOVEMENT_NOISE = 0.08;
// -- Scenario Settings --
int DEFAULT_INITIAL_NUM_ROCKS = 25;
int DEFAULT_INITIAL_NUM_PAPERS = 25;
int DEFAULT_INITIAL_NUM_SCISSORS = 25;

namespace emp {
namespace web {

  template <typename SHAPE_TYPE>
  void DrawRPS(Canvas canvas, const Surface2D<SHAPE_TYPE> & surface) {
    canvas.Clear();

    const double w = surface.GetWidth();
    const double h = surface.GetHeight();

    // Setup a black background for the surface
    canvas.Rect(0, 0, w, h, "black");

    // Draw rocks, papers, and scissors.
    const auto &shape_set = surface.GetConstShapeSet();
    for (auto *shape : shape_set) {
      // TODO: below is a gross looking hack.
      RPS_TYPE type = shape->GetOwnerPtr()->template GetOwnerPtr<RPSOrg, 0>()->GetType();
      int r = shape->GetRadius();
      int d = 2 * r;
      int x = shape->GetCenterX() - r;
      int y = shape->GetCenterY() - r;
      if (type == RPS_TYPE::ROCK) {
        // Draw the circles.
        EM_ASM_ARGS({
          emp.this_ctx.drawImage(emp.rock_img, $0, $1, width = $2, height = $2);
        }, x, y, d);
      } else if (type == RPS_TYPE::PAPER) {
        // Draw the circles.
        EM_ASM_ARGS({
          emp.this_ctx.drawImage(emp.paper_img, $0, $1, width = $2, height = $2);
        }, x, y, d);
      } else if (type == RPS_TYPE::SCISSORS) {
        // Draw the circles.
        EM_ASM_ARGS({
          emp.this_ctx.drawImage(emp.scissors_img, $0, $1, width = $2, height = $2);
        }, x, y, d);
      }
    }
  }
}
}

class RockPaperScissorsInterface {
  private:
    // Aliases
    using Organism_t = RPSOrg;
    using RPSWorld = emp::evo::World<Organism_t, emp::evo::PopulationManager_RPS<Organism_t> >;

    emp::Random *random;
    RPSWorld *world;

    // Interface-specific objects.
    //  - Exp run views.
    web::Document world_view;     // Visible during exp page mode.
    // Animation
    web::Animate anim;
    // Localized exp configuration variables.
    // -- General Settings --
    int random_seed;
    int world_width;
    int world_height;
    // -- Physics Settings --
    double surface_friction;
    // -- Organism Settings --
    double type_mutation_rate;
    double cost_of_repro;
    double movement_noise;
    // -- Scenario Settings --
    int initial_num_rocks;
    int initial_num_papers;
    int initial_num_scissors;

  public:
    RockPaperScissorsInterface(int argc, char *argv[]) :
      world_view("world-view-rps"),
      anim([this]() { RockPaperScissorsInterface::Animate(anim); })
    {
      /* Web interface constructor. */

      // Localize parameter values.
      // -- General --
      world_width = DEFAULT_WORLD_WIDTH;
      world_height = DEFAULT_WORLD_HEIGHT;
      random_seed = DEFAULT_RANDOM_SEED;
      // -- Physics --
      surface_friction = DEFAULT_SURFACE_FRICTION;
      // -- Organisms --
      type_mutation_rate = DEFAULT_TYPE_MUTATION_RATE;
      cost_of_repro = DEFAULT_COST_OF_REPRO;
      movement_noise = DEFAULT_MOVEMENT_NOISE;
      // -- Scenario Settings --
      initial_num_rocks = DEFAULT_INITIAL_NUM_ROCKS;
      initial_num_papers = DEFAULT_INITIAL_NUM_PAPERS;
      initial_num_scissors = DEFAULT_INITIAL_NUM_SCISSORS;

      // Setup page
      // --- Setup Config View. ---
      //param_view.SetAttr("class", "well");
      ////// Make sure images are loaded.
      // param_view << web::Image("imgs/rock.svg", "rock-img") << "<br>";
      // auto rock_img = param_view.Image("rock-img");
      // rock_img.SetAttr("style", "width:50px;height:50px;display:none;");
      // param_view << web::Image("imgs/paper.svg", "paper-img") << "<br>";
      // auto paper_img = param_view.Image("paper-img");
      // paper_img.SetAttr("style", "width:50px;height:50px;display:none;");
      // param_view << web::Image("imgs/scissors.svg", "scissors-img") << "<br>";
      // auto scissors_img = param_view.Image("scissors-img");
      // scissors_img.SetAttr("style", "width:50px;height:50px;display:none;");

      // Configure page view.
      InitializeExperiment();
      DoToggleRun();
    }

    ~RockPaperScissorsInterface() {

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
      if (random != nullptr) delete random;
      random = new emp::Random(random_seed);
      world = new RPSWorld(random, "rps-world");
      // Setup world view on canvas.
      world_view.ClearChildren();
      world_view << web::Canvas(world_width, world_height, "rps-canvas") << "<br>";
      EM_ASM({
        emp.this_ctx = document.getElementById("rps-canvas").getContext("2d");
        emp.rock_img = document.getElementById("rock-img");
        emp.paper_img = document.getElementById("paper-img");
        emp.scissors_img = document.getElementById("scissors-img");
      });
      // Configure the new experiment.
      world->ConfigPop(world_width, world_height, surface_friction,
                       type_mutation_rate, cost_of_repro, movement_noise);
      // Run a reset
      DoReset();
    }

    void ResetEvolution() {
      // Purge the world!
      world->Clear();
      world->update = 0;
      // Initialize the population.
      // - 1 random rock
      for (int i = 0; i < initial_num_rocks; i++) {
        const emp::Point<double> loc(random->GetInt(world_width), random->GetInt(world_height));
        RPSOrg rockie(emp::Circle(loc, 6), RPS_TYPE::ROCK);
        world->Insert(rockie);
      }
      // - 1 random scissor
      for (int i = 0; i < initial_num_scissors; i++) {
        const emp::Point<double> loc(random->GetInt(world_width), random->GetInt(world_height));
        RPSOrg scissorie(emp::Circle(loc, 6), RPS_TYPE::SCISSORS);
        world->Insert(scissorie);
      }
      // - 1 random paper
      for (int i = 0; i < initial_num_papers; i++) {
        const emp::Point<double> loc(random->GetInt(world_width), random->GetInt(world_height));
        RPSOrg paperie(emp::Circle(loc, 6), RPS_TYPE::PAPER);
        world->Insert(paperie);
      }
    }

    // Single animation step for this interface.
    void Animate(const web::Animate &anim) {
      world->Update();
      // Draw
      //web::Draw(world_view.Canvas("simple-world-canvas"), world->popM.GetPhysics().GetSurface(), emp::GetHueMap(360));
      web::DrawRPS(world_view.Canvas("rps-canvas"), world->popM.GetPhysics().GetSurface());
    }

    // Called on start/stop button press.
    bool DoToggleRun() {
      anim.ToggleActive();

      return true;
    }

    // Called on reset button press and when initializing the experiment.
    bool DoReset() {
      ResetEvolution();
      web::DrawRPS(world_view.Canvas("rps-canvas"), world->popM.GetPhysics().GetSurface());
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
      // Update page
      return true;
    }

    // Called on reconfigure exp button press (from run exp page).
    bool DoReconfigureExperiment() {
      // Set page mode to CONFIG.

      return true;
    }

    // Update parameter values from webpage using javascript.
    void UpdateParams() {
      // -- General --
      random_seed = EM_ASM_INT_V({ return $("#random-seed-param").val(); });
      world_width = EM_ASM_INT_V({ return $("#world-width-param").val(); });
      world_height = EM_ASM_INT_V({ return $("#world-height-param").val(); });
      initial_num_rocks = EM_ASM_INT_V({ return $("#initial-num-rocks-param").val(); });
      initial_num_papers = EM_ASM_INT_V({ return $("#initial-num-papers-param").val(); });
      initial_num_scissors = EM_ASM_INT_V({ return $("#initial-num-scissors-param").val(); });
      // -- Organism --
      type_mutation_rate = EM_ASM_DOUBLE_V({ return $("#type-mutation-rate-param").val(); });
      cost_of_repro = EM_ASM_DOUBLE_V({ return $("#cost-of-repro-param").val(); });
      // -- Physics-Specific --
      surface_friction = EM_ASM_DOUBLE_V({ return $("#surface-friction-param").val(); });
      movement_noise = EM_ASM_DOUBLE_V({ return $("#movement-noise-param").val(); });
    }
};

RockPaperScissorsInterface *interface;

int main(int argc, char *argv[]) {
  interface = new RockPaperScissorsInterface(argc, argv);
  return 0;
}
