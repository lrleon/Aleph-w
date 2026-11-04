
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

/** @file tpl_dfs_stack.H
 *  @brief Stack of the frames of a depth-first search that does not recurse.
 *
 *  A depth-first search can go as deep as the graph has nodes, which
 *  overflows the call stack if it recurses. The searches that do not
 *  recurse keep, in a Dfs_Stack, what recursion would keep in each frame:
 *  the node being visited and the iterator over its arcs.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef TPL_DFS_STACK_H
#define TPL_DFS_STACK_H

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

#if defined(__GLIBC__)
#include <unistd.h>
#endif

/** @def ALEPH_FLATTEN
    @brief `[[gnu::flatten]]`, or nothing where it would only make the
    compilation slower.

    The depth-first searches put it, with `[[gnu::always_inline]]`, on the
    functions that they call for each node or arc, so that the compiler
    inlines into them all that they call: GCC would leave out of line the
    iterators and the stack, which then cost more than a recursion. It
    makes the compilation take more time and memory, for nothing in a
    build that does not inline or that has sanitizers, so that it is empty:

    - with `ALEPH_NO_FLATTEN` defined, as the build defines it with
      `ALEPH_USE_SANITIZERS`, for UndefinedBehaviorSanitizer, which defines
      no macro;
    - under AddressSanitizer or ThreadSanitizer;
    - with `__NO_INLINE__`, as with `-O0`.

    @ingroup Graphs
*/
#if defined(ALEPH_NO_FLATTEN) || defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__) ||  \
  defined(__NO_INLINE__)
#define ALEPH_FLATTEN
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define ALEPH_FLATTEN
#else
#define ALEPH_FLATTEN [[gnu::flatten]]
#endif
#else
#define ALEPH_FLATTEN [[gnu::flatten]]
#endif

// Under AddressSanitizer with ALEPH_ANNOTATE_CONTAINERS, which every
// translation unit of the program must agree on, as for MemArray, the part
// of the room of a stack that holds no frame cannot be reached
#if defined(ALEPH_ANNOTATE_CONTAINERS)
#if defined(__SANITIZE_ADDRESS__)
#define ALEPH_DFS_STACK_POISON 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ALEPH_DFS_STACK_POISON 1
#endif
#endif
#endif
#ifdef ALEPH_DFS_STACK_POISON
#include <sanitizer/asan_interface.h>
#endif

namespace Aleph {
namespace dfs_stack_detail {
// The largest room that a thread keeps
inline constexpr size_t Kept_Bytes = 1024 * 1024;

// Whether the part of a room that holds no frame is poisoned
#ifdef ALEPH_DFS_STACK_POISON
inline constexpr bool Poisoned = true;
#else
inline constexpr bool Poisoned = false;
#endif

// Under ALEPH_DFS_STACK_POISON, makes the bytes from p unreachable, so
// that AddressSanitizer reports the use of a frame that was popped, or
// of the room of a stack that is gone, which its thread keeps
inline void poison([[maybe_unused]] const void *p, [[maybe_unused]] size_t bytes) noexcept
{
#ifdef ALEPH_DFS_STACK_POISON
  ASAN_POISON_MEMORY_REGION(p, bytes);
#endif
}

// Makes the bytes from p reachable again
inline void unpoison([[maybe_unused]] const void *p, [[maybe_unused]] size_t bytes) noexcept
{
#ifdef ALEPH_DFS_STACK_POISON
  ASAN_UNPOISON_MEMORY_REGION(p, bytes);
#endif
}

// Frees a room, which may be poisoned, of the given bytes, or nothing
inline void free_room(void *r, [[maybe_unused]] size_t bytes) noexcept
{
  if constexpr (Poisoned)
    if (r != nullptr)
      unpoison(r, bytes);
  ::operator delete (r);
}

/// Room of a destroyed stack, kept for the next stack of the thread
struct Room
{
  void *base = nullptr;
  size_t bytes = 0;
  bool open = false;  // from the first stack of the thread to its end
};

// Trivially destructible, so that a stack destroyed when the thread
// ends, as a static one is, can still look at it
inline Room &kept_room() noexcept
{
  static thread_local constinit Room room;
  return room;
}

// Whether this thread is the one that runs main(): with glibc, the one
// whose id is that of the process. It is asked, and not taken from a
// mark before main() starts, which a library that another thread loads
// with dlopen() would take in that thread, and which costs an
// initializer in each unit. Elsewhere, no thread is taken for it.
inline bool in_main_thread() noexcept
{
#if defined(__GLIBC__)
#if __GLIBC_PREREQ(2, 30)
  return ::gettid() == ::getpid();
#else
  return false;
#endif
#else
  return false;
#endif
}

/// Opens the room of the thread, and frees it when the thread ends, but
/// for the thread of main() with glibc: then the process ends, which
/// frees it, and freeing a room of 64 KiB or more would make glibc merge
/// the small free blocks, which at the end of the process are often
/// many.
struct Room_Keeper
{
  Room_Keeper() noexcept
  {
    kept_room().open = true;
  }

