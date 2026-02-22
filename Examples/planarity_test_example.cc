/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**
 * @file planarity_test_example.cc
 * @brief Example usage of planarity test on Aleph graphs.
 *
 * Build and run:
 *
 *   cmake -S . -B build
 *   cmake --build build --target planarity_test_example
 *   ./build/Examples/planarity_test_example
 */

# include <algorithm>
# include <cstdlib>
# include <filesystem>
# include <fstream>
# include <iostream>
# include <string>

# include <Planarity_Test.H>
# include <tpl_graph.H>

using namespace std;
using namespace Aleph;

namespace
{
  using UGraph = List_Graph<Graph_Node<string>, Graph_Arc<int>>;
  using DGraph = List_Digraph<Graph_Node<string>, Graph_Arc<int>>;
  using UDual = Default_Planar_Dual_Graph<UGraph>;

  /** Print planarity test results. */
  template <class GT>
  void print_result(const string & title,
                    const Planarity_Test_Result<GT> & r)
  {
    cout << title << '\n';
    cout << "  planar: " << (r.is_planar ? "yes" : "no") << '\n';
    cout << "  input nodes/arcs: " << r.num_nodes << "/" << r.num_input_arcs << '\n';
    cout << "  simplified nodes/edges: "
         << r.simplified_num_nodes << "/" << r.simplified_num_edges << '\n';
    cout << "  ignored loops: " << r.ignored_loops << '\n';
    cout << "  ignored parallel arcs: " << r.ignored_parallel_arcs << '\n';
    if (r.failed_euler_bound)
      cout << "  rejected by Euler bound before LR test\n";

    cout << "  embedding available: "
         << (r.has_combinatorial_embedding ? "yes" : "no") << '\n';
    if (r.embedding_search_truncated)
      cout << "  embedding search truncated by options\n";
    if (r.has_combinatorial_embedding)
      {
        cout << "  embedding faces: " << r.embedding_num_faces << '\n';
        cout << "  embedding mode: "
             << (r.embedding_is_lr_linear ? "LR-linear" : "fallback-exact")
             << '\n';
      }

    cout << "  non-planar certificate available: "
         << (r.has_nonplanar_certificate ? "yes" : "no") << '\n';
    if (r.certificate_search_truncated)
      cout << "  certificate search truncated by options\n";
    if (r.has_nonplanar_certificate)
      {
        cout << "  certificate type: "
             << Aleph::to_string(r.certificate_type) << '\n';
        cout << "  witness edges: "
             << r.certificate_obstruction_edges.size() << '\n';
        if (r.certificate_obstruction_edges.size() > 0)
          cout << "  first witness edge input-arc multiplicity: "
               << r.certificate_obstruction_edges[0].input_arcs.size() << '\n';
        if (r.certificate_type == Planarity_Certificate_Type::K5_Subdivision
            or r.certificate_type == Planarity_Certificate_Type::K33_Subdivision)
          {
            cout << "  branch nodes: " << r.certificate_branch_nodes.size() << '\n';
            cout << "  witness paths: " << r.certificate_paths.size() << '\n';
            if (r.certificate_paths.size() > 0)
              cout << "  first witness path edges: "
                   << r.certificate_paths[0].edges.size() << '\n';
          }
      }

    cout << '\n';
  }


  /** Print geometric drawing details. */
  template <class GT>
  void print_drawing(const Planar_Geometric_Drawing<GT> & d)
  {
    cout << "  geometric drawing available: "
         << (d.drawing_available ? "yes" : "no") << '\n';
    if (not d.drawing_available)
      return;

    cout << "  drawing crossings: " << d.crossing_count << '\n';
    cout << "  drawing validated no-crossings: "
         << (d.drawing_validated_no_crossings ? "yes" : "no") << '\n';
    cout << "  drawing relaxation iterations: " << d.relaxation_iterations << '\n';

    const size_t show = std::min(static_cast<size_t>(3), d.node_positions.size());
    for (size_t i = 0; i < show; ++i)
      {
        const auto & p = d.node_positions[i];
        cout << "  node[" << i << "] @" << p.x << ", " << p.y << '\n';
      }
  }
}


