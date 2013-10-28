//-------------------------------------------------------------------------------
// Copyright (c) 2013 Mathias Michel.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Mathias Michel - initial API and implementation
//-------------------------------------------------------------------------------
//! Physics Module with simple 1D Breit-Wigner.
/*! \class BreitWigner
 * @file BreitWigner.hpp
 * This class provides a simple Breit-Wigner calculation with given parameters.
 * It fulfills the Amplitude interface to be compatible with other ComPWA modules.
*/

#ifndef _PIFBW_HPP
#define _PIFBW_HPP

#include <vector>
#include <memory>
#include "Physics/Amplitude.hpp"
#include "Core/ParameterList.hpp"

class BreitWigner : public Amplitude {

public:
  /// Default Constructor (0x0)
  BreitWigner(const double min, const double max);

  //For normalization
  virtual const double integral(ParameterList& par);
  virtual const double volume();
  virtual const double drawInt(double *x, double *p); //For easy usage in a root TF1
  virtual const ParameterList intensity(double x, double M, double T);
  virtual const ParameterList intensity(std::vector<double>& x, ParameterList& par);
  virtual const bool fillStartParVec(ParameterList& outPar);

  /** Destructor */
  virtual ~BreitWigner();

protected:
  double min_;
  double max_;

 private:
  const double BreitWignerValue(double x, double M, double T);

};

#endif /* _PIFBW_HPP */