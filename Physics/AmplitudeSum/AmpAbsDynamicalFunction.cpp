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

#include <stdlib.h>
#include "gsl/gsl_monte.h"
#include "gsl/gsl_monte_plain.h"
#include "gsl/gsl_monte_miser.h"
#include "gsl/gsl_monte_vegas.h"

#include "Core/PhysConst.hpp"
#include "Physics/DPKinematics/DalitzKinematics.hpp"
#include "Physics/AmplitudeSum/AmpAbsDynamicalFunction.hpp"

AmpAbsDynamicalFunction::AmpAbsDynamicalFunction( normStyle nS, int calls) :
_ffType(formFactorType::BlattWeisskopf), _nCalls(calls),
_normStyle(nS), _modified(1), _norm(1.0)
{

}


AmpAbsDynamicalFunction::AmpAbsDynamicalFunction(const char *name,
		unsigned int varIdA, unsigned int varIdB,
		std::shared_ptr<DoubleParameter> mag,
		std::shared_ptr<DoubleParameter> phase,
		std::shared_ptr<DoubleParameter> mass,
		Spin spin, Spin m, Spin n,
		std::string mother, std::string particleA, std::string particleB,
		std::shared_ptr<DoubleParameter> mesonR, //  meson radius
		std::shared_ptr<DoubleParameter> motherR, //  mother radius
		formFactorType type,
		int nCalls, normStyle nS) :
						_name(name), _mag(mag), _phase(phase), _mass(mass), _subSys(varIdA),
						_spin(spin),
						_m(m), _n(n), _mesonRadius(mesonR), _motherRadius(motherR), _ffType(type),
						_nCalls(nCalls), _normStyle(nS), _norm(1.0), _modified(1),
						_wignerD(varIdB, spin)
{
	initialize();
}

AmpAbsDynamicalFunction::AmpAbsDynamicalFunction(const char *name,
		unsigned int varIdA, unsigned int varIdB,
		std::shared_ptr<DoubleParameter> mag,
		std::shared_ptr<DoubleParameter> phase,
		std::shared_ptr<DoubleParameter> mass,
		Spin spin, Spin m, Spin n,
		std::string mother, std::string particleA, std::string particleB,
		formFactorType type,
		int nCalls, normStyle nS) :
						_name(name), _mag(mag), _phase(phase), _mass(mass),
						_subSys(varIdA), _spin(spin), _m(m), _n(n),
						_mesonRadius(std::make_shared<DoubleParameter>(name, 1.0)),
						_motherRadius(std::make_shared<DoubleParameter>(name, 1.0)), _ffType(type),
						_nCalls(nCalls), _normStyle(nS), _norm(1.0), _modified(1),
						_wignerD(varIdB, spin)
{
	initialize();
}

std::string AmpAbsDynamicalFunction::to_str() const
{
	std::stringstream str;
	str<<"AmpAbsDynamicalFunction | "<<_name<<" enabled="<<_enable
			<< " nCalls="<<_nCalls << " subSys="<<_subSys
			<<" J="<<_spin<<" ffType="<<_ffType<<std::endl;
	str<<" normStyle="<<_normStyle<< " norm="<<_norm
			<<" modified?"<<_modified<<std::endl;
	str<<"Parameters:"<<std::endl;
	str<<_mag->to_str()<<std::endl;
	str<<_phase->to_str()<<std::endl;
	str<<_mass->to_str()<<std::endl;
	str<<_mesonRadius->to_str()<<std::endl;
	str<<_motherRadius->to_str()<<std::endl;

	return str.str();
}

