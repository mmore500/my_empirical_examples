
#include <iostream>
#include <sstream>
#include <map>
#include <unordered_set>
#include <set>
#include <string>
#include <utility>
#include "base/Ptr.h"
#include "hardware/EventDrivenGP.h"
#include "hardware/InstLib.h"
#include "hardware/EventLib.h"
#include "tools/map_utils.h"
#include "tools/string_utils.h"

#include "web/init.h"
#include "web/JSWrap.h"
#include "web/Document.h"
#include "web/Animate.h"
#include "web/web.h"
#include "web/d3/visualizations.h"
#include "web/d3/dataset.h"

// @amlalejini - TODO:
// [ ] Switch to using linear scales
// [x] Make sizing of everything dynamic
// [ ] Wrap x/y/text things in named functions
// [ ] Move all styling to SCSS
// [ ] Beautify (with SASS)
// [ ] Put in place safety features
//    [ ] What if we've knocked out the entire program?
//    [ ] Protect vis functions from getting called in unexpected order.
// [ ] Parameterize everything.


using event_lib_t = typename emp::EventDrivenGP::event_lib_t;
using event_t = typename emp::EventDrivenGP::event_t;
using inst_lib_t = typename::emp::EventDrivenGP::inst_lib_t;
using inst_t = typename::emp::EventDrivenGP::inst_t;
using affinity_t = typename::emp::EventDrivenGP::affinity_t;
using program_t = emp::EventDrivenGP::Program;
using fun_t = emp::EventDrivenGP::Function;
using state_t = emp::EventDrivenGP::State;

constexpr size_t EVAL_TIME = 50;
constexpr size_t DIST_SYS_WIDTH = 5;
constexpr size_t DIST_SYS_HEIGHT = 5;
constexpr size_t DIST_SYS_SIZE = DIST_SYS_WIDTH * DIST_SYS_HEIGHT;

constexpr size_t TRAIT_ID__ROLE_ID = 0;
constexpr size_t TRAIT_ID__X_LOC = 1;
constexpr size_t TRAIT_ID__Y_LOC = 2;

constexpr size_t CPU_SIZE = emp::EventDrivenGP::CPU_SIZE;
constexpr size_t MAX_INST_ARGS = emp::EventDrivenGP::MAX_INST_ARGS;

constexpr int DEFAULT_RANDOM_SEED = -1;

// This will be the target of evolution (what the world manages/etc.)
struct Agent {
  size_t valid_uid_cnt;
  size_t valid_id_cnt;
  program_t program;

  Agent(emp::Ptr<inst_lib_t> _ilib)
  : valid_uid_cnt(0), valid_id_cnt(0), program(_ilib) { ; }

  Agent(const program_t & _program)
  : valid_uid_cnt(0), valid_id_cnt(0), program(_program) { ; }

};

// Deme structure for holding distributed system.
struct Deme {
  using hardware_t = emp::EventDrivenGP;
  using memory_t = typename emp::EventDrivenGP::memory_t;
  using grid_t = emp::vector<emp::Ptr<hardware_t>>;
  using pos_t = std::pair<size_t, size_t>;

  grid_t grid;
  size_t width;
  size_t height;
  emp::Ptr<emp::Random> rnd;
  emp::Ptr<event_lib_t> event_lib;
  emp::Ptr<inst_lib_t> inst_lib;

  emp::Ptr<Agent> agent_ptr;
  bool agent_loaded;

  std::unordered_set<size_t> knockouts;

  Deme(emp::Ptr<emp::Random> _rnd, size_t _w, size_t _h, emp::Ptr<event_lib_t> _elib, emp::Ptr<inst_lib_t> _ilib)
    : grid(_w * _h), width(_w), height(_h), rnd(_rnd), event_lib(_elib), inst_lib(_ilib), agent_ptr(nullptr), agent_loaded(false), knockouts() {
    // Register dispatch function.
    event_lib->RegisterDispatchFun("Message", [this](hardware_t & hw_src, const event_t & event){ this->DispatchMessage(hw_src, event); });
    // Fill out the grid with hardware.
    for (size_t i = 0; i < width * height; ++i) {
      grid[i].New(inst_lib, event_lib, rnd);
      pos_t pos = GetPos(i);
      grid[i]->SetTrait(TRAIT_ID__ROLE_ID, 0);
      grid[i]->SetTrait(TRAIT_ID__X_LOC, pos.first);
      grid[i]->SetTrait(TRAIT_ID__Y_LOC, pos.second);
    }
  }

  ~Deme() {
    Reset();
    for (size_t i = 0; i < grid.size(); ++i) {
      grid[i].Delete();
    }
    grid.resize(0);
  }

  void Reset() {
    agent_ptr = nullptr;
    agent_loaded = false;
    for (size_t i = 0; i < grid.size(); ++i) {
      grid[i]->ResetHardware();
      grid[i]->SetTrait(TRAIT_ID__ROLE_ID, 0);
    }
  }

  void LoadAgent(emp::Ptr<Agent> _agent_ptr) {
    Reset();
    agent_ptr = _agent_ptr;
    for (size_t i = 0; i < grid.size(); ++i) {
      grid[i]->SetProgram(agent_ptr->program);
      grid[i]->SpawnCore(0, memory_t(), true);
    }
    agent_loaded = true;
  }

