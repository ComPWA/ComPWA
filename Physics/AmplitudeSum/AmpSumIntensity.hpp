//-------------------------------------------------------------------------------
// Copyright (c) 2013 Mathias Michel.
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the GNU Public License v3.0
// which accompanies this distribution, and is available at
// http://www.gnu.org/licenses/gpl.html
//
// Contributors:
//     Mathias Michel - initial API and implementation
//		Peter Weidenkaff - adding flatte type resonance, removing root dependence
//-------------------------------------------------------------------------------
//****************************************************************************
// Wrapper to provide intensity of amplitude sum
//****************************************************************************

#ifndef _AMPSUMINTENSITY_HPP
#define _AMPSUMINTENSITY_HPP

#include <vector>
#include <memory>
#include <map>
#include <string>

#include "Core/Amplitude.hpp"
#include "Core/Parameter.hpp"
#include "Core/ParameterList.hpp"
#include "Core/FunctionTree.hpp"
//#include "Estimator/AmpFcn.cpp"
//#include "Optimizer/Minuit2/MinuitIF.hpp"
//#include "Estimator/MinLogLH/MinLogLH.hpp"

#include "Physics/AmplitudeSum/AmplitudeSetup.hpp"
#include "Physics/AmplitudeSum/AmpRelBreitWignerRes.hpp"
#include "Physics/AmplitudeSum/AmpGausRes.hpp"
#include "Physics/AmplitudeSum/AmpFlatteRes.hpp"
#include "Physics/AmplitudeSum/AmpWigner2.hpp"
#include "Physics/AmplitudeSum/AmpSumOfAmplitudes.hpp"
#include "Physics/DPKinematics/DalitzKinematics.hpp"
#include "Core/Efficiency.hpp"
#include "Core/DataPoint.hpp"
#include "Core/Generator.hpp"

class AmpSumIntensity : public Amplitude {

public:
	enum normStyle {
		none, /*!< no normaliztion between Amplitudes. */
		/*!< all amplitudes are normalized to one.
		 *  The normalization factor is \f$ 1/\sqrt(\int |A|^2)\f$ */
		one,
		/*!<all amplitudes are normalized to the number of entries in dalitz plot.
		 *  The normalization factor is \f$ 1/\sqrt(entries/area * \int |A|^2)\f$*/
		entries
	};
	//! Default Constructor (0x0)
	AmpSumIntensity(const double inM, const double inBr, const double in1,const double in2, const double in3,
			std::string nameM, std::string name1,std::string name2,std::string name3,
			AmplitudeSetup ini, unsigned int entries=9999,
			normStyle ns=none, double dpArea=-999);
	AmpSumIntensity(AmplitudeSetup ini, normStyle ns, std::shared_ptr<Efficiency> eff=std::shared_ptr<Efficiency>(new UnitEfficiency()),
			unsigned int entries=9999, double dpArea=-999);
	AmpSumIntensity(AmplitudeSetup ini, std::shared_ptr<Efficiency> eff=std::shared_ptr<Efficiency>(new UnitEfficiency()),
			unsigned int entries=9999, double dpArea=-999);
	AmpSumIntensity(const AmpSumIntensity& other);

	double evaluate(double x[], size_t dim);
	//! normalization integral for parameters \par
	virtual const double integral(ParameterList& par);
	//! normalization integral
	virtual const double integral();
	//! get maximum value of amplitude with current parameters
	virtual double getMaxVal( std::shared_ptr<Generator> gen);
	//! get maximum value of amplitude with parameters \par
	virtual double getMaxVal(ParameterList& par, std::shared_ptr<Generator> gen);

	virtual std::complex<double> getFirstAmp(dataPoint& point, ParameterList& par);
	virtual std::complex<double> getFirstReso(dataPoint& point, ParameterList& par);
	virtual std::complex<double> getFirstBW(dataPoint& point, ParameterList& par);


	virtual void setNevents(unsigned int n) { _entries=n; };
	virtual unsigned int getNevents() { return _entries; };

	//! setting new parameterList
	virtual void setParameterList(ParameterList& par);
	//! fill ParameterList with copied shared_ptr
	void copyParameterList(ParameterList& par);
	//! evaluate total amplitude using parameters \par at phsp point \point
	virtual const ParameterList& intensity(dataPoint& point, ParameterList& par);
	//! evaluate total amplitude using current set of parameters at phsp point \point. Amplitude is multiplied with efficiency of datapoint.
	virtual const ParameterList& intensity(dataPoint& point);
	//! evaluate total amplitude using current set of parameters at phsp point \point. No efficiency correction.
	virtual const ParameterList& intensityNoEff(dataPoint& point);
	//! evaluate total amplitude using current set of parameters at phsp point \point. Amplitude is multiplied with efficiency of datapoint.
	virtual const ParameterList& intensity(std::vector<double> point, ParameterList& par);
	virtual const double sliceIntensity(dataPoint& dataP, ParameterList& par,std::complex<double>* reso, unsigned int nResos);

