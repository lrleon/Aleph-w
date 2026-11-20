/** @file adaptive_huffman_example.cc
 *  @brief Three guided demonstrations of adaptive Huffman coding.
 *
 *  Build: cmake --build build --target adaptive_huffman_example
 *  Run:   ./build/Examples/adaptive_huffman_example [text]
 *
 *  Demonstrates exact transmitted bits and tree snapshots, adaptation to
 *  changing frequencies, and binary data received in arbitrarily cut chunks.
 *  All demonstrations verify their decoded output. Display trees use only
 *  public encoder operations.
 */

#include <Adaptive_Huffman.H>
#include <ah-errors.H>
#include "adaptive_huffman_demo_support.H"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
  /** @brief Print a numbered demonstration heading.
   *  @param number Section number.
   *  @param title Human-readable topic.
   */
  void heading(const int number, const std::string_view title)
  {
    std::cout << '\n' << std::string(76, '=') << '\n'
              << "  " << number << " / " << title << '\n'
              << std::string(76, '=') << "\n\n";
  }

  /** @brief Show the actual bits and evolving tree for a short message.
   *  @param input Message to trace, limited to 32 bytes for readable output.
   */
  void demonstrate_trace(const std::string_view input)
  {
    using namespace adaptive_huffman_demo;
    heading(1, "THE CODE LEARNS ONE BYTE AT A TIME");
    std::cout << "  Left edge = 0. Right edge = 1. NYT = not yet transmitted.\n"
              << "  A new byte sends the NYT path followed by its 8 literal bits.\n"
              << "  A repeated byte sends only its current tree path.\n\n";
    const auto frames = trace(input);
    std::cout << "  " << std::left << std::setw(5) << "Step" << std::setw(8) << "Byte"
              << std::setw(11) << "Status" << std::setw(15) << "Tree path"
              << std::setw(12) << "Literal" << std::right << std::setw(6) << "Bits"
              << std::setw(9) << "Total" << '\n'
              << "  " << std::string(66, '-') << '\n';
    for (size_t i = 1; i < frames.size(); ++i)
      {
        const auto &frame = frames[i];
        std::cout << "  " << std::left << std::setw(5) << i
                  << std::setw(8) << byte_label(frame.byte)
                  << std::setw(11) << (frame.first_appearance ? "new / NYT" : "known")
                  << std::setw(15) << (frame.path.empty() ? "(root)" : frame.path)
                  << std::setw(12) << (frame.literal.empty() ? "-" : frame.literal)
                  << std::right << std::setw(6) << frame.path.size() + frame.literal.size()
                  << std::setw(9) << frame.total_bits << '\n';
      }
    std::cout << "\n  Initial tree\n";
    print_tree(std::cout, frames[0].tree);
    if (input.size() > 3)
      {
        std::cout << "\n  After byte 3 (weights are frequencies)\n";
        print_tree(std::cout, frames[3].tree);
      }
    const auto &last = frames[frames.size() - 1];
    std::cout << "\n  Final tree\n";
    print_tree(std::cout, last.tree);
    std::cout << "\n  Original: " << input.size() * 8 << " bits   Encoded: "
              << last.total_bits << " bits\n"
              << "  Every byte was decoded exactly on its last bit.\n"
              << "  The displayed trees are AFTER each update; transmitted paths\n"
              << "  in the table come from the tree BEFORE that update.\n";
  }

  /// @brief Show how one continuous model follows a changing distribution.
  void demonstrate_adaptation()
  {
    using adaptive_huffman_demo::next_code;
    heading(2, "WHEN THE FREQUENT BYTE CHANGES");
    std::cout << "  One continuous stream, with no reset between phases.\n"
              << "  Bar scale: one # represents about 16 transmitted bits.\n\n";
    Aleph::Adaptive_Huffman_Encoder encoder;
    Aleph::Adaptive_Huffman_Decoder decoder;
    std::string expected, decoded;
    const std::string phases[] = {std::string(64, 'A'), std::string(128, 'B'),
                                  std::string(256, 'A')};
    for (size_t i = 0; i < 3; ++i)
      {
        const auto &phase = phases[i];
        const auto byte = static_cast<unsigned char>(phase[0]);
        const auto first_cost = next_code(encoder, byte).size();
        Aleph::BitArray bits;
        encoder.encode(phase, bits);
        decoder.decode(bits, decoded);
        decoder.finish();
        expected += phase;
        ah_logic_error_if(decoded != expected) << "Distribution-change round trip failed";
        const auto code = next_code(encoder, byte);
        std::cout << "  Phase " << i + 1 << " : " << phase.size() << " x " << phase[0]
                  << "\n    Raw " << std::setw(4) << phase.size() * 8
                  << " bits -> " << std::setw(4) << bits.size() << " bits  "
                  << std::string((bits.size() + 15) / 16, '#')
                  << "\n    First byte: " << first_cost << " bits; next " << phase[0]
                  << " now: " << code << " (" << code.size() << " bit)"
                  << "\n    Cumulative frequencies: A=" << encoder.frequency('A')
                  << ", B=" << encoder.frequency('B') << "\n\n";
      }
  }

  /// @brief Recover NUL, high bytes and text from deliberately unaligned chunks.
  void demonstrate_fragmentation()
  {
    heading(3, "BINARY BYTES, ARBITRARY BIT CHUNKS");
    const std::string input("\0\xff" "A\0\n\xff\0", 7);
    Aleph::Adaptive_Huffman_Encoder encoder;
    Aleph::Adaptive_Huffman_Decoder decoder;
    Aleph::BitArray bits;
    encoder.encode(input, bits);
    std::cout << "  Payload: NUL 0xFF A NUL LF 0xFF NUL\n"
              << "  Chunk widths repeat 3, 1, 7, 2, 5 bits; incomplete symbols wait.\n\n"
              << "  " << std::left << std::setw(8) << "Chunk" << std::setw(11) << "Bits"
              << std::setw(12) << "New bytes" << "Pending symbol\n"
              << "  " << std::string(48, '-') << '\n';
    constexpr size_t widths[] = {3, 1, 7, 2, 5};
    std::string output;
    size_t chunk_number = 0;
    for (size_t offset = 0; offset < bits.size(); ++chunk_number)
      {
        Aleph::BitArray chunk;
        const size_t count = std::min(widths[chunk_number % 5], bits.size() - offset);
        for (size_t i = 0; i < count; ++i)
          chunk.push(bits[offset + i]);
        offset += count;
        const size_t emitted = decoder.decode(chunk, output);
        std::cout << "  " << std::setw(8) << chunk_number + 1
                  << std::setw(11) << chunk.get_bit_str() << std::setw(12) << emitted
                  << (decoder.has_pending_symbol() ? "yes" : "no") << '\n';
      }
    decoder.finish();
    ah_logic_error_if(output != input) << "Fragmented binary round trip failed";
    std::cout << "\n  All 7 binary bytes recovered exactly.\n"
              << "  Keep the exact bit count when storing or transmitting the stream.\n";
  }
}

/** @brief Run the guided, self-checking adaptive Huffman demonstrations.
 *  @param argc Number of command-line arguments.
 *  @param argv Optional short trace text in argv[1]; --help prints usage.
 *  @return Zero on success; one if arguments or verification fail.
 */
int main(int argc, char *argv[])
{
  try
    {
      if (argc > 1 and std::string_view(argv[1]) == "--help")
        {
          std::cout << "Usage: adaptive_huffman_example [text of at most 32 bytes]\n"
                    << "Runs a symbol trace, changing-distribution phases, and binary chunks.\n";
          return 0;
        }
      const std::string_view input = argc > 1 ? argv[1] : "ABRACADABRA";
      ah_invalid_argument_if(argc > 2 or input.size() > 32)
        << "Use adaptive_huffman_example [text of at most 32 bytes]";
      std::cout << "\n  ALEPH / ADAPTIVE HUFFMAN\n"
                << "  A guided FGK coding demonstration\n";
      demonstrate_trace(input);
      demonstrate_adaptation();
      demonstrate_fragmentation();
      std::cout << "\n  Round trip verified in all three demonstrations.\n\n";
      return 0;
    }
  catch (const std::exception &error)
    {
      std::cerr << error.what() << '\n';
      return 1;
    }
}