  size_t GetWidth() const { return width; }
  size_t GetHeight() const { return height; }

  pos_t GetPos(size_t id) { return pos_t(id % width, id / width); }
  size_t GetID(size_t x, size_t y) { return (y * width) + x; }

  void Print(std::ostream & os=std::cout) {
    os << "=============DEME=============\n";
    for (size_t i = 0; i < grid.size(); ++i) {
      os << "--- Agent @ (" << GetPos(i).first << ", " << GetPos(i).second << ") ---\n";
      grid[i]->PrintState(os); os << "\n";
    }
  }

  void DispatchMessage(hardware_t & hw_src, const event_t & event) {
    const size_t x = (size_t)hw_src.GetTrait(TRAIT_ID__X_LOC);
    const size_t y = (size_t)hw_src.GetTrait(TRAIT_ID__Y_LOC);
    emp::vector<size_t> recipients;
    if (event.HasProperty("send")) {
      // Send to random neighbor.
      recipients.push_back(GetRandomNeighbor(GetID(x, y)));
    } else {
      // Treat as broadcast, send to all neighbors.
      recipients.push_back(GetID((size_t)emp::Mod((int)x - 1, (int)width), (size_t)emp::Mod((int)y, (int)height)));
      recipients.push_back(GetID((size_t)emp::Mod((int)x + 1, (int)width), (size_t)emp::Mod((int)y, (int)height)));
      recipients.push_back(GetID((size_t)emp::Mod((int)x, (int)width), (size_t)emp::Mod((int)y - 1, (int)height)));
      recipients.push_back(GetID((size_t)emp::Mod((int)x, (int)width), (size_t)emp::Mod((int)y + 1, (int)height)));
    }
    // Dispatch event to recipients.
    for (size_t i = 0; i < recipients.size(); ++i)
      grid[recipients[i]]->QueueEvent(event);
  }

  // Pulled this function from PopMng_Grid.
  size_t GetRandomNeighbor(size_t id) {
    const int offset = rnd->GetInt(9);
    const int rand_x = (int)(id%width) + offset%3-1;
    const int rand_y = (int)(id/width) + offset/3-1;
    return ((size_t)emp::Mod(rand_x, (int)width) + (size_t)emp::Mod(rand_y, (int)height)*width);
  }

  void Advance(size_t t=1) { for (size_t i = 0; i < t; ++i) SingleAdvance(); }

  void SingleAdvance() {
    emp_assert(agent_loaded);
    for (size_t i = 0; i < grid.size(); ++i) {
      if (!knockouts.count(i)) grid[i]->SingleProcess();
    }
  }
};

// Some extra instructions for this experiment.
void Inst_GetRoleID(emp::EventDrivenGP & hw, const inst_t & inst) {
  state_t & state = *hw.GetCurState();
  state.SetLocal(inst.args[0], hw.GetTrait(TRAIT_ID__ROLE_ID));
}

void Inst_SetRoleID(emp::EventDrivenGP & hw, const inst_t & inst) {
  state_t & state = *hw.GetCurState();
  hw.SetTrait(TRAIT_ID__ROLE_ID, (int)state.AccessLocal(inst.args[0]));
}

void Inst_GetXLoc(emp::EventDrivenGP & hw, const inst_t & inst) {
  state_t & state = *hw.GetCurState();
  state.SetLocal(inst.args[0], hw.GetTrait(TRAIT_ID__X_LOC));
}

void Inst_GetYLoc(emp::EventDrivenGP & hw, const inst_t & inst) {
  state_t & state = *hw.GetCurState();
  state.SetLocal(inst.args[0], hw.GetTrait(TRAIT_ID__Y_LOC));
}


namespace emp {
namespace web {

// @amlalejini - NOTE: This is inspired by/mirrors Emily's D3Visualizations from web/d3/visualizations.h
class EventDrivenGP_ProgramVis : public D3Visualization {

protected:
  struct ProgramPosition {
    EMP_BUILD_INTROSPECTIVE_TUPLE(int, fID,
                                  int, iID
                                )
  };

  using pos_t = std::pair<int, int>;

  std::map<std::string, program_t> program_map;     // Map of available programs.
  Ptr<program_t> cur_program;                       // Program built from program data.
  std::string display_program;

  std::map<pos_t, pos_t> program_pos_map; // Map from original(base) program fp/ip space to cur program fp/ip space.
  std::map<pos_t, double> landscape_map;  // Map from cur program fp/ip --> fitness contribution for that location.

  std::set<std::pair<int, int>> inst_knockouts;
  std::set<int> func_knockouts;

  std::function<double(Ptr<program_t>)> eval_program = [](Ptr<program_t>) { return 0.0; };

  std::function<bool(int, int)> is_knockedout = [this](int fID, int iID = -1) {
    if (iID == -1) {
      return (bool)this->func_knockouts.count(fID);
    }
    return (bool)this->inst_knockouts.count(std::make_pair(fID, iID));
  };

  std::function<void(int, int)> knockout_inst = [this](int fp, int ip) {
    std::pair<int, int> inst_loc(fp, ip);
    if (this->inst_knockouts.count(inst_loc))
      this->inst_knockouts.erase(inst_loc);
    else
      this->inst_knockouts.insert(inst_loc);
  };

  std::function<void(int)> knockout_func = [this](int fp) {
    if (this->func_knockouts.count(fp))
      this->func_knockouts.erase(fp);
    else
      this->func_knockouts.insert(fp);
  };

  std::function<ProgramPosition(int, int)> original_to_built_space = [this](int fID, int iID) {
    std::pair<int, int> loc(fID, iID);
    ProgramPosition pos;
    if (!program_pos_map.count(loc)) {
      pos.fID(-1);
      pos.iID(-1);
    } else {
      pos.fID(program_pos_map.at(loc).first);
      pos.iID(program_pos_map.at(loc).second);
    }
    return pos;
  };

  std::function<double(int, int)> get_landscape_val = [this](int fID, int iID) {
    std::pair<int, int> loc(fID, iID);
    if (!landscape_map.count(loc)) return -1.0 * (size_t)-1;
    return landscape_map.at(loc);
  };


  std::function<void(std::string)> on_program_select = [this](std::string name) {
    DisplayProgram(name);
  };

  void InitializeVariables() {
    JSWrap(knockout_func, "knockout_func");
    JSWrap(knockout_inst, "knockout_inst");
    JSWrap(is_knockedout, "is_code_knockedout");
    JSWrap(original_to_built_space, "original_to_built_prog_space");
    JSWrap(get_landscape_val, "get_landscape_val");
    JSWrap(on_program_select, "on_program_select");
    JSWrap([this](){ return this->program_data->GetID(); }, "get_prog_data_obj_id");
    JSWrap([this](){ return this->GetSVG()->GetID(); }, "get_prog_vis_svg_obj_id");
    EM_ASM({ window.addEventListener("resize", resizeProgVis); });
    GetSVG()->Move(0, 0);
  }

