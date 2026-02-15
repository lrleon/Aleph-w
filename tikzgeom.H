# ifndef TIKZGEOM_H
# define TIKZGEOM_H

# include <algorithm>
# include <array>
# include <cmath>
# include <cstddef>
# include <iomanip>
# include <ostream>
# include <sstream>
# include <string>
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
  std::string draw_color = "black"; ///< TikZ draw color (`draw=<color>`)
  std::string fill_color; ///< TikZ fill color (`fill=<color>`)
  std::string text_color; ///< TikZ text color (`text=<color>`)
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
                              Polygon, Regular_Polygon, Rectangle, LineEq>;

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
  double point_radius_mm_ = 1.0;
  double clip_padding_mm_ = -1.0;

  Tikz_Style default_style_;
  Tikz_Style axis_style_ = []()
  {
    Tikz_Style s;
    s.draw_color = "gray";
    s.with_arrow = true;
    return s;
  }();

  std::vector<Styled_Object> objects_;
  size_t next_order_ = 0;

  [[nodiscard]] static std::string fmt_double(const double v)
  {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << v;

    std::string out = ss.str();
    const auto dot_pos = out.find('.');
    if (dot_pos != std::string::npos)
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

    if (not style.draw_color.empty())
      append_option(opts, "draw=" + style.draw_color);

    if (allow_fill and style.fill)
      {
        if (not style.fill_color.empty())
          append_option(opts, "fill=" + style.fill_color);
        else if (not style.draw_color.empty())
          append_option(opts, "fill=" + style.draw_color);
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

    if (not style.text_color.empty())
      append_option(opts, "text=" + style.text_color);
    else if (not style.draw_color.empty())
      append_option(opts, "text=" + style.draw_color);

    if (style.opacity >= 0.0)
      append_option(opts, "opacity=" + fmt_double(style.opacity));

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
    const Geom_Number x0 = Geom_Number(0);
    const Geom_Number x1 = Geom_Number(1);
    const Geom_Number y0 = l(x0);
    const Geom_Number y1 = l(x1);
    return make_box(std::min(x0, x1), std::max(x0, x1),
                    std::min(y0, y1), std::max(y0, y1));
  }

  Geom_Box compute_box() const
  {
    Geom_Box box;
    for (const auto & entry : objects_)
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
    for (const char ch : text)
      switch (ch)
        {
        case '\\': escaped += "\\textbackslash{}"; break;
        case '{': escaped += "\\{"; break;
        case '}': escaped += "\\}"; break;
        case '$': escaped += "\\$"; break;
        case '%': escaped += "\\%"; break;
        case '#': escaped += "\\#"; break;
        case '&': escaped += "\\&"; break;
        case '_': escaped += "\\_"; break;
        case '^': escaped += "\\^{}"; break;
        case '~': escaped += "\\~{}"; break;
        default: escaped.push_back(ch); break;
        }
    return escaped;
  }

  Mapping compute_mapping(const Geom_Box & box, const bool squarize) const
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

  double map_x(const Geom_Number & x,
               const Geom_Box & box, const Mapping & map) const
  {
    if (not map.has_x_range)
      return map.dx;

    return map.dx + geom_number_to_double(x - box.xmin) * map.sx;
  }

  double map_y(const Geom_Number & y,
               const Geom_Box & box, const Mapping & map) const
  {
    if (not map.has_y_range)
      return map.dy;

    return map.dy + geom_number_to_double(y - box.ymin) * map.sy;
  }

  std::pair<double, double>
  map_point(const Point & p, const Geom_Box & box, const Mapping & map) const
  {
    return {map_x(p.get_x(), box, map), map_y(p.get_y(), box, map)};
  }

  double map_x_length(const Geom_Number & dx, const Mapping & map) const
  {
    if (not map.has_x_range)
      return 0.0;
    return std::fabs(geom_number_to_double(dx)) * map.sx;
  }

  double map_y_length(const Geom_Number & dy, const Mapping & map) const
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
  void draw_rotated_ellipse(const RotatedEllipse & e, const Tikz_Style & style,
                            Out & output, const Geom_Box & box,
                            const Mapping & map) const
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
  void draw_text(const Text & t, const Tikz_Style & style, Out & output,
                 const Geom_Box & box, const Mapping & map) const
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
    for (const Point & p : corners)
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
        for (const Point & existing : clipped)
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

    const Geom_Number y_left = l(box.xmin);
    if (y_in_range(y_left))
      append_if_new(Point(box.xmin, y_left));

    const Geom_Number y_right = l(box.xmax);
    if (y_in_range(y_right))
      append_if_new(Point(box.xmax, y_right));

    if (l.m != Geom_Number(0))
      {
        const Geom_Number x_bottom = (box.ymin - l.y0) / l.m;
        if (x_in_range(x_bottom))
          append_if_new(Point(x_bottom, box.ymin));

        const Geom_Number x_top = (box.ymax - l.y0) / l.m;
        if (x_in_range(x_top))
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
    std::visit(
        [&](const auto & value)
        {
          using T = std::decay_t<decltype(value)>;
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

  /** @brief Configure point marker radius.
   *
   *  @param radius_mm Circle radius used for `Point` and degenerate fallbacks.
   *  @throw std::domain_error If `radius_mm <= 0`.
   */
  void set_point_radius_mm(const double & radius_mm)
  {
    ah_domain_error_if(radius_mm <= 0)
      << "point radius must be greater than zero";
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
    output << "\\begin{tikzpicture}[x=1mm,y=1mm]" << std::endl;
    output << "  % " << objects_.size() << " geometric objects in plane" << std::endl;
    output << "  \\begin{scope}[shift={(" << xoffset_ << "mm," << yoffset_ << "mm)}]"
           << std::endl;
    const double clip_padding = effective_clip_padding_mm();
    output << "    \\clip (" << -clip_padding << "," << -clip_padding << ") rectangle ("
           << (wide_ + clip_padding) << "," << (height_ + clip_padding) << ");"
           << std::endl;

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

    for (const size_t idx : order)
      draw_object(objects_[idx], output, box, map);

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

  Polygon curve;
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
      curve.add_vertex(Point(x, y));
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

  Polygon curve;
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
      curve.add_vertex(Point(x, y));
    }

  put_in_plane(plane, curve, style, layer);
}

} // namespace Aleph

# endif // TIKZGEOM_H
