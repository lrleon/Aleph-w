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

# ifndef TIKZGEOM_H
# define TIKZGEOM_H

# include <algorithm>
# include <array>
# include <cmath>
# include <cstddef>
# include <iomanip>
# include <map>
# include <ostream>
# include <sstream>
# include <string>
# include <unordered_set>
# include <type_traits>
# include <utility>
# include <variant>
# include <vector>

# include "line.H"
# include "point.H"
# include "polygon.H"

namespace Aleph
{
  /** @file tikzgeom.H
   *  @ingroup Geometry
   *  @brief TikZ/LaTeX geometric drawing utilities.
   *
   *  This module provides a PGF/TikZ backend to render geometry objects into a
   *  fixed-size plane. Its public API mirrors the EEPIC model
   *  (`Eepic_Plane` + `put_in_plane`) so existing workflows can migrate
   *  incrementally.
   *
   *  Typical usage:
   *
   *  @code
   *  Tikz_Plane plane(120, 80); // width/height in mm
   *  put_in_plane(plane, Point(0, 0));
   *  put_in_plane(plane, Segment(Point(0, 0), Point(10, 20)));
   *  plane.draw(std::cout);
   *  @endcode
   *
   *  Robustness notes:
   *  - Degenerate global ranges (all x equal or all y equal) are handled without
   *    division by zero.
   *  - Empty planes are valid and produce an empty clipped `tikzpicture`.
   */

  /** @brief Style descriptor for TikZ primitives.
   *
   *  The same style can be reused across points/segments/polygons/labels.
   *  Unsupported flags for a specific primitive are ignored (for example,
   *  `with_arrow` on text).
   */
  struct Tikz_Style
  {
    std::string tikz_style_name; ///< Optional named style (from `\tikzset`)
    std::string draw_color = "black"; ///< TikZ draw color (`draw=<color>`)
    std::string fill_color; ///< TikZ fill color (`fill=<color>`)
    std::string pattern; ///< TikZ pattern (e.g. `north east lines`)
    std::string pattern_color; ///< TikZ pattern color (`pattern color=<color>`)
    std::string text_color; ///< TikZ text color (`text=<color>`)
    std::string text_anchor; ///< TikZ node anchor (`anchor=<value>`)
    std::string text_placement; ///< TikZ placement (`above`, `below left`, ...)
    std::string extra_options; ///< Raw extra options appended as-is
    std::string text_font_command; ///< e.g. `\\small`, `\\footnotesize`

    double line_width_mm = -1.0; ///< `line width=<value>mm` when > 0
    double opacity = -1.0; ///< `opacity=<value>` in [0,1], when >= 0

    bool dashed = false; ///< Add `dashed`
    bool dotted = false; ///< Add `dotted`
    bool thick = false; ///< Add `thick`
    bool with_arrow = false; ///< Add `->`
    bool fill = false; ///< Fill closed shapes (polygon/triangle/ellipse)
  };

  /** @brief Represents a quadratic Bézier curve for native TikZ rendering.
   *
   *  @ingroup Geometry
   *
   *  Used with `put_quadratic_bezier_native_in_plane` to generate a
   *  `.. controls (p1) ..` path command.
   *
   *  @see Tikz_Plane::Object, put_quadratic_bezier_native_in_plane()
   */
  struct Tikz_Quadratic_Bezier
  {
    Point p0; ///< Start point.
    Point p1; ///< Control point.
    Point p2; ///< End point.
  };

  /** @brief Represents a cubic Bézier curve for native TikZ rendering.
   *
   *  @ingroup Geometry
   *
   *  Used with `put_cubic_bezier_native_in_plane` to generate a
   *  `.. controls (p1) and (p2) ..` path command.
   *
   *  @see Tikz_Plane::Object, put_cubic_bezier_native_in_plane()
   */
  struct Tikz_Cubic_Bezier
  {
    Point p0; ///< Start point.
    Point p1; ///< First control point.
    Point p2; ///< Second control point.
    Point p3; ///< End point.
  };

  /** @brief An open or closed polyline for TikZ rendering.
   *
   *  @ingroup Geometry
   *
   *  Unlike `Polygon`, this type stores raw vertices without colinearity
   *  merging.  It is used by Bézier polyline approximations where every
   *  sample point must be preserved.
   */
  struct Tikz_Polyline
  {
    std::vector<Point> vertices; ///< Ordered vertices.
    bool closed = false;         ///< Whether to close the polyline.
  };

  /** @brief Create a basic draw style with a custom color. */
  inline Tikz_Style make_tikz_draw_style(const std::string & draw_color)
  {
    Tikz_Style s;
    s.draw_color = draw_color;
    return s;
  }

  /** @brief Create a basic fill style with custom draw/fill colors. */
  inline Tikz_Style make_tikz_fill_style(const std::string & draw_color,
                                         const std::string & fill_color)
  {
    Tikz_Style s;
    s.draw_color = draw_color;
    s.fill_color = fill_color;
    s.fill = true;
    return s;
  }

  /** @brief 2D TikZ canvas storing geometry objects and emitting LaTeX output.
   *
   *  `Tikz_Plane`:
   *  1. Collects geometry objects inserted with `put()` / `put_in_plane(...)`.
   *  2. Computes a global bounding box and maps it to the fixed frame.
   *  3. Emits a `tikzpicture` block using millimeter units.
   *
   *  The frame (`wide`, `height`) is fixed at construction time.
   */
  class Tikz_Plane
  {
  public:
    static constexpr int Layer_Background = -100;
    static constexpr int Layer_Default = 0;
    static constexpr int Layer_Foreground = 100;
    static constexpr int Layer_Overlay = 1000;

    /** @brief Runtime set of supported objects. */
    using Object = std::variant<Point, Polar_Point, Segment, Triangle,
                                Ellipse, RotatedEllipse, Text,
                                Polygon, Regular_Polygon, Rectangle, LineEq,
                                Tikz_Quadratic_Bezier, Tikz_Cubic_Bezier,
                                Tikz_Polyline>;

  private:
    struct Geom_Box
    {
      Geom_Number xmin = 0;
      Geom_Number xmax = 0;
      Geom_Number ymin = 0;
      Geom_Number ymax = 0;
      bool initialized = false;
    };

    struct Mapping
    {
      double sx = 1.0;
      double sy = 1.0;
      double dx = 0.0;
      double dy = 0.0;
      bool has_x_range = false;
      bool has_y_range = false;
    };