  ~Room_Keeper()
  {
    Room &room = kept_room();
    room.open = false;
    if (in_main_thread())
      return;  // the room stays reachable, so that it is not a leak

    free_room(room.base, room.bytes);
    room = Room();
  }
};

// The room of the thread, which the first stack of the thread opens
inline Room &open_room() noexcept
{
  static thread_local Room_Keeper keeper;
  return kept_room();
}
}  // namespace dfs_stack_detail

/** @brief Stack of the frames of a depth-first search that does not
    recurse.

    The frames are contiguous and built in place, so that the top one is
    reached through a pointer, as a frame of the call stack is, and a
    frame is moved only when the stack grows. A reference to a frame
    stays valid until the stack grows.

    When the stack is destroyed or released, its room, up to 1 MiB, is
    kept for the next stack of the thread, so that a search that follows
    allocates nothing; the room that it took from the thread goes back to
    the thread when the stack outgrows it, so that a search that goes
    beyond 1 MiB does not leave the thread without room. A stack that
    never needs room costs nothing. An allocation of 1 KiB or more also
    makes glibc merge its small free blocks, which then slows down those
    of the graph algorithms: the first room is smaller, so that a search
    that does not go deep does not cause it; the second has 256 frames.

    Under AddressSanitizer, with ALEPH_ANNOTATE_CONTAINERS defined for the
    whole program, as MemArray asks, the part of the room that holds no
    frame cannot be reached: the use of a frame that was popped, or of the
    room of a stack that is gone, is reported.

    @tparam T Type of the frames.
    @ingroup Graphs
*/
template <class T>
class Dfs_Stack
{
  static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__,
                "the frames need more alignment than operator new gives");

  // The frames of the first room: as many as fit in 1000 bytes, which
  // glibc gives without merging its free blocks, or one
  static constexpr size_t Initial_Capacity = sizeof(T) < 1000 ? 1000 / sizeof(T) : 1;

  // The frames of the second room, at least: a search that outgrows the
  // first one goes deep, and then grows by four from 256 frames
  static constexpr size_t Second_Capacity = 256;

  void *room = nullptr;  // where the frames are
  size_t room_bytes = 0;
  T *base = nullptr;     // the first frame
  T *top_ptr = nullptr;  // one past the top frame
  T *limit = nullptr;    // one past the last frame that fits in the room

  void set_room(void *r, size_t bytes) noexcept
  {
    room = r;
    room_bytes = bytes;
    base = top_ptr = static_cast<T *>(r);
    limit = base + bytes / sizeof(T);
  }

  // Gives the room, whose frames are gone, to the thread, which keeps it,
  // poisoned, in the place of the one that it kept, if it fits in
  // Kept_Bytes and is larger; else frees it. The room is read after the
  // room of the thread, which code built for a shared library gets
  // through a call: else it would be held across that call, in registers
  // that the loop of the search would then lack.
  void give_room_back() noexcept
  {
    dfs_stack_detail::Room &kept = dfs_stack_detail::kept_room();
    if (kept.open and room_bytes <= dfs_stack_detail::Kept_Bytes and room_bytes > kept.bytes)
      {
        dfs_stack_detail::free_room(kept.base, kept.bytes);
        dfs_stack_detail::poison(room, room_bytes);
        kept.base = room;
        kept.bytes = room_bytes;
      }
    else
      dfs_stack_detail::free_room(room, room_bytes);
  }

