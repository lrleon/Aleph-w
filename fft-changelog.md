# FFT Technical Changelog

## 2026-03-06

### Phase 0
- added stable benchmark JSON output and validation baseline
- added NumPy differential probe and regression tooling
- documented sanitizer and validation workflows

### Phase 1
- added arbitrary-length FFT dispatch via mixed-radix `2/3/4/5` and Bluestein
- extended `Plan` to arbitrary positive sizes
- added compact `rfft/irfft` APIs and batch equivalents

### Phase 2
- added SIMD dispatch policy (`auto|scalar|avx2|neon`)
- added reusable-plan batch FFT throughput paths
- hardened STFT/OverlapAddBank locality and dispatch heuristics

### Phase 3
- added flat-buffer axis/stride transforms
- added 2-D / 3-D transforms and batched 2-D paths
- added channel-major and frame-major multichannel STFT/ISTFT layouts

### Phase 4
- strengthened root solving with balanced monic scaling, Aberth-Ehrlich,
  fallback Durand-Kerner, Newton polishing, and residual checks
- refined SOS delays, gain margins, and stability diagnostics

### Phase 5
- added FIR Kaiser, `firwin`, `firls`, and dense-grid `remez`
- completed Butterworth, Chebyshev I/II, Bessel, and elliptic/Cauer SOS design

### Phase 6
- added window metrics and framing helpers
- added Welch PSD, cross-spectrum, and coherence estimation
- added `upfirdn` and `resample_poly`
- added `OverlapSave` and low-latency `PartitionedConvolver`

### Phase 7
- documented contracts, complexity, thread-safety, and support scope
- added release-audit automation via `scripts/fft_release_audit.rb`
- integrated benchmark/reference/audit checks into `ctest`
