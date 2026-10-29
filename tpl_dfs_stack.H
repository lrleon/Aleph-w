
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

# ifndef TPL_DFS_STACK_H
# define TPL_DFS_STACK_H

# include <algorithm>
# include <cstddef>
# include <memory>
# include <new>
# include <type_traits>
# include <utility>

namespace Aleph
{
  namespace dfs_stack_detail
  {
    // The largest room that a thread keeps
    inline constexpr size_t Kept_Bytes = 1024 * 1024;

    /// Room of a destroyed stack, kept for the next stack of the thread
    struct Room
    {
      void *base = nullptr;
      size_t bytes = 0;
      bool open = false; // from the first stack of the thread to its end
    };

    // Trivially destructible, so that a stack destroyed when the thread
    // ends, as a static one is, can still look at it
    inline Room &kept_room() noexcept
    {
      static thread_local constinit Room room;
      return room;
    }

    // A variable of each thread; its address in the thread that runs
    // main(), taken before main() starts, tells which thread that is
    inline thread_local constinit char thread_mark = 0;
    inline char * const main_thread_mark = &thread_mark;

    /// Opens the room of the thread, and frees it when the thread ends, but
    /// for main(): then the process ends, which frees it. Freeing a room of
    /// 64 KiB or more makes glibc merge the small free blocks, which at the
    /// end of the process are often many.
    struct Room_Keeper
    {
      Room_Keeper() noexcept { kept_room().open = true; }

      ~Room_Keeper()
      {
        Room &room = kept_room();
        room.open = false;
        if (&thread_mark == main_thread_mark)
          return; // the room stays reachable, so that it is not a leak

        ::operator delete(room.base);
        room = Room();
      }
    };

    // The room of the thread, which the first stack of the thread opens
    inline Room &open_room() noexcept
    {
      static thread_local Room_Keeper keeper;
      return kept_room();
    }
  } // namespace dfs_stack_detail