    struct Styled_Object
    {
      Object object;
      Tikz_Style style;
      int layer = Layer_Default;
      size_t order = 0;
    };

    double wide_ = 0.0;
    double height_ = 0.0;
    double xoffset_ = 0.0;
    double yoffset_ = 0.0;

    bool with_cartesian_axis_ = false;
    bool with_coordinate_grid_ = false;
    bool with_grid_ticks_ = false;
    bool with_native_layers_ = false;
    bool with_auto_legend_ = false;
    double point_radius_mm_ = 1.0;
    double clip_padding_mm_ = -1.0;
    double grid_step_x_ = 1.0;
    double grid_step_y_ = 1.0;
    double tick_size_mm_ = 1.5;
    double legend_x_mm_ = 3.0;
    double legend_y_mm_ = 3.0;
    double legend_entry_step_mm_ = 5.0;

    Tikz_Style default_style_;
    Tikz_Style axis_style_ = []()
      {
        Tikz_Style s;
        s.draw_color = "gray";
        s.with_arrow = true;
        return s;
      }();

    Tikz_Style grid_style_ = []()
      {
        Tikz_Style s;
        s.draw_color = "gray!40";
        s.dotted = true;
        s.line_width_mm = 0.2;
        return s;
      }();

    struct Legend_Entry
    {
      std::string label;
      Tikz_Style style;
    };

    std::vector<Legend_Entry> legend_entries_;
    std::map<std::string, Tikz_Style> tikz_style_defs_;

    std::vector<Styled_Object> objects_;
    size_t next_order_ = 0;

    [[nodiscard]] static std::string fmt_double(const double v)
    {
      std::ostringstream ss;
      ss << std::fixed << std::setprecision(3) << v;

      std::string out = ss.str();
      if (const auto dot_pos = out.find('.'); dot_pos != std::string::npos)
        {
          while (not out.empty() and out.back() == '0')
            out.pop_back();
          if (not out.empty() and out.back() == '.')
            out.pop_back();
        }

      if (out == "-0")
        out = "0";

      return out;
    }

    static void append_option(std::string & opts, const std::string & opt)
    {
      if (opt.empty())
        return;
      if (not opts.empty())
        opts += ",";
      opts += opt;
    }

    [[nodiscard]] static std::string draw_options(const Tikz_Style & style,
                                                  const bool allow_fill = false)
    {
      std::string opts;

      if (not style.tikz_style_name.empty())
        append_option(opts, style.tikz_style_name);

      if (not style.draw_color.empty())
        append_option(opts, "draw=" + style.draw_color);

      if (allow_fill and style.fill)
        {
          if (not style.fill_color.empty())
            append_option(opts, "fill=" + style.fill_color);
          else if (not style.draw_color.empty())
            append_option(opts, "fill=" + style.draw_color);

          if (not style.pattern.empty())
            {
              append_option(opts, "pattern=" + style.pattern);
              if (not style.pattern_color.empty())
                append_option(opts, "pattern color=" + style.pattern_color);
            }
        }

      if (style.line_width_mm > 0.0)
        append_option(opts, "line width=" + fmt_double(style.line_width_mm) + "mm");

      if (style.opacity >= 0.0)
        append_option(opts, "opacity=" + fmt_double(style.opacity));

      if (style.dashed)
        append_option(opts, "dashed");
      if (style.dotted)
        append_option(opts, "dotted");
      if (style.thick)
        append_option(opts, "thick");
      if (style.with_arrow)
        append_option(opts, "->");

      if (not style.extra_options.empty())
        append_option(opts, style.extra_options);

      return opts;
    }

    [[nodiscard]] static std::string point_options(const Tikz_Style & style)
    {
      std::string opts;
      if (not style.tikz_style_name.empty())
        append_option(opts, style.tikz_style_name);

      if (not style.fill_color.empty())
        append_option(opts, "fill=" + style.fill_color);
      else if (not style.draw_color.empty())
        append_option(opts, "fill=" + style.draw_color);

      if (style.opacity >= 0.0)
        append_option(opts, "opacity=" + fmt_double(style.opacity));

      if (not style.extra_options.empty())
        append_option(opts, style.extra_options);

      return opts;
    }

    [[nodiscard]] static std::string text_options(const Tikz_Style & style)
    {
      std::string opts;
      if (not style.tikz_style_name.empty())
        append_option(opts, style.tikz_style_name);

      if (not style.text_color.empty())
        append_option(opts, "text=" + style.text_color);
      else if (not style.draw_color.empty())
        append_option(opts, "text=" + style.draw_color);

      if (style.opacity >= 0.0)
        append_option(opts, "opacity=" + fmt_double(style.opacity));

      if (not style.text_anchor.empty())
        append_option(opts, "anchor=" + style.text_anchor);
      if (not style.text_placement.empty())
        append_option(opts, style.text_placement);

      if (not style.extra_options.empty())
        append_option(opts, style.extra_options);

      return opts;
    }

    static Geom_Number rotated_x_extent(const RotatedEllipse & e)
    {
      const Geom_Number a2 = e.get_a() * e.get_a();
      const Geom_Number b2 = e.get_b() * e.get_b();
      const Geom_Number c2 = e.get_cos() * e.get_cos();
      const Geom_Number s2 = e.get_sin() * e.get_sin();
      return square_root(a2 * c2 + b2 * s2);
    }

    static Geom_Number rotated_y_extent(const RotatedEllipse & e)
    {
      const Geom_Number a2 = e.get_a() * e.get_a();
      const Geom_Number b2 = e.get_b() * e.get_b();
      const Geom_Number c2 = e.get_cos() * e.get_cos();
      const Geom_Number s2 = e.get_sin() * e.get_sin();
      return square_root(a2 * s2 + b2 * c2);
    }

    static Geom_Box make_box(const Geom_Number & xmin,
                             const Geom_Number & xmax,
                             const Geom_Number & ymin,
                             const Geom_Number & ymax)
    {
      Geom_Box box;
      box.xmin = xmin;
      box.xmax = xmax;
      box.ymin = ymin;
      box.ymax = ymax;
      box.initialized = true;
      return box;
    }

    static void merge_box(Geom_Box & dst, const Geom_Box & src)
    {
      if (not src.initialized)
        return;

      if (not dst.initialized)
        {
          dst = src;
          return;
        }

      dst.xmin = std::min(dst.xmin, src.xmin);
      dst.xmax = std::max(dst.xmax, src.xmax);
      dst.ymin = std::min(dst.ymin, src.ymin);
      dst.ymax = std::max(dst.ymax, src.ymax);
    }

