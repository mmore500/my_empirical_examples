//  This file is part of Project Name
//  Copyright (C) Michigan State University, 2017.
//  Released under the MIT Software license; see doc/LICENSE

#include <limits>
#include <cmath>
#include <iostream>
#include <algorithm>

#include "base/vector.h"
#include "tools/Random.h"
#include "tools/random_utils.h"
#include "tools/math.h"
#include "geometry/Circle2D.h"
#include "web/web.h"
#include "web/JSWrap.h"
#include "web/color_map.h"

namespace UI = emp::web;

constexpr size_t MAX_RANDOMIZE_TRIES = 10000; //< How many times should we retry dropping random points to avoid point-collisions?
constexpr size_t RANDOM_POINT_DROP_CNT = 10;
/// WARNING: KMeansExample does not support having multiple instances on the same HTML page.
/// WARNING: KMeansExample makes assumptions about the associated .html page. Not really meant to be
///          used outside of the context of this application.
class KMeansExample {
protected:

UI::Document viewer;        //< Div that contains the canvas viewer.
UI::Document data_dash;     //< Div that contains the data configuration dashboard.
UI::Document cluster_dash;  //< Div that contains the cluster configuration dashboard.
UI::Animate anim;

emp::Random random;   //< The RNGod.

size_t width;         //< Width in pixels of point viewer.
size_t height;        //< Height in pixels of point viewer.
size_t point_radius;  //< Radius of data points. (only matters for drawing)

double canvas_pos_x;  //< Internally-used variable to keep track of canvas x position on client window. (NOTE: only updated on mouse click events)
double canvas_pos_y;  //< Internally-used variable to keep track of canvas y position on client window. (NOTE: only updated on mouse click events)

size_t cluster_iteration; //< What iteration of the clustering algorithm are we on?
size_t num_bins;          //< How many K-means clustering bins should we have?

emp::vector<emp::Circle> points;    //< Vector to keep track of data points.
emp::vector<size_t> cluster_ids;    //< Vector to keep track of which cluster each point belongs to.
emp::vector<emp::Circle> centroids; //< Vector to keep track of cluster centroids.

enum class Mode { CLUSTER, CONFIG } page_mode;  //< What mode is the page in?

/// This function is wrapped in javascript. Used to update canvas position from javascript.
void SetCanvasPositionInternal(double x, double y) { canvas_pos_x = x; canvas_pos_y = y; }

/// Given a field name, field_id, and field value: generate the HTML for a numeric input field.
/// WARNING: this function calls a function (see below) that is copy-pasta from some old, possibly
///          out-dated code. Shit still works and looks okay, though.
template<typename FieldType>
std::string GenerateParamNumberField(std::string field_name, std::string field_id, FieldType default_value) {
  return GenerateParamField(field_name, field_id, default_value, "Number");
}

/// Given a field name, field_id, field value, and input type: generate the HTML for an input field
/// of the provided type.
/// WARNING: Pulled this function out of some old code. No promises on whether or not it uses good
///          practice, or even if it's up to date with the newest version of Bootstrap.
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

public:
  KMeansExample()
    : viewer("emp_viewer"),
      data_dash("emp_data_dash"),
      cluster_dash("emp_cluster_dash"),
      anim([this]() { this->Animate(anim); } ),
      random(),
      width(500), height(500),
      point_radius(10),
      cluster_iteration(0),
      num_bins(3),
      points(), cluster_ids(), centroids(),
      page_mode(Mode::CONFIG)
  {
    // Wrap some necessary functions for js<-->c++ comms.
    emp::JSWrap([this](double x, double y) { this->SetCanvasPositionInternal(x, y); }, "KMeansExample__SetCanvasPosition");

    // -- Configure page. --
    // Add a canvas to the page, paint it black, and center.
    auto canvas = viewer.AddCanvas(width, height, "DataViewer");
    canvas.On("click", [this](UI::MouseEvent event) { this->OnMouseClick(event); });
    canvas.Clear("black");
    viewer.SetAttr("class", "d-flex justify-content-center");

    // Build data configuration dashboard.
    data_dash << UI::Button([this]() { this->DoAddPoints(); }, "Drop Points", "drop_points_button");
    data_dash << UI::Button([this]() { this->DoClear(); }, "Clear", "clear_button");
    data_dash << UI::Button([this]() { this->DoToggleMode(); }, "Cluster Mode", "cluster_mode_button");
    data_dash << GenerateParamNumberField("K", "num_bins", num_bins);

    // Build cluster dashboard.
    cluster_dash << UI::Button([this]() { this->DoToggleRun(); }, "Run", "run_pause_button");
    cluster_dash << UI::Button([this]() { this->DoStep(); }, "Step", "step_button");
    cluster_dash << UI::Button([this]() { this->Reset(); }, "Reset", "reset_button");
    cluster_dash << UI::Button([this]() { this->DoToggleMode(); }, "Data Mode", "config_mode_button");
    cluster_dash << "<br/>";
    cluster_dash << "<h2><span class=\"badge badge-secondary" << "\" style=\"margin-left:5px\">" << "K: " << UI::Live([this]() { return this->num_bins; }) << "</span></h2>";

    // Configure all 'da buttons.
    // - They'll be bootstrap buttons: class="btn"
    // - We'll give them some margins so they're not right on top of one another.
    auto dp_button = data_dash.Button("drop_points_button");
    dp_button.SetAttr("class", "btn btn-primary");
    dp_button.SetAttr("style", "margin:5px");
    auto cl_button = data_dash.Button("clear_button");
    cl_button.SetAttr("class", "btn btn-primary");
    cl_button.SetAttr("style", "margin:5px");
    auto cm_button = data_dash.Button("cluster_mode_button");
    cm_button.SetAttr("class", "btn btn-primary");
    cm_button.SetAttr("style", "margin:5px");
    auto rp_button = cluster_dash.Button("run_pause_button");
    rp_button.SetAttr("class", "btn btn-primary");
    rp_button.SetAttr("style", "margin:5px");
    auto st_button = cluster_dash.Button("step_button");
    st_button.SetAttr("class", "btn btn-primary");
    st_button.SetAttr("style", "margin:5px");
    auto rt_button = cluster_dash.Button("reset_button");
    rt_button.SetAttr("class", "btn btn-primary");
    rt_button.SetAttr("style", "margin:5px");
    auto cg_button = cluster_dash.Button("config_mode_button");
    cg_button.SetAttr("class", "btn btn-primary");
    cg_button.SetAttr("style", "margin:5px");

    // Get init.
    InitConfigMode(); // Initialize configuration mode.
  }

