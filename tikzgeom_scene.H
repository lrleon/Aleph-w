# ifndef TIKZGEOM_SCENE_H
# define TIKZGEOM_SCENE_H

# include <ostream>
# include <sstream>
# include <string>
# include <type_traits>
# include <utility>
# include <vector>

# include "tikzgeom_algorithms.H"

namespace Aleph
{

/** @brief Options used by @ref Tikz_Scene standalone document export. */
struct Tikz_Standalone_Document_Options
{
  std::string document_class = "standalone";
  std::string class_options = "tikz,border=8pt";
  std::string extra_preamble;
};

/** @brief Options used by @ref Tikz_Scene beamer/handout export helpers. */
struct Tikz_Beamer_Document_Options
{
  /** @brief `beamer` class options (for example `aspectratio=169`). */
  std::string class_options = "aspectratio=169";
  /** @brief Frame options passed to `\\begin{frame}[...]`. */
  std::string frame_options = "t";
  /** @brief Frame title used by scene exports. */
  std::string frame_title = "TikZ Scene";
  /** @brief Whether to hide navigation symbols (`\\setbeamertemplate`). */
  bool hide_navigation_symbols = true;
  /** @brief Extra LaTeX preamble lines inserted before `\\begin{document}`. */
  std::string extra_preamble;
};

/** @brief High-level scene wrapper to compose objects and algorithm visualizations.
 *
 *  `Tikz_Scene` owns a @ref Tikz_Plane and exposes:
 *  - object insertion (`add`),
 *  - algorithm wrappers (`visualize_*`), and
 *  - export helpers (`draw`, `to_tikz`, `draw_standalone`, `to_standalone`,
 *    `draw_beamer`, `draw_handout`, overlay deck helpers).
 *
 *  This keeps a single interface when a figure combines multiple geometric
 *  layers (inputs, intermediate states, final outputs, annotations).
 */
class Tikz_Scene
{
  [[nodiscard]] static std::string compose_beamer_class_options(
      const bool handout_mode,
      const std::string & class_options)
  {
    if (not handout_mode)
      return class_options;

    if (class_options.empty())
      return "handout";

    if (class_options.find("handout") != std::string::npos)
      return class_options;

    return "handout," + class_options;
  }

  static void draw_beamer_document_preamble(
      std::ostream & output,
      const Tikz_Beamer_Document_Options & options,
      const bool handout_mode)
  {
    output << "\\documentclass";

    const std::string class_options =
        compose_beamer_class_options(handout_mode, options.class_options);
    if (not class_options.empty())
      output << "[" << class_options << "]";

    output << "{beamer}\n"
           << "\\usepackage{tikz}\n";

    if (options.hide_navigation_symbols)
      output << "\\setbeamertemplate{navigation symbols}{}\n";

    if (not options.extra_preamble.empty())
      output << options.extra_preamble << '\n';

    output << "\\begin{document}\n\n";
  }

  static void draw_beamer_document_epilogue(std::ostream & output)
  {
    output << "\n\\end{document}\n";
  }

  static void draw_beamer_frame_begin(
      std::ostream & output,
      const std::string & frame_title,
      const std::string & frame_options)
  {
    output << "\\begin{frame}";
    if (not frame_options.empty())
      output << "[" << frame_options << "]";
    output << "{" << frame_title << "}\n";
  }

  static void draw_beamer_overlays_document(
      std::ostream & output,
      const std::vector<Tikz_Scene> & steps,
      const Tikz_Beamer_Document_Options & options,
      const bool handout_mode)
  {
    draw_beamer_document_preamble(output, options, handout_mode);
    draw_beamer_frame_begin(output, options.frame_title, options.frame_options);

    if (steps.empty())
      output << "\\centering\\small No overlays provided.\n";
    else
      for (size_t i = 0; i < steps.size(); ++i)
        {
          output << "\\only<" << (i + 1) << ">{\n";
          steps[i].draw(output);
          output << "}\n";
        }

    output << "\\end{frame}\n";
    draw_beamer_document_epilogue(output);
  }

  void draw_beamer_document(
      std::ostream & output,
      const Tikz_Beamer_Document_Options & options,
      const bool handout_mode) const
  {
    draw_beamer_document_preamble(output, options, handout_mode);
    draw_beamer_frame(output, options.frame_title, options.frame_options);
    draw_beamer_document_epilogue(output);
  }

