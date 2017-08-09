var program_name_set = new Set();
var prog_reader = new FileReader();
var control = document.getElementById("prog-file-selector");
prog_reader.onload = function(){ emp.read_prog_from_str(control.files[0].name, prog_reader.result); };
control.addEventListener("change", function(event) { prog_reader.readAsText(control.files[0]); }, false);

var on_inst_click = function(inst, i) {
  emp.knockout_inst(inst.function, inst.position);
  var inst_blk = d3.select(this);
  if (inst_blk.attr("knockout") == "false") {
    inst_blk.attr("knockout", "true");
    d3.select(this.parentNode).attr("knockout", "true");
  } else {
    inst_blk.attr("knockout", "false");
    d3.select(this.parentNode).attr("knockout", "false");
  }
};

var on_func_click = function(func, i) {
  emp.knockout_func(i);
  var func_def = d3.select(this);
  if (func_def.attr("knockout") == "false") {
    func_def.attr("knockout", "true");
    d3.select(this.parentNode).attr("knockout", "true");
  } else {
    func_def.attr("knockout", "false");
    d3.select(this.parentNode).attr("knockout", "false");
  }
};

var on_deme_cell_click = function(d, i) {
  emp.deme_cell_knockout(i);
  var cell = d3.select(this.parentNode);
  if (!d.knockedout) {
    cell.attr("knockout", "true");
    d.knockedout = true;
  } else {
    cell.attr("knockout", "false");
    d.knockedout = false;
  }
}

var updateProgramSelection = function() {
  var menu = $("#select_prog_dropdown_menu");
  menu.empty();
  $.each(Array.from(program_name_set), function(i, v) {
    menu.append($("<button type='button' class='dropdown-item' onclick='emp.on_program_select(\"" + v + "\")'></button>").html(v).val(v));
    // $('#select_prog_dropdown_menu').append($('<option></option>').val("hello"+i).html("hello"+i));
  });
}

// var onProgramSelect = function(name) {
//   console.log(name);
//   //console.log($(this).attr("value"));
// }

var landscapeProg = function() {
  var max_del_lscolor = "#b2182b";
  var max_ben_lscolor = "#2166ac";
  var neutral_lscolor = "grey";
  var lethal_lscolor = "purple";
  var cScale = d3.scale.linear().domain([0, 1.0, 2.0]).range([max_del_lscolor, neutral_lscolor, max_ben_lscolor]);

  var prog_vis = d3.select("#program-vis");
  var svg = prog_vis.select("svg");
  var functions = svg.selectAll(".program-function");
  functions.each(function(func, fID) {
    // var instructions = d3.select(this).selectAll(".program-instruction");
    var ls_blks = d3.select(this).selectAll(".fitness-contribution-blk");
    ls_blks.attr({
      "fill": function(d, i) {
        if (emp.is_code_knockedout(fID, i)) {
          return "black";
        } else {
          var built_loc = emp.original_to_built_prog_space(fID, i);
          if (built_loc.fID == -1 && built_loc.iID == -1) return "black";
          // Terrible and hacky color coding. (no support for negative fitness).
          var base_fitness = emp.get_landscape_val(-1, -1);
          var ko_fitness = emp.get_landscape_val(built_loc.fID, built_loc.iID);

          if (base_fitness < 0.0) base_fitness = 0.0;
          if (ko_fitness < 0.0) ko_fitness = 0.0;
          if (base_fitness == 0.0 && base_fitness == 0.0) return cScale(1.0);
          if (base_fitness == 0.0) return cScale(2.0);
          return cScale(ko_fitness / base_fitness);
        }
      }
    });
  });
}

// Ran into some weird bugs... just redraw the entire thing for now.
var resizeProgVis = function() {
  var prog_vis = d3.select("#program-vis");
  var svg = prog_vis.select("svg");

  var iblk_h= 20;
  var iblk_w = 65;
  var fblk_w = 100;
  var fitblk_w = 10;
  var iblk_lpad = fblk_w - (iblk_w + fitblk_w);
  var txt_lpad = 2;

  var vis_w = prog_vis[0][0].clientWidth;

  var x_domain = Array(0, 100);
  var x_range = Array(0, vis_w);
  var xScale = d3.scale.linear().domain(x_domain).range(x_range);

  // Update width/x attributes (height/y never changes with resize).
  svg.attr({"width": xScale(fblk_w)});
  var functions = svg.selectAll(".program-function");
  functions.attr({
    "transform": function(func, fID) {
      var x_trans = xScale(0);
      var y_trans = (fID * iblk_h + func.cumulative_seq_len * iblk_h);
      return "translate(" + x_trans + "," + y_trans + ")";
    }
  });
  var min_fsize = -1;
  var func_blks = functions.selectAll(".function-def-rect");
  func_blks.attr({"width": xScale(fblk_w)});
  var func_txt = functions.selectAll(".function-def-txt");
  func_txt.style("font-size", "1px");
  func_txt.each(function(d) {
    var box = this.getBBox();
    var rbox = $(".function-def-rect")[0].getBBox();
    var fsize = Math.min(rbox.width/box.width, rbox.height/box.height)*0.9;
    if (min_fsize == -1 || fsize < min_fsize) {
      min_fsize = fsize;
    }
  });
  func_txt.style("font-size", min_fsize)
          .attr({
            "dy": function(d) { return (-1 * 2.0)+"px"; }//return (-1 * (d.shift + 2)) + "px"; }
          });

  // Resize instructions for each function.
  functions.each(function(func, fID) {
    var instructions = d3.select(this).selectAll(".program-instruction");
    instructions.attr({
      "transform": function(d, i) {
                            var x_trans = xScale(iblk_lpad);
                            var y_trans = (iblk_h + i * iblk_h);
                            return "translate(" + x_trans + "," + y_trans + ")";
                          }
    });
    var inst_blks = instructions.selectAll(".program-instruction-blk");
    inst_blks.attr({"width": xScale(iblk_w)});
    min_fsize = -1;
    var inst_txt = instructions.selectAll(".program-instruction-txt");
    inst_txt.style("font-size", "1px")
                  .each(function(d) {
                    var box = this.getBBox();
                    var pbox = this.parentNode.getBBox();
                    var fsize = Math.min(pbox.width/(box.width+1), pbox.height/box.height)*0.9;
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
    var ls_blks = instructions.selectAll(".fitness-contribution-blk");
    ls_blks.attr({
      "x": xScale(iblk_w),
      "width": xScale(fitblk_w)
    });
  });
}

var resizeDemeVis = function() {
  // update deme sizing.
  var deme_vis = d3.select("#deme-vis");
  var deme_svg = deme_vis.select("svg");
  var deme_vis_w = deme_vis[0][0].clientWidth;
  deme_svg.attr({"width": deme_vis_w, "height": deme_vis_w});
  var deme_width = deme_svg.attr("deme-width");
  var cell_size = deme_vis_w / deme_svg.attr("deme-width");
  var cells = deme_svg.selectAll("g");
  cells.attr({"transform": function(d, i) {
                  var x_trans = cell_size * (d.loc % deme_width);
                  var y_trans = cell_size * Math.floor(d.loc / deme_width);
                  return "translate(" + x_trans + "," + y_trans + ")";
                }
              });
  cells.selectAll("rect").attr({"width": cell_size, "height": cell_size});
  // Resize text.
  var cell_txt = cells.selectAll("text").attr({"y": cell_size});
  var min_fsize = -1;
  cell_txt.style("font-size", "1px")
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
}
