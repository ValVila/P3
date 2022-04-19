/// @file

#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>

#include "wavfile_mono.h"
#include "pitch_analyzer.h"

#include "docopt.h"

#define FRAME_LEN   0.030 /* 30 ms. */
#define FRAME_SHIFT 0.015 /* 15 ms. */

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Estimator 

Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version

Options:
    -m FLOAT, --umaxnorm=FLOAT  umbral autocorrelación a largo plazo [default: 0.5]
    -1 FLOAT, --u1norm=FLOAT  umbral autocorrelación [default: 0.4]
    -p FLOAT, --poth=FLOAT  umbral potencia [default: -16]
    -h, --help  Show this screen
    --version   Show the version of the project

Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the estimation:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0
)";
int MOT(int a, int b, int c) //middle of 3 values
{
    // Checking for b
    if ((a < b && b < c) || (c < b && b < a))
       return b;
 
    // Checking for a
    else if ((b < a && a < c) || (c < a && a < b))
       return a;
 
    else
       return c;
}

int main(int argc, const char *argv[]) {
	/// \TODO 
	///  Modify the program syntax and the call to **docopt()** in order to
	///  add options and arguments to the program.
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},	// array of arguments, without the program name
        true,    // show help if requested
        "2.0");  // version string

	std::string input_wav = args["<input-wav>"].asString();
	std::string output_txt = args["<output-txt>"].asString();
  float umaxnorm = stof(args["--umaxnorm"].asString());
  float u1norm = stof(args["--u1norm"].asString());
  float poth = stof(args["--poth"].asString());
  
  /// \DONE added umaxnorm u1norm and poth
  float max_pot = 0.0;
  float th_cc = 0.0;  //threshold for central-clipping
  // Read input sound file
  unsigned int rate;
  vector<float> x;
  if (readwav_mono(input_wav, rate, x) != 0) {
    cerr << "Error reading input file " << input_wav << " (" << strerror(errno) << ")\n";
    return -2;
  }

  int n_len = rate * FRAME_LEN;
  int n_shift = rate * FRAME_SHIFT;

  // Define analyzer
  PitchAnalyzer analyzer(n_len, rate, umaxnorm, u1norm, poth, PitchAnalyzer::HAMMING, 50, 500);

  /// \TODO
  /// Preprocess the input signal in order to ease pitch estimation. For instance,
  /// central-clipping or low pass filtering may be used.
  
  //Normalizamos i aplicamos central-clipping a la vez de la
  //Primeramente cojemos el valor maximo de potencia para normalizar el señal

  for(long unsigned int i=0; i < x.size(); i++){
    if(x[i] > max_pot){
      max_pot = x[i];
    }
  }
  th_cc= 0.03 * max_pot;
  for(long unsigned int i = 0; i < x.size(); i++){
    x[i] = x[i] / max_pot;
    if(abs(x[i]) < th_cc){
      x[i]=0;
    }
  }
  /// \DONE central-clipping 



  // Iterate for each frame and save values in f0 vector
  vector<float>::iterator iX;
  vector<float> f0;
  for (iX = x.begin(); iX + n_len < x.end(); iX = iX + n_shift) {
    float f = analyzer(iX, iX + n_len);
    f0.push_back(f);
  }

  /// \TODO
  /// Postprocess the estimation in order to supress errors. For instance, a median filter
  /// or time-warping may be used.
  // median filter zero padding
  
  vector<float> median(f0.size(),0);
  vector<float> h0(3,0);
  
  median[0] = MOT(h0[0],h0[1],h0[2]);
  for (long unsigned int i = 1; i <f0.size(); i++){
    h0[0] = f0[i-1];
    h0[1] = f0[i];
    h0[2] = f0[i+1];
    median[i] = MOT(h0[0],h0[1],h0[2]);
  }
  h0[0]=f0[f0.size()-1];
  h0[1]=f0[f0.size()];
  h0[2]=0;

  /// \DONE median filter zero padding
  // Write f0 contour into the output file
  ofstream os(output_txt);
  if (!os.good()) {
    cerr << "Error reading output file " << output_txt << " (" << strerror(errno) << ")\n";
    return -3;
  }

  os << 0 << '\n'; //pitch at t=0
  for (iX = f0.begin(); iX != f0.end(); ++iX) 
    os << *iX << '\n';
  os << 0 << '\n';//pitch at t=Dur

  return 0;
}