  Tikz_Plane plane_;
  bool squarize_ = true;

public:
  Tikz_Scene(const double width_mm,
             const double height_mm,
             const double xoffset_mm = 0.0,
             const double yoffset_mm = 0.0,
             const bool squarize = true)
    : plane_(width_mm, height_mm, xoffset_mm, yoffset_mm),
      squarize_(squarize)
  {
  }

  [[nodiscard]] Tikz_Plane & plane() { return plane_; }
  [[nodiscard]] const Tikz_Plane & plane() const { return plane_; }

  [[nodiscard]] bool squarize() const { return squarize_; }
  void set_squarize(const bool value) { squarize_ = value; }

  Tikz_Scene & clear()
  {
    plane_.clear();
    return *this;
  }

  Tikz_Scene & put_cartesian_axis()
  {
    plane_.put_cartesian_axis();
    return *this;
  }

  Tikz_Scene & remove_cartesian_axis()
  {
    plane_.remove_cartesian_axis();
    return *this;
  }

  Tikz_Scene & set_default_style(const Tikz_Style & style)
  {
    plane_.set_default_style(style);
    return *this;
  }

  Tikz_Scene & set_axis_style(const Tikz_Style & style)
  {
    plane_.set_axis_style(style);
    return *this;
  }

  Tikz_Scene & set_point_radius_mm(const double radius_mm)
  {
    plane_.set_point_radius_mm(radius_mm);
    return *this;
  }

  template <typename Geom>
  Tikz_Scene & add(const Geom & object)
  {
    put_in_plane(plane_, object);
    return *this;
  }

  template <typename Geom>
  Tikz_Scene & add(const Geom & object,
                   const Tikz_Style & style,
                   const int layer = Tikz_Plane::Layer_Default)
  {
    put_in_plane(plane_, object, style, layer);
    return *this;
  }

  Tikz_Scene & add_points(
      const Array<Point> & points,
      const Tikz_Style & style = tikz_points_style("black"),
      const int layer = Tikz_Plane::Layer_Default)
  {
    Aleph::put_points(plane_, points, style, layer);
    return *this;
  }

  Tikz_Scene & add_points(
      const DynList<Point> & points,
      const Tikz_Style & style = tikz_points_style("black"),
      const int layer = Tikz_Plane::Layer_Default)
  {
    Aleph::put_points(plane_, points, style, layer);
    return *this;
  }

  Tikz_Scene & add_polygons(
      const Array<Polygon> & polys,
      const Tikz_Style & style = tikz_wire_style("black", true),
      const int layer = Tikz_Plane::Layer_Default)
  {
    Aleph::put_polygons(plane_, polys, style, layer);
    return *this;
  }

  template <typename Points, typename HullAlgorithm, typename... Args>
  Polygon visualize_convex_hull(const Points & points,
                                HullAlgorithm && algorithm,
                                Args &&... args)
  {
    if constexpr (std::is_same_v<std::decay_t<Points>, Array<Point>>)
      {
        DynList<Point> converted;
        for (size_t i = 0; i < points.size(); ++i)
          converted.append(points(i));

        return Aleph::visualize_convex_hull(
            plane_,
            converted,
            std::forward<HullAlgorithm>(algorithm),
            std::forward<Args>(args)...);
      }
    else
      {
        return Aleph::visualize_convex_hull(
            plane_,
            points,
            std::forward<HullAlgorithm>(algorithm),
            std::forward<Args>(args)...);
      }
  }

  template <typename... Args>
  Polygon visualize_convex_intersection(Args &&... args)
  {
    return Aleph::visualize_convex_intersection(
        plane_, std::forward<Args>(args)...);
  }

