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

namespace emp {
namespace web {
  void DrawRPS(Canvas canvas) {
    canvas.Clear();

    const double w = 500;
    const double h = 500;

    // Setup a black background for the surface
    canvas.Rect(0, 0, w, h, "black");

    // Draw the circles.
    EM_ASM({
      var ctx = document.getElementById("rps-canvas").getContext("2d");
      var img = document.getElementById("rock-img");
      ctx.drawImage(img, 10, 10);
    });
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
    web::Document dashboard;      // Visible during exp page mode.
    web::Document world_view;     // Visible during exp page mode.
    web::Document stats_view;     // Visible during exp page mode.
    web::Document param_view;     // Visible during config page mode.
    //  - Config views.
    web::Document exp_config;
    // Animation
    web::Animate anim;
    // Page mode.
    enum class PageMode { EXPERIMENT, CONFIG } page_mode;
    // Localized exp configuration variables.
    // -- General Settings --
    int random_seed;
    int world_width;
    int world_height;


  public:
    RockPaperScissorsInterface(int argc, char *argv[]) :
      dashboard("dashboard-panel-body"),
      world_view("world-view"),
      stats_view("stats-view"),
      param_view("config-view"),
      exp_config("exp-config-panel-body"),
      anim([this]() { RockPaperScissorsInterface::Animate(anim); }),
      page_mode(PageMode::CONFIG)
    {
      /* Web interface constructor. */

      // Localize parameter values.
      // -- General --
      world_width = 500;
      world_height = 500;
      random_seed = 1;

      // Setup page
      // - Setup EXPERIMENT RUN mode view. -
      // --- Setup Dashboard. ---
      // ----- start/stop button -----
      dashboard << web::Button([this]() { DoToggleRun(); }, PLAY_GLYPH, "start_but");
      auto start_button = dashboard.Button("start_but");
      start_button.SetAttr("class", "btn btn-success");
      // ----- refresh button -----
      dashboard << web::Button([this]() { DoReset(); }, REFRESH_GLYPH, "reset_but");
      auto reset_button = dashboard.Button("reset_but");
      reset_button.SetAttr("class", "btn btn-primary");
      // ----- step button ------
      dashboard << web::Button([this]() { DoStep(); }, SINGLE_STEP_GLYPH, "step_but");
      auto step_button = dashboard.Button("step_but");
      step_button.SetAttr("class", "btn btn-default");
      // ----- reconfigure button -----
      dashboard << web::Button([this]() { DoReconfigureExperiment(); }, SETTINGS_GLYPH, "settings_but");
      auto reconfigure_button = dashboard.Button("settings_but");
      reconfigure_button.SetAttr("class", "btn btn-default pull-right");

      // --- Setup Stats View. ---
      stats_view.SetAttr("class", "well");

      // --- Setup Config View. ---
      param_view.SetAttr("class", "well");
      param_view << web::Image("imgs/rock.svg", "rock-img") << "<br>";
      auto rock_img = param_view.Image("rock-img");
      rock_img.SetAttr("style", "width:50px;height:50px;");
      param_view << web::Image("imgs/paper.svg", "paper-img") << "<br>";
      auto paper_img = param_view.Image("paper-img");
      paper_img.SetAttr("style", "width:50px;height:50px;");
      param_view << web::Image("imgs/scissors.svg", "scissors-img") << "<br>";
      auto scissors_img = param_view.Image("scissors-img");
      scissors_img.SetAttr("style", "width:50px;height:50px;");

      // - Setup EXPERIMENT CONFIG mode view. -
      exp_config << web::Button([this]() { DoRunExperiment(); }, LOLLY_GLYPH, "start_exp_but");
      auto start_exp_but = exp_config.Button("start_exp_but");
      start_exp_but.SetAttr("class", "btn btn-danger");
      exp_config << "<hr>";
      //  - Experiment Settings -


      // Configure page view.
      ChangePageView(page_mode);

      if (page_mode == PageMode::EXPERIMENT) InitializeExperiment();

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
      std::cout << "Interface::InitializeExperiment" << std::endl;
      // Setup the world.
      if (world != nullptr) delete world;
      if (random != nullptr) delete random;
      random = new emp::Random(random_seed);
      world = new RPSWorld(random, "rps-world");
      // Setup world view on canvas.
      world_view.ClearChildren();
      world_view << web::Canvas(world_width, world_height, "rps-canvas") << "<br>";
      // Configure the new experiment.
      world->ConfigPop(world_width, world_height, 0.0025);
      // Run a reset
      DoReset();
    }

    void ResetEvolution() {
      std::cout << "Interface::ResetEvolution" << std::endl;
      // Purge the world!
      world->Clear();
      world->update = 0;
      // Initialize the population.
      int num_rocks = 1;
      int num_scissors = 1;
      int num_papers = 1;
      // - 1 random rock
      for (int i = 0; i < num_rocks; i++) {
        const emp::Point<double> loc(random->GetInt(world_width), random->GetInt(world_height));
        RPSOrg rockie(emp::Circle(loc, 10), RPS_TYPE::ROCK);
        world->Insert(rockie);
      }
      // - 1 random scissor
      for (int i = 0; i < num_rocks; i++) {
        const emp::Point<double> loc(random->GetInt(world_width), random->GetInt(world_height));
        RPSOrg scissorie(emp::Circle(loc, 10), RPS_TYPE::SCISSORS);
        world->Insert(scissorie);
      }
      // - 1 random paper
      for (int i = 0; i < num_rocks; i++) {
        const emp::Point<double> loc(random->GetInt(world_width), random->GetInt(world_height));
        RPSOrg paperie(emp::Circle(loc, 10), RPS_TYPE::PAPER);
        world->Insert(paperie);
      }
    }

    // Single animation step for this interface.
    void Animate(const web::Animate &anim) {
      world->Update();
      // Draw
      //web::Draw(world_view.Canvas("simple-world-canvas"), world->popM.GetPhysics().GetSurface(), emp::GetHueMap(360));
      web::DrawRPS(world_view.Canvas("rps-canvas"));
      stats_view.Redraw();
    }

    // Called on start/stop button press.
    bool DoToggleRun() {
      std::cout << "Interface::DoToggleRun" << std::endl;
      anim.ToggleActive();
      // Grab buttons to manipulate:
      auto start_but = dashboard.Button("start_but");
      auto step_but = dashboard.Button("step_but");
      // Update buttons
      if (anim.GetActive()) {
        // If active, set button to show 'stop' button.
        start_but.Label(PAUSE_GLYPH);
        start_but.SetAttr("class", "btn btn-danger");
        step_but.Disabled(true);
      } else {
        // If inactive, set button to show 'play' option.
        start_but.Label(PLAY_GLYPH);
        start_but.SetAttr("class", "btn btn-success");
        step_but.Disabled(false);
      }
      return true;
    }

    // Called on reset button press and when initializing the experiment.
    bool DoReset() {
      std::cout << "Interface::DoReset" << std::endl;
      ResetEvolution();
      web::DrawRPS(world_view.Canvas("rps-canvas"));
      //web::Draw(world_view.Canvas("simple-world-canvas"), world->popM.GetPhysics().GetSurface(), emp::GetHueMap(360));
      stats_view.Redraw();
      return true;
    }

    // Called on step button press.
    bool DoStep() {
      std::cout << "Interface::DoStep" << std::endl;
      emp_assert(anim.GetActive() == false);
      anim.Step();
      return true;
    }

    // Called on run experiment button press (from config exp page).
    bool DoRunExperiment() {
      std::cout << "Interface::DoRunExperiment" << std::endl;
      // Collect parameter values.
      UpdateParams();
      // Initialize the experiment.
      InitializeExperiment();
      // Update page
      page_mode = ChangePageView(PageMode::EXPERIMENT);
      return true;
    }

    // Called on reconfigure exp button press (from run exp page).
    bool DoReconfigureExperiment() {
      std::cout << "Interface::DoReconfigureExperiment" << std::endl;
      // Set page mode to CONFIG.
      if (anim.GetActive()) {
        anim.ToggleActive();
        auto start_but = dashboard.Button("start_but");
        auto step_but = dashboard.Button("step_but");
        start_but.Label(PLAY_GLYPH);
        start_but.SetAttr("class", "btn btn-success");
        step_but.Disabled(false);
      }
      page_mode = ChangePageView(PageMode::CONFIG);
      return true;
    }

    // Update parameter values from webpage using javascript.
    void UpdateParams() {

    }

    PageMode ChangePageView(PageMode new_mode) {
      /* Calling this function updates the view to match the current page mode. */
      switch (new_mode) {
        case PageMode::CONFIG:
          // Transition to CONFIG view.
          // Hide: dashboard, stats-view, param-view
          EM_ASM( {
            $("#exp-config-view").show();
            $("#exp-run-view").hide();
          });
          break;
        case PageMode::EXPERIMENT:
          // Transition to EXPERIMENT view.
          stats_view.Redraw();
          param_view.Redraw();
          EM_ASM( {
            $("#exp-config-view").hide();
            $("#exp-run-view").show();
          });
          break;
      }
      return new_mode;
    }
};

RockPaperScissorsInterface *interface;

int main(int argc, char *argv[]) {
  interface = new RockPaperScissorsInterface(argc, argv);
  return 0;
}