  // Gives the stack room for more frames: that which the thread kept, if
  // the stack has none, or else, through enlarge(), a larger one. Out of
  // line, so that push() stays small, and apart from enlarge(), so that
  // the search that takes the room of the thread, as most do, does not
  // pay the registers that enlarge() saves.
  [[gnu::noinline]] void grow()
  {
    if (room == nullptr)
      {
        dfs_stack_detail::Room &kept = dfs_stack_detail::open_room();
        if (kept.bytes >= sizeof(T))
          {
            set_room(kept.base, kept.bytes);  // poisoned, as kept
            kept.base = nullptr;
            kept.bytes = 0;
            return;
          }
      }
    enlarge();
  }

  // Moves the frames to a room four times as large, and of
  // Second_Capacity frames at least, so that a search that goes deep
  // moves a third of the frames that doubling would. The room that it
  // leaves goes to the thread if the thread keeps none, as when the stack
  // took it from the thread, so that a search that outgrows it does not
  // leave the thread without room; else it is freed at once, which leaves
  // no free block in the middle of the heap for the small allocations
  // that follow to be carved from. A member that told which room was the
  // thread's would cost up to 12 % with GCC, which would no longer hold
  // the stack in registers in the loop of a search. If it throws, the new
  // room goes back and the stack is as it was, unless a frame that cannot
  // be copied threw while it was moved: those moved before it are left
  // moved from.
  [[gnu::noinline]] void enlarge()
  {
    const size_t n = size();
    const size_t frames = n == 0 ? Initial_Capacity : std::max(4 * n, Second_Capacity);
    const size_t new_bytes = frames * sizeof(T);
    void *new_room = ::operator new (new_bytes);
    T *new_base = static_cast<T *>(new_room);
    dfs_stack_detail::poison(new_base + n, new_bytes - n * sizeof(T));
    if constexpr (std::is_nothrow_move_constructible_v<T>)
      std::uninitialized_move(base, top_ptr, new_base);
    else
      {
        try
          {
            // from const frames, which a constructor that forwards
            // what it takes cannot take for itself
            if constexpr (std::is_copy_constructible_v<T>)
              std::uninitialized_copy(static_cast<const T *>(base), static_cast<const T *>(top_ptr),
                                      new_base);
            else  // the frames that were moved are left moved from
              std::uninitialized_move(base, top_ptr, new_base);
          }
        catch (...)
          {
            dfs_stack_detail::free_room(new_room, new_bytes);
            throw;
          }
      }
    std::destroy(base, top_ptr);
    if (dfs_stack_detail::kept_room().bytes == 0)
      give_room_back();
    else
      dfs_stack_detail::free_room(room, room_bytes);
    set_room(new_room, new_bytes);
    top_ptr = new_base + n;
  }

  // push() under ALEPH_DFS_STACK_POISON, once the stack has room: the
  // frame becomes reachable, and unreachable again if building it throws
  template <class... Args>
  [[gnu::always_inline]] T &push_in_poisoned_room(Args &&...args)
  {
    void *place = top_ptr;
    dfs_stack_detail::unpoison(place, sizeof(T));
    T *frame;
    if constexpr (std::is_nothrow_constructible_v<T, Args &&...>)
      frame = ::new (place) T(std::forward<Args>(args)...);
    else
      try
        {
          frame = ::new (place) T(std::forward<Args>(args)...);
        }
      catch (...)
        {
          dfs_stack_detail::poison(place, sizeof(T));
          throw;
        }
    ++top_ptr;
    return *frame;
  }

public:
  /// Builds an empty stack, which takes the room that the thread kept
  /// when it first needs room
  Dfs_Stack() noexcept = default;

  /// Takes the frames and the room of other, which is left empty
  Dfs_Stack(Dfs_Stack &&other) noexcept
    : room(other.room), room_bytes(other.room_bytes), base(other.base), top_ptr(other.top_ptr),
      limit(other.limit)
  {
    other.set_room(nullptr, 0);
  }

  /// Takes the frames and the room of other, which is left with the
  /// room of this stack and no frames
  Dfs_Stack &operator = (Dfs_Stack &&other) noexcept
  {
    clear();
    std::swap(room, other.room);
    std::swap(room_bytes, other.room_bytes);
    std::swap(base, other.base);
    std::swap(top_ptr, other.top_ptr);
    std::swap(limit, other.limit);
    return *this;
  }

  /// Destroys the frames; the thread keeps the room, if it fits in
  /// 1 MiB and is larger than the one kept
  ~Dfs_Stack()
  {
    if (room == nullptr)
      return;

    clear();
    give_room_back();
  }

