//-------------------------------------------------------------------------------
// Copyright (c) 2013 Mathias Michel.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Mathias Michel
//     Peter Weidenkaff
//-------------------------------------------------------------------------------

//! Angular distribution based on WignerD functions
/*!
 * @file AmpWignerD.hpp
 *\class AmpWignerD
 *The helicity angle for sub system \_subSys is calculated and the value of the
 *WignerD function is returned
 */

#ifndef AMPWIGNER_D
#define AMPWIGNER_D

#include <vector>
#include <memory>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/exceptions.hpp>

#include "Core/ParameterList.hpp"
#include "Core/Functions.hpp"
#include "Core/DataPoint.hpp"
#include "Core/FunctionTree.hpp"
#include "Core/Spin.hpp"
#include "Core/PhysConst.hpp"

namespace ComPWA {
namespace Physics {
namespace HelicityFormalism {

class AmpWignerD {
public:
  AmpWignerD(ComPWA::Spin spin = ComPWA::Spin(0), unsigned int mu = 0,
             unsigned int muPrime = 0);

  virtual ~AmpWignerD(){};

  virtual double Evaluate(const ComPWA::dataPoint &point, int pos1,
                          int pos2) const;

  static double dynamicalFunction(ComPWA::Spin J, ComPWA::Spin mu,
                                  ComPWA::Spin muPrime, double cosTheta);

  static std::complex<double> dynamicalFunction(double cosAlpha, double cosBeta,
                                                double cosGamma, ComPWA::Spin J,
                                                ComPWA::Spin mu,
                                                ComPWA::Spin muPrime);

  virtual std::shared_ptr<ComPWA::FunctionTree>
  GetTree(ComPWA::ParameterList &sample, int posTheta, int posPhi,
          std::string suffix = "");

  /**
   Factory for AmpWignerD

   @param pt Configuration tree
   @return Constructed object
   */
  static std::shared_ptr<AmpWignerD>
  Factory(const boost::property_tree::ptree &pt) {
    LOG(trace) << "AmpWignerD::Factory() | Construction....";
    auto obj = std::make_shared<AmpWignerD>();

    auto decayParticle = pt.get_child("DecayParticle");

    std::string name = pt.get<std::string>("DecayParticle.<xmlattr>.Name");
    ComPWA::Spin J = PhysConst::Instance()->FindParticle(name).GetSpin();
    obj->SetSpin(J);
    ComPWA::Spin mu(pt.get<double>("DecayParticle.<xmlattr>.Helicity"));
    obj->SetMu(mu);

    auto decayProducts = pt.get_child("DecayProducts");
    std::vector<ComPWA::Spin> vHelicity;
    for (auto i : decayProducts) {
      vHelicity.push_back(
          ComPWA::Spin(i.second.get<double>("<xmlattr>.Helicity")));
    }

    if (vHelicity.size() != 2)
      throw boost::property_tree::ptree_error(
          "AmpWignerD::Factory() | Expect exactly two decay products (" +
          std::to_string(decayProducts.size()) + " given)!");

    obj->SetMuPrime(vHelicity.at(0) - vHelicity.at(1));

    return obj;
  }

  void SetSpin(ComPWA::Spin s) { _spin = s; }
  ComPWA::Spin GetSpin() const { return _spin; }

  void SetMu(ComPWA::Spin s) { _mu = s; }
  ComPWA::Spin GetMu() const { return _mu; }

  void SetMuPrime(ComPWA::Spin s) {
    _helicities = std::pair<ComPWA::Spin, ComPWA::Spin>(0, s);
  }
  ComPWA::Spin GetMuPrime() const {
    return _helicities.first - _helicities.second;
  }

  void SetHelicities(std::pair<ComPWA::Spin, ComPWA::Spin> hel) {
    _helicities = hel;
  }
  
  std::pair<ComPWA::Spin, ComPWA::Spin> GetHelicities() const {
    return _helicities;
  }

protected:
  ComPWA::Spin _spin;
  ComPWA::Spin _mu;
  std::pair<ComPWA::Spin, ComPWA::Spin> _helicities;
};

class WignerDStrategy : public Strategy {
public:
  WignerDStrategy(const std::string resonanceName)
      : Strategy(ParType::MDOUBLE), name(resonanceName) {}

  virtual const std::string to_str() const { return ("WignerD of " + name); }

  virtual bool execute(ParameterList &paras,
                       std::shared_ptr<AbsParameter> &out);

protected:
  std::string name;
};

} /* namespace AmplitudeSum */
} /* namespace Physics */
} /* namespace ComPWA */

#endif
