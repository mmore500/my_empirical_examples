
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
  console.log(d);
  console.log(cell);
  if (!d.knockedout) {
    cell.attr("knockout", "true");
    d.knockedout = true;
  } else {
    cell.attr("knockout", "false");
    d.knockedout = false;
  }
}

// Ran into some weird bugs... just redraw the entire thing for now.
var resizeProgVis = function() {
  // var prog_vis = d3.select("#program-vis");
  // var prog_svg = prog_vis.select("svg");
  // var prog_vis_w = prog_vis[0][0].clientWidth;
  // var x_domain = Array(0, 100);
  // var x_range = Array(0, prog_vis_w);
  // var xScale = d3.scale.linear().domain(x_domain).range(x_range);
  //
  // var iblk_h = 20;
  // var iblk_w = 75;
  // var fblk_w = 100;
  // var iblk_lpad = fblk_w - iblk_w;
  // var txt_lpad = 2;
  //
  // prog_svg.attr({"width": xScale(fblk_w)});
  // var functions = prog_svg.selectAll(".program-function");
  // functions.attr({
  //   "transform": function(func, fID) {
  //     var x_trans = xScale(0);
  //     var y_trans = (fID * iblk_h + func.cumulative_seq_len * iblk_h);
  //     return "translate(" + x_trans + "," + y_trans + ")";
  //   }
  // });
  //
  //
  // var fun_rects = functions.selectAll(".function-def-rect");
  // fun_rects.attr({"width": xScale(fblk_w)});
  //
  // var min_fsize = -1;
  // var fun_txt = functions.selectAll(".function-def-txt");
  // fun_txt.style("font-size", "1px")
  //         .each(function(d) {
  //           var box = this.getBBox();
  //           var pbox = this.parentNode.getBBox();
  //           var fsize = Math.min(pbox.width/box.width, iblk_h/box.height) * 0.9;
  //           if (min_fsize == -1 || fsize < min_fsize) {
  //             min_fsize = fsize;
  //           }
  //         })
  //         .style("font-size", min_fsize)
  //         .each(function(d) {
  //           var box = this.getBBox();
  //           d.shift = (iblk_h - box.height)/2;
  //         })
  //         .attr({
  //           "dy": function(d) { return (-1 * (d.shift + 2)) + "px"; }
  //         });

  var program_data_obj_id = emp.get_prog_data_obj_id();
  var svg_obj_id = emp.get_prog_vis_svg_obj_id();
  if (program_data_obj_id == 255) return; // TODO: make this more robust.
  var program_data = js.objects[program_data_obj_id][0];
  var svg = js.objects[svg_obj_id];
  var prg_name = program_data["name"];
  var iblk_h = 20;
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
  d3.select("#program-vis-head").text("Program: " + prg_name);
  // Add a group for each function.
  var functions = svg.selectAll("g").data(program_data["functions"]);
  functions.enter().append("g");
  functions.exit().remove();
  functions.attr({"class": "program-function",
                  "knockout": "false",
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
             "knockout": "false"
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
                      }});
    instructions.append("rect")
                .attr({
                  "width": function(d) { d.w = xScale(iblk_w); return d.w; },
                  "height": iblk_h,
                  "knockout": "false"
                })
                .on("click", on_inst_click);
    var min_fsize = -1;
    instructions.append("text")
                .attr({
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
                  "fill": "grey"
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
