
#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <utility>
#include "base/Ptr.h"
#include "hardware/EventDrivenGP.h"
#include "hardware/InstLib.h"
#include "hardware/EventLib.h"
#include "tools/map_utils.h"

#include "web/init.h"
#include "web/JSWrap.h"
#include "web/Document.h"
#include "web/web.h"
#include "web/d3/visualizations.h"
#include "web/d3/dataset.h"

// @amlalejini - TODO:
// [ ] Switch to using linear scales
// [ ] Make sizing of everything dynamic
// [ ] Wrap x/y/text things in named functions

using event_lib_t = typename emp::EventDrivenGP::event_lib_t;
using inst_lib_t = typename::emp::EventDrivenGP::inst_lib_t;
using inst_t = typename::emp::EventDrivenGP::inst_t;
using affinity_t = typename::emp::EventDrivenGP::affinity_t;
using program_t = emp::EventDrivenGP::Program;
using fun_t = emp::EventDrivenGP::Function;

namespace emp {
namespace web {

// @amlalejini - NOTE: This is inspired by/mirrors Emily's D3Visualizations from web/d3/visualizations.h
class EventDrivenGP_Roles_LSVis : public D3Visualization {
public:
  // struct Inst { // @amlalejini - TODO: Ask how this works...
  //   EMP_BUILD_INTROSPECTIVE_TUPLE( int, position,
  //                                  int, function)
  // };

protected:

  Ptr<Random> random;
  std::map<std::string, program_t> program_map;     // Map of available programs.
  //program_t cur_program;                            // Program built from program data.

  double y_margin;
  double x_margin;

  double inst_blk_height = 50;
  double inst_blk_width = 150;
  double func_blk_width = 200;



  std::function<void(int, int)> knockout_inst = [this](int fp, int ip) {
    std::cout << "Knockout an instruction: " << fp << ", " << ip << std::endl;
  };

  std::function<void(int)> knockout_func = [this](int fp) {
    std::cout << "Knockout a function:" << fp << std::endl;
  };

  void InitializeVariables() {
    std::cout << "Init vars" << std::endl;
    //JSWrap(on_inst_click, GetID() + "on_inst_click");
    JSWrap(knockout_func, "knockout_func");
    JSWrap(knockout_inst, "knockout_inst");
    std::cout << GetID() << std::endl;
    GetSVG()->Move(0, 0);
  }

  void SetProgramData(std::string name) {
    emp_assert(Has(program_map, name));
    std::cout << "Set program data to: " << name << std::endl;
    if (program_data) program_data.Delete();
    const program_t & program = program_map.at(name);
    size_t cum_inst_cnt = 0;
    std::stringstream p_data;
    p_data << "{\"name\":\"" << name << "\",\"functions\":[";
    // For each function, append data.
    for (size_t fID = 0; fID < program.GetSize(); ++fID) {
      if (fID != 0) p_data << ",";
      p_data << "{\"affinity\":\""; program[fID].affinity.Print(p_data); p_data << "\",";
      p_data << "\"sequence_len\":" << program[fID].GetSize() << ",";
      p_data << "\"cumulative_seq_len\":" << cum_inst_cnt << ",";
      p_data << "\"sequence\":[";
      for (size_t iID = 0; iID < program[fID].GetSize(); ++iID) {
        if (iID != 0) p_data << ",";
        const inst_t & inst = program[fID].inst_seq[iID];
        p_data << "{\"name\":\"" << program.inst_lib->GetName(inst.id) << "\",";
        p_data << "\"position\":" << iID << ",";
        p_data << "\"function\":" << fID << ",";
        p_data << "\"has_affinity\":" << program.inst_lib->HasProperty(inst.id, "affinity") << ",";
        p_data << "\"num_args\":" << program.inst_lib->GetNumArgs(inst.id) << ",";
        p_data << "\"affinity\":\""; inst.affinity.Print(p_data); p_data << "\",";
        p_data << "\"args\":[" << inst.args[0] << "," << inst.args[1] << "," << inst.args[2] << "]";
        p_data << "}";
      }
      p_data << "]";
      p_data << "}";
      cum_inst_cnt += program[fID].GetSize();
    }
    p_data << "]}";
    program_data = NewPtr<D3::JSONDataset>();
    program_data->Append(p_data.str());
  }

  void DisplayProgram(std::string name) {
    emp_assert(Has(program_map, name));
    std::cout << "Display program: " << name << std::endl;
    // Set program data to correct program
    SetProgramData(name);
    // Draw program from program data.
    DrawProgram();
  }

