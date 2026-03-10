# Tutorial: FFT y Procesamiento Digital de Senales en Aleph-w

Idioma: Español | [English](fft-tutorial.en.md)

## 1. Fundamentos conceptuales

### 1.1 Que es la Transformada de Fourier Discreta (DFT)?

La DFT descompone una senal discreta de N muestras en N componentes
de frecuencia. Dado un vector x[0..N-1], la DFT produce X[0..N-1] donde:

    X[k] = sum_{t=0}^{N-1} x[t] * exp(-2*pi*i*k*t / N)

Cada X[k] es un numero complejo cuya **magnitud** indica la amplitud de
la frecuencia k y cuya **fase** indica el desfase temporal.

La DFT directa tiene complejidad O(N^2). La **Fast Fourier Transform (FFT)**
calcula la misma operacion en O(N log N) usando la factorizacion recursiva
de Cooley-Tukey.

### 1.2 Propiedades clave

- **Linealidad**: FFT(a*x + b*y) = a*FFT(x) + b*FFT(y)
- **Simetria hermitiana**: Si x es real, X[k] = conj(X[N-k])
- **Teorema de convolucion**: x * y en tiempo = X . Y en frecuencia
- **Parseval**: La energia se conserva entre dominios

### 1.3 Donde se usa la FFT?

| Dominio | Aplicacion |
|---------|-----------|
| Audio | Analisis espectral, ecualizadores, compresion |
| Imagen | Filtrado, compresion JPEG |
| Telecomunicaciones | OFDM, estimacion de canal |
| Ciencia | Procesamiento de senales sismicas, astronomia |
| Matematicas | Multiplicacion rapida de polinomios, numeros grandes |

---

## 2. Arquitectura de `fft.H`

### 2.1 Clase principal

```cpp
# include <fft.H>

// Clase template parametrizada por precision
template <std::floating_point Real = double>
class FFT { ... };

// Aliases convenientes
using FFTD = FFT<double>;   // Precision doble (lo mas comun)
using FFTF = FFT<float>;    // Precision simple
```

Todos los metodos son `static` -- no se necesita instanciar la clase.

### 2.2 Tipos internos importantes

```cpp
using Complex = std::complex<Real>;

// Seccion biquad para filtros IIR
struct BiquadSection { Real b0, b1, b2, a0, a1, a2; };

// Respuesta en frecuencia
struct FrequencyResponse {
  Array<Real>    omega;      // Frecuencias angulares
  Array<Complex> response;   // H(omega) complejo
};

// Resultado del analisis STFT
// Array<Array<Complex>>  -- cada fila es un frame espectral
```

### 2.3 Estrategia de implementacion

La FFT de Aleph-w usa tres estrategias segun el tamano N:

1. **Potencia de 2** (N = 2^k): Cooley-Tukey radix-2/4 optimizado, con
   despacho opcional AVX2/NEON
2. **Tamano arbitrario**: Algoritmo de Bluestein (chirp-z), que reduce
   cualquier N a una convolucion de tamano potencia de 2
3. **Plan reutilizable**: Precomputa bit-reversal y twiddle factors para
   transformadas repetidas del mismo tamano

---

## 3. Uso basico

### 3.1 Transformada directa e inversa

```cpp
# include <fft.H>
# include <tpl_array.H>

using namespace Aleph;
using FFTD = FFT<double>;
using Complex = FFTD::Complex;

// FFT de una senal real
Array<double> signal = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
auto spectrum = FFTD::spectrum(signal);  // Array<Complex>

// Inversa: volver al dominio del tiempo
auto recovered = FFTD::inverse_transform_real(spectrum);
// recovered ≈ signal (con error de punto flotante)

// FFT compacta para senales reales (solo N/2+1 bins, sin redundancia)
auto compact = FFTD::rfft(signal);              // Array<Complex>, tamano N/2+1
auto restored = FFTD::irfft(compact, signal.size());  // reconstruye N muestras
```

### 3.2 Analisis espectral

```cpp
auto magnitudes = FFTD::magnitude_spectrum(spectrum);  // |X[k]|
auto powers     = FFTD::power_spectrum(spectrum);      // |X[k]|^2
auto phases     = FFTD::phase_spectrum(spectrum);      // arg(X[k])

// Aplicar ventana antes de la FFT (reduce spectral leakage)
auto window = FFTD::hann_window(signal.size());
auto windowed = FFTD::windowed_spectrum(signal, window);
```