    static Geom_Box bbox_of(const Point & p)
    {
      return make_box(p.get_x(), p.get_x(), p.get_y(), p.get_y());
    }

    static Geom_Box bbox_of(const Polar_Point & pp)
    {
      return bbox_of(Point(pp));
    }

    static Geom_Box bbox_of(const Segment & s)
    {
      return make_box(s.leftmost_point().get_x(), s.rightmost_point().get_x(),
                      s.lowest_point().get_y(), s.highest_point().get_y());
    }

    static Geom_Box bbox_of(const Triangle & t)
    {
      return make_box(t.leftmost_point().get_x(), t.rightmost_point().get_x(),
                      t.lowest_point().get_y(), t.highest_point().get_y());
    }

    static Geom_Box bbox_of(const Ellipse & e)
    {
      return make_box(e.leftmost_point().get_x(), e.rightmost_point().get_x(),
                      e.lowest_point().get_y(), e.highest_point().get_y());
    }

    static Geom_Box bbox_of(const RotatedEllipse & e)
    {
      const Geom_Number ex = rotated_x_extent(e);
      const Geom_Number ey = rotated_y_extent(e);
      const Point & c = e.get_center();
      return make_box(c.get_x() - ex, c.get_x() + ex,
                      c.get_y() - ey, c.get_y() + ey);
    }

    static Geom_Box bbox_of(const Text & t)
    {
      return make_box(t.leftmost_point().get_x(), t.rightmost_point().get_x(),
                      t.lowest_point().get_y(), t.highest_point().get_y());
    }

    static Geom_Box bbox_of(const Polygon & poly)
    {
      if (poly.size() == 0)
        return {};

      Geom_Box box;
      for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
        {
          const Point p = it.get_current_vertex().to_point();
          merge_box(box, bbox_of(p));
        }
      return box;
    }

    static Geom_Box bbox_of(const Regular_Polygon & poly)
    {
      if (poly.size() == 0)
        return {};

      Geom_Box box;
      for (size_t i = 0; i < poly.size(); ++i)
        merge_box(box, bbox_of(poly.get_vertex(i)));
      return box;
    }

    static Geom_Box bbox_of(const Rectangle & r)
    {
      return make_box(r.get_xmin(), r.get_xmax(), r.get_ymin(), r.get_ymax());
    }

    static Geom_Box bbox_of(const LineEq & l)
    {
      const auto x0 = Geom_Number(0);
      const auto x1 = Geom_Number(1);
      const Geom_Number y0 = l(x0);
      const Geom_Number y1 = l(x1);
      return make_box(std::min(x0, x1), std::max(x0, x1),
                      std::min(y0, y1), std::max(y0, y1));
    }

    static Geom_Box bbox_of(const Tikz_Quadratic_Bezier & b)
    {
      Geom_Box box;
      merge_box(box, bbox_of(b.p0));
      merge_box(box, bbox_of(b.p1));
      merge_box(box, bbox_of(b.p2));
      return box;
    }

    static Geom_Box bbox_of(const Tikz_Cubic_Bezier & b)
    {
      Geom_Box box;
      merge_box(box, bbox_of(b.p0));
      merge_box(box, bbox_of(b.p1));
      merge_box(box, bbox_of(b.p2));
      merge_box(box, bbox_of(b.p3));
      return box;
    }

    static Geom_Box bbox_of(const Tikz_Polyline & pl)
    {
      Geom_Box box;
      for (const auto & p : pl.vertices)
        merge_box(box, bbox_of(p));
      return box;
    }

    [[nodiscard]] Geom_Box compute_box() const
    {
      Geom_Box box;
      for (const auto & entry: objects_)
        {
          merge_box(box, std::visit(
                                    [](const auto & val) { return bbox_of(val); }, entry.object));
        }

      return box;
    }

    [[nodiscard]] double effective_clip_padding_mm() const
    {
      return std::max(point_radius_mm_, clip_padding_mm_ > 0.0 ? clip_padding_mm_ : 0.0);
    }

    [[nodiscard]] static std::string escape_latex(const std::string & text)
    {
      std::string escaped;
      escaped.reserve(text.size() + 8);
      for (const char ch: text)
        switch (ch)
          {
          case '\\': escaped += "\\textbackslash{}";
            break;
          case '{': escaped += "\\{";
            break;
          case '}': escaped += "\\}";
            break;
          case '$': escaped += "\\$";
            break;
          case '%': escaped += "\\%";
            break;
          case '#': escaped += "\\#";
            break;
          case '&': escaped += "\\&";
            break;
          case '_': escaped += "\\_";
            break;
          case '^': escaped += "\\^{}";
            break;
          case '~': escaped += "\\~{}";
            break;
          default: escaped.push_back(ch);
            break;
          }
      return escaped;
    }

    [[nodiscard]] Mapping compute_mapping(const Geom_Box & box, const bool squarize) const
    {
      Mapping map;

      if (not box.initialized)
        {
          map.dx = wide_ / 2.0;
          map.dy = height_ / 2.0;
          return map;
        }

      const Geom_Number x_range = box.xmax - box.xmin;
      const Geom_Number y_range = box.ymax - box.ymin;

      map.has_x_range = x_range != 0;
      map.has_y_range = y_range != 0;

      const double xr = map.has_x_range ? geom_number_to_double(x_range) : 0.0;
      const double yr = map.has_y_range ? geom_number_to_double(y_range) : 0.0;

      if (map.has_x_range)
        map.sx = wide_ / xr;
      if (map.has_y_range)
        map.sy = height_ / yr;

      if (squarize and map.has_x_range and map.has_y_range)
        {
          const double s = std::min(map.sx, map.sy);
          map.sx = s;
          map.sy = s;

          const double used_w = xr * s;
          const double used_h = yr * s;
          map.dx = (wide_ - used_w) / 2.0;
          map.dy = (height_ - used_h) / 2.0;
        }
      else
        {
          map.dx = 0.0;
          map.dy = 0.0;
        }

      if (not map.has_x_range)
        map.dx = wide_ / 2.0;
      if (not map.has_y_range)
        map.dy = height_ / 2.0;

      return map;
    }

    static double map_x(const Geom_Number & x,
                        const Geom_Box & box, const Mapping & map)
    {
      if (not map.has_x_range)
        return map.dx;

      return map.dx + geom_number_to_double(x - box.xmin) * map.sx;
    }