  void SetProgramData(const std::string & name) {
    emp_assert(Has(program_map, name));
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

public:
  Ptr<D3::JSONDataset> program_data;

  EventDrivenGP_ProgramVis() : D3Visualization(1, 1) { ; }

  void Setup() {
    InitializeVariables();
    this->init = true;
    this->pending_funcs.Run();
  }

  void ResetKnockouts() {
    inst_knockouts.clear();
    func_knockouts.clear();
  }

  void ResetLandscaping() {
    program_pos_map.clear();
    landscape_map.clear();
  }

  Ptr<D3::JSONDataset> GetDataset() { return program_data; }

  // void LoadDataFromFile(std::string filename) {
  //   if (this->init) {
  //     program_data->LoadDataFromFile(filename, [this](){ std::cout << "On load?" << std::endl; });
  //   } else {
  //     this->pending_funcs.Add([this, filename]() {
  //       program_data->LoadDataFromFile(filename, [this](){ std::cout << "On load?" << std::endl; });
  //     });
  //   }
  // }

  void BuildCurProgram() {
    emp_assert(Has(program_map, display_program));
    if (cur_program) cur_program.Delete();
    const program_t & ref_program = program_map.at(display_program);
    cur_program = NewPtr<program_t>(ref_program.inst_lib);
    // Building a new program, reset landscaping.
    ResetLandscaping();
    // Build cur_program up, knocking out appropriate functions/instructions
    for (size_t fID = 0; fID < ref_program.GetSize(); ++fID) {
      if (func_knockouts.count(fID)) continue;
      fun_t new_fun = fun_t(ref_program[fID].affinity);
      for (size_t iID = 0; iID < ref_program[fID].GetSize(); ++iID) {
        if (inst_knockouts.count(std::make_pair<int, int>(fID, iID))) continue;
        program_pos_map.insert(std::make_pair(std::make_pair(fID, iID),
                                              std::make_pair(cur_program->GetSize(), new_fun.GetSize())
                                              )); // Lets me recover original position given cur program position.
        new_fun.inst_seq.emplace_back(ref_program[fID].inst_seq[iID]);
      }
      // Add new function to program if not empty.
      if (new_fun.GetSize()) cur_program->PushFunction(new_fun);
    }
    std::cout << "Built program: " << std::endl;
    cur_program->PrintProgram();
  }

  Ptr<program_t> GetCurProgram() { return cur_program; }

  void AddProgram(const std::string & _name, const program_t & _program) {
    program_map.emplace(_name, _program);
  }

  void SetEvalProgramFun(std::function<double(Ptr<program_t>)> eval_fun) {
    eval_program = eval_fun;
  }

  // @amlalejini - TODO
  void Clear() {

  }

  void Start(std::string name = "") {
    std::cout << "Run" << std::endl;
    if (name == "") return;
    if (this->init) {
      DisplayProgram(name);
    } else {
      this->pending_funcs.Add([this, name]() { DisplayProgram(name); });
    }
  }

  void DisplayProgram(const std::string & name) {
    emp_assert(Has(program_map, name));
    std::cout << "Display program: " << name << std::endl;
    display_program = name;
    // Reset knockouts
    ResetKnockouts();
    // Reset Landscaping.
    ResetLandscaping();
    // Set program data to correct program
    SetProgramData(name);
    // Draw program from program data.
    DrawProgram();
  }

  // @amlalejini - TODO
  void DrawProgram() {
    if (!program_data) return;
    EM_ASM({
      var program_data_obj_id = emp.get_prog_data_obj_id();
      var svg_obj_id = emp.get_prog_vis_svg_obj_id();
      if (program_data_obj_id == 255) return; // TODO: make this more robust.
      var program_data = js.objects[program_data_obj_id][0];
      var svg = js.objects[svg_obj_id];
      var prg_name = program_data["name"];
      var iblk_h= 20;
      var iblk_w = 65;
      var fblk_w = 100;
      var fitblk_w = 10;
      var iblk_lpad = fblk_w - (iblk_w + fitblk_w);
      var txt_lpad = 2;

      var vis_w = d3.select("#program-vis")[0][0].clientWidth;
      var x_domain = Array(0, 100);
      var x_range = Array(0, vis_w);
      var xScale = d3.scale.linear().domain(x_domain).range(x_range);

      // Compute program display height.
      // @amlalejini - TODO: make this a function.
      var prg_h = program_data["functions"].length * iblk_h;
      for (fID = 0; fID < program_data["functions"].length; fID++) {
        prg_h += program_data["functions"][fID].sequence_len * iblk_h;
      }
      // Reconfigure vis size based on program data.
      svg.selectAll("*").remove();
      svg.attr({"width": xScale(fblk_w), "height": prg_h});
      // Set program name.
      d3.select("#select_prog_dropdown_btn").text("Current Program: " + prg_name);
      // Add a group for each function.
      var functions = svg.selectAll("g").data(program_data["functions"]);
      functions.enter().append("g");
      functions.exit().remove();
      functions.attr({"class": "program-function",
                      "knockout": function(func, fID) {
                        var ko = (emp.is_code_knockedout(fID, -1)) ? "true" : "false";
                        return ko;
                      },
                      "transform": function(func, fID) {
                        var x_trans = xScale(0);
                        var y_trans = (fID * iblk_h + func.cumulative_seq_len * iblk_h);
                        return "translate(" + x_trans + "," + y_trans + ")";
                      }});
      var min_fsize = -1;
      functions.selectAll(".function-def-rect").remove();
      functions.selectAll(".function-def-txt").remove();
      functions.append("rect")
               .attr({
                 "class": "function-def-rect",
                 "width": xScale(fblk_w),
                 "height": iblk_h,
                 "knockout": function(func, fID) {
                   var ko = (emp.is_code_knockedout(fID, -1)) ? "true" : "false";
                   return ko;
                 }
                })
               .on("click", on_func_click);
      functions.append("text")
               .attr({
                 "class": "function-def-txt",
                 "x": txt_lpad,
                 "y": iblk_h,
                 "pointer-events": "none"
               })
               .text(function(func, fID) { return "fn-" + fID + " " + func.affinity + ":"; })
               .style("font-size", "1px")
               .each(function(d) {
                 var box = this.getBBox();
                 var pbox = this.parentNode.getBBox();
                 var fsize = Math.min(pbox.width/box.width, pbox.height/box.height)*0.9;
                 if (min_fsize == -1 || fsize < min_fsize) {
                   min_fsize = fsize;
                 }
               })
               .style("font-size", min_fsize)
               .each(function(d) {
                 var box = this.getBBox();
                 var pbox = this.parentNode.getBBox();
                 d.shift = ((pbox.height - box.height)/2);
               })
               .attr({
                 "dy": function(d) { return (-1 * (d.shift + 2)) + "px"; }
               });
      // Draw instructions for each function.
      functions.each(function(func, fID) {
        var instructions = d3.select(this).selectAll("g").data(func.sequence);
        instructions.enter().append("g");
        instructions.exit().remove();
        instructions.attr({
                          "class": "program-instruction",
                          "transform": function(d, i) {
                            var x_trans = xScale(iblk_lpad);
                            var y_trans = (iblk_h + i * iblk_h);
                            return "translate(" + x_trans + "," + y_trans + ")";
                          },
                          "knockout": function(inst, iID) { return emp.is_code_knockedout(fID, iID) ? "true" : "false"; }
                        });
        instructions.append("rect")
                    .attr({
                      "class": "program-instruction-blk",
                      "width": function(d) { d.w = xScale(iblk_w); return d.w; },
                      "height": iblk_h,
                      "knockout": function(inst, iID) { return emp.is_code_knockedout(fID, iID) ? "true" : "false"; }
                    })
                    .on("click", on_inst_click);
        var min_fsize = -1;
        instructions.append("text")
                    .attr({
                      "class": "program-instruction-txt",
                      "x": txt_lpad,
                      "y": iblk_h,
                      "pointer-events": "none"
                    })
                    .text(function(d, i) {
                      var inst_str = d.name;
                      if (d.has_affinity) inst_str += " " + d.affinity;
                      for (arg = 0; arg < d.num_args; arg++) inst_str += " " + d.args[arg];
                      return inst_str;
                    })
                    .style("font-size", "1px")
                    .each(function(d) {
                      var box = this.getBBox();
                      var pbox = this.parentNode.getBBox();
                      var fsize = Math.min(pbox.width/box.width, pbox.height/box.height)*0.9;
                      if (min_fsize == -1 || fsize < min_fsize) {
                        min_fsize = fsize;
                      }
                    })
                    .style("font-size", min_fsize)
                    .each(function(d) {
                      var box = this.getBBox();
                      var pbox = this.parentNode.getBBox();
                      d.shift = ((pbox.height - box.height)/2);
                    })
                    .attr({
                      "dy": function(d) { return (-1 * (d.shift + 2)) + "px"; }
                    });
        // Fitness contribution indicator.
        instructions.append("rect")
                    .attr({
                      "class": "fitness-contribution-blk",
                      "x": xScale(iblk_w),
                      "width": xScale(fitblk_w),
                      "height": iblk_h,
                      "fill": "white"
                    });
      });
    });
  }

  /// Landscape current program.
  void Landscape() {
    std::cout << "Program vis::Landscape" << std::endl;
    // Build current program.
    BuildCurProgram();
    // Get baseline fitness.
    double base_fitness = eval_program(cur_program);
    // Do single instruction knockouts.
    program_t & base_prog = *cur_program;
    landscape_map.insert(std::make_pair(std::make_pair(-1, -1), base_fitness));
    program_t ko_prog(base_prog);
    for (size_t fID = 0; fID < base_prog.GetSize(); ++fID) {
      pos_t func_loc(fID, -1); // Key for function knockout fitness value.
      for (size_t iID = 0; iID < base_prog[fID].GetSize(); ++iID) {
        pos_t inst_loc(fID, iID); // Key for instruction knockout fitness value.
        // Build program w/this location knocked out.
        ko_prog.SetInst(fID, iID, ko_prog.inst_lib->GetID("Nop"));
        // Evaluate program w/this location knocked out.
        double ko_fitness = eval_program(&ko_prog);
        // Store fitness value of program w/this location knocked out.
        landscape_map.insert(std::make_pair(inst_loc, ko_fitness));
        // Restore ko_prog.
        ko_prog.SetInst(fID, iID, base_prog[fID].inst_seq[iID]);
      }
    }
    // Update landscaping on visualization.
    EM_ASM({ landscapeProg(); });
  }
};

class EventDrivenGP_DemeVis : public D3Visualization {
public:
  struct HardwareDatum {
    EMP_BUILD_INTROSPECTIVE_TUPLE(int, role_id,
                                  int, loc,
                                  bool, knockedout
                                )
  };

private:
  emp::Ptr<Deme> cur_deme;
  emp::vector<HardwareDatum> deme_data;

