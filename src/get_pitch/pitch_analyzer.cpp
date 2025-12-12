/// @file

#include <iostream>
#include <math.h>
#include "pitch_analyzer.h"
#include <fstream>

using namespace std;

/// Name space of UPC
namespace upc {

  // Podemos usar este valor para el incremento temporal si queremos sacar tiempo en los .dat
  // (coincide con FRAME_SHIFT = 0.015 s de get_pitch.cpp)
  static const float FRAME_SHIFT_SEC = 0.015f;

  void PitchAnalyzer::autocorrelation(const vector<float> &x, vector<float> &r) const {

    for (unsigned int l = 0; l < r.size(); ++l) {
  		/// \TODO Compute the autocorrelation r[l]
      r[l] = 0.0F;
      for (unsigned int n = 0; n < x.size()-l ; ++n){
        r[l] += x[n] * x[n+l];
      }
      /** 
       * \DONE Autocorrelación calculada 
       * \f[
       * r[l] = \sum_{n=0}^{N-l} x[n] x[n+l]
       * \f]
       * - Inicializamos la autocorrelación a 0
       * - Sumamos multiplicación de la señal natural con la señal desplazada
       * */
    }

    if (r[0] == 0.0F) //to avoid log() and divide zero 
      r[0] = 1e-10; 
  }

  void PitchAnalyzer::set_window(Window win_type) {
    if (frameLen == 0)
      return;

    window.resize(frameLen);

    switch (win_type) {
    case HAMMING:
      /// \TODO Implement the Hamming window
      // *** IMPLEMENTACIÓN SENCILLA DE HAMMING ***
      // w[n] = 0.54 - 0.46 cos(2*pi*n/(N-1))
      for (unsigned int n = 0; n < frameLen; ++n) {
        window[n] = 0.54f - 0.46f * cosf(2.0f * M_PI * n / (frameLen - 1));
      }
      break;

    case RECT:
    default:
      window.assign(frameLen, 1.0f);
    }
  }

  void PitchAnalyzer::set_f0_range(float min_F0, float max_F0) {
    npitch_min = (unsigned int) samplingFreq/max_F0;
    if (npitch_min < 2)
      npitch_min = 2;  // samplingFreq/2

    npitch_max = 1 + (unsigned int) samplingFreq/min_F0;

    //frameLen should include at least 2*T0
    if (npitch_max > frameLen/2)
      npitch_max = frameLen/2;
  }

  bool PitchAnalyzer::unvoiced(float pot, float r1norm, float rmaxnorm) const {
    /// \TODO Implement a rule to decide whether the sound is voiced or not.
    /// * You can use the standard features (pot, r1norm, rmaxnorm),
    ///   or compute and use other ones.

    // Regla sencilla (nivel estudiante):
    //  - Potencia muy baja  -> sordo
    //  - r1norm muy pequeño -> sordo
    //  - rmaxnorm pequeño   -> sordo

    // Potencia (en dB) muy baja = sordo
    if (pot < -20.0F) {
      return true;
    }

    // Primera autocorrelación normalizada muy baja = sordo
    if (r1norm < 0.4F) {
      return true;
    }

    // Máximo secundario pequeño = sordo
    if (rmaxnorm < this->umaxnorm) {
      return true;
    }

    return false;
  }

  float PitchAnalyzer::compute_pitch(vector<float> & x) const {
    if (x.size() != frameLen)
      return -1.0F;

    //Window input frame
    for (unsigned int i=0; i<x.size(); ++i)
      x[i] *= window[i];

    vector<float> r(npitch_max);

    //Compute correlation
    autocorrelation(x, r);

    vector<float>::const_iterator iR = r.begin(), iRMax = iR + npitch_min;

    /// \TODO 
	  /// Find the lag of the maximum value of the autocorrelation away from the origin.<br>
	  /// Choices to set the minimum value of the lag are:
	  ///    - The first negative value of the autocorrelation.
	  ///    - The lag corresponding to the maximum value of the pitch.
    ///	   .
	  /// In either case, the lag should not exceed that of the minimum value of the pitch.

    // *** BÚSQUEDA DEL MÁXIMO ENTRE npitch_min Y npitch_max (versión sencilla) ***
    iRMax = std::max_element(iR + npitch_min, iR + npitch_max);
    unsigned int lag = iRMax - r.begin();

    float pot = 10.0F * log10f(r[0]);

    float r1norm   = r[1] / r[0];
    float rmaxnorm = r[lag] / r[0];

    //You can print these (and other) features, look at them using wavesurfer
    //Based on that, implement a rule for unvoiced
    //change to #if 1 and compile
#if 1
    // *** SALIDA PARA GENERAR .DAT DIRECTAMENTE A UN FICHERO ***
    // Formato: tiempo  pot(dB)  r1norm  rmaxnorm
    // El tiempo lo aproximamos como n_frame * 0.015 s (15 ms de desplazamiento)

    static float t = 0.0f;

    // Abrimos el fichero solo la primera vez (static)
    static std::ofstream featfile("prueba_feats2.dat");

    if (featfile.good()) {        // ← ESTE if SÍ lleva llaves
        featfile << t << '\t'
                 << pot << '\t'
                 << r1norm << '\t'
                 << rmaxnorm << '\n';
        featfile.flush();         // recomendado
    }                             // ← cierre del if(featfile.good())

    t += FRAME_SHIFT_SEC;

#endif


    
    if (unvoiced(pot, r1norm, rmaxnorm))
      return 0.0F;
    else
      return (float) samplingFreq/(float) lag;
  }
}