    static double map_y(const Geom_Number & y,
                        const Geom_Box & box, const Mapping & map)
    {
      if (not map.has_y_range)
        return map.dy;

      return map.dy + geom_number_to_double(y - box.ymin) * map.sy;
    }

    static std::pair<double, double>
    map_point(const Point & p, const Geom_Box & box, const Mapping & map)
    {
      return {map_x(p.get_x(), box, map), map_y(p.get_y(), box, map)};
    }

    static double map_x_length(const Geom_Number & dx, const Mapping & map)
    {
      if (not map.has_x_range)
        return 0.0;
      return std::fabs(geom_number_to_double(dx)) * map.sx;
    }

    static double map_y_length(const Geom_Number & dy, const Mapping & map)
    {
      if (not map.has_y_range)
        return 0.0;
      return std::fabs(geom_number_to_double(dy)) * map.sy;
    }

    template <typename Out>
    void draw_point(const Point & p, const Tikz_Style & style, Out & output,
                    const Geom_Box & box, const Mapping & map) const
    {
      const auto [x, y] = map_point(p, box, map);
      output << "    \\fill";
      if (const std::string opts = point_options(style); not opts.empty())
        output << "[" << opts << "]";

      output << " (" << x << "," << y << ") "
          << "circle[radius=" << point_radius_mm_ << "mm];" << std::endl;
    }

    template <typename Out>
    void draw_polyline(Out & output,
                       const std::vector<Point> & vertices,
                       const bool closed,
                       const Tikz_Style & style,
                       const Geom_Box & box, const Mapping & map) const
    {
      if (vertices.empty())
        return;

      if (vertices.size() == 1)
        {
          draw_point(vertices.front(), style, output, box, map);
          return;
        }

      output << "    \\draw";
      if (const std::string opts = draw_options(style, closed); not opts.empty())
        output << "[" << opts << "]";
      output << " ";

      for (size_t i = 0; i < vertices.size(); ++i)
        {
          const auto [x, y] = map_point(vertices[i], box, map);
          if (i == 0)
            output << "(" << x << "," << y << ")";
          else
            output << " -- (" << x << "," << y << ")";
        }

      if (closed)
        output << " -- cycle";
      output << ";" << std::endl;
    }

    template <typename Out>
    void draw_segment(const Segment & s, const Tikz_Style & style, Out & output,
                      const Geom_Box & box, const Mapping & map) const
    {
      draw_polyline(output, {s.get_src_point(), s.get_tgt_point()}, false, style, box, map);
    }

    template <typename Out>
    void draw_triangle(const Triangle & t, const Tikz_Style & style, Out & output,
                       const Geom_Box & box, const Mapping & map) const
    {
      draw_polyline(output, {t.get_p1(), t.get_p2(), t.get_p3()}, true, style, box, map);
    }

    template <typename Out>
    void draw_ellipse(const Ellipse & e, const Tikz_Style & style, Out & output,
                      const Geom_Box & box, const Mapping & map) const
    {
      const auto [cx, cy] = map_point(e.get_center(), box, map);
      const double rx = map_x_length(e.get_hradius(), map);
      const double ry = map_y_length(e.get_vradius(), map);

      if (rx == 0.0 or ry == 0.0)
        {
          draw_point(e.get_center(), style, output, box, map);
          return;
        }

      output << "    \\draw";
      if (const std::string opts = draw_options(style, true); not opts.empty())
        output << "[" << opts << "]";
      output << " (" << cx << "," << cy << ") "
          << "ellipse [x radius=" << rx << ", y radius=" << ry << "];"
          << std::endl;
    }

    template <typename Out>
    static void draw_rotated_ellipse(const RotatedEllipse & e, const Tikz_Style & style,
                                     Out & output, const Geom_Box & box,
                                     const Mapping & map)
    {
      constexpr size_t sample_count = 96;
      constexpr double two_pi = 2.0 * 3.14159265358979323846;

      output << "    \\draw";
      if (const std::string opts = draw_options(style, true); not opts.empty())
        output << "[" << opts << "]";
      output << " ";

      for (size_t i = 0; i <= sample_count; ++i)
        {
          const double t = two_pi * static_cast<double>(i) /
                           static_cast<double>(sample_count);

          const Geom_Number cos_t = std::cos(t);
          const Geom_Number sin_t = std::sin(t);

          const Geom_Number local_x = e.get_a() * cos_t;
          const Geom_Number local_y = e.get_b() * sin_t;

          const Geom_Number world_x = e.get_center().get_x() +
                                      local_x * e.get_cos() - local_y * e.get_sin();
          const Geom_Number world_y = e.get_center().get_y() +
                                      local_x * e.get_sin() + local_y * e.get_cos();

          const auto [x, y] = map_point(Point(world_x, world_y), box, map);

          if (i == 0)
            output << "(" << x << "," << y << ")";
          else
            output << " -- (" << x << "," << y << ")";
        }
      output << " -- cycle;" << std::endl;
    }

    template <typename Out>
    static void draw_text(const Text & t, const Tikz_Style & style, Out & output,
                          const Geom_Box & box, const Mapping & map)
    {
      const auto [x, y] = map_point(t.get_point(), box, map);

      output << "    \\node[inner sep=1pt";
      if (const std::string opts = text_options(style); not opts.empty())
        output << "," << opts;
      output << "] at (" << x << "," << y << ") {";

      if (not style.text_font_command.empty())
        output << style.text_font_command << " ";

      output << escape_latex(t.get_str()) << "};" << std::endl;
    }

    template <typename Out>
    void draw_polygon(const Polygon & poly, const Tikz_Style & style, Out & output,
                      const Geom_Box & box, const Mapping & map) const
    {
      std::vector<Point> vertices;
      vertices.reserve(poly.size());
      for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
        vertices.push_back(it.get_current_vertex().to_point());

      draw_polyline(output, vertices, poly.is_closed(), style, box, map);
    }

    template <typename Out>
    void draw_regular_polygon(const Regular_Polygon & poly, const Tikz_Style & style,
                              Out & output, const Geom_Box & box,
                              const Mapping & map) const
    {
      std::vector<Point> vertices;
      vertices.reserve(poly.size());
      for (size_t i = 0; i < poly.size(); ++i)
        vertices.push_back(poly.get_vertex(i));

      draw_polyline(output, vertices, true, style, box, map);
    }