  std::function<void(size_t)> knockout = [this](size_t id) {
    if (!this->cur_deme) return;
    if (this->cur_deme->knockouts.count(id))
      this->cur_deme->knockouts.erase(id);
    else
      this->cur_deme->knockouts.insert(id);
  };

  void InitializeVariables() {
    std::cout << "DemeVis Init vars" << std::endl;
    JSWrap(knockout, "deme_cell_knockout");
    EM_ASM( window.addEventListener("resize", resizeDemeVis); );
    GetSVG()->Move(0, 0);
  }

public:
  EventDrivenGP_DemeVis() : D3Visualization(1, 1) { ; }

  void Setup() {
    std::cout << "Deme vis setup running." << std::endl;
    InitializeVariables();
    this->init = true;
    this->pending_funcs.Run();
  }

  void Start(emp::Ptr<Deme> deme) {
    if (this->init) {
      DrawDeme(deme);
    } else {
      this->pending_funcs.Add([this, deme]() { DrawDeme(deme); });
    }
  }

  void DrawDeme(emp::Ptr<Deme> deme) {
    emp_assert(deme);
    cur_deme = deme; // @amlalejini TODO: clean this up. So hacky.
    // Resize deme data to match deme size.
    deme_data.resize(deme->grid.size());
    for (size_t i = 0; i < deme_data.size(); ++i) {
      deme_data[i].role_id(deme->grid[i]->GetTrait(TRAIT_ID__ROLE_ID));
      deme_data[i].loc(i);
      deme_data[i].knockedout((bool)deme->knockouts.count(i));
    }
    D3::Selection * svg = GetSVG();
    svg->SelectAll("g").Remove(); // Clean up old deme elements.
    svg->SelectAll("g").Data(deme_data)
                       .EnterAppend("g")
                       .SetAttr("class", "deme_cell");
    EM_ASM_ARGS({
      var svg = js.objects[$0];

      var deme_width = $1;
      var deme_height = $2;
      var txt_lpad = 2;

      var vis_w = d3.select("#deme-vis")[0][0].clientWidth;
      svg.attr({"deme-width": deme_width, "deme-height": deme_height, "width": vis_w, "height": vis_w});

      var cell_size = vis_w / deme_width;

      var cells = svg.selectAll("g");
      cells.attr({"transform": function(d, i) {
                      var x_trans = cell_size * (d.loc % deme_width);
                      var y_trans = cell_size * Math.floor(d.loc / deme_width);
                      return "translate(" + x_trans + "," + y_trans + ")";
                    },
                  "knockout": function(d) {
                    if (d.knockedout) return "true";
                    else return "false";
                  }
                  });
      cells.append("rect")
           .attr({
             "width": cell_size,
             "height": cell_size,
             "fill": "white",
             "stroke": "black"
           })
           .on("click", on_deme_cell_click);
      var min_fsize = -1;
      cells.append("text")
            .attr({"y": cell_size,
                   "x": txt_lpad,
                   "dy": "0.5em",
                   "pointer-events": "none"
                 })
            .text(function(d, i) { return "ID::" + d.role_id;})
            .style("font-size", "1px")
            .each(function(d) {
              var box = this.getBBox();
              var pbox = this.parentNode.getBBox();
              var fsize = Math.min(pbox.width/box.width, pbox.height/box.height)*0.9;
              if (min_fsize == -1 || fsize < min_fsize) {
                min_fsize = fsize;
              }
            })
            .style("font-size", min_fsize)
            .each(function(d) {
              var box = this.getBBox();
              var pbox = this.parentNode.getBBox();
              d.shift = ((pbox.height - box.height)/2);
            })
            .attr({"dy": function(d) { return (-1 * (d.shift + 2)) + "px"; }});

    }, svg->GetID(), deme->GetWidth(), deme->GetHeight());

  }

};

}
}

namespace web = emp::web;

class Application {
private:
  emp::Ptr<emp::Random> random;