  /// Handler function for canvas mouse clicks.
  /// For each click in the canvas, make a new point in position where click occured.
  /// In the process, update canvas_pos_x, canvas_pos_y
  void OnMouseClick(UI::MouseEvent & event) {
    EM_ASM({
      // Get accurate X/Y position of canvas.
      var rect = $("#DataViewer")[0].getBoundingClientRect();
      emp.KMeansExample__SetCanvasPosition(rect["x"], rect["y"]);
    });
    int x = event.clientX - canvas_pos_x;
    int y = event.clientY - canvas_pos_y;
    AddPoint(x, y, point_radius);
    Draw();
  }

  /// Animate function: called every step of animation/running algorithm (i.e. this is the body of the run loop).
  void Animate(const UI::Animate & anim) {
    // 1) Update (iterate the clustering algorithm)
    ClusterSingleStep();
    // 2) Redraw the world.
    Draw();
  }

  /// Toggle between the running algorithm state and the not running the algorithm state.
  bool DoToggleRun() {
    // Toggle animation active.
    anim.ToggleActive();
    // Update buttons.
    auto run_but = cluster_dash.Button("run_pause_button");
    auto step_but = cluster_dash.Button("step_button");
    if (anim.GetActive()) {
      // If active, run button should show 'Pause', and step button should be disabled.
      run_but.Label("Pause");
      step_but.Disabled(true);
    } else {
      run_but.Label("Run");
      step_but.Disabled(false);
    }
    return true;
  }

  /// Single animation/algorithm step.
  bool DoStep() {
    anim.Step();
    return true;
  }

  /// Called on Drop Points button press. Drops some random points on the canvas.
  void DoAddPoints() {
    AddPoints(RANDOM_POINT_DROP_CNT);
  }

  /// The nuclear option! Clear the canvas of all points.
  void DoClear() {
    points.clear();
    cluster_ids.clear();
    centroids.clear();
    Draw();
  }

  /// Toggle between data config mode and cluster running mode.
  void DoToggleMode() {
    // Switch demo mode. Update page as necessary.
    switch(page_mode) {
      case Mode::CONFIG:
        InitClusterMode(); break;
      case Mode::CLUSTER:
        InitConfigMode(); break;
    }
  }

  /// Draw points and cluster centroids. Color everything appropriately.
  void Draw() {
    auto canvas = viewer.Canvas("DataViewer");
    canvas.Clear("black");
    const auto & color_map = emp::GetHueMap(num_bins, 0.0, 330);
    // Draw points.
    for (size_t i = 0; i < points.size(); ++i) {
      std::string color = (cluster_iteration) ? color_map[cluster_ids[i]] : "yellow";
      canvas.Circle(points[i], color);
    }
    // Draw centroids.
    for (size_t i = 0; i < centroids.size(); ++i) {
      auto & centroid = centroids[i];
      canvas.Circle(centroid.GetCenterX(), centroid.GetCenterY(), centroid.GetRadius() * 2, "grey");
      canvas.Circle(centroid, color_map[i]);
    }
  }

  /// Update dashboard visibility based on page mode.
  void UpdateDash() {
    // Hide/display a particular dashboard depending on page mode.
    switch(page_mode) {
      case Mode::CONFIG:
        EM_ASM({
          $("#data_dash_card").attr("style", "display:block;");
          $("#cluster_dash_card").attr("style", "display:none;");
        });
        break;
      case Mode::CLUSTER:
        EM_ASM({
          $("#data_dash_card").attr("style", "display:none;");
          $("#cluster_dash_card").attr("style", "display:block;");
        });
        break;
    }
    cluster_dash.Redraw();
  }