  /** @brief Stack of the frames of a depth-first search that does not
      recurse.

      The frames are contiguous and built in place, so that the top one is
      reached through a pointer, as a frame of the call stack is, and a
      frame is moved only when the stack grows. A reference to a frame
      stays valid until the stack grows.

      When the stack is destroyed, its room, up to 1 MiB, is kept for the
      next stack of the thread, so that a search that follows allocates
      nothing; a stack that never needs room costs nothing. An allocation of
      1 KiB or more also makes glibc merge its small free blocks, which then
      slows down those of the graph algorithms: the first room is smaller,
      so that a search that does not go deep does not cause it; the second
      has 256 frames.

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
    static constexpr size_t Initial_Capacity =
      sizeof(T) < 1000 ? 1000 / sizeof(T) : 1;

    // The frames of the second room, at least: a search that outgrows the
    // first one goes deep, and then grows by four from 256 frames
    static constexpr size_t Second_Capacity = 256;

    void *room = nullptr; // where the frames are
    size_t room_bytes = 0;
    T *base = nullptr;    // the first frame
    T *top_ptr = nullptr; // one past the top frame
    T *limit = nullptr;   // one past the last frame that fits in the room

    void set_room(void *r, size_t bytes) noexcept
    {
      room = r;
      room_bytes = bytes;
      base = top_ptr = static_cast<T *>(r);
      limit = base + bytes / sizeof(T);
    }

    // Gives the stack room for more frames: that which the thread kept, if
    // the stack has none, or else a room four times as large, and of
    // Second_Capacity frames at least, so that a search that goes deep
    // moves a third of the frames that doubling would, to which it moves
    // the frames. If it throws, the new room goes back and the stack is as
    // it was, unless a frame that cannot be copied threw while it was
    // moved: those moved before it are left moved from. Out of line, so
    // that push() stays small.
    [[gnu::noinline]] void grow()
    {
      if (room == nullptr)
        {
          dfs_stack_detail::Room &kept = dfs_stack_detail::open_room();
          if (kept.bytes >= sizeof(T))
            {
              set_room(kept.base, kept.bytes);
              kept.base = nullptr;
              kept.bytes = 0;
              return;
            }
        }

      const size_t n = size();
      const size_t frames =
        n == 0 ? Initial_Capacity : std::max(4 * n, Second_Capacity);
      const size_t new_bytes = frames * sizeof(T);
      void *new_room = ::operator new(new_bytes);
      T *new_base = static_cast<T *>(new_room);
      if constexpr (std::is_nothrow_move_constructible_v<T>)
        std::uninitialized_move(base, top_ptr, new_base);
      else
        {
          try
            {
              if constexpr (std::is_copy_constructible_v<T>)
                std::uninitialized_copy(base, top_ptr, new_base);
              else // the frames that were moved are left moved from
                std::uninitialized_move(base, top_ptr, new_base);
            }
          catch (...)
            {
              ::operator delete(new_room);
              throw;
            }
        }
      std::destroy(base, top_ptr);
      ::operator delete(room);
      set_room(new_room, new_bytes);
      top_ptr = new_base + n;
    }

  public:
    /// Builds an empty stack, which takes the room that the thread kept
    /// when it first needs room
    Dfs_Stack() noexcept = default;

    /// Takes the frames and the room of other, which is left empty
    Dfs_Stack(Dfs_Stack &&other) noexcept
      : room(other.room), room_bytes(other.room_bytes), base(other.base),
        top_ptr(other.top_ptr), limit(other.limit)
    {
      other.set_room(nullptr, 0);
    }

    /// Takes the frames and the room of other, which is left with the
    /// room of this stack and no frames
    Dfs_Stack &operator=(Dfs_Stack &&other) noexcept
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
      dfs_stack_detail::Room &kept = dfs_stack_detail::kept_room();
      if (kept.open and room_bytes <= dfs_stack_detail::Kept_Bytes
          and room_bytes > kept.bytes)
        {
          ::operator delete(kept.base);
          kept.base = room;
          kept.bytes = room_bytes;
        }
      else
        ::operator delete(room);
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
    [[gnu::always_inline]] T &push(Args &&... args)
    {
      if (top_ptr == limit) [[unlikely]]
        grow();
      T *frame = ::new (static_cast<void *>(top_ptr)) T(std::forward<Args>(args)...);
      ++top_ptr;
      return *frame;
    }

    /// Destroys the top frame; the stack must not be empty
    void pop() noexcept
    {
      --top_ptr;
      std::destroy_at(top_ptr);
    }

    /// The top frame; the stack must not be empty
    [[nodiscard]] T &top() noexcept { return top_ptr[-1]; }

    /// Whether the stack has no frames
    [[nodiscard]] bool is_empty() const noexcept { return top_ptr == base; }

    /// The number of frames
    [[nodiscard]] size_t size() const noexcept { return size_t(top_ptr - base); }

    /// Destroys all the frames, and keeps the room
    void clear() noexcept
    {
      std::destroy(base, top_ptr);
      top_ptr = base;
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

    It it; ///< The arcs of the node, on the next one to look at

    /// Builds the iterator with args; inlined, with all it calls
    template <class... Args>
    [[gnu::always_inline, gnu::flatten]]
    explicit Dfs_Arc_Frame(Args &&... args) : it(std::forward<Args>(args)...) {}

    /// Moves to the next arc; inlined, with all it calls
    [[gnu::always_inline, gnu::flatten]] void next_arc() noexcept { it.next_ne(); }

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
    [[gnu::always_inline, gnu::flatten]]
    Node * next_target(Node * goal = nullptr) noexcept
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

  /** @brief The current item of an iterator that has one.

      A search that knows that `it` has a current item asks for it with
      get_curr_ne(), which does not check that there is one, if `it` has
      it, or else with get_curr().

      @param it Iterator positioned on an item.
      @return The current item of `it`.
      @ingroup Graphs
  */
  template <class It>
  [[gnu::always_inline]] inline decltype(auto) dfs_curr(const It & it)
  {
    if constexpr (requires { it.get_curr_ne(); })
      return it.get_curr_ne();
    else
      return it.get_curr();
  }
} // namespace Aleph

# endif // TPL_DFS_STACK_H