    template <typename Out>
    void draw_rectangle(const Rectangle & r, const Tikz_Style & style, Out & output,
                        const Geom_Box & box, const Mapping & map) const
    {
      std::vector<Point> vertices;
      vertices.reserve(4);
      const std::array<Point, 4> corners = r.corners();
      for (const Point & p: corners)
        vertices.push_back(p);
      draw_polyline(output, vertices, true, style, box, map);
    }

    template <typename Out>
    void draw_line_eq(const LineEq & l, const Tikz_Style & style, Out & output,
                      const Geom_Box & box, const Mapping & map) const
    {
      if (not box.initialized)
        return;

      std::vector<Point> clipped;
      clipped.reserve(4);

      const auto append_if_new = [&](const Point & p)
        {
          for (const Point & existing: clipped)
            if (existing == p)
              return;
          clipped.push_back(p);
        };

      const auto y_in_range = [&](const Geom_Number & y)
        {
          return box.ymin <= y and y <= box.ymax;
        };
      const auto x_in_range = [&](const Geom_Number & x)
        {
          return box.xmin <= x and x <= box.xmax;
        };

      if (const Geom_Number y_left = l(box.xmin); y_in_range(y_left))
        append_if_new(Point(box.xmin, y_left));

      if (const Geom_Number y_right = l(box.xmax); y_in_range(y_right))
        append_if_new(Point(box.xmax, y_right));

      if (l.m != Geom_Number(0))
        {
          if (const Geom_Number x_bottom = (box.ymin - l.y0) / l.m; x_in_range(x_bottom))
            append_if_new(Point(x_bottom, box.ymin));

          if (const Geom_Number x_top = (box.ymax - l.y0) / l.m; x_in_range(x_top))
            append_if_new(Point(x_top, box.ymax));
        }

      if (clipped.size() < 2)
        return;

      size_t best_i = 0;
      size_t best_j = 1;
      Geom_Number best_d2 = clipped[0].distance_squared_to(clipped[1]);
      for (size_t i = 0; i < clipped.size(); ++i)
        for (size_t j = i + 1; j < clipped.size(); ++j)
          {
            const Geom_Number d2 = clipped[i].distance_squared_to(clipped[j]);
            if (d2 > best_d2)
              {
                best_d2 = d2;
                best_i = i;
                best_j = j;
              }
          }

      draw_segment(Segment(clipped[best_i], clipped[best_j]), style, output, box, map);
    }

    template <typename Out>
    static void draw_quadratic_bezier(const Tikz_Quadratic_Bezier & b,
                                      const Tikz_Style & style, Out & output,
                                      const Geom_Box & box, const Mapping & map)
    {
      const auto [x0, y0] = map_point(b.p0, box, map);
      const auto [x1, y1] = map_point(b.p1, box, map);
      const auto [x2, y2] = map_point(b.p2, box, map);

      output << "    \\draw";
      if (const std::string opts = draw_options(style, false); not opts.empty())
        output << "[" << opts << "]";
      output << " (" << x0 << "," << y0 << ") .. controls ("
          << x1 << "," << y1 << ") .. (" << x2 << "," << y2 << ");"
          << std::endl;
    }

    template <typename Out>
    static void draw_cubic_bezier(const Tikz_Cubic_Bezier & b,
                                  const Tikz_Style & style, Out & output,
                                  const Geom_Box & box, const Mapping & map)
    {
      const auto [x0, y0] = map_point(b.p0, box, map);
      const auto [x1, y1] = map_point(b.p1, box, map);
      const auto [x2, y2] = map_point(b.p2, box, map);
      const auto [x3, y3] = map_point(b.p3, box, map);

      output << "    \\draw";
      if (const std::string opts = draw_options(style, false); not opts.empty())
        output << "[" << opts << "]";
      output << " (" << x0 << "," << y0 << ") .. controls ("
          << x1 << "," << y1 << ") and (" << x2 << "," << y2
          << ") .. (" << x3 << "," << y3 << ");"
          << std::endl;
    }

    [[nodiscard]] static const char * layer_name(const int layer)
    {
      if (layer <= Layer_Background)
        return "background";
      if (layer >= Layer_Overlay)
        return "overlay";
      if (layer >= Layer_Foreground)
        return "foreground";
      return "main";
    }

    template <typename Out>
    void draw_coordinate_grid(Out & output,
                              const Geom_Box & box, const Mapping & map) const
    {
      if (not with_coordinate_grid_ or not box.initialized)
        return;

      if (grid_step_x_ <= 0.0 or grid_step_y_ <= 0.0)
        return;

      const std::string opts = draw_options(grid_style_, false);
      const double xmin = geom_number_to_double(box.xmin);
      const double xmax = geom_number_to_double(box.xmax);
      const double ymin = geom_number_to_double(box.ymin);
      const double ymax = geom_number_to_double(box.ymax);

      const double sx = grid_step_x_;
      const double sy = grid_step_y_;
      const int nx0 = static_cast<int>(std::ceil(xmin / sx));
      const int nx1 = static_cast<int>(std::floor(xmax / sx));
      const int ny0 = static_cast<int>(std::ceil(ymin / sy));
      const int ny1 = static_cast<int>(std::floor(ymax / sy));

      for (int i = nx0; i <= nx1; ++i)
        {
          const Geom_Number x = Geom_Number(static_cast<double>(i) * sx);
          const double xm = map_x(x, box, map);
          output << "    \\draw";
          if (not opts.empty())
            output << "[" << opts << "]";
          output << " (" << xm << ",0) -- (" << xm << "," << height_ << ");"
              << std::endl;
        }

      for (int i = ny0; i <= ny1; ++i)
        {
          const auto y = Geom_Number(static_cast<double>(i) * sy);
          const double ym = map_y(y, box, map);
          output << "    \\draw";
          if (not opts.empty())
            output << "[" << opts << "]";
          output << " (0," << ym << ") -- (" << wide_ << "," << ym << ");"
              << std::endl;
        }

      if (not with_grid_ticks_)
        return;

      const std::string tick_opts = draw_options(axis_style_, false);
      const bool has_x_axis = map.has_y_range and box.ymin <= 0 and 0 <= box.ymax;
      const bool has_y_axis = map.has_x_range and box.xmin <= 0 and 0 <= box.xmax;
      const double y_axis_mm = has_x_axis ? map_y(Geom_Number(0), box, map) : -1.0;
      const double x_axis_mm = has_y_axis ? map_x(Geom_Number(0), box, map) : -1.0;

      if (has_x_axis)
        for (int i = nx0; i <= nx1; ++i)
          {
            const auto x = Geom_Number(static_cast<double>(i) * sx);
            const double xm = map_x(x, box, map);
            output << "    \\draw";
            if (not tick_opts.empty())
              output << "[" << tick_opts << "]";
            output << " (" << xm << "," << (y_axis_mm - tick_size_mm_ / 2.0)
                << ") -- (" << xm << "," << (y_axis_mm + tick_size_mm_ / 2.0) << ");"
                << std::endl;
          }

      if (has_y_axis)
        for (int i = ny0; i <= ny1; ++i)
          {
            const auto y = Geom_Number(static_cast<double>(i) * sy);
            const double ym = map_y(y, box, map);
            output << "    \\draw";
            if (not tick_opts.empty())
              output << "[" << tick_opts << "]";
            output << " (" << (x_axis_mm - tick_size_mm_ / 2.0) << "," << ym
                << ") -- (" << (x_axis_mm + tick_size_mm_ / 2.0) << "," << ym << ");"
                << std::endl;
          }
    }