void AmpAbsDynamicalFunction::Configure(
		boost::property_tree::ptree::value_type const& v,
		ParameterList& list)
{
	boost::property_tree::ptree pt = v.second;

	//Name (mandatory)
	auto tmp_name= pt.get_optional<std::string>("<xmlattr>.name");
	if(!tmp_name)
		tmp_name= pt.get_optional<std::string>("name");
	if(!tmp_name)
		throw BadParameter("AmpAbsDynamicalFunction::Configure() | "
				"No name for resonance specified!");
	_name = tmp_name.get();

	//Enable/Disable resonance (optional)
	auto tmp_enable= pt.get_optional<bool>("<xmlattr>.enable");
	if(!tmp_enable)
		tmp_enable= pt.get_optional<bool>("enable");
	if(!tmp_enable) _enable = 0;
	else _enable = tmp_enable.get();

	//Magnitude (mandatory)
	auto tmp_mag_fix = pt.get<bool>("mag_fix",1);
	auto tmp_mag_min = pt.get<double>("mag_min",0.0);
	auto tmp_mag_max = pt.get<double>("mag_max",5.0);
	auto tmp_mag_name = pt.get_optional<std::string>("mag_name");
	if(!tmp_mag_name){
		auto tmp_mag = pt.get_optional<double>("mag");
		if(!tmp_mag)
			throw BadParameter("AmpAbsDynamicalFunction::Configure() | "
					"mag for "+_name+" not specified!");
		_mag = std::shared_ptr<DoubleParameter>(
				new DoubleParameter(
						"mag_"+_name,tmp_mag.get(),
						tmp_mag_min, tmp_mag_max
				)
		);
		_mag->FixParameter(tmp_mag_fix);
		if(_enable) list.AddParameter(_mag);
		_mag_writeByName = 0;
	} else {
		try{
			_mag = list.GetDoubleParameter(tmp_mag_name.get());
			_mag_writeByName = 1;
		} catch (BadParameter& ex){
			if(!_enable){
				_mag = std::shared_ptr<DoubleParameter>(
						new DoubleParameter(
								tmp_mag_name.get(),0.0)
				);
				_mag_writeByName = 1;
			} else {
				BOOST_LOG_TRIVIAL(error) <<"AmpAbsDynamicalFunction::Configure() | "
						"Requesting parameter "<<tmp_mag_name.get()<<" but"
						" was not found in parameter list. "
						"Quit since parameter is mandatory!";
				throw;
			}
		}
	}

	//Phase (mandatory)
	auto tmp_phase_fix = pt.get<bool>("phase_fix",1);
	auto tmp_phase_min = pt.get<double>("phase_min",-300.0);
	auto tmp_phase_max = pt.get<double>("phase_max",300.0);
	auto tmp_phase_name = pt.get_optional<std::string>("phase_name");
	if(!tmp_phase_name){
		auto tmp_phase= pt.get_optional<double>("phase");
		if(!tmp_phase)
			throw BadParameter("AmpAbsDynamicalFunction::Configure() | "
					"phase for "+_name+" not specified!");
		_phase = std::shared_ptr<DoubleParameter>(
				new DoubleParameter(
						"phase_"+_name,tmp_phase.get(),
						tmp_phase_min, tmp_phase_max
				)
		);
		_phase->FixParameter(tmp_phase_fix);
		if(_enable) list.AddParameter(_phase);
		_phase_writeByName = 0;
	} else {
		try{
			_phase = list.GetDoubleParameter(tmp_phase_name.get());
			_phase_writeByName = 1;
		} catch (BadParameter& ex){
			if(!_enable){
				_phase = std::shared_ptr<DoubleParameter>(
						new DoubleParameter(
								tmp_phase_name.get(),0.0)
				);
				_phase_writeByName = 1;
			} else {
				BOOST_LOG_TRIVIAL(error) <<"AmpAbsDynamicalFunction::Configure() | "
						"Requesting parameter "<<tmp_phase_name.get()<<" but"
						" was not found in parameter list. "
						"Quit since parameter is mandatory!";
				throw;
			}
		}
	}

	//Mass (mandatory)
	auto tmp_mass_fix = pt.get<bool>("mass_fix",1);
	auto tmp_mass_min = pt.get<double>("mass_min",0.0);
	auto tmp_mass_max = pt.get<double>("mass_max",10.0);
	auto tmp_mass_name = pt.get_optional<std::string>("mass_name");
	if(!tmp_mass_name){
		auto tmp_mass = pt.get_optional<double>("mass");
		if(!tmp_mass)
			throw BadParameter("AmpAbsDynamicalFunction::Configure() | "
					"mass for "+_name+" not specified!");
		_mass = std::shared_ptr<DoubleParameter>(
				new DoubleParameter(
						"mass_"+_name,tmp_mass.get(),
						tmp_mass_min, tmp_mass_max
				)
		);
		_mass->FixParameter(tmp_mass_fix);
		if(_enable) list.AddParameter(_mass);
		_mass_writeByName = 0;
	} else {
		try{
			_mass = list.GetDoubleParameter(tmp_mass_name.get());
			_mass_writeByName = 1;
		} catch (BadParameter& ex){
			if(!_enable){
				_mass = std::shared_ptr<DoubleParameter>(
						new DoubleParameter(
								tmp_mass_name.get(),0.0)
				);
				_mass_writeByName = 1;
			} else {
				BOOST_LOG_TRIVIAL(error) <<"AmpAbsDynamicalFunction::Configure() | "
						"Requesting parameter "<<tmp_mass_name.get()<<" but"
						" was not found in parameter list. "
						"Quit since parameter is mandatory!";
				throw;
			}
		}
	}

	//Mother radius (optional)
	auto tmp_motherRadius_fix = pt.get<bool>("motherRadius_fix",1);
	auto tmp_motherRadius_min = pt.get<double>("motherRadius_min",0.);
	auto tmp_motherRadius_max = pt.get<double>("motherRadius_max",10.);
	auto tmp_motherRadius_name = pt.get_optional<std::string>("motherRadius_name");
	if(!tmp_motherRadius_name){
		//mother radius is not a strict requriement
		double tmp_motherRadius= pt.get<double>("motherRadius",1.0);
		_motherRadius = std::shared_ptr<DoubleParameter>(
				new DoubleParameter(
						"motherRadius_"+_name,tmp_motherRadius,
						tmp_motherRadius_min, tmp_motherRadius_max
				)
		);
		_motherRadius->FixParameter(tmp_motherRadius_fix);
		//if(_enable) list.AddParameter(_motherRadius);
		_motherRadius_writeByName = 0;
	} else {
		try{
			_motherRadius = list.GetDoubleParameter(tmp_motherRadius_name.get());
			_motherRadius_writeByName = 1;
		} catch (BadParameter& ex){
			if(!_enable){
				_motherRadius = std::shared_ptr<DoubleParameter>(
						new DoubleParameter(
								tmp_motherRadius_name.get(),0.0)
				);
				_motherRadius_writeByName = 1;
			} else {
				BOOST_LOG_TRIVIAL(error) <<"AmpAbsDynamicalFunction::Configure() | "
						"Requesting parameter "<<tmp_motherRadius_name.get()<<" but"
						" was not found in parameter list. "
						"Continue since parameter is not mandatory!";
			}
		}
		_motherRadius =	std::shared_ptr<DoubleParameter>(
				new DoubleParameter("motherRadius_"+_name,1.0)
		);
	}

	//FormFactor (optional)
	auto tmp_ffType= pt.get<unsigned int>("FormFactorType", 1);
	_ffType = formFactorType(tmp_ffType);

	//Meson radius (mandatory)
	auto tmp_mesonRadius_fix = pt.get<bool>("mesonRadius_fix",1);
	auto tmp_mesonRadius_min = pt.get<double>("mesonRadius_min",0.0);
	auto tmp_mesonRadius_max = pt.get<double>("mesonRadius_max",10.0);
	auto tmp_mesonRadius_name = pt.get_optional<std::string>("mesonRadius_name");
	if(!tmp_mesonRadius_name){
		auto tmp_mesonRadius = pt.get_optional<double>("mesonRadius");
		if(!tmp_mesonRadius)
			throw BadParameter("AmpAbsDynamicalFunction::Configure() | "
					"mesonRadius for "+_name+" not specified!");
		_mesonRadius = std::shared_ptr<DoubleParameter>(
				new DoubleParameter(
						"d_"+_name,tmp_mesonRadius.get(),
						tmp_mesonRadius_min, tmp_mesonRadius_max
				)
		);
		_mesonRadius->FixParameter(tmp_mesonRadius_fix);
		if(_enable) list.AddParameter(_mesonRadius);
		_mesonRadius_writeByName = 0;
	} else {
		try{
			_mesonRadius = list.GetDoubleParameter(tmp_mesonRadius_name.get());
			_mesonRadius_writeByName = 1;
		} catch (BadParameter& ex){
			if(!_enable){
				_mesonRadius = std::shared_ptr<DoubleParameter>(
						new DoubleParameter(
								tmp_mesonRadius_name.get(),0.0)
				);
				_mesonRadius_writeByName = 1;
			} else {
				BOOST_LOG_TRIVIAL(error) <<"AmpAbsDynamicalFunction::Configure() | "
						"Requesting parameter "<<tmp_mesonRadius_name.get()<<" but"
						" was not found in parameter list. "
						"Quit since parameter is mandatory!";
				throw;
			}
		}
	}

	auto tmp_spin = pt.get_optional<int>("spin");
	if(!tmp_spin)
		throw BadParameter("AmpAbsDynamicalFunction::Configure() | "
				"spin for "+_name+" not specified!");
	_spin = Spin(tmp_spin.get());
	//optional parameters
	double tmp_m = pt.get<int>("m",0);
	_m = Spin(tmp_m);
	double tmp_n = pt.get<int>("n",0);
	_n = Spin(tmp_n);

	auto tmp_varIdA = pt.get_optional<int>("varIdA");
	if(!tmp_varIdA)
		throw BadParameter("AmpAbsDynamicalFunction::Configure() | "
				"varIdA for "+_name+" not specified!");
	_subSys = tmp_varIdA.get();

	auto tmp_varIdB = pt.get_optional<int>("varIdB");
	if(!tmp_varIdB)
		throw BadParameter("AmpAbsDynamicalFunction::Configure() | "
				"varIdB for "+_name+" not specified!");

	//Initialize angular distribution
	_wignerD = AmpWigner2(tmp_varIdB.get(),_spin);

	//Read mother name
	auto tmp_nameMother = pt.get_optional<int>("Mother");
	if(!tmp_nameMother) //if no mother is provided we assume the head paricle
		_nameMother = Kinematics::instance()->getMotherName();
	else
		_nameMother = tmp_nameMother.get();

	//Read name1
	auto tmp_name1 = pt.get_optional<int>("ParticleA");
	if(!tmp_name1)
		throw BadParameter("AmpAbsDynamicalFunction::Configure() | "
				"ParticleA for "+_name+" not specified!");
	_name1 = tmp_name1.get();

	//Read name2
	auto tmp_name2 = pt.get_optional<int>("ParticleA");
	if(!tmp_name2)
		throw BadParameter("AmpAbsDynamicalFunction::Configure() | "
				"ParticleA for "+_name+" not specified!");
	_name2 = tmp_name2.get();

	initialize();

	return;
}