  // Localized parameter values.
  // -- General --
  int random_seed;
  size_t deme_width;
  size_t deme_height;
  size_t deme_size;
  size_t deme_eval_time;
  size_t cur_time;

  // Interface-specific objects.
  web::EventDrivenGP_ProgramVis program_vis;
  web::EventDrivenGP_DemeVis deme_vis;

  web::Document program_vis_doc;
  web::Document deme_vis_doc;
  web::Document vis_dash;

  // Animation
  web::Animate anim;

  // Simulation/evaluation objects.
  emp::Ptr<Deme> eval_deme;
  emp::Ptr<Agent> eval_agent;
  emp::Ptr<Deme> landscape_deme;
  emp::Ptr<Agent> landscape_agent;
  emp::Ptr<event_lib_t> event_lib;
  emp::Ptr<inst_lib_t> inst_lib;

  std::function<void(std::string, std::string)> read_prog_from_str = [this](std::string prog_name, std::string prog_str) {
    std::istringstream in_prog(prog_str);
    LoadProgram(prog_name, in_prog);
    std::cout << "Successfully loaded program!" << std::endl;
  };

  std::function<double(Deme*)> fit_fun =
    [this](Deme * deme) {
      if (deme == nullptr) { return 0.0; }
      std::unordered_set<double> valid_uids;
      double valid_id_cnt = 0;
      for (size_t i = 0; i < deme->grid.size(); ++i) {
        const double role_id = deme->grid[i]->GetTrait(TRAIT_ID__ROLE_ID);
        if (role_id > 0 && role_id <= deme_size) {
          ++valid_id_cnt; // Increment valid id cnt.
          valid_uids.insert(role_id); // Add to set.
        }
      }
      return (valid_id_cnt >= deme->grid.size()) ? (valid_id_cnt + (double)valid_uids.size()) : (valid_id_cnt);
    };

public:
  Application()
    : random(),
      program_vis(),
      deme_vis(),
      program_vis_doc("program-vis"),
      deme_vis_doc("deme-vis"),
      vis_dash("vis-dashboard"),
      anim([this]() { Application::Animate(anim); } ),
      eval_deme(),
      eval_agent(),
      landscape_deme(),
      landscape_agent(),
      event_lib(),
      inst_lib()
  {

    // Localize parameter values.
    random_seed = DEFAULT_RANDOM_SEED;
    deme_width = DIST_SYS_WIDTH;
    deme_height = DIST_SYS_HEIGHT;
    deme_size = deme_width * deme_height;
    deme_eval_time = EVAL_TIME;
    cur_time = 0;

    // Create random number generator.
    random = emp::NewPtr<emp::Random>(random_seed);
    // Confiigure instruction set/event library.
    event_lib = emp::NewPtr<event_lib_t>(*emp::EventDrivenGP::DefaultEventLib());
    inst_lib = emp::NewPtr<inst_lib_t>(*emp::EventDrivenGP::DefaultInstLib());
    inst_lib->AddInst("GetRoleID", Inst_GetRoleID, 1, "Local memory[Arg1] = Trait[RoleID]");
    inst_lib->AddInst("SetRoleID", Inst_SetRoleID, 1, "Trait[RoleID] = Local memory[Arg1]");
    inst_lib->AddInst("GetXLoc", Inst_GetXLoc, 1, "Local memory[Arg1] = Trait[XLoc]");
    inst_lib->AddInst("GetYLoc", Inst_GetYLoc, 1, "Local memory[Arg1] = Trait[YLoc]");

    // Configure evaluation deme.
    eval_deme = emp::NewPtr<Deme>(random, deme_width, deme_height, event_lib, inst_lib);
    // Need a separate deme for landscaping.
    landscape_deme = emp::NewPtr<Deme>(random, deme_width, deme_height, event_lib, inst_lib);

    // Add program visualization to page.
    program_vis_doc << program_vis;
    deme_vis_doc << deme_vis;

    // Add dashboard components to page.
    emp::JSWrap([this]() { this->RunCurProgram(); }, "run_program");
    emp::JSWrap([this]() { this->DoReset(); }, "reset_application");
    emp::JSWrap([this]() { this->DoLandscape(); }, "landscape_program");
    emp::JSWrap(read_prog_from_str, "read_prog_from_str");

    vis_dash  << "<div class='row'>"
                << "<div class='col'>"
                  << "<div class='btn-group' role='group'>"
                    << "<button id='run_program_button' onclick='emp.run_program()' class='btn btn-primary'>Run</button>"
                    << "<button id='landscape_button' onclick='emp.landscape_program()' class='btn btn-primary'>Landscape</button>"
                    << "<button id='reset_button' onclick='emp.reset_application()' class='btn btn-primary'>Reset</button>"
                  << "</div>"
                << "</div>"
              << "</div>"
              << "<div class='row justify-content-center pad-top-row'>"
                << "<div class='col'>"
                  << "<h3>Update: <span class=\"badge badge-default\">" << web::Live([this]() { return this->cur_time; }) << "</span></h3>"
                << "</div>"
                << "<div class='col'>"
                  << "<h3>Fitness: <span class=\"badge badge-default\">" << web::Live([this]() { return this->fit_fun(eval_deme); }) << "</span></h3>"
                << "</div>"
              << "</div>";
    // Some interface setup.
    // EM_ASM({
    // });

    // Configure program visualization.
    // Define a convenient affinity table.
    emp::vector<affinity_t> affinity_table(256);
    for (size_t i = 0; i < affinity_table.size(); ++i) {
      affinity_table[i].SetByte(0, (uint8_t)i);
    }
    // Test program.
    program_t test_program(inst_lib);
    test_program.PushFunction(fun_t());
    for (size_t i=0; i < 5; ++i) test_program.PushInst("Nop");
    test_program.PushInst("GetXLoc", 0);
    test_program.PushInst("SetRoleID", 0);
    test_program.PushFunction(fun_t());
    for (size_t i=0; i < 5; ++i) test_program.PushInst("Nop");
    test_program.PushInst("Call", 0, 0, 0, affinity_t());
    test_program.PushInst("Inc", 1);
    test_program.PushInst("Add", 0, 1, 5);
    DoAddProgram("Test", test_program);
    // Another test program.
    program_t prog2(inst_lib);
    prog2.PushFunction(fun_t(affinity_table[166]));
    prog2.PushInst("Call", 0, 0, 0, affinity_table[66]);
    prog2.PushInst("SwapMem", 5, 6);
    prog2.PushInst("Call", 0, 0, 0, affinity_table[164]);
    prog2.PushInst("Call", 0, 0, 0, affinity_table[240]);
    prog2.PushInst("TestLess", 0, 7, 0);
    prog2.PushInst("Call", 0, 0, 0, affinity_table[183]);
    prog2.PushInst("Inc", 4);
    prog2.PushInst("SetRoleID", 0);
    prog2.PushFunction(fun_t(affinity_table[187]));
    prog2.PushInst("Pull", 4, 0);
    prog2.PushInst("Nop");
    prog2.PushInst("Nop");
    prog2.PushInst("BroadcastMsg", 0, 0, 0, affinity_table[216]);
    prog2.PushInst("BroadcastMsg", 0, 0, 0, affinity_table[139]);
    prog2.PushInst("GetRoleID", 0);
    prog2.PushInst("Nop");
    prog2.PushInst("Add", 0, 0, 0);
    prog2.PushInst("Inc", 0);
    prog2.PushInst("SetRoleID", 0);
    prog2.PushFunction(fun_t(affinity_table[50]));
    prog2.PushInst("Call", 0, 0, 0, affinity_table[232]);
    prog2.PushInst("SetMem", 0, 0);
    prog2.PushInst("TestEqu", 0, 0, 0);
    prog2.PushInst("BroadcastMsg", 0, 0, 0, affinity_table[225]);
    prog2.PushInst("Pull", 7, 0);
    prog2.PushInst("GetRoleID", 0);
    prog2.PushInst("Break");
    prog2.PushInst("Inc", 0);
    prog2.PushInst("SetRoleID", 0);
    DoAddProgram("Test2", prog2);

    program_vis.SetEvalProgramFun([this](emp::Ptr<program_t> prog_ptr) {
      // 1) Load program into deme.
      if (landscape_agent) landscape_agent.Delete();
      landscape_agent = emp::NewPtr<Agent>(*prog_ptr);
      this->landscape_deme->LoadAgent(landscape_agent);
      //  - Configure landscape deme knockouts:
      this->landscape_deme->knockouts = this->eval_deme->knockouts;
      // 2) Run deme.
      for (size_t ls_time = 0; ls_time < EVAL_TIME; ++ls_time) {
        this->landscape_deme->SingleAdvance();
      }
      // 3) Evaluate deme fitness.
      return this->fit_fun(this->landscape_deme);
    });
    // Start the visualization.
    program_vis.Start("Test2");
    program_vis.On("resize", [this]() { std::cout << "On program vis resize!" << std::endl; });
    // Configure deme visualization.
    deme_vis.Start(eval_deme);
  }

