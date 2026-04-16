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


/** @file tpl_scope.H
 *  @brief Gestión de ámbitos léxicos (lexical scopes) mediante pilas de marcos.
 *
 *  Este encabezado proporciona la clase `Scope`, una estructura fundamental para
 *  compiladores e intérpretes que necesitan gestionar la visibilidad de nombres
 *  (bindings) siguiendo reglas de sombreado (shadowing) léxico.
 *
 *  La misión de este módulo es permitir que el usuario defina "niveles" de
 *  visibilidad. Al entrar en un nuevo ámbito (p.ej. una función o un bloque `{}`),
 *  se apila un marco. Las inserciones ocurren en el marco actual, y las
 *  búsquedas recorren la pila desde el interior hacia el exterior.
 *
 *  @ingroup Utilities
 */

# ifndef TPL_SCOPE_H
# define TPL_SCOPE_H

# include <concepts>
# include <cstddef>
# include <utility>

# include <ah-errors.H>
# include <tpl_dynArray.H>

namespace Aleph
{
  /** @brief Pila de ámbitos léxicos para asociaciones de `Key` a `Value`.
   *
   *  La clase `Scope` implementa un diccionario jerárquico. Es ideal para tablas
   *  de símbolos donde las declaraciones en bloques internos ocultan las del
   *  exterior.
   *
   *  ### Ejemplo de uso:
   *  @code
   *  Scope<string, Symbol*> symbols;
   *  symbols.enter_scope(); // Entrar en ámbito global
   *  symbols.insert("x", var1);
   *
   *  symbols.enter_scope(); // Entrar en un bloque { ... }
   *  symbols.insert("x", var2); // Sombrea la "x" global
   *  auto* s = symbols.lookup("x"); // Retorna var2
   *
   *  symbols.leave_scope(); // Salir del bloque
   *  s = symbols.lookup("x"); // Retorna var1
   *  @endcode
   *
   *  @tparam Key Tipo de la clave de búsqueda. Debe ser comparable por igualdad.
   *  @tparam Value Carga útil asociada a la clave.
   *
   *  @note Thread-safety: No es seguro para hilos en operaciones concurrentes
   *        de mutación. Las operaciones `const` (lookup, depth, empty) son 
   *        seguras para lectores concurrentes solo si no hay mutaciones activas.
   *  @note Complexity: O(1) para construcción.
   */
  template <typename Key, typename Value>
    requires std::equality_comparable<Key>
  class Scope
  {
  public:
    /** @brief Entrada individual en un marco léxico.
     *
     *  Representa un par clave-valor almacenado en el nivel de visibilidad actual.
     */
    struct Binding
    {
      Key key; ///< Clave de búsqueda (ej. nombre de variable).
      Value value; ///< Valor asociado (ej. tipo o dirección).
    };

  private:
    struct Frame
    {
      DynArray<Binding> bindings;
    };

    DynArray<Frame> frames;

    static Value *
    find_value(DynArray<Binding> & bindings, const Key & key) noexcept
    {
      for (size_t i = bindings.size(); i > 0; --i)
        if (bindings.access(i - 1).key == key)
          return &bindings.access(i - 1).value;
      return nullptr;
    }

    static const Value *
    find_value(const DynArray<Binding> & bindings, const Key & key) noexcept
    {
      for (size_t i = bindings.size(); i > 0; --i)
        if (bindings.access(i - 1).key == key)
          return &bindings.access(i - 1).value;
      return nullptr;
    }

    Frame &
    current_frame()
    {
      ah_runtime_error_unless(frames.size() > 0)
        << "Scope::current_frame(): no active scope";
      return frames.top();
    }

    const Frame &
    current_frame() const
    {
      ah_runtime_error_unless(frames.size() > 0)
        << "Scope::current_frame(): no active scope";
      return frames.top();
    }

  public:
    /** @brief Constructor por defecto. Crea una instancia sin marcos activos.
     *
     *  Debe llamarse a `enter_scope()` antes de realizar la primera inserción.
     */
    Scope() = default;

    /** @brief Consulta la profundidad actual de la pila de ámbitos.
     *
     *  @return Cantidad de marcos (scopes) activos.
     *  @note Un retorno de 0 indica que no se puede insertar nada hasta llamar
     *        a `enter_scope()`.
     */
    [[nodiscard]] size_t depth() const noexcept
    {
      return frames.size();
    }

    /** @brief Indica si la estructura está vacía de marcos.
     *
     *  @return `true` si no hay ningún ámbito activo.
     */
    [[nodiscard]] bool empty() const noexcept
    {
      return frames.is_empty();
    }