void AmpAbsDynamicalFunction::put(boost::property_tree::ptree &pt){
	pt.put("<xmlattr>.name", _name);
	pt.put("<xmlattr>.enable", _enable);
	if(_mag_writeByName){
		pt.put("mag_name", _mag->GetName());
	} else {
		pt.put("mag", _mag->GetValue());
		pt.put("mag_fix", _mag->IsFixed());
		pt.put("mag_min", _mag->GetMinValue());
		pt.put("mag_max", _mag->GetMaxValue());
	}
	if(_phase_writeByName){
		pt.put("phase_name", _phase->GetName());
	} else {
		pt.put("phase", _phase->GetValue());
		pt.put("phase_fix", _phase->IsFixed());
		pt.put("phase_min", _phase->GetMinValue());
		pt.put("phase_max", _phase->GetMaxValue());
	}
	if(_mass_writeByName){
		pt.put("mass_name", _mass->GetName());
	} else {
		pt.put("mass", _mass->GetValue());
		pt.put("mass_fix", _mass->IsFixed());
		pt.put("mass_min", _mass->GetMinValue());
		pt.put("mass_max", _mass->GetMaxValue());
	}
	pt.put("FormFactorType", _ffType);
	if(_mesonRadius_writeByName){
		pt.put("mesonRadius_name", _mesonRadius->GetName());
	} else {
		pt.put("mesonRadius", _mesonRadius->GetValue());
		pt.put("mesonRadius_fix", _mesonRadius->IsFixed());
		pt.put("mesonRadius_min", _mesonRadius->GetMinValue());
		pt.put("mesonRadius_max", _mesonRadius->GetMaxValue());
	}
	pt.put("spin", _spin);
	pt.put("m", _m);
	pt.put("n", _n);
	pt.put("varIdA", GetVarIdA());
	pt.put("varIdB", GetVarIdB());
	if(Kinematics::instance()->getMotherName() != _nameMother)
		pt.put("Mother", _nameMother);
	pt.put("ParticleA", _name1);
	pt.put("ParticleB", _name2);
}