### 3.3 Multiplicacion de polinomios / convolucion

```cpp
// Polinomios reales: p(x) = 1 + 2x + 3x^2,  q(x) = 4 + 5x
Array<double> p = {1.0, 2.0, 3.0};
Array<double> q = {4.0, 5.0};
auto product = FFTD::multiply(p, q);
// product = {4.0, 13.0, 22.0, 15.0}  -->  4 + 13x + 22x^2 + 15x^3

// Polinomios complejos
Array<Complex> a = {Complex(1,0), Complex(2,-1)};
Array<Complex> b = {Complex(3,0), Complex(0,1)};
auto c = FFTD::multiply(a, b);
```

### 3.4 Contenedores genericos

La API acepta cualquier contenedor iterable, no solo `Array`:

```cpp
std::vector<double> vec_signal = {1.0, 2.0, 3.0, 4.0};
auto vec_spectrum = FFTD::transform(vec_signal);  // retorna Array<Complex>
```

---

## 4. Funciones ventana

Las ventanas reducen el fenomeno de *spectral leakage* al analizar
senales que no son periodicas en la ventana de observacion.

```cpp
auto hann     = FFTD::hann_window(256);
auto hamming  = FFTD::hamming_window(256);
auto blackman = FFTD::blackman_window(256);
auto kaiser   = FFTD::kaiser_window(256, 8.0);  // beta = 8

// Calcular beta de Kaiser a partir de atenuacion deseada
double beta = FFTD::kaiser_beta(60.0);  // 60 dB de atenuacion

// Propiedades de la ventana
double cg   = FFTD::window_coherent_gain(hann);
double enbw = FFTD::window_enbw(hann);
```

**Guia de seleccion**:
- **Hann**: Uso general, buen balance resolucion/leakage
- **Hamming**: Lobulos laterales mas bajos que Hann
- **Blackman**: Excelente supresion de lobulos laterales
- **Kaiser**: Parametrizable via beta -- versatil

---

## 5. STFT: Analisis tiempo-frecuencia

La Short-Time Fourier Transform analiza la senal por ventanas
deslizantes, produciendo un **espectrograma** (matriz tiempo x frecuencia).

### 5.1 Analisis y sintesis

```cpp
Array<double> signal = { /* muestras de audio */ };
auto window = FFTD::hann_window(512);

// Analisis
FFTD::STFTOptions opts;
opts.hop_size = 128;       // Avance entre frames
opts.fft_size = 1024;      // Zero-padding para mejor resolucion
opts.centered = true;
opts.pad_end = true;

auto spectrogram = FFTD::stft(signal, window, opts);
// spectrogram: Array<Array<Complex>>
// spectrogram[frame][bin]

// Sintesis (reconstruccion perfecta bajo NOLA)
FFTD::ISTFTOptions iopts;
iopts.hop_size = 128;
iopts.centered = true;
iopts.signal_length = signal.size();

auto reconstructed = FFTD::istft(spectrogram, window, iopts);
// reconstructed ≈ signal
```

### 5.2 Procesamiento frame-a-frame (streaming)

```cpp
FFTD::STFTProcessor analyzer(window, opts);
FFTD::ISTFTProcessor synthesizer(fft_size, window, iopts);

// Alimentar bloques de muestras
auto frames = analyzer.process_block(chunk_of_samples);
// Cada frame es un Array<Complex>

// Aqui puedes modificar los frames (ej: noise gate, pitch shift)

auto output_samples = synthesizer.process_block(frames);

// Al finalizar
auto tail_frames = analyzer.flush();
auto tail_samples = synthesizer.flush();
```

---

## 6. Diseno de filtros FIR

### 6.1 Metodo de la ventana (firwin)

```cpp
double sample_rate = 44100.0;

// Pasa-bajos: corte en 1 kHz, 51 taps, 60 dB de atenuacion
auto lp = FFTD::firwin_lowpass(51, 1000.0, sample_rate, 60.0);

// Pasa-altos
auto hp = FFTD::firwin_highpass(51, 1000.0, sample_rate, 60.0);

// Pasa-banda
auto bp = FFTD::firwin_bandpass(51, 500.0, 2000.0, sample_rate, 60.0);

// Rechaza-banda (notch)
auto bs = FFTD::firwin_bandstop(51, 500.0, 2000.0, sample_rate, 60.0);
```

