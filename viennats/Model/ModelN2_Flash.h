#ifndef MODELN2_FLASH_H_
#define MODELN2_FLASH_H_

#include "../Statistics.h"
#include "../message.h"
#include <boost/spirit/include/classic.hpp>
#include <cassert>
#include <stack>
#include <string>

namespace model {

class N2_FLASH {
  double StartDirection[3];
  double temperature;
  double pressure;
  double flux;

  double const stickingCoeff = 5e-9; // just test different values
  double Flux_ev = 1e16;             // evaporation flux of SiN from surface
  double const rho_CF = 1.27e22;     // particles/cm^3
  double const rho_N2 = 2.69e19;
  double const kB_over_m_N2 = 593.878;

public:
  static const int CoverageStorageSize = 1;
  static const int RatesStorageSize = 2;

  static const unsigned int NumberOfParticleTypes = 1;
  unsigned int NumberOfParticleClusters[1];

  static const bool OutputFluxes = false;
  static const bool SpatiallyEqualDistributedFlux = true;

  static const bool CalculateVisibilities = false;
  static const bool CalculateConnectivities = false;

  static const bool CalculateNormalVectors = true;

  static const bool ReemissionIsMaterialDependent = true;

  class ParticleType {
  public:
    double Direction[3];
    double Probability;
    double Energy;
    double Flux;
    int Type;
  };

  N2_FLASH(const std::string &Parameters) {
    using namespace boost::spirit::classic;

    double Accuracy;

    bool b = parse(Parameters.begin(), Parameters.end(),
                   *((str_p("direction") >> '=' >> '{' >>
                      real_p[assign_a(StartDirection[0])] >> "," >>
                      real_p[assign_a(StartDirection[1])] >> "," >>
                      real_p[assign_a(StartDirection[2])] >> '}' >> ';') |
                     (str_p("statistical_accuracy") >> '=' >>
                      real_p[assign_a(Accuracy)] >> ';') |
                     (str_p("flux") >> '=' >> real_p[assign_a(flux)] >> ';') |
                     (str_p("pressure") >> '=' >> real_p[assign_a(pressure)] >>
                      ';') |
                     (str_p("temperature") >> '=' >>
                      real_p[assign_a(temperature)] >> ';')),
                   space_p | comment_p("//") | comment_p("/*", "*/"))
                 .full;

    if (!b)
      msg::print_error("Failed interpreting process parameters!");

    NumberOfParticleClusters[0] = static_cast<unsigned int>(Accuracy);

    Flux_ev = 5e17 * std::exp(-0.1 / (8.617e-5 * temperature));
  }

  template <class VecType>
  void CalculateVelocity(double &Velocity, const VecType &NormalVector,
                         const double *Coverages, const double *Rates,
                         int Material, bool Connected, bool Visible) const {
    if (Material == 0) {
      Velocity = -Rates[0] * Coverages[0] / (rho_CF * 100); // m/s
    } else {
      Velocity = 0;
    }
  }

  template <class VecType>
  void CalculateVectorVelocity(double *Velocity, const VecType &NormalVector,
                               const double *Coverages, const double *Rates,
                               int Material, bool connected,
                               bool visible) const {}

  static void UpdateCoverage(double *Coverages, const double *Rates) {
    Coverages[0] = Rates[0] / (Rates[0] + Rates[1]);
  }

  template <class PT>
  void ParticleGeneration(PT &p, int ParticleType, double ProcessTime,
                          double *Position) const {
    p.Type = ParticleType;
    my::stat::Cosine1DistributedRandomDirection(StartDirection, p.Direction);
    p.Energy = 1.;
    p.Probability = 1.;
    p.Flux = 0.25 * rho_N2 *
             std::sqrt(3 * kB_over_m_N2 *
                       temperature); // flux just by thermal energy TODO: add
                                     // pressure dependent rho_N2
  }

  template <class PT, class NormVecType>
  void ParticleCollision(const PT &p, const NormVecType &NormalVector,
                         double *Rates, const double *Coverages,
                         double RelTime) const {
    Rates[0] += p.Flux * stickingCoeff;
    Rates[1] = Flux_ev;
  }

  template <class PT, class VecType>
  void ParticleReflexion(const PT &p, std::stack<PT> &particle_stack,
                         const VecType &NormalVector, const double *Coverages,
                         int Material) const {}
};
} // namespace model

#endif