void AmpAbsDynamicalFunction::CheckModified()
{
	if(_mass->GetValue() != tmp_mass){
		SetModified();
		tmp_mass = _mass->GetValue();
	}
}

void AmpAbsDynamicalFunction::initialize()
{
	auto phys = PhysConst::instance();
	try{
		_M=phys->getMass(_nameMother);
	} catch (...) {
		throw BadConfig("AmpAbsDynamicalFunction::initialize() | "
				"Can not obtain mass of mother particle: "+_nameMother);
	}

	try{
		_mass1=phys->getMass(_name1);
	} catch (...) {
		throw BadConfig("AmpAbsDynamicalFunction::initialize() | "
				"Can not obtain mass of daughter 1: "+_name1);
	}
	try{
		_mass2=phys->getMass(_name2);
	} catch (...) {
		throw BadConfig("AmpAbsDynamicalFunction::initialize() | "
				"DCan not obtain mass of daughter 2: "+_name2);
	}
}

AmpAbsDynamicalFunction::~AmpAbsDynamicalFunction()
{
}

std::complex<double> AmpAbsDynamicalFunction::GetCoefficient() const
{
	return std::complex<double>(
			std::fabs(_mag->GetValue())*cos(_phase->GetValue()),
			std::fabs(_mag->GetValue())*sin(_phase->GetValue())
	);
}