int main(int argc, char ** argv)
{
  namespace fs = std::filesystem;

  const char * env_out_dir = std::getenv("PLANARITY_OUT_DIR");
  const fs::path output_dir =
      (argc > 1 and argv[1] != nullptr and argv[1][0] != '\0')
          ? fs::path(argv[1])
          : ((env_out_dir != nullptr and env_out_dir[0] != '\0')
                 ? fs::path(env_out_dir)
                 : fs::path("/tmp"));

  std::error_code mkdir_ec;
  fs::create_directories(output_dir, mkdir_ec);
  if (mkdir_ec)
    cout << "warning: could not ensure output directory exists: "
         << output_dir.string() << '\n';

  Planarity_Test_Options opts;
  opts.compute_embedding = true;
  opts.compute_nonplanar_certificate = true;
  Planarity_Test_Options strict_lr_opts = opts;
  strict_lr_opts.embedding_allow_bruteforce_fallback = false;

  cout << "Planarity test on Aleph graphs\n";
  cout << "=============================\n\n";
  cout << "Certificate output directory: " << output_dir.string() << "\n\n";

  {
    UGraph g;
    auto * a = g.insert_node("A");
    auto * b = g.insert_node("B");
    auto * c = g.insert_node("C");
    auto * d = g.insert_node("D");
    auto * e = g.insert_node("E");

    // Planar graph (pentagon + one chord)
    g.insert_arc(a, b, 1);
    g.insert_arc(b, c, 1);
    g.insert_arc(c, d, 1);
    g.insert_arc(d, e, 1);
    g.insert_arc(e, a, 1);
    g.insert_arc(a, c, 1);

    const auto r = planarity_test(g, strict_lr_opts);
    print_result("Planar sample (strict LR embedding)", r);

    if (r.has_combinatorial_embedding)
      {
        const auto md = planar_dual_metadata(r);
        auto dual = build_planar_dual_graph<UGraph, UDual>(md);
        cout << "  dual graph nodes/arcs (local faces): "
             << dual.get_num_nodes() << "/" << dual.get_num_arcs() << "\n\n";

        const auto drawing = planar_geometric_drawing(r);
        print_drawing(drawing);
        cout << '\n';
      }
    else
      {
        cout << "  dual metadata skipped (strict LR did not build embedding)\n\n";
      }
  }

  {
    UGraph g;
    auto * n0 = g.insert_node("0");
    auto * n1 = g.insert_node("1");
    auto * n2 = g.insert_node("2");
    auto * n3 = g.insert_node("3");
    auto * n4 = g.insert_node("4");

    // Dense planar sample: K5 minus one edge.
    g.insert_arc(n0, n2, 1);
    g.insert_arc(n0, n3, 1);
    g.insert_arc(n0, n4, 1);
    g.insert_arc(n1, n2, 1);
    g.insert_arc(n1, n3, 1);
    g.insert_arc(n1, n4, 1);
    g.insert_arc(n2, n3, 1);
    g.insert_arc(n2, n4, 1);
    g.insert_arc(n3, n4, 1);

    const auto r = planarity_test(g, strict_lr_opts);
    print_result("Planar dense sample (strict LR embedding)", r);

    if (r.has_combinatorial_embedding)
      {
        const auto md = planar_dual_metadata(r);
        cout << "  dual metadata local/global faces: "
             << md.num_faces_local << "/" << md.num_faces_global << "\n\n";

        const auto drawing = planar_geometric_drawing(r);
        print_drawing(drawing);
        cout << '\n';
      }
    else
      {
        cout << "  dual metadata skipped (embedding unavailable)\n\n";
      }
  }

  {
    UGraph g;
    auto * u0 = g.insert_node("u0");
    auto * u1 = g.insert_node("u1");
    auto * u2 = g.insert_node("u2");
    auto * v0 = g.insert_node("v0");
    auto * v1 = g.insert_node("v1");
    auto * v2 = g.insert_node("v2");

    // K3,3 (non-planar)
    g.insert_arc(u0, v0, 1); g.insert_arc(u0, v1, 1); g.insert_arc(u0, v2, 1);
    g.insert_arc(u1, v0, 1); g.insert_arc(u1, v1, 1); g.insert_arc(u1, v2, 1);
    g.insert_arc(u2, v0, 1); g.insert_arc(u2, v1, 1); g.insert_arc(u2, v2, 1);

    const auto r = planarity_test(g, opts);
    print_result("Non-planar sample (K3,3)", r);

    if (r.has_nonplanar_certificate)
      {
        const auto json = nonplanar_certificate_to_json(r);
        const auto dot = nonplanar_certificate_to_dot(r);
        const auto graphml = nonplanar_certificate_to_graphml(r);
        const auto gexf = nonplanar_certificate_to_gexf(r);
        const auto vr = validate_nonplanar_certificate(r);

        const string json_path =
            (output_dir / "planarity_k33_certificate.json").string();
        const string dot_path =
            (output_dir / "planarity_k33_certificate.dot").string();
        const string graphml_path =
            (output_dir / "planarity_k33_certificate.graphml").string();
        const string gexf_path =
            (output_dir / "planarity_k33_certificate.gexf").string();
        {
          ofstream fj(json_path);
          fj << json;
        }
        {
          ofstream fd(dot_path);
          fd << dot;
        }
        {
          ofstream fg(graphml_path);
          fg << graphml;
        }
        {
          ofstream fx(gexf_path);
          fx << gexf;
        }

        cout << "  certificate JSON written to: " << json_path << '\n';
        cout << "  certificate DOT written to: " << dot_path << '\n';
        cout << "  certificate GraphML written to: " << graphml_path << '\n';
        cout << "  certificate GEXF written to: " << gexf_path << '\n';
        cout << "  certificate JSON bytes: " << json.size() << '\n';
        cout << "  certificate DOT bytes: " << dot.size() << '\n';
        cout << "  certificate GraphML bytes: " << graphml.size() << '\n';
        cout << "  certificate GEXF bytes: " << gexf.size() << '\n';
        cout << "  certificate structural validation: "
             << (vr.is_valid ? "valid" : "invalid") << '\n';
        cout << "  external validator command:\n";
        cout << "    ruby scripts/planarity_certificate_validator.rb"
             << " --input " << graphml_path
             << " --input " << gexf_path << '\n';
        cout << "  optional Gephi adapter command:\n";
        cout << "    ruby scripts/planarity_certificate_validator.rb"
             << " --input " << graphml_path
             << " --gephi"
             << " --gephi-cmd \"gephi --headless --import {input}\"" << '\n';
        cout << "  optional Gephi catalog template command:\n";
        cout << "    ruby scripts/planarity_certificate_validator.rb"
             << " --input " << graphml_path
             << " --gephi --require-gephi"
             << " --gephi-template portable.python-file-exists" << '\n';
        cout << "  optional render profile catalog command:\n";
        cout << "    ruby scripts/planarity_certificate_validator.rb"
             << " --list-gephi-render-profiles --json" << '\n';
        const string render_dir =
            (output_dir / "aleph_planarity_renders").string();
        const string ci_report =
            (output_dir / "aleph_planarity_ci_report.json").string();
        const string ci_render_report =
            (output_dir / "aleph_planarity_ci_render_report.json").string();
        const string visual_render_dir =
            (output_dir / "aleph_planarity_visual_renders").string();
        const string visual_report =
            (output_dir / "aleph_planarity_visual_diff_report.json").string();
        const string nightly_comparison_json =
            (output_dir / "gephi_nightly_comparison.json").string();
        const string nightly_alert_md =
            (output_dir / "gephi_nightly_alert.md").string();
        cout << "  optional render validation command:\n";
        cout << "    ruby scripts/planarity_certificate_validator.rb"
             << " --input " << graphml_path
             << " --render-gephi --require-render"
             << " --render-profile portable.python-render-svg"
             << " --render-output-dir " << render_dir << '\n';
        cout << "  optional CI batch command:\n";
        cout << "    ruby scripts/planarity_certificate_ci_batch.rb"
             << " --input " << graphml_path
             << " --input " << gexf_path
             << " --gephi --require-gephi"
             << " --gephi-template portable.python-file-exists"
             << " --report " << ci_report << " --print-summary"
             << '\n';
        cout << "  optional CI batch render command:\n";
        cout << "    ruby scripts/planarity_certificate_ci_batch.rb"
             << " --input " << graphml_path
             << " --render-gephi --require-render"
             << " --render-profile portable.python-render-svg"
             << " --render-output-dir " << render_dir
             << " --report " << ci_render_report
             << " --print-summary" << '\n';
        cout << "  optional visual golden diff command:\n";
        cout << "    ruby scripts/planarity_certificate_ci_visual_diff.rb"
             << " --input " << graphml_path
             << " --profile portable.python-render-svg"
             << " --profile portable.python-render-pdf"
             << " --render-output-dir " << visual_render_dir
             << " --report " << visual_report
             << " --print-summary" << '\n';
        cout << "  optional nightly real-Gephi workflow:\n";
        cout << "    .github/workflows/planarity_gephi_nightly.yml"
             << " (workflow_dispatch / weekly schedule; multi-tag matrix)"
             << '\n';
        cout << "  optional nightly regression notify command:\n";
        cout << "    ruby scripts/planarity_gephi_regression_notify.rb"
             << " --report-json " << nightly_comparison_json
             << " --output-md " << nightly_alert_md
             << " --repository lrleon/Aleph-w"
             << " --run-url https://github.com/lrleon/Aleph-w/actions/runs/123"
             << " --webhook-env ALEPH_PLANARITY_ALERT_WEBHOOK --print-summary"
             << '\n';
        if (not vr.is_valid)
          {
            cout << "    null branch nodes: " << vr.null_branch_nodes << '\n';
            cout << "    path/edge shape mismatches: "
                 << vr.path_node_edge_length_mismatch << '\n';
            cout << "    path edges not in obstruction: "
                 << vr.path_edge_not_in_obstruction << '\n';
          }
        cout << '\n';
      }
  }

  {
    DGraph g;
    auto * s = g.insert_node("S");
    auto * t = g.insert_node("T");
    auto * x = g.insert_node("X");

    // Underlying simple graph is just a triangle (planar),
    // even with loops and parallel directed arcs.
    g.insert_arc(s, t, 1);
    g.insert_arc(t, x, 1);
    g.insert_arc(x, s, 1);

    g.insert_arc(s, s, 1); // loop, ignored
    g.insert_arc(t, t, 1); // loop, ignored

    g.insert_arc(s, t, 1); // parallel, collapsed
    g.insert_arc(t, s, 1); // opposite orientation, same undirected edge

    print_result("Digraph normalization sample", planarity_test(g, opts));
  }

  return 0;
}