    [[nodiscard]] std::vector<Legend_Entry> collect_legend_entries() const
    {
      std::vector<Legend_Entry> entries = legend_entries_;
      if (not with_auto_legend_)
        return entries;

      std::unordered_set<std::string> seen;
      for (const auto & [label, style]: entries)
        seen.insert(label);

      for (const auto & obj: objects_)
        {
          std::string color = obj.style.draw_color;
          if (color.empty() and not obj.style.fill_color.empty())
            color = obj.style.fill_color;
          if (color.empty())
            continue;
          if (seen.contains(color))
            continue;

          Tikz_Style legend_style;
          legend_style.draw_color = color;
          legend_style.fill_color = obj.style.fill_color;
          legend_style.fill = obj.style.fill;
          legend_style.pattern = obj.style.pattern;
          legend_style.pattern_color = obj.style.pattern_color;
          entries.push_back({color, legend_style});
          seen.insert(color);
        }

      return entries;
    }

    template <typename Out>
    void draw_legend(Out & output) const
    {
      const std::vector<Legend_Entry> entries = collect_legend_entries();
      if (entries.empty())
        return;

      for (size_t i = 0; i < entries.size(); ++i)
        {
          const double y = height_ - legend_y_mm_ - static_cast<double>(i) * legend_entry_step_mm_;
          const double x0 = legend_x_mm_;
          const double x1 = legend_x_mm_ + 7.0;
          const std::string opts = draw_options(entries[i].style, true);

          output << "    \\draw";
          if (not opts.empty())
            output << "[" << opts << "]";
          output << " (" << x0 << "," << y << ") -- (" << x1 << "," << y << ");"
              << std::endl;

          output << "    \\node[anchor=west] at (" << (x1 + 1.2) << "," << y
              << ") {" << escape_latex(entries[i].label) << "};" << std::endl;
        }
    }

    template <typename Out>
    void draw_tikzset_styles(Out & output) const
    {
      if (tikz_style_defs_.empty())
        return;

      for (const auto & [name, style]: tikz_style_defs_)
        {
          std::string opts = draw_options(style, true);
          if (const std::string txt = text_options(style); not txt.empty())
            append_option(opts, txt);
          output << "  \\tikzset{" << name << "/.style={" << opts << "}}" << std::endl;
        }
    }

    template <typename Out>
    void draw_cartesian_axis(Out & output,
                             const Geom_Box & box, const Mapping & map) const
    {
      if (not with_cartesian_axis_ or not box.initialized)
        return;

      const std::string opts = draw_options(axis_style_, false);

      if (map.has_y_range and box.ymin <= 0 and 0 <= box.ymax)
        {
          const double y0 = map_y(Geom_Number(0), box, map);
          output << "    \\draw";
          if (not opts.empty())
            output << "[" << opts << "]";
          output << " (0," << y0 << ") -- (" << wide_ << "," << y0 << ");"
              << std::endl;
        }

      if (map.has_x_range and box.xmin <= 0 and 0 <= box.xmax)
        {
          const double x0 = map_x(Geom_Number(0), box, map);
          output << "    \\draw";
          if (not opts.empty())
            output << "[" << opts << "]";
          output << " (" << x0 << ",0) -- (" << x0 << "," << height_ << ");"
              << std::endl;
        }
    }

    template <typename Out>
    void draw_object(const Styled_Object & entry, Out & output,
                     const Geom_Box & box, const Mapping & map) const
    {
      std::visit([&]<typename T0>(const T0 & value)
                   {
                     using T = std::decay_t<T0>;
                     if constexpr (std::is_same_v<T, Point>)
                       draw_point(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Polar_Point>)
                       draw_point(Point(value), entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Segment>)
                       draw_segment(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Triangle>)
                       draw_triangle(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Ellipse>)
                       draw_ellipse(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, RotatedEllipse>)
                       draw_rotated_ellipse(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Text>)
                       draw_text(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Polygon>)
                       draw_polygon(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Regular_Polygon>)
                       draw_regular_polygon(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Rectangle>)
                       draw_rectangle(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, LineEq>)
                       draw_line_eq(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Tikz_Quadratic_Bezier>)
                       draw_quadratic_bezier(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Tikz_Cubic_Bezier>)
                       draw_cubic_bezier(value, entry.style, output, box, map);
                     else if constexpr (std::is_same_v<T, Tikz_Polyline>)
                       draw_polyline(output, value.vertices, value.closed,
                                     entry.style, box, map);
                   },
                 entry.object);
    }

  public:
    /** @brief Construct a TikZ plane with fixed frame size.
     *
     *  @param wide Plane width in millimeters.
     *  @param height Plane height in millimeters.
     *  @param xoffset X shift (mm) applied to the internal drawing scope.
     *  @param yoffset Y shift (mm) applied to the internal drawing scope.
     *
     *  @throw std::domain_error If `wide <= 0` or `height <= 0`.
     */
    Tikz_Plane(const double & wide, const double & height,
               const double & xoffset = 0.0, const double & yoffset = 0.0)
      : wide_(wide), height_(height), xoffset_(xoffset), yoffset_(yoffset)
    {
      ah_domain_error_if(wide_ <= 0 or height_ <= 0)
        << "Tikz_Plane width and height must be greater than zero";
    }

