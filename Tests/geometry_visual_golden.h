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

#ifndef ALEPH_TEST_GEOMETRY_VISUAL_GOLDEN_H
#define ALEPH_TEST_GEOMETRY_VISUAL_GOLDEN_H

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include <point.H>
#include <polygon.H>
#include <tpl_array.H>

namespace Aleph::TestVisual
{

struct SvgScene
{
  ::Array<::Polygon> polygons;
  ::Array<::Segment> segments;
  ::Array<::Point> points;
  ::Array<::Point> highlighted_points;
};

inline std::string sanitize_filename(const std::string & raw)
{
  std::string out;
  out.reserve(raw.size());
  for (const char ch : raw)
    if (std::isalnum(static_cast<unsigned char>(ch)) or ch == '-' or ch == '_')
      out.push_back(ch);
    else
      out.push_back('_');

  return out.empty() ? std::string("case") : out;
}

inline std::string xml_escape(const std::string & s)
{
  std::string out;
  out.reserve(s.size());
  for (const char ch : s)
    {
      switch (ch)
        {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&apos;"; break;
        default: out.push_back(ch); break;
        }
    }

  return out;
}

inline std::filesystem::path golden_output_dir()
{
  const char * env_dir = std::getenv("ALEPH_TEST_GOLDEN_DIR");
  if (env_dir != nullptr and env_dir[0] != '\0')
    return std::filesystem::path(env_dir);

  return std::filesystem::path("test_artifacts/golden_svg");
}

inline void add_polygon_vertices(SvgScene & scene, const ::Polygon & poly,
                                 const bool as_highlight = false)
{
  if (poly.size() == 0)
    return;

  for (::Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
    {
      if (as_highlight)
        scene.highlighted_points.append(it.get_current_vertex());
      else
        scene.points.append(it.get_current_vertex());
    }
}

inline void collect_scene_points(const SvgScene & scene, ::Array<::Point> & all_points)
{
  for (size_t i = 0; i < scene.points.size(); ++i)
    all_points.append(scene.points(i));
  for (size_t i = 0; i < scene.highlighted_points.size(); ++i)
    all_points.append(scene.highlighted_points(i));

  for (size_t i = 0; i < scene.segments.size(); ++i)
    {
      all_points.append(scene.segments(i).get_src_point());
      all_points.append(scene.segments(i).get_tgt_point());
    }

  for (size_t i = 0; i < scene.polygons.size(); ++i)
    {
      if (scene.polygons(i).size() == 0)
        continue;
      for (::Polygon::Vertex_Iterator it(scene.polygons(i)); it.has_curr(); it.next_ne())
        all_points.append(it.get_current_vertex());
    }
}

inline std::filesystem::path emit_case_svg(const std::string & case_id,
                                           const SvgScene & scene,
                                           const std::string & note = "")
{
  const std::filesystem::path out_dir = golden_output_dir();
  std::error_code ec;
  std::filesystem::create_directories(out_dir, ec);

  const std::filesystem::path out_file =
      out_dir / (sanitize_filename(case_id) + ".svg");

  std::ofstream out(out_file);
  if (not out)
    return out_file;

  ::Array<::Point> all_points;
  collect_scene_points(scene, all_points);

  double xmin = 0.0;
  double xmax = 1.0;
  double ymin = 0.0;
  double ymax = 1.0;
  bool initialized = false;

  for (size_t i = 0; i < all_points.size(); ++i)
    {
      const double x = all_points(i).get_x().get_d();
      const double y = all_points(i).get_y().get_d();
      if (not initialized)
        {
          xmin = xmax = x;
          ymin = ymax = y;
          initialized = true;
        }
      else
        {
          xmin = std::min(xmin, x);
          xmax = std::max(xmax, x);
          ymin = std::min(ymin, y);
          ymax = std::max(ymax, y);
        }
    }

  if (not initialized)
    {
      xmin = ymin = -1.0;
      xmax = ymax = 1.0;
    }

  double dx = xmax - xmin;
  double dy = ymax - ymin;
  if (dx < 1e-9)
    {
      xmin -= 1.0;
      xmax += 1.0;
      dx = xmax - xmin;
    }
  if (dy < 1e-9)
    {
      ymin -= 1.0;
      ymax += 1.0;
      dy = ymax - ymin;
    }

  double pad = std::max(dx, dy) * 0.08;
  if (pad < 1e-6)
    pad = 1.0;
  xmin -= pad;
  xmax += pad;
  ymin -= pad;
  ymax += pad;
  dx = xmax - xmin;
  dy = ymax - ymin;

  constexpr double width = 960.0;
  constexpr double height = 720.0;
  constexpr double margin = 32.0;
  const double drawable_w = width - 2.0 * margin;
  const double drawable_h = height - 2.0 * margin;
  const double scale = std::min(drawable_w / dx, drawable_h / dy);
  const double xoff = margin + (drawable_w - scale * dx) * 0.5;
  const double yoff = margin + (drawable_h - scale * dy) * 0.5;

  const auto map_point = [=](const ::Point & p)
    {
      const double x = xoff + (p.get_x().get_d() - xmin) * scale;
      const double y = height - (yoff + (p.get_y().get_d() - ymin) * scale);
      return std::pair<double, double>{x, y};
    };

  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
      << "width=\"" << static_cast<int>(width) << "\" "
      << "height=\"" << static_cast<int>(height) << "\" "
      << "viewBox=\"0 0 " << static_cast<int>(width) << " "
      << static_cast<int>(height) << "\">\n";
  out << "  <rect x=\"0\" y=\"0\" width=\"" << static_cast<int>(width)
      << "\" height=\"" << static_cast<int>(height)
      << "\" fill=\"#ffffff\"/>\n";
  out << "  <rect x=\"1\" y=\"1\" width=\"" << static_cast<int>(width - 2)
      << "\" height=\"" << static_cast<int>(height - 2)
      << "\" fill=\"none\" stroke=\"#e5e7eb\" stroke-width=\"2\"/>\n";

  if (not note.empty())
    out << "  <text x=\"16\" y=\"22\" font-size=\"14\" "
        << "font-family=\"monospace\" fill=\"#111827\">"
        << xml_escape(note) << "</text>\n";

  static constexpr std::array<const char *, 8> stroke_colors = {
      "#2563eb", "#16a34a", "#ea580c", "#7c3aed",
      "#0f766e", "#b91c1c", "#1d4ed8", "#0369a1"
  };
  static constexpr std::array<const char *, 8> fill_colors = {
      "#bfdbfe", "#bbf7d0", "#fed7aa", "#ddd6fe",
      "#99f6e4", "#fecaca", "#c7d2fe", "#bae6fd"
  };

  out << std::fixed << std::setprecision(3);

  for (size_t pi = 0; pi < scene.polygons.size(); ++pi)
    {
      const ::Polygon & poly = scene.polygons(pi);
      if (poly.size() == 0)
        continue;
      std::ostringstream pts_stream;
      size_t count = 0;
      for (::Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
        {
          const auto [x, y] = map_point(it.get_current_vertex());
          if (count != 0)
            pts_stream << ' ';
          pts_stream << x << ',' << y;
          ++count;
        }

      if (count == 0)
        continue;

      const char * stroke = stroke_colors[pi % stroke_colors.size()];
      const char * fill = fill_colors[pi % fill_colors.size()];
      if (poly.is_closed() and count >= 3)
        out << "  <polygon points=\"" << pts_stream.str()
            << "\" fill=\"" << fill
            << "\" fill-opacity=\"0.22\" stroke=\"" << stroke
            << "\" stroke-width=\"2\"/>\n";
      else
        out << "  <polyline points=\"" << pts_stream.str()
            << "\" fill=\"none\" stroke=\"" << stroke
            << "\" stroke-width=\"2\"/>\n";

      for (::Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
        {
          const auto [vx, vy] = map_point(it.get_current_vertex());
          out << "  <circle cx=\"" << vx << "\" cy=\"" << vy
              << "\" r=\"3.0\" fill=\"" << stroke
              << "\" stroke=\"#ffffff\" stroke-width=\"1\"/>\n";
        }
    }

  for (size_t si = 0; si < scene.segments.size(); ++si)
    {
      const ::Segment & seg = scene.segments(si);
      const auto [x1, y1] = map_point(seg.get_src_point());
      const auto [x2, y2] = map_point(seg.get_tgt_point());
      out << "  <line x1=\"" << x1 << "\" y1=\"" << y1
          << "\" x2=\"" << x2 << "\" y2=\"" << y2
          << "\" stroke=\"#4b5563\" stroke-width=\"2.0\"/>\n";
    }

  for (size_t i = 0; i < scene.points.size(); ++i)
    {
      const auto [x, y] = map_point(scene.points(i));
      out << "  <circle cx=\"" << x << "\" cy=\"" << y
          << "\" r=\"3.2\" fill=\"#111827\" stroke=\"#ffffff\" "
          << "stroke-width=\"0.9\"/>\n";
    }

  for (size_t i = 0; i < scene.highlighted_points.size(); ++i)
    {
      const auto [x, y] = map_point(scene.highlighted_points(i));
      out << "  <circle cx=\"" << x << "\" cy=\"" << y
          << "\" r=\"4.3\" fill=\"#dc2626\" stroke=\"#ffffff\" "
          << "stroke-width=\"1.2\"/>\n";
    }

  out << "  <text x=\"16\" y=\"" << static_cast<int>(height - 14)
      << "\" font-size=\"12\" font-family=\"monospace\" fill=\"#4b5563\">"
      << xml_escape(case_id) << "</text>\n";
  out << "</svg>\n";

  return out_file;
}

} // namespace Aleph::TestVisual

#endif // ALEPH_TEST_GEOMETRY_VISUAL_GOLDEN_H