  /** @brief Builds a frame on top of the stack.

      @param args Arguments for the constructor of the frame.
      @return Reference to the new frame.
      @throw bad_alloc if there is not enough memory; anything that the
      constructor of the frame throws, or, when the stack grows, the copy
      or the move of a frame. Then the stack is as it was, but for the
      frames that cannot be copied that the stack moved before one threw.
      @note Always inlined, since a search pushes a frame for each node.
  */
  template <class... Args>
  [[gnu::always_inline]] T &push(Args &&...args)
  {
    if (top_ptr == limit) [[unlikely]]
      grow();
    if constexpr (dfs_stack_detail::Poisoned)
      return push_in_poisoned_room(std::forward<Args>(args)...);
    else
      {
        T *frame = ::new (static_cast<void *>(top_ptr)) T(std::forward<Args>(args)...);
        ++top_ptr;
        return *frame;
      }
  }

  /// Destroys the top frame; the stack must not be empty
  void pop() noexcept
  {
    --top_ptr;
    std::destroy_at(top_ptr);
    if constexpr (dfs_stack_detail::Poisoned)
      dfs_stack_detail::poison(top_ptr, sizeof(T));
  }

  /// The top frame; the stack must not be empty
  [[nodiscard]] T &top() noexcept
  {
    return top_ptr[-1];
  }

  /// Whether the stack has no frames
  [[nodiscard]] bool is_empty() const noexcept
  {
    return top_ptr == base;
  }

  /// The number of frames
  [[nodiscard]] size_t size() const noexcept
  {
    return size_t(top_ptr - base);
  }

  /// The number of frames that fit in the room of the stack, 0 while it
  /// has none
  [[nodiscard]] size_t capacity() const noexcept
  {
    return size_t(limit - base);
  }

  /// Destroys all the frames, and keeps the room
  void clear() noexcept
  {
    std::destroy(base, top_ptr);
    if constexpr (dfs_stack_detail::Poisoned)
      dfs_stack_detail::poison(base, size() * sizeof(T));
    top_ptr = base;
  }

  /** @brief Destroys all the frames, and gives the room back.

      The thread keeps the room, as when the stack is destroyed, if it
      fits in 1 MiB and is larger than the one kept; else it is freed.
      The stack is left without room, which it takes again, from the
      thread if it can, when a frame is pushed.
  */
  void release() noexcept
  {
    if (room == nullptr)
      return;

    clear();
    give_room_back();
    set_room(nullptr, 0);
  }
};

/** @brief The next arc of a node to look at, for a depth-first search
    that keeps only that in a frame.

    The search keeps the frame of the node that it visits in a local
    variable until that node visits another; then the frame waits in a
    Dfs_Stack until the search comes back to it.

    @tparam It Iterator over the arcs of a node, with has_curr(),
    next_ne(), get_current_arc_ne() and get_tgt_node_ne().
    @ingroup Graphs
*/
template <class It>
struct Dfs_Arc_Frame
{
  /// The type of the nodes of the graph
  using Node = std::remove_pointer_t<decltype(std::declval<It &>().get_tgt_node_ne())>;

  It it;  ///< The arcs of the node, on the next one to look at

  /// Builds the iterator with args, if they build one, so that a frame
  /// is copied by the copy constructor; inlined, with all it calls
  template <class... Args>
    requires std::constructible_from<It, Args &&...>
  [[gnu::always_inline]] ALEPH_FLATTEN explicit Dfs_Arc_Frame(Args &&...args)
    : it(std::forward<Args>(args)...)
  {}

  /// Moves to the next arc; inlined, with all it calls
  [[gnu::always_inline]] ALEPH_FLATTEN void next_arc() noexcept(noexcept(it.next_ne()))
  {
    it.next_ne();
  }