### 6.2 Diseno avanzado: firls y Remez

```cpp
// firls: minimos cuadrados ponderado
Array<double> bands   = {0.0, 900.0, 1200.0, 4000.0};
Array<double> desired = {1.0, 1.0,   0.0,    0.0};
Array<double> weights = {1.0, 12.0};  // 12x mas peso en stopband
auto firls_taps = FFTD::firls(33, bands, desired, 8000.0, weights);

// Remez (equiripple): minimiza error maximo
auto remez_taps = FFTD::remez(33, bands, desired, 8000.0, weights);
```

### 6.3 Aplicacion del filtro FIR

```cpp
// Convolucion directa via FFT
auto filtered = FFTD::overlap_add_convolution(signal, lp);

// Filtrado de fase cero (sin desfase, no causal)
auto zero_phase = FFTD::filtfilt(signal, lp, 4);
```

---

## 7. Diseno de filtros IIR

Los filtros IIR se disenan como cascadas de secciones biquad
(second-order sections, SOS) para estabilidad numerica.

### 7.1 Familias de filtros

```cpp
double sr = 8000.0;  // sample rate

// Butterworth: respuesta maximamente plana
auto bw = FFTD::butterworth_lowpass(4, 1000.0, sr);

// Chebyshev I: ripple en banda de paso (0.5 dB)
auto ch1 = FFTD::chebyshev1_lowpass(4, 0.5, 1000.0, sr);

// Chebyshev II: ripple en banda de rechazo (40 dB)
auto ch2 = FFTD::chebyshev2_lowpass(4, 40.0, 1000.0, sr);

// Bessel: retardo de grupo maximamente plano
auto bs = FFTD::bessel_lowpass(4, 1000.0, sr);

// Eliptico (Cauer): equiripple en ambas bandas
auto el = FFTD::elliptic_lowpass(4, 1.0, 40.0, 1000.0, sr);
```

Cada familia tiene variantes `_lowpass`, `_highpass`, `_bandpass`, `_bandstop`.

### 7.2 Analisis de la respuesta

```cpp
// Respuesta en frecuencia (512 puntos, media banda)
auto resp = FFTD::freqz(bw, 512, false);
// resp.omega[k]    -- frecuencia angular
// resp.response[k] -- H(omega) complejo

// Polos y ceros
auto poles = FFTD::poles(bw);
auto zeros = FFTD::zeros(bw);

// Estabilidad
bool stable = FFTD::is_stable(bw);
double margin = FFTD::stability_margin(bw);

// Retardo de grupo y fase
auto gd = FFTD::group_delay(bw, 512);
auto pd = FFTD::phase_delay(bw, 512);
```

### 7.3 Aplicacion del filtro IIR

```cpp
// Causal (una pasada)
auto y = FFTD::sosfilt(signal, bw);

// Fase cero (forward-backward)
auto y_zp = FFTD::filtfilt(signal, bw);

// Streaming causal con estado
FFTD::LFilter lf(bw);
auto chunk1_out = lf.filter(chunk1);
auto chunk2_out = lf.filter(chunk2);  // mantiene estado
```

### 7.4 Guia de seleccion de filtro

| Familia | Ventaja | Desventaja |
|---------|---------|------------|
| Butterworth | Sin ripple, respuesta suave | Transicion lenta |
| Chebyshev I | Transicion mas rapida | Ripple en passband |
| Chebyshev II | Passband plano | Ripple en stopband |
| Bessel | Fase lineal (preserva forma) | Transicion muy lenta |
| Eliptico | Transicion mas abrupta | Ripple en ambas bandas |

---

## 8. Convolucion eficiente para senales largas

### 8.1 Overlap-Add

```cpp
Array<double> long_signal = { /* miles de muestras */ };
Array<double> kernel = { /* filtro FIR */ };

// Una sola llamada (elige block_size automaticamente)
auto result = FFTD::overlap_add_convolution(long_signal, kernel);

// Objeto reutilizable (streaming)
FFTD::OverlapAdd convolver(kernel, 1024);  // block_size = 1024
auto out1 = convolver.process_block(chunk1);
auto out2 = convolver.process_block(chunk2);
auto tail = convolver.flush();
convolver.reset();  // listo para otra senal
```