    /// @brief Plane width in millimeters.
    [[nodiscard]] const double &get_wide() const { return wide_; }
    /// @brief Plane height in millimeters.
    [[nodiscard]] const double &get_height() const { return height_; }
    /// @brief Horizontal scope offset in millimeters.
    [[nodiscard]] const double &get_xoffset() const { return xoffset_; }
    /// @brief Vertical scope offset in millimeters.
    [[nodiscard]] const double &get_yoffset() const { return yoffset_; }
    /// @brief Radius used to draw point primitives (millimeters).
    [[nodiscard]] const double &get_point_radius_mm() const { return point_radius_mm_; }
    /// @brief Clip expansion around the frame; <= 0 means auto (point radius).
    [[nodiscard]] const double &get_clip_padding_mm() const { return clip_padding_mm_; }
    /// @brief Number of currently inserted objects.
    [[nodiscard]] size_t size() const { return objects_.size(); }

    /// @brief Default style used by `put(obj)` and non-styled `put_in_plane`.
    [[nodiscard]] const Tikz_Style &get_default_style() const { return default_style_; }

    /// @brief Style used for Cartesian axes when enabled.
    [[nodiscard]] const Tikz_Style &get_axis_style() const { return axis_style_; }
    [[nodiscard]] const Tikz_Style &get_grid_style() const { return grid_style_; }

    /** @brief Configure the default style for subsequent non-styled inserts. */
    void set_default_style(const Tikz_Style & style)
    {
      default_style_ = style;
    }

    /** @brief Configure the style used to draw Cartesian axes. */
    void set_axis_style(const Tikz_Style & style)
    {
      axis_style_ = style;
    }

    /** @brief Configure the style used to draw coordinate grids. */
    void set_grid_style(const Tikz_Style & style)
    {
      grid_style_ = style;
    }

    /** @brief Configure point marker radius.
     *
     *  @param radius_mm Circle radius used for `Point` and degenerate fallbacks.
     *  @throw std::domain_error If `radius_mm <= 0`.
     */
    void set_point_radius_mm(const double & radius_mm)
    {
      ah_domain_error_if(radius_mm <= 0) << "point radius must be greater than zero";
      point_radius_mm_ = radius_mm;
    }

    /** @brief Configure clip expansion around frame borders.
     *
     *  When <= 0, the plane uses `point_radius_mm_` automatically.
     */
    void set_clip_padding_mm(const double & padding_mm)
    {
      clip_padding_mm_ = padding_mm;
    }

    /** @brief Enable Cartesian axes drawing (only when 0 lies in range). */
    void put_cartesian_axis()
    {
      with_cartesian_axis_ = true;
    }

    /// @brief Disable Cartesian axes drawing.
    void remove_cartesian_axis()
    {
      with_cartesian_axis_ = false;
    }

    /** @brief Enable coordinate grid/ticks with user step sizes. */
    void put_coordinate_grid(const double step_x = 1.0,
                             const double step_y = 1.0,
                             const bool draw_ticks = true)
    {
      ah_domain_error_if(step_x <= 0.0 or step_y <= 0.0)
        << "Grid steps must be greater than zero";
      with_coordinate_grid_ = true;
      with_grid_ticks_ = draw_ticks;
      grid_step_x_ = step_x;
      grid_step_y_ = step_y;
    }

    /// @brief Disable coordinate grid rendering.
    void remove_coordinate_grid()
    {
      with_coordinate_grid_ = false;
    }

    /** @brief Enable native PGF layers (`\pgfdeclarelayer`). */
    void enable_native_tikz_layers(const bool enabled = true)
    {
      with_native_layers_ = enabled;
    }

    /** @brief Enable auto-legend generation from style colors. */
    void enable_auto_legend(const bool enabled = true)
    {
      with_auto_legend_ = enabled;
    }

    /** @brief Add a legend entry to be rendered in the top-left corner. */
    void add_legend_entry(const std::string & label, const Tikz_Style & style)
    {
      legend_entries_.push_back({label, style});
    }

    /// @brief Remove user-provided legend entries.
    void clear_legend()
    {
      legend_entries_.clear();
    }

    /** @brief Register a reusable `\tikzset` style for the rendered picture. */
    void register_tikz_style(const std::string & name, const Tikz_Style & style)
    {
      ah_domain_error_if(name.empty()) << "Style name cannot be empty";
      tikz_style_defs_[name] = style;
    }

    /// @brief Clear all registered `\tikzset` style definitions.
    void clear_tikz_styles()
    {
      tikz_style_defs_.clear();
    }

    /// @brief Remove all inserted objects from the plane.
    void clear()
    {
      objects_.clear();
      next_order_ = 0;
    }

    /** @brief Insert an object with default style on default layer. */
    void put(const Object & obj)
    {
      put(obj, default_style_, Layer_Default);
    }

    /** @brief Insert an object with explicit style/layer. */
    void put(const Object & obj, const Tikz_Style & style, const int layer)
    {
      objects_.push_back(Styled_Object{obj, style, layer, next_order_++});
    }

    /** @brief Emit a complete `tikzpicture` with all inserted objects.
     *
     *  @param output Destination stream.
     *  @param squarize When true, preserve aspect ratio (uniform scaling). When
     *         false, x/y scales are independent and fill the full frame.
     */
    void draw(std::ostream & output, const bool squarize = true) const
    {
      const Geom_Box box = compute_box();
      const Mapping map = compute_mapping(box, squarize);

      output << std::fixed << std::setprecision(6);
      output << "% Requires: \\usepackage{tikz}" << std::endl;
      if (with_native_layers_)
        {
          output << "\\pgfdeclarelayer{background}" << std::endl
              << "\\pgfdeclarelayer{foreground}" << std::endl
              << "\\pgfdeclarelayer{overlay}" << std::endl;
        }
      output << "\\begin{tikzpicture}[x=1mm,y=1mm]" << std::endl;
      if (with_native_layers_)
        output << "  \\pgfsetlayers{background,main,foreground,overlay}" << std::endl;
      draw_tikzset_styles(output);
      output << "  % " << objects_.size() << " geometric objects in plane" << std::endl;
      output << "  \\begin{scope}[shift={(" << xoffset_ << "mm," << yoffset_ << "mm)}]"
          << std::endl;
      const double clip_padding = effective_clip_padding_mm();
      output << "    \\clip (" << -clip_padding << "," << -clip_padding << ") rectangle ("
          << (wide_ + clip_padding) << "," << (height_ + clip_padding) << ");"
          << std::endl;

      draw_coordinate_grid(output, box, map);
      draw_cartesian_axis(output, box, map);

      std::vector<size_t> order(objects_.size());
      for (size_t i = 0; i < objects_.size(); ++i)
        order[i] = i;

      std::stable_sort(order.begin(), order.end(),
                       [&](const size_t lhs, const size_t rhs)
                         {
                           if (objects_[lhs].layer != objects_[rhs].layer)
                             return objects_[lhs].layer < objects_[rhs].layer;
                           return objects_[lhs].order < objects_[rhs].order;
                         });

      if (with_native_layers_)
        {
          const char *current = nullptr;
          for (const size_t idx: order)
            {
              const char *target = layer_name(objects_[idx].layer);
              if (current == nullptr or std::string(current) != target)
                {
                  if (current != nullptr)
                    output << "    \\end{pgfonlayer}" << std::endl;
                  output << "    \\begin{pgfonlayer}{" << target << "}" << std::endl;
                  current = target;
                }
              draw_object(objects_[idx], output, box, map);
            }
          if (current != nullptr)
            output << "    \\end{pgfonlayer}" << std::endl;
        }
      else
        for (const size_t idx: order)
          draw_object(objects_[idx], output, box, map);

      draw_legend(output);

      output << "  \\end{scope}" << std::endl;
      output << "\\end{tikzpicture}" << std::endl;
    }
  };