  void DoReset() {
    if (anim.GetActive()) anim.Stop();
    cur_time = 0;

    program_vis.ResetKnockouts();
    program_vis.DrawProgram();

    eval_deme->knockouts.clear();
    eval_deme->Reset();
    deme_vis.DrawDeme(eval_deme);

    vis_dash.Redraw();
  }

  void DoFinishEval() {
    std::cout << "Finish eval" << std::endl;
    anim.Stop();
    // eval_deme->Print();
  }

  void Animate(const web::Animate & anim) {
    std::cout << "Cur time: " << cur_time << std::endl;
    if (cur_time >= deme_eval_time) {
      DoFinishEval();
      return;
    }
    eval_deme->SingleAdvance();
    ++cur_time;
    vis_dash.Redraw();
    deme_vis.DrawDeme(eval_deme);
  }

  void RunCurProgram() {
    std::cout << "Run cur program!" << std::endl;
    if (anim.GetActive()) anim.Stop();
    // Build current program.
    program_vis.BuildCurProgram();
    emp::Ptr<program_t> cur_prog = program_vis.GetCurProgram();
    if (cur_prog->GetSize() == 0) {
      std::cout << "Warning! Empty program!" << std::endl;
      return;
    }
    // Configure eval agent.
    if (eval_agent) eval_agent.Delete();
    eval_agent = emp::NewPtr<Agent>(*cur_prog);
    cur_time = 0;
    // Load eval agent into deme.
    eval_deme->LoadAgent(eval_agent);
    // Evaluate deme.
    anim.Start();
  }