  /** @brief Moves the iterator to the next arc to follow, and gives its
      target.

      The arc to follow is the next one without the control bit `Bit`
      whose target is `goal`, or has not `Bit` either, or has it, if
      `Marked_Too`. The arcs without `Bit` that it passes, and the one
      that it gives, get `Bit`.

      @param goal A node to give even if it has `Bit`.
      @return The target of the arc where the iterator stops, or nullptr
      when the arcs are done.
  */
  template <int Bit, bool Marked_Too = false>
  [[gnu::always_inline]] ALEPH_FLATTEN Node *next_target(Node *goal = nullptr) noexcept(
    noexcept(it.next_ne()))
  {
    // the control bits, as the macros of aleph-graph.H give them: this
    // header does not include it, so that the trees can take the stack
    for (; it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        if (arc->attrs.control_bits.get_bit(Bit))
          continue;

        // the target before the mark, which the compiler takes for a
        // store that could change the iterator
        auto tgt = it.get_tgt_node_ne();
        arc->attrs.control_bits.set_bit(Bit, true);
        if (Marked_Too or tgt == goal or not tgt->attrs.control_bits.get_bit(Bit))
          return tgt;
      }
    return nullptr;
  }
};

/** @brief The variable that holds the frame of the node that a
    depth-first search visits.

    A search that does not recurse keeps the frame of the node that it
    visits in a local variable until that node visits another; then the
    frame waits in a Dfs_Stack, and the variable takes the frame of the
    next node. A frame that keeps an arc filter, in its iterator, can be
    assigned only if the filter can, which ArcFilter does not ask: a
    lambda that captures, or a filter with a reference or a const member,
    cannot. So the variable is the frame itself, which dfs_replace()
    assigns, as the searches did, if the frames can be assigned, and else
    an optional frame, which dfs_replace() builds again in its place.
    dfs_here() builds it, and dfs_frame() gives the frame that it holds.

    @tparam T Type of the frames.
    @ingroup Graphs
*/
template <class T>
using Dfs_Here = std::conditional_t<std::is_move_assignable_v<T>, T, std::optional<T>>;

/** @brief Builds the variable of the frame of a search, with the frame
    built with args.

    @tparam T Type of the frames.
    @param args Arguments for the constructor of the frame.
    @return A Dfs_Here<T> with the frame.
    @ingroup Graphs
*/
template <class T, class... Args>
[[gnu::always_inline]] inline Dfs_Here<T> dfs_here(Args &&...args)
{
  if constexpr (std::is_move_assignable_v<T>)
    return T(std::forward<Args>(args)...);
  else
    return Dfs_Here<T>(std::in_place, std::forward<Args>(args)...);
}

/** @brief The frame that a Dfs_Here holds.

    @param here The frame, if frames can be assigned.
    @return here.
    @ingroup Graphs
*/
template <class T>
[[gnu::always_inline]] inline T &dfs_frame(T &here) noexcept
{
  return here;
}

/** @brief The frame that a Dfs_Here holds.

    @param here The optional frame, if frames cannot be assigned; it
    must hold one.
    @return The frame that here holds.
    @ingroup Graphs
*/
template <class T>
[[gnu::always_inline]] inline T &dfs_frame(std::optional<T> &here) noexcept
{
  return *here;
}

/** @brief Replaces the frame that a Dfs_Here holds with one built with
    args, by assignment.

    @param here The frame, if frames can be assigned.
    @param args Arguments for the constructor of the frame.
    @throw anything that the constructor or the assignment of the frame
    throws; then here keeps its frame.
    @ingroup Graphs
*/
template <class T, class... Args>
[[gnu::always_inline]] inline void dfs_replace(T &here, Args &&...args)
{
  here = T(std::forward<Args>(args)...);
}

/** @brief Replaces the frame that a Dfs_Here holds with one built in its
    place with args.

    @param here The optional frame, if frames cannot be assigned.
    @param args Arguments for the constructor of the frame.
    @throw anything that the constructor of the frame throws; then here
    holds no frame.
    @ingroup Graphs
*/
template <class T, class... Args>
[[gnu::always_inline]] inline void dfs_replace(std::optional<T> &here, Args &&...args)
{
  here.emplace(std::forward<Args>(args)...);
}

/** @brief The current item of an iterator that has one.

    A search that knows that `it` has a current item asks for it with
    get_curr_ne(), which does not check that there is one, if `it` has
    it, or else with get_curr().

    @param it Iterator positioned on an item.
    @return The current item of `it`.
    @ingroup Graphs
*/
template <class It>
[[gnu::always_inline]] inline decltype(auto) dfs_curr(const It &it)
{
  if constexpr (requires { it.get_curr_ne(); })
    return it.get_curr_ne();
  else
    return it.get_curr();
}
}  // namespace Aleph

#endif  // TPL_DFS_STACK_H