  // @amlalejini - TODO
  void DrawProgram() {
    emp_assert(program_data);
    std::cout << "Draw Program" << std::endl;
    D3::Selection * svg = GetSVG();
    // Pro tip: can do all of the drawing in JS
    EM_ASM_ARGS({
      var on_inst_click = function(inst, i) {
        emp.knockout_inst(inst.function, inst.position);
      };
      var on_func_click = function(func, i) {
        emp.knockout_func(i);
      };
      var program_data = js.objects[$0][0];
      var svg = js.objects[$1];
      var prg_name = program_data["name"];
      var iblk_h = $2;
      var iblk_w = $3;
      var fblk_w = $4;
      var iblk_lpad = fblk_w - iblk_w;
      // Compute program display height.
      // @amlalejini - TODO: make this a function.
      var prg_h = program_data["functions"].length * iblk_h;
      for (fID = 0; fID < program_data["functions"].length; fID++) {
        prg_h += program_data["functions"][fID].sequence_len * iblk_h;
      }
      // Reconfigure vis size based on program data.
      svg.attr({"width": fblk_w, "height": prg_h});

      // Add a group for each function.
      var functions = svg.selectAll("g").data(program_data["functions"]);
      functions.enter().append("g");
      functions.exit().remove();
      functions.attr({"class": "program-function",
                      "transform": function(func, fID) {
                        var x_trans = 0;
                        var y_trans = (fID * iblk_h + func.cumulative_seq_len * iblk_h);
                        return "translate(" + x_trans + "," + y_trans + ")";
                      }});
      //var definitions = functions.append("g").attr({"class": "program-function-definition"});
      functions.append("rect")
               .attr({
                 "width": fblk_w,
                 "height": iblk_h,
                 "fill": "green",
                 "stroke": "black"
               })
               .on("click", on_func_click);
      functions.append("text")
               .attr({
                 "x": 0,
                 "y": iblk_h / 2,
                 "dy": ".5em",
                 "pointer-events": "none"
               })
               .text(function(func, fID) { return "fn-" + fID + " " + func.affinity + ":"; });

      // Draw instructions for each function.
      functions.each(function(func, fID) {
        instructions = d3.select(this).selectAll("g").data(func.sequence);
        instructions.enter().append("g");
        instructions.exit().remove();
        instructions.attr({
                          "class": "program-instruction",
                          "transform": function(d, i) {
                            var x_trans = iblk_lpad;
                            var y_trans = (iblk_h + i * iblk_h);
                            return "translate(" + x_trans + "," + y_trans + ")";
                          }});
        instructions.on("click", on_inst_click);
        instructions.append("rect")
                    .attr({
                      "width": iblk_w,
                      "height": iblk_h,
                      "fill": "blue",
                      "stroke": "black"
                    });
        instructions.append("text")
                    .attr({
                      "x": 0,
                      "y": iblk_h / 2,
                      "dy": "0.5em"
                    })
                    .text(function(d, i) {
                      var inst_str = d.name;
                      if (d.has_affinity) inst_str += " " + d.affinity;
                      for (arg = 0; arg < d.num_args; arg++) inst_str += " " + d.args[arg];
                      return inst_str;
                    });

      });


    }, program_data->GetID(),
       svg->GetID(),
       inst_blk_height,
       inst_blk_width,
       func_blk_width
    );

    //svg->SelectAll(".program-instruction").On("click", on_inst_click);
  }

public:
  // Program data: [{
  //  name: "prgm_name",
  //  functions: [
  //    {"affinity": "0000",
  //     "sequence": [{"name": "inst_name", "has_affinity": bool, "affinity": "0000", "args": [...]}, {}, {}, ...]
  //    },
  //    {...},
  //    ...
  //   ]
  // }
  // ]
  //
  Ptr<D3::JSONDataset> program_data;

  EventDrivenGP_Roles_LSVis(int width, int height) : D3Visualization(width, height) {

  }

  void Setup() {
    std::cout << "LSVis setup getting run." << std::endl;
    InitializeVariables();
    this->init = true;
    this->pending_funcs.Run();
  }

  Ptr<D3::JSONDataset> GetDataset() { return program_data; }

  void SetDataset(Ptr<D3::JSONDataset> d) { program_data = d; }


  // void LoadDataFromFile(std::string filename) {
  //   if (this->init) {
  //     program_data->LoadDataFromFile(filename, [this](){ std::cout << "On load?" << std::endl; });
  //   } else {
  //     this->pending_funcs.Add([this, filename]() {
  //       program_data->LoadDataFromFile(filename, [this](){ std::cout << "On load?" << std::endl; });
  //     });
  //   }
  // }

  void AddProgram(const std::string & _name, const program_t & _program) {
    std::cout << "Adding program: " << _name << std::endl;
    program_map.emplace(_name, _program);
  }

  // @amlalejini - TODO
  void Clear() {

  }

  void Run(std::string name = "") {
    std::cout << "Run" << std::endl;
    if (name == "")
      std::cout << "Run w/defaults....(TODO)" << std::endl;
    if (this->init) {
      DisplayProgram(name);
    } else {
      this->pending_funcs.Add([this, name]() { DisplayProgram(name); });
    }
  }


};

}
}

emp::web::Document doc("program-vis");
emp::web::EventDrivenGP_Roles_LSVis visualization(100, 100);

int main(int argc, char *argv[]) {
  emp::Initialize();
  doc << visualization;

  // Test program.
  // emp::Ptr<event_lib_t> event_lib = emp::NewPtr<event_lib_t>(*emp::EventDrivenGP::DefaultInstLib());
  emp::Ptr<inst_lib_t> inst_lib = emp::NewPtr<inst_lib_t>(*emp::EventDrivenGP::DefaultInstLib());
  program_t test_program(inst_lib);
  test_program.PushFunction(fun_t());
  for (size_t i=0; i < 5; ++i) test_program.PushInst("Nop");
  test_program.PushFunction(fun_t());
  for (size_t i=0; i < 5; ++i) test_program.PushInst("Nop");
  test_program.PushInst("Call", 0, 0, 0, affinity_t());
  test_program.PushInst("Inc", 1);
  test_program.PushInst("Add", 0, 1, 5);
  visualization.AddProgram("Test", test_program);

  // Start the visualization.
  visualization.Run("Test");


  return 0;
}