  void DoLandscape() {
    std::cout << "Landscape cur program (and deme, etc)!" << std::endl;
    program_vis.Landscape();
  }

  void DoAddProgram(const std::string & _name, const program_t & _program) {
    program_vis.AddProgram(_name, _program);
    // Update dropdown menu.
    EM_ASM_ARGS({
      program_name_set.add(Pointer_stringify($0));
      updateProgramSelection();
    }, _name.c_str());
  }

  /// Here's a horrible, inefficient, and unforgiving load program function that definitely does not fail gracefully.
  void LoadProgram(std::string prog_name, std::istream & input) {
    program_t prog(inst_lib);
    std::string cur_line;
    emp::vector<std::string> line_components;

    while (!input.eof()) {
      std::getline(input, cur_line);
      emp::left_justify(cur_line); // Clear out leading whitespace.

      if (cur_line == "") continue; // Skip empty lines.

      //std::string command = emp::string_pop_word(cur_line);
      emp::slice(cur_line, line_components, '-');
      if (line_components[0] == "Fn") {
        line_components.resize(0);
        emp::slice(cur_line, line_components, ' ');
        std::string aff_str = line_components[1];
        // Extract function affinity.
        affinity_t fun_aff;
        for (size_t i = 0; i < aff_str.size(); ++i) {
          if (i >= fun_aff.GetSize()) break;
          if (aff_str[i] == '1') fun_aff.Set(fun_aff.GetSize() - i - 1, true);
        }
        // Push a new function onto the program.
        prog.PushFunction(fun_t(fun_aff));
      } else {
        line_components.resize(0);
        emp::slice(cur_line, line_components, ' ');
        std::string inst_name = line_components[0];
        size_t inst_id = inst_lib->GetID(inst_name);
        bool has_affinity = inst_lib->HasProperty(inst_id, "affinity");
        int arg = 1;
        affinity_t inst_aff;
        if (has_affinity) {
          std::string aff_str = line_components[1];
          ++arg;
          for (size_t i = 0; i < aff_str.size(); ++i) {
            if (i >= inst_aff.GetSize()) break;
            if (aff_str[i] == '1') inst_aff.Set(inst_aff.GetSize() - i - 1, true);
          }
        }
        int arg0 = 0;
        int arg1 = 0;
        int arg2 = 0;
        if (arg < line_components.size()) {
          arg0 = std::stoi(line_components[arg]); ++arg;
        }
        if (arg < line_components.size()) {
          arg1 = std::stoi(line_components[arg]); ++arg;
        }
        if (arg < line_components.size()) {
          arg2 = std::stoi(line_components[arg]); ++arg;
        }
        prog.PushInst(inst_id, arg0, arg1, arg2, inst_aff);
      }

    }
    std::cout << "Look, mah! Here's the program I made: " << std::endl;
    prog.PrintProgram();
    DoAddProgram(prog_name, prog);
    //return prog;
  }

};

emp::Ptr<Application> app;

int main(int argc, char *argv[]) {
  emp::Initialize();
  app = emp::NewPtr<Application>();
  std::cout << "Right below app init." << std::endl;
  return 0;
}