### 8.2 Overlap-Save

```cpp
FFTD::OverlapSave ols(kernel, 256);
auto out = ols.process_block(chunk);
auto rem = ols.flush();
```

### 8.3 Convolucion particionada (baja latencia)

```cpp
// Latencia = partition_size muestras
FFTD::PartitionedConvolver pc(kernel, 64);  // 64 muestras de latencia
auto out = pc.process_block(chunk);
```

### 8.4 Multicanal

```cpp
size_t num_channels = 2;
FFTD::OverlapAddBank bank(num_channels, kernel, 512);

Array<Array<double>> channels = {left_signal, right_signal};
auto filtered = bank.pconvolve(pool, channels);
// filtered[0] = left filtrado, filtered[1] = right filtrado
```

---

## 9. Estimacion espectral

### 9.1 Welch (PSD)

```cpp
auto window = FFTD::hann_window(256);
FFTD::WelchOptions opts;
opts.hop_size = 128;
opts.fft_size = 512;

auto psd = FFTD::welch(signal, window, sample_rate, opts);
// psd.frequency[k] -- Hz
// psd.density[k]   -- V^2/Hz
```

### 9.2 Densidad espectral cruzada

```cpp
auto csd = FFTD::csd(signal_x, signal_y, window, sample_rate, opts);
```

### 9.3 Coherencia

```cpp
auto coh = FFTD::coherence(signal_x, signal_y, window, sample_rate, opts);
// coh.magnitude_squared[k] en [0, 1]
// 1.0 = correlacion perfecta a esa frecuencia
```

---

## 10. Resampleo

```cpp
// Upsample por 3, downsample por 2 (factor 3/2)
auto resampled = FFTD::resample_poly(signal, 3, 2);

// upfirdn: operacion primitiva upsample-filter-downsample
auto result = FFTD::upfirdn(signal, filter_taps, up_factor, down_factor);
```

---

## 11. Concurrencia con ThreadPool

Todos los metodos computacionales tienen variante paralela con prefijo `p`:

```cpp
# include <thread_pool.H>

ThreadPool pool(4);  // 4 hilos

// Transformadas
auto spec = FFTD::ptransform(pool, signal);
auto inv  = FFTD::pinverse_transform(pool, spectrum);

// Convolucion
auto conv = FFTD::pmultiply(pool, signal, kernel);

// STFT
auto sg = FFTD::pstft(pool, signal, window, opts);
auto rec = FFTD::pistft(pool, sg, window, iopts);

// Lotes de FFTs
FFTD::Plan plan(n);
auto batch = plan.ptransformed_batch(pool, signals, false);
```

---

## 12. Plan reutilizable

Cuando se transforman multiples senales del mismo tamano, un `Plan`
precomputa las tablas de twiddle factors una sola vez:

```cpp
FFTD::Plan plan(1024);  // precomputa para N=1024

// Transformadas individuales
auto X = plan.transformed(signal);
auto x = plan.inverse_transform(X);

// Lotes
Array<Array<Complex>> batch = { sig1, sig2, sig3 };
auto results = plan.transformed_batch(batch, false);

// FFT real compacta
auto R = plan.rfft(real_signal);
auto s = plan.irfft(R, 1024);
```

---

## 13. FFT N-dimensional

```cpp
// 2-D (filas de una "imagen" compleja)
Array<Array<Complex>> image = { row0, row1, row2, row3 };
auto spectrum_2d = FFTD::transformed2d(image);
auto recovered_2d = FFTD::transformed2d(spectrum_2d, true);  // inversa

// 3-D
Array<Array<Array<Complex>>> volume = { /* ... */ };
auto spectrum_3d = FFTD::transformed3d(volume);
```

---

## 14. Raices de polinomios

```cpp
// Encontrar raices de p(x) = 1 + 2x + x^2 = (1+x)^2
Array<double> coeffs = {1.0, 2.0, 1.0};  // coef[i] = coef de x^i
auto roots = FFTD::polynomial_roots(coeffs);
// roots ≈ {(-1, 0), (-1, 0)}
```