  template <typename... Args>
  Array<Polygon> visualize_boolean_operation(Args &&... args)
  {
    return Aleph::visualize_boolean_operation(
        plane_, std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto visualize_voronoi(Args &&... args)
  {
    return Aleph::visualize_voronoi(
        plane_, std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto visualize_power_diagram(Args &&... args)
  {
    return Aleph::visualize_power_diagram(
        plane_, std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto visualize_segment_arrangement(Args &&... args)
  {
    return Aleph::visualize_segment_arrangement(
        plane_, std::forward<Args>(args)...);
  }

  template <typename... Args>
  DynList<Point> visualize_shortest_path_in_polygon(Args &&... args)
  {
    return Aleph::visualize_shortest_path_in_polygon(
        plane_, std::forward<Args>(args)...);
  }

  template <typename... Args>
  ShortestPathDebugResult visualize_shortest_path_with_portals(Args &&... args)
  {
    return Aleph::visualize_shortest_path_with_portals(
        plane_, std::forward<Args>(args)...);
  }

  template <typename... Args>
  Array<Polygon> visualize_convex_decomposition(Args &&... args)
  {
    return Aleph::visualize_convex_decomposition(
        plane_, std::forward<Args>(args)...);
  }

  template <typename... Args>
  AlphaShape::Result visualize_alpha_shape(Args &&... args)
  {
    return Aleph::visualize_alpha_shape(
        plane_, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void put_funnel_trace_step(Args &&... args)
  {
    Aleph::put_funnel_trace_step(plane_, std::forward<Args>(args)...);
  }

  template <typename Callable>
  decltype(auto) run(Callable && callable)
  {
    return std::forward<Callable>(callable)(plane_);
  }

  template <typename Callable>
  decltype(auto) run(Callable && callable) const
  {
    return std::forward<Callable>(callable)(plane_);
  }

  void draw(std::ostream & output) const
  {
    plane_.draw(output, squarize_);
  }

  [[nodiscard]] std::string to_tikz() const
  {
    std::ostringstream out;
    draw(out);
    return out.str();
  }

  void draw_beamer_frame(
      std::ostream & output,
      const std::string & frame_title = "TikZ Scene",
      const std::string & frame_options = "t") const
  {
    draw_beamer_frame_begin(output, frame_title, frame_options);

    draw(output);
    output << "\\end{frame}\n";
  }

  void draw_standalone(
      std::ostream & output,
      const Tikz_Standalone_Document_Options & options = {}) const
  {
    output << "\\documentclass";
    if (not options.class_options.empty())
      output << "[" << options.class_options << "]";

    output << "{";
    if (options.document_class.empty())
      output << "standalone";
    else
      output << options.document_class;
    output << "}\n"
           << "\\usepackage{tikz}\n";

    if (not options.extra_preamble.empty())
      output << options.extra_preamble << '\n';

    output << "\\begin{document}\n\n";
    draw(output);
    output << "\n\\end{document}\n";
  }

  [[nodiscard]] std::string to_standalone(
      const Tikz_Standalone_Document_Options & options = {}) const
  {
    std::ostringstream out;
    draw_standalone(out, options);
    return out.str();
  }

  void draw_beamer(
      std::ostream & output,
      const Tikz_Beamer_Document_Options & options = {}) const
  {
    draw_beamer_document(output, options, false);
  }

  [[nodiscard]] std::string to_beamer(
      const Tikz_Beamer_Document_Options & options = {}) const
  {
    std::ostringstream out;
    draw_beamer(out, options);
    return out.str();
  }

  void draw_handout(
      std::ostream & output,
      const Tikz_Beamer_Document_Options & options = {}) const
  {
    draw_beamer_document(output, options, true);
  }

  [[nodiscard]] std::string to_handout(
      const Tikz_Beamer_Document_Options & options = {}) const
  {
    std::ostringstream out;
    draw_handout(out, options);
    return out.str();
  }

  /** @brief Export multiple scenes as beamer overlays (`\\only<k>{...}`) in one frame. */
  static void draw_beamer_overlays(
      std::ostream & output,
      const std::vector<Tikz_Scene> & steps,
      const Tikz_Beamer_Document_Options & options = {})
  {
    draw_beamer_overlays_document(output, steps, options, false);
  }

  /** @brief String variant of @ref draw_beamer_overlays. */
  [[nodiscard]] static std::string to_beamer_overlays(
      const std::vector<Tikz_Scene> & steps,
      const Tikz_Beamer_Document_Options & options = {})
  {
    std::ostringstream out;
    draw_beamer_overlays(out, steps, options);
    return out.str();
  }

  /** @brief Export multiple scenes as handout overlays (forces `beamer[handout]`). */
  static void draw_handout_overlays(
      std::ostream & output,
      const std::vector<Tikz_Scene> & steps,
      const Tikz_Beamer_Document_Options & options = {})
  {
    draw_beamer_overlays_document(output, steps, options, true);
  }

  /** @brief String variant of @ref draw_handout_overlays. */
  [[nodiscard]] static std::string to_handout_overlays(
      const std::vector<Tikz_Scene> & steps,
      const Tikz_Beamer_Document_Options & options = {})
  {
    std::ostringstream out;
    draw_handout_overlays(out, steps, options);
    return out.str();
  }
};

} // namespace Aleph

# endif // TIKZGEOM_SCENE_H
