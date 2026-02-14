#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <tikzgeom_algorithms.H>

using namespace Aleph;

namespace
{

Polygon make_funnel_polygon()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(24, 0));
  p.add_vertex(Point(24, 20));
  p.add_vertex(Point(14, 20));
  p.add_vertex(Point(14, 8));
  p.add_vertex(Point(10, 8));
  p.add_vertex(Point(10, 20));
  p.add_vertex(Point(0, 20));
  p.close();
  return p;
}

std::string step_event(const FunnelTraceStep & step)
{
  if (step.emitted_left) return "emit-left";
  if (step.emitted_right) return "emit-right";
  if (step.tightened_left and step.tightened_right) return "tighten-both";
  if (step.tightened_left) return "tighten-left";
  if (step.tightened_right) return "tighten-right";
  return "noop";
}

std::string fmt_point(const Point & p)
{
  std::ostringstream ss;
  ss << "(" << std::fixed << std::setprecision(2)
     << geom_number_to_double(p.get_x()) << ","
     << geom_number_to_double(p.get_y()) << ")";
  return ss.str();
}

std::string render_trace_step_tikz(const Polygon & polygon,
                                   const Point & source,
                                   const Point & target,
                                   const FunnelTraceResult & trace,
                                   const size_t step_index)
{
  Tikz_Plane plane(178, 108, 0, 0);
  plane.put_cartesian_axis();
  plane.set_point_radius_mm(0.70);

  put_funnel_trace_step(
      plane,
      polygon,
      source,
      target,
      trace,
      step_index,
      tikz_area_style("black", "gray!15", 0.22),
      tikz_points_style("green!50!black"),
      tikz_points_style("blue"),
      tikz_wire_style("purple", true),
      tikz_path_style("purple"),
      tikz_path_style("orange!90!black"),
      tikz_path_style("red"),
      true,
      tikz_points_style("red"));

  std::ostringstream ss;
  plane.draw(ss, true);
  return ss.str();
}

std::string render_final_tikz(const Polygon & polygon,
                              const Point & source,
                              const Point & target)
{
  Tikz_Plane plane(178, 108, 0, 0);
  plane.put_cartesian_axis();
  plane.set_point_radius_mm(0.70);

  visualize_shortest_path_with_portals(
      plane, polygon, source, target, ShortestPathInPolygon());

  std::ostringstream ss;
  plane.draw(ss, true);
  return ss.str();
}

std::string render_trace_step_panel(const FunnelTraceResult & trace,
                                    const size_t step_index)
{
  const FunnelTraceStep & step = trace.steps(step_index);

  std::ostringstream ss;
  ss << "\\small\\textbf{Step " << (step_index + 1) << "/" << trace.steps.size()
     << "}\\\\\n"
     << "\\footnotesize event: \\texttt{" << step_event(step) << "}\\\\\n"
     << "portal: " << step.portal_index << "\\\\[1.2mm]\n"
     << "\\begin{tabular}{@{}ll@{}}\n"
     << "Apex & " << fmt_point(step.apex) << "\\\\\n"
     << "Left & " << fmt_point(step.left_boundary) << "\\\\\n"
     << "Right & " << fmt_point(step.right_boundary) << "\\\\\n"
     << "L-portal & " << fmt_point(step.portal_left) << "\\\\\n"
     << "R-portal & " << fmt_point(step.portal_right) << "\\\\\n"
     << "Committed & " << step.committed_path.size() << " pts\\\\\n"
     << "\\end{tabular}";

  return ss.str();
}

std::string render_final_panel(const FunnelTraceResult & trace)
{
  std::ostringstream ss;
  ss << "\\small\\textbf{Final Path}\\\\\n"
     << "\\footnotesize frames: " << (trace.steps.size() + 1) << "\\\\\n"
     << "portals: " << trace.portals.size() << "\\\\\n"
     << "path nodes: " << trace.final_path.size() << "\\\\[1.2mm]\n"
     << "\\begin{tabular}{@{}ll@{}}\n"
     << "Source & " << fmt_point(trace.final_path(0)) << "\\\\\n"
     << "Target & " << fmt_point(trace.final_path(trace.final_path.size() - 1))
     << "\\\\\n"
     << "\\end{tabular}";

  return ss.str();
}

void write_twocol_frame(std::ostream & out,
                        const std::string & title,
                        const std::string & left,
                        const std::string & right)
{
  out << "\\begin{frame}[t]{" << title << "}\n"
      << "\\begin{columns}[T,totalwidth=\\textwidth]\n"
      << "\\begin{column}{0.70\\textwidth}\n"
      << left << '\n'
      << "\\end{column}\n"
      << "\\begin{column}{0.29\\textwidth}\n"
      << right << '\n'
      << "\\end{column}\n"
      << "\\end{columns}\n"
      << "\\end{frame}\n\n";
}

} // namespace

int main(int argc, char * argv[])
{
  const std::string output_path =
      argc > 1 ? argv[1] : "tikz_funnel_beamer_handout_example.tex";

  std::ofstream out(output_path);
  if (not out)
    {
      std::cerr << "Cannot open output file: " << output_path << '\n';
      return 1;
    }

  const Polygon polygon = make_funnel_polygon();
  const Point source(2, 16);
  const Point target(22, 16);

  const FunnelTraceResult trace =
      compute_shortest_path_funnel_trace(polygon, source, target);

  out << "\\documentclass[handout]{beamer}\n"
      << "\\usepackage{tikz}\n"
      << "\\setbeamertemplate{navigation symbols}{}\n"
      << "\\begin{document}\n\n";

  if (trace.steps.size() == 0)
    {
      write_twocol_frame(
          out,
          "Shortest Path Funnel Trace (Handout)",
          render_final_tikz(polygon, source, target),
          "\\small\\textbf{No funnel iterations}\\\\\\n"
          "Direct/degenerate case");
    }
  else
    {
      for (size_t i = 0; i < trace.steps.size(); ++i)
        {
          const std::string title =
              "Shortest Path Funnel Trace (Handout) - Step " +
              std::to_string(i + 1);
          write_twocol_frame(
              out,
              title,
              render_trace_step_tikz(polygon, source, target, trace, i),
              render_trace_step_panel(trace, i));
        }

      write_twocol_frame(
          out,
          "Shortest Path Funnel Trace (Handout) - Final",
          render_final_tikz(polygon, source, target),
          render_final_panel(trace));
    }

  out << "\\end{document}\n";

  std::cout << "Generated " << output_path << '\n'
            << "Frames: " << (trace.steps.size() + 1) << '\n'
            << "Compile with: pdflatex " << output_path << '\n';

  return 0;
}