  /** @brief Insert any supported geometry type in a @ref Tikz_Plane. */
  template <typename Geom,
            typename = std::enable_if_t<
              std::is_constructible_v<Tikz_Plane::Object, std::decay_t<Geom>>>>
  inline void put_in_plane(Tikz_Plane & plane, const Geom & geom_obj)
  {
    plane.put(Tikz_Plane::Object(geom_obj));
  }

  /** @brief Insert any supported geometry type with style/layer. */
  template <typename Geom,
            typename = std::enable_if_t<
              std::is_constructible_v<Tikz_Plane::Object, std::decay_t<Geom>>>>
  inline void put_in_plane(Tikz_Plane & plane, const Geom & geom_obj,
                           const Tikz_Style & style,
                           const int layer = Tikz_Plane::Layer_Default)
  {
    plane.put(Tikz_Plane::Object(geom_obj), style, layer);
  }

  /// @brief Approximate and insert a quadratic Bézier as an open polyline.
  inline void put_quadratic_bezier_in_plane(Tikz_Plane & plane,
                                            const Point & p0,
                                            const Point & p1,
                                            const Point & p2,
                                            const size_t subdivisions = 64,
                                            const Tikz_Style & style = {},
                                            const int layer = Tikz_Plane::Layer_Default)
  {
    ah_domain_error_if(subdivisions == 0) << "Need at least 1 subdivision";

    Tikz_Polyline curve;
    curve.vertices.reserve(subdivisions + 1);
    for (size_t i = 0; i <= subdivisions; ++i)
      {
        const Geom_Number t = Geom_Number(i) / Geom_Number(subdivisions);
        const Geom_Number s = Geom_Number(1) - t;
        const Geom_Number x = s * s * p0.get_x() +
                              Geom_Number(2) * s * t * p1.get_x() +
                              t * t * p2.get_x();
        const Geom_Number y = s * s * p0.get_y() +
                              Geom_Number(2) * s * t * p1.get_y() +
                              t * t * p2.get_y();
        curve.vertices.emplace_back(x, y);
      }

    put_in_plane(plane, curve, style, layer);
  }

  /// @brief Approximate and insert a cubic Bézier as an open polyline.
  inline void put_cubic_bezier_in_plane(Tikz_Plane & plane,
                                        const Point & p0,
                                        const Point & p1,
                                        const Point & p2,
                                        const Point & p3,
                                        const size_t subdivisions = 64,
                                        const Tikz_Style & style = {},
                                        const int layer = Tikz_Plane::Layer_Default)
  {
    ah_domain_error_if(subdivisions == 0) << "Need at least 1 subdivision";

    Tikz_Polyline curve;
    curve.vertices.reserve(subdivisions + 1);
    for (size_t i = 0; i <= subdivisions; ++i)
      {
        const Geom_Number t = Geom_Number(i) / Geom_Number(subdivisions);
        const Geom_Number s = Geom_Number(1) - t;
        const Geom_Number s2 = s * s;
        const Geom_Number t2 = t * t;
        const Geom_Number x = s2 * s * p0.get_x() +
                              Geom_Number(3) * s2 * t * p1.get_x() +
                              Geom_Number(3) * s * t2 * p2.get_x() +
                              t2 * t * p3.get_x();
        const Geom_Number y = s2 * s * p0.get_y() +
                              Geom_Number(3) * s2 * t * p1.get_y() +
                              Geom_Number(3) * s * t2 * p2.get_y() +
                              t2 * t * p3.get_y();
        curve.vertices.emplace_back(x, y);
      }

    put_in_plane(plane, curve, style, layer);
  }

  /// @brief Insert a quadratic Bézier using native TikZ controls syntax.
  inline void put_quadratic_bezier_native_in_plane(
      Tikz_Plane & plane,
      const Point & p0,
      const Point & p1,
      const Point & p2,
      const Tikz_Style & style = {},
      const int layer = Tikz_Plane::Layer_Default)
  {
    put_in_plane(plane, Tikz_Quadratic_Bezier{p0, p1, p2}, style, layer);
  }

  /// @brief Insert a cubic Bézier using native TikZ controls syntax.
  inline void put_cubic_bezier_native_in_plane(
      Tikz_Plane & plane,
      const Point & p0,
      const Point & p1,
      const Point & p2,
      const Point & p3,
      const Tikz_Style & style = {},
      const int layer = Tikz_Plane::Layer_Default)
  {
    put_in_plane(plane, Tikz_Cubic_Bezier{p0, p1, p2, p3}, style, layer);
  }

  /// @brief Insert a point label with configurable placement (`above`, etc.).
  inline void put_point_label_in_plane(
      Tikz_Plane & plane,
      const Point & point,
      const std::string & label,
      const std::string & placement = "above",
      const Tikz_Style & style = make_tikz_draw_style("black"),
      const int layer = Tikz_Plane::Layer_Overlay)
  {
    Tikz_Style label_style = style;
    label_style.text_placement = placement;
    put_in_plane(plane, Text(point, label), label_style, layer);
  }
} // namespace Aleph

# endif // TIKZGEOM_H