  /// Reset clustering algorithm (cluster memberships, etc, etc.) then redraw.
  void Reset() {
    // Cluster reset.
    cluster_iteration = 0;
    centroids.clear(); centroids.resize(num_bins);
    for (size_t i = 0; i < centroids.size(); ++i) { centroids[i].Set(0, 0, 0); }
    for (size_t i = 0; i < cluster_ids.size(); ++i) { cluster_ids[i] = 0; }
    Draw();
  }

  /// Initialize cluster mode.
  /// This should be called to enter cluster mode.
  /// Does everything that needs doing to enter cluster mode.
  void InitClusterMode() {
    page_mode = Mode::CLUSTER;
    num_bins = std::min(std::max(1, EM_ASM_INT_V({ return $("#num_bins-param").val(); })), (int)points.size());
    Reset();
    UpdateDash();
  }

  /// Initialize configuration mode.
  /// This should be called to enter configuration mode.
  /// Does everything that needs doing to enter config mode.
  void InitConfigMode() {
    page_mode = Mode::CONFIG;
    if (anim.GetActive()) { DoToggleRun(); }
    std::cout << "INIT CONFIG MODE" << std::endl;
    EM_ASM_ARGS({
      console.log($0);
      $("#num_bins-param").attr("value", $0);
      console.log($("#num_bins-param").val());
    }, num_bins);
    Reset();
    UpdateDash();
  }

  /// Add a single point to data.
  void AddPoint(const emp::Circle & circ) {
    points.emplace_back(circ);
    cluster_ids.emplace_back(0);
  }

  /// Add a single point to data.
  void AddPoint(double x, double y, double r) {
    points.emplace_back(x, y, r);
    cluster_ids.emplace_back(0);
  }

  /// Generate n random points.
  void AddPoints(size_t n) {
    // range: 0 + point_radius : width - point_radius
    // range: 0 + point_radius : height - point_radius
    size_t counter = 0;
    for (size_t i = 0; i < n; ++i) {
      emp::Circle c = emp::Circle(0, 0, point_radius);
      bool sat;
      while (counter < MAX_RANDOMIZE_TRIES) {
        double y = random.GetDouble(point_radius, height - point_radius);
        double x = random.GetDouble(point_radius, width - point_radius);
        c.SetCenter(x, y);
        sat = true;
        for (size_t j = 0; j < points.size(); ++j) {
          if (c.HasOverlap(points[j])) { sat = false; break; }
        }
        if (sat) { break; }
        ++counter;
      }
      AddPoint(c);
    }
    Draw();
  }

  /// Take a single step in the k-means clustering algorithm.
  void ClusterSingleStep() {
    // If no points have been laid down... do nothing.
    if (points.empty()) return;
    // Assign membership.
    if (cluster_iteration == 0) {
      // Randomly if on initial iteration.
      for (size_t i = 0; i < cluster_ids.size(); ++i) { cluster_ids[i] = i % num_bins; }
      emp::Shuffle(random, cluster_ids);
    } else {
      // Assign each point to it's nearest centroid.
      for (size_t i = 0; i < points.size(); ++i) {
        double min_dist = std::numeric_limits<double>::max();
        size_t cID = 0;
        for (size_t c = 0; c < centroids.size(); ++c) {
          double cen_dist = Distance(points[i], centroids[c]);
          if (cen_dist < min_dist) { min_dist = cen_dist; cID = c; }
        }
        cluster_ids[i] = cID;
      }
    }
    // Update centurions.
    // - Reset all centroids to 0,0 (make maths easier for ourselves).
    for (size_t i = 0; i < centroids.size(); ++i) { centroids[i].Set(0, 0, 0); }
    for (size_t i = 0; i < points.size(); ++i) {
      auto & centroid = centroids[cluster_ids[i]];
      centroid.SetCenterX(centroid.GetCenterX() + points[i].GetCenterX());
      centroid.SetCenterY(centroid.GetCenterY() + points[i].GetCenterY());
      centroid.SetRadius(centroid.GetRadius() + 1);
    }
    for (size_t i = 0; i < centroids.size(); ++i) {
      auto & centroid = centroids[i];
      centroid.SetCenterX(centroid.GetCenterX() / centroid.GetRadius());
      centroid.SetCenterY(centroid.GetCenterY() / centroid.GetRadius());
      centroid.SetRadius(point_radius/2.0); // Set radius to normal size.
    }
    ++cluster_iteration;
  }

  /// Compute the euclidean distance between two points (a, b).
  /// Used here as the point similarity metric for k-means clustering algorithm.
  double Distance(const emp::Circle & a, const emp::Circle & b) {
    return (std::sqrt( std::pow(a.GetCenterX() - b.GetCenterX(), 2) + std::pow(a.GetCenterY() - b.GetCenterY(), 2) ));
  }

};

KMeansExample e;

int main()
{
 ;
}