std::complex<double> AmpAbsDynamicalFunction::Evaluate(dataPoint& point){
	CheckModified();
	std::complex<double> res = EvaluateAmp(point);
	double ang = EvaluateWignerD(point);
	return (GetCoefficient()*GetNormalization()*res*ang);
}

double evalAmp(double* x, size_t dim, void* param)
{
	/* We need a wrapper here because a eval() is a member function of AmpAbsDynamicalFunction
	 * and can therefore not be referenced. But gsl_monte_function expects a function reference.
	 * As third parameter we pass the reference to the current instance of AmpAbsDynamicalFunction
	 */
	if(dim!=2) return 0;

	auto amp = static_cast<AmpAbsDynamicalFunction*>(param);
	dataPoint point;
	try{
		Kinematics::instance()->FillDataPoint( 0, 1, x[1], x[0], point );
	} catch (BeyondPhsp& ex){
		return 0;
	}

	std::complex<double> res = amp->EvaluateAmp(point);
	//include angular distribution in normalization
	res *= amp->EvaluateWignerD(point);
	return ( std::norm(res) ); //integrate over |F|^2
}

double AmpAbsDynamicalFunction::GetIntegral() const
{
	size_t dim=2;
	double res=0.0, err=0.0;

	DalitzKinematics* kin = dynamic_cast<DalitzKinematics*>(Kinematics::instance());

	auto var1_limit = kin->GetMinMax( 0 );
	auto var2_limit = kin->GetMinMax( 1 );
	double xLimit_low[2] = {var2_limit.first,var1_limit.first};
	double xLimit_high[2] = {var2_limit.second,var1_limit.second};

	gsl_rng_env_setup ();
	const gsl_rng_type *T = gsl_rng_default; //type of random generator
	gsl_rng *r = gsl_rng_alloc(T); //random generator
	gsl_monte_function F = {&evalAmp,dim, const_cast<AmpAbsDynamicalFunction*> (this)};
	//	gsl_monte_function F = {&twoDimGaussian,dim, new int()};//using test function; result should be 1

	/*	Choosing vegas algorithm here, because it is the most accurate:
	 * 		-> 10^5 calls gives (in my example) an accuracy of 0.03%
	 * 		 this should be sufficiency for most applications
	 */
	gsl_monte_vegas_state *s = gsl_monte_vegas_alloc (dim);
	gsl_monte_vegas_integrate (&F, xLimit_low, xLimit_high, 2, _nCalls, r,s,&res, &err);
	gsl_monte_vegas_free(s);
	BOOST_LOG_TRIVIAL(debug)<<"AmpAbsDynamicalFunction::GetIntegral() Integration result for |"
			<<_name<<"|^2: "<<res<<"+-"<<err<<" relAcc [%]: "<<100*err/res;

	return res;
}

double AmpAbsDynamicalFunction::GetNormalization()
{
	if(_norm<0) return 1.0; //normalization is disabled
	//	return _norm; //disable recalculation of normalization
	if(!_modified) return _norm;
	_norm = 1/sqrt(GetIntegral());
	_modified=0;
	return _norm;
}

double eval(double* x, size_t dim, void* param)
{
	/* We need a wrapper here because evaluate() is a member function of AmpAbsDynamicalFunction
	 * and can therefore not be referenced. But gsl_monte_function expects a function reference.
	 * As third parameter we pass the reference to the current instance of AmpAbsDynamicalFunction
	 */
	if(dim!=2) return 0;

	auto amp = static_cast<AmpAbsDynamicalFunction*>(param);
	dataPoint point;
	try{
		Kinematics::instance()->FillDataPoint( 0, 1, x[1], x[0], point );
	} catch (BeyondPhsp& ex){
		return 0;
	}

	std::complex<double> res = amp->EvaluateAmp(point);
	double ang = amp->EvaluateWignerD(point);
	double norm = amp->GetNormalization();
	return ( std::norm(res*ang*norm) ); //integrate over |F|^2
}