	/** Setup physics tree.
	 * the @param thePHSPMasses needs to be toy phsp sample. It is used to normalize the amplitudes.
	 *
	 * @param theMasses data sample
	 * @param toyPhspSample Sample with flat toy phsp events.
	 * @return shared_ptr of physics tree
	 */
	virtual std::shared_ptr<FunctionTree> functionTree(allMasses& theMasses, allMasses& toyPhspSample);
	/** Setup phspTree for normalization of the likelihood.
	 * This method implements an unbinned efficiency approach.
	 *
	 * @param accPhspSample Sample of accepted MC events. Should come from a flat PHSP sample.
	 * @param toyPhspSample Sample with flat toy phsp events.
	 * @return shared_ptr of PHSP tree
	 */
	virtual std::shared_ptr<FunctionTree> phspTree(allMasses& accPhspSample, allMasses& toyPhspSample);
	/** Setup phspTree for normalization of the likelihood.
	 *
	 * @param toyPhspSample Sample with flat toy phsp events. It is assumed that efficiency vector is filled!
	 * @return shared_ptr of PHSP tree
	 */
	virtual std::shared_ptr<FunctionTree> phspTree(allMasses& toyPhspSample);
	//! reset trees
	virtual void resetTree(){
		myTree = std::shared_ptr<FunctionTree>();
		myPhspTree = std::shared_ptr<FunctionTree>();
	}
	//! fill internal parameter list with (start) parameter
	virtual const bool fillStartParVec(ParameterList& outPar);
	//! print overview over all amplitudes
	virtual void printAmps();
	//! get magnitude of resonance \param name
	virtual double getMagnitude(std::string name) {
		unsigned int id=getIdOfResonance(name);
		return rr[namer[id]]->GetValue();
	};
	//! convert resonance \param name to id
	unsigned int getIdOfResonance(std::string name){
		for(unsigned int i=0; i<nAmps; i++)	if(namer[i]==name) return i;
	}
	//! convert resonance \param id to name
	std::string getNameOfResonance(unsigned int id){ return namer[id]; }
	//! get magnitude of resonance \param id
	virtual double getMagnitude(unsigned int id) { return rr[namer[id]]->GetValue(); };
	//! get total integral for resonance \param id
	virtual double getTotalIntegral(unsigned int id) { return totAmp.getTotalIntegral(id); };
	//! get total integral for resonance \param name
	virtual double getTotalIntegral(std::string name) { return totAmp.getTotalIntegral(name); };
	//! get fit fraction for resonance \param name
	virtual double getFraction(std::string name) { return totAmp.getUnormalizedFraction(name)/integral(); };
	//! get fit fraction for resonance \param id
	virtual double getFraction(unsigned int id) { return totAmp.getUnormalizedFraction(id)/integral(); };
	//! print all fit fractions; fitting errors are not available here
	virtual void printFractions();
	/** Calculate partial integral over amplitude
	 *
	 * Currently only integration over m23sq and m13sq is supported
	 * @param var1 first integration variables, choose m23sq or m13sq
	 * @param min1 min of first integration variable
	 * @param max1 min of first integration variable
	 * @param var2 second integration variables, choose m23sq or m13sq
	 * @param min2 min of second integration variable
	 * @param max2 max of second integration variable
	 * @return
	 */
	virtual double getIntValue(std::string var1, double min1, double max1, std::string var2, double min2=0, double max2=0);
	//! \return Number of resonances
	virtual unsigned int getNumberOfResonances() { return totAmp.getNAmps(); }
	//! calculate normalization of resonance \param amp
	double normReso(std::shared_ptr<AmpAbsDynamicalFunction> amp);
	//! Destructor
	virtual ~AmpSumIntensity(){};
	//! Clone function
	virtual AmpSumIntensity* Clone(){
		return (new AmpSumIntensity(*this));
	}

protected:
	void init();
	/**Initialize function tree
	 *
	 * @param theMasses data sample
	 * @param toyPhspSample sample of flat toy MC events for normalization of the resonances
	 * @param opt Which tree should be created? "data" data Tree, "norm" normalization tree
	 * with efficiency corrected toy phsp sample or "normAcc" normalization tree with sample
	 * of accepted flat phsp events
	 */
	void setupTree(allMasses& theMasses, allMasses& toyPhspSample, std::string opt);
	//! calculate maximum value of amplitude with parameters \par
	virtual void calcMaxVal(ParameterList& par ,std::shared_ptr<Generator> gen);
	//! calculate maximum value of amplitude with current parameters
	virtual void calcMaxVal( std::shared_ptr<Generator> gen);

	std::shared_ptr<Efficiency> eff_;
	bool _calcMaxFcnVal;
	bool _calcNorm;
	double _maxFcnVal;
	AmpSumOfAmplitudes totAmp;
	AmplitudeSetup ampSetup;
	std::shared_ptr<FunctionTree> myTree;
	std::shared_ptr<FunctionTree> myPhspTree;
	//	std::shared_ptr<ParameterList> treePar;

	double maxVal;

	normStyle _normStyle;
	unsigned int _entries;
	double _dpArea;
	unsigned int nAmps;

	//Resonance Variables
	std::vector<std::string> namer;
	std::map<std::string,std::shared_ptr<DoubleParameter> > mr;
	std::map<std::string,std::shared_ptr<DoubleParameter> > gr;
	std::map<std::string,std::shared_ptr<DoubleParameter> > rr;
	std::map<std::string,std::shared_ptr<DoubleParameter> > phir;

private:


};

#endif