    /** @brief Reinicia la estructura eliminando todos los marcos y bindings.
     *
     *  Útil para reutilizar el objeto `Scope` en una nueva unidad de traducción.
     */
    void clear() noexcept
    {
      frames.clear();
    }

    /** @brief Crea un nuevo nivel de visibilidad.
     *
     *  Apila un marco vacío. Todas las llamadas a `insert()` posteriores
     *  afectarán a este marco hasta que se llame a `leave_scope()`.
     *
     *  @return La nueva profundidad de la pila.
     */
    size_t enter_scope()
    {
      // Reinitialize reused slots explicitly because DynArray::pop() moves the
      // last frame out before shrinking, and a later append may reuse that
      // moved-from storage.
      frames.append(Frame());
      return frames.size();
    }

    /** @brief Destruye el nivel de visibilidad actual y todas sus asociaciones.
     *
     *  Restaura la visibilidad del marco anterior (exterior).
     *
     *  @throws std::runtime_error Si se intenta llamar sin marcos activos.
     *  @note Use este método al encontrar el cierre de un bloque (ej. '}').
     */
    void leave_scope()
    {
      ah_runtime_error_unless(frames.size() > 0)
        << "Scope::leave_scope(): no active scope";
      (void) frames.pop();
    }

    /** @brief Registra una nueva asociación en el ámbito actual.
     *
     *  El nombre se añade al marco más interno. Si el nombre ya existe en
     *  un marco exterior, este método lo "sombreará" sin error. Si ya existe
     *  en el marco **actual**, la inserción fallará para evitar colisiones.
     *
     *  @param key Clave a registrar (ej. nombre de variable).
     *  @param value Valor a asociar (ej. metadatos del símbolo).
     *  @return `true` si la inserción fue exitosa, `false` si la clave ya existe
     *          en el nivel de visibilidad actual (redefinición prohibida).
     *  @throws std::runtime_error Si no hay ningún ámbito activo.
     */
    bool insert(const Key & key, const Value & value)
    {
      auto & frame = current_frame();
      if (find_value(frame.bindings, key) != nullptr)
        return false;
      frame.bindings.append({key, value});
      return true;
    }

    /** @brief Versión por movimiento de `insert()`.
     *
     *  Evita copias costosas de claves y valores.
     *  @see insert(const Key&, const Value&)
     */
    bool insert(Key && key, Value && value)
    {
      auto & frame = current_frame();
      if (find_value(frame.bindings, key) != nullptr)
        return false;
      frame.bindings.append({std::move(key), std::move(value)});
      return true;
    }

    /** @brief Busca una clave únicamente en el ámbito más interno (actual).
     *
     *  Útil para detectar redefiniciones en el mismo bloque de código sin
     *  verse afectado por lo que ocurra en ámbitos exteriores.
     *
     *  @param key Clave a buscar.
     *  @return Puntero al valor si se encuentra, `nullptr` en caso contrario.
     */
    Value * lookup_local(const Key & key) noexcept
    {
      if (frames.is_empty())
        return nullptr;
      return find_value(frames.top().bindings, key);
    }

    /** @brief Versión constante de `lookup_local()`. */
    const Value * lookup_local(const Key & key) const noexcept
    {
      if (frames.is_empty())
        return nullptr;
      return find_value(frames.top().bindings, key);
    }

    /** @brief Busca una clave recorriendo todos los ámbitos activos (de dentro afuera).
     *
     *  Implementa la búsqueda léxica estándar: busca primero en el marco
     *  actual, luego en el inmediatamente superior, y así hasta el global.
     *  Esto permite el "sombreado" (shadowing) de nombres.
     *
     *  @param key Clave a buscar.
     *  @return Puntero al valor de la asociación más cercana, o `nullptr`
     *          si la clave no está definida en ningún nivel accesible.
     */
    Value * lookup(const Key & key) noexcept
    {
      for (size_t i = frames.size(); i > 0; --i)
        if (auto * value = find_value(frames.access(i - 1).bindings, key);
            value != nullptr)
          return value;
      return nullptr;
    }

    /** @brief Versión constante de `lookup()`. */
    const Value * lookup(const Key & key) const noexcept
    {
      for (size_t i = frames.size(); i > 0; --i)
        if (const auto * value = find_value(frames.access(i - 1).bindings, key);
            value != nullptr)
          return value;
      return nullptr;
    }

  };
}

# endif // TPL_SCOPE_H