double AmpAbsDynamicalFunction::GetTotalIntegral() const
{
	//Save CPU time
	return 1;

	size_t dim=2;
	double res=0.0, err=0.0;

	DalitzKinematics* kin = dynamic_cast<DalitzKinematics*>(Kinematics::instance());

	auto var1_limit = kin->GetMinMax( 0 );
	auto var2_limit = kin->GetMinMax( 1 );
	double xLimit_low[2] = {var2_limit.first,var1_limit.first};
	double xLimit_high[2] = {var2_limit.second,var1_limit.second};

	gsl_rng_env_setup ();
	const gsl_rng_type *T = gsl_rng_default; //type of random generator
	gsl_rng *r = gsl_rng_alloc(T); //random generator
	gsl_monte_function F = {&eval,dim, const_cast<AmpAbsDynamicalFunction*> (this)};

	/*	Choosing vegas algorithm here, because it is the most accurate:
	 * 		-> 10^5 calls gives (in my example) an accuracy of 0.03%
	 * 		 this should be sufficiency for most applications
	 */
	gsl_monte_vegas_state *s = gsl_monte_vegas_alloc (dim);
	gsl_monte_vegas_integrate (&F, xLimit_low, xLimit_high, 2, _nCalls, r,s,&res, &err);
	gsl_monte_vegas_free(s);
	//	BOOST_LOG_TRIVIAL(debug)<<"AmpAbsDynamicalFunction::totalIntegral() | "
	//			<<" Result for |"<<_name<<"|^2: "<<res<<"+-"<<err<<" relAcc [%]: "<<100*err/res;

	return res;
}

std::complex<double> AmpAbsDynamicalFunction::widthToCoupling(
		double mSq, double mR, double width, double ma, double mb,
		double spin, double mesonRadius, formFactorType type)
{
	double sqrtS = sqrt(mSq);
	//calculate gammaA(s_R)
	double ffR = Kinematics::FormFactor(mR,ma,mb,spin,mesonRadius,type);
	std::complex<double> qR = Kinematics::qValue(mR,ma,mb);
	//calculate phsp factor
	std::complex<double> rho = Kinematics::phspFactor(sqrtS,ma,mb);
	std::complex<double> denom = std::pow(qR,spin)*ffR*sqrt(rho);
	std::complex<double> result = std::complex<double>(sqrt(mR*width), 0) / denom;
	return result;
}

std::complex<double> AmpAbsDynamicalFunction::couplingToWidth(
		double mSq, double mR, double g, double ma, double mb,
		double spin, double mesonRadius, formFactorType type)
{
	double sqrtM = sqrt(mSq);
	//calculate gammaA(s_R)
	double ffR = Kinematics::FormFactor(mR,ma,mb,spin,mesonRadius,type);
	std::complex<double> qR = std::pow(Kinematics::qValue(mR,ma,mb),spin);
	std::complex<double> gammaA = ffR*qR;
	//calculate phsp factor
	std::complex<double> rho = Kinematics::phspFactor(sqrtM,ma,mb);
	std::complex<double> result = std::norm(gammaA)*g*g*rho/ mR;
	if(result.real()!=result.real() || result.imag()!=result.imag()){
		std::cout<<"AmpKinematics::couplingToWidth() | NaN! mSq="<<mSq
				<<" mR="<<mR<<" g="<<g<<" ma="<<ma<<" mb="<<mb<<std::endl;
		std::cout<<qR<<" "<<gammaA<<" "<<rho<<" "<<g<<std::endl;
	}
	return result;
}

double twoDimGaussian(double* z, size_t dim, void *param)
{
	if(dim!=2) return 0;
	/* test environment for numeric integration:
	 * 	Calculating integral of normalized gaussian:
	 * 	f(x,y) = A exp( - (x-x0)^2/(2*sigmaX^2) + (y-y0)^2/(2*sigmaY^2)
	 * 	with A=1/(2*pi*sigmaX*sigmaY) this function is normalized to 1
	 */
	double x = z[0]; double y = z[1];
	//mean and width need to be adjusted according to final state kinematics
	double x0=1.1, y0=1.1; //mean
	double sigmaX=0.01, sigmaY=0.01; //width
	double pi = PhysConst::instance()->getConstValue("Pi");

	double result = exp( -(x-x0)*(x-x0)/(2*sigmaX*sigmaX) - (y-y0)*(y-y0)/(2*sigmaY*sigmaY) );
	result/=2*pi*sigmaY*sigmaX;
	return result;
}