---

## 15. Control de SIMD

El backend SIMD se selecciona via variable de entorno:

```bash
# Forzar escalar (sin SIMD)
ALEPH_FFT_SIMD=scalar ./mi_programa

# Forzar AVX2
ALEPH_FFT_SIMD=avx2 ./mi_programa

# Automatico (default): usa el mejor disponible
ALEPH_FFT_SIMD=auto ./mi_programa
```

Consulta programatica:

```cpp
auto backend = FFTD::simd_backend_name();       // "scalar", "avx2", "neon"
bool avx2_ok = FFTD::avx2_dispatch_available();
```

---

## 16. Ejemplo completo: analizador de frecuencia

```cpp
# include <cmath>
# include <numbers>
# include <iostream>
# include <fft.H>
# include <tpl_array.H>

using namespace Aleph;
using FFTD = FFT<double>;

int main()
{
  constexpr double sample_rate = 8000.0;
  constexpr size_t N = 256;

  // Generar senal: 440 Hz + 1000 Hz
  Array<double> signal;
  signal.reserve(N);
  for (size_t i = 0; i < N; ++i)
    {
      double t = static_cast<double>(i) / sample_rate;
      signal.append(std::sin(2.0 * std::numbers::pi * 440.0 * t)
                    + 0.5 * std::sin(2.0 * std::numbers::pi * 1000.0 * t));
    }

  // Ventana + FFT
  auto window = FFTD::hann_window(N);
  auto spectrum = FFTD::windowed_spectrum(signal, window);
  auto magnitudes = FFTD::magnitude_spectrum(spectrum);

  // Encontrar picos
  double freq_resolution = sample_rate / static_cast<double>(N);
  for (size_t k = 1; k < N / 2; ++k)
    if (magnitudes[k] > magnitudes[k-1] and magnitudes[k] > magnitudes[k+1]
        and magnitudes[k] > 5.0)
      std::cout << "Pico en " << k * freq_resolution << " Hz, "
                << "magnitud = " << magnitudes[k] << "\n";

  // Filtrar: mantener solo < 800 Hz
  auto lp = FFTD::butterworth_lowpass(4, 800.0, sample_rate);
  auto filtered = FFTD::sosfilt(signal, lp);

  // Verificar que el pico de 1000 Hz desaparecio
  auto filtered_spec = FFTD::windowed_spectrum(filtered, window);
  auto filtered_mags = FFTD::magnitude_spectrum(filtered_spec);

  std::cout << "Magnitud a 440 Hz tras filtrar: "
            << filtered_mags[static_cast<size_t>(440.0 / freq_resolution)]
            << "\n";
  std::cout << "Magnitud a 1000 Hz tras filtrar: "
            << filtered_mags[static_cast<size_t>(1000.0 / freq_resolution)]
            << "\n";

  return 0;
}
```

---

## 17. Referencia rapida

| Que quieres hacer? | Metodo |
|--------------------|--------|
| FFT de senal real | `FFTD::spectrum(signal)` o `FFTD::rfft(signal)` |
| FFT inversa | `FFTD::inverse_transform(X)` o `FFTD::irfft(R, N)` |
| Convolucion / multiplicacion | `FFTD::multiply(a, b)` |
| Espectrograma | `FFTD::stft(signal, window, opts)` |
| Filtro FIR | `FFTD::firwin_lowpass(taps, fc, sr, att)` |
| Filtro IIR | `FFTD::butterworth_lowpass(order, fc, sr)` |
| Aplicar filtro IIR | `FFTD::sosfilt(signal, sections)` |
| Fase cero | `FFTD::filtfilt(signal, filter)` |
| PSD (Welch) | `FFTD::welch(signal, window, sr, opts)` |
| Resampleo | `FFTD::resample_poly(signal, up, down)` |
| Respuesta en frecuencia | `FFTD::freqz(filter, npoints)` |
| Raices de polinomio | `FFTD::polynomial_roots(coeffs)` |
| Streaming FIR | `FFTD::OverlapAdd(kernel, block_size)` |
| Streaming IIR | `FFTD::LFilter(biquads)` |
| FFT paralela | `FFTD::ptransform(pool, signal)` |
| FFT 2-D | `FFTD::transformed2d(matrix)` |